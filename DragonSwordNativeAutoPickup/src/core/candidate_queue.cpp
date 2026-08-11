#include <dsnap/candidate_queue.hpp>

#include <algorithm>
#include <limits>

namespace dsnap {

CandidateQueue::CandidateQueue(
    std::size_t capacity,
    std::size_t max_retries,
    std::chrono::milliseconds retry_backoff)
    : capacity_(capacity), max_retries_(max_retries), retry_backoff_(retry_backoff) {}

bool CandidateQueue::register_candidate(WeakObjectId id, std::string_view exact_class_path, std::uint64_t epoch) {
    if (!id.valid() || exact_class_path != kAllowedClassPath || epoch != epoch_) {
        return false;
    }
    if (entries_.contains(id)) {
        return true;
    }
    if (entries_.size() >= capacity_) {
        return false;
    }
    entries_.emplace(id, CandidateEntry{.id = id, .epoch = epoch});
    return true;
}

bool CandidateQueue::update_validation(WeakObjectId id, const CandidateValidation& validation) {
    const auto found = entries_.find(id);
    if (found == entries_.end()) {
        return false;
    }
    found->second.validation = validation;
    return true;
}

void CandidateQueue::erase(WeakObjectId id) {
    entries_.erase(id);
}

void CandidateQueue::reset(std::uint64_t new_epoch) {
    entries_.clear();
    epoch_ = new_epoch;
}

std::optional<CandidateEntry> CandidateQueue::next_eligible(
    MonotonicTime now,
    std::uint64_t current_epoch,
    double radius_meters) const {
    const CandidateEntry* best = nullptr;
    double best_distance = std::numeric_limits<double>::max();

    for (const auto& [_, entry] : entries_) {
        if (entry.epoch != current_epoch || now < entry.next_attempt ||
            !entry.validation.eligible(radius_meters)) {
            continue;
        }
        if (entry.validation.distance_meters < best_distance) {
            best = &entry;
            best_distance = entry.validation.distance_meters;
        }
    }

    return best ? std::optional<CandidateEntry>{*best} : std::nullopt;
}

void CandidateQueue::record_result(WeakObjectId id, ActionResult result, MonotonicTime now) {
    const auto found = entries_.find(id);
    if (found == entries_.end()) {
        return;
    }
    if (result == ActionResult::Succeeded || result == ActionResult::Rejected ||
        result == ActionResult::Invalidated) {
        entries_.erase(found);
        return;
    }

    auto& entry = found->second;
    ++entry.retries;
    if (entry.retries > max_retries_) {
        entries_.erase(found);
        return;
    }
    entry.next_attempt = now + retry_backoff_ * static_cast<int>(entry.retries);
}

} // namespace dsnap
