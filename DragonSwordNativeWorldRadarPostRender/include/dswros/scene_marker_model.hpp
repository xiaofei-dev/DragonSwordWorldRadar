#pragma once

#include <dswros/object_state.hpp>
#include <dswros/scene_preferences.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace dswros {

inline constexpr std::size_t kSceneMarkerCapacity = 50;
inline constexpr double kSceneMaximumDistanceMeters = 1000.0;

enum class SceneMarkerKind : std::uint8_t {
    TreasureOther, TreasureMiniGame, TreasureMap, TreasurePuzzle, AreaQuest, MiniGame,
};

enum class SceneMiniGameKind : std::uint8_t { None, Fly, Mole, Wave };

// Positions are authoritative catalog navigation anchors in Unreal centimeters.
// In particular, an AreaQuest anchor is not a claimed exact quest-object height.
struct SceneMarker {
    std::int64_t id{};
    SceneMarkerKind kind{SceneMarkerKind::TreasureOther};
    Position position{};
    SceneMiniGameKind minigame_kind{SceneMiniGameKind::None};
};

// Display height is a projection-only offset. Selection, actual distance and
// every catalog/compact height consumer continue to use marker.position.
[[nodiscard]] constexpr Position scene_display_position(const SceneMarker& marker) noexcept {
    Position position = marker.position;
    position.z += marker.kind == SceneMarkerKind::AreaQuest ? 180.0
        : marker.kind == SceneMarkerKind::MiniGame ? 150.0 : 160.0;
    return position;
}

[[nodiscard]] constexpr double scene_distance_offset_meters(const SceneMarker& marker) noexcept {
    if (marker.kind == SceneMarkerKind::MiniGame)
        return marker.minigame_kind == SceneMiniGameKind::Mole ? 2.0 : 0.0;
    return 1.0;
}

[[nodiscard]] constexpr unsigned scene_identity_namespace(SceneMarkerKind kind) noexcept {
    return kind == SceneMarkerKind::AreaQuest ? 1U
        : kind == SceneMarkerKind::MiniGame ? 2U : 0U;
}

[[nodiscard]] inline bool same_scene_identity(
    const SceneMarker& left, const SceneMarker& right) noexcept {
    return left.id == right.id
        && scene_identity_namespace(left.kind) == scene_identity_namespace(right.kind);
}

struct SceneCandidate {
    SceneMarker marker{};
    double distance_meters{};
    double rank{};
};

struct SceneSelection {
    std::array<SceneCandidate, kSceneMarkerCapacity> values{};
    std::size_t count{};
};

[[nodiscard]] inline bool valid_scene_position(Position position) noexcept {
    return std::isfinite(position.x) && std::isfinite(position.y)
        && std::isfinite(position.z);
}

// Fixed storage, deterministic identity tie breaks, and a small incumbent bias
// avoid rapid replacement at the distance cut without keeping stale positions.
// The caller supplies its already-filtered catalog; no live object discovery is
// involved and duplicates cannot consume multiple projection slots.
[[nodiscard]] inline SceneSelection select_scene_markers(
    Position player, std::span<const SceneMarker> markers,
    const SceneSelection& previous = {}, SceneDisplaySettings settings = {}) noexcept {
    SceneSelection result{};
    settings = normalize_scene_display_settings(settings);
    if (!valid_scene_position(player) || settings.range_meters == 0
        || settings.marker_limit == 0) return result;
    const std::size_t limit = settings.marker_limit;
    const auto less = [](const SceneCandidate& a, const SceneCandidate& b) {
        if (a.rank != b.rank) return a.rank < b.rank;
        const auto a_namespace = scene_identity_namespace(a.marker.kind);
        const auto b_namespace = scene_identity_namespace(b.marker.kind);
        if (a_namespace != b_namespace) return a_namespace < b_namespace;
        return a.marker.id < b.marker.id;
    };
    const auto duplicate_precedes = [](const SceneCandidate& a,
                                       const SceneCandidate& b) {
        if (a.distance_meters != b.distance_meters)
            return a.distance_meters < b.distance_meters;
        // Conflicting treasure subtypes still identify one chest. Resolve an
        // equally near duplicate consistently, independent of input order.
        if (a.marker.kind != b.marker.kind) return a.marker.kind < b.marker.kind;
        if (a.marker.minigame_kind != b.marker.minigame_kind)
            return a.marker.minigame_kind < b.marker.minigame_kind;
        if (a.marker.position.x != b.marker.position.x)
            return a.marker.position.x < b.marker.position.x;
        if (a.marker.position.y != b.marker.position.y)
            return a.marker.position.y < b.marker.position.y;
        return a.marker.position.z < b.marker.position.z;
    };
    for (const auto& marker : markers) {
        if (marker.id <= 0 || !valid_scene_position(marker.position)
            || static_cast<unsigned>(marker.kind)
                > static_cast<unsigned>(SceneMarkerKind::MiniGame)) continue;
        const double distance = std::hypot(
            marker.position.x - player.x, marker.position.y - player.y,
            marker.position.z - player.z) / 100.0;
        if (!std::isfinite(distance) || distance > settings.range_meters)
            continue;
        bool incumbent = false;
        for (std::size_t i = 0; i < std::min(previous.count,
                                            previous.values.size()); ++i) {
            incumbent |= same_scene_identity(marker, previous.values[i].marker);
        }
        SceneCandidate candidate{marker, distance,
                                 distance * (incumbent ? 0.92 : 1.0)};
        std::size_t duplicate = result.count;
        for (std::size_t i = 0; i < result.count; ++i) {
            if (same_scene_identity(marker, result.values[i].marker)) {
                duplicate = i;
                break;
            }
        }
        if (duplicate < result.count) {
            if (!duplicate_precedes(candidate, result.values[duplicate])) continue;
            for (std::size_t i = duplicate + 1; i < result.count; ++i)
                result.values[i - 1] = result.values[i];
            --result.count;
        }
        std::size_t insert = 0;
        while (insert < result.count && !less(candidate, result.values[insert]))
            ++insert;
        if (insert >= limit) continue;
        const std::size_t end = std::min(result.count,
                                        limit - 1);
        for (std::size_t i = end; i > insert; --i)
            result.values[i] = result.values[i - 1];
        result.values[insert] = candidate;
        result.count = std::min(result.count + 1, limit);
    }
    return result;
}

