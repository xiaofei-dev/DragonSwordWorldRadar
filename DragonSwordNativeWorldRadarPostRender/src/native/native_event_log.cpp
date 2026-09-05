#include "native_event_log.hpp"

#include <dswros/diagnostic_log_format.hpp>
#include <dswros/diagnostics_config.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include <windows.h>

namespace dsnwr {
namespace {

constexpr std::uintmax_t kMaximumLogBytes = 1024U * 1024U;
constexpr std::size_t kBufferedLineLimit = 16U;
constexpr std::size_t kQueueCapacity = 256U;
constexpr std::size_t kWriterBatchLimit = 16U;
constexpr std::size_t kMaximumEventBytes = 96U;
constexpr std::size_t kMaximumDetailBytes = 8192U;
constexpr auto kWriterIdleFlushInterval = std::chrono::milliseconds{250};

enum class RecordKind : std::uint8_t {
    Text,
    EngineTickSlow,
    EngineTickProfile,
};

struct TextRecord final {
    std::array<char, kMaximumEventBytes> event{};
    std::array<char, kMaximumDetailBytes> detail{};
    std::uint16_t event_size{};
    std::uint16_t detail_size{};
};

struct QueuedRecord final {
    RecordKind kind{RecordKind::Text};
    std::uint64_t sequence{};
    std::uint64_t utc_ms{};
    std::uint64_t elapsed_ms{};
    TextRecord text{};
    NativeEngineTickProfileSample slow{};
    NativeEngineTickProfileReport profile{};
    std::uint64_t slow_threshold_us{};
    std::uint64_t slow_total{};
};

struct NativeEventLogState final {
    std::mutex queue_mutex;
    std::condition_variable queue_ready;
    std::array<QueuedRecord, kQueueCapacity> queue{};
    std::size_t queue_head{};
    std::size_t queue_tail{};
    std::size_t queue_size{};
    std::thread writer;
    std::filesystem::path current_path;
    std::filesystem::path previous_path;
    std::ofstream output;
    std::uintmax_t current_bytes{};
    std::size_t buffered_lines{};
    std::uint64_t session_started_tick_ms{};
    std::uint64_t next_sequence{1U};
    std::atomic_uint64_t dropped_records{};
    std::atomic_uint64_t truncated_records{};
    bool initialized{};
    bool stopping{};
    std::atomic_bool enabled{};
};

NativeEventLogState& event_log_state() noexcept {
    // Avoid a joinable std::thread destructor during loader teardown. Normal
    // CppUserMod shutdown drains and joins the process-lifetime writer.
    static auto* state = new NativeEventLogState{};
    return *state;
}

bool requires_immediate_flush(std::string_view event) noexcept {
    return event == "SESSION_BEGIN"
        || event == "START"
        || event == "F7_ACTIVATED"
        || event == "F8_DISABLED"
        || event == "RADAR_ACTIVITY_SUPPRESSION"
        || event.find("FAILED") != std::string_view::npos
        || event.find("FAULT") != std::string_view::npos
        || event.find("DISABLED") != std::string_view::npos
        || event.find("REJECTED") != std::string_view::npos;
}

std::uint64_t utc_unix_time_ms() noexcept {
    FILETIME file_time{};
    GetSystemTimeAsFileTime(&file_time);
    ULARGE_INTEGER value{};
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    constexpr std::uint64_t windows_to_unix_epoch_100ns{
        116444736000000000ULL};
    if (value.QuadPart < windows_to_unix_epoch_100ns) {
        return 0;
    }
    return (value.QuadPart - windows_to_unix_epoch_100ns) / 10000ULL;
}

void close_output(NativeEventLogState& state) noexcept {
    if (state.output.is_open()) {
        state.output.flush();
        state.output.close();
    }
    state.output.clear();
    state.buffered_lines = 0;
}

void disable_output(NativeEventLogState& state) noexcept {
    state.enabled.store(false, std::memory_order_release);
    close_output(state);
    state.initialized = false;
}

void open_current_output(NativeEventLogState& state) noexcept {
    state.output.clear();
    state.output.open(
        state.current_path,
        std::ios::binary | std::ios::app);
    std::error_code error;
    state.current_bytes = std::filesystem::file_size(
        state.current_path, error);
    if (error) {
        state.current_bytes = 0;
    }
}

void open_truncated_output(NativeEventLogState& state) noexcept {
    state.output.clear();
    state.output.open(
        state.current_path,
        std::ios::binary | std::ios::trunc);
    state.current_bytes = 0;
    state.buffered_lines = 0;
}

bool rotate_current_output(NativeEventLogState& state) noexcept;

bool flush_output(NativeEventLogState& state) noexcept {
    if (!state.output.is_open()) {
        return false;
    }
    if (state.buffered_lines == 0U) {
        return true;
    }
    state.output.flush();
    if (!state.output.good()) {
        return false;
    }
    state.buffered_lines = 0;
    return true;
}

bool append_line_locked(
    NativeEventLogState& state,
    std::string_view event,
    std::string_view detail,
    std::uint64_t sequence,
    std::uint64_t utc_ms,
    std::uint64_t elapsed_ms) noexcept {
    std::array<char, dswros::kMaximumDiagnosticLogMetadataBytes> metadata{};
    const auto metadata_bytes = dswros::format_diagnostic_log_metadata(
        metadata, sequence, utc_ms, elapsed_ms);
    if (metadata_bytes == 0U) {
        return false;
    }
    const std::uintmax_t line_bytes = event.size() + metadata_bytes
        + (detail.empty() ? 0U : 1U + detail.size()) + 1U;
    if (state.current_bytes != 0
        && state.current_bytes + line_bytes > kMaximumLogBytes) {
        if (!rotate_current_output(state)) {
            open_truncated_output(state);
        } else {
            open_current_output(state);
        }
        if (!state.output.is_open() || !state.output.good()) {
            return false;
        }
    }
    state.output.write(
        event.data(), static_cast<std::streamsize>(event.size()));
    state.output.write(
        metadata.data(), static_cast<std::streamsize>(metadata_bytes));
    if (!detail.empty()) {
        state.output.put(' ');
        state.output.write(
            detail.data(), static_cast<std::streamsize>(detail.size()));
    }
    state.output.put('\n');
    if (!state.output.good()) {
        return false;
    }
    state.current_bytes += line_bytes;
    ++state.buffered_lines;
    if (requires_immediate_flush(event)
        || state.buffered_lines >= kBufferedLineLimit) {
        return flush_output(state);
    }
    return true;
}

bool rotate_current_output(NativeEventLogState& state) noexcept {
    close_output(state);
    if (state.current_path.empty() || state.previous_path.empty()) {
        return false;
    }
    std::error_code error;
    const bool current_exists = std::filesystem::exists(
        state.current_path, error);
    if (error || !current_exists) {
        return !error;
    }
    if (MoveFileExW(
            state.current_path.c_str(), state.previous_path.c_str(),
            MOVEFILE_REPLACE_EXISTING) == FALSE) {
        return false;
    }
    state.current_bytes = 0;
    return true;
}

template <std::size_t Capacity>
std::string_view bounded_view(
    const std::array<char, Capacity>& value,
    std::uint16_t size) noexcept {
    return {value.data(), std::min<std::size_t>(size, value.size())};
}

template <std::size_t Capacity>
std::uint16_t copy_bounded(
    std::array<char, Capacity>& destination,
    std::string_view source,
    bool* truncated) noexcept {
    const auto copied = std::min(source.size(), destination.size());
    if (copied != 0U) {
        std::memcpy(destination.data(), source.data(), copied);
    }
    if (truncated) {
        *truncated = copied != source.size();
    }
    return static_cast<std::uint16_t>(copied);
}

bool write_queued_record(
    NativeEventLogState& state,
    const QueuedRecord& record) {
    if (record.kind == RecordKind::Text) {
        return append_line_locked(
            state,
            bounded_view(record.text.event, record.text.event_size),
            bounded_view(record.text.detail, record.text.detail_size),
            record.sequence, record.utc_ms, record.elapsed_ms);
    }
    if (record.kind == RecordKind::EngineTickSlow) {
        const auto& sample = record.slow;
        const auto& elapsed = sample.elapsed_us;
        const auto detail = std::format(
            "activation={} epoch={} total_us={} threshold_us={} world_map_layering_us={} area_quest_us={} activity_us={} position_save_us={} compact_umg_us={} encounter_us={} bird_egg_us={} observed_us={} area_scan_active={} area_scan_index={}/{} compact_called={} discovery_called={} world_map_layering_pending={} bird_egg_active={} slow_total={} logger_dropped={} logger_truncated={}",
            sample.activation, sample.epoch, elapsed[0],
            record.slow_threshold_us, elapsed[1], elapsed[2], elapsed[3],
            elapsed[4], elapsed[5], elapsed[6], elapsed[7], elapsed[8],
            sample.area_quest_scan_active, sample.area_quest_scan_index,
            sample.area_quest_catalog_size, sample.compact_update_called,
            sample.discovery_called, sample.world_map_layering_pending,
            sample.bird_egg_active_count, record.slow_total,
            state.dropped_records.load(std::memory_order_relaxed),
            state.truncated_records.load(std::memory_order_relaxed));
        return append_line_locked(
            state, "ENGINE_TICK_SLOW", detail,
            record.sequence, record.utc_ms, record.elapsed_ms);
    }

    const auto& report = record.profile;
    const auto format_metric = [&report](std::size_t index) {
        const auto& metric = report.metrics[index];
        return std::format("{}/{}/{}", metric.calls, metric.total_us,
                           metric.maximum_us);
    };
    const auto detail = std::format(
        "activation={} epoch={} interval_ms={} metric_format=calls_total_us_max_us ticks={} total={} world_map_layering={} area_quest={} activity={} position_save={} compact_umg={} encounter={} bird_egg={} observed={} slow_ticks={} slow_threshold_us={} logger_dropped={} logger_truncated={}",
        report.activation, report.epoch, report.interval_ms,
        report.metrics[0].calls, format_metric(0), format_metric(1),
        format_metric(2), format_metric(3), format_metric(4),
        format_metric(5), format_metric(6), format_metric(7),
        format_metric(8), report.slow_ticks, report.slow_threshold_us,
        state.dropped_records.load(std::memory_order_relaxed),
        state.truncated_records.load(std::memory_order_relaxed));
    return append_line_locked(
        state, "ENGINE_TICK_PROFILE", detail,
        record.sequence, record.utc_ms, record.elapsed_ms);
}

template <typename Fill>
bool try_enqueue(RecordKind kind, Fill&& fill) noexcept {
    auto& state = event_log_state();
    if (!state.enabled.load(std::memory_order_relaxed)) {
        return false;
    }
    const auto captured_utc_ms = utc_unix_time_ms();
    const auto captured_tick_ms = static_cast<std::uint64_t>(GetTickCount64());
    std::unique_lock lock{state.queue_mutex, std::try_to_lock};
    if (!lock.owns_lock()
        || !state.enabled.load(std::memory_order_relaxed)
        || state.stopping || state.queue_size >= state.queue.size()) {
        state.dropped_records.fetch_add(1U, std::memory_order_relaxed);
        return false;
    }
    auto& record = state.queue[state.queue_tail];
    record.kind = kind;
    record.sequence = state.next_sequence++;
    record.utc_ms = captured_utc_ms;
    record.elapsed_ms = captured_tick_ms >= state.session_started_tick_ms
        ? captured_tick_ms - state.session_started_tick_ms
        : 0U;
    fill(record, state);
    state.queue_tail = (state.queue_tail + 1U) % state.queue.size();
    ++state.queue_size;
    lock.unlock();
    state.queue_ready.notify_one();
    return true;
}

void writer_loop(NativeEventLogState* state) noexcept {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    std::array<QueuedRecord, kWriterBatchLimit> batch{};
    try {
        for (;;) {
            std::size_t batch_size{};
            bool idle_flush{};
            {
                std::unique_lock lock{state->queue_mutex};
                const bool ready = state->queue_ready.wait_for(
                    lock, kWriterIdleFlushInterval,
                    [state] {
                        return state->stopping || state->queue_size != 0U;
                    });
                idle_flush = !ready && state->queue_size == 0U;
                while (state->queue_size != 0U
                       && batch_size < batch.size()) {
                    batch[batch_size++] = state->queue[state->queue_head];
                    state->queue_head =
                        (state->queue_head + 1U) % state->queue.size();
                    --state->queue_size;
                }
                if (state->stopping && batch_size == 0U
                    && state->queue_size == 0U) {
                    break;
                }
            }
            if (idle_flush && !flush_output(*state)) {
                disable_output(*state);
                return;
            }
            for (std::size_t index = 0; index < batch_size; ++index) {
                if (!write_queued_record(*state, batch[index])) {
                    disable_output(*state);
                    return;
                }
            }
        }
        if (!flush_output(*state)) {
            disable_output(*state);
        }
    } catch (...) {
        disable_output(*state);
    }
}

} // namespace

bool load_native_event_log_enabled(
    const std::filesystem::path& config_path) noexcept {
    try {
        std::ifstream input{config_path, std::ios::binary};
        if (!input) {
            return false;
        }
        std::string document;
        document.resize(dswros::kMaximumDiagnosticsConfigBytes + 1U);
        input.read(document.data(), static_cast<std::streamsize>(document.size()));
        document.resize(static_cast<std::size_t>(input.gcount()));
        if (document.size() > dswros::kMaximumDiagnosticsConfigBytes
            || input.bad()) {
            return false;
        }
        const auto parsed = dswros::parse_event_log_enabled(document);
        return parsed.value_or(false);
    } catch (...) {
        return false;
    }
}

void begin_native_event_log_session(
    const std::filesystem::path& mod_directory, bool enabled) noexcept {
    flush_native_event_log();
    auto& state = event_log_state();
    state.enabled.store(false, std::memory_order_relaxed);
    if (!enabled) {
        return;
    }
    try {
        const auto directory = mod_directory / "runtime" / "logs";
        std::filesystem::create_directories(directory);
        state.current_path = directory
            / "DragonSwordNativeWorldRadarPostRender.Native.log";
        state.previous_path = directory
            / "DragonSwordNativeWorldRadarPostRender.Native.previous.log";
        if (rotate_current_output(state)) {
            open_current_output(state);
        } else {
            // A viewer can temporarily block the atomic rollover on Windows.
            // Truncate the current file instead of allowing an unbounded log.
            open_truncated_output(state);
        }
        state.initialized = state.output.is_open() && state.output.good();
        if (!state.initialized) {
            disable_output(state);
            return;
        }
        state.session_started_tick_ms = static_cast<std::uint64_t>(
            GetTickCount64());
        state.next_sequence = 1U;
        state.dropped_records.store(0U, std::memory_order_relaxed);
        state.truncated_records.store(0U, std::memory_order_relaxed);
        {
            std::lock_guard lock{state.queue_mutex};
            state.queue_head = 0U;
            state.queue_tail = 0U;
            state.queue_size = 0U;
            state.stopping = false;
        }
        if (!append_line_locked(
                state, "SESSION_BEGIN",
                "log_schema=2 bounded_files=2 max_file_bytes=1048576 writer=fixed_queue_async queue_capacity=256 batch_lines=16 idle_flush_ms=250 drop_on_contention=1",
                state.next_sequence++, utc_unix_time_ms(), 0U)) {
            disable_output(state);
            return;
        }
        state.writer = std::thread{writer_loop, &state};
        state.enabled.store(true, std::memory_order_release);
    } catch (...) {
        disable_output(state);
    }
}

bool native_event_log_enabled() noexcept {
    return event_log_state().enabled.load(std::memory_order_relaxed);
}

bool append_native_event_log(
    std::string_view event, std::string_view detail) noexcept {
    if (!event_log_state().enabled.load(std::memory_order_relaxed)) {
        return false;
    }
    return try_enqueue(RecordKind::Text,
        [event, detail](QueuedRecord& record, NativeEventLogState& state) {
            bool event_truncated{};
            bool detail_truncated{};
            record.text.event_size = copy_bounded(
                record.text.event, event, &event_truncated);
            record.text.detail_size = copy_bounded(
                record.text.detail, detail, &detail_truncated);
            if (event_truncated || detail_truncated) {
                state.truncated_records.fetch_add(
                    1U, std::memory_order_relaxed);
            }
        });
}

bool append_native_engine_tick_slow(
    const NativeEngineTickProfileSample& sample,
    std::uint64_t slow_threshold_us,
    std::uint64_t slow_total) noexcept {
    return try_enqueue(RecordKind::EngineTickSlow,
        [&sample, slow_threshold_us, slow_total](
            QueuedRecord& record, NativeEventLogState&) {
            record.slow = sample;
            record.slow_threshold_us = slow_threshold_us;
            record.slow_total = slow_total;
        });
}

bool append_native_engine_tick_profile(
    const NativeEngineTickProfileReport& report) noexcept {
    return try_enqueue(RecordKind::EngineTickProfile,
        [&report](QueuedRecord& record, NativeEventLogState&) {
            record.profile = report;
        });
}

void flush_native_event_log() noexcept {
    auto& state = event_log_state();
    state.enabled.store(false, std::memory_order_release);
    try {
        if (state.writer.joinable()) {
            {
                std::lock_guard lock{state.queue_mutex};
                state.stopping = true;
            }
            state.queue_ready.notify_one();
            state.writer.join();
        }
        close_output(state);
        state.initialized = false;
    } catch (...) {
        disable_output(state);
    }
}

} // namespace dsnwr
