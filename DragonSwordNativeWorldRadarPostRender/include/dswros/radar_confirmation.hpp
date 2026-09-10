#pragma once

#include <cstdint>

namespace dswros {

enum class RadarConfirmationAction : std::uint8_t {
    None, Endorse, BugReport, RestoreDefaults,
};

struct RadarConfirmationResult {
    bool dismissed{};
    RadarConfirmationAction confirmed{RadarConfirmationAction::None};
};

// This model owns intent only, never settings or website side effects. A fresh
// neutral sample must precede Yes, so a queued/held click cannot confirm the
// dialog which that same gesture just opened. No always wins a simultaneous Yes.
class RadarConfirmation final {
public:
    [[nodiscard]] bool begin(RadarConfirmationAction action) noexcept {
        if (active() || action == RadarConfirmationAction::None
            || action > RadarConfirmationAction::RestoreDefaults) return false;
        pending_ = action;
        armed_ = false;
        return true;
    }
    [[nodiscard]] RadarConfirmationResult sample(bool yes, bool no) noexcept {
        if (!active()) return {};
        if (no) { clear(); return {true, RadarConfirmationAction::None}; }
        if (!armed_) {
            if (!yes) armed_ = true;
            return {};
        }
        if (!yes) return {};
        const auto action = pending_;
        clear(); // Consume before the caller can dispatch/re-enter.
        return {true, action};
    }
    void clear() noexcept { pending_ = RadarConfirmationAction::None; armed_ = false; }
    [[nodiscard]] bool active() const noexcept { return pending_ != RadarConfirmationAction::None; }
    [[nodiscard]] RadarConfirmationAction pending() const noexcept { return pending_; }
private:
    RadarConfirmationAction pending_{RadarConfirmationAction::None};
    bool armed_{};
};

} // namespace dswros
