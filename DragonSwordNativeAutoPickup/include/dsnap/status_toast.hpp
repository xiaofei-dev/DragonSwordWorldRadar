#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace dsnap {

enum class StatusToastKind : std::uint8_t {
    None,
    Starting,
    Enabled,
    Disabled,
    Unavailable,
};

struct StatusToastFrame {
    StatusToastKind kind{StatusToastKind::None};
    std::uint64_t revision{};
    bool visible{};
    double opacity{};
    double vertical_offset{};
};

// Pure timing/state model for the optional native UMG status card. Gameplay
// code only publishes completed lifecycle outcomes into this object; the
// model never feeds a decision back into pickup selection or input dispatch.
class StatusToastTimeline final {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr auto kStartingMinimum = std::chrono::milliseconds{320};
    static constexpr auto kStartingLifetime = std::chrono::milliseconds{1200};
    static constexpr auto kResultLifetime = std::chrono::milliseconds{1800};
    static constexpr auto kUnavailableLifetime = std::chrono::milliseconds{2200};
    static constexpr auto kFadeIn = std::chrono::milliseconds{120};
    static constexpr auto kFadeOut = std::chrono::milliseconds{260};
    static constexpr double kSlideDistance = 8.0;

    void notify(StatusToastKind kind, Clock::time_point now) noexcept {
        if (kind == StatusToastKind::None) {
            clear();
            return;
        }
        const std::uint64_t revision = ++next_revision_;
        if ((kind == StatusToastKind::Enabled
             || kind == StatusToastKind::Unavailable)
            && active_kind_ == StatusToastKind::Starting
            && now >= active_since_
            && now - active_since_ < kStartingMinimum) {
            pending_kind_ = kind;
            pending_revision_ = revision;
            return;
        }
        activate(kind, revision, now);
    }

    [[nodiscard]] StatusToastFrame frame(Clock::time_point now) noexcept {
        advance_pending(now);
        if (active_kind_ == StatusToastKind::None || now < active_since_) {
            return {};
        }

        const auto lifetime = lifetime_for(active_kind_);
        const auto elapsed = now - active_since_;
        if (elapsed >= lifetime) {
            active_kind_ = StatusToastKind::None;
            active_revision_ = 0;
            return {};
        }

        const auto remaining = lifetime - elapsed;
        double opacity = 1.0;
        if (elapsed < kFadeIn) {
            opacity = eased(std::chrono::duration<double>(elapsed).count()
                / std::chrono::duration<double>(kFadeIn).count());
        } else if (remaining < kFadeOut) {
            opacity = eased(std::chrono::duration<double>(remaining).count()
                / std::chrono::duration<double>(kFadeOut).count());
        }
        opacity = std::clamp(opacity, 0.0, 1.0);
        return {
            active_kind_,
            active_revision_,
            true,
            opacity,
            -kSlideDistance * (1.0 - opacity),
        };
    }

    void clear() noexcept {
        active_kind_ = StatusToastKind::None;
        pending_kind_ = StatusToastKind::None;
        active_revision_ = 0;
        pending_revision_ = 0;
        active_since_ = {};
    }

private:
    [[nodiscard]] static constexpr double eased(double value) noexcept {
        const double bounded = std::clamp(value, 0.0, 1.0);
        return bounded * bounded * (3.0 - 2.0 * bounded);
    }

    [[nodiscard]] static constexpr std::chrono::milliseconds lifetime_for(
        StatusToastKind kind) noexcept {
        switch (kind) {
        case StatusToastKind::Starting:
            return kStartingLifetime;
        case StatusToastKind::Unavailable:
            return kUnavailableLifetime;
        case StatusToastKind::Enabled:
        case StatusToastKind::Disabled:
            return kResultLifetime;
        case StatusToastKind::None:
        default:
            return std::chrono::milliseconds{0};
        }
    }

    void activate(StatusToastKind kind, std::uint64_t revision,
                  Clock::time_point now) noexcept {
        active_kind_ = kind;
        active_revision_ = revision;
        active_since_ = now;
        pending_kind_ = StatusToastKind::None;
        pending_revision_ = 0;
    }

    void advance_pending(Clock::time_point now) noexcept {
        if (active_kind_ != StatusToastKind::Starting
            || pending_kind_ == StatusToastKind::None
            || now < active_since_
            || now - active_since_ < kStartingMinimum) {
            return;
        }
        activate(pending_kind_, pending_revision_, now);
    }

    StatusToastKind active_kind_{StatusToastKind::None};
    StatusToastKind pending_kind_{StatusToastKind::None};
    Clock::time_point active_since_{};
    std::uint64_t next_revision_{};
    std::uint64_t active_revision_{};
    std::uint64_t pending_revision_{};
};

} // namespace dsnap
