#include <dsnap/action_evidence.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/candidate_queue.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/discovery_state.hpp>
#include <dsnap/gate_attribution.hpp>
#include <dsnap/pickup_controller.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/runtime_contract.hpp>
#include <dsnap/session_calibration.hpp>
#include <dsnap/single_target_latch.hpp>
#include <dsnap/windows_fingerprint.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
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
read_only_diagnostic=true
toggle_hotkey=F9
radius_meters=5.0
max_queue=64
perf_log_interval_seconds=30
)");
    require(valid.valid(), "valid configuration should parse");
    require(valid.value.radius_meters == 5.0, "radius should parse");

    const auto unsafe = dsnap::parse_configuration_text("enabled_on_launch=true\nread_only_diagnostic=true\ntoggle_hotkey=F7\n");
    require(!unsafe.valid(), "unsafe defaults and Radar hotkey conflict must fail");
    const auto unsupported_key = dsnap::parse_configuration_text("enabled_on_launch=false\nread_only_diagnostic=true\ntoggle_hotkey=F10\n");
    require(!unsupported_key.valid(), "evidence build must reject any key other than F9");
    const auto legacy = dsnap::parse_configuration_text("enabled_on_launch=false\npassive_observation=true\ntoggle_hotkey=F9\n");
    require(legacy.valid(), "a prior passive=true config must remain a safe backward-compatible alias");
    const auto canary_legacy = dsnap::parse_configuration_text("enabled_on_launch=false\nsingle_target_canary=true\ntoggle_hotkey=F9\n");
    require(canary_legacy.valid(), "a prior canary=true config must remain safely diagnostic-only");
    const auto diagnostic_off = dsnap::parse_configuration_text("enabled_on_launch=false\nread_only_diagnostic=false\ntoggle_hotkey=F9\n");
    require(!diagnostic_off.valid(), "diagnostic mode cannot be disabled into an undefined active mode");
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
    dsnap::InteractionContract contract{true, 1};
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
    dsnap::InteractionContract contract{true, 1};
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
    dsnap::PickupController no_contract{config, {false, 0}};
    no_contract.set_build_trusted(true);
    require(!no_contract.request_active(true), "unaccepted capture contract must fail closed");

    dsnap::PickupController unknown_build{config, {true, 1}};
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

void test_complete_gate_reason_attribution() {
    constexpr std::array expected{
        std::string_view{"owner_invalid"}, std::string_view{"class_invalid"},
        std::string_view{"interact_component_missing"}, std::string_view{"component_ownership_mismatch"},
        std::string_view{"state_field_missing"}, std::string_view{"state_value_mismatch"},
        std::string_view{"owner_location_unavailable"}, std::string_view{"non_finite_distance"},
        std::string_view{"outside_radius"}, std::string_view{"eligible"},
        std::string_view{"guarded_evaluation_exception"},
    };
    require(expected.size() == dsnap::kGateReasonCount, "every gate reason must have an aggregate slot");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto reason = static_cast<dsnap::GateReason>(index);
        require(dsnap::gate_reason_index(reason) == index, "gate reason indices must remain stable");
        require(dsnap::gate_reason_name(reason) == expected[index], "every gate reason must have a stable name");
    }
}

void test_bounded_change_only_gate_diagnostics() {
    dsnap::BoundedGateDiagnosticState state{};
    dsnap::GateObservation observation{.reason = dsnap::GateReason::InteractComponentMissing};
    require(state.should_log(observation), "first gate observation must log");
    require(!state.should_log(observation), "unchanged gate observation must not log again");
    for (std::size_t index = 1; index < dsnap::kMaxGateDiagnosticLogsPerCandidate; ++index) {
        observation.reason = index % 2 == 0 ? dsnap::GateReason::OutsideRadius : dsnap::GateReason::StateFieldMissing;
        observation.distance_meters = static_cast<double>(index);
        require(state.should_log(observation), "changed observation should log before the bound");
    }
    observation.reason = dsnap::GateReason::Eligible;
    require(!state.should_log(observation), "diagnostic logging must stop at the per-candidate bound");
    require(state.emitted() == dsnap::kMaxGateDiagnosticLogsPerCandidate, "diagnostic bound must be exact");
}

