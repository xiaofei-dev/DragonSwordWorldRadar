#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

namespace dswros {

// All rectangles are expressed in the fixed host-root's local UMG coordinates.
// The renderer obtains them from completed Slate geometries, not map zoom or
// guessed Canvas offsets. The 42-unit container has an ink centre at Y=15.
struct CompactClockRect {
    double left{};
    double top{};
    double right{};
    double bottom{};
};

struct CompactClockPosition {
    double left{};
    double top{};
};

inline constexpr double kCompactClockReferenceWidth = 116.0;
inline constexpr double kCompactClockReferenceHeight = 42.0;
inline constexpr double kCompactClockInkCenterY = 15.0;
inline constexpr double kCompactClockInkHalfHeight = 13.0;
inline constexpr double kCompactClockGapClearance = 2.0;

[[nodiscard]] inline bool valid_compact_clock_rect(
    const CompactClockRect& rect) noexcept {
    return std::isfinite(rect.left) && std::isfinite(rect.top)
        && std::isfinite(rect.right) && std::isfinite(rect.bottom)
        && rect.left < rect.right && rect.top < rect.bottom;
}

[[nodiscard]] inline std::optional<CompactClockPosition>
calculate_compact_clock_gap_position(
    const CompactClockRect& minimap,
    const CompactClockRect& quest,
    const CompactClockRect& usable_host,
    double content_scale) noexcept {
    if (!valid_compact_clock_rect(minimap)
        || !valid_compact_clock_rect(quest)
        || !valid_compact_clock_rect(usable_host)
        || !std::isfinite(content_scale) || content_scale <= 0.0) {
        return std::nullopt;
    }
    const double width = kCompactClockReferenceWidth * content_scale;
    const double height = kCompactClockReferenceHeight * content_scale;
    const double centre_x = minimap.left + (minimap.right - minimap.left) * 0.5;
    const double gap = quest.top - minimap.bottom;
    // A full-screen/zero-sized/collapsed task container is not a gap witness.
    // The clock must fit horizontally under the minimap and overlap the task
    // column; do not slide it sideways or compress its existing glyphs.
    if (minimap.right - minimap.left < width
        || centre_x <= quest.left || centre_x >= quest.right
        || gap < 2.0 * (kCompactClockInkHalfHeight
                        + kCompactClockGapClearance) * content_scale) {
        return std::nullopt;
    }
    const double centre_y = minimap.bottom + gap * 0.5;
    CompactClockPosition result{
        centre_x - width * 0.5,
        centre_y - kCompactClockInkCenterY * content_scale};
    if (!std::isfinite(result.left) || !std::isfinite(result.top)
        || result.left < usable_host.left || result.top < usable_host.top
        || result.left + width > usable_host.right
        || result.top + height > usable_host.bottom) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] inline bool compact_clock_position_changed(
    const CompactClockPosition& previous,
    const CompactClockPosition& next,
    double content_scale) noexcept {
    const double epsilon = 0.25 * content_scale;
    return std::abs(previous.left - next.left) > epsilon
        || std::abs(previous.top - next.top) > epsilon;
}

} // namespace dswros
