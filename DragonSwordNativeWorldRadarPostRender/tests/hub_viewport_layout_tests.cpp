#include <dswros/hub_viewport_layout.hpp>

#include <array>
#include <iostream>
#include <limits>

namespace {
int checks{}, failures{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::cerr << message << '\n'; }
}
bool near(double a, double b) { return std::abs(a - b) < 1e-8; }
using dswros::compute_hub_viewport_layout;
}

int main() {
    const auto normal = compute_hub_viewport_layout(1920, 1080, 1);
    check(normal.valid && near(normal.display_scale, 1)
        && near(normal.panel_reference_height, 876)
        && near(normal.body_viewport_reference_height, 720),
        "1080p keeps full panel and ten-unit body bottom gap");
    check(near(normal.footer_reference_y, 818)
        && near(normal.modal_reference_y, 346), "full-height footer and compact modal anchors");
    const auto low = compute_hub_viewport_layout(1280, 720, 1);
    check(low.valid && near(low.display_scale, 1)
        && near(low.body_viewport_reference_height, 532),
        "720p scrolls body without shrinking 24-pixel controls");
    check(near(low.physical_top, 16) && near(low.footer_reference_y, 630),
        "short viewport preserves bottom margin and footer");
    const auto minimum = compute_hub_viewport_layout(640, 360, 1);
    check(minimum.valid && near(minimum.display_scale, 0.8)
        && near(minimum.panel_reference_height, 410)
        && near(minimum.body_viewport_reference_height, 254),
        "minimum supported viewport fits scrollable panel");
    check(near(minimum.physical_left, 16) && near(minimum.physical_top, 16)
        && near(24 * minimum.display_scale, 19.2),
        "minimum width limits scale with symmetric margins");
    const auto very_wide = compute_hub_viewport_layout(2560, 1080, 1);
    check(very_wide.valid && near(very_wide.display_scale, normal.display_scale)
        && near(very_wide.body_viewport_reference_height, normal.body_viewport_reference_height)
        && near(very_wide.physical_left, 900), "21:9 adds side space without stretching");
    const auto ultrawide = compute_hub_viewport_layout(3440, 1440, 1.5);
    check(ultrawide.valid && near(ultrawide.display_scale, 4.0 / 3.0)
        && near(ultrawide.unit_scale * 1.5, ultrawide.display_scale),
        "1440p ultrawide and DPI use one physical scale");
    const auto tall = compute_hub_viewport_layout(3840, 10000, 1);
    check(tall.valid && near(tall.display_scale, 2.5)
        && near(tall.panel_reference_height, 876), "large displays retain scale cap");

    const std::array<std::array<double, 2>, 8> viewports{{
        {640, 360}, {854, 480}, {1280, 720}, {1366, 768},
        {1920, 1080}, {2560, 1080}, {3440, 1440}, {3840, 2160}}};
    for (const auto& viewport : viewports) {
        const auto baseline = compute_hub_viewport_layout(viewport[0], viewport[1], 1);
        for (const double dpi : {0.1, 1.25, 2.0, 10.0}) {
            const auto layout = compute_hub_viewport_layout(viewport[0], viewport[1], dpi);
            check(layout.valid && near(layout.display_scale, baseline.display_scale)
                && near(layout.physical_left, baseline.physical_left)
                && near(layout.physical_top, baseline.physical_top)
                && near(layout.unit_scale * dpi, layout.display_scale),
                "DPI changes logical units without changing physical bounds");
            check(layout.physical_left >= 16 - 1e-8 && layout.physical_top >= 16 - 1e-8
                && near(2 * layout.physical_left + 760 * layout.display_scale, viewport[0])
                && near(2 * layout.physical_top + layout.panel_reference_height * layout.display_scale,
                        viewport[1])
                && layout.body_viewport_reference_height >= 160
                && near(98 + layout.body_viewport_reference_height, layout.footer_reference_y)
                && near(layout.footer_reference_y + 58, layout.panel_reference_height)
                && layout.modal_reference_y >= 0
                && layout.modal_reference_y + 184 <= layout.panel_reference_height,
                "body, fixed footer and modal fit the same viewport snapshot");
        }
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (const auto& input : std::array<std::array<double, 3>, 10>{{
        {639.99, 720, 1}, {1280, 359.99, 1}, {1280, 720, 0.099},
        {1280, 720, 10.001}, {nan, 720, 1}, {1280, nan, 1},
        {1280, 720, nan}, {inf, 720, 1}, {1280, inf, 1}, {1280, 720, inf}}}) {
        const auto invalid = compute_hub_viewport_layout(input[0], input[1], input[2]);
        check(!invalid.valid && invalid.display_scale == 0 && invalid.unit_scale == 0
            && invalid.panel_reference_height == 0, "invalid viewport never publishes partial geometry");
    }
    std::cout << "Hub viewport checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
