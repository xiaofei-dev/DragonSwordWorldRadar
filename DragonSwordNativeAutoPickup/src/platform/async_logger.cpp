#include <dsnap/async_logger.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

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
    : user_log_(std::move(user_log)), debug_log_(std::move(debug_log)), capacity_(capacity),
      worker_([this](std::stop_token stop_token) { run(stop_token); }) {}

AsyncLogger::~AsyncLogger() {
    worker_.request_stop();
    signal_.notify_all();
}

void AsyncLogger::write(LogAudience audience, std::string event, std::string details) {
    {
        std::lock_guard lock{mutex_};
        if (queue_.size() >= capacity_) {
            ++dropped_;
            return;
        }
        queue_.push_back(Message{audience, std::move(event), std::move(details)});
    }
    signal_.notify_one();
}

std::size_t AsyncLogger::dropped_messages() const noexcept {
    std::lock_guard lock{mutex_};
    return dropped_;
}

void AsyncLogger::run(std::stop_token stop_token) {
    while (true) {
        Message message{};
        {
            std::unique_lock lock{mutex_};
            signal_.wait(lock, stop_token, [this] { return !queue_.empty(); });
            if (queue_.empty()) {
                if (stop_token.stop_requested()) {
                    return;
                }
                continue;
            }
            message = std::move(queue_.front());
            queue_.pop_front();
        }
        append_line(message.audience == LogAudience::User ? user_log_ : debug_log_, message.event, message.details);
    }
}

} // namespace dsnap
