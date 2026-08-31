#pragma once

#include <dsnap/types.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace dsnap {

inline constexpr auto kActionConfirmationWindow = std::chrono::milliseconds{650};
inline constexpr auto kAutomaticActionRetryDelay = std::chrono::milliseconds{100};
inline constexpr std::uint8_t kMaximumAutomaticActionAttempts = 2;
inline constexpr std::size_t kAutomaticActionQuarantineCapacity = 128;

struct PendingActionEvidence {
    WeakObjectId candidate{};
    MonotonicTime invoked_at{};
    double distance_meters{};
    std::uint8_t attempt_ordinal{1};
};

class PendingActionTracker {
public:
    [[nodiscard]] bool begin(WeakObjectId candidate,
                             MonotonicTime now,
                             double distance_meters,
                             std::uint8_t attempt_ordinal = 1) noexcept {
        if (pending_ || !candidate.valid() || distance_meters < 0.0 ||
            attempt_ordinal == 0 || attempt_ordinal > kMaximumAutomaticActionAttempts) {
            return false;
        }
        pending_ = PendingActionEvidence{candidate, now, distance_meters, attempt_ordinal};
        return true;
    }

    [[nodiscard]] std::optional<PendingActionEvidence> confirm_delete(WeakObjectId candidate) noexcept {
        if (!pending_ || pending_->candidate != candidate) return std::nullopt;
        auto confirmed = pending_;
        pending_.reset();
        return confirmed;
    }

    [[nodiscard]] std::optional<PendingActionEvidence> expire(MonotonicTime now) noexcept {
        if (!pending_ || now - pending_->invoked_at < kActionConfirmationWindow) return std::nullopt;
        auto expired = pending_;
        pending_.reset();
        return expired;
    }

    [[nodiscard]] bool cancel(WeakObjectId candidate) noexcept {
        if (!pending_ || pending_->candidate != candidate) return false;
        pending_.reset();
        return true;
    }

    void reset() noexcept { pending_.reset(); }
    [[nodiscard]] bool pending() const noexcept { return pending_.has_value(); }
    [[nodiscard]] const std::optional<PendingActionEvidence>& evidence() const noexcept {
        return pending_;
    }

private:
    std::optional<PendingActionEvidence> pending_{};
};

enum class AutomaticActionDecision : std::uint8_t {
    Ready,
    InvalidCandidate,
    Pending,
    Cooldown,
    Quarantined,
    FailClosed,
};

struct AutomaticActionAttemptRecord {
    WeakObjectId candidate{};
    MonotonicTime retry_not_before{};
    std::uint8_t timeout_count{};
    bool quarantined{};
};

// Owns the complete per-activation automatic-action lifecycle. A successful
// admission establishes the one global pending action before ProcessEvent is
// called. The game must present the same exact candidate again after a bounded
// cooldown before one retry is admitted. A second unconfirmed timeout
// quarantines that exact scalar weak identity for the rest of the activation;
// record exhaustion fails the whole activation closed.
class AutomaticActionState {
public:
    [[nodiscard]] AutomaticActionDecision inspect(
        WeakObjectId candidate,
        MonotonicTime now,
        double distance_meters) const noexcept {
        if (!candidate.valid() || !std::isfinite(distance_meters) || distance_meters < 0.0) {
            return AutomaticActionDecision::InvalidCandidate;
        }
        if (fail_closed_) return AutomaticActionDecision::FailClosed;
        if (pending_.pending()) return AutomaticActionDecision::Pending;
        const auto* record = find_record(candidate);
        if (record && record->quarantined) return AutomaticActionDecision::Quarantined;
        if (record && now < record->retry_not_before) return AutomaticActionDecision::Cooldown;
        if (record && record->timeout_count >= kMaximumAutomaticActionAttempts) {
            return AutomaticActionDecision::FailClosed;
        }
        return AutomaticActionDecision::Ready;
    }

    [[nodiscard]] AutomaticActionDecision begin(
        WeakObjectId candidate,
        MonotonicTime now,
        double distance_meters) noexcept {
        const auto decision = inspect(candidate, now, distance_meters);
        if (decision != AutomaticActionDecision::Ready) return decision;
        const auto* record = find_record(candidate);
        const auto attempt_ordinal = record
            ? static_cast<std::uint8_t>(record->timeout_count + 1)
            : std::uint8_t{1};
        if (attempt_ordinal > kMaximumAutomaticActionAttempts ||
            !pending_.begin(candidate, now, distance_meters, attempt_ordinal)) {
            return AutomaticActionDecision::FailClosed;
        }
        return AutomaticActionDecision::Ready;
    }

    [[nodiscard]] std::optional<PendingActionEvidence> confirm(WeakObjectId candidate) noexcept {
        auto confirmed = pending_.confirm_delete(candidate);
        if (confirmed) remove_record(candidate);
        return confirmed;
    }

