// OWNER_AUTHORIZED_READ_ONLY_DIAGNOSTIC targeting pinned RE-UE4SS v3.0.1.
// This build records one exact manual DropItemActor interaction and never
// invokes a gameplay action. Runtime evidence still requires owner testing.

#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/discovery_state.hpp>
#include <dsnap/gate_attribution.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/runtime_contract.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Input/KeyDef.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <UE4SSProgram.hpp>
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#pragma warning(pop)
#pragma warning(disable : 4324) // Pinned UEPseudo aligned engine templates instantiate after their includes.

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <future>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

constexpr auto kVersion = STR("0.6.0-dropitem-closed-loop-diagnostic");
constexpr auto kDiagnosticLabel = "OWNER_AUTHORIZED_READ_ONLY_DIAGNOSTIC";
constexpr auto kDropItemClass = STR("/Script/DS.DropItemActor");
constexpr auto kInteractableComponentClass = STR("/Script/DS.DInteractableComponent");
constexpr auto kPlayerCharacterClass = STR("/Script/DS.DsPlayerCharacter");
constexpr auto kPlayerControllerClass = STR("/Script/DS.DsPlayerController");
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kPulseInterval = std::chrono::milliseconds{150};
constexpr std::uint8_t kRequiredInteractableValue = 2;
constexpr std::size_t kCandidatesPerPulse = 8;
constexpr std::int32_t kDiscoveryObjectsPerPulse = 16384;
constexpr auto kDiscoveryBatchBudget = std::chrono::microseconds{2000};
constexpr auto kCalibrationWindow = std::chrono::seconds{60};
constexpr auto kTraceDeleteWindow = std::chrono::seconds{10};
constexpr std::size_t kMaxTraceCalls = 256;
constexpr std::size_t kMaxTraceWatches = 128;
constexpr std::size_t kMaxTraceObjectPropertiesPerRoot = 48;
constexpr std::size_t kMaxTraceTextBytes = 8192;

struct HookSpec {
    const CharType* path;
    const CharType* name;
    dsnap::ReplayFunction replay;
    dsnap::ReceiverRole receiver;
    bool replay_eligible;
    enum class OwnerKind : std::uint8_t { InteractableComponent, PlayerController, DropItemActor } owner_kind;
};

constexpr std::array kCalibrationHookSpecs{
    HookSpec{STR("/Script/DS.DInteractableComponent:CallActivePlayer"), STR("CallActivePlayer"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:Server_RunInteractV2"), STR("Server_RunInteractV2"), dsnap::ReplayFunction::ServerRunInteractV2, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:Server_InputInteractKeyAfterAction"), STR("Server_InputInteractKeyAfterAction"), dsnap::ReplayFunction::ServerInputInteractKeyAfterAction, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:Server_InputInteractKeyAction"), STR("Server_InputInteractKeyAction"), dsnap::ReplayFunction::ServerInputInteractKeyAction, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:ServerReturnInteract_EnableState"), STR("ServerReturnInteract_EnableState"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:SetInteractUIV2"), STR("SetInteractUIV2"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:ClientSetInteractUIV2"), STR("ClientSetInteractUIV2"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:OnBeginOverlap"), STR("OnBeginOverlap"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DInteractableComponent:OnEndOverlap"), STR("OnEndOverlap"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::InteractableComponent},
    HookSpec{STR("/Script/DS.DsPlayerController:Interact"), STR("Interact"), dsnap::ReplayFunction::ControllerInteract, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DsPlayerController:OnPressInteractionButton"), STR("OnPressInteractionButton"), dsnap::ReplayFunction::ControllerPressInteractionButton, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DsPlayerController:OnReleaseInteractionButton"), STR("OnReleaseInteractionButton"), dsnap::ReplayFunction::ControllerPressInteractionButton, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DsPlayerController:Client_InteractActionSetting"), STR("Client_InteractActionSetting"), dsnap::ReplayFunction::ControllerInteract, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DsPlayerController:Client_InteractActionSetting_Restore"), STR("Client_InteractActionSetting_Restore"), dsnap::ReplayFunction::ControllerInteract, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DsPlayerController:ServerReturnInteractState"), STR("ServerReturnInteractState"), dsnap::ReplayFunction::ControllerInteract, dsnap::ReceiverRole::CurrentController, false, HookSpec::OwnerKind::PlayerController},
    HookSpec{STR("/Script/DS.DropItemActor:AniPickUp"), STR("AniPickUp"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::DropItemActor},
    HookSpec{STR("/Script/DS.DropItemActor:SetDestroy"), STR("SetDestroy"), dsnap::ReplayFunction::CallActivePlayer, dsnap::ReceiverRole::CandidateComponent, false, HookSpec::OwnerKind::DropItemActor},
};

enum class CalibrationRejectReason : std::uint8_t {
    None,
    StateOrContext,
    ReceiverMismatch,
    CandidateRelationMissing,
    CandidateAmbiguous,
    PlayerLocationUnavailable,
    ParameterMetadataInvalid,
    UnsupportedParameter,
    GuardedException,
    Count,
};

constexpr std::size_t kCalibrationRejectReasonCount = static_cast<std::size_t>(CalibrationRejectReason::Count);

[[nodiscard]] const char* calibration_reject_reason_name(CalibrationRejectReason reason) noexcept {
    switch (reason) {
    case CalibrationRejectReason::None: return "none";
    case CalibrationRejectReason::StateOrContext: return "state_or_context";
    case CalibrationRejectReason::ReceiverMismatch: return "receiver_mismatch";
    case CalibrationRejectReason::CandidateRelationMissing: return "candidate_relation_missing";
    case CalibrationRejectReason::CandidateAmbiguous: return "candidate_ambiguous";
    case CalibrationRejectReason::PlayerLocationUnavailable: return "player_location_unavailable";
    case CalibrationRejectReason::ParameterMetadataInvalid: return "parameter_metadata_invalid";
    case CalibrationRejectReason::UnsupportedParameter: return "unsupported_parameter";
    case CalibrationRejectReason::GuardedException: return "guarded_exception";
    default: return "unknown";
    }
}

std::filesystem::path binary_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path mod_directory() {
    return binary_directory() / "ue4ss" / "Mods" / "DragonSwordNativeAutoPickup";
}

[[nodiscard]] std::uint64_t pack_weak_identity(const FWeakObjectPtr& weak) noexcept {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(weak.ObjectIndex)) << 32U) |
           static_cast<std::uint32_t>(weak.ObjectSerialNumber);
}

[[nodiscard]] FWeakObjectPtr unpack_weak_identity(std::uint64_t packed) noexcept {
    FWeakObjectPtr weak{};
    if (packed == 0) return weak;
    weak.ObjectIndex = static_cast<std::int32_t>(packed >> 32U);
    weak.ObjectSerialNumber = static_cast<std::int32_t>(packed & 0xffffffffU);
    return weak;
}

struct CandidateState {
    FWeakObjectPtr weak{};
    dsnap::BoundedGateDiagnosticState diagnostics{};
    bool derived_class{};
    bool discovered{};
    bool identity_logged{};
};

struct CandidateSnapshot {
    std::int32_t index{-1};
    std::int32_t serial{};
    FWeakObjectPtr weak{};
};

struct CandidateIdentitySnapshot {
    std::int32_t index{-1};
    FWeakObjectPtr weak{};
    bool derived_class{};
    bool discovered{};
};

enum class GateResult { Eligible, RetryLater, PermanentReject, Invalid };

struct GateOutput {
    GateResult result{GateResult::Invalid};
    dsnap::GateObservation observation{};
    UObject* component{};
};

struct PlayerContext {
    UObject* controller{};
    UObject* player{};
    FVector location{};
    dsnap::PlayerMode mode{dsnap::PlayerMode::ExpectedCharacter};
};

struct RegisteredFunctionHook {
    UFunction* function{};
    std::pair<int, int> ids{};
    std::size_t spec_index{};
};

struct TraceWatch {
    dsnap::WeakObjectId identity{};
    std::uint64_t call_sequence{};
    std::uint64_t world_epoch{};
    Clock::time_point captured_at{};
    std::string source{};
    std::string object_name{};
    std::string class_name{};
};

