#pragma once

#include <algorithm>
#include <array>
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

struct WorldMapSlateGeometry {
    double absolute_left{};
    double absolute_top{};
    double local_width{};
    double local_height{};
    double absolute_scale_x{};
    double absolute_scale_y{};
};

// Converts an atlas rectangle from the live native-map geometry into the
// independent viewport-host geometry. This is the arithmetic equivalent of
// Slate LocalToAbsolute(native) followed by AbsoluteToLocal(viewport). It is
// intentionally independent of resolution and DPI policy: each geometry owns
// its exact live transform, so DPI is applied once rather than guessed from
// viewport dimensions.
[[nodiscard]] inline std::optional<WorldMapAtlasPlacement>
calculate_world_map_viewport_placement(
    WorldMapAtlasPlacement native_local_atlas,
    WorldMapSlateGeometry native_geometry,
    WorldMapSlateGeometry viewport_geometry) noexcept {
    const auto valid_placement = [](const WorldMapAtlasPlacement& value) {
        return std::isfinite(value.left) && std::isfinite(value.top)
            && std::isfinite(value.width) && std::isfinite(value.height)
            && value.width > 0.0 && value.height > 0.0;
    };
    const auto valid_geometry = [](const WorldMapSlateGeometry& value) {
        return std::isfinite(value.absolute_left)
            && std::isfinite(value.absolute_top)
            && std::isfinite(value.local_width)
            && std::isfinite(value.local_height)
            && std::isfinite(value.absolute_scale_x)
            && std::isfinite(value.absolute_scale_y)
            && value.local_width > 0.0 && value.local_height > 0.0
            && value.absolute_scale_x > 0.0
            && value.absolute_scale_y > 0.0;
    };
    if (!valid_placement(native_local_atlas)
        || !valid_geometry(native_geometry)
        || !valid_geometry(viewport_geometry)) {
        return std::nullopt;
    }

    const WorldMapAtlasPlacement viewport_local{
        (native_geometry.absolute_left
             + native_local_atlas.left * native_geometry.absolute_scale_x
             - viewport_geometry.absolute_left)
            / viewport_geometry.absolute_scale_x,
        (native_geometry.absolute_top
             + native_local_atlas.top * native_geometry.absolute_scale_y
             - viewport_geometry.absolute_top)
            / viewport_geometry.absolute_scale_y,
        native_local_atlas.width * native_geometry.absolute_scale_x
            / viewport_geometry.absolute_scale_x,
        native_local_atlas.height * native_geometry.absolute_scale_y
            / viewport_geometry.absolute_scale_y,
    };
    return valid_placement(viewport_local)
        ? std::optional<WorldMapAtlasPlacement>{viewport_local}
        : std::nullopt;
}

inline constexpr double kWorldMapViewportTransformTolerance = 0.5;

[[nodiscard]] inline std::optional<bool>
world_map_viewport_transform_changed(
    const WorldMapAtlasPlacement& retained,
    const WorldMapAtlasPlacement& current,
    double tolerance = kWorldMapViewportTransformTolerance) noexcept {
    const auto valid = [](const WorldMapAtlasPlacement& value) {
        return std::isfinite(value.left) && std::isfinite(value.top)
            && std::isfinite(value.width) && std::isfinite(value.height)
            && value.width > 0.0 && value.height > 0.0;
    };
    if (!valid(retained) || !valid(current) || !std::isfinite(tolerance)
        || tolerance < 0.0) {
        return std::nullopt;
    }
    const double maximum_delta = std::max(
        std::max(
            std::abs(current.left - retained.left),
            std::abs(current.top - retained.top)),
        std::max(
            std::abs(current.width - retained.width),
            std::abs(current.height - retained.height)));
    return maximum_delta > tolerance;
}

enum class WorldMapTransformObservationFailure : std::uint8_t {
    NativeCanvasUnavailable,
    GeometryUnavailable,
    OwnedHostInvalid,
    AbiInvalid,
    RuntimeFault,
};

enum class WorldMapTransformObservationFailureAction : std::uint8_t {
    RetryHidden,
    RetainLastValid,
    Fault,
};

// The guarded runtime records which part of a transform refresh was active
// when a reflected call failed. Observation stages read only game-owned state
// and can be transient while the map animates. Validation and application
// stages prove or mutate only the Mod-owned payload and remain hard failures.
enum class WorldMapTransformSyncStage : std::uint8_t {
    None,
    NativeCanvasObservation,
    GeometryObservation,
    OwnedHostValidation,
    AbiValidation,
    OwnedHostApplication,
};

