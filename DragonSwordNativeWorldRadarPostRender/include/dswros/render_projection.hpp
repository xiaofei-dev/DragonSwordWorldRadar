#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

#include <dswros/object_state.hpp>

namespace dswros {

struct ScreenPoint {
    double x{};
    double y{};
    double display_scale{};
};

struct CompactViewportLayout {
    double display_scale{};
    double umg_unit_scale{};
    double host_origin_x{};
    double host_origin_y{};
    double host_render_scale{};
};

struct WorldMapPoint {
    double x{};
    double y{};
};

struct WorldMapAtlasPlacement {
    double left{};
    double top{};
    double width{};
    double height{};
};

struct WorldMapGeometrySample {
    double player_canvas_x{};
    double player_canvas_y{};
    double parent_width{};
    double parent_height{};
};

[[nodiscard]] inline std::optional<double>
world_map_geometry_maximum_delta(
    const WorldMapGeometrySample& retained,
    const WorldMapGeometrySample& current) noexcept {
    if (!std::isfinite(retained.player_canvas_x)
        || !std::isfinite(retained.player_canvas_y)
        || !std::isfinite(retained.parent_width)
        || !std::isfinite(retained.parent_height)
        || !std::isfinite(current.player_canvas_x)
        || !std::isfinite(current.player_canvas_y)
        || !std::isfinite(current.parent_width)
        || !std::isfinite(current.parent_height)
        || retained.parent_width <= 0.0 || retained.parent_height <= 0.0
        || current.parent_width <= 0.0 || current.parent_height <= 0.0) {
        return std::nullopt;
    }
    return std::max(
        std::max(
            std::abs(current.player_canvas_x - retained.player_canvas_x),
            std::abs(current.player_canvas_y - retained.player_canvas_y)),
        std::max(
            std::abs(current.parent_width - retained.parent_width),
            std::abs(current.parent_height - retained.parent_height)));
}

enum class WorldMapGeometryStabilityResult : std::uint8_t {
    None,
    Seeded,
    Replaced,
    Stable,
};

// Cached Slate layout can lag the map widget by one frame. Half a logical
// Slate unit admits sub-pixel rounding while remaining far below the
// aspect-ratio/window-layout offsets this gate is intended to reject.
inline constexpr double kWorldMapGeometryStabilityTolerance = 0.5;

[[nodiscard]] inline WorldMapGeometryStabilityResult
observe_world_map_geometry_sample(
    bool& retained_valid,
    WorldMapGeometrySample& retained,
    const WorldMapGeometrySample& current,
    double& maximum_delta) noexcept {
    if (!std::isfinite(current.player_canvas_x)
        || !std::isfinite(current.player_canvas_y)
        || !std::isfinite(current.parent_width)
        || !std::isfinite(current.parent_height)
        || current.parent_width <= 0.0 || current.parent_height <= 0.0) {
        maximum_delta = 0.0;
        return WorldMapGeometryStabilityResult::None;
    }
    if (!retained_valid) {
        retained = current;
        retained_valid = true;
        maximum_delta = 0.0;
        return WorldMapGeometryStabilityResult::Seeded;
    }

    const auto observed_delta = world_map_geometry_maximum_delta(
        retained, current);
    if (!observed_delta) {
        maximum_delta = 0.0;
        return WorldMapGeometryStabilityResult::None;
    }
    maximum_delta = *observed_delta;
    retained = current;
    return maximum_delta <= kWorldMapGeometryStabilityTolerance
        ? WorldMapGeometryStabilityResult::Stable
        : WorldMapGeometryStabilityResult::Replaced;
}

[[nodiscard]] constexpr const char* world_map_geometry_stability_name(
    WorldMapGeometryStabilityResult result) noexcept {
    switch (result) {
    case WorldMapGeometryStabilityResult::Seeded:
        return "stabilizing_seeded";
    case WorldMapGeometryStabilityResult::Replaced:
        return "stabilizing_replaced";
    case WorldMapGeometryStabilityResult::Stable:
        return "stable";
    default:
        return "none";
    }
}

// Attachment failures 3-9 are transient widget-tree readiness failures.
// Failure 24 is the equivalent transient case for the live Slate geometry
// conversion used by non-16:9 and windowed world-map layouts.
[[nodiscard]] constexpr bool world_map_attach_failure_retryable(
    std::uint32_t failure) noexcept {
    return (failure >= 3U && failure <= 9U) || failure == 24U;
}

// A failed retryable attempt must leave the one-session attachment latch open.
// Successful and non-retryable attempts are terminal for the current session.
[[nodiscard]] constexpr bool world_map_attach_attempt_is_terminal(
    bool attached,
    std::uint32_t failure) noexcept {
    return attached || !world_map_attach_failure_retryable(failure);
}

inline constexpr double kCompactReferenceCenterRight = 220.0;
inline constexpr double kCompactReferenceCenterY = 217.0;
inline constexpr double kCompactReferenceHostHalfSize = 242.25;

// Validates a point already converted through Slate from the live player
// widget into the exact native icon Canvas local space. That Canvas is allowed
// to use the live viewport/aspect-ratio extent; WorldMapUISize is the authored
// world-delta scale, not proof that every parent geometry is a 3000x3000
// square. The small point tolerance is layout-rounding only.
[[nodiscard]] inline std::optional<WorldMapPoint>
validate_world_map_canvas_anchor(
    double player_canvas_x,
    double player_canvas_y,
    double parent_width,
    double parent_height,
    double map_ui_size) noexcept {
    if (!std::isfinite(player_canvas_x) || !std::isfinite(player_canvas_y)
        || !std::isfinite(parent_width) || !std::isfinite(parent_height)
        || !std::isfinite(map_ui_size) || map_ui_size <= 0.0
        || parent_width <= 0.0 || parent_height <= 0.0) {
        return std::nullopt;
    }
    const double point_tolerance = std::max(
        2.0, std::max(parent_width, parent_height) * 0.01);
    if (player_canvas_x < -point_tolerance
        || player_canvas_y < -point_tolerance
        || player_canvas_x > parent_width + point_tolerance
        || player_canvas_y > parent_height + point_tolerance) {
        return std::nullopt;
    }
    return WorldMapPoint{player_canvas_x, player_canvas_y};
}

// A zoom tier may replace only the native icon Canvas while keeping the same
// parent-local extent and atlas pixels. Rebase the retained bounds by the exact
// live player-anchor delta only when both coordinate spaces have the same
// extent. An aspect/DPI reflow needs a fresh atlas so marker glyphs are not
// scaled together with their positions.
[[nodiscard]] inline std::optional<WorldMapAtlasPlacement>
rebase_world_map_atlas_placement(
    WorldMapAtlasPlacement retained,
    WorldMapGeometrySample retained_geometry,
    WorldMapGeometrySample current_geometry) noexcept {
    if (!std::isfinite(retained.left) || !std::isfinite(retained.top)
        || !std::isfinite(retained.width) || !std::isfinite(retained.height)
        || retained.width <= 0.0 || retained.height <= 0.0
        || !world_map_geometry_maximum_delta(
            retained_geometry, current_geometry)
        || std::abs(
            retained_geometry.parent_width - current_geometry.parent_width)
            > kWorldMapGeometryStabilityTolerance
        || std::abs(
            retained_geometry.parent_height - current_geometry.parent_height)
            > kWorldMapGeometryStabilityTolerance) {
        return std::nullopt;
    }
    const WorldMapAtlasPlacement rebased{
        retained.left
            + (current_geometry.player_canvas_x
               - retained_geometry.player_canvas_x),
        retained.top
            + (current_geometry.player_canvas_y
               - retained_geometry.player_canvas_y),
        retained.width,
        retained.height,
    };
    if (!std::isfinite(rebased.left) || !std::isfinite(rebased.top)) {
        return std::nullopt;
    }
    return rebased;
}

// Calculates both raw viewport placement and the render-only correction needed
// when a fixed compact host survives a resolution, window-mode, or DPI change.
// content_umg_unit_scale is the immutable scale used when the host was built.
[[nodiscard]] inline std::optional<CompactViewportLayout>
calculate_compact_viewport_layout(
    double viewport_width,
    double viewport_height,
    double viewport_dpi_scale,
    double content_umg_unit_scale) noexcept {
    if (!std::isfinite(viewport_width) || !std::isfinite(viewport_height)
        || !std::isfinite(viewport_dpi_scale)
        || !std::isfinite(content_umg_unit_scale)
        || viewport_width <= 0.0 || viewport_height <= 0.0
        || viewport_dpi_scale <= 0.0 || content_umg_unit_scale <= 0.0) {
        return std::nullopt;
    }

    const double display_scale = std::clamp(
        std::min(viewport_width / 2560.0, viewport_height / 1440.0),
        0.25,
        4.0);
    const double umg_unit_scale = display_scale / viewport_dpi_scale;
    const double host_render_scale =
        umg_unit_scale / content_umg_unit_scale;
    if (!std::isfinite(display_scale) || !std::isfinite(umg_unit_scale)
        || !std::isfinite(host_render_scale)
        || umg_unit_scale <= 0.0 || host_render_scale <= 0.0) {
        return std::nullopt;
    }

    return CompactViewportLayout{
        display_scale,
        umg_unit_scale,
        viewport_width
            - (kCompactReferenceCenterRight
               + kCompactReferenceHostHalfSize) * display_scale,
        (kCompactReferenceCenterY - kCompactReferenceHostHalfSize)
            * display_scale,
        host_render_scale,
    };
}

// Expanded-map coordinates are relative to the exact live native icon Canvas.
// Supplying both the live player anchor and the parent-local extent preserves
// the coordinate basis used by the game's own map icons. WorldMapUISize is
// authored metadata and is not the parent extent on every aspect ratio.
[[nodiscard]] inline std::optional<WorldMapPoint> project_world_map_point(
    double player_canvas_x,
    double player_canvas_y,
    Position player,
    Position target,
    double map_dimensions,
    double parent_width,
    double parent_height) noexcept {
    if (!std::isfinite(player_canvas_x) || !std::isfinite(player_canvas_y)
        || !std::isfinite(player.x) || !std::isfinite(player.y)
        || !std::isfinite(target.x) || !std::isfinite(target.y)
        || !std::isfinite(map_dimensions) || map_dimensions <= 0.0
        || !std::isfinite(parent_width) || parent_width <= 0.0
        || !std::isfinite(parent_height) || parent_height <= 0.0) {
        return std::nullopt;
    }

    const double x = player_canvas_x
        + (target.x - player.x) / map_dimensions * parent_width;
    const double y = player_canvas_y
        + (target.y - player.y) / map_dimensions * parent_height;
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return std::nullopt;
    }
    return WorldMapPoint{x, y};
}

