#pragma once

#include <algorithm>
#include <cmath>

namespace dswros {

struct HubViewportLayout {
    bool valid{};
    double display_scale{};
    double unit_scale{};
    double panel_reference_height{};
    double body_viewport_reference_height{};
    double footer_reference_y{};
    double modal_reference_y{};
    double physical_left{};
    double physical_top{};
};

// Keep header/footer controls at their reference size when width permits.
// Only the body viewport contracts on a short display; its 720-unit content
// scrolls independently. All positions returned here describe the same frame.
[[nodiscard]] inline HubViewportLayout compute_hub_viewport_layout(
    double width, double height, double dpi) noexcept {
    if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(dpi)
        || width < 640.0 || height < 360.0 || dpi < 0.1 || dpi > 10.0) {
        return {};
    }
    constexpr double panel_width = 760.0;
    constexpr double full_height = 876.0;
    constexpr double header_height = 98.0;
    constexpr double footer_height = 58.0;
    constexpr double margin = 16.0;
    constexpr double modal_height = 184.0;
    const double display = std::min(std::clamp(height / 1080.0, 1.0, 2.5),
                                    (width - 2.0 * margin) / panel_width);
    const double panel_height = std::min(full_height,
                                         (height - 2.0 * margin) / display);
    const double body_height = panel_height - header_height - footer_height;
    if (!std::isfinite(display) || display <= 0.0
        || !std::isfinite(body_height) || body_height < 160.0) {
        return {};
    }
    return {true, display, display / dpi, panel_height, body_height,
        panel_height - footer_height, (panel_height - modal_height) * 0.5,
        (width - panel_width * display) * 0.5,
        (height - panel_height * display) * 0.5};
}

} // namespace dswros