[[nodiscard]] constexpr WorldMapTransformObservationFailure
world_map_transform_failure_for_stage(
    WorldMapTransformSyncStage stage) noexcept {
    switch (stage) {
    case WorldMapTransformSyncStage::NativeCanvasObservation:
        return WorldMapTransformObservationFailure::NativeCanvasUnavailable;
    case WorldMapTransformSyncStage::GeometryObservation:
        return WorldMapTransformObservationFailure::GeometryUnavailable;
    case WorldMapTransformSyncStage::OwnedHostValidation:
        return WorldMapTransformObservationFailure::OwnedHostInvalid;
    case WorldMapTransformSyncStage::AbiValidation:
        return WorldMapTransformObservationFailure::AbiInvalid;
    case WorldMapTransformSyncStage::OwnedHostApplication:
    case WorldMapTransformSyncStage::None:
    default:
        return WorldMapTransformObservationFailure::RuntimeFault;
    }
}

// A live world-map layer can briefly replace its native icon Canvas while its
// opening or zoom animation settles. That is an observation gap, not evidence
// that the Mod-owned native-child hosts are corrupt. Preserve a last verified
// transform only for the same layer; ownership and ABI failures remain terminal
// so native game widgets are never used as a recovery target.
[[nodiscard]] constexpr WorldMapTransformObservationFailureAction
classify_world_map_transform_observation_failure(
    WorldMapTransformObservationFailure failure,
    bool same_layer,
    bool has_valid_transform) noexcept {
    switch (failure) {
    case WorldMapTransformObservationFailure::NativeCanvasUnavailable:
    case WorldMapTransformObservationFailure::GeometryUnavailable:
        return same_layer && has_valid_transform
            ? WorldMapTransformObservationFailureAction::RetainLastValid
            : WorldMapTransformObservationFailureAction::RetryHidden;
    case WorldMapTransformObservationFailure::OwnedHostInvalid:
    case WorldMapTransformObservationFailure::AbiInvalid:
    case WorldMapTransformObservationFailure::RuntimeFault:
    default:
        return WorldMapTransformObservationFailureAction::Fault;
    }
}

struct WorldMapHostVisibilityInputs {
    bool content_intent{};
    bool runtime_allowed{};
    bool attached{};
    bool transform_ready{};
};

// Content policy, current-world visibility, host ownership, and a verified
// transform are independent gates. No one gate may make a viewport host live.
[[nodiscard]] constexpr bool world_map_host_visibility_target(
    WorldMapHostVisibilityInputs inputs) noexcept {
    return inputs.content_intent && inputs.runtime_allowed
        && inputs.attached && inputs.transform_ready;
}

// A disengaged value belongs to a new, detached, or partially written host
// generation. Once both hosts publish a value, identical requests are pure
// no-ops and perform no UObject access.
[[nodiscard]] constexpr bool world_map_host_visibility_write_required(
    std::optional<bool> applied_visible,
    bool target_visible) noexcept {
    return !applied_visible || *applied_visible != target_visible;
}

struct WorldMapTransformVisibilityPolicy {
    bool retain_host{};
    bool transform_ready{};
    bool force_collapsed{};
};

[[nodiscard]] constexpr WorldMapTransformVisibilityPolicy
world_map_transform_visibility_policy(
    WorldMapTransformObservationFailureAction action) noexcept {
    switch (action) {
    case WorldMapTransformObservationFailureAction::RetryHidden:
        return {true, false, true};
    case WorldMapTransformObservationFailureAction::RetainLastValid:
        return {true, true, false};
    case WorldMapTransformObservationFailureAction::Fault:
    default:
        return {false, false, true};
    }
}

// A reflected write to a Mod-owned viewport host can fail transiently while
// Slate is replacing its viewport wrapper.  If the exact layer, the last
// verified placement, and both independently owned hosts are still valid,
// retain that placement and let the existing bounded settle tail retry.  A
// stale layer, missing placement, or invalid host remains terminal.
[[nodiscard]] constexpr WorldMapTransformObservationFailureAction
classify_world_map_owned_host_application_failure(
    bool same_layer,
    bool has_valid_transform,
    bool owned_hosts_still_valid) noexcept {
    return same_layer && has_valid_transform && owned_hosts_still_valid
        ? WorldMapTransformObservationFailureAction::RetainLastValid
        : WorldMapTransformObservationFailureAction::Fault;
}

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

