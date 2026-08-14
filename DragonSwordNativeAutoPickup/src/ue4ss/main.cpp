// OWNER_AUTHORIZED_ACTIVE_PICKUP targeting pinned RE-UE4SS v3.0.1.
// Active pickup uses the exact interaction chain observed on one ordinary
// DropItemActor derivative in the 0.6.0 manual same-object trace. Applying that
// chain to other structurally identical DropItemActor instances remains a
// runtime hypothesis until owner gameplay acceptance.

#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/discovery_state.hpp>
#include <dsnap/gate_attribution.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/runtime_contract.hpp>
#include <dsnap/single_target_latch.hpp>
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
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#include <Unreal/World.hpp>
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
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

extern "C" IMAGE_DOS_HEADER __ImageBase;

[[nodiscard]] bool pin_own_module_for_process_lifetime() noexcept {
    HMODULE module{};
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                              reinterpret_cast<LPCWSTR>(&__ImageBase), &module) != FALSE;
}

[[nodiscard]] ProcessShutdownProbe resolve_process_shutdown_probe() noexcept {
    const auto module = GetModuleHandleW(L"ntdll.dll");
    if (!module) return nullptr;
    const auto procedure = GetProcAddress(module, "RtlDllShutdownInProgress");
    static_assert(sizeof(procedure) == sizeof(ProcessShutdownProbe));
    return procedure ? std::bit_cast<ProcessShutdownProbe>(procedure) : nullptr;
}

constexpr auto kVersion = STR("0.7.6-qualified-f9-release");
constexpr auto kDiagnosticLabel = "OWNER_AUTHORIZED_ACTIVE_PICKUP";
constexpr auto kDropItemClass = STR("/Script/DS.DropItemActor");
constexpr auto kInteractableComponentClass = STR("/Script/DS.DInteractableComponent");
constexpr auto kPlayerCharacterClass = STR("/Script/DS.DsPlayerCharacter");
constexpr auto kPlayerControllerClass = STR("/Script/DS.DsPlayerController");
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kPickupFunction = STR("/Script/DS.DInteractableComponent:Server_RunInteractV2");
constexpr auto kPulseInterval = std::chrono::milliseconds{150};
constexpr std::uint8_t kRequiredInteractableValue = 2;
constexpr std::size_t kCalibrationCandidateLimit = 8;
constexpr std::int32_t kDiscoveryObjectsPerPulse = 16384;
constexpr auto kDiscoveryBatchBudget = std::chrono::microseconds{2000};
constexpr auto kSelectionBudget = std::chrono::milliseconds{4};
constexpr auto kF9Debounce = std::chrono::milliseconds{750};
constexpr auto kF9ReleaseQualification = std::chrono::milliseconds{250};
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
    bool derived_class{};
};

struct CandidateIdentitySnapshot {
    std::int32_t index{-1};
    FWeakObjectPtr weak{};
    bool derived_class{};
    bool discovered{};
};

enum class GateResult { Eligible, RetryLater, PermanentReject, Invalid };

enum class CandidateProbeStatus : std::uint8_t { Evaluated, StaleIdentity, DifferentWorld, GuardedException };

enum class ActionGateReason : std::uint8_t {
    Ready,
    DiscoveryIncomplete,
    RegistryOverflow,
    LifecycleThreadMismatch,
    CandidateRegistryFault,
    UnsupportedPlayerMode,
    SelectionBudgetExceeded,
    InputInvalid,
    WorldMismatch,
    CandidateComponentMismatch,
    ReceiverPropertyMissing,
    ReceiverInvalid,
    TargetPropertyMissing,
    TargetObjectBusy,
    TargetComponentBusy,
    InvocationLatched,
    GuardedException,
};

[[nodiscard]] const char* action_gate_reason_name(ActionGateReason reason) noexcept {
    switch (reason) {
    case ActionGateReason::Ready: return "ready";
    case ActionGateReason::DiscoveryIncomplete: return "discovery_incomplete";
    case ActionGateReason::RegistryOverflow: return "registry_overflow";
    case ActionGateReason::LifecycleThreadMismatch: return "lifecycle_thread_mismatch";
    case ActionGateReason::CandidateRegistryFault: return "candidate_registry_fault";
    case ActionGateReason::UnsupportedPlayerMode: return "unsupported_player_mode";
    case ActionGateReason::SelectionBudgetExceeded: return "selection_budget_exceeded";
    case ActionGateReason::InputInvalid: return "input_invalid";
    case ActionGateReason::WorldMismatch: return "world_mismatch";
    case ActionGateReason::CandidateComponentMismatch: return "candidate_component_mismatch";
    case ActionGateReason::ReceiverPropertyMissing: return "receiver_property_missing";
    case ActionGateReason::ReceiverInvalid: return "receiver_invalid";
    case ActionGateReason::TargetPropertyMissing: return "target_property_missing";
    case ActionGateReason::TargetObjectBusy: return "target_object_busy";
    case ActionGateReason::TargetComponentBusy: return "target_component_busy";
    case ActionGateReason::InvocationLatched: return "invocation_latched";
    case ActionGateReason::GuardedException: return "guarded_exception";
    default: return "unknown";
    }
}

struct GateOutput {
    GateResult result{GateResult::Invalid};
    dsnap::GateObservation observation{};
    UObject* component{};
};