class NativeAutoPickup final : public CppUserModBase, public FUObjectCreateListener, public FUObjectDeleteListener {
public:
    NativeAutoPickup()
        : callback_gate_(next_generation_.fetch_add(1, std::memory_order_relaxed) + 1),
          configuration_result_(dsnap::load_configuration(mod_directory() / "config.ini")),
          logger_(mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.log",
                  mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.Debug.log"),
          fingerprint_future_(std::async(std::launch::async, [] { return dsnap::verify_build_fingerprint(binary_directory()); })) {
        ModName = STR("DragonSwordNativeAutoPickup");
        ModVersion = kVersion;
        ModDescription = STR("Read-only DropItemActor closed-loop interaction diagnostic");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "START",
                      std::format("label={} armed=0 active=0 generation={}", kDiagnosticLabel, callback_gate_.generation()));
        for (const auto& error : configuration_result_.errors) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIG_REJECTED", error);
        }
    }

    ~NativeAutoPickup() override {
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        active_.store(false, std::memory_order_release);
        if (!uobject_array_shutdown_seen_.load(std::memory_order_acquire)) {
            unregister_calibration_hooks();
            unregister_object_listeners();
            if (engine_tick_callback_id_ != Hook::ERROR_ID) {
                Hook::UnregisterCallback(engine_tick_callback_id_);
                engine_tick_callback_id_ = Hook::ERROR_ID;
            }
            if (world_reset_callback_id_ != Hook::ERROR_ID) {
                Hook::UnregisterCallback(world_reset_callback_id_);
                world_reset_callback_id_ = Hook::ERROR_ID;
            }
        }
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        logger_.write(dsnap::LogAudience::User, "STOP", std::format("generation={}", callback_gate_.generation()));
    }

    void on_unreal_init() override {
        if (!configuration_result_.valid()) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "invalid configuration");
            return;
        }
        drop_item_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kDropItemClass);
        interactable_component_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInteractableComponentClass);
        player_character_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kPlayerCharacterClass);
        player_controller_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kPlayerControllerClass);
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunction);
        if (!drop_item_class_ || !interactable_component_class_ || !player_character_class_ || !player_controller_class_ ||
            !location_function_) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required class or UFunction metadata is missing");
            return;
        }

        const auto generation = callback_gate_.generation();
        engine_tick_callback_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                if (auto* self = current_instance(generation)) self->engine_tick_post(engine);
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("BoundedPickupPulse")});
        world_reset_callback_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});
        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID) {
            if (engine_tick_callback_id_ != Hook::ERROR_ID) {
                Hook::UnregisterCallback(engine_tick_callback_id_);
                engine_tick_callback_id_ = Hook::ERROR_ID;
            }
            if (world_reset_callback_id_ != Hook::ERROR_ID) {
                Hook::UnregisterCallback(world_reset_callback_id_);
                world_reset_callback_id_ = Hook::ERROR_ID;
            }
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required native callback registration failed");
            return;
        }
        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey=F9 mode=read_only_dropitem_closed_loop pulse=EngineTickPost pulse_ms={} "
                                  "discovery=bounded_DropItemActor_sweep_plus_lifecycle window_seconds={} generation={}",
                                  kDiagnosticLabel, kPulseInterval.count(), kCalibrationWindow.count(), generation));
    }

    void on_update() override {
        // Event-loop callback: plain atomics, future completion, and logging only.
        if (shutting_down_.load(std::memory_order_acquire)) return;
        if (!fingerprint_applied_ &&
            fingerprint_future_.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
            const auto result = fingerprint_future_.get();
            fingerprint_applied_ = true;
            build_trusted_.store(result.trusted, std::memory_order_release);
            fingerprint_result_ = result;
            if (!result.trusted) active_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                          std::format("trusted={} game={} ue4ss={} error={}", result.trusted, result.game_sha256,
                                      result.ue4ss_sha256, result.error));
            if (!result.trusted) logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY", "unknown build fingerprint");
            else logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_AVAILABLE",
                               "stand beside exactly one ordinary ground drop, press F9, then pick up that same item manually");
        }

        // UE4SS native keydown dispatch is not reliable for this game/build even
        // though registration returns normally. Poll only the scalar OS key bit
        // here and preserve the same physical rising-edge latch. UObject work is
        // still deferred to EngineTick through f9_toggle_requested_.
        const bool f9_down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (f9_down) {
            f9_keydown();
        } else {
            const std::scoped_lock lock{state_mutex_}; f9_edge_.key_up();
        }
        const auto now = Clock::now();
        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            const auto player_chain_attempts = player_chain_attempts_.load();
            const auto player_chain_total_us = player_chain_total_us_.load();
            const auto player_chain_average_us = player_chain_attempts == 0 ? 0 : player_chain_total_us / player_chain_attempts;
            const auto player_chain_reasons = player_chain_reason_aggregate_text();
            const auto calibration_specs = calibration_spec_aggregate_text();
            const auto calibration_reasons = calibration_rejection_aggregate_text();
            logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                          std::format("label={} create_callbacks={} drop_item_captures={} base_class_captures={} "
                                      "derived_class_captures={} delete_callbacks={} "
                                      "engine_tick_callbacks={} candidate_nonempty_pulses={} candidate_batches={} "
                                      "candidates_evaluated={} idle_pulses={} "
                                      "throttle_rejects={} player_chain_attempts={} player_chain_successes={} "
                                      "player_chain_failures={} player_chain_average_us={} player_chain_max_us={} "
                                      "player_chain_reasons={} expected_character_pulses={} alternate_pawn_pulses={} "
                                      "f9_repeat_rejects={} discovery_sweeps_started={} discovery_sweeps_completed={} "
                                      "discovery_objects_examined={} discovery_candidates_found={} discovery_total_us={} discovery_max_batch_us={} "
                                      "lifecycle_candidates_found={} lifecycle_filter_total_us={} "
                                      "gate_owner_invalid={} gate_class_invalid={} gate_interact_component_missing={} "
                                      "gate_component_ownership_mismatch={} gate_state_field_missing={} "
                                      "gate_state_value_mismatch={} gate_owner_location_unavailable={} "
                                      "gate_non_finite_distance={} gate_outside_radius={} gate_eligible={} "
                                      "gate_guarded_evaluation_exception={} gate_rejections={} seh_rejections={} "
                                       "contract_hook_calls={} contract_hook_rejections={} contract_hook_failures={} contract_hook_total_us={} "
                                       "calibration_functions_examined={} calibration_functions_registered={} calibration_specs={} calibration_rejection_reasons={} "
                                       "calibration_calls={} calibration_ambiguity_rejections={} contracts_validated_by_delete={} contracts_loaded={} contract_rejections={} "
                                      "contract_persist_failures={} contracts_quarantined={} runtime_state={} "
                                       "world_resets={} candidates={} trace_calls={} trace_guard_failures={} trace_watches={} "
                                       "armed={} world_ready={} trusted={} active={} dropped_logs={}",
                                      kDiagnosticLabel, create_callbacks_.load(), drop_item_captures_.load(),
                                      base_class_captures_.load(), derived_class_captures_.load(), delete_callbacks_.load(),
                                      engine_tick_callbacks_.load(), candidate_nonempty_pulses_.load(),
                                      candidate_batches_.load(), candidates_evaluated_.load(), idle_pulses_.load(),
                                      throttle_rejects_.load(), player_chain_attempts, player_chain_successes_.load(),
                                      player_chain_failures_.load(), player_chain_average_us, player_chain_max_us_.load(),
                                      player_chain_reasons, expected_character_pulses_.load(), alternate_pawn_pulses_.load(),
                                      f9_repeat_rejects_.load(), discovery_sweeps_started_.load(),
                                      discovery_sweeps_completed_.load(), discovery_objects_examined_.load(),
                                      discovery_candidates_found_.load(), discovery_total_us_.load(), discovery_max_batch_us_.load(),
                                      lifecycle_candidates_found_.load(), lifecycle_filter_total_us_.load(),
                                      gate_reason_count(dsnap::GateReason::OwnerInvalid),
                                      gate_reason_count(dsnap::GateReason::ClassInvalid),
                                      gate_reason_count(dsnap::GateReason::InteractComponentMissing),
                                      gate_reason_count(dsnap::GateReason::ComponentOwnershipMismatch),
                                      gate_reason_count(dsnap::GateReason::StateFieldMissing),
                                      gate_reason_count(dsnap::GateReason::StateValueMismatch),
                                      gate_reason_count(dsnap::GateReason::OwnerLocationUnavailable),
                                      gate_reason_count(dsnap::GateReason::NonFiniteDistance),
                                      gate_reason_count(dsnap::GateReason::OutsideRadius),
                                      gate_reason_count(dsnap::GateReason::Eligible),
                                      gate_reason_count(dsnap::GateReason::GuardedEvaluationException),
                                      gate_rejections_.load(), seh_rejections_.load(),
                                       contract_hook_calls_.load(), contract_hook_rejections_.load(), contract_hook_failures_.load(),
                                       contract_hook_total_us_.load(), calibration_functions_examined_.load(),
                                       calibration_functions_registered_.load(), calibration_specs, calibration_reasons,
                                       calibration_calls_observed_.load(), calibration_ambiguity_rejections_.load(),
                                      contracts_validated_by_delete_.load(), contracts_loaded_.load(), contract_rejections_.load(),
                                      contract_persist_failures_.load(), contracts_quarantined_.load(), state_name(runtime_state_value()),
                                       world_resets_.load(), candidate_count(), trace_call_sequence_.load(),
                                       trace_guard_failures_.load(), trace_watch_count(), armed_.load(), world_ready_.load(),
                                      build_trusted_.load(), active_.load(), logger_.dropped_messages()));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

    void NotifyUObjectCreated(const UObjectBase* object, int32 index) override {
        ++create_callbacks_;
        if (!object || shutting_down_.load(std::memory_order_acquire) || !armed_.load(std::memory_order_acquire)) return;
        FWeakObjectPtr weak{};
        bool derived{};
        if (!capture_drop_item_guarded(object, &weak, &derived)) return;
        const auto started = Clock::now();
        register_candidate(index, weak, derived, false);
        lifecycle_filter_total_us_.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count()));
    }

    void NotifyUObjectDeleted(const UObjectBase*, int32 index) override {
        ++delete_callbacks_;
        if (runtime_state_value() != dsnap::RuntimeState::Calibrating) return;
        trace_delete_match(index);
        erase_candidate(index);
    }

    void OnUObjectArrayShutdown() override {
        uobject_array_shutdown_seen_.store(true, std::memory_order_release);
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        active_.store(false, std::memory_order_release);
        abandon_calibration_hooks_after_uobject_shutdown();
        unregister_object_listeners();
    }