// The control pass already selected and deduplicated this bounded list. Keep
// its stable ordering while refreshing actual distance/range every visual frame;
// do not repeat nearest-N sorting merely because the camera rotated.
[[nodiscard]] inline SceneSelection refresh_scene_frame_selection(
    Position player, std::span<const SceneMarker> markers,
    SceneDisplaySettings settings = {}) noexcept {
    SceneSelection result{};
    settings = normalize_scene_display_settings(settings);
    if (!valid_scene_position(player) || settings.range_meters == 0
        || settings.marker_limit == 0) return result;
    for (std::size_t i = 0; i < std::min(markers.size(), kSceneMarkerCapacity)
         && result.count < settings.marker_limit; ++i) {
        const auto& marker = markers[i];
        if (marker.id <= 0 || !valid_scene_position(marker.position)
            || static_cast<unsigned>(marker.kind)
                > static_cast<unsigned>(SceneMarkerKind::MiniGame)) continue;
        const double distance = std::hypot(marker.position.x - player.x,
            marker.position.y - player.y, marker.position.z - player.z) / 100.0;
        if (!std::isfinite(distance) || distance > settings.range_meters) continue;
        result.values[result.count++] = {marker, distance, distance};
    }
    return result;
}

struct SceneProjectedPoint {
    double x{};
    double y{};
    bool in_front{};
};

struct SceneVisibleMarker {
    std::size_t candidate_index{};
    double x{};
    double y{};
    bool show_distance{};
};

struct SceneFocusIdentity {
    std::int64_t id{};
    SceneMarkerKind kind{SceneMarkerKind::TreasureOther};
};

[[nodiscard]] constexpr bool same_scene_focus(
    SceneFocusIdentity left, SceneFocusIdentity right) noexcept {
    return left.id > 0 && left.id == right.id
        && scene_identity_namespace(left.kind) == scene_identity_namespace(right.kind);
}

// A tall aiming region tolerates terrain/anchor height without requiring a
// precise vertical camera alignment. Both axes scale with the viewport's
// shorter side, so ultrawide screens do not turn Aim into Auto focus.
inline constexpr double kSceneAimHorizontalRadiusFraction = 0.16;
inline constexpr double kSceneAimVerticalRadiusFraction = 0.34;
inline constexpr std::uint64_t kSceneAimDwellMilliseconds = 100;
inline constexpr std::uint64_t kSceneAimSwitchMilliseconds = 350;
inline constexpr std::uint64_t kSceneAutoSwitchMilliseconds = 500;
inline constexpr double kSceneAimExitRadiusMultiplier = 1.2;

// Pure monotonic-time state, independent from transient projection slot order.
// In Aim this identity may still be dwelling, with no visible distance label.
struct SceneFocusState {
    SceneFocusIdentity identity{};
    std::uint64_t since_ms{};
    SceneDistanceMode mode{SceneDistanceMode::Off};
    bool acquired{};
    SceneFocusIdentity challenger{};
    std::uint64_t challenger_since_ms{};
    std::uint64_t last_update_ms{};
};

