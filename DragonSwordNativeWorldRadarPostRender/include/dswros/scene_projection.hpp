#pragma once

#include <dswros/scene_marker_model.hpp>
#include <array>
#include <cmath>
#include <numbers>

namespace dswros {

// A frame-local camera basis. Calibration uses the engine's actual projection,
// including its FOV, aspect constraint, viewport offset and DPI transformation.
// Nothing from a previous frame is interpolated or reused.
struct SceneCameraBasis {
    Position origin{}, forward{}, right{}, up{};
};

inline Position scene_add_scaled(Position a, Position b, double scale) noexcept {
    return {a.x + b.x * scale, a.y + b.y * scale, a.z + b.z * scale};
}
inline double scene_dot(Position a, Position b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline SceneCameraBasis scene_camera_basis(Position origin, Position rotation) noexcept {
    const double factor = std::numbers::pi / 180.0;
    const double sp = std::sin(rotation.x * factor), cp = std::cos(rotation.x * factor);
    const double sy = std::sin(rotation.y * factor), cy = std::cos(rotation.y * factor);
    const double sr = std::sin(rotation.z * factor), cr = std::cos(rotation.z * factor);
    return {origin, {cp * cy, cp * sy, sp},
        {sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp},
        {-cr * sp * cy - sr * sy, -cr * sp * sy + sr * cy, cr * cp}};
}

inline constexpr double kSceneCalibrationDepth = 4096.0;
inline std::array<Position, 5> scene_calibration_points(const SceneCameraBasis& camera) noexcept {
    const auto center = scene_add_scaled(camera.origin, camera.forward, kSceneCalibrationDepth);
    return {center,
        scene_add_scaled(center, camera.right, kSceneCalibrationDepth),
        scene_add_scaled(center, camera.up, kSceneCalibrationDepth),
        scene_add_scaled(scene_add_scaled(center, camera.forward, kSceneCalibrationDepth),
                         camera.right, kSceneCalibrationDepth),
        scene_add_scaled(scene_add_scaled(center, camera.forward, kSceneCalibrationDepth),
                         camera.up, kSceneCalibrationDepth)};
}

struct SceneFrameProjection {
    SceneCameraBasis camera{};
    SceneProjectedPoint center{}, right{}, up{};
    bool valid{};

    [[nodiscard]] SceneProjectedPoint project(Position point) const noexcept {
        if (!valid || !valid_scene_position(point)) return {};
        const Position delta{point.x - camera.origin.x, point.y - camera.origin.y,
                             point.z - camera.origin.z};
        const double depth = scene_dot(delta, camera.forward);
        if (!std::isfinite(depth) || depth <= 1.0) return {};
        const double x = scene_dot(delta, camera.right) / depth;
        const double y = scene_dot(delta, camera.up) / depth;
        const double screen_x = center.x + (right.x - center.x) * x + (up.x - center.x) * y;
        const double screen_y = center.y + (right.y - center.y) * x + (up.y - center.y) * y;
        return {screen_x, screen_y, std::isfinite(screen_x) && std::isfinite(screen_y)};
    }
};

inline bool scene_projection_matches(SceneProjectedPoint a, SceneProjectedPoint b,
                                      double tolerance) noexcept {
    return a.in_front && b.in_front && std::isfinite(a.x) && std::isfinite(a.y)
        && std::isfinite(b.x) && std::isfinite(b.y)
        && std::isfinite(tolerance) && tolerance >= 0.0
        && std::hypot(a.x - b.x, a.y - b.y) <= tolerance;
}

[[nodiscard]] inline SceneFrameProjection calibrate_scene_projection(
    SceneCameraBasis camera, const std::array<SceneProjectedPoint, 5>& samples,
    double tolerance) noexcept {
    SceneFrameProjection result{camera, samples[0], samples[1], samples[2], false};
    if (!valid_scene_position(camera.origin) || !std::isfinite(tolerance)
        || tolerance <= 0.0) return result;
    for (const auto& point : samples)
        if (!point.in_front || !std::isfinite(point.x) || !std::isfinite(point.y)) return result;
    // Depth-independent samples belong on the native path even when a large
    // physical-to-widget tolerance could also accept the perspective estimate.
    if (scene_projection_matches(samples[3], samples[1], tolerance)
        && scene_projection_matches(samples[4], samples[2], tolerance)) return result;
    const double rx = samples[1].x - samples[0].x, ry = samples[1].y - samples[0].y;
    const double ux = samples[2].x - samples[0].x, uy = samples[2].y - samples[0].y;
    if (!std::isfinite(rx * uy - ry * ux) || std::abs(rx * uy - ry * ux) < 1.0) return result;
    result.valid = true;
    // Both axes must follow perspective depth. A horizontal sample alone can
    // miss a camera-pitch mismatch. Orthographic views keep the native path.
    const auto points = scene_calibration_points(camera);
    result.valid = scene_projection_matches(result.project(points[3]), samples[3], tolerance)
        && scene_projection_matches(result.project(points[4]), samples[4], tolerance);
    return result;
}

} // namespace dswros