private:
    void abandon_calibration_hooks_after_uobject_shutdown() noexcept {
        const std::scoped_lock lock{calibration_mutex_};
        calibration_hooks_.clear();
    }

    void unregister_calibration_hooks() noexcept {
        std::vector<RegisteredFunctionHook> hooks{};
        {
            const std::scoped_lock lock{calibration_mutex_};
            hooks.swap(calibration_hooks_);
        }
        for (const auto& hook : hooks) {
            try { UObjectGlobals::UnregisterHook(hook.function, hook.ids); }
            catch (...) { /* Generation and state gates already fail closed. */ }
        }
    }

    [[nodiscard]] bool hook_owner_is_allowed(UFunction* function, std::size_t spec_index) const noexcept {
        if (!function || spec_index >= kCalibrationHookSpecs.size()) return false;
        auto* outer = function->GetOuterPrivate();
        if (!outer || !outer->IsA(UClass::StaticClass())) return false;
        auto* owner_class = static_cast<UClass*>(outer);
        const auto& spec = kCalibrationHookSpecs[spec_index];
        UClass* required_base{};
        switch (spec.owner_kind) {
        case HookSpec::OwnerKind::InteractableComponent: required_base = interactable_component_class_; break;
        case HookSpec::OwnerKind::PlayerController: required_base = player_controller_class_; break;
        case HookSpec::OwnerKind::DropItemActor: required_base = drop_item_class_; break;
        }
        return required_base && owner_class->IsChildOf(required_base);
    }

    bool register_calibration_function(UFunction* function, std::size_t spec_index, const char* source) {
        if (shutting_down_.load(std::memory_order_acquire) ||
            runtime_state_value() != dsnap::RuntimeState::Calibrating ||
            !hook_owner_is_allowed(function, spec_index)) return false;
        {
            const std::scoped_lock lock{calibration_mutex_};
            for (const auto& registered : calibration_hooks_)
                if (registered.function == function) return false;
        }
        const auto generation = callback_gate_.generation();
        try {
            const auto ids = UObjectGlobals::RegisterHook(function,
                [generation, spec_index](UnrealScriptFunctionCallableContext& context, void*) {
                    if (auto* self = current_instance(generation)) self->calibration_pre_hook(spec_index, context);
                }, [generation, spec_index](UnrealScriptFunctionCallableContext& context, void*) {
                    if (auto* self = current_instance(generation)) self->calibration_post_hook(spec_index, context);
                }, nullptr);
            if (shutting_down_.load(std::memory_order_acquire) ||
                runtime_state_value() != dsnap::RuntimeState::Calibrating) {
                UObjectGlobals::UnregisterHook(function, ids);
                return false;
            }
            {
                const std::scoped_lock lock{calibration_mutex_};
                calibration_hooks_.push_back({function, ids, spec_index});
            }
            ++calibration_functions_registered_;
            std::string parameter_metadata{};
            for (auto* property : TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated)) {
                if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                    property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm)) continue;
                if (!parameter_metadata.empty()) parameter_metadata += ";";
                parameter_metadata += std::format("{}:{}:{}", to_string(property->GetName()),
                                                  to_string(property->GetClass().GetName()), property->GetSize());
            }
            if (parameter_metadata.empty()) parameter_metadata = "none";
            logger_.write(dsnap::LogAudience::Debug, "CALIBRATION_HOOK_METADATA",
                          std::format("function_id={} source={} parameter_bytes={} parameters={} replay_eligible={} path={}",
                                      spec_index, source, function->GetParmsSize(),
                                      parameter_metadata, kCalibrationHookSpecs[spec_index].replay_eligible,
                                      to_string(function->GetPathName())));
            return true;
        } catch (...) {
            ++contract_hook_failures_;
            return false;
        }
    }

    void discover_calibration_function(UFunction* function) {
        if (!function || runtime_state_value() != dsnap::RuntimeState::Calibrating) return;
        ++calibration_functions_examined_;
        const auto function_name = function->GetName();
        for (std::size_t index = 0; index < kCalibrationHookSpecs.size(); ++index) {
            if (function_name != kCalibrationHookSpecs[index].name) continue;
            static_cast<void>(register_calibration_function(function, index, "discovery"));
        }
    }

    void register_object_listeners() {
        if (listeners_registered_.exchange(true, std::memory_order_acq_rel)) return;
        UObjectArray::AddUObjectCreateListener(this);
        UObjectArray::AddUObjectDeleteListener(this);
    }

    void unregister_object_listeners() noexcept {
        if (!listeners_registered_.exchange(false, std::memory_order_acq_rel)) return;
        UObjectArray::RemoveUObjectCreateListener(this);
        UObjectArray::RemoveUObjectDeleteListener(this);
    }

    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    void engine_tick_post(UEngine* engine) {
        ++engine_tick_callbacks_;
        apply_game_thread_control();
        if (!engine || !armed_.load(std::memory_order_acquire) ||
            !build_trusted_.load(std::memory_order_acquire)) return;
        try {
            if (!handle_engine_tick_guarded(this, engine)) ++seh_rejections_;
        } catch (...) {
            ++gate_rejections_;
        }
    }

    static bool handle_engine_tick_guarded(NativeAutoPickup* self, UEngine* engine) noexcept {
        bool ok{};
#if defined(_MSC_VER)
        __try {
            self->handle_engine_tick(engine);
            ok = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
#else
        self->handle_engine_tick(engine);
        ok = true;
#endif
        return ok;
    }

    void handle_engine_tick(UEngine* engine) {
        const auto now = Clock::now();
        if (next_pulse_due_ != Clock::time_point{} && now < next_pulse_due_) {
            ++throttle_rejects_;
            return;
        }
        next_pulse_due_ = now + kPulseInterval;
        process_discovery_batch();
        const auto player_chain_started = Clock::now();
        ++player_chain_attempts_;
        PlayerContext context{};
        dsnap::PlayerChainReason player_chain_reason{dsnap::PlayerChainReason::EngineOrOutputInvalid};
        if (!resolve_player_context_guarded(this, engine, &context, &player_chain_reason)) {
            record_player_chain_reason(player_chain_reason);
            current_pawn_identity_.store(0, std::memory_order_release);
            ++player_chain_failures_;
            world_ready_.store(false, std::memory_order_release);
            active_.store(false, std::memory_order_release);
            record_player_chain_duration(player_chain_started);
            return;
        }

        record_player_chain_reason(player_chain_reason);
        FWeakObjectPtr current_pawn_weak{context.player};
        current_pawn_identity_.store(pack_weak_identity(current_pawn_weak), std::memory_order_release);
        FWeakObjectPtr current_controller_weak{context.controller};
        current_controller_identity_.store(pack_weak_identity(current_controller_weak), std::memory_order_release);
        if (runtime_state_value() == dsnap::RuntimeState::Calibrating &&
            !player_context_trace_emitted_.exchange(true, std::memory_order_acq_rel)) {
            trace_player_context_guarded(context);
        }
        if (context.mode == dsnap::PlayerMode::ExpectedCharacter) ++expected_character_pulses_;
        else ++alternate_pawn_pulses_;
        ++player_chain_successes_;
        world_ready_.store(true, std::memory_order_release);
        if (runtime_state_value() == dsnap::RuntimeState::Calibrating) {
            update_diagnostic_lock(context);
            expire_trace_watches(now);
            if (now >= calibration_deadline_ &&
                !calibration_timeout_requested_.exchange(true, std::memory_order_acq_rel)) {
                logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_WINDOW_COMPLETE",
                              "read-only capture ended; press F9 to return Off and preserve both logs");
            }
        } else {
            ++idle_pulses_;
        }
        record_player_chain_duration(player_chain_started);
    }

    void process_discovery_batch() {
        std::pair<std::int32_t, std::int32_t> range{};
        {
            const std::scoped_lock lock{state_mutex_};
            if (!discovery_.active() || discovery_.epoch() != world_epoch_.load(std::memory_order_acquire)) return;
            range = discovery_.next(kDiscoveryObjectsPerPulse);
        }
        if (range.second <= range.first) return;
        const auto started = Clock::now();
        const bool discover_functions = runtime_state_value() == dsnap::RuntimeState::Calibrating;
        std::uint64_t examined{};
        std::int32_t committed = range.first;
        for (auto index = range.first; index < range.second; ++index) {
            committed = index + 1; ++examined;
            if (shutting_down_.load(std::memory_order_acquire) || !armed_.load(std::memory_order_acquire)) break;
            auto* item = UObjectArray::IndexToObject(index);
            if (item && item->IsValid(false) && !item->IsUnreachable()) {
                auto* object = item->GetUObject(); FWeakObjectPtr weak{};
                if (object && discover_functions && object->IsA(UFunction::StaticClass())) {
                    discover_calibration_function(static_cast<UFunction*>(object));
                }
                bool derived{};
                if (object && capture_drop_item_guarded(object, &weak, &derived) && weak.ObjectIndex == index)
                    register_candidate(index, weak, derived, true);
            }
            if ((examined & 63U) == 0 && Clock::now() - started >= kDiscoveryBatchBudget) break;
        }
        { const std::scoped_lock lock{state_mutex_}; discovery_.commit(committed); }
        const auto batch_us = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count());
        discovery_objects_examined_.fetch_add(examined, std::memory_order_relaxed);
        discovery_total_us_.fetch_add(batch_us, std::memory_order_relaxed);
        auto previous = discovery_max_batch_us_.load(std::memory_order_relaxed);
        while (previous < batch_us && !discovery_max_batch_us_.compare_exchange_weak(previous, batch_us, std::memory_order_relaxed)) {}
        bool complete{};
        { const std::scoped_lock lock{state_mutex_}; complete = !discovery_.active(); }
        if (complete) {
            ++discovery_sweeps_completed_;
            if (runtime_state_value() == dsnap::RuntimeState::Calibrating)
                logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_READY", "perform exactly one normal manual pickup now, then wait three seconds");
            logger_.write(dsnap::LogAudience::Debug, "DISCOVERY_COMPLETE",
                          std::format("epoch={} examined={} elapsed_ms={}", world_epoch_.load(),
                                      discovery_objects_examined_.load(),
                                      std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - discovery_started_at_).count()));
        }
    }

    void register_candidate(std::int32_t index, const FWeakObjectPtr& weak, bool derived, bool discovered) {
        CandidateState candidate{};
        candidate.weak = weak;
        candidate.derived_class = derived;
        candidate.discovered = discovered;
        bool inserted{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            if (candidates_.size() >= configuration_result_.value.max_queue && !candidates_.contains(index)) { ++queue_rejections_; return; }
            if (candidates_.contains(index)) return;
            candidates_[index] = candidate;
            inserted = true;
        }
        if (!inserted) return;
        ++drop_item_captures_;
        if (derived) ++derived_class_captures_; else ++base_class_captures_;
        if (discovered) ++discovery_candidates_found_; else ++lifecycle_candidates_found_;
    }

    static bool resolve_player_context_guarded(NativeAutoPickup* self, UEngine* engine,
                                               PlayerContext* output,
                                               dsnap::PlayerChainReason* reason) noexcept {
        bool resolved{};
#if defined(_MSC_VER)
        __try {
            resolved = self->resolve_player_context_unsafe(engine, output, reason);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            if (reason) *reason = dsnap::PlayerChainReason::GuardedException;
            resolved = false;
        }
#else
        resolved = self->resolve_player_context_unsafe(engine, output, reason);
#endif
        return resolved;
    }

    bool resolve_player_context_unsafe(UEngine* engine, PlayerContext* output,
                                       dsnap::PlayerChainReason* reason) noexcept {
        if (!reason) return false;
        if (!engine || !output) {
            *reason = dsnap::PlayerChainReason::EngineOrOutputInvalid;
            return false;
        }
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        if (!viewport_value) {
            *reason = dsnap::PlayerChainReason::GameViewportPropertyMissing;
            return false;
        }
        auto* viewport = *viewport_value;
        if (!viewport) {
            *reason = dsnap::PlayerChainReason::GameViewportValueNull;
            return false;
        }
        auto** game_instance_value = viewport->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameInstance"));
        if (!game_instance_value) {
            *reason = dsnap::PlayerChainReason::GameInstancePropertyMissing;
            return false;
        }
        auto* game_instance = *game_instance_value;
        if (!game_instance) {
            *reason = dsnap::PlayerChainReason::GameInstanceValueNull;
            return false;
        }
        auto* local_players = game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers"));
        if (!local_players) {
            *reason = dsnap::PlayerChainReason::LocalPlayersPropertyMissing;
            return false;
        }
        if (!local_players->IsValidIndex(0)) {
            *reason = dsnap::PlayerChainReason::LocalPlayersInvalidIndex;
            return false;
        }
        if (!local_players->GetData()) {
            *reason = dsnap::PlayerChainReason::LocalPlayersNullData;
            return false;
        }
        auto* local_player = static_cast<UObject* const*>(local_players->GetData())[0];
        if (!local_player) {
            *reason = dsnap::PlayerChainReason::LocalPlayerEntryNull;
            return false;
        }
        auto** controller_value = local_player->GetValuePtrByPropertyNameInChain<UObject*>(STR("PlayerController"));
        if (!controller_value) {
            *reason = dsnap::PlayerChainReason::LocalPlayerControllerPropertyMissing;
            return false;
        }
        auto* controller = *controller_value;
        if (!controller) {
            *reason = dsnap::PlayerChainReason::LocalPlayerControllerValueNull;
            return false;
        }
        if (!controller->IsA(player_controller_class_)) {
            *reason = dsnap::PlayerChainReason::ControllerClassMismatch;
            return false;
        }
        auto** controller_player_value = controller->GetValuePtrByPropertyNameInChain<UObject*>(STR("Player"));
        if (!controller_player_value) {
            *reason = dsnap::PlayerChainReason::ControllerPlayerPropertyMissing;
            return false;
        }
        if (*controller_player_value != local_player) {
            *reason = dsnap::PlayerChainReason::ControllerPlayerIdentityMismatch;
            return false;
        }
        auto** pawn_value = controller->GetValuePtrByPropertyNameInChain<UObject*>(STR("Pawn"));
        if (!pawn_value) {
            *reason = dsnap::PlayerChainReason::PawnPropertyMissing;
            return false;
        }
        auto* player = *pawn_value;
        if (!player) {
            *reason = dsnap::PlayerChainReason::PawnValueNull;
            return false;
        }
        const bool expected_character = player->IsA(player_character_class_);
        auto** player_controller_value = player->GetValuePtrByPropertyNameInChain<UObject*>(STR("Controller"));
        if (!player_controller_value) {
            *reason = dsnap::PlayerChainReason::PlayerControllerPropertyMissing;
            return false;
        }
        if (*player_controller_value != controller) {
            *reason = dsnap::PlayerChainReason::PlayerControllerIdentityMismatch;
            return false;
        }
        FVector player_location{};
        if (!read_location_guarded(player, &player_location)) {
            *reason = dsnap::PlayerChainReason::LocationUnavailable;
            return false;
        }
        const auto player_mode = dsnap::classify_controlled_pawn(expected_character, true, true);
        if (!player_mode) {
            *reason = dsnap::PlayerChainReason::PlayerControllerIdentityMismatch;
            return false;
        }
        *output = PlayerContext{controller, player, player_location, *player_mode};
        *reason = dsnap::PlayerChainReason::Success;
        return true;
    }

    void record_player_chain_reason(dsnap::PlayerChainReason reason) {
        player_chain_reason_counts_[dsnap::player_chain_reason_index(reason)].fetch_add(1, std::memory_order_relaxed);
        bool should_log{};
        {
            const std::scoped_lock lock{player_chain_diagnostic_mutex_};
            should_log = player_chain_diagnostics_.should_log(reason);
        }
        if (should_log) {
            logger_.write(dsnap::LogAudience::Debug, "PLAYER_CHAIN_ATTRIBUTION",
                          std::format("reason={}", dsnap::player_chain_reason_name(reason)));
        }
    }

    [[nodiscard]] std::string player_chain_reason_aggregate_text() const {
        std::size_t dominant_index{};
        std::uint64_t dominant_count{};
        std::string counts{};
        for (std::size_t index = 0; index < dsnap::kPlayerChainReasonCount; ++index) {
            const auto count = player_chain_reason_counts_[index].load(std::memory_order_relaxed);
            if (count > dominant_count) {
                dominant_index = index;
                dominant_count = count;
            }
            if (!counts.empty()) counts.push_back(',');
            counts += std::format("{}={}",
                                  dsnap::player_chain_reason_name(static_cast<dsnap::PlayerChainReason>(index)), count);
        }
        return std::format("dominant:{}={} counts:[{}]",
                           dsnap::player_chain_reason_name(static_cast<dsnap::PlayerChainReason>(dominant_index)),
                           dominant_count, counts);
    }

    [[nodiscard]] std::string calibration_spec_aggregate_text() const {
        std::string counts{"["};
        for (std::size_t index = 0; index < kCalibrationHookSpecs.size(); ++index) {
            if (index != 0) counts.push_back(',');
            counts += std::format("{}:{}/{}", index,
                                  calibration_hook_calls_by_spec_[index].load(std::memory_order_relaxed),
                                  calibration_hook_rejections_by_spec_[index].load(std::memory_order_relaxed));
        }
        counts.push_back(']');
        return counts;
    }

    [[nodiscard]] std::string calibration_rejection_aggregate_text() const {
        std::string counts{"["};
        for (std::size_t index = 1; index < kCalibrationRejectReasonCount; ++index) {
            if (index != 1) counts.push_back(',');
            counts += std::format("{}={}",
                                  calibration_reject_reason_name(static_cast<CalibrationRejectReason>(index)),
                                  calibration_rejection_reasons_[index].load(std::memory_order_relaxed));
        }
        counts.push_back(']');
        return counts;
    }

    void record_player_chain_duration(Clock::time_point started) noexcept {
        const auto elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count());
        player_chain_total_us_.fetch_add(elapsed, std::memory_order_relaxed);
        auto previous = player_chain_max_us_.load(std::memory_order_relaxed);
        while (previous < elapsed &&
               !player_chain_max_us_.compare_exchange_weak(previous, elapsed, std::memory_order_relaxed)) {}
    }

    static bool capture_drop_item_guarded(const UObjectBase* object, FWeakObjectPtr* weak,
                                          bool* derived_class) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            auto* self = instance_.load(std::memory_order_acquire);
            auto* unreal_object = std::bit_cast<UObject*>(object);
            auto* object_class = unreal_object->GetClassPrivate();
            if (self && object_class && unreal_object->IsA(self->drop_item_class_) &&
                !unreal_object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject) &&
                !unreal_object->HasAnyFlags(EObjectFlags::RF_ArchetypeObject) &&
                !unreal_object->HasAnyFlags(EObjectFlags::RF_DefaultSubObject)) {
                *weak = unreal_object;
                *derived_class = object_class != self->drop_item_class_;
                captured = true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { captured = false; }
#else
        auto* self = instance_.load(std::memory_order_acquire);
        auto* unreal_object = std::bit_cast<UObject*>(object);
        auto* object_class = unreal_object->GetClassPrivate();
        if (self && object_class && unreal_object->IsA(self->drop_item_class_) &&
            !unreal_object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject) &&
            !unreal_object->HasAnyFlags(EObjectFlags::RF_ArchetypeObject) &&
            !unreal_object->HasAnyFlags(EObjectFlags::RF_DefaultSubObject)) {
            *weak = unreal_object;
            *derived_class = object_class != self->drop_item_class_;
            captured = true;
        }