struct SceneFrame {
    std::array<SceneVisibleMarker, kSceneMarkerCapacity> values{};
    std::size_t count{};
    // The focused candidate receives the only distance label. No focus means
    // no distance text; category glyphs continue to show the visible anchors.
    std::size_t focus{std::numeric_limits<std::size_t>::max()};
    SceneFocusIdentity focus_identity{};
    SceneFocusState focus_state{};
};

// Input coordinates and viewport dimensions share engine-provided DPI-adjusted
// widget space. Behind-camera and off-screen points are hidden, never mirrored
// or clamped into misleading edge markers. Tight clusters retain nearer points.
[[nodiscard]] inline SceneFrame layout_scene_markers(
    const SceneSelection& selection,
    std::span<const SceneProjectedPoint> projected,
    double width, double height, SceneDisplaySettings settings = {},
    SceneFocusState previous_focus = {}, std::uint64_t now_ms = 0,
    std::span<const SceneFocusIdentity> previous_visible = {}) noexcept {
    SceneFrame frame{};
    settings = normalize_scene_display_settings(settings);
    if (!std::isfinite(width) || !std::isfinite(height)
        || width < 160.0 || height < 120.0 || settings.range_meters == 0
        || settings.marker_limit == 0) return frame;
    if (previous_focus.mode != settings.distance_mode
        || now_ms < previous_focus.last_update_ms
        || now_ms < previous_focus.since_ms) previous_focus = {};
    const double short_side = std::min(width, height);
    const double aim_radius_x = short_side * kSceneAimHorizontalRadiusFraction;
    const double aim_radius_y = short_side * kSceneAimVerticalRadiusFraction;
    double best_focus = std::numeric_limits<double>::infinity();
    double incumbent_distance = std::numeric_limits<double>::infinity();
    std::size_t incumbent = std::numeric_limits<std::size_t>::max();
    const std::size_t count = std::min({selection.count,
        selection.values.size(), projected.size(),
        static_cast<std::size_t>(settings.marker_limit)});
    for (std::size_t i = 0; i < count; ++i) {
        const auto& candidate = selection.values[i];
        if (candidate.marker.id <= 0 || !valid_scene_position(candidate.marker.position)
            || static_cast<unsigned>(candidate.marker.kind)
                > static_cast<unsigned>(SceneMarkerKind::MiniGame)
            || !std::isfinite(candidate.distance_meters)
            || candidate.distance_meters < 0
            || candidate.distance_meters > settings.range_meters) continue;
        const auto& point = projected[i];
        const bool was_visible = std::any_of(previous_visible.begin(), previous_visible.end(),
            [&candidate](const SceneFocusIdentity& identity) {
                return same_scene_focus(identity, {candidate.marker.id, candidate.marker.kind});
            });
        // Small spatial hysteresis suppresses flicker at screen/cluster edges
        // without delaying motion or retaining a marker behind the camera.
        const double margin = was_visible ? 44.0 : 52.0;
        if (!point.in_front || !std::isfinite(point.x) || !std::isfinite(point.y)
            || point.x < margin || point.x > width - margin
            || point.y < margin || point.y > height - margin) continue;
        bool crowded = false;
        for (std::size_t j = 0; j < frame.count; ++j) {
            const double dx = point.x - frame.values[j].x;
            const double dy = point.y - frame.values[j].y;
            const double separation = was_visible ? 32.0 : 40.0;
            if (dx * dx + dy * dy < separation * separation) {
                crowded = true;
                break;
            }
        }
        if (crowded) continue;
        const std::size_t slot = frame.count++;
        frame.values[slot] = {i, point.x, point.y,
            settings.distance_mode == SceneDistanceMode::All};
        if (settings.distance_mode == SceneDistanceMode::Off
            || settings.distance_mode == SceneDistanceMode::All) continue;
        const double dx = point.x - width * 0.5;
        const double dy = point.y - height * 0.5;
        const double aim_x = dx / aim_radius_x;
        const double aim_y = dy / aim_radius_y;
        const double center_distance = settings.distance_mode == SceneDistanceMode::CentralRadius
            ? aim_x * aim_x + aim_y * aim_y : dx * dx + dy * dy;
        const bool is_incumbent = same_scene_focus(
            {candidate.marker.id, candidate.marker.kind}, previous_focus.identity);
        const double exit_radius = is_incumbent && previous_focus.acquired
            ? kSceneAimExitRadiusMultiplier : 1.0;
        if (is_incumbent && (settings.distance_mode != SceneDistanceMode::CentralRadius
            || center_distance < exit_radius * exit_radius - 1e-12)) {
            incumbent = slot;
            incumbent_distance = center_distance;
        }
        if (settings.distance_mode == SceneDistanceMode::CentralRadius
            // Decimal radius fractions can place an exact edge one ULP
            // inside the ellipse. Keep the boundary consistently excluded.
            && center_distance >= 1.0 - 1e-12) continue;
        if (center_distance < best_focus) {
            best_focus = center_distance;
            frame.focus = slot;
        }
    }
    const auto identity_at = [&frame, &selection](std::size_t slot) {
        const auto& marker = selection.values[
            frame.values[slot].candidate_index].marker;
        return SceneFocusIdentity{marker.id, marker.kind};
    };
    const bool aim = settings.distance_mode == SceneDistanceMode::CentralRadius;
    if (incumbent < frame.count && previous_focus.acquired) {
        // Keep showing the current target while a clearly better challenger
        // settles. Separate identity-based timers avoid blanking the old label
        // or accumulating dwell across different targets during camera sweeps.
        auto state = previous_focus;
        state.challenger = {};
        state.challenger_since_ms = 0;
        const double minimum_advantage = aim ? 0.08 : short_side * 0.012;
        const double best_distance = std::sqrt(best_focus);
        const double current_distance = std::sqrt(incumbent_distance);
        const bool better = frame.focus < frame.count && frame.focus != incumbent
            && best_distance < current_distance * 0.8
            && current_distance - best_distance > minimum_advantage;
        std::size_t chosen = incumbent;
        if (better) {
            state.challenger = identity_at(frame.focus);
            state.challenger_since_ms = same_scene_focus(
                state.challenger, previous_focus.challenger)
                && now_ms >= previous_focus.challenger_since_ms
                ? previous_focus.challenger_since_ms : now_ms;
            const auto wait = aim ? kSceneAimSwitchMilliseconds : kSceneAutoSwitchMilliseconds;
            if (now_ms - state.challenger_since_ms >= wait) {
                chosen = frame.focus;
                state = {identity_at(chosen), now_ms, settings.distance_mode, true};
            }
        }
        state.last_update_ms = now_ms;
        frame.focus_state = state;
        frame.focus = chosen;
    } else if (frame.focus < frame.count) {
        const auto identity = identity_at(frame.focus);
        const auto since = same_scene_focus(identity, previous_focus.identity)
            ? previous_focus.since_ms : now_ms;
        const bool acquired = !aim || now_ms - since >= kSceneAimDwellMilliseconds;
        frame.focus_state = {identity, since, settings.distance_mode, acquired, {}, 0, now_ms};
        if (!acquired) frame.focus = std::numeric_limits<std::size_t>::max();
    }
    if (frame.focus < frame.count) {
        frame.values[frame.focus].show_distance = true;
        frame.focus_identity = identity_at(frame.focus);
    }
    return frame;
}