void test_complete_player_chain_reason_attribution() {
    constexpr std::array expected{
        std::string_view{"engine_or_output_invalid"}, std::string_view{"game_viewport_property_missing"},
        std::string_view{"game_viewport_value_null"}, std::string_view{"game_instance_property_missing"},
        std::string_view{"game_instance_value_null"}, std::string_view{"local_players_property_missing"},
        std::string_view{"local_players_invalid_index"}, std::string_view{"local_players_null_data"},
        std::string_view{"local_player_entry_null"}, std::string_view{"local_player_controller_property_missing"},
        std::string_view{"local_player_controller_value_null"}, std::string_view{"controller_class_mismatch"},
        std::string_view{"controller_player_property_missing"}, std::string_view{"controller_player_identity_mismatch"},
        std::string_view{"pawn_property_missing"}, std::string_view{"pawn_value_null"},
        std::string_view{"player_controller_property_missing"},
        std::string_view{"player_controller_identity_mismatch"}, std::string_view{"location_unavailable"},
        std::string_view{"success"}, std::string_view{"guarded_exception"},
    };
    require(expected.size() == dsnap::kPlayerChainReasonCount, "every player-chain exit must have an aggregate slot");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto reason = static_cast<dsnap::PlayerChainReason>(index);
        require(dsnap::player_chain_reason_index(reason) == index, "player-chain reason indices must remain stable");
        require(dsnap::player_chain_reason_name(reason) == expected[index], "every player-chain reason needs a stable name");
    }
}

void test_bounded_player_chain_diagnostics() {
    dsnap::BoundedPlayerChainDiagnosticState state{};
    require(state.should_log(dsnap::PlayerChainReason::PawnValueNull), "first player-chain reason must log");
    require(!state.should_log(dsnap::PlayerChainReason::PawnValueNull), "unchanged player-chain reason must not repeat");
    for (std::size_t index = 1; index < dsnap::kMaxPlayerChainDiagnosticLogs; ++index) {
        const auto reason = index % 2 == 0 ? dsnap::PlayerChainReason::PawnValueNull
                                          : dsnap::PlayerChainReason::LocationUnavailable;
        require(state.should_log(reason), "changed player-chain reason should log before the bound");
    }
    require(!state.should_log(dsnap::PlayerChainReason::Success), "player-chain logging must stop at its bound");
}

void test_monotonic_f9_debounce() {
    using namespace std::chrono_literals;
    dsnap::MonotonicDebounce debounce{250ms};
    const auto start = std::chrono::steady_clock::time_point{1s};
    require(debounce.accept(start), "first F9 transition must be accepted");
    require(!debounce.accept(start), "same-timestamp F9 repeat must be rejected");
    require(!debounce.accept(start + 249ms), "F9 repeat inside 250 ms must be rejected");
    require(debounce.accept(start + 250ms), "F9 transition at 250 ms must be accepted");
}

void test_controlled_alternate_pawn_acceptance() {
    const auto expected = dsnap::classify_controlled_pawn(true, true, true);
    require(expected == dsnap::PlayerMode::ExpectedCharacter, "expected character should retain its mode");
    const auto mounted = dsnap::classify_controlled_pawn(false, true, true);
    require(mounted == dsnap::PlayerMode::ControllerBoundAlternatePawn,
            "controller-bound alternate Pawn should be accepted for mounted play");
    require(!dsnap::classify_controlled_pawn(false, false, true).has_value(),
            "alternate Pawn without controller identity must fail closed");
    require(!dsnap::classify_controlled_pawn(false, true, false).has_value(),
            "alternate Pawn without a fresh location must fail closed");
}

void test_pending_action_confirmation() {
    using namespace std::chrono_literals;
    dsnap::PendingActionTracker tracker{};
    const dsnap::WeakObjectId candidate{42, 9};
    const auto now = std::chrono::steady_clock::time_point{1s};
    require(tracker.begin(candidate, now, 1.5), "first action may enter pending state");
    require(!tracker.begin({43, 10}, now, 2.0), "a second action must not start while one is pending");
    require(!tracker.confirm_delete({42, 10}).has_value(), "different serial must not confirm the pending action");
    require(tracker.pending(), "unrelated delete must preserve pending interest");
    require(tracker.confirm_delete(candidate).has_value(), "same candidate delete must confirm the action");
    require(!tracker.pending(), "confirmed action must leave pending state");
}