#endif
        return captured;
    }

    void update_diagnostic_lock(const PlayerContext& context) {
        const auto locked_identity = diagnostic_candidate_identity_.load(std::memory_order_acquire);
        if (locked_identity != 0) {
            auto locked = unpack_weak_identity(locked_identity);
            if (locked.Get()) return;
            diagnostic_candidate_identity_.store(0, std::memory_order_release);
            diagnostic_component_identity_.store(0, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "DIAGNOSTIC_TARGET_INVALIDATED",
                          std::format("object_index={} serial={}", locked.ObjectIndex, locked.ObjectSerialNumber));
        }

        std::vector<CandidateSnapshot> snapshots{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            snapshots.reserve(candidates_.size());
            for (const auto& [index, candidate] : candidates_)
                snapshots.push_back({index, candidate.weak.ObjectSerialNumber, candidate.weak});
        }
        CandidateSnapshot selected{};
        GateOutput selected_gate{};
        std::size_t eligible_count{};
        for (const auto& snapshot : snapshots) {
            auto* owner = snapshot.weak.Get();
            if (!owner || owner->GetWorld() != context.player->GetWorld()) continue;
            GateOutput gate{};
            if (!evaluate_candidate_guarded(this, owner, &context.location, &gate) ||
                gate.result != GateResult::Eligible) continue;
            selected = snapshot;
            selected_gate = gate;
            ++eligible_count;
            if (eligible_count > 1) break;
        }
        if (eligible_count != 1 || !selected_gate.component) {
            const auto previous = diagnostic_eligible_count_.exchange(eligible_count, std::memory_order_acq_rel);
            if (previous != eligible_count && eligible_count > 1) {
                logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_TARGET_AMBIGUOUS",
                              std::format("eligible_candidates={}; leave only one ordinary drop inside {:.1f} meters",
                                          eligible_count, configuration_result_.value.radius_meters));
            }
            return;
        }
        const auto owner_identity = pack_weak_identity(selected.weak);
        const FWeakObjectPtr component_weak{selected_gate.component};
        diagnostic_candidate_identity_.store(owner_identity, std::memory_order_release);
        diagnostic_component_identity_.store(pack_weak_identity(component_weak), std::memory_order_release);
        diagnostic_eligible_count_.store(1, std::memory_order_release);
        watch_trace_object_unsafe(selected.weak.Get(), "locked_drop_item", 0);
        watch_trace_object_unsafe(selected_gate.component, "locked_drop_item.InteractComponent", 0);
        log_candidate_identity_guarded(selected.index, selected.weak, true, true);
        logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_TARGET_LOCKED",
                      std::format("object_index={} serial={} distance_meters={:.3f}; manually pick up this same item now",
                                  selected.index, selected.serial, selected_gate.observation.distance_meters));
    }

    static bool evaluate_candidate_guarded(NativeAutoPickup* self, UObject* owner,
                                           const FVector* player_location, GateOutput* output) noexcept {
        bool evaluated{};
#if defined(_MSC_VER)
        __try {
            *output = self->evaluate_candidate_unsafe(owner, *player_location);
            evaluated = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { evaluated = false; }
#else
        *output = self->evaluate_candidate_unsafe(owner, *player_location);
        evaluated = true;
#endif
        return evaluated;
    }

    GateOutput evaluate_candidate_unsafe(UObject* owner, const FVector& player_location) {
        if (!owner) return {GateResult::Invalid, {.reason = dsnap::GateReason::OwnerInvalid}, nullptr};
        if (!owner->IsA(drop_item_class_)) {
            return {GateResult::Invalid, {.reason = dsnap::GateReason::ClassInvalid}, nullptr};
        }
        auto** component_value = owner->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractComponent"));
        auto* component = component_value ? *component_value : nullptr;
        if (!component) {
            return {GateResult::RetryLater, {.reason = dsnap::GateReason::InteractComponentMissing}, nullptr};
        }
        if (!candidate_component_matches(owner, component)) {
            return {GateResult::PermanentReject, {.reason = dsnap::GateReason::ComponentOwnershipMismatch}, nullptr};
        }
        auto* interactable = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"));
        auto* interact_type = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"));
        if (!interactable || !interact_type) {
            return {GateResult::RetryLater, {.reason = dsnap::GateReason::StateFieldMissing}, nullptr};
        }
        const dsnap::GateObservation state_observation{
            .reason = dsnap::GateReason::StateValueMismatch,
            .has_state_values = true,
            .interactable_value = *interactable,
            .interact_type_value = *interact_type,
        };
        if (*interactable != kRequiredInteractableValue || *interact_type != dsnap::kDropItemInteractType) {
            return {GateResult::PermanentReject, state_observation, nullptr};
        }

        FVector owner_location{};
        if (!read_location_guarded(owner, &owner_location)) {
            auto observation = state_observation;
            observation.reason = dsnap::GateReason::OwnerLocationUnavailable;
            return {GateResult::RetryLater, observation, nullptr};
        }
        const auto dx = owner_location.X() - player_location.X();
        const auto dy = owner_location.Y() - player_location.Y();
        const auto dz = owner_location.Z() - player_location.Z();
        const auto distance_squared = dx * dx + dy * dy + dz * dz;
        const auto radius_world = configuration_result_.value.radius_meters * 100.0;
        if (!std::isfinite(distance_squared)) {
            auto observation = state_observation;
            observation.reason = dsnap::GateReason::NonFiniteDistance;
            return {GateResult::Invalid, observation, nullptr};
        }
        auto observation = state_observation;
        observation.has_distance = true;
        observation.distance_meters = std::sqrt(distance_squared) / 100.0;
        if (distance_squared > radius_world * radius_world) {
            observation.reason = dsnap::GateReason::OutsideRadius;
            return {GateResult::RetryLater, observation, nullptr};
        }
        observation.reason = dsnap::GateReason::Eligible;
        return {GateResult::Eligible, observation, component};
    }

    bool read_location_unsafe(UObject* actor, FVector* result) noexcept {
        if (!actor || !result || !location_function_) return false;
        struct LocationParameters { FVector return_value{}; } parameters{};
        actor->ProcessEvent(location_function_, &parameters);
        *result = parameters.return_value;
        return std::isfinite(result->X()) && std::isfinite(result->Y()) && std::isfinite(result->Z());
    }

    bool read_location_guarded(UObject* actor, FVector* result) noexcept {
        bool ok{};
#if defined(_MSC_VER)
        __try { ok = read_location_unsafe(actor, result); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
#else
        ok = read_location_unsafe(actor, result);
#endif
        return ok;
    }

    [[nodiscard]] std::uint64_t gate_reason_count(dsnap::GateReason reason) const noexcept {
        return gate_reason_counts_[dsnap::gate_reason_index(reason)].load(std::memory_order_relaxed);
    }

    void record_gate_observation(std::int32_t index, const dsnap::GateObservation& observation) {
        gate_reason_counts_[dsnap::gate_reason_index(observation.reason)].fetch_add(1, std::memory_order_relaxed);
        bool should_log{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            const auto found = candidates_.find(index);
            if (found != candidates_.end()) should_log = found->second.diagnostics.should_log(observation);
        }
        if (!should_log) return;
        logger_.write(dsnap::LogAudience::Debug, "GATE_ATTRIBUTION",
                      std::format("object_index={} reason={} has_state_values={} interactable_value={} "
                                  "interact_type_value={} has_distance={} distance_meters={:.3f}",
                                  index, dsnap::gate_reason_name(observation.reason), observation.has_state_values,
                                  observation.interactable_value, observation.interact_type_value,
                                  observation.has_distance, observation.distance_meters));
    }

    void erase_candidate(std::int32_t index) {
        const std::scoped_lock lock{candidates_mutex_};
        candidates_.erase(index);
    }

    [[nodiscard]] std::size_t candidate_count() const {
        const std::scoped_lock lock{candidates_mutex_};
        return candidates_.size();
    }

    [[nodiscard]] std::size_t trace_watch_count() const {
        const std::scoped_lock lock{trace_mutex_};
        return trace_watches_.size();
    }

    static const char* state_name(dsnap::RuntimeState state) noexcept {
        switch (state) {
        case dsnap::RuntimeState::Off: return "Off";
        case dsnap::RuntimeState::Calibrating: return "Calibrating";
        case dsnap::RuntimeState::ArmedReady: return "ArmedReady";
        default: return "Disabled/ContractInvalid";
        }
    }

    void f9_keydown() noexcept {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        const std::scoped_lock lock{state_mutex_};
        if (!f9_edge_.key_down()) { ++f9_repeat_rejects_; return; }
        f9_toggle_requested_.store(true, std::memory_order_release);
    }

    void apply_game_thread_control() {
        if (cleanup_requested_.exchange(false, std::memory_order_acq_rel)) {
            unregister_calibration_hooks(); unregister_object_listeners(); clear_activation_state();
        }
        if (calibration_timeout_requested_.exchange(false, std::memory_order_acq_rel)) {
            {
                const std::scoped_lock lock{state_mutex_};
                runtime_state_.reset_off();
                discovery_.cancel();
            }
            armed_.store(false, std::memory_order_release);
            active_.store(false, std::memory_order_release);
            unregister_calibration_hooks();
            unregister_object_listeners();
            clear_activation_state();
            logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                          "state=Off reason=diagnostic_window_complete automatic_actions=0");
        }
        if (!f9_toggle_requested_.exchange(false, std::memory_order_acq_rel)) return;
        {
            const std::scoped_lock lock{state_mutex_};
            const auto next = runtime_state_.press_f9(build_trusted_.load(std::memory_order_acquire), false);
            if (next == dsnap::RuntimeState::Off) {
                armed_.store(false, std::memory_order_release);
                active_.store(false, std::memory_order_release);
                discovery_.cancel();
            } else if (next == dsnap::RuntimeState::Calibrating) {
                armed_.store(true, std::memory_order_release);
                calibration_deadline_ = Clock::now() + kCalibrationWindow;
            } else if (next == dsnap::RuntimeState::ArmedReady) {
                armed_.store(true, std::memory_order_release);
            }
        }
        const auto state = runtime_state_value();
        if (state == dsnap::RuntimeState::Off || state == dsnap::RuntimeState::DisabledContractInvalid) {
            unregister_calibration_hooks();
            unregister_object_listeners();
            clear_activation_state();
        } else if (state == dsnap::RuntimeState::Calibrating) {
            register_object_listeners();
            register_calibration_hooks();
            start_discovery();
            logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_ARMED",
                          "read-only 60-second window started; no ProcessEvent pickup action can be invoked");
        } else if (state == dsnap::RuntimeState::ArmedReady) {
            unregister_calibration_hooks();
            unregister_object_listeners();
            active_.store(false, std::memory_order_release);
        }
        logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                      std::format("state={} trusted={} contract_valid={} world_ready={} active={}",
                                  state_name(state), build_trusted_.load(), false, world_ready_.load(), active_.load()));
        if (state == dsnap::RuntimeState::ArmedReady)
            logger_.write(dsnap::LogAudience::User, "DIAGNOSTIC_FAIL_CLOSED",
                          "unexpected replay state rejected; toggle F9 to return Off");
    }

    [[nodiscard]] dsnap::RuntimeState runtime_state_value() const noexcept {
        const std::scoped_lock lock{state_mutex_};
        return runtime_state_.state();
    }

    void clear_activation_state() {
        { const std::scoped_lock lock{candidates_mutex_}; candidates_.clear(); }
        { const std::scoped_lock lock{trace_mutex_}; trace_watches_.clear(); }
        current_pawn_identity_.store(0, std::memory_order_release);
        current_controller_identity_.store(0, std::memory_order_release);
        player_context_trace_emitted_.store(false, std::memory_order_release);
        trace_call_sequence_.store(0, std::memory_order_release);
        trace_guard_failures_.store(0, std::memory_order_release);
        trace_limit_emitted_.store(false, std::memory_order_release);
        diagnostic_candidate_identity_.store(0, std::memory_order_release);
        diagnostic_component_identity_.store(0, std::memory_order_release);
        diagnostic_eligible_count_.store(0, std::memory_order_release);
        calibration_timeout_requested_.store(false, std::memory_order_release);
    }

    void start_discovery() {
        const auto count = UObjectArray::GetNumElements();
        const std::scoped_lock lock{state_mutex_};
        discovery_.begin(world_epoch_.load(std::memory_order_acquire), count);
        discovery_started_at_ = Clock::now();
        ++discovery_sweeps_started_;
    }

    void register_calibration_hooks() {
        receiver_relation_diagnostic_emitted_.store(false, std::memory_order_release);
        player_context_trace_emitted_.store(false, std::memory_order_release);
        unregister_calibration_hooks();
        for (std::size_t index = 0; index < kCalibrationHookSpecs.size(); ++index) {
            auto* function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kCalibrationHookSpecs[index].path);
            if (!function) continue;
            static_cast<void>(register_calibration_function(function, index, "static"));
        }
    }

    static std::uint32_t property_name_hash(FProperty* property) {
        const auto name = property->GetName();
        std::uint32_t hash = 2166136261U;
        for (const auto ch : name) { hash ^= static_cast<std::uint8_t>(ch); hash *= 16777619U; }
        return hash;
    }

    static void append_trace_text(std::string& output, std::string_view value) {
        if (output.size() >= kMaxTraceTextBytes) return;
        const auto remaining = kMaxTraceTextBytes - output.size();
        output.append(value.substr(0, remaining));
    }

    [[nodiscard]] static std::string object_full_name(UObject* object) {
        return object ? to_string(object->GetFullName()) : "null";
    }

    [[nodiscard]] static std::string object_class_name(UObject* object) {
        auto* object_class = object ? object->GetClassPrivate() : nullptr;
        return object_class ? to_string(object_class->GetPathName()) : "null";
    }

    void watch_trace_object_unsafe(UObject* object, std::string_view source,
                                   std::uint64_t call_sequence) {
        if (!object) return;
        const FWeakObjectPtr weak{object};
        const dsnap::WeakObjectId identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        if (!identity.valid()) return;
        TraceWatch watch{
            identity,
            call_sequence,
            world_epoch_.load(std::memory_order_acquire),
            Clock::now(),
            std::string{source},
            object_full_name(object),
            object_class_name(object),
        };
        const auto key = pack_weak_identity(weak);
        const std::scoped_lock lock{trace_mutex_};
        const auto existing = trace_watches_.find(key);
        if (existing != trace_watches_.end()) {
            if (existing->second.source.find(source) == std::string::npos && existing->second.source.size() < 512) {
                existing->second.source += ",";
                existing->second.source += source;
            }
            existing->second.call_sequence = call_sequence;
            existing->second.captured_at = watch.captured_at;
            return;
        }
        if (trace_watches_.size() < kMaxTraceWatches) trace_watches_.emplace(key, std::move(watch));
    }

    void append_object_properties_unsafe(std::string& output, std::string_view root_label,
                                         UObject* root, std::uint64_t call_sequence) {
        if (!root || !root->GetClassPrivate()) return;
        std::size_t emitted{};
        for (auto* property : TFieldRange<FProperty>(
                 root->GetClassPrivate(), EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->IsA<FObjectProperty>()) continue;
            auto* address = property->ContainerPtrToValuePtr<void>(root);
            auto* value = address ? static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(address) : nullptr;
            if (!value) continue;
            if (emitted++ >= kMaxTraceObjectPropertiesPerRoot) {
                append_trace_text(output, std::format("{}.<limit>;", root_label));
                break;
            }
            const auto property_name = to_string(property->GetName());
            append_trace_text(output, std::format("{}.{}=>{};", root_label, property_name,
                                                  object_full_name(value)));
            watch_trace_object_unsafe(value, std::format("{}.{}", root_label, property_name), call_sequence);
        }
    }

    void trace_pending_candidate_identities() {
        if (runtime_state_value() != dsnap::RuntimeState::Calibrating) return;
        std::vector<CandidateIdentitySnapshot> pending{};
        pending.reserve(16);
        {
            const std::scoped_lock lock{candidates_mutex_};
            for (auto& [index, candidate] : candidates_) {
                if (candidate.identity_logged) continue;
                candidate.identity_logged = true;
                pending.push_back({index, candidate.weak, candidate.derived_class, candidate.discovered});
                if (pending.size() >= 16) break;
            }
        }
        for (const auto& candidate : pending) {
            log_candidate_identity_guarded(candidate.index, candidate.weak,
                                           candidate.derived_class, candidate.discovered);
        }
    }

    void log_candidate_identity_unsafe(std::int32_t index, const FWeakObjectPtr& weak,
                                       bool derived, bool discovered) {
        auto* object = weak.Get();
        if (!object) {
            logger_.write(dsnap::LogAudience::Debug, "TRACE_CANDIDATE_IDENTITY",
                          std::format("object_index={} serial={} valid=0 source={}", index,
                                      weak.ObjectSerialNumber, discovered ? "discovery" : "lifecycle"));
            return;
        }
        const auto flags = object->GetObjectFlags();
        auto* outer1 = object->GetOuterPrivate();
        auto* outer2 = outer1 ? outer1->GetOuterPrivate() : nullptr;
        auto* object_world = object->GetWorld();
        auto current_pawn = unpack_weak_identity(current_pawn_identity_.load(std::memory_order_acquire));
        auto* pawn = current_pawn.Get();
        auto* player_world = pawn ? pawn->GetWorld() : nullptr;
        FVector location{};
        const bool has_location = read_location_guarded(object, &location);
        auto** component_value = object->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractComponent"));
        auto* component = component_value ? *component_value : nullptr;
        auto* interactable = component ? component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue")) : nullptr;
        auto* interact_type = component ? component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue")) : nullptr;
        watch_trace_object_unsafe(object, "drop_item_candidate", 0);
        if (component) watch_trace_object_unsafe(component, "drop_item_candidate.InteractComponent", 0);
        logger_.write(dsnap::LogAudience::Debug, "TRACE_CANDIDATE_IDENTITY",
                      std::format("object_index={} serial={} valid=1 source={} derived={} object={} class={} flags=0x{:08X} "
                                  "cdo={} archetype={} default_subobject={} outer1={} outer2={} world={} player_world={} same_world={} "
                                  "location_valid={} x={:.3f} y={:.3f} z={:.3f} component={} interactable={} interact_type={}",
                                  index, weak.ObjectSerialNumber, discovered ? "discovery" : "lifecycle", derived,
                                  object_full_name(object), object_class_name(object), static_cast<std::uint32_t>(flags),
                                  object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject),
                                  object->HasAnyFlags(EObjectFlags::RF_ArchetypeObject),
                                  object->HasAnyFlags(EObjectFlags::RF_DefaultSubObject),
                                  object_full_name(outer1), object_full_name(outer2),
                                  object_full_name(reinterpret_cast<UObject*>(object_world)),
                                  object_full_name(reinterpret_cast<UObject*>(player_world)),
                                  object_world && player_world && object_world == player_world,
                                  has_location, location.X(), location.Y(), location.Z(), object_full_name(component),
                                  interactable ? std::to_string(*interactable) : "missing",
                                  interact_type ? std::to_string(*interact_type) : "missing"));
    }

    void log_candidate_identity_guarded(std::int32_t index, const FWeakObjectPtr& weak,
                                        bool derived, bool discovered) noexcept {
#if defined(_MSC_VER)
        __try { log_candidate_identity_unsafe(index, weak, derived, discovered); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ++trace_guard_failures_; }
#else
        log_candidate_identity_unsafe(index, weak, derived, discovered);
#endif
    }

    void trace_player_context_unsafe(const PlayerContext& context) {
        auto* world = context.player ? context.player->GetWorld() : nullptr;
        watch_trace_object_unsafe(context.controller, "current_controller", 0);
        watch_trace_object_unsafe(context.player, "current_pawn", 0);
        logger_.write(dsnap::LogAudience::Debug, "TRACE_PLAYER_CONTEXT",
                      std::format("epoch={} mode={} controller={} controller_class={} pawn={} pawn_class={} world={} "
                                  "x={:.3f} y={:.3f} z={:.3f}",
                                  world_epoch_.load(std::memory_order_acquire), static_cast<unsigned>(context.mode),
                                  object_full_name(context.controller), object_class_name(context.controller),
                                  object_full_name(context.player), object_class_name(context.player),
                                  object_full_name(reinterpret_cast<UObject*>(world)),
                                  context.location.X(), context.location.Y(), context.location.Z()));
    }

    void trace_player_context_guarded(const PlayerContext& context) noexcept {
#if defined(_MSC_VER)
        __try { trace_player_context_unsafe(context); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ++trace_guard_failures_; }
#else
        trace_player_context_unsafe(context);
#endif
    }

    void trace_interaction_call_unsafe(std::size_t spec_index,
                                       UnrealScriptFunctionCallableContext& context,
                                       std::string_view phase,
                                       bool force_correlated) {
        if (runtime_state_value() != dsnap::RuntimeState::Calibrating ||
            spec_index >= kCalibrationHookSpecs.size() || !context.Context) return;
        auto* receiver = context.Context;
        auto* outer1 = receiver->GetOuterPrivate();
        auto* outer2 = outer1 ? outer1->GetOuterPrivate() : nullptr;
        auto* receiver_world = receiver->GetWorld();
        auto* function = static_cast<UFunction*>(context.TheStack.Node());
        void* parameters = context.TheStack.Locals();
        auto controller_weak = unpack_weak_identity(current_controller_identity_.load(std::memory_order_acquire));
        auto pawn_weak = unpack_weak_identity(current_pawn_identity_.load(std::memory_order_acquire));
        auto* controller = controller_weak.Get();
        auto* pawn = pawn_weak.Get();
        auto* player_world = pawn ? pawn->GetWorld() : nullptr;
        const auto locked_owner_identity = diagnostic_candidate_identity_.load(std::memory_order_acquire);
        const auto locked_component_identity = diagnostic_component_identity_.load(std::memory_order_acquire);
        const FWeakObjectPtr receiver_weak{receiver};
        const FWeakObjectPtr outer1_weak{outer1};
        const auto receiver_identity = pack_weak_identity(receiver_weak);
        const auto outer1_identity = pack_weak_identity(outer1_weak);
        const bool direct_locked_relation = receiver_identity == locked_owner_identity ||
                                            receiver_identity == locked_component_identity ||
                                            outer1_identity == locked_owner_identity;
        bool parameter_locked_relation{};
        if (function) {
            for (auto* property : TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated)) {
                if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                    property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm) ||
                    !property->IsA<FObjectProperty>()) continue;
                auto* address = parameters ? property->ContainerPtrToValuePtr<void>(parameters) : nullptr;
                auto* value = address ? static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(address) : nullptr;
                if (!value) continue;
                const FWeakObjectPtr value_weak{value};
                const auto value_identity = pack_weak_identity(value_weak);
                if (value_identity == locked_owner_identity || value_identity == locked_component_identity) {
                    parameter_locked_relation = true;
                    break;
                }
            }
        }
        auto** receiver_target_object = receiver->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetObject"));
        auto** receiver_target_component = receiver->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetComponent"));
        auto* target_object = receiver_target_object ? *receiver_target_object : nullptr;
        auto* target_component = receiver_target_component ? *receiver_target_component : nullptr;
        const FWeakObjectPtr target_object_weak{target_object};
        const FWeakObjectPtr target_component_weak{target_component};
        const bool receiver_field_locked_relation =
            pack_weak_identity(target_object_weak) == locked_owner_identity ||
            pack_weak_identity(target_component_weak) == locked_component_identity;
        if (!force_correlated && !direct_locked_relation && !parameter_locked_relation &&
            !receiver_field_locked_relation) return;

        const auto sequence = trace_call_sequence_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (sequence > kMaxTraceCalls) {
            if (!trace_limit_emitted_.exchange(true, std::memory_order_acq_rel)) {
                logger_.write(dsnap::LogAudience::User, "TRACE_LIMIT_REACHED",
                              "correlated interaction trace reached 256 calls; start a fresh bounded window");
            }
            return;
        }

        watch_trace_object_unsafe(receiver, "receiver", sequence);
        watch_trace_object_unsafe(outer1, "receiver.outer1", sequence);
        watch_trace_object_unsafe(outer2, "receiver.outer2", sequence);

        std::string parameter_text{};
        if (function) {
            for (auto* property : TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated)) {
                if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                    property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm)) continue;
                const auto name = to_string(property->GetName());
                auto* address = parameters ? property->ContainerPtrToValuePtr<void>(parameters) : nullptr;
                if (!address) {
                    append_trace_text(parameter_text, std::format("{}:unavailable;", name));
                } else if (property->IsA<FObjectProperty>()) {
                    auto* value = static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(address);
                    append_trace_text(parameter_text, std::format("{}:object={};", name, object_full_name(value)));
                    if (value) watch_trace_object_unsafe(value, std::format("parameter.{}", name), sequence);
                } else if (property->IsA<FBoolProperty>()) {
                    append_trace_text(parameter_text, std::format("{}:bool={};", name,
                                                                  static_cast<FBoolProperty*>(property)->GetPropertyValue(address)));
                } else if (property->IsA<FByteProperty>() ||
                           (property->IsA<FEnumProperty>() && property->GetSize() == 1)) {
                    append_trace_text(parameter_text, std::format("{}:byte={};", name,
                                                                  *static_cast<std::uint8_t*>(address)));
                } else if (property->IsA<FIntProperty>()) {
                    append_trace_text(parameter_text, std::format("{}:int32={};", name,
                                                                  *static_cast<std::int32_t*>(address)));
                } else {
                    append_trace_text(parameter_text, std::format("{}:unsupported_size={};", name, property->GetSize()));
                }
            }
        }

        std::string object_properties{};
        append_object_properties_unsafe(object_properties, "receiver", receiver, sequence);
        append_object_properties_unsafe(object_properties, "outer1", outer1, sequence);
        append_object_properties_unsafe(object_properties, "controller", controller, sequence);
        append_object_properties_unsafe(object_properties, "pawn", pawn, sequence);
        auto locked_owner_weak = unpack_weak_identity(locked_owner_identity);
        auto locked_component_weak = unpack_weak_identity(locked_component_identity);
        auto* locked_owner = locked_owner_weak.Get();
        auto* locked_component = locked_component_weak.Get();
        append_object_properties_unsafe(object_properties, "locked_owner", locked_owner, sequence);
        append_object_properties_unsafe(object_properties, "locked_component", locked_component, sequence);
        auto* locked_interactable = locked_component
            ? locked_component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue")) : nullptr;
        auto* locked_interact_type = locked_component
            ? locked_component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue")) : nullptr;

        logger_.write(dsnap::LogAudience::Debug, "TRACE_INTERACTION_CALL",
                      std::format("sequence={} phase={} function_id={} forced_from_correlated_pre={} direct_locked_relation={} parameter_locked_relation={} "
                                  "receiver_field_locked_relation={} configured_path={} actual_path={} receiver={} receiver_class={} "
                                  "flags=0x{:08X} outer1={} outer2={} receiver_world={} player_world={} same_world={} "
                                  "controller={} pawn={} locked_owner={} locked_component={} locked_interactable={} "
                                  "locked_interact_type={} parameters=[{}] object_properties=[{}]",
                                  sequence, phase, spec_index, force_correlated, direct_locked_relation, parameter_locked_relation,
                                  receiver_field_locked_relation,
                                  to_string(kCalibrationHookSpecs[spec_index].path),
                                  function ? to_string(function->GetPathName()) : "null", object_full_name(receiver),
                                  object_class_name(receiver), static_cast<std::uint32_t>(receiver->GetObjectFlags()),
                                  object_full_name(outer1), object_full_name(outer2),
                                  object_full_name(reinterpret_cast<UObject*>(receiver_world)),
                                  object_full_name(reinterpret_cast<UObject*>(player_world)),
                                  receiver_world && player_world && receiver_world == player_world,
                                  object_full_name(controller), object_full_name(pawn),
                                  object_full_name(locked_owner), object_full_name(locked_component),
                                  locked_interactable ? std::to_string(*locked_interactable) : "missing",
                                  locked_interact_type ? std::to_string(*locked_interact_type) : "missing",
                                  parameter_text, object_properties));
    }

    void trace_interaction_call_guarded(std::size_t spec_index,
                                        UnrealScriptFunctionCallableContext& context,
                                        std::string_view phase,
                                        bool force_correlated = false) noexcept {
#if defined(_MSC_VER)
        __try { trace_interaction_call_unsafe(spec_index, context, phase, force_correlated); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ++trace_guard_failures_; }
#else
        trace_interaction_call_unsafe(spec_index, context, phase, force_correlated);
#endif
    }

    static bool read_deleted_serial_guarded(std::int32_t index, std::int32_t* serial) noexcept {
        bool ok{};
#if defined(_MSC_VER)
        __try {
            auto* item = UObjectArray::IndexToObject(index);
            if (item) { *serial = item->GetSerialNumber(); ok = true; }
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
#else
        auto* item = UObjectArray::IndexToObject(index);
        if (item) { *serial = item->GetSerialNumber(); ok = true; }
#endif
        return ok;
    }

    void trace_delete_match(std::int32_t index) {
        std::int32_t observed_serial{};
        const bool serial_available = read_deleted_serial_guarded(index, &observed_serial);
        std::vector<TraceWatch> matches{};
        {
            const std::scoped_lock lock{trace_mutex_};
            for (auto it = trace_watches_.begin(); it != trace_watches_.end();) {
                const bool index_match = it->second.identity.object_index == index;
                const bool serial_match = !serial_available || observed_serial == 0 ||
                                          it->second.identity.serial_number == observed_serial;
                if (index_match && serial_match) {
                    matches.push_back(it->second);
                    it = trace_watches_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        const auto now = Clock::now();
        for (const auto& match : matches) {
            logger_.write(dsnap::LogAudience::Debug, "TRACE_DELETE_MATCH",
                          std::format("object_index={} stored_serial={} observed_serial={} serial_available={} sequence={} "
                                      "age_ms={} epoch={} source={} object={} class={}",
                                      index, match.identity.serial_number, observed_serial, serial_available,
                                      match.call_sequence,
                                      std::chrono::duration_cast<std::chrono::milliseconds>(now - match.captured_at).count(),
                                      match.world_epoch, match.source, match.object_name, match.class_name));
        }
    }

    void expire_trace_watches(Clock::time_point now) {
        const std::scoped_lock lock{trace_mutex_};
        for (auto it = trace_watches_.begin(); it != trace_watches_.end();) {
            if (now - it->second.captured_at > kTraceDeleteWindow) it = trace_watches_.erase(it);
            else ++it;
        }
    }

    [[nodiscard]] bool candidate_component_matches(UObject* owner, UObject* component) const {
        if (!owner || !component || !owner->IsA(drop_item_class_)) return false;
        auto** value = owner->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractComponent"));
        return value && *value == component;
    }

    [[nodiscard]] bool find_exact_object_property(UObject* root, UObject* expected,
                                                  std::uint32_t* name_hash) const {
        if (!root || !expected || !name_hash || !root->GetClassPrivate()) return false;
        FProperty* matched{};
        for (auto* property : TFieldRange<FProperty>(
                 root->GetClassPrivate(), EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->IsA<FObjectProperty>()) continue;
            auto* value = property->ContainerPtrToValuePtr<void>(root);
            if (!value || static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(value) != expected) continue;
            if (matched) return false;
            matched = property;
        }
        if (!matched) return false;
        *name_hash = property_name_hash(matched);
        return *name_hash != 0;
    }

    [[nodiscard]] UObject* resolve_exact_object_property(UObject* root, std::uint32_t name_hash) const {
        if (!root || name_hash == 0 || !root->GetClassPrivate()) return nullptr;
        FProperty* matched{};
        for (auto* property : TFieldRange<FProperty>(
                 root->GetClassPrivate(), EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->IsA<FObjectProperty>() || property_name_hash(property) != name_hash) continue;
            if (matched) return nullptr;
            matched = property;
        }
        if (!matched) return nullptr;
        auto* value = matched->ContainerPtrToValuePtr<void>(root);
        return value ? static_cast<FObjectPropertyBase*>(matched)->GetObjectPropertyValue(value) : nullptr;
    }

    [[nodiscard]] std::string matching_object_properties(UObject* root, UObject* first, UObject* second) const {
        if (!root || !root->GetClassPrivate()) return "none";
        std::string matches{};
        for (auto* property : TFieldRange<FProperty>(
                 root->GetClassPrivate(), EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->IsA<FObjectProperty>()) continue;
            auto* value = property->ContainerPtrToValuePtr<void>(root);
            auto* object = value ? static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(value) : nullptr;
            if (!object || (object != first && object != second)) continue;
            if (!matches.empty()) matches += ",";
            matches += to_string(property->GetName());
            matches += object == first ? "=receiver" : "=outer1";
        }
        return matches.empty() ? "none" : matches;
    }

    void log_receiver_relation_diagnostic(UObject* receiver, UObject* controller, UObject* pawn) {
        if (receiver_relation_diagnostic_emitted_.exchange(true, std::memory_order_acq_rel)) return;
        auto* outer1 = receiver ? receiver->GetOuterPrivate() : nullptr;
        auto* outer2 = outer1 ? outer1->GetOuterPrivate() : nullptr;
        std::string candidate_components{};
        std::vector<CandidateSnapshot> snapshots{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            snapshots.reserve(candidates_.size());
            for (const auto& [index, candidate] : candidates_)
                snapshots.push_back({index, candidate.weak.ObjectSerialNumber, candidate.weak});
        }
        for (const auto& snapshot : snapshots) {
            if (candidate_components.size() > 1024) break;
            auto* owner = snapshot.weak.Get();
            auto** value = owner ? owner->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractComponent")) : nullptr;
            auto* component = value ? *value : nullptr;
            if (!candidate_components.empty()) candidate_components += ";";
            candidate_components += std::format("{}:{}", snapshot.index,
                                                component ? to_string(component->GetFullName()) : "null");
        }
        logger_.write(dsnap::LogAudience::Debug, "CALIBRATION_RECEIVER_RELATION",
                      std::format("receiver={} outer1={} outer2={} controller={} pawn={} controller_matches={} pawn_matches={} candidate_components=[{}]",
                                  receiver ? to_string(receiver->GetFullName()) : "null",
                                  outer1 ? to_string(outer1->GetFullName()) : "null",
                                  outer2 ? to_string(outer2->GetFullName()) : "null",
                                  controller ? to_string(controller->GetFullName()) : "null",
                                  pawn ? to_string(pawn->GetFullName()) : "null",
                                  matching_object_properties(controller, receiver, outer1),
                                  matching_object_properties(pawn, receiver, outer1), candidate_components));
    }

    bool correlate_eligible_candidates(UObject* pawn,
                                       std::array<dsnap::WeakObjectId, kCandidatesPerPulse>* correlated,
                                       std::uint8_t* correlated_count,
                                       CalibrationRejectReason* reason) {
        FVector location{};
        if (!read_location_guarded(pawn, &location)) {
            *reason = CalibrationRejectReason::PlayerLocationUnavailable;
            return false;
        }
        std::vector<CandidateSnapshot> snapshots{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            snapshots.reserve(candidates_.size());
            for (const auto& [index, candidate] : candidates_)
                snapshots.push_back({index, candidate.weak.ObjectSerialNumber, candidate.weak});
        }
        for (const auto& snapshot : snapshots) {
            auto* owner = snapshot.weak.Get();
            GateOutput gate{};
            if (!owner || !evaluate_candidate_guarded(this, owner, &location, &gate) || gate.result != GateResult::Eligible)
                continue;
            if (*correlated_count >= correlated->size()) {
                ++calibration_ambiguity_rejections_;
                *reason = CalibrationRejectReason::CandidateAmbiguous;
                return false;
            }
            (*correlated)[(*correlated_count)++] = {snapshot.index, snapshot.serial};
        }
        if (*correlated_count == 0) {
            *reason = CalibrationRejectReason::CandidateRelationMissing;
            return false;
        }
        return true;
    }

    bool correlate_candidate_component(UObject* component,
                                       std::array<dsnap::WeakObjectId, kCandidatesPerPulse>* correlated,
                                       std::uint8_t* correlated_count,
                                       CalibrationRejectReason* reason) {
        std::vector<CandidateSnapshot> snapshots{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            snapshots.reserve(candidates_.size());
            for (const auto& [index, candidate] : candidates_)
                snapshots.push_back({index, candidate.weak.ObjectSerialNumber, candidate.weak});
        }
        for (const auto& snapshot : snapshots) {
            auto* owner = snapshot.weak.Get();
            if (!candidate_component_matches(owner, component)) continue;
            if (*correlated_count != 0) {
                ++calibration_ambiguity_rejections_;
                *reason = CalibrationRejectReason::CandidateAmbiguous;
                return false;
            }
            (*correlated)[0] = {snapshot.index, snapshot.serial};
            *correlated_count = 1;
        }
        if (*correlated_count == 0) {
            *reason = CalibrationRejectReason::CandidateRelationMissing;
            return false;
        }
        return true;
    }

    void record_calibration_capture(std::size_t spec_index, bool ok, CalibrationRejectReason reason) {
        ++contract_hook_calls_;
        if (spec_index < calibration_hook_calls_by_spec_.size()) ++calibration_hook_calls_by_spec_[spec_index];
        if (ok) return;
        ++contract_hook_rejections_;
        if (spec_index < calibration_hook_rejections_by_spec_.size()) ++calibration_hook_rejections_by_spec_[spec_index];
        const auto reason_index = static_cast<std::size_t>(reason);
        if (reason_index < calibration_rejection_reasons_.size() &&
            calibration_rejection_reasons_[reason_index].fetch_add(1, std::memory_order_relaxed) == 0) {
            logger_.write(dsnap::LogAudience::Debug, "CALIBRATION_HOOK_REJECTED",
                          std::format("function_id={} reason={}", spec_index, calibration_reject_reason_name(reason)));
        }
    }

    void calibration_pre_hook(std::size_t spec_index, UnrealScriptFunctionCallableContext& context) noexcept {
        const auto started = Clock::now();
        const auto before = trace_call_sequence_.load(std::memory_order_acquire);
        trace_interaction_call_guarded(spec_index, context, "pre");
        if (trace_call_sequence_.load(std::memory_order_acquire) == before) return;
        {
            std::uint32_t expected{};
            static_cast<void>(correlated_pre_pending_[spec_index].compare_exchange_strong(
                expected, 1, std::memory_order_acq_rel));
        }
        bool ok{};
        CalibrationRejectReason reason{CalibrationRejectReason::GuardedException};
#if defined(_MSC_VER)
        __try { ok = capture_calibration_call(spec_index, context, &reason); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; reason = CalibrationRejectReason::GuardedException; }
#else
        ok = capture_calibration_call(spec_index, context, &reason);
#endif
        record_calibration_capture(spec_index, ok, reason);
        contract_hook_total_us_.fetch_add(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count()));
    }

    void calibration_post_hook(std::size_t spec_index, UnrealScriptFunctionCallableContext& context) noexcept {
        if (spec_index >= correlated_pre_pending_.size()) return;
        if (correlated_pre_pending_[spec_index].exchange(0, std::memory_order_acq_rel) != 0)
            trace_interaction_call_guarded(spec_index, context, "post", true);
    }

    bool capture_calibration_call(std::size_t spec_index, UnrealScriptFunctionCallableContext& context,
                                  CalibrationRejectReason* reason) {
        auto reject = [reason](CalibrationRejectReason value) { if (reason) *reason = value; return false; };
        if (!reason || runtime_state_value() != dsnap::RuntimeState::Calibrating ||
            spec_index >= kCalibrationHookSpecs.size() || !context.Context)
            return reject(CalibrationRejectReason::StateOrContext);
        const auto& spec = kCalibrationHookSpecs[spec_index];
        std::array<dsnap::WeakObjectId, kCandidatesPerPulse> correlated{};
        std::uint8_t correlated_count{};
        const auto locked_packed = diagnostic_candidate_identity_.load(std::memory_order_acquire);
        const auto locked_component_packed = diagnostic_component_identity_.load(std::memory_order_acquire);
        const auto locked_weak = unpack_weak_identity(locked_packed);
        auto* locked_owner = locked_weak.Get();
        auto locked_component_weak = unpack_weak_identity(locked_component_packed);
        auto* locked_component = locked_component_weak.Get();
        if (!locked_owner || !locked_component) return reject(CalibrationRejectReason::CandidateRelationMissing);
        auto captured_receiver = spec.receiver;
        std::uint32_t captured_receiver_property_hash{};
        if (spec.owner_kind == HookSpec::OwnerKind::DropItemActor) {
            if (context.Context != locked_owner) return reject(CalibrationRejectReason::ReceiverMismatch);
            correlated[0] = {locked_weak.ObjectIndex, locked_weak.ObjectSerialNumber};
            correlated_count = 1;
        } else if (spec.receiver == dsnap::ReceiverRole::CandidateComponent) {
            CalibrationRejectReason component_reason{CalibrationRejectReason::CandidateRelationMissing};
            if (!correlate_candidate_component(context.Context, &correlated, &correlated_count, &component_reason)) {
                correlated_count = 0;
                auto controller_weak = unpack_weak_identity(current_controller_identity_.load(std::memory_order_acquire));
                auto pawn_weak = unpack_weak_identity(current_pawn_identity_.load(std::memory_order_acquire));
                auto* controller = controller_weak.Get();
                auto* pawn = pawn_weak.Get();
                if (find_exact_object_property(controller, context.Context, &captured_receiver_property_hash)) {
                    captured_receiver = dsnap::ReceiverRole::CurrentControllerProperty;
                } else if (find_exact_object_property(pawn, context.Context, &captured_receiver_property_hash)) {
                    captured_receiver = dsnap::ReceiverRole::CurrentPawnProperty;
                } else {
                    log_receiver_relation_diagnostic(context.Context, controller, pawn);
                    return reject(component_reason);
                }
                if (!correlate_eligible_candidates(pawn, &correlated, &correlated_count, reason)) return false;
            }
        } else {
            const FWeakObjectPtr receiver{context.Context};
            const auto expected = spec.receiver == dsnap::ReceiverRole::CurrentController
                                      ? current_controller_identity_.load()
                                      : current_pawn_identity_.load();
            if (pack_weak_identity(receiver) != expected) return reject(CalibrationRejectReason::ReceiverMismatch);
            UObject* pawn = context.Context;
            if (spec.receiver == dsnap::ReceiverRole::CurrentController) {
                auto** pawn_value = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("Pawn"));
                pawn = pawn_value ? *pawn_value : nullptr;
            }
            if (!correlate_eligible_candidates(pawn, &correlated, &correlated_count, reason)) return false;
        }
        if (correlated_count != 1 || correlated[0].object_index != locked_weak.ObjectIndex ||
            correlated[0].serial_number != locked_weak.ObjectSerialNumber)
            return reject(CalibrationRejectReason::CandidateRelationMissing);
        auto* function = static_cast<UFunction*>(context.TheStack.Node());
        void* parameters = context.TheStack.Locals();
        if (!function || !hook_owner_is_allowed(function, spec_index) || function->GetName() != spec.name ||
            (!parameters && function->GetParmsSize() != 0))
            return reject(CalibrationRejectReason::ParameterMetadataInvalid);
        dsnap::RuntimeContract contract{};
        contract.game_sha256 = fingerprint_result_.game_sha256;
        contract.ue4ss_sha256 = fingerprint_result_.ue4ss_sha256;
        contract.ue4ss_git_sha = dsnap::kExpectedUe4ssGitSha;
        contract.function_path = to_string(function->GetPathName());
        contract.function = spec.replay;
        contract.receiver = captured_receiver;
        contract.receiver_property_hash = captured_receiver_property_hash;
        for (auto* property : TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm)) continue;
            if (contract.parameter_count >= dsnap::kMaxContractParameters)
                return reject(CalibrationRejectReason::ParameterMetadataInvalid);
            auto* value = property->ContainerPtrToValuePtr<void>(parameters);
            if (!value) return reject(CalibrationRejectReason::ParameterMetadataInvalid);
            dsnap::ContractParameter captured{property_name_hash(property), {}, 0};
            if (property->IsA<FObjectProperty>()) {
                auto* object = static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(value);
                if (object && object->IsA(drop_item_class_)) {
                    const FWeakObjectPtr object_weak{object};
                    bool matched{};
                    for (std::size_t i = 0; i < correlated_count; ++i)
                        if (correlated[i].object_index == object_weak.ObjectIndex &&
                            correlated[i].serial_number == object_weak.ObjectSerialNumber) matched = true;
                    if (!matched) return reject(CalibrationRejectReason::CandidateRelationMissing);
                    captured.kind = dsnap::ParameterKind::CandidateObject;
                    correlated[0] = {object_weak.ObjectIndex, object_weak.ObjectSerialNumber};
                    correlated_count = 1;
                } else if (object) {
                    const FWeakObjectPtr weak{object};
                    const auto identity = pack_weak_identity(weak);
                    if (identity == current_pawn_identity_.load()) captured.kind = dsnap::ParameterKind::CurrentPawn;
                    else if (identity == current_controller_identity_.load()) captured.kind = dsnap::ParameterKind::CurrentController;
                    else return reject(CalibrationRejectReason::UnsupportedParameter);
                } else return reject(CalibrationRejectReason::UnsupportedParameter);
            } else if (property->IsA<FBoolProperty>()) {
                captured.kind = dsnap::ParameterKind::Bool;
                captured.scalar = static_cast<FBoolProperty*>(property)->GetPropertyValue(value) ? 1 : 0;
            } else if (property->IsA<FByteProperty>() ||
                       (property->IsA<FEnumProperty>() && property->GetSize() == 1)) {
                captured.kind = dsnap::ParameterKind::Byte;
                captured.scalar = *static_cast<std::uint8_t*>(value);
            } else if (property->IsA<FIntProperty>()) {
                captured.kind = dsnap::ParameterKind::Int32;
                captured.scalar = *static_cast<std::int32_t*>(value);
            } else return reject(CalibrationRejectReason::UnsupportedParameter);
            contract.parameters[contract.parameter_count++] = captured;
        }
        // The diagnostic deliberately does not turn an observed call into a
        // replay contract. Structural capture is logged only so the exact
        // manual sequence can be reviewed before any later implementation.
        if (!contract.structurally_valid()) return reject(CalibrationRejectReason::ParameterMetadataInvalid);
        *reason = CalibrationRejectReason::None;
        ++calibration_sequence_;
        ++calibration_calls_observed_;
        logger_.write(dsnap::LogAudience::Debug, "CALIBRATION_CHAIN_CALL",
                      std::format("function_id={} receiver_role={} receiver_property_hash={} replay_eligible={} parameter_count={} correlated_candidates={} path={}",
                                  spec_index, static_cast<unsigned>(contract.receiver),
                                  contract.receiver_property_hash, spec.replay_eligible,
                                  contract.parameter_count, correlated_count,
                                  contract.function_path));
        logger_.write(dsnap::LogAudience::Debug, "TRACE_CORRELATION_RESULT",
                      std::format("sequence={} function_id={} correlated_candidates={} result=observed_only contract_persistence=disabled",
                                  calibration_sequence_, spec_index, correlated_count));
        return true;
    }

    void reset_world(const char* source) {
        active_.store(false, std::memory_order_release);
        world_ready_.store(false, std::memory_order_release);
        next_pulse_due_ = {};
        {
            const std::scoped_lock lock{candidates_mutex_};
            candidates_.clear();
        }
        { const std::scoped_lock lock{trace_mutex_}; trace_watches_.clear(); }
        current_pawn_identity_.store(0, std::memory_order_release);
        current_controller_identity_.store(0, std::memory_order_release);
        player_context_trace_emitted_.store(false, std::memory_order_release);
        trace_call_sequence_.store(0, std::memory_order_release);
        trace_guard_failures_.store(0, std::memory_order_release);
        trace_limit_emitted_.store(false, std::memory_order_release);
        diagnostic_candidate_identity_.store(0, std::memory_order_release);
        diagnostic_component_identity_.store(0, std::memory_order_release);
        diagnostic_eligible_count_.store(0, std::memory_order_release);
        calibration_timeout_requested_.store(false, std::memory_order_release);
        ++world_epoch_;
        {
            const std::scoped_lock lock{state_mutex_};
            discovery_.cancel();
            runtime_state_.reset_off();
        }
        armed_.store(false, std::memory_order_release);
        cleanup_requested_.store(true, std::memory_order_release);
        ++world_resets_;
        logger_.write(dsnap::LogAudience::Debug, "WORLD_RESET",
                      std::format("source={} active=0 candidates=0 armed=0 diagnostic_cancelled=1", source));
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};
    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    std::future<dsnap::FingerprintResult> fingerprint_future_;
    dsnap::FingerprintResult fingerprint_result_{};
    mutable std::mutex candidates_mutex_{};
    std::unordered_map<std::int32_t, CandidateState> candidates_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<bool> uobject_array_shutdown_seen_{};
    std::atomic<bool> armed_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> world_ready_{};
    std::atomic<bool> active_{};
    std::atomic<std::uint64_t> create_callbacks_{};
    std::atomic<std::uint64_t> drop_item_captures_{};
    std::atomic<std::uint64_t> base_class_captures_{};
    std::atomic<std::uint64_t> derived_class_captures_{};
    std::atomic<std::uint64_t> delete_callbacks_{};
    std::atomic<std::uint64_t> queue_rejections_{};
    std::atomic<std::uint64_t> engine_tick_callbacks_{};
    std::atomic<std::uint64_t> candidate_nonempty_pulses_{};
    std::atomic<std::uint64_t> candidate_batches_{};
    std::atomic<std::uint64_t> candidates_evaluated_{};
    std::atomic<std::uint64_t> idle_pulses_{};
    std::atomic<std::uint64_t> throttle_rejects_{};
    std::atomic<std::uint64_t> player_chain_attempts_{};
    std::atomic<std::uint64_t> player_chain_successes_{};
    std::atomic<std::uint64_t> player_chain_failures_{};
    std::atomic<std::uint64_t> player_chain_total_us_{};
    std::atomic<std::uint64_t> player_chain_max_us_{};
    std::atomic<std::uint64_t> expected_character_pulses_{};
    std::atomic<std::uint64_t> alternate_pawn_pulses_{};
    std::atomic<std::uint64_t> gate_rejections_{};
    std::atomic<std::uint64_t> seh_rejections_{};
    std::atomic<std::uint64_t> contract_hook_calls_{};
    std::atomic<std::uint64_t> contract_hook_rejections_{};
    std::atomic<std::uint64_t> contract_hook_failures_{};
    std::atomic<std::uint64_t> contract_hook_total_us_{};
    std::atomic<std::uint64_t> calibration_functions_examined_{};
    std::atomic<std::uint64_t> calibration_functions_registered_{};
    std::array<std::atomic<std::uint64_t>, kCalibrationHookSpecs.size()> calibration_hook_calls_by_spec_{};
    std::array<std::atomic<std::uint64_t>, kCalibrationHookSpecs.size()> calibration_hook_rejections_by_spec_{};
    std::array<std::atomic<std::uint32_t>, kCalibrationHookSpecs.size()> correlated_pre_pending_{};
    std::array<std::atomic<std::uint64_t>, kCalibrationRejectReasonCount> calibration_rejection_reasons_{};
    std::atomic<std::uint64_t> calibration_calls_observed_{};
    std::atomic<std::uint64_t> contracts_validated_by_delete_{};
    std::atomic<std::uint64_t> contracts_loaded_{};
    std::atomic<std::uint64_t> contract_rejections_{};
    std::atomic<std::uint64_t> contract_persist_failures_{};
    std::atomic<std::uint64_t> contracts_quarantined_{};
    std::atomic<std::uint64_t> discovery_sweeps_started_{};
    std::atomic<std::uint64_t> discovery_sweeps_completed_{};
    std::atomic<std::uint64_t> discovery_objects_examined_{};
    std::atomic<std::uint64_t> discovery_candidates_found_{};
    std::atomic<std::uint64_t> discovery_total_us_{};
    std::atomic<std::uint64_t> discovery_max_batch_us_{};
    std::atomic<std::uint64_t> lifecycle_candidates_found_{};
    std::atomic<std::uint64_t> lifecycle_filter_total_us_{};
    std::atomic<std::uint64_t> calibration_ambiguity_rejections_{};
    std::atomic<std::uint64_t> world_resets_{};
    std::array<std::atomic<std::uint64_t>, dsnap::kGateReasonCount> gate_reason_counts_{};
    std::array<std::atomic<std::uint64_t>, dsnap::kPlayerChainReasonCount> player_chain_reason_counts_{};
    mutable std::mutex player_chain_diagnostic_mutex_{};
    dsnap::BoundedPlayerChainDiagnosticState player_chain_diagnostics_{};
    mutable std::mutex state_mutex_{};
    dsnap::CalibrationStateMachine runtime_state_{};
    dsnap::RisingEdgeLatch f9_edge_{};
    dsnap::IncrementalDiscoveryState discovery_{};
    std::atomic<std::uint64_t> f9_repeat_rejects_{};
    std::atomic<std::uint64_t> world_epoch_{1};
    std::atomic<std::uint64_t> current_pawn_identity_{};
    std::atomic<std::uint64_t> current_controller_identity_{};
    std::atomic<bool> receiver_relation_diagnostic_emitted_{};
    std::atomic<bool> player_context_trace_emitted_{};
    mutable std::mutex trace_mutex_{};
    std::unordered_map<std::uint64_t, TraceWatch> trace_watches_{};
    std::atomic<std::uint64_t> trace_call_sequence_{};
    std::atomic<std::uint64_t> trace_guard_failures_{};
    std::atomic<bool> trace_limit_emitted_{};
    mutable std::mutex calibration_mutex_{};
    std::vector<RegisteredFunctionHook> calibration_hooks_{};
    std::uint64_t calibration_sequence_{};
    Clock::time_point calibration_deadline_{};
    Clock::time_point discovery_started_at_{};
    std::atomic<bool> f9_toggle_requested_{};
    std::atomic<bool> cleanup_requested_{};
    std::atomic<bool> calibration_timeout_requested_{};
    UClass* drop_item_class_{};
    UClass* interactable_component_class_{};
    UClass* player_character_class_{};
    UClass* player_controller_class_{};
    UFunction* location_function_{};
    std::atomic<std::uint64_t> diagnostic_candidate_identity_{};
    std::atomic<std::uint64_t> diagnostic_component_identity_{};
    std::atomic<std::size_t> diagnostic_eligible_count_{};
    Clock::time_point next_pulse_due_{};
    Clock::time_point next_perf_log_{};
    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};
    std::atomic<bool> listeners_registered_{};
    bool fingerprint_applied_{};
};

} // namespace

#define DSNAP_API __declspec(dllexport)
extern "C" {
DSNAP_API RC::CppUserModBase* start_mod() { return new NativeAutoPickup(); }
DSNAP_API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