[[nodiscard]] inline std::optional<ScreenPoint> project_compact_radar_point(
    Position player,
    Position target,
    double world_radius,
    int canvas_width,
    int canvas_height) noexcept {
    if (!std::isfinite(player.x) || !std::isfinite(player.y)
        || !std::isfinite(target.x) || !std::isfinite(target.y)
        || !std::isfinite(world_radius) || world_radius <= 0.0
        || canvas_width <= 0 || canvas_height <= 0) {
        return std::nullopt;
    }

    const double scale = std::min(canvas_width / 2560.0, canvas_height / 1440.0);
    const double delta_x = target.x - player.x;
    const double delta_y = target.y - player.y;
    const double distance_squared = delta_x * delta_x + delta_y * delta_y;
    const double radius_squared = world_radius * world_radius;
    if (!std::isfinite(scale) || scale <= 0.0 || !std::isfinite(distance_squared)
        || distance_squared > radius_squared) {
        return std::nullopt;
    }

    // These are the accepted stable overlay's 2560x1440 compact-radar
    // geometry constants: 360 px square, 40 px right margin, 37 px top
    // margin, and 170 px usable radius. Keeping the same axes avoids a
    // behavioral change while the rendering backend is replaced.
    constexpr double kReferenceRadarRadius = 170.0;
    const double center_x =
        canvas_width - kCompactReferenceCenterRight * scale;
    const double center_y = kCompactReferenceCenterY * scale;
    const double radar_radius = kReferenceRadarRadius * scale;
    return ScreenPoint{
        center_x + delta_x / world_radius * radar_radius,
        center_y + delta_y / world_radius * radar_radius,
        scale,
    };
}

} // namespace dswros