void test_pending_action_timeout_is_terminal() {
    using namespace std::chrono_literals;
    dsnap::PendingActionTracker tracker{};
    const dsnap::WeakObjectId candidate{7, 11};
    const auto now = std::chrono::steady_clock::time_point{2s};
    require(tracker.begin(candidate, now, 4.0), "action should enter pending state");
    require(!tracker.expire(now + dsnap::kActionConfirmationWindow - 1ms).has_value(), "pending action must retain its full window");
    const auto expired = tracker.expire(now + dsnap::kActionConfirmationWindow);
    require(expired.has_value() && expired->candidate == candidate, "timeout must return the exact pending candidate");
    require(!tracker.pending(), "timeout must clear pending state without retry");
}

void test_distinct_interaction_capture_logging() {
    dsnap::DistinctInteractionCaptureLog log{};
    require(log.first(dsnap::InteractionSource::Manual, 13), "first manual source/key tuple must log");
    require(!log.first(dsnap::InteractionSource::Manual, 13), "duplicate manual tuple must be suppressed");
    require(log.first(dsnap::InteractionSource::Automatic, 13), "automatic source is a distinct tuple");
    require(log.first(dsnap::InteractionSource::Manual, 14), "different key action is a distinct tuple");
}

dsnap::RuntimeContract valid_runtime_contract() {
    dsnap::RuntimeContract contract{};
    contract.game_sha256 = std::string(64, 'A');
    contract.ue4ss_sha256 = std::string(64, 'B');
    contract.ue4ss_git_sha = "1c1a1497f942c707f47ba668db75b25e86f6c08a";
    contract.function_path = "/Game/Blueprints/BP_DsPlayerController.BP_DsPlayerController_C:OnPressInteractionButton";
    contract.function = dsnap::ReplayFunction::ControllerPressInteractionButton;
    contract.receiver = dsnap::ReceiverRole::CurrentController;
    contract.parameter_count = 2;
    contract.parameters[0] = {dsnap::stable_name_hash("InteractActor"), dsnap::ParameterKind::CandidateObject, 0};
    contract.parameters[1] = {dsnap::stable_name_hash("CurrentPC"), dsnap::ParameterKind::CurrentController, 0};
    return contract;
}

void test_runtime_contract_validation_and_persistence() {
    const auto contract = valid_runtime_contract();
    require(contract.structurally_valid(), "canonical runtime contract should be valid");
    const auto serialized = dsnap::serialize_contract(contract);
    const auto parsed = dsnap::parse_contract(serialized);
    require(parsed.has_value() && *parsed == contract, "runtime contract must round-trip canonically");
    require(dsnap::fingerprint_matches(*parsed, contract.game_sha256, contract.ue4ss_sha256, contract.ue4ss_git_sha),
            "exact fingerprints should validate");
    require(!dsnap::parse_contract(serialized + "schema=12\n").has_value(), "duplicate keys must fail closed");
    require(!dsnap::parse_contract("schema=12\nunknown=x\n").has_value(), "unknown keys must fail closed");
    auto malformed = contract;
    malformed.parameters[0].name_hash = 0;
    require(!malformed.structurally_valid(), "zero parameter hashes must fail closed");
    malformed = contract;
    malformed.parameters[0].kind = static_cast<dsnap::ParameterKind>(99);
    require(!malformed.structurally_valid(), "unknown parameter kinds must fail closed");
    malformed = contract;
    malformed.function_path = "relative:OnPressInteractionButton";
    require(!malformed.structurally_valid(), "relative function paths must fail closed");
    malformed = contract;
    malformed.function_path = "/Game/Invalid\nFunction:OnPressInteractionButton";
    require(!malformed.structurally_valid(), "control characters in function paths must fail closed");
    require(!dsnap::parse_contract(serialized.substr(0, serialized.find("git=") + 4) + std::string(40, 'Z') +
                                   "\nfunction=6\npath=/Script/DS.DsPlayerController:OnPressInteractionButton\nreceiver=2\nreceiver_property=0\ncount=0\n").has_value(),
            "non-hex git fingerprints must fail closed");

    auto property_receiver = contract;
    property_receiver.function = dsnap::ReplayFunction::ServerRunInteractV2;
    property_receiver.function_path = "/Script/DS.DInteractableComponent:Server_RunInteractV2";
    property_receiver.receiver = dsnap::ReceiverRole::CurrentControllerProperty;
    property_receiver.receiver_property_hash = dsnap::stable_name_hash("PickupInteractComponent");
    property_receiver.parameter_count = 0;
    property_receiver.parameters = {};
    require(property_receiver.structurally_valid(), "exact controller property receiver should be valid");
    const auto property_serialized = dsnap::serialize_contract(property_receiver);
    const auto property_parsed = dsnap::parse_contract(property_serialized);
    require(property_parsed.has_value() && *property_parsed == property_receiver,
            "property-bound receiver contract must round-trip canonically");
    property_receiver.receiver_property_hash = 0;
    require(!property_receiver.structurally_valid(), "property receiver must require a nonzero property hash");
    property_receiver = contract;
    property_receiver.receiver_property_hash = dsnap::stable_name_hash("UnexpectedProperty");
    require(!property_receiver.structurally_valid(), "direct receiver must reject a stray property hash");

    const auto root = std::filesystem::temp_directory_path() / "dsnap-contract-test";
    const auto path = root / "contract";
    require(dsnap::atomic_replace_text(path, serialized), "atomic contract persistence should succeed");
    std::ifstream input{path, std::ios::binary};
    const std::string persisted{std::istreambuf_iterator<char>{input}, {}};
    require(persisted == serialized, "atomic persistence must preserve exact canonical bytes");
    std::error_code error; std::filesystem::remove_all(root, error);
}