inline constexpr std::array<std::uint8_t, 10> kSceneDigitSegments{
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

[[nodiscard]] inline std::uint16_t scene_rounded_distance(double meters) noexcept {
    if (!std::isfinite(meters) || meters < 0.0) return 1001;
    return static_cast<std::uint16_t>(
        std::clamp(std::round(meters), 0.0, kSceneMaximumDistanceMeters));
}

[[nodiscard]] inline std::uint16_t scene_display_distance(
    const SceneCandidate& candidate) noexcept {
    if (!std::isfinite(candidate.distance_meters)) return 1001;
    // One total offset, clamped before rounding: nearby targets always read 0m,
    // never -1m, and neither selection nor the stored true distance is changed.
    return scene_rounded_distance(std::max(0.0,
        candidate.distance_meters - scene_distance_offset_meters(candidate.marker)));
}

[[nodiscard]] inline std::array<std::uint8_t, 4> scene_distance_digits(
    double meters) noexcept {
    const auto rounded = scene_rounded_distance(meters);
    if (rounded > 1000) return {};
    return {
        rounded >= 1000 ? kSceneDigitSegments[rounded / 1000] : std::uint8_t{0},
        rounded >= 100 ? kSceneDigitSegments[(rounded / 100) % 10] : std::uint8_t{0},
        rounded >= 10 ? kSceneDigitSegments[(rounded / 10) % 10] : std::uint8_t{0},
        kSceneDigitSegments[rounded % 10]};
}

[[nodiscard]] inline std::array<wchar_t, 8> scene_distance_text(
    std::uint16_t meters) noexcept {
    std::array<wchar_t, 8> text{};
    if (meters > 1000) return text;
    std::size_t index = 0;
    if (meters >= 1000) text[index++] = L'1';
    if (meters >= 100) text[index++] = static_cast<wchar_t>(L'0' + (meters / 100) % 10);
    if (meters >= 10) text[index++] = static_cast<wchar_t>(L'0' + (meters / 10) % 10);
    text[index++] = static_cast<wchar_t>(L'0' + meters % 10);
    text[index++] = L' ';
    text[index] = L'm';
    return text;
}

} // namespace dswros
