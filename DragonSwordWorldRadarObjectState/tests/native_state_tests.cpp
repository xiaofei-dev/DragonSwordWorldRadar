#include <dswros/object_state.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

dswros::ObjectStateTracker tracker() {
    dswros::ObjectStateTracker value;
    value.set_catalog({
        {1001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
        {1002, "TreasureBox02_C", {8000.0, 9000.0, 1000.0}},
    });
    value.reset(7, 11);
    return value;
}

} // namespace

int main() {
    {
        auto value = tracker();
        const auto id = value.observe({4, 8}, "TreasureBox01_C", {1001.0, 1998.0, 2999.0});
        require(id && *id == 1001, "exact class and nearby coordinate must match");
        const auto event = value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 7, 11);
        require(event && event->kind == dswros::EventKind::TreasureOpened && event->id == 1001,
                "destroyed observed nearby treasure must publish opened event");
    }
    {
        dswros::DisappearanceConfirmation gate;
        require(!gate.sample(false, true), "first missing sample must not complete");
        require(gate.sample(false, true), "second missing sample must complete");
        gate.reset();
        require(!gate.sample(false, true), "reset must clear missing history");
        require(!gate.sample(true, true), "reappearance must cancel missing history");
        require(!gate.sample(false, false), "invalid context must fail closed");
        gate.mark_eligible_end();
        require(gate.sample(false, true), "eligible EndPlay plus missing probe must complete");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, false, false, {1000.0, 2000.0, 3000.0}, 7, 11),
                "streaming removal must not publish opened event");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, true, {1000.0, 2000.0, 3000.0}, 7, 11),
                "transition destruction must fail closed");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 8, 11),
                "stale activation must not publish");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {10000.0, 2000.0, 3000.0}, 7, 11),
                "distant destruction must not publish");
    }
    {
        auto value = tracker();
        require(!value.observe({4, 8}, "TreasureBox02_C", {1000.0, 2000.0, 3000.0}),
                "wrong exact class must not match");
        require(!value.observe({4, 8}, "TreasureBox01_C", {2000.0, 2000.0, 3000.0}),
                "coordinate outside match radius must not match");
    }
    {
        auto value = tracker();
        const auto nearby = value.nearby_classes({900.0, 2000.0, 3000.0}, 3000.0);
        require(nearby.size() == 1 && nearby.front() == "TreasureBox01_C",
                "catch-up must request only classes near the player");
    }
    std::cout << "NATIVE_STATE_TESTS_OK assertions=17\n";
    return 0;
}
