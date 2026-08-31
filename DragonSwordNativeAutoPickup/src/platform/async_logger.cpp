#include <dsnap/async_logger.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace dsnap {
namespace {

std::string timestamp_utc(std::int64_t utc_unix_ms) {
    const auto milliseconds = utc_unix_ms % 1000;
    const auto captured = std::chrono::system_clock::time_point{
        std::chrono::milliseconds{utc_unix_ms}};
    const auto time = std::chrono::system_clock::to_time_t(captured);
    std::tm utc{};
    gmtime_s(&utc, &time);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << milliseconds << 'Z';
    return output.str();
}

void append_line(std::ofstream& output,
                 const std::string& event,
                 const std::string& details,
                 std::uint64_t sequence,
                 std::int64_t utc_unix_ms,
                 std::uint64_t session_elapsed_ms,
                 std::uint64_t tick_sequence) {
    output << timestamp_utc(utc_unix_ms) << '\t' << event
           << "\tsequence=" << sequence
           << " utc_unix_ms=" << utc_unix_ms
           << " session_elapsed_ms=" << session_elapsed_ms
           << " tick_sequence=" << tick_sequence;
    if (!details.empty()) {
        output << '\t' << details;
    }
    output << "\r\n";
}

bool ensure_output_open(std::ofstream& output,
                        const std::filesystem::path& path,
                        std::error_code& error) {
    if (output.is_open() && output.good()) return true;
    if (output.is_open()) output.close();
    output.clear();
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;
    output.open(path, std::ios::app | std::ios::binary);
    return output.good();
}

} // namespace

AsyncLogger::AsyncLogger(std::filesystem::path user_log, std::filesystem::path debug_log, std::size_t capacity)
    : user_log_(std::move(user_log)),
      debug_log_(std::move(debug_log)),
      capacity_(capacity),
      session_started_(std::chrono::steady_clock::now()) {}

AsyncLogger::~AsyncLogger() = default;

void AsyncLogger::set_tick_sequence(std::uint64_t tick_sequence) noexcept {
    current_tick_sequence_.store(tick_sequence, std::memory_order_release);
}

void AsyncLogger::write(LogAudience audience, std::string event, std::string details) {
    const auto utc_now = std::chrono::system_clock::now();
    const auto steady_now = std::chrono::steady_clock::now();
    const auto utc_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        utc_now.time_since_epoch()).count();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        steady_now - session_started_).count();
    const auto session_elapsed_ms = elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0;
    const auto tick_sequence = current_tick_sequence_.load(std::memory_order_acquire);
    std::lock_guard lock{mutex_};
    if (queue_.size() >= capacity_) {
        ++dropped_;
        return;
    }
    queue_.push_back(Message{audience, std::move(event), std::move(details),
                             ++next_sequence_, utc_unix_ms,
                             session_elapsed_ms, tick_sequence});
    if (queue_.size() > peak_queue_size_) peak_queue_size_ = queue_.size();
}

std::size_t AsyncLogger::flush(std::size_t max_messages) {
    if (max_messages == 0) {
        return 0;
    }

    std::deque<Message> pending{};
    {
        std::lock_guard lock{mutex_};
        while (!queue_.empty() && pending.size() < max_messages) {
            pending.push_back(std::move(queue_.front()));
            queue_.pop_front();
        }
    }

    return flush_pending(pending);
}

std::size_t AsyncLogger::flush_all() {
    std::deque<Message> pending{};
    {
        std::lock_guard lock{mutex_};
        pending.swap(queue_);
    }

    return flush_pending(pending);
}

std::size_t AsyncLogger::flush_pending(std::deque<Message>& pending) {
    const auto count = pending.size();
    if (count == 0) return 0;

    bool has_user{};
    bool has_debug{};
    for (const auto& message : pending) {
        has_user = has_user || message.audience == LogAudience::User;
        has_debug = has_debug || message.audience == LogAudience::Debug;
    }

    std::error_code error;
    const bool user_ready = !has_user || ensure_output_open(user_output_, user_log_, error);
    error.clear();
    const bool debug_ready = !has_debug || ensure_output_open(debug_output_, debug_log_, error);
    std::size_t failed{};

    for (const auto& message : pending) {
        const bool ready = message.audience == LogAudience::User ? user_ready : debug_ready;
        auto& output = message.audience == LogAudience::User ? user_output_ : debug_output_;
        if (ready) {
            append_line(output, message.event, message.details,
                        message.sequence, message.utc_unix_ms,
                        message.session_elapsed_ms, message.tick_sequence);
        } else {
            ++failed;
        }
    }
    if (has_user && user_ready) user_output_.flush();
    if (has_debug && debug_ready) debug_output_.flush();
    if ((has_user && user_ready && !user_output_.good()) ||
        (has_debug && debug_ready && !debug_output_.good())) {
        for (const auto& message : pending) {
            if ((message.audience == LogAudience::User && !user_output_.good()) ||
                (message.audience == LogAudience::Debug && !debug_output_.good())) {
                ++failed;
            }
        }
    }
    if (failed != 0) {
        std::lock_guard lock{mutex_};
        failed_ += failed;
    }
    return count;
}

std::size_t AsyncLogger::dropped_messages() const noexcept {
    std::lock_guard lock{mutex_};
    return dropped_;
}

std::size_t AsyncLogger::failed_messages() const noexcept {
    std::lock_guard lock{mutex_};
    return failed_;
}

std::size_t AsyncLogger::peak_queue_size() const noexcept {
    std::lock_guard lock{mutex_};
    return peak_queue_size_;
}

} // namespace dsnap
