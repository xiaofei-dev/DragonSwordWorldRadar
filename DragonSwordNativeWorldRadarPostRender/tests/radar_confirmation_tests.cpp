#include <dswros/radar_confirmation.hpp>

#include <iostream>

namespace {
int checks{}, failures{};
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
using namespace dswros;
}

int main() {
    RadarConfirmation modal;
    check(!modal.active(), "starts idle");
    check(!modal.begin(RadarConfirmationAction::None), "None cannot open a dialog");
    check(!modal.begin(static_cast<RadarConfirmationAction>(255)), "unknown intent rejected");
    check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
          "idle Yes cannot dispatch");
    for (auto action : {RadarConfirmationAction::Endorse, RadarConfirmationAction::BugReport,
                        RadarConfirmationAction::RestoreDefaults}) {
        check(modal.begin(action), "every authorized intent opens");
        check(modal.pending() == action, "pending identity retained");
        check(!modal.begin(RadarConfirmationAction::RestoreDefaults),
              "another click cannot overwrite pending identity");
        check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
              "opening gesture cannot approve");
        check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
              "held or repeated initial Yes cannot approve");
        check(!modal.sample(false, false).dismissed, "neutral sample arms without action");
        const auto approved = modal.sample(true, false);
        check(approved.dismissed && approved.confirmed == action, "fresh Yes consumes exact intent");
        check(!modal.active(), "intent cleared before side effect");
        check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
              "double click dispatches at most once");

        check(modal.begin(action), "cancel scenario opens");
        const auto cancelled = modal.sample(true, true);
        check(cancelled.dismissed && cancelled.confirmed == RadarConfirmationAction::None,
              "No wins even with simultaneous Yes");
        check(!modal.active(), "No clears intent only");

        check(modal.begin(action), "lifecycle scenario opens");
        (void)modal.sample(false, false);
        modal.clear(); // Esc, F6 close, focus loss or travel.
        check(!modal.active() && modal.pending() == RadarConfirmationAction::None,
              "lifecycle cancel drops pending identity");
        check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
              "late response after lifecycle cancel cannot execute");
        check(modal.begin(action), "reopen after cancellation");
        check(modal.sample(true, false).confirmed == RadarConfirmationAction::None,
              "reopen must rearm");
        modal.clear();
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
