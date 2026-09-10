#pragma once

#include <dswros/compact_render_model.hpp>

#include <cmath>

namespace dswros {

// Keep the renderer's enum independent of Unreal headers in scalar-only tests.
template <class MarkerKind>
[[nodiscard]] constexpr bool is_encounter_height_marker(
    MarkerKind kind) noexcept {
    return kind == MarkerKind::Boss || kind == MarkerKind::Assault;
}

// Encounter guidance refers to the authored spawn point, not a moving actor.
// A missing source remains unknown; it must never become a fabricated Z=0.
[[nodiscard]] inline AreaQuestHeightProfile encounter_spawn_height_profile(
    double spawn_z) noexcept {
    AreaQuestHeightProfile profile{};
    if (std::isfinite(spawn_z)) {
        profile.bands[0] = {spawn_z, spawn_z};
        profile.band_count = 1;
    }
    return profile;
}

[[nodiscard]] inline bool encounter_height_profile_valid(
    const AreaQuestHeightProfile& profile) noexcept {
    return profile.band_count == 1
        && area_quest_height_profile_valid(profile)
        && profile.bands[0].minimum_z == profile.bands[0].maximum_z;
}

[[nodiscard]] inline AreaQuestHeightIndicatorShape
encounter_height_indicator_shape(
    const AreaQuestHeightProfile& profile,
    double comparable_player_z) noexcept {
    return encounter_height_profile_valid(profile)
        ? area_quest_height_indicator_shape(profile, comparable_player_z)
        : AreaQuestHeightIndicatorShape::Unavailable;
}

} // namespace dswros
