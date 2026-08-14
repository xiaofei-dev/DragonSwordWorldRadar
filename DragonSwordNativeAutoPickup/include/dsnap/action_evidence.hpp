#pragma once

#include <dsnap/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace dsnap {

inline constexpr auto kActionConfirmationWindow = std::chrono::milliseconds{1500};

struct PendingActionEvidence {
    WeakObjectId candidate{};
    MonotonicTime invoked_at{};
    double distance_meters{};
};

class PendingActionTracker {
public:
    [[nodiscard]] bool begin(WeakObjectId candidate, MonotonicTime now, double distance_meters) noexcept {
        if (pending_ || !candidate.valid() || distance_meters < 0.0) return false;
        pending_ = PendingActionEvidence{candidate, now, distance_meters};
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

private:
    std::optional<PendingActionEvidence> pending_{};
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
