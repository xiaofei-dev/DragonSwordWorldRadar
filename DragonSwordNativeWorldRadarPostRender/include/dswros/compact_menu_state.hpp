#pragma once

#include <cmath>
#include <cstdint>

namespace dswros {

// Unknown is not evidence that gameplay HUD is hidden. Preserve the existing
// cursor/map/pause guards instead of latching an old menu state indefinitely.
enum class NativeMinimapPaint { Unknown, Visible, Hidden };

[[nodiscard]] inline NativeMinimapPaint native_widget_paint(
    bool visibility_known, std::uint64_t visibility,
    bool opacity_known, double opacity) noexcept {
    if (visibility_known && (visibility == 1 || visibility == 2)) {
        return NativeMinimapPaint::Hidden; // Collapsed / Hidden
    }
    if (opacity_known && std::isfinite(opacity) && opacity == 0.0) {
        return NativeMinimapPaint::Hidden;
    }
    if (visibility_known && visibility <= 4 && opacity_known
        && std::isfinite(opacity) && opacity > 0.0 && opacity <= 1.0) {
        return NativeMinimapPaint::Visible; // Includes both hit-test-invisible modes.
    }
    return NativeMinimapPaint::Unknown;
}

struct CompactMenuState final {
    bool any_category_enabled{};
    bool position_valid{};
    bool mouse_cursor_visible{};
    bool world_map_visible{};
    bool game_paused{};
    bool activity_suppressed{};
    bool native_minimap_hidden{};
    // Only the radar's own settings cursor may be exempted. The caller must
    // prove gameplay HUD paint and that no cursor existed before opening it.
    bool settings_cursor_only{};
};

[[nodiscard]] inline constexpr bool compact_render_suppressed(
    CompactMenuState state) noexcept {
    return !state.any_category_enabled
        || !state.position_valid
        || (state.mouse_cursor_visible && !state.settings_cursor_only)
        || state.world_map_visible
        || state.game_paused
        || state.activity_suppressed
        || state.native_minimap_hidden;
}

} // namespace dswros
