#pragma once

#include <algorithm>
#include <cstdint>

namespace dswros {

enum class SceneDistanceMode : std::uint8_t {
    Off, CentralRadius, NearestCenter, All,
};

struct SceneDisplaySettings {
    std::uint16_t range_meters{600};
    std::uint8_t marker_limit{24};
    SceneDistanceMode distance_mode{SceneDistanceMode::NearestCenter};
};

[[nodiscard]] constexpr SceneDisplaySettings normalize_scene_display_settings(
    SceneDisplaySettings settings) noexcept {
    settings.range_meters = std::min<std::uint16_t>(settings.range_meters, 1000);
    settings.marker_limit = std::min<std::uint8_t>(settings.marker_limit, 50);
    if (static_cast<unsigned>(settings.distance_mode)
        > static_cast<unsigned>(SceneDistanceMode::All))
        settings.distance_mode = SceneDistanceMode::NearestCenter;
    return settings;
}

} // namespace dswros
