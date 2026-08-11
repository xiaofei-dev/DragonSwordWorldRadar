#include <dsnap/callback_generation.hpp>
#include <dsnap/candidate_queue.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/pickup_controller.hpp>
#include <dsnap/session_calibration.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

dsnap::CandidateValidation valid_candidate(double distance = 2.0) {
    return dsnap::CandidateValidation{
        .object_valid = true,
        .exact_allowed_class = true,
        .current_world = true,
        .pawn_valid = true,
        .controller_valid = true,
        .component_valid = true,
        .interactable_state_proven = true,
        .interact_type = dsnap::kDropItemInteractType,
        .distance_meters = distance,
    };
}

void test_configuration() {
    const auto valid = dsnap::parse_configuration_text(R"(
[auto_pickup]
enabled_on_launch=false
passive_observation=true
toggle_hotkey=F9
radius_meters=5.0
max_queue=64
max_retries=2
action_interval_ms=200
retry_backoff_ms=700
perf_log_interval_seconds=30
)");
    require(valid.valid(), "valid configuration should parse");
    require(valid.value.radius_meters == 5.0, "radius should parse");

    const auto unsafe = dsnap::parse_configuration_text("enabled_on_launch=true\npassive_observation=true\ntoggle_hotkey=F7\n");
    require(!unsafe.valid(), "unsafe defaults and Radar hotkey conflict must fail");
    const auto unsupported_key = dsnap::parse_configuration_text("enabled_on_launch=false\npassive_observation=true\ntoggle_hotkey=F10\n");
    require(!unsupported_key.valid(), "evidence build must reject any key other than F9");
}

void test_session_calibration() {
    dsnap::SessionCalibration calibration{};
    require(!calibration.observe(10, false), "rejected manual calls must not calibrate");
    require(!calibration.observe(10, true), "first exact object starts calibration");
    require(calibration.matches() == 1, "first distinct match should be recorded once");
    require(!calibration.observe(10, true), "same object cannot satisfy the second sample");
    require(calibration.observe(11, true), "second distinct exact object accepts the session contract");
    require(calibration.accepted(), "session contract should remain accepted");
    require(calibration.matches() == 2, "accepted calibration requires exactly two distinct objects");
    require(!calibration.observe(12, true), "accepted session must not emit a second acceptance transition");
}

void test_callback_generation_gate() {
    dsnap::CallbackGenerationGate first{10};
    require(first.accepts(10), "current callback generation should be accepted");
    require(!first.accepts(9), "old callback generation must be rejected");
    require(!first.accepts(11), "future callback generation must be rejected");
    first.invalidate();
    require(!first.accepts(10), "invalidated instance must reject its own callbacks");

    dsnap::CallbackGenerationGate second{11};
    require(!second.accepts(10), "new instance must reject accumulated old callbacks");
    require(second.accepts(11), "new instance accepts only its own callback token");
}

void test_whitelist_and_bounds() {
    dsnap::CandidateQueue queue{2, 2, std::chrono::milliseconds{500}};
    queue.reset(5);
    require(!queue.register_candidate({1, 1}, "/Script/DS.TreasureBox", 5), "chests must be rejected");
    require(!queue.register_candidate({1, 1}, dsnap::kAllowedClassPath, 4), "stale epochs must be rejected");
    require(queue.register_candidate({1, 1}, dsnap::kAllowedClassPath, 5), "first allowed candidate should register");
    require(queue.register_candidate({2, 1}, dsnap::kAllowedClassPath, 5), "second allowed candidate should register");
    require(!queue.register_candidate({3, 1}, dsnap::kAllowedClassPath, 5), "queue bound must be enforced");
}

void test_epoch_invalidation() {
    dsnap::Configuration config{};
    dsnap::InteractionContract contract{true, dsnap::kDropItemKeyActionEnumCandidate, true, true};
    dsnap::PickupController controller{config, contract};
    controller.set_build_trusted(true);
    const dsnap::WeakObjectId object{10, 20};
    require(controller.observe_candidate(object, dsnap::kAllowedClassPath), "candidate should register in current epoch");
    require(controller.update_candidate(object, valid_candidate()), "candidate should validate");
    require(controller.request_active(true), "approved test-only contract should permit active state");
    require(controller.plan_action(std::chrono::steady_clock::now()).has_value(), "eligible candidate should plan");
    controller.reset_world();
    require(!controller.active(), "world reset must disable active mode");
    require(controller.queue_size() == 0, "world reset must clear candidates");
}

void test_fail_closed_validation() {
    dsnap::Configuration config{};
    dsnap::InteractionContract contract{true, dsnap::kDropItemKeyActionEnumCandidate, true, true};
    dsnap::PickupController controller{config, contract};
    controller.set_build_trusted(true);
    const dsnap::WeakObjectId object{12, 4};
    require(controller.observe_candidate(object, dsnap::kAllowedClassPath), "candidate should register");
    auto invalid = valid_candidate();
    invalid.interact_type = 4;
    require(controller.update_candidate(object, invalid), "invalid candidate state should still be recorded");
    require(controller.request_active(true), "test-only contract should activate");
    require(!controller.plan_action(std::chrono::steady_clock::now()).has_value(), "TreasureBox interact type must not plan");
}

void test_contract_and_build_gates() {
    dsnap::Configuration config{};
    dsnap::PickupController no_contract{config, {false, 0, false, false}};
    no_contract.set_build_trusted(true);
    require(!no_contract.request_active(true), "unaccepted capture contract must fail closed");

    dsnap::PickupController unknown_build{config, {true, dsnap::kDropItemKeyActionEnumCandidate, true, true}};
    require(!unknown_build.request_active(true), "unknown build must fail closed");
}

void test_retry_backoff() {
    dsnap::CandidateQueue queue{4, 2, std::chrono::milliseconds{500}};
    queue.reset(1);
    const dsnap::WeakObjectId object{7, 9};
    require(queue.register_candidate(object, dsnap::kAllowedClassPath, 1), "candidate should register");
    require(queue.update_validation(object, valid_candidate()), "candidate should validate");
    const auto now = std::chrono::steady_clock::now();
    require(queue.next_eligible(now, 1, 4.5).has_value(), "candidate should initially be eligible");
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now);
    require(!queue.next_eligible(now + std::chrono::milliseconds{499}, 1, 4.5).has_value(), "first backoff must hold");
    require(queue.next_eligible(now + std::chrono::milliseconds{500}, 1, 4.5).has_value(), "first backoff should expire");
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now + std::chrono::milliseconds{500});
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now + std::chrono::milliseconds{1500});
    require(queue.size() == 0, "candidate must be removed after retry limit");
}

} // namespace

int main() {
    test_configuration();
    test_session_calibration();
    test_callback_generation_gate();
    test_whitelist_and_bounds();
    test_epoch_invalidation();
    test_fail_closed_validation();
    test_contract_and_build_gates();
    test_retry_backoff();
    std::cout << "All DragonSwordNativeAutoPickup core tests passed.\n";
    return 0;
}