struct PlayerContext {
    UObject* controller{};
    UObject* player{};
    UWorld* world{};
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

class NativeAutoPickup final : public CppUserModBase {
public:
    NativeAutoPickup()
        : callback_gate_(next_generation_.fetch_add(1, std::memory_order_relaxed) + 1),
          configuration_result_(dsnap::load_configuration(mod_directory() / "config.ini")),
          logger_(mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.log",
                  mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.Debug.log"),
          process_shutdown_probe_(resolve_process_shutdown_probe()) {
        ModName = STR("DragonSwordNativeAutoPickup");
        ModVersion = kVersion;
        ModDescription = STR("Evidence-backed native ordinary-drop automatic pickup");
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
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        const bool process_shutdown = process_shutdown_probe_ && process_shutdown_probe_() != FALSE;
        const bool registry_available = !process_shutdown && UnrealInitializer::StaticStorage::bIsInitialized;
        if (registry_available) {
            unregister_global_callbacks();
            unregister_calibration_hooks();
            static_cast<void>(logger_.flush_all());
        } else {
            abandon_calibration_hooks();
            begin_play_callback_id_ = Hook::ERROR_ID;
            end_play_callback_id_ = Hook::ERROR_ID;
            engine_tick_callback_id_ = Hook::ERROR_ID;
            world_reset_callback_id_ = Hook::ERROR_ID;
        }
    }

    [[nodiscard]] bool process_shutdown_in_progress() const noexcept {
        return process_shutdown_probe_ && process_shutdown_probe_() != FALSE;
    }

    [[nodiscard]] bool unreal_registry_available() const noexcept {
        return UnrealInitializer::StaticStorage::bIsInitialized;
    }

    void prepare_for_abandoned_host_unload() noexcept {
        // The pinned UE4SS host may call uninstall_mod and then FreeLibrary even
        // when its Unreal callback registry is unavailable. The DLL is pinned
        // for process lifetime, so invalidate every route into this intentionally
        // leaked instance instead of running registry-dependent teardown.
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        active_.store(false, std::memory_order_release);
        armed_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
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
        pickup_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kPickupFunction);
        if (!drop_item_class_ || !interactable_component_class_ || !player_character_class_ || !player_controller_class_ ||
            !location_function_ || !pickup_function_ || pickup_function_->GetParmsSize() != 0 ||
            !pickup_function_owner_is_valid()) {
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
                if (auto* self = current_instance(generation);
                    self && self->world_reset_callback_is_on_game_thread())
                    self->reset_world("InitGameStatePre");
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});
        begin_play_callback_id_ = Hook::RegisterBeginPlayPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor) {
                if (auto* self = current_instance(generation)) self->actor_begin_play_post(actor);
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("DropItemBeginPlay")});
        end_play_callback_id_ = Hook::RegisterEndPlayPreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor, EEndPlayReason reason) {
                if (auto* self = current_instance(generation)) self->actor_end_play_pre(actor, reason);
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("DropItemEndPlay")});
        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID ||
            begin_play_callback_id_ == Hook::ERROR_ID || end_play_callback_id_ == Hook::ERROR_ID) {
            unregister_global_callbacks();
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required native callback registration failed");
            return;
        }
        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey=F9 mode=active_dropitem_exact_contract pulse=EngineTickPost pulse_ms={} "
                                  "discovery=one_bounded_snapshot_plus_actor_lifecycle generation={}",
                                  kDiagnosticLabel, kPulseInterval.count(), generation));
    }

    void on_update() override {
        // Event-loop callback: scalar key polling and bounded log flushing only.
        static_cast<void>(logger_.flush());
        if (shutting_down_.load(std::memory_order_acquire)) return;
        if (!fingerprint_applied_) {
            // This one-time synchronous hash avoids executable code continuing on
            // a Mod-owned worker after UE4SS releases the DLL at process exit.
            const auto result = dsnap::verify_build_fingerprint(binary_directory());
            fingerprint_applied_ = true;
            build_trusted_.store(result.trusted, std::memory_order_release);
            fingerprint_result_ = result;
            if (!result.trusted) active_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                          std::format("trusted={} game={} ue4ss={} error={}", result.trusted, result.game_sha256,
                                      result.ue4ss_sha256, result.error));
            if (!result.trusted) logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY", "unknown build fingerprint");
            else logger_.write(dsnap::LogAudience::User, "ACTIVE_PICKUP_AVAILABLE",
                                "press F9 to start; only one unambiguous nearby ordinary drop is invoked at a time");
            static_cast<void>(logger_.flush_all());
        }

        // UE4SS native keydown dispatch is not reliable for this game/build even
        // though registration returns normally. Poll only the scalar OS key bit
        // here and preserve the same physical rising-edge latch. UObject work is
        // still deferred to EngineTick through f9_toggle_requested_.
        const bool f9_down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        const auto f9_now = Clock::now();
        if (f9_input_.sample(f9_down, f9_now)) f9_keydown(f9_now);
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
                          std::format("label={} drop_item_captures={} base_class_captures={} derived_class_captures={} "
                                      "discovery_sweeps_started={} discovery_sweeps_completed={} "
                                      "discovery_objects_examined={} discovery_candidates_found={} "
                                      "discovery_total_us={} discovery_max_batch_us={} "
                                      "begin_play_callbacks={} begin_play_candidates={} end_play_callbacks={} "
                                       "engine_tick_callbacks={} candidate_nonempty_pulses={} candidate_batches={} "
                                       "candidates_evaluated={} selection_scans={} selection_total_us={} selection_max_us={} "
                                       "selection_budget_exceeded={} registry_revision={} registry_overflow={} lifecycle_thread_violation={} idle_pulses={} "
                                      "throttle_rejects={} player_chain_attempts={} player_chain_successes={} "
                                      "player_chain_failures={} player_chain_average_us={} player_chain_max_us={} "
                                      "player_chain_reasons={} expected_character_pulses={} alternate_pawn_pulses={} "
                                      "f9_repeat_rejects={} f9_debounce_rejects={} f9_discovery_off_rejects={} "
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
                                      "action_attempts={} actions_invoked={} action_rejections={} actions_confirmed={} action_unconfirmed={} "
                                      "armed={} world_ready={} trusted={} active={} dropped_logs={}",
                                      kDiagnosticLabel, drop_item_captures_.load(), base_class_captures_.load(),
                                      derived_class_captures_.load(), discovery_sweeps_started_.load(),
                                      discovery_sweeps_completed_.load(), discovery_objects_examined_.load(),
                                      discovery_candidates_found_.load(), discovery_total_us_.load(),
                                      discovery_max_batch_us_.load(), actor_begin_play_callbacks_.load(),
                                      actor_begin_play_candidates_.load(), actor_end_play_callbacks_.load(),
                                       engine_tick_callbacks_.load(), candidate_nonempty_pulses_.load(),
                                       candidate_batches_.load(), candidates_evaluated_.load(), selection_scans_.load(),
                                       selection_total_us_.load(), selection_max_us_.load(), selection_budget_exceeded_.load(),
                                       candidate_registry_revision_.load(), registry_overflow_latched_.load(),
                                       lifecycle_thread_violation_.load(), idle_pulses_.load(),
                                      throttle_rejects_.load(), player_chain_attempts, player_chain_successes_.load(),
                                      player_chain_failures_.load(), player_chain_average_us, player_chain_max_us_.load(),
                                      player_chain_reasons, expected_character_pulses_.load(), alternate_pawn_pulses_.load(),
                                      f9_repeat_rejects_.load(), f9_debounce_rejects_.load(),
                                      f9_discovery_off_rejects_.load(),
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
                                       trace_guard_failures_.load(), trace_watch_count(), action_attempts_.load(),
                                       actions_invoked_.load(), action_rejections_.load(), actions_confirmed_.load(),
                                       action_unconfirmed_.load(),
                                       armed_.load(), world_ready_.load(),
                                      build_trusted_.load(), active_.load(), logger_.dropped_messages()));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

