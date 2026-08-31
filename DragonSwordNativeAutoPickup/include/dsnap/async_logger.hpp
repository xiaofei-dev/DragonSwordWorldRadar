#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
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

    void set_tick_sequence(std::uint64_t tick_sequence) noexcept;
    void write(LogAudience audience, std::string event, std::string details = {});
    std::size_t flush(std::size_t max_messages = 64);
    std::size_t flush_all();
    [[nodiscard]] std::size_t dropped_messages() const noexcept;
    [[nodiscard]] std::size_t failed_messages() const noexcept;
    [[nodiscard]] std::size_t peak_queue_size() const noexcept;

private:
    struct Message {
        LogAudience audience{};
        std::string event{};
        std::string details{};
        std::uint64_t sequence{};
        std::int64_t utc_unix_ms{};
        std::uint64_t session_elapsed_ms{};
        std::uint64_t tick_sequence{};
    };

    std::size_t flush_pending(std::deque<Message>& pending);

    std::filesystem::path user_log_{};
    std::filesystem::path debug_log_{};
    std::size_t capacity_{};
    std::chrono::steady_clock::time_point session_started_{};
    std::atomic<std::uint64_t> current_tick_sequence_{};
    mutable std::mutex mutex_{};
    std::deque<Message> queue_{};
    std::uint64_t next_sequence_{};
    std::size_t dropped_{};
    std::size_t failed_{};
    std::size_t peak_queue_size_{};
    std::ofstream user_output_{};
    std::ofstream debug_output_{};
};

} // namespace dsnap
