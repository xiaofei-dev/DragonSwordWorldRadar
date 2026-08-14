#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>

namespace dsnap {

enum class LogAudience {
    User,
    Debug,
};

class AsyncLogger {
public:
    AsyncLogger(std::filesystem::path user_log, std::filesystem::path debug_log, std::size_t capacity = 1024);
    ~AsyncLogger();

    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    void write(LogAudience audience, std::string event, std::string details = {});
    std::size_t flush(std::size_t max_messages = 64);
    std::size_t flush_all();
    [[nodiscard]] std::size_t dropped_messages() const noexcept;

private:
    struct Message {
        LogAudience audience{};
        std::string event{};
        std::string details{};
    };

    std::filesystem::path user_log_{};
    std::filesystem::path debug_log_{};
    std::size_t capacity_{};
    mutable std::mutex mutex_{};
    std::deque<Message> queue_{};
    std::size_t dropped_{};
};

} // namespace dsnap