    [[nodiscard]] std::optional<PendingActionEvidence> expire(MonotonicTime now) noexcept {
        auto expired = pending_.expire(now);
        if (!expired) return std::nullopt;
        auto* record = find_record(expired->candidate);
        if (!record) record = append_record(expired->candidate);
        if (!record) {
            fail_closed_ = true;
            return expired;
        }
        record->timeout_count = expired->attempt_ordinal;
        if (expired->attempt_ordinal >= kMaximumAutomaticActionAttempts) {
            if (!record->quarantined) {
                record->quarantined = true;
                ++quarantine_size_;
            }
        } else {
            record->retry_not_before = now + kAutomaticActionRetryDelay;
        }
        return expired;
    }

    [[nodiscard]] bool cancel(WeakObjectId candidate) noexcept {
        return pending_.cancel(candidate);
    }

    [[nodiscard]] bool pending() const noexcept { return pending_.pending(); }

    [[nodiscard]] WeakObjectId pending_candidate() const noexcept {
        return pending_.evidence() ? pending_.evidence()->candidate : WeakObjectId{};
    }

    [[nodiscard]] std::uint8_t pending_attempt() const noexcept {
        return pending_.evidence() ? pending_.evidence()->attempt_ordinal : 0;
    }

    [[nodiscard]] bool quarantined(WeakObjectId candidate) const noexcept {
        const auto* record = find_record(candidate);
        return record && record->quarantined;
    }

    [[nodiscard]] std::size_t quarantine_size() const noexcept { return quarantine_size_; }
    [[nodiscard]] bool fail_closed() const noexcept { return fail_closed_; }

    void reset_activation() noexcept {
        pending_.reset();
        records_ = {};
        record_size_ = 0;
        quarantine_size_ = 0;
        fail_closed_ = false;
    }

private:
    [[nodiscard]] AutomaticActionAttemptRecord* find_record(WeakObjectId candidate) noexcept {
        for (std::size_t index = 0; index < record_size_; ++index) {
            if (records_[index].candidate == candidate) return &records_[index];
        }
        return nullptr;
    }

    [[nodiscard]] const AutomaticActionAttemptRecord* find_record(
        WeakObjectId candidate) const noexcept {
        for (std::size_t index = 0; index < record_size_; ++index) {
            if (records_[index].candidate == candidate) return &records_[index];
        }
        return nullptr;
    }

    [[nodiscard]] AutomaticActionAttemptRecord* append_record(WeakObjectId candidate) noexcept {
        if (record_size_ >= records_.size()) return nullptr;
        records_[record_size_] = AutomaticActionAttemptRecord{candidate};
        return &records_[record_size_++];
    }

    void remove_record(WeakObjectId candidate) noexcept {
        for (std::size_t index = 0; index < record_size_; ++index) {
            if (records_[index].candidate != candidate) continue;
            if (records_[index].quarantined && quarantine_size_ != 0) --quarantine_size_;
            --record_size_;
            if (index != record_size_) records_[index] = records_[record_size_];
            records_[record_size_] = {};
            return;
        }
    }

    PendingActionTracker pending_{};
    std::array<AutomaticActionAttemptRecord, kAutomaticActionQuarantineCapacity> records_{};
    std::size_t record_size_{};
    std::size_t quarantine_size_{};
    bool fail_closed_{};
};

enum class InteractionSource : std::uint8_t { Automatic, Manual };

class DistinctInteractionCaptureLog {
public:
    [[nodiscard]] bool first(InteractionSource source, std::uint8_t key_action) noexcept {
        const auto source_index = source == InteractionSource::Automatic ? 0U : 1U;
        const auto index = source_index * 256U + key_action;
        if (seen_[index]) return false;
        seen_[index] = true;
        return true;
    }

private:
    std::array<bool, 512> seen_{};
};

struct TransientTargetOwnership {
    bool target_object{};
    bool target_component{};
};

[[nodiscard]] constexpr TransientTargetOwnership transient_target_ownership(
    bool target_object_is_null,
    bool target_component_is_null) noexcept {
    return {target_object_is_null, target_component_is_null};
}

[[nodiscard]] constexpr bool should_clear_transient_target(
    bool owned_by_mod,
    bool still_matches_mod_value) noexcept {
    return owned_by_mod && still_matches_mod_value;
}

enum class ExactOneSelectionDecision : std::uint8_t {
    NoEligibleCandidate,
    Ready,
    Ambiguous,
    PreviouslyAttempted,
};

[[nodiscard]] constexpr ExactOneSelectionDecision decide_exact_one_selection(
    std::size_t eligible_count,
    bool sole_candidate_was_previously_attempted) noexcept {
    if (eligible_count == 0) return ExactOneSelectionDecision::NoEligibleCandidate;
    if (eligible_count > 1) return ExactOneSelectionDecision::Ambiguous;
    return sole_candidate_was_previously_attempted
        ? ExactOneSelectionDecision::PreviouslyAttempted
        : ExactOneSelectionDecision::Ready;
}

} // namespace dsnap