void test_calibration_state_and_true_edge() {
    dsnap::CalibrationStateMachine state{};
    require(state.press_f9(true, false) == dsnap::RuntimeState::Calibrating, "missing contract should enter calibration");
    state.calibration_validated();
    require(state.state() == dsnap::RuntimeState::ArmedReady, "positive delete validation should arm ready");
    state.invalidate_contract();
    require(state.press_f9(true, false) == dsnap::RuntimeState::Off, "invalid state must remain disarmable");
    require(state.press_f9(true, false) == dsnap::RuntimeState::Calibrating, "later F9 should permit fresh calibration");
    dsnap::RisingEdgeLatch edge{};
    require(edge.key_down(), "first physical down is a rising edge");
    require(!edge.key_down(), "OS repeat while held must not toggle");
    edge.key_up(); require(edge.key_down(), "release enables the next physical press");
}

void test_discovery_epoch_and_cancellation() {
    dsnap::IncrementalDiscoveryState discovery{};
    discovery.begin(7, 100);
    require(discovery.next(30) == std::pair<std::int32_t, std::int32_t>{0, 30}, "first discovery batch should be bounded");
    discovery.commit(20);
    require(discovery.next(30) == std::pair<std::int32_t, std::int32_t>{20, 50}, "time-budget stop must resume without gaps");
    require(discovery.epoch() == 7, "discovery must retain its world epoch");
    discovery.cancel(); require(!discovery.active(), "disarm must cancel discovery");
}

void test_single_target_invocation_latch() {
    dsnap::SingleTargetInvocationLatch latch{};
    const dsnap::WeakObjectId first{101, 7};
    const dsnap::WeakObjectId second{102, 8};

    require(latch.observe(std::nullopt).decision == dsnap::TargetObservationDecision::NoTarget,
            "an empty latch with no target must remain idle");
    require(latch.observe(first).decision == dsnap::TargetObservationDecision::Ready,
            "a fresh valid target must be ready");
    require(latch.mark_invoked(first), "the first target may be marked exactly once");
    require(!latch.mark_invoked(first), "the same pending target must not be marked twice");
    require(latch.observe(first).decision == dsnap::TargetObservationDecision::Debounce,
            "an unchanged target must debounce every later pulse");

    const auto changed = latch.observe(second);
    require(changed.decision == dsnap::TargetObservationDecision::PreviousChanged &&
                changed.previous == first && changed.current == second,
            "a different target must release the previous latch and report both identities");
    require(latch.mark_invoked(second), "the changed target may be invoked once after validation");
    const auto cleared = latch.observe(std::nullopt);
    require(cleared.decision == dsnap::TargetObservationDecision::PreviousCleared &&
                cleared.previous == second,
            "a cleared game target must confirm the pending target transition");
    require(!latch.pending(), "target clearing must leave no pending invocation");
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
    test_complete_gate_reason_attribution();
    test_bounded_change_only_gate_diagnostics();
    test_complete_player_chain_reason_attribution();
    test_bounded_player_chain_diagnostics();
    test_monotonic_f9_debounce();
    test_controlled_alternate_pawn_acceptance();
    test_pending_action_confirmation();
    test_pending_action_timeout_is_terminal();
    test_distinct_interaction_capture_logging();
    test_runtime_contract_validation_and_persistence();
    test_calibration_state_and_true_edge();
    test_discovery_epoch_and_cancellation();
    test_single_target_invocation_latch();
    std::cout << "All DragonSwordNativeAutoPickup core tests passed.\n";
    return 0;
}