[[nodiscard]] inline std::optional<double>
world_map_parent_extent_maximum_delta(
    const WorldMapGeometrySample& retained,
    const WorldMapGeometrySample& current) noexcept {
    if (!std::isfinite(retained.parent_width)
        || !std::isfinite(retained.parent_height)
        || !std::isfinite(current.parent_width)
        || !std::isfinite(current.parent_height)
        || retained.parent_width <= 0.0 || retained.parent_height <= 0.0
        || current.parent_width <= 0.0 || current.parent_height <= 0.0) {
        return std::nullopt;
    }
    return std::max(
        std::abs(current.parent_width - retained.parent_width),
        std::abs(current.parent_height - retained.parent_height));
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

// The atlas rectangle is authored once in the exact native parent's local
// coordinate space. PlayerIconWidget can expose a different cached Slate
// anchor while that same parent animates zoom; that sibling observation is not
// a coordinate-space migration and must never move the retained atlas. A real
// parent-extent change instead needs a fresh atlas so marker glyphs are not
// scaled together with their positions.
[[nodiscard]] inline std::optional<WorldMapAtlasPlacement>
retain_world_map_atlas_placement(
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
    return retained;
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

// Attach-only readiness. Raw player-anchor motion is expected while flying;
// compare the world-to-Canvas origin instead. Do not use this to reposition a
// retained atlas or change the separate parent-extent rebuild sampler.
struct WorldMapProjectionIdentity {
    std::int32_t index{-1};
    std::int32_t serial{};
    bool operator==(const WorldMapProjectionIdentity&) const = default;
};

struct WorldMapProjectionSample {
    WorldMapGeometrySample geometry{};
    Position player_world{};
    double map_dimensions{};
    double map_ui_size{};
    std::int32_t map_id{};
    // Layer, native parent, player icon, owning player. Numeric weak identities
    // only: no UObject or cached FGeometry survives a service attempt.
    std::array<WorldMapProjectionIdentity, 4> identities{};
};

struct WorldMapProjectionDelta {
    bool comparable{};
    double anchor{};
    double player_world{};
    double extent{};
    double origin{};
    double maximum{};
};

[[nodiscard]] inline std::optional<WorldMapPoint> world_map_projection_origin(
    const WorldMapProjectionSample& sample) noexcept {
    if (sample.map_id <= 0
        || !validate_world_map_canvas_anchor(
            sample.geometry.player_canvas_x, sample.geometry.player_canvas_y,
            sample.geometry.parent_width, sample.geometry.parent_height,
            sample.map_ui_size)) {
        return std::nullopt;
    }
    for (const auto& identity : sample.identities) {
        if (identity.index < 0 || identity.serial <= 0) {
            return std::nullopt;
        }
    }
    return project_world_map_point(
        sample.geometry.player_canvas_x, sample.geometry.player_canvas_y,
        sample.player_world, {0.0, 0.0, 0.0}, sample.map_dimensions,
        sample.geometry.parent_width, sample.geometry.parent_height);
}

[[nodiscard]] inline WorldMapGeometryStabilityResult
observe_world_map_projection_sample(
    bool& retained_valid,
    WorldMapProjectionSample& retained,
    const WorldMapProjectionSample& current,
    WorldMapProjectionDelta& delta) noexcept {
    delta = {};
    const auto current_origin = world_map_projection_origin(current);
    if (!current_origin) {
        retained_valid = false;
        retained = {};
        return WorldMapGeometryStabilityResult::None;
    }
    const auto previous_origin = retained_valid
        ? world_map_projection_origin(retained) : std::nullopt;
    if (!previous_origin || retained.identities != current.identities
        || retained.map_id != current.map_id
        || retained.map_dimensions != current.map_dimensions
        || retained.map_ui_size != current.map_ui_size) {
        retained = current;
        retained_valid = true;
        return WorldMapGeometryStabilityResult::Seeded;
    }

    delta.anchor = std::max(
        std::abs(current.geometry.player_canvas_x - retained.geometry.player_canvas_x),
        std::abs(current.geometry.player_canvas_y - retained.geometry.player_canvas_y));
    delta.player_world = std::max(
        std::abs(current.player_world.x - retained.player_world.x),
        std::abs(current.player_world.y - retained.player_world.y));
    delta.extent = *world_map_parent_extent_maximum_delta(
        retained.geometry, current.geometry);
    delta.origin = std::max(
        std::abs(current_origin->x - previous_origin->x),
        std::abs(current_origin->y - previous_origin->y));
    delta.maximum = std::max(delta.extent, delta.origin);
    if (!std::isfinite(delta.anchor) || !std::isfinite(delta.player_world)
        || !std::isfinite(delta.maximum)) {
        retained_valid = false;
        retained = {};
        delta = {};
        return WorldMapGeometryStabilityResult::None;
    }
    delta.comparable = true;
    retained = current;
    return delta.maximum <= kWorldMapGeometryStabilityTolerance
        ? WorldMapGeometryStabilityResult::Stable
        : WorldMapGeometryStabilityResult::Replaced;
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