private:
    void abandon_calibration_hooks() noexcept {
        const std::scoped_lock lock{calibration_mutex_};
        calibration_hooks_.clear();
    }

    void unregister_global_callbacks() noexcept {
        // Quiesce EngineTick first so it cannot race the remaining hook teardown.
        if (engine_tick_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(engine_tick_callback_id_);
            engine_tick_callback_id_ = Hook::ERROR_ID;
        }
        if (begin_play_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(begin_play_callback_id_);
            begin_play_callback_id_ = Hook::ERROR_ID;
        }
        if (end_play_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(end_play_callback_id_);
            end_play_callback_id_ = Hook::ERROR_ID;
        }
        if (world_reset_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(world_reset_callback_id_);
            world_reset_callback_id_ = Hook::ERROR_ID;
        }
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

    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    bool lifecycle_callback_is_on_game_thread(const char* source) noexcept {
        const auto expected = game_thread_id_.load(std::memory_order_acquire);
        if (expected == 0) return false;
        if (expected == GetCurrentThreadId()) return true;
        latch_lifecycle_thread_violation(source);
        return false;
    }

    bool world_reset_callback_is_on_game_thread() noexcept {
        const auto expected = game_thread_id_.load(std::memory_order_acquire);
        if (expected == 0) return false;
        if (expected == GetCurrentThreadId()) return true;
        latch_lifecycle_thread_violation("InitGameStatePre");
        return false;
    }

    void latch_lifecycle_thread_violation(const char* source) noexcept {
        active_.store(false, std::memory_order_release);
        armed_.store(false, std::memory_order_release);
        {
            const std::scoped_lock lock{state_mutex_};
            runtime_state_.reset_off();
        }
        cleanup_requested_.store(true, std::memory_order_release);
        if (!lifecycle_thread_violation_.exchange(true, std::memory_order_acq_rel)) {
            logger_.write(dsnap::LogAudience::User, "ACTIVE_PICKUP_SUSPENDED",
                          std::format("reason=lifecycle_callback_thread_mismatch source={} state=Off", source));
        }
    }

    void latch_active_pickup_fault(const char* source) noexcept {
        active_.store(false, std::memory_order_release);
        armed_.store(false, std::memory_order_release);
        {
            const std::scoped_lock lock{state_mutex_};
            runtime_state_.reset_off();
        }
        cleanup_requested_.store(true, std::memory_order_release);
        if (!active_pickup_fault_latched_.exchange(true, std::memory_order_acq_rel)) {
            logger_.write(dsnap::LogAudience::User, "ACTIVE_PICKUP_SUSPENDED",
                          std::format("reason=guarded_runtime_fault source={}; state=Off; release and press F9 once to rebuild the registry",
                                      source));
        }
    }

    void actor_begin_play_post_unsafe(AActor* actor) {
        ++actor_begin_play_callbacks_;
        if (!actor) return;
        FWeakObjectPtr weak{};
        bool derived{};
        if (!capture_drop_item_guarded(actor, &weak, &derived)) return;
        const auto current_world_packed = current_world_identity_.load(std::memory_order_acquire);
        if (current_world_packed != 0) {
            auto current_world = unpack_weak_identity(current_world_packed);
            auto* world = current_world.Get();
            if (!world || actor->GetWorld() != world) return;
        }
        register_candidate(weak.ObjectIndex, weak, derived, false);
        ++actor_begin_play_candidates_;
    }

    void actor_begin_play_post(AActor* actor) noexcept {
        if (!armed_.load(std::memory_order_acquire)) return;
        if (!lifecycle_callback_is_on_game_thread("BeginPlay")) return;
#if defined(_MSC_VER)
        __try { actor_begin_play_post_unsafe(actor); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ++seh_rejections_; }
#else
        actor_begin_play_post_unsafe(actor);
#endif
    }

    void actor_end_play_pre_unsafe(AActor* actor, EEndPlayReason reason) {
        ++actor_end_play_callbacks_;
        if (!actor) return;
        FWeakObjectPtr weak{};
        bool derived{};
        if (!capture_drop_item_guarded(actor, &weak, &derived)) return;
        remove_candidate(weak, reason == EEndPlayReason::Destroyed, "actor_end_play_destroyed");
    }

    void actor_end_play_pre(AActor* actor, EEndPlayReason reason) noexcept {
        if (!armed_.load(std::memory_order_acquire)) return;
        if (!lifecycle_callback_is_on_game_thread("EndPlay")) return;
#if defined(_MSC_VER)
        __try { actor_end_play_pre_unsafe(actor, reason); }
        __except (EXCEPTION_EXECUTE_HANDLER) { ++seh_rejections_; }
#else
        actor_end_play_pre_unsafe(actor, reason);
#endif
    }

    void engine_tick_post(UEngine* engine) {
        ++engine_tick_callbacks_;
        const auto thread_id = GetCurrentThreadId();
        DWORD expected{};
        if (!game_thread_id_.compare_exchange_strong(expected, thread_id, std::memory_order_acq_rel) &&
            expected != thread_id) {
            latch_lifecycle_thread_violation("EngineTick");
            return;
        }
        apply_game_thread_control();
        if (!engine || !armed_.load(std::memory_order_acquire) ||
            !build_trusted_.load(std::memory_order_acquire)) return;
        try {
            if (!handle_engine_tick_guarded(this, engine)) {
                ++seh_rejections_;
                latch_active_pickup_fault("engine_tick");
            }
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
        FWeakObjectPtr current_world_weak{context.world};
        const auto world_identity = pack_weak_identity(current_world_weak);
        const auto previous_world_identity = current_world_identity_.load(std::memory_order_acquire);
        if (previous_world_identity != 0 && previous_world_identity != world_identity) {
            reset_world("PlayerWorldIdentityChanged");
            record_player_chain_duration(player_chain_started);
            return;
        }
        current_world_identity_.store(world_identity, std::memory_order_release);
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
        if (runtime_state_value() == dsnap::RuntimeState::ArmedReady) {
            process_discovery_batch();
            bool discovery_complete{};
            {
                const std::scoped_lock lock{state_mutex_};
                discovery_complete = discovery_.complete() &&
                    discovery_ready_logged_.load(std::memory_order_acquire);
            }
            const bool action_ready = discovery_complete &&
                !registry_overflow_latched_.load(std::memory_order_acquire) &&
                !lifecycle_thread_violation_.load(std::memory_order_acquire) &&
                !active_pickup_fault_latched_.load(std::memory_order_acquire) &&
                dsnap::supports_active_pickup(context.mode);
            active_.store(action_ready, std::memory_order_release);
            process_active_pickup(context, now);
        } else {
            ++idle_pulses_;
        }
        record_player_chain_duration(player_chain_started);
    }

    void process_discovery_batch() {
        std::pair<std::int32_t, std::int32_t> range{};
        {
            const std::scoped_lock lock{state_mutex_};
            if (discovery_.epoch() != world_epoch_.load(std::memory_order_acquire)) return;
            if (!discovery_.active()) {
                // BeginPlay callbacks are kept as the primary incremental path.
                // This tail check only covers newly allocated UObject indices
                // on builds where the Actor lifecycle callback is not emitted.
                static_cast<void>(discovery_.extend_upper_bound(UObjectArray::GetNumElements()));
            }
            if (!discovery_.active()) return;
            discovery_ready_logged_.store(false, std::memory_order_release);
            range = discovery_.next(kDiscoveryObjectsPerPulse);
        }
        if (range.second <= range.first) return;
        const auto started = Clock::now();
        std::uint64_t examined{};
        std::int32_t committed = range.first;
        for (auto index = range.first; index < range.second; ++index) {
            committed = index + 1;
            ++examined;
            if (shutting_down_.load(std::memory_order_acquire) || !armed_.load(std::memory_order_acquire)) break;
            auto* item = UObjectArray::IndexToObject(index);
            if (item && item->IsValid(false) && !item->IsUnreachable()) {
                auto* object = item->GetUObject();
                FWeakObjectPtr weak{};
                bool derived{};
                // The 0.6.0 bounded scanner captured a real derived drop by
                // class first. Do not call Actor::GetWorld while walking the
                // global object array; current-World ownership is revalidated
                // from the weak identity before selection and again before the
                // one allowed ProcessEvent call.
                if (object && capture_drop_item_guarded(object, &weak, &derived) && weak.ObjectIndex == index) {
                    register_candidate(index, weak, derived, true);
                }
            }
            if ((examined & 63U) == 0 && Clock::now() - started >= kDiscoveryBatchBudget) break;
        }
        {
            const std::scoped_lock lock{state_mutex_};
            discovery_.commit(committed);
        }
        const auto batch_us = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count());
        discovery_objects_examined_.fetch_add(examined, std::memory_order_relaxed);
        discovery_total_us_.fetch_add(batch_us, std::memory_order_relaxed);
        auto previous = discovery_max_batch_us_.load(std::memory_order_relaxed);
        while (previous < batch_us &&
               !discovery_max_batch_us_.compare_exchange_weak(previous, batch_us, std::memory_order_relaxed)) {}
        bool complete{};
        {
            const std::scoped_lock lock{state_mutex_};
            complete = discovery_.complete();
        }
        if (complete && !discovery_ready_logged_.exchange(true, std::memory_order_acq_rel)) {
            ++discovery_sweeps_completed_;
            const auto current_candidates = candidate_count();
            logger_.write(dsnap::LogAudience::User, "ACTIVE_DISCOVERY_READY",
                          std::format("bounded baseline complete; candidates={} actor lifecycle tracking active",
                                      current_candidates));
            logger_.write(dsnap::LogAudience::Debug, "DISCOVERY_COMPLETE",
                          std::format("epoch={} examined={} candidates={} elapsed_ms={}", world_epoch_.load(),
                                      discovery_objects_examined_.load(), current_candidates,
                                      std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                                           discovery_started_at_).count()));
        }
    }

    void remove_candidate(const FWeakObjectPtr& weak, bool confirm_disappearance, const char* evidence) {
        bool removed{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            const auto found = candidates_.find(weak.ObjectIndex);
            if (found != candidates_.end() &&
                found->second.weak.ObjectSerialNumber == weak.ObjectSerialNumber) {
                candidates_.erase(found);
                removed = true;
            }
        }
        if (!removed) return;
        candidate_registry_revision_.fetch_add(1, std::memory_order_acq_rel);
        const dsnap::WeakObjectId identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        std::optional<dsnap::PendingActionEvidence> confirmed{};
        {
            const std::scoped_lock lock{action_mutex_};
            if (confirm_disappearance) confirmed = pending_action_.confirm_delete(identity);
        }
        if (confirmed) {
            ++actions_confirmed_;
            logger_.write(dsnap::LogAudience::User, "ACTION_CONFIRMED",
                          std::format("object_index={} serial={} evidence={} distance_meters={:.3f} outcome=target_disappeared_gameplay_acceptance_pending",
                                      identity.object_index, identity.serial_number, evidence,
                                      confirmed->distance_meters));
        }
    }

    void register_candidate(std::int32_t index, const FWeakObjectPtr& weak, bool derived, bool discovered) {
        CandidateState candidate{};
        candidate.weak = weak;
        candidate.derived_class = derived;
        candidate.discovered = discovered;
        bool inserted{};
        bool overflow{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            if (candidates_.size() >= configuration_result_.value.max_queue && !candidates_.contains(index)) {
                overflow = true;
            }
            if (overflow) {
                // The registry is now known to be incomplete. Never select from
                // its truncated contents during this activation.
            } else {
            const auto existing = candidates_.find(index);
            if (existing != candidates_.end() &&
                existing->second.weak.ObjectSerialNumber == weak.ObjectSerialNumber) return;
            candidates_[index] = candidate;
            inserted = true;
            }
        }
        if (overflow) {
            ++queue_rejections_;
            active_.store(false, std::memory_order_release);
            if (!registry_overflow_latched_.exchange(true, std::memory_order_acq_rel)) {
                logger_.write(dsnap::LogAudience::User, "ACTIVE_PICKUP_SUSPENDED",
                              std::format("reason=registry_overflow max_queue={} epoch={}; press F9 off/on after leaving the dense area",
                                          configuration_result_.value.max_queue, world_epoch_.load()));
            }
            return;
        }
        if (!inserted) return;
        candidate_registry_revision_.fetch_add(1, std::memory_order_acq_rel);
        ++drop_item_captures_;
        if (derived) ++derived_class_captures_; else ++base_class_captures_;
        if (discovered) ++discovery_candidates_found_;
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
        auto* world = player->GetWorld();
        if (!world) {
            *reason = dsnap::PlayerChainReason::LocationUnavailable;
            return false;
        }
        const auto player_mode = dsnap::classify_controlled_pawn(expected_character, true, true);
        if (!player_mode) {
            *reason = dsnap::PlayerChainReason::PlayerControllerIdentityMismatch;
            return false;
        }
        *output = PlayerContext{controller, player, world, player_location, *player_mode};
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

    void process_active_pickup(const PlayerContext& context, Clock::time_point now) {
        {
            const std::scoped_lock lock{action_mutex_};
            if (const auto expired = pending_action_.expire(now)) {
                ++action_unconfirmed_;
                logger_.write(dsnap::LogAudience::Debug, "ACTION_UNCONFIRMED",
                              std::format("object_index={} serial={} elapsed_ms={} outcome=not_claimed",
                                          expired->candidate.object_index, expired->candidate.serial_number,
                                          dsnap::kActionConfirmationWindow.count()));
            }
            if (pending_action_.pending()) return;
        }
        const auto epoch = world_epoch_.load(std::memory_order_acquire);
        const auto current_world_identity = current_world_identity_.load(std::memory_order_acquire);
        bool discovery_complete{};
        std::uint64_t discovery_epoch{};
        {
            const std::scoped_lock lock{state_mutex_};
            discovery_complete = discovery_.complete() && discovery_ready_logged_.load(std::memory_order_acquire);
            discovery_epoch = discovery_.epoch();
        }
        if (!discovery_complete || discovery_epoch != epoch || current_world_identity == 0) {
            record_action_suppression(ActionGateReason::DiscoveryIncomplete);
            return;
        }
        if (registry_overflow_latched_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::RegistryOverflow);
            return;
        }
        if (lifecycle_thread_violation_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::LifecycleThreadMismatch);
            return;
        }
        if (!dsnap::supports_active_pickup(context.mode)) {
            record_action_suppression(ActionGateReason::UnsupportedPlayerMode);
            return;
        }

        std::vector<CandidateSnapshot> snapshot{};
        std::uint64_t registry_revision{};
        {
            const std::scoped_lock lock{candidates_mutex_};
            registry_revision = candidate_registry_revision_.load(std::memory_order_acquire);
            snapshot.reserve(candidates_.size());
            for (const auto& [index, candidate] : candidates_) {
                snapshot.push_back({index, candidate.weak.ObjectSerialNumber, candidate.weak,
                                    candidate.derived_class});
            }
        }
        if (snapshot.empty()) {
            const std::scoped_lock action_lock{action_mutex_};
            static_cast<void>(action_latch_.observe(std::nullopt));
            return;
        }

        ++candidate_nonempty_pulses_;
        ++candidate_batches_;
        ++selection_scans_;
        const auto selection_started = Clock::now();
        std::size_t eligible_count{};
        CandidateSnapshot selected{};
        std::vector<FWeakObjectPtr> permanent_rejects{};
        bool selection_budget_exceeded{};
        bool selection_faulted{};
        const char* selection_fault_source{"selection_guarded_exception"};
        for (const auto& candidate : snapshot) {
            if (Clock::now() - selection_started > kSelectionBudget) {
                selection_budget_exceeded = true;
                break;
            }
            ++candidates_evaluated_;
            GateOutput gate{};
            UObject* owner{};
            const auto probe = probe_candidate_guarded(this, candidate.weak, context.world,
                                                        &context.location, &owner, &gate);
            if (probe == CandidateProbeStatus::StaleIdentity) {
                permanent_rejects.push_back(candidate.weak);
                selection_faulted = true;
                selection_fault_source = "selection_stale_identity";
                break;
            }
            if (probe == CandidateProbeStatus::DifferentWorld) {
                permanent_rejects.push_back(candidate.weak);
                continue;
            }
            if (probe == CandidateProbeStatus::GuardedException) {
                gate.observation.reason = dsnap::GateReason::GuardedEvaluationException;
                record_gate_observation(candidate.index, gate.observation);
                ++gate_rejections_;
                selection_faulted = true;
                break;
            }
            record_gate_observation(candidate.index, gate.observation);
            if (gate.result != GateResult::Eligible) {
                ++gate_rejections_;
                if (gate.result == GateResult::PermanentReject || gate.result == GateResult::Invalid)
                    permanent_rejects.push_back(candidate.weak);
                continue;
            }
            if (eligible_count == 0) selected = candidate;
            ++eligible_count;
        }

        const auto selection_elapsed = Clock::now() - selection_started;
        const auto selection_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(selection_elapsed).count());
        selection_total_us_.fetch_add(selection_us, std::memory_order_relaxed);
        auto previous_max = selection_max_us_.load(std::memory_order_relaxed);
        while (previous_max < selection_us &&
               !selection_max_us_.compare_exchange_weak(previous_max, selection_us, std::memory_order_relaxed)) {}
        if (selection_budget_exceeded || selection_elapsed > kSelectionBudget) {
            ++selection_budget_exceeded_;
            record_action_suppression(ActionGateReason::SelectionBudgetExceeded);
            return;
        }

        if (selection_faulted) {
            for (const auto& weak : permanent_rejects) remove_candidate(weak, false, "stale_selection_identity");
            record_action_suppression(ActionGateReason::CandidateRegistryFault);
            latch_active_pickup_fault(selection_fault_source);
            return;
        }
        if (!permanent_rejects.empty()) {
            for (const auto& weak : permanent_rejects) remove_candidate(weak, false, "permanent_gate_reject");
            return;
        }
        if (registry_revision != candidate_registry_revision_.load(std::memory_order_acquire) ||
            epoch != world_epoch_.load(std::memory_order_acquire) ||
            current_world_identity != current_world_identity_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::DiscoveryIncomplete);
            return;
        }
        if (registry_overflow_latched_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::RegistryOverflow);
            return;
        }
        if (lifecycle_thread_violation_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::LifecycleThreadMismatch);
            return;
        }

        bool selected_was_previously_attempted{};
        if (eligible_count == 1) {
            const std::scoped_lock lock{action_mutex_};
            selected_was_previously_attempted =
                invoked_candidates_.contains(pack_weak_identity(selected.weak));
        }
        const auto selection_decision =
            dsnap::decide_exact_one_selection(eligible_count, selected_was_previously_attempted);
        if (selection_decision != dsnap::ExactOneSelectionDecision::Ready) {
            const auto previous = diagnostic_eligible_count_.exchange(eligible_count, std::memory_order_acq_rel);
            if (previous != eligible_count && eligible_count > 1) {
                logger_.write(dsnap::LogAudience::Debug, "ACTION_TARGET_AMBIGUOUS",
                              std::format("eligible_candidates={}; invocation suppressed inside {:.1f} meters",
                                          eligible_count, configuration_result_.value.radius_meters));
            }
            const std::scoped_lock lock{action_mutex_};
            static_cast<void>(action_latch_.observe(std::nullopt));
            last_action_gate_reason_.store(
                selection_decision == dsnap::ExactOneSelectionDecision::PreviouslyAttempted
                    ? ActionGateReason::InvocationLatched
                    : ActionGateReason::Ready,
                std::memory_order_release);
            return;
        }
        diagnostic_eligible_count_.store(1, std::memory_order_release);
        const dsnap::WeakObjectId identity{selected.index, selected.serial};
        UObject* owner{};
        GateOutput current_gate{};
        const auto current_probe = probe_candidate_guarded(this, selected.weak, context.world,
                                                            &context.location, &owner, &current_gate);
        if (current_probe != CandidateProbeStatus::Evaluated) {
            record_action_suppression(ActionGateReason::CandidateRegistryFault);
            latch_active_pickup_fault("pre_invoke_revalidation");
            return;
        }
        record_gate_observation(selected.index, current_gate.observation);
        if (current_gate.result != GateResult::Eligible || !current_gate.component) {
            if (current_gate.result == GateResult::PermanentReject || current_gate.result == GateResult::Invalid)
                remove_candidate(selected.weak, false, "pre_invoke_gate_reject");
            return;
        }
        if (registry_revision != candidate_registry_revision_.load(std::memory_order_acquire) ||
            epoch != world_epoch_.load(std::memory_order_acquire) ||
            current_world_identity != current_world_identity_.load(std::memory_order_acquire)) {
            record_action_suppression(ActionGateReason::DiscoveryIncomplete);
            return;
        }
        ++action_attempts_;
        ActionGateReason action_reason{ActionGateReason::InputInvalid};
        const auto invoked = invoke_pickup_guarded(this, context, owner, current_gate.component,
                                                   identity, selected.weak, now,
                                                   current_gate.observation.distance_meters, &action_reason);
        if (!invoked || action_reason != ActionGateReason::Ready) {
            ++action_rejections_;
            const auto previous = last_action_gate_reason_.exchange(action_reason, std::memory_order_acq_rel);
            if (previous != action_reason) {
                logger_.write(dsnap::LogAudience::Debug, "ACTION_REJECTED",
                              std::format("object_index={} serial={} reason={}", selected.index, selected.serial,
                                          action_gate_reason_name(action_reason)));
            }
            if (action_reason == ActionGateReason::GuardedException)
                latch_active_pickup_fault("invocation");
            return;
        }
        last_action_gate_reason_.store(ActionGateReason::Ready, std::memory_order_release);
        ++actions_invoked_;
        logger_.write(dsnap::LogAudience::User, "ACTION_INVOKED_PENDING",
                      std::format("object_index={} serial={} distance_meters={:.3f} contract=player.InteractableComponent.Server_RunInteractV2",
                                  selected.index, selected.serial, current_gate.observation.distance_meters));
    }

    void record_action_suppression(ActionGateReason reason) {
        const auto previous = last_action_gate_reason_.exchange(reason, std::memory_order_acq_rel);
        if (previous != reason) {
            ++action_rejections_;
            logger_.write(dsnap::LogAudience::Debug, "ACTION_REJECTED",
                          std::format("reason={}", action_gate_reason_name(reason)));
        }
    }

    [[nodiscard]] bool pickup_function_owner_is_valid() const noexcept {
        if (!pickup_function_ || pickup_function_->GetParmsSize() != 0) return false;
        auto* outer = pickup_function_->GetOuterPrivate();
        return outer && outer->IsA(UClass::StaticClass()) &&
               static_cast<UClass*>(outer)->IsChildOf(interactable_component_class_);
    }

    bool invoke_pickup_unsafe(const PlayerContext& context, UObject* owner, UObject* candidate_component,
                              dsnap::WeakObjectId identity, const FWeakObjectPtr& selected_weak,
                              Clock::time_point now, double distance_meters, ActionGateReason* reason) {
        if (!reason) return false;
        if (!context.player || !owner || !candidate_component || !pickup_function_) {
            *reason = ActionGateReason::InputInvalid;
            return false;
        }
        auto* world = context.player->GetWorld();
        if (!world || owner->GetWorld() != world || candidate_component->GetWorld() != world) {
            *reason = ActionGateReason::WorldMismatch;
            return false;
        }
        if (!candidate_component_matches(owner, candidate_component)) {
            *reason = ActionGateReason::CandidateComponentMismatch;
            return false;
        }
        auto** receiver_value = context.player->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractableComponent"));
        if (!receiver_value) {
            *reason = ActionGateReason::ReceiverPropertyMissing;
            return false;
        }
        auto* receiver = receiver_value ? *receiver_value : nullptr;
        if (!receiver || !receiver->IsA(interactable_component_class_) || receiver->GetWorld() != world) {
            *reason = ActionGateReason::ReceiverInvalid;
            return false;
        }
        auto** target_object = receiver->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetObject"));
        auto** target_component = receiver->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetComponent"));
        if (!target_object || !target_component) {
            *reason = ActionGateReason::TargetPropertyMissing;
            return false;
        }
        if (*target_object && *target_object != owner) {
            *reason = ActionGateReason::TargetObjectBusy;
            return false;
        }
        if (*target_component && *target_component != candidate_component) {
            *reason = ActionGateReason::TargetComponentBusy;
            return false;
        }

        {
            const std::scoped_lock lock{action_mutex_};
            const auto observation = action_latch_.observe(identity);
            if (observation.decision == dsnap::TargetObservationDecision::PreviousChanged) {
                const auto refreshed = action_latch_.observe(identity);
                if (refreshed.decision != dsnap::TargetObservationDecision::Ready) {
                    *reason = ActionGateReason::InvocationLatched;
                    return false;
                }
            } else if (observation.decision != dsnap::TargetObservationDecision::Ready) {
                *reason = ActionGateReason::InvocationLatched;
                return false;
            }
            if (!action_latch_.mark_invoked(identity)) {
                *reason = ActionGateReason::InvocationLatched;
                return false;
            }
            if (!pending_action_.begin(identity, now, distance_meters)) {
                static_cast<void>(action_latch_.clear(identity));
                *reason = ActionGateReason::InvocationLatched;
                return false;
            }
            invoked_candidates_.insert(pack_weak_identity(selected_weak));
        }

        const auto target_ownership = dsnap::transient_target_ownership(
            *target_object == nullptr, *target_component == nullptr);
        if (!assign_transient_targets_guarded(target_object, target_component, owner, candidate_component,
                                              target_ownership.target_object, target_ownership.target_component)) {
            clear_transient_targets_guarded(target_object, target_component, owner, candidate_component,
                                            target_ownership.target_object, target_ownership.target_component);
            *reason = ActionGateReason::GuardedException;
            return false;
        }
        const auto invoked = process_pickup_event_guarded(receiver, pickup_function_);
        // Never overwrite a target the game changed during ProcessEvent.
        clear_transient_targets_guarded(target_object, target_component, owner, candidate_component,
                                        target_ownership.target_object, target_ownership.target_component);
        *reason = invoked ? ActionGateReason::Ready : ActionGateReason::GuardedException;
        return invoked;
    }

    static void clear_transient_targets_guarded(UObject** target_object, UObject** target_component,
                                                UObject* owner, UObject* candidate_component,
                                                bool owns_target_object, bool owns_target_component) noexcept {
#if defined(_MSC_VER)
        __try {
            if (target_object && dsnap::should_clear_transient_target(owns_target_object, *target_object == owner))
                *target_object = nullptr;
            if (target_component && dsnap::should_clear_transient_target(
                                        owns_target_component, *target_component == candidate_component))
                *target_component = nullptr;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        if (target_object && dsnap::should_clear_transient_target(owns_target_object, *target_object == owner))
            *target_object = nullptr;
        if (target_component && dsnap::should_clear_transient_target(
                                    owns_target_component, *target_component == candidate_component))
            *target_component = nullptr;
#endif
    }

    static bool assign_transient_targets_guarded(UObject** target_object, UObject** target_component,
                                                 UObject* owner, UObject* candidate_component,
                                                 bool owns_target_object, bool owns_target_component) noexcept {
        bool assigned{};
#if defined(_MSC_VER)
        __try {
            if (owns_target_object) *target_object = owner;
            if (owns_target_component) *target_component = candidate_component;
            assigned = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { assigned = false; }
#else
        if (owns_target_object) *target_object = owner;
        if (owns_target_component) *target_component = candidate_component;
        assigned = true;
#endif
        return assigned;
    }

    static bool process_pickup_event_guarded(UObject* receiver, UFunction* function) noexcept {
        bool invoked{};
#if defined(_MSC_VER)
        __try {
            receiver->ProcessEvent(function, nullptr);
            invoked = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { invoked = false; }
#else
        receiver->ProcessEvent(function, nullptr);
        invoked = true;
#endif
        return invoked;
    }

    static bool invoke_pickup_guarded(NativeAutoPickup* self, const PlayerContext& context,
                                      UObject* owner, UObject* component, dsnap::WeakObjectId identity,
                                      const FWeakObjectPtr& selected_weak, Clock::time_point now,
                                      double distance_meters, ActionGateReason* reason) noexcept {
        bool invoked{};
#if defined(_MSC_VER)
        __try {
            invoked = self->invoke_pickup_unsafe(context, owner, component, identity, selected_weak,
                                                 now, distance_meters, reason);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            if (reason) *reason = ActionGateReason::GuardedException;
            invoked = false;
        }
#else
        invoked = self->invoke_pickup_unsafe(context, owner, component, identity, selected_weak,
                                             now, distance_meters, reason);
#endif
        return invoked;
    }

    static CandidateProbeStatus probe_candidate_guarded(NativeAutoPickup* self,
                                                         const FWeakObjectPtr& weak,
                                                         UWorld* expected_world,
                                                         const FVector* player_location,
                                                         UObject** owner_output,
                                                         GateOutput* output) noexcept {
        auto status = CandidateProbeStatus::GuardedException;
#if defined(_MSC_VER)
        __try {
            auto* owner = weak.Get();
            if (!owner) {
                status = CandidateProbeStatus::StaleIdentity;
            } else if (owner->GetWorld() != expected_world) {
                status = CandidateProbeStatus::DifferentWorld;
            } else {
                *output = self->evaluate_candidate_unsafe(owner, *player_location);
                *owner_output = owner;
                status = CandidateProbeStatus::Evaluated;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { status = CandidateProbeStatus::GuardedException; }
#else
        auto* owner = weak.Get();
        if (!owner) {
            status = CandidateProbeStatus::StaleIdentity;
        } else if (owner->GetWorld() != expected_world) {
            status = CandidateProbeStatus::DifferentWorld;
        } else {
            *output = self->evaluate_candidate_unsafe(owner, *player_location);
            *owner_output = owner;
            status = CandidateProbeStatus::Evaluated;
        }
#endif
        return status;
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
            return {GateResult::RetryLater, state_observation, nullptr};
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

    void f9_keydown(Clock::time_point now) noexcept {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        const std::scoped_lock lock{state_mutex_};
        if (!f9_debounce_.accept(now)) {
            ++f9_debounce_rejects_;
            return;
        }
        if (runtime_state_.state() == dsnap::RuntimeState::ArmedReady && !discovery_.complete()) {
            ++f9_discovery_off_rejects_;
            return;
        }
        f9_toggle_requested_.store(true, std::memory_order_release);
    }

    void apply_game_thread_control() {
        if (cleanup_requested_.exchange(false, std::memory_order_acq_rel)) {
            unregister_calibration_hooks(); clear_activation_state();
        }
        if (calibration_timeout_requested_.exchange(false, std::memory_order_acq_rel)) {
            {
                const std::scoped_lock lock{state_mutex_};
                runtime_state_.reset_off();
            }
            armed_.store(false, std::memory_order_release);
            active_.store(false, std::memory_order_release);
            unregister_calibration_hooks();
            clear_activation_state();
            logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                          "state=Off reason=diagnostic_window_complete automatic_actions=0");
        }
        if (!f9_toggle_requested_.exchange(false, std::memory_order_acq_rel)) return;
        {
            const std::scoped_lock lock{state_mutex_};
            const auto next = runtime_state_.press_f9(build_trusted_.load(std::memory_order_acquire), true);
            if (next == dsnap::RuntimeState::Off) {
                armed_.store(false, std::memory_order_release);
                active_.store(false, std::memory_order_release);
            } else if (next == dsnap::RuntimeState::ArmedReady) {
                armed_.store(true, std::memory_order_release);
            }
        }
        const auto state = runtime_state_value();
        if (state == dsnap::RuntimeState::Off || state == dsnap::RuntimeState::DisabledContractInvalid) {
            unregister_calibration_hooks();
            clear_activation_state();
        } else if (state == dsnap::RuntimeState::ArmedReady) {
            unregister_calibration_hooks();
            clear_activation_state();
            active_pickup_fault_latched_.store(false, std::memory_order_release);
            start_discovery();
            active_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::User, "ACTIVE_PICKUP_ARMED",
                          "baseline discovery started; actions remain blocked until ACTIVE_DISCOVERY_READY");
        }
        logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                      std::format("state={} trusted={} contract_valid={} world_ready={} active={}",
                                  state_name(state), build_trusted_.load(), true, world_ready_.load(), active_.load()));
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
        current_world_identity_.store(0, std::memory_order_release);
        player_context_trace_emitted_.store(false, std::memory_order_release);
        trace_call_sequence_.store(0, std::memory_order_release);
        trace_guard_failures_.store(0, std::memory_order_release);
        trace_limit_emitted_.store(false, std::memory_order_release);
        diagnostic_candidate_identity_.store(0, std::memory_order_release);
        diagnostic_component_identity_.store(0, std::memory_order_release);
        diagnostic_eligible_count_.store(0, std::memory_order_release);
        calibration_timeout_requested_.store(false, std::memory_order_release);
        {
            const std::scoped_lock lock{state_mutex_};
            discovery_.cancel();
        }
        discovery_ready_logged_.store(false, std::memory_order_release);
        registry_overflow_latched_.store(false, std::memory_order_release);
        lifecycle_thread_violation_.store(false, std::memory_order_release);
        candidate_registry_revision_.store(0, std::memory_order_release);
        {
            const std::scoped_lock lock{action_mutex_};
            action_latch_.reset();
            pending_action_.reset();
            invoked_candidates_.clear();
        }
        last_action_gate_reason_.store(ActionGateReason::Ready, std::memory_order_release);
    }

    void start_discovery() {
        const auto count = UObjectArray::GetNumElements();
        const std::scoped_lock lock{state_mutex_};
        discovery_.begin(world_epoch_.load(std::memory_order_acquire), count);
        discovery_started_at_ = Clock::now();
        discovery_objects_examined_.store(0, std::memory_order_release);
        discovery_candidates_found_.store(0, std::memory_order_release);
        discovery_total_us_.store(0, std::memory_order_release);
        discovery_max_batch_us_.store(0, std::memory_order_release);
        discovery_ready_logged_.store(false, std::memory_order_release);
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
                                       std::array<dsnap::WeakObjectId, kCalibrationCandidateLimit>* correlated,
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
                                       std::array<dsnap::WeakObjectId, kCalibrationCandidateLimit>* correlated,
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
        std::array<dsnap::WeakObjectId, kCalibrationCandidateLimit> correlated{};
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
        current_world_identity_.store(0, std::memory_order_release);
        player_context_trace_emitted_.store(false, std::memory_order_release);
        trace_call_sequence_.store(0, std::memory_order_release);
        trace_guard_failures_.store(0, std::memory_order_release);
        trace_limit_emitted_.store(false, std::memory_order_release);
        diagnostic_candidate_identity_.store(0, std::memory_order_release);
        diagnostic_component_identity_.store(0, std::memory_order_release);
        diagnostic_eligible_count_.store(0, std::memory_order_release);
        calibration_timeout_requested_.store(false, std::memory_order_release);
        {
            const std::scoped_lock lock{action_mutex_};
            action_latch_.reset();
            pending_action_.reset();
            invoked_candidates_.clear();
        }
        last_action_gate_reason_.store(ActionGateReason::Ready, std::memory_order_release);
        ++world_epoch_;
        {
            const std::scoped_lock lock{state_mutex_};
            discovery_.cancel();
            runtime_state_.reset_off();
        }
        discovery_ready_logged_.store(false, std::memory_order_release);
        registry_overflow_latched_.store(false, std::memory_order_release);
        lifecycle_thread_violation_.store(false, std::memory_order_release);
        active_pickup_fault_latched_.store(false, std::memory_order_release);
        candidate_registry_revision_.store(0, std::memory_order_release);
        armed_.store(false, std::memory_order_release);
        cleanup_requested_.store(true, std::memory_order_release);
        ++world_resets_;
        logger_.write(dsnap::LogAudience::Debug, "WORLD_RESET",
                      std::format("source={} active=0 candidates=0 armed=0 pickup_cancelled=1", source));
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};
    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    dsnap::FingerprintResult fingerprint_result_{};
    ProcessShutdownProbe process_shutdown_probe_{};
        mutable std::mutex candidates_mutex_{};
        std::unordered_map<std::int32_t, CandidateState> candidates_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<bool> armed_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> world_ready_{};
    std::atomic<bool> active_{};
    std::atomic<std::uint64_t> drop_item_captures_{};
    std::atomic<std::uint64_t> base_class_captures_{};
    std::atomic<std::uint64_t> derived_class_captures_{};
    std::atomic<std::uint64_t> queue_rejections_{};
    std::atomic<std::uint64_t> candidate_registry_revision_{};
    std::atomic<bool> registry_overflow_latched_{};
    std::atomic<bool> lifecycle_thread_violation_{};
    std::atomic<bool> active_pickup_fault_latched_{};
    std::atomic<DWORD> game_thread_id_{};
    std::atomic<std::uint64_t> engine_tick_callbacks_{};
    std::atomic<std::uint64_t> candidate_nonempty_pulses_{};
    std::atomic<std::uint64_t> candidate_batches_{};
    std::atomic<std::uint64_t> candidates_evaluated_{};
    std::atomic<std::uint64_t> selection_scans_{};
    std::atomic<std::uint64_t> selection_total_us_{};
    std::atomic<std::uint64_t> selection_max_us_{};
    std::atomic<std::uint64_t> selection_budget_exceeded_{};
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
    std::atomic<bool> discovery_ready_logged_{};
    std::atomic<std::uint64_t> actor_begin_play_callbacks_{};
    std::atomic<std::uint64_t> actor_begin_play_candidates_{};
    std::atomic<std::uint64_t> actor_end_play_callbacks_{};
    std::atomic<std::uint64_t> calibration_ambiguity_rejections_{};
    std::atomic<std::uint64_t> world_resets_{};
    std::atomic<std::uint64_t> action_attempts_{};
    std::atomic<std::uint64_t> actions_invoked_{};
    std::atomic<std::uint64_t> action_rejections_{};
    std::atomic<std::uint64_t> actions_confirmed_{};
    std::atomic<std::uint64_t> action_unconfirmed_{};
    std::atomic<ActionGateReason> last_action_gate_reason_{ActionGateReason::Ready};
    std::array<std::atomic<std::uint64_t>, dsnap::kGateReasonCount> gate_reason_counts_{};
    std::array<std::atomic<std::uint64_t>, dsnap::kPlayerChainReasonCount> player_chain_reason_counts_{};
    mutable std::mutex player_chain_diagnostic_mutex_{};
    dsnap::BoundedPlayerChainDiagnosticState player_chain_diagnostics_{};
    mutable std::mutex state_mutex_{};
    dsnap::CalibrationStateMachine runtime_state_{};
    dsnap::QualifiedReleaseEdge f9_input_{kF9ReleaseQualification};
    dsnap::MonotonicDebounce f9_debounce_{kF9Debounce};
    dsnap::IncrementalDiscoveryState discovery_{};
    mutable std::mutex action_mutex_{};
    dsnap::SingleTargetInvocationLatch action_latch_{};
    dsnap::PendingActionTracker pending_action_{};
    std::unordered_set<std::uint64_t> invoked_candidates_{};
    std::atomic<std::uint64_t> f9_repeat_rejects_{};
    std::atomic<std::uint64_t> f9_debounce_rejects_{};
    std::atomic<std::uint64_t> f9_discovery_off_rejects_{};
    std::atomic<std::uint64_t> world_epoch_{1};
    std::atomic<std::uint64_t> current_pawn_identity_{};
    std::atomic<std::uint64_t> current_controller_identity_{};
    std::atomic<std::uint64_t> current_world_identity_{};
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
    std::atomic<bool> f9_toggle_requested_{};
    std::atomic<bool> cleanup_requested_{};
    std::atomic<bool> calibration_timeout_requested_{};
    UClass* drop_item_class_{};
    UClass* interactable_component_class_{};
    UClass* player_character_class_{};
    UClass* player_controller_class_{};
    UFunction* location_function_{};
    UFunction* pickup_function_{};
    std::atomic<std::uint64_t> diagnostic_candidate_identity_{};
    std::atomic<std::uint64_t> diagnostic_component_identity_{};
    std::atomic<std::size_t> diagnostic_eligible_count_{};
    Clock::time_point next_pulse_due_{};
    Clock::time_point discovery_started_at_{};
    Clock::time_point next_perf_log_{};
    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId begin_play_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId end_play_callback_id_{Hook::ERROR_ID};
    bool fingerprint_applied_{};
};

} // namespace

#define DSNAP_API __declspec(dllexport)
extern "C" {
DSNAP_API RC::CppUserModBase* start_mod() {
    if (!pin_own_module_for_process_lifetime()) return nullptr;
    return new NativeAutoPickup();
}
DSNAP_API void uninstall_mod(RC::CppUserModBase* mod) {
    auto* native_mod = static_cast<NativeAutoPickup*>(mod);
    if (!native_mod) return;
    if (native_mod->process_shutdown_in_progress() || !native_mod->unreal_registry_available()) {
        // CppMod calls FreeLibrary after this returns. The process-lifetime module
        // pin keeps callback code mapped; invalidating the generation and global
        // instance makes any abandoned callbacks permanent no-ops.
        native_mod->prepare_for_abandoned_host_unload();
        return;
    }
    delete native_mod;
}
}
