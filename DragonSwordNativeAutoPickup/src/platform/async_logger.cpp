#include <dsnap/async_logger.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace dsnap {
namespace {

std::string timestamp_utc() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &time);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

void append_line(const std::filesystem::path& path, const std::string& event, const std::string& details) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream output{path, std::ios::app | std::ios::binary};
    if (!output) {
        return;
    }
    output << timestamp_utc() << '\t' << event;
    if (!details.empty()) {
        output << '\t' << details;
    }
    output << "\r\n";
}

} // namespace

AsyncLogger::AsyncLogger(std::filesystem::path user_log, std::filesystem::path debug_log, std::size_t capacity)
    : user_log_(std::move(user_log)), debug_log_(std::move(debug_log)), capacity_(capacity) {}

AsyncLogger::~AsyncLogger() = default;

void AsyncLogger::write(LogAudience audience, std::string event, std::string details) {
    std::lock_guard lock{mutex_};
    if (queue_.size() >= capacity_) {
        ++dropped_;
        return;
    }
    queue_.push_back(Message{audience, std::move(event), std::move(details)});
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

    const auto count = pending.size();
    for (const auto& message : pending) {
        append_line(message.audience == LogAudience::User ? user_log_ : debug_log_, message.event, message.details);
    }
    return count;
}

std::size_t AsyncLogger::flush_all() {
    std::deque<Message> pending{};
    {
        std::lock_guard lock{mutex_};
        pending.swap(queue_);
    }

    const auto count = pending.size();
    for (const auto& message : pending) {
        append_line(message.audience == LogAudience::User ? user_log_ : debug_log_, message.event, message.details);
    }
    return count;
}

std::size_t AsyncLogger::dropped_messages() const noexcept {
    std::lock_guard lock{mutex_};
    return dropped_;
}

} // namespace dsnap
