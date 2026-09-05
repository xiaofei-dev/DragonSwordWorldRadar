#pragma once

namespace dswros {

struct CompactMenuState final {
    bool any_category_enabled{};
    bool position_valid{};
    bool mouse_cursor_visible{};
    bool world_map_visible{};
    bool game_paused{};
    bool activity_suppressed{};
};

[[nodiscard]] inline constexpr bool compact_render_suppressed(
    CompactMenuState state) noexcept {
    return !state.any_category_enabled
        || !state.position_valid
        || state.mouse_cursor_visible
        || state.world_map_visible
        || state.game_paused
        || state.activity_suppressed;
}

} // namespace dswros
