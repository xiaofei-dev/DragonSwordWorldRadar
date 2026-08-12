#pragma once

#include <dsnap/types.hpp>

#include <optional>

namespace dsnap {

enum class TargetObservationDecision {
    NoTarget,
    Ready,
    Debounce,
    PreviousCleared,
    PreviousChanged,
};

struct TargetObservation {
    TargetObservationDecision decision{TargetObservationDecision::NoTarget};
    WeakObjectId previous{};
    WeakObjectId current{};
};

class SingleTargetInvocationLatch {
public:
    [[nodiscard]] TargetObservation observe(std::optional<WeakObjectId> current) noexcept {
        if (!current || !current->valid()) {
            if (!pending_.valid()) return {};
            const auto previous = pending_;
            pending_ = {};
            return {TargetObservationDecision::PreviousCleared, previous, {}};
        }
        if (!pending_.valid()) return {TargetObservationDecision::Ready, {}, *current};
        if (pending_ == *current) return {TargetObservationDecision::Debounce, pending_, *current};
        const auto previous = pending_;
        pending_ = {};
        return {TargetObservationDecision::PreviousChanged, previous, *current};
    }

    [[nodiscard]] bool mark_invoked(WeakObjectId target) noexcept {
        if (pending_.valid() || !target.valid()) return false;
        pending_ = target;
        return true;
    }

    void reset() noexcept { pending_ = {}; }
    [[nodiscard]] bool pending() const noexcept { return pending_.valid(); }
    [[nodiscard]] WeakObjectId target() const noexcept { return pending_; }

private:
    WeakObjectId pending_{};
};

} // namespace dsnap
