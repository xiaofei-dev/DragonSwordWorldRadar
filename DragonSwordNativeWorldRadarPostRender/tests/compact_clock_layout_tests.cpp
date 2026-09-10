#include <dswros/compact_clock_layout.hpp>

#include <cmath>
#include <iostream>
#include <limits>

namespace {
int checks{};
int failures{};
void check(bool value, const char* label) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool close(double a, double b) { return std::abs(a - b) < 1.0e-9; }
using namespace dswros;
}

int main() {
    const CompactClockRect map{72, 72, 412, 412};
    const CompactClockRect quest{42, 472, 472, 700};
    const CompactClockRect host{0, 0, 484.5, 484.5};
    const auto position = calculate_compact_clock_gap_position(map, quest, host, 1);
    check(position.has_value(), "a measured gap fits retained host");
    check(position && close(position->left + 58, 242), "centred under minimap");
    check(position && close(position->top + 15, 442), "ink centre equals actual gap midpoint");
    check(position && !close(position->top + 21, 442), "container padding must not bias visible centre");

    for (double scale : {0.25, 0.5, 0.75, 1.0, 1.25, 2.0, 4.0}) {
        const auto scaled = [scale](CompactClockRect r) {
            return CompactClockRect{r.left * scale, r.top * scale,
                                    r.right * scale, r.bottom * scale};
        };
        const auto value = calculate_compact_clock_gap_position(
            scaled(map), scaled(quest), scaled(host), scale);
        check(value && close(value->left, position->left * scale)
            && close(value->top, position->top * scale), "DPI/content scale preserves midpoint");
    }
    check(!calculate_compact_clock_gap_position(map, {42, 441.99, 472, 700}, host, 1),
          "insufficient ink clearance fails closed");
    check(calculate_compact_clock_gap_position(map, {42, 442, 472, 700}, host, 1).has_value(),
          "exact thirty-unit gap accepts two-unit clearance");
    check(!calculate_compact_clock_gap_position(map, {42, 400, 472, 700}, host, 1),
          "overlapping minimap/task rejects stale or full-screen geometry");
    check(!calculate_compact_clock_gap_position(map, {500, 472, 800, 700}, host, 1),
          "unrelated task column rejected");
    check(!calculate_compact_clock_gap_position({220, 72, 260, 412}, quest, host, 1),
          "clock does not fit narrower minimap");
    check(!calculate_compact_clock_gap_position(map, {42, 600, 472, 700}, host, 1),
          "large gap cannot escape host clip budget");
    check(!calculate_compact_clock_gap_position(map, quest, {190, 0, 484.5, 484.5}, 1),
          "viewport clipping bound enforced");
    check(!calculate_compact_clock_gap_position({72, 72, 72, 412}, quest, host, 1),
          "zero-sized cached geometry rejected");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    check(!calculate_compact_clock_gap_position({nan, 72, 412, 412}, quest, host, 1),
          "NaN geometry rejected");
    check(!calculate_compact_clock_gap_position(map, {42, 472, inf, 700}, host, 1),
          "infinite geometry rejected");
    for (double scale : {0.0, -1.0, nan, inf})
        check(!calculate_compact_clock_gap_position(map, quest, host, scale),
              "invalid content scale rejected");

    CompactClockPosition previous{100, 200};
    check(!compact_clock_position_changed(previous, {100.25, 200.25}, 1),
          "quarter-reference jitter does not mutate layout");
    // The caller retains the last applied position, not the last sample.
    check(compact_clock_position_changed(previous, {100.30, 200.30}, 1),
          "small cumulative movement eventually applies");
    check(!compact_clock_position_changed({50, 100}, {50.125, 100.125}, 0.5),
          "position tolerance scales with content");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
