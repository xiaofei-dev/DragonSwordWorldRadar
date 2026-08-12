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

} // namespace dsnap
