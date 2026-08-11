#pragma once

#include <dsnap/types.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace dsnap {

struct CandidateEntry {
    WeakObjectId id{};
    std::uint64_t epoch{};
    CandidateValidation validation{};
    std::size_t retries{};
    MonotonicTime next_attempt{};
};

class CandidateQueue {
public:
    explicit CandidateQueue(std::size_t capacity, std::size_t max_retries, std::chrono::milliseconds retry_backoff);

    [[nodiscard]] bool register_candidate(WeakObjectId id, std::string_view exact_class_path, std::uint64_t epoch);
    [[nodiscard]] bool update_validation(WeakObjectId id, const CandidateValidation& validation);
    void erase(WeakObjectId id);
    void reset(std::uint64_t new_epoch);

    [[nodiscard]] std::optional<CandidateEntry> next_eligible(
        MonotonicTime now,
        std::uint64_t current_epoch,
        double radius_meters) const;
    void record_result(WeakObjectId id, ActionResult result, MonotonicTime now);

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] std::uint64_t epoch() const noexcept { return epoch_; }

private:
    std::size_t capacity_{};
    std::size_t max_retries_{};
    std::chrono::milliseconds retry_backoff_{};
    std::uint64_t epoch_{};
    std::unordered_map<WeakObjectId, CandidateEntry, WeakObjectIdHash> entries_{};
};

} // namespace dsnap
