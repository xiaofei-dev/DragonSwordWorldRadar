#include <dswros/scene_projection.hpp>
#include <iostream>
#include <limits>

namespace {
int failures{}, checks{};
void check(bool value, const char* name) {
    ++checks;
    if (!value) { ++failures; std::cerr << name << '\n'; }
}
using namespace dswros;
bool near(double a, double b) { return std::abs(a - b) < 1e-7; }
}

int main() {
    const std::array<SceneProjectedPoint, 5> samples{{
        {700, 450, true}, {1600, 450, true}, {700, -450, true},
        {1150, 450, true}, {700, 0, true}}};
    auto projection = calibrate_scene_projection(scene_camera_basis({}, {}), samples, 0.25);
    check(projection.valid, "perspective calibration");
    check(scene_projection_matches(projection.project({1000, 250, 100}),
        {925, 360, true}, 1e-7), "known perspective point with off-center viewport");
    check(!projection.project({-100, 10, 10}).in_front, "behind camera cannot mirror");
    check(!projection.project({0, 10, 10}).in_front, "camera plane is hidden");
    const auto old = projection.project({1000, 250, 100});
    for (const auto rotation : {Position{0, 90, 0}, Position{45, 0, 0}, Position{0, 0, 90},
                               Position{30, -70, 20}}) {
        const auto basis = scene_camera_basis({1200000, -850000, 6500}, rotation);
        check(near(scene_dot(basis.forward, basis.right), 0)
            && near(scene_dot(basis.right, basis.up), 0)
            && near(scene_dot(basis.up, basis.forward), 0), "orthogonal rotated camera");
        check(near(scene_dot(basis.forward, basis.forward), 1)
            && near(scene_dot(basis.right, basis.right), 1)
            && near(scene_dot(basis.up, basis.up), 1), "unit rotated axes");
        const auto point = scene_add_scaled(scene_add_scaled(
            scene_add_scaled(basis.origin, basis.forward, 1000), basis.right, 250), basis.up, 100);
        const auto rotated = calibrate_scene_projection(basis, samples, 0.25);
        check(scene_projection_matches(rotated.project(point), old, 1e-7),
            "translation, yaw, pitch and roll retain expected screen position");
    }
    auto wide = samples;
    for (auto& p : wide) { p.x = p.x * 1.5 + 270; p.y = p.y * 1.5 + 100; }
    auto wide_projection = calibrate_scene_projection(scene_camera_basis({}, {}), wide, 0.25);
    check(scene_projection_matches(wide_projection.project({1000, 250, 100}),
        {925 * 1.5 + 270, 360 * 1.5 + 100, true}, 1e-7), "aspect, DPI and viewport offset calibration");
    auto ortho = samples;
    ortho[3] = ortho[1];
    ortho[4] = ortho[2];
    auto orthographic = calibrate_scene_projection(scene_camera_basis({}, {}), ortho, 0.25);
    check(!orthographic.valid, "orthographic projection requires native fallback");
    const std::array<SceneProjectedPoint, 5> tiny_ortho{{
        {0, 0, true}, {2, 0, true}, {0, -2, true}, {2, 0, true}, {0, -2, true}}};
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), tiny_ortho, 2).valid,
          "large widget tolerance cannot accidentally accept orthographic projection");
    auto invalid = samples;
    invalid[3].x += 40;
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "nonlinear or inconsistent engine projection falls back");
    invalid = samples; invalid[4].y += 40;
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "vertical depth mismatch falls back despite valid horizontal depth");
    invalid = samples; invalid[4].in_front = false;
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "failed vertical depth sample falls back");
    invalid = samples; invalid[2].in_front = false;
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "failed native sample falls back");
    invalid = samples; invalid[2] = invalid[1];
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "degenerate screen basis fails closed");
    invalid = samples; invalid[0].x = std::numeric_limits<double>::quiet_NaN();
    check(!calibrate_scene_projection(scene_camera_basis({}, {}), invalid, 0.25).valid,
          "NaN sample fails closed");
    check(!scene_projection_matches({1, 1, true}, {1, 1, false}, .25), "witness checks front state");
    check(!scene_projection_matches({1, 1, true}, {1.3, 1, true}, .25), "witness rejects pixel deviation");
    check(!scene_projection_matches({0, 0, true}, {.2, .2, true}, .25),
          "diagonal witness error uses radial pixel tolerance");
    check(scene_projection_matches({0, 0, true}, {.25, 0, true}, .25),
          "radial tolerance includes its exact boundary");
    check(!scene_projection_matches({1, 1, true}, {1, 1, true},
          std::numeric_limits<double>::quiet_NaN()), "NaN tolerance is rejected");
    check(!scene_projection_matches({1, 1, true}, {1, 1, true},
          std::numeric_limits<double>::infinity()), "infinite tolerance is rejected");
    check(!scene_projection_matches({1, 1, true}, {1, 1, true}, -.25),
          "negative tolerance is rejected");

    // A cached camera basis can differ from the projection's current camera.
    // Pitch error leaves the horizontal depth probe and an on-axis real marker
    // consistent while an elevated marker visibly diverges.
    const auto assumed_camera = scene_camera_basis({}, {});
    const auto actual_camera = scene_camera_basis({}, {5, 0, 0});
    const auto actual_project = [&actual_camera](Position point) {
        const double depth = scene_dot(point, actual_camera.forward);
        return SceneProjectedPoint{960 + 900 * scene_dot(point, actual_camera.right) / depth,
            540 - 900 * scene_dot(point, actual_camera.up) / depth, depth > 0};
    };
    const auto calibration_points = scene_calibration_points(assumed_camera);
    std::array<SceneProjectedPoint, 5> pitched_samples{};
    for (std::size_t i = 0; i < calibration_points.size(); ++i)
        pitched_samples[i] = actual_project(calibration_points[i]);
    const SceneFrameProjection unchecked_pitch{
        assumed_camera, pitched_samples[0], pitched_samples[1], pitched_samples[2], true};
    check(scene_projection_matches(unchecked_pitch.project(calibration_points[3]),
        pitched_samples[3], .25), "pitch counterexample passes horizontal depth probe");
    check(scene_projection_matches(unchecked_pitch.project({10000, 0, 0}),
        actual_project({10000, 0, 0}), .25), "pitch counterexample passes on-axis real witness");
    check(!scene_projection_matches(unchecked_pitch.project({4096, 0, 2048}),
        actual_project({4096, 0, 2048}), .25), "pitch counterexample displaces elevated marker");
    check(!calibrate_scene_projection(assumed_camera, pitched_samples, .25).valid,
          "vertical depth probe rejects camera-pitch counterexample");
    check(near(scene_camera_basis({}, {0, 90, 0}).forward.y, 1), "Unreal yaw orientation");
    check(near(scene_camera_basis({}, {90, 0, 0}).forward.z, 1), "Unreal pitch orientation");
    check(near(scene_camera_basis({}, {0, 0, 90}).right.z, -1), "Unreal roll orientation");
    std::cout << checks << " scene projection checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
