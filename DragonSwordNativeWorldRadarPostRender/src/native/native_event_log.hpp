#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace dsnwr {

inline constexpr std::size_t kNativeEngineTickProfileStageCount = 9U;

struct NativeEngineTickProfileSample final {
    std::uint64_t activation{};
    std::uint64_t epoch{};
    std::array<std::uint64_t, kNativeEngineTickProfileStageCount> elapsed_us{};
    bool area_quest_scan_active{};
    std::uint64_t area_quest_scan_index{};
    std::uint64_t area_quest_catalog_size{};
    bool compact_update_called{};
    bool discovery_called{};
    bool world_map_layering_pending{};
    std::uint64_t bird_egg_active_count{};
};

struct NativeEngineTickProfileMetric final {
    std::uint64_t calls{};
    std::uint64_t total_us{};
    std::uint64_t maximum_us{};
};

struct NativeEngineTickProfileReport final {
    std::uint64_t activation{};
    std::uint64_t epoch{};
    std::uint64_t interval_ms{};
    std::array<NativeEngineTickProfileMetric,
               kNativeEngineTickProfileStageCount> metrics{};
    std::uint64_t slow_ticks{};
    std::uint64_t slow_threshold_us{};
};

[[nodiscard]] bool load_native_event_log_enabled(
    const std::filesystem::path& config_path) noexcept;

// Starts one bounded process-session log with one session header. The previous
// session is retained in a single rollover file and no UObject or game-thread
// identity is stored.
void begin_native_event_log_session(
    const std::filesystem::path& mod_directory, bool enabled) noexcept;

[[nodiscard]] bool native_event_log_enabled() noexcept;

// Enqueues one diagnostic event into a fixed-capacity process-lifetime queue.
// The caller never waits for the writer lock or performs file I/O. Enabled
// events receive capture-time sequence, UTC Unix-millisecond, and
// process-session elapsed-millisecond fields. A dedicated worker serializes
// and flushes records; queue contention or saturation drops the record instead
// of blocking gameplay. Returns true only when the record entered the queue.
bool append_native_event_log(
    std::string_view event, std::string_view detail) noexcept;

// Structured performance records avoid string formatting and allocation on
// the game thread. They are converted to the stable text schema by the writer
// and likewise return true only when enqueued.
bool append_native_engine_tick_slow(
    const NativeEngineTickProfileSample& sample,
    std::uint64_t slow_threshold_us,
    std::uint64_t slow_total) noexcept;

bool append_native_engine_tick_profile(
    const NativeEngineTickProfileReport& report) noexcept;

// Stops the writer after draining the bounded queue and flushes the file. This
// is the shutdown boundary, not a gameplay-thread flush operation.
void flush_native_event_log() noexcept;

} // namespace dsnwr
