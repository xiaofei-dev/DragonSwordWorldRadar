// OWNER_AUTHORIZED_WORLD_DROP_RPC_CANARY targeting pinned RE-UE4SS v3.0.1.
//
// Version 1.4 removes the widget-release target-source assumption rejected by
// the 1.3 owner test. One qualified F9 press starts one bounded, chunked scan
// of the current UObject snapshot. Class checks happen before any World access.
// The nearest eligible same-World DropItemActor is revalidated immediately
// before the Mod temporarily supplies the exact target pair observed during a
// normal manual pickup and invokes Server_RunInteractV2 once. There is no idle
// object scan, automatic action loop, synthetic input, native detour, or retry.

#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Mod/CppUserModBase.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#pragma warning(pop)

#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <string>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

extern "C" IMAGE_DOS_HEADER __ImageBase;

constexpr auto kVersion = STR("1.4.0-world-drop-rpc-canary");
constexpr auto kLabel = "OWNER_AUTHORIZED_WORLD_DROP_RPC_CANARY";
constexpr auto kCurrentGameSha256 = "85E0F6BAFF78940C53451A282559A9378F6541E24B1CC81CD8CAF1556204A52E";
constexpr auto kActorClassPath = STR("/Script/Engine.Actor");
constexpr auto kLocationFunctionPath = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kDropItemClassPath = STR("/Script/DS.DropItemActor");
constexpr auto kInteractableClassPath = STR("/Script/DS.DInteractableComponent");
constexpr auto kPickupFunctionPath = STR("/Script/DS.DInteractableComponent:Server_RunInteractV2");
constexpr auto kF9ReleaseQualification = std::chrono::milliseconds{250};
constexpr auto kScanBatchBudget = std::chrono::milliseconds{2};
constexpr auto kScanTimeout = std::chrono::seconds{8};
constexpr std::int32_t kMaxObjectsPerPulse = 32768;
constexpr std::uint8_t kRequiredInteractableValue = 2;

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

std::filesystem::path binary_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path mod_directory() {
    return binary_directory() / "ue4ss" / "Mods" / "DragonSwordNativeAutoPickup";
}

[[nodiscard]] std::uint64_t weak_key(const FWeakObjectPtr& weak) noexcept {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(weak.ObjectIndex)) << 32U) |
           static_cast<std::uint32_t>(weak.ObjectSerialNumber);
}

[[nodiscard]] std::uint64_t object_key(UObject* object) noexcept {
    return object ? weak_key(FWeakObjectPtr{object}) : 0;
}

struct PlayerContext {
    UObject* controller{};
    UObject* pawn{};
    UObject* interaction_receiver{};
    const char* receiver_source{"none"};
};

struct Candidate {
    FWeakObjectPtr object{};
    FWeakObjectPtr component{};
    double distance_meters{std::numeric_limits<double>::infinity()};
};

struct ScanState {
    bool active{};
    std::int32_t next_index{};
    std::int32_t upper_bound{};
    FWeakObjectPtr pawn{};
    std::uint64_t world_key{};
    Candidate best{};
    std::uint64_t objects_examined{};
    std::uint64_t drop_objects{};
    std::uint64_t eligible_objects{};
    Clock::time_point started_at{};
    Clock::time_point next_progress_log{};
};

class NativeAutoPickup final : public CppUserModBase {
public:
    NativeAutoPickup()
        : callback_gate_(next_generation_.fetch_add(1, std::memory_order_relaxed) + 1),
          configuration_result_(dsnap::load_configuration(mod_directory() / "config.ini")),
          logger_(mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.log",
                  mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.Debug.log"),
          shutdown_probe_(resolve_process_shutdown_probe()),
          f9_input_(kF9ReleaseQualification) {
        ModName = STR("DragonSwordNativeAutoPickup");
        ModVersion = kVersion;
        ModDescription = STR("One-shot current-World ordinary-drop interaction canary");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "START",
                      std::format("label={} mode=one_shot_f9 generation={}", kLabel,
                                  callback_gate_.generation()));
        for (const auto& error : configuration_result_.errors) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIG_REJECTED", error);
        }
    }

    ~NativeAutoPickup() override {
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        test_requests_.store(0, std::memory_order_release);
        scan_active_public_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        const bool process_shutdown = shutdown_probe_ && shutdown_probe_() != FALSE;
        const bool registry_available = !process_shutdown && UnrealInitializer::StaticStorage::bIsInitialized;
        if (registry_available) {
            unregister_callbacks();
            static_cast<void>(logger_.flush_all());
        } else {
            abandon_callback_ids();
        }
    }

    [[nodiscard]] bool process_shutdown_in_progress() const noexcept {
        return shutdown_probe_ && shutdown_probe_() != FALSE;
    }

    [[nodiscard]] bool unreal_registry_available() const noexcept {
        return UnrealInitializer::StaticStorage::bIsInitialized;
    }

    void prepare_for_abandoned_host_unload() noexcept {
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        test_requests_.store(0, std::memory_order_release);
        scan_active_public_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        abandon_callback_ids();
    }

    void on_unreal_init() override {
        if (!configuration_result_.valid()) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "invalid configuration");
            return;
        }

        fingerprint_result_ = dsnap::verify_build_fingerprint(binary_directory());
        const bool exact_game = fingerprint_result_.game_sha256 == kCurrentGameSha256;
        build_trusted_.store(fingerprint_result_.trusted && exact_game, std::memory_order_release);
        logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                      std::format("trusted={} exact_current_build={} game={} ue4ss={} error={}",
                                  fingerprint_result_.trusted, exact_game, fingerprint_result_.game_sha256,
                                  fingerprint_result_.ue4ss_sha256, fingerprint_result_.error));
        if (!build_trusted_.load(std::memory_order_acquire)) {
            logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY", "unknown current game or UE4SS fingerprint");
            return;
        }

        actor_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kActorClassPath);
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunctionPath);
        drop_item_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kDropItemClassPath);
        interactable_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInteractableClassPath);
        pickup_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kPickupFunctionPath);
        if (!validate_reflection_contract()) {
            build_trusted_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::User, "DISABLED", "current-build world-drop/RPC contract rejected");
            return;
        }

        const auto generation = callback_gate_.generation();
        engine_tick_callback_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                if (auto* self = current_instance(generation)) self->engine_tick_post(engine);
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldDropCanary")});
        world_reset_callback_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
            }, {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});

        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID) {
            unregister_callbacks();
            build_trusted_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required game-thread callback registration failed");
            return;
        }

        logger_.write(dsnap::LogAudience::User, "READY",
                      "hotkey=F9 action=one_current_world_scan_then_validated_Server_RunInteractV2 "
                      "one_shot=1 automatic_loop=0 transient_target_writes=1 SendInput=0");
        logger_.write(dsnap::LogAudience::User, "CANARY_AVAILABLE",
                      "stand within 4.5 meters of one ordinary ground drop, then press F9 once");
    }

    void on_update() override {
        static_cast<void>(logger_.flush());
        if (shutting_down_.load(std::memory_order_acquire)) return;

        const auto now = Clock::now();
        const bool f9_down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (f9_input_.sample(f9_down, now)) {
            test_requests_.fetch_add(1, std::memory_order_release);
        }

        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                          std::format("requests={} attempts={} scans_started={} scans_completed={} "
                                      "objects_examined={} drop_objects={} eligible_objects={} rpc_invocations={} "
                                      "rejected_context={} rejected_scan={} rejected_target={} faults={} world_resets={} "
                                      "scan_active={}",
                                      test_requests_.load(), attempts_.load(), scans_started_.load(),
                                      scans_completed_.load(), objects_examined_.load(), drop_objects_.load(),
                                      eligible_objects_.load(), rpc_invocations_.load(), rejected_context_.load(),
                                      rejected_scan_.load(), rejected_target_.load(), faults_.load(),
                                      world_resets_.load(), scan_active_public_.load(std::memory_order_acquire)));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

private:
    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] bool validate_reflection_contract() const noexcept {
        if (!actor_class_ || !location_function_ || !drop_item_class_ || !interactable_class_ ||
            !pickup_function_) return false;
        auto* location_owner = location_function_->GetOuterPrivate();
        auto* pickup_owner = pickup_function_->GetOuterPrivate();
        return location_owner && location_owner->IsA(UClass::StaticClass()) &&
               static_cast<UClass*>(location_owner)->IsChildOf(actor_class_) &&
               location_function_->GetParmsSize() == sizeof(FVector) &&
               pickup_owner && pickup_owner->IsA(UClass::StaticClass()) &&
               static_cast<UClass*>(pickup_owner)->IsChildOf(interactable_class_) &&
               pickup_function_->GetParmsSize() == 0;
    }

    [[nodiscard]] bool game_thread() noexcept {
        const DWORD current = GetCurrentThreadId();
        if (game_thread_id_ == 0) game_thread_id_ = current;
        return game_thread_id_ == current;
    }

    [[nodiscard]] static UObject* named_object(UObject* owner, const wchar_t* name) {
        if (!owner) return nullptr;
        auto** value = owner->GetValuePtrByPropertyNameInChain<UObject*>(name);
        return value ? *value : nullptr;
    }

    [[nodiscard]] bool resolve_player_context(UEngine* engine, PlayerContext* output) {
        if (!engine || !output) return false;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        auto* game_instance = named_object(viewport, STR("GameInstance"));
        auto* players = game_instance
            ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers")) : nullptr;
        if (!players || !players->IsValidIndex(0) || !players->GetData()) return false;
        auto* local_player = static_cast<UObject* const*>(players->GetData())[0];
        auto* controller = named_object(local_player, STR("PlayerController"));
        auto* pawn = named_object(controller, STR("Pawn"));
        if (!controller || !pawn || !pawn->GetWorld()) return false;

        UObject* receiver = named_object(pawn, STR("InteractableComponent"));
        const char* receiver_source = "pawn.InteractableComponent";
        if (!receiver) {
            receiver = named_object(pawn, STR("InteractionComponent"));
            receiver_source = "pawn.InteractionComponent";
        }
        if (!receiver) {
            receiver = named_object(controller, STR("InteractableComponent"));
            receiver_source = "controller.InteractableComponent";
        }
        if (!receiver) {
            receiver = named_object(controller, STR("InteractionComponent"));
            receiver_source = "controller.InteractionComponent";
        }
        *output = {controller, pawn, receiver, receiver ? receiver_source : "none"};
        return true;
    }

    [[nodiscard]] static std::string object_full_name(UObject* object) {
        return object ? to_string(object->GetFullName()) : "null";
    }

    [[nodiscard]] static std::string object_class_name(UObject* object) {
        auto* object_class = object ? object->GetClassPrivate() : nullptr;
        return object_class ? to_string(object_class->GetPathName()) : "null";
    }

    [[nodiscard]] bool read_location_unsafe(UObject* actor, FVector* result) {
        if (!actor || !result || !location_function_) return false;
        struct LocationParameters { FVector return_value{}; } parameters{};
        actor->ProcessEvent(location_function_, &parameters);
        *result = parameters.return_value;
        return std::isfinite(result->X()) && std::isfinite(result->Y()) && std::isfinite(result->Z());
    }

    [[nodiscard]] static bool read_location_guarded(NativeAutoPickup* self,
                                                    UObject* actor,
                                                    FVector* result) noexcept {
#if defined(_MSC_VER)
        __try { return self->read_location_unsafe(actor, result); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#else
        return self->read_location_unsafe(actor, result);
#endif
    }

    [[nodiscard]] bool validate_receiver(const PlayerContext& context, const char** reason) const {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!context.pawn || !context.pawn->GetWorld()) return reject("pawn_world_unavailable");
        if (!context.interaction_receiver || !context.interaction_receiver->IsA(interactable_class_)) {
            return reject("receiver_not_interactable_component");
        }
        if (context.interaction_receiver->GetWorld() != context.pawn->GetWorld()) {
            return reject("receiver_world_mismatch");
        }
        if (reason) *reason = "validated";
        return true;
    }

    [[nodiscard]] bool probe_candidate_unsafe(UObject* object,
                                              const PlayerContext& context,
                                              const FVector& player_location,
                                              Candidate* candidate,
                                              bool* is_drop) {
        if (is_drop) *is_drop = false;
        if (!object || !object->IsA(drop_item_class_)) return false;
        if (is_drop) *is_drop = true;
        if (object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject) ||
            object->HasAnyFlags(EObjectFlags::RF_ArchetypeObject) ||
            object->HasAnyFlags(EObjectFlags::RF_DefaultSubObject)) return false;

        const FWeakObjectPtr object_weak{object};
        if (object_weak.ObjectIndex < 0 || object_weak.ObjectSerialNumber <= 0 || object_weak.Get() != object) {
            return false;
        }
        auto* world = context.pawn ? context.pawn->GetWorld() : nullptr;
        if (!world || object->GetWorld() != world) return false;

        auto* component = named_object(object, STR("InteractComponent"));
        if (!component || !component->IsA(interactable_class_) || component->GetWorld() != world ||
            component->GetOuterPrivate() != object) return false;
        const FWeakObjectPtr component_weak{component};
        if (component_weak.ObjectIndex < 0 || component_weak.ObjectSerialNumber <= 0 ||
            component_weak.Get() != component) return false;

        auto* interactable = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"));
        auto* interact_type = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"));
        if (!interactable || !interact_type || *interactable != kRequiredInteractableValue ||
            *interact_type != dsnap::kDropItemInteractType) return false;

        FVector object_location{};
        if (!read_location_unsafe(object, &object_location)) return false;
        const auto dx = object_location.X() - player_location.X();
        const auto dy = object_location.Y() - player_location.Y();
        const auto dz = object_location.Z() - player_location.Z();
        const auto distance_squared = dx * dx + dy * dy + dz * dz;
        if (!std::isfinite(distance_squared)) return false;
        const auto radius_world = configuration_result_.value.radius_meters * 100.0;
        if (distance_squared > radius_world * radius_world) return false;

        if (candidate) {
            candidate->object = object_weak;
            candidate->component = component_weak;
            candidate->distance_meters = std::sqrt(distance_squared) / 100.0;
        }
        return true;
    }

    [[nodiscard]] static bool probe_candidate_guarded(NativeAutoPickup* self,
                                                       UObject* object,
                                                       const PlayerContext* context,
                                                       const FVector* player_location,
                                                       Candidate* candidate,
                                                       bool* is_drop) noexcept {
#if defined(_MSC_VER)
        __try { return self->probe_candidate_unsafe(object, *context, *player_location, candidate, is_drop); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#else
        return self->probe_candidate_unsafe(object, *context, *player_location, candidate, is_drop);
#endif
    }

    [[nodiscard]] bool game_window_is_foreground() const noexcept {
        const auto foreground = GetForegroundWindow();
        if (!foreground) return false;
        DWORD process_id{};
        static_cast<void>(GetWindowThreadProcessId(foreground, &process_id));
        return process_id == GetCurrentProcessId();
    }

    void start_scan_unsafe(UEngine* engine) {
        ++attempts_;
        if (!build_trusted_.load(std::memory_order_acquire) || !configuration_result_.value.automatic_pickup) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=build_or_config_not_authorized");
            return;
        }
        if (!game_window_is_foreground()) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=game_not_foreground");
            return;
        }
        if (scan_.active) {
            ++rejected_scan_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=scan_already_in_progress");
            return;
        }

        PlayerContext context{};
        const char* receiver_reason{"unknown"};
        FVector player_location{};
        if (!resolve_player_context(engine, &context)) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=player_context_unavailable");
            return;
        }
        if (!validate_receiver(context, &receiver_reason)) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason={} receiver_source={}", receiver_reason, context.receiver_source));
            return;
        }
        if (!read_location_guarded(this, context.pawn, &player_location)) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=player_location_unavailable");
            return;
        }

        const auto upper_bound = UObjectArray::GetNumElements();
        if (upper_bound <= 0) {
            ++rejected_scan_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=uobject_snapshot_empty");
            return;
        }

        scan_ = {};
        scan_.active = true;
        scan_.upper_bound = upper_bound;
        scan_.pawn = FWeakObjectPtr{context.pawn};
        scan_.world_key = object_key(reinterpret_cast<UObject*>(context.pawn->GetWorld()));
        scan_.started_at = Clock::now();
        scan_.next_progress_log = scan_.started_at + std::chrono::milliseconds{500};
        scan_active_public_.store(true, std::memory_order_release);
        ++scans_started_;
        logger_.write(dsnap::LogAudience::User, "SCAN_STARTED",
                      std::format("snapshot_objects={} radius_meters={:.2f} receiver=0x{:X} receiver_source={}",
                                  upper_bound, configuration_result_.value.radius_meters,
                                  object_key(context.interaction_receiver), context.receiver_source));
    }

    [[nodiscard]] static bool start_scan_guarded(NativeAutoPickup* self, UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try { self->start_scan_unsafe(engine); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#else
        self->start_scan_unsafe(engine);
        return true;
#endif
    }

    [[nodiscard]] bool revalidate_candidate(const PlayerContext& context,
                                            const Candidate& candidate,
                                            UObject** object_output,
                                            UObject** component_output,
                                            double* distance_output,
                                            const char** reason) {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        auto* object = candidate.object.Get();
        auto* component = candidate.component.Get();
        if (!object || !component) return reject("candidate_weak_identity_stale");
        Candidate current{};
        FVector player_location{};
        if (!read_location_unsafe(context.pawn, &player_location)) return reject("player_location_unavailable");
        bool is_drop{};
        if (!probe_candidate_unsafe(object, context, player_location, &current, &is_drop)) {
            return reject(is_drop ? "candidate_no_longer_eligible" : "candidate_class_changed");
        }
        if (current.component.Get() != component) return reject("candidate_component_changed");
        if (object_output) *object_output = object;
        if (component_output) *component_output = component;
        if (distance_output) *distance_output = current.distance_meters;
        if (reason) *reason = "validated";
        return true;
    }

    void invoke_selected_unsafe(UEngine* engine) {
        PlayerContext context{};
        const char* receiver_reason{"unknown"};
        if (!resolve_player_context(engine, &context) || !validate_receiver(context, &receiver_reason)) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason={} phase=pre_rpc", receiver_reason));
            return;
        }
        if (weak_key(FWeakObjectPtr{context.pawn}) != weak_key(scan_.pawn) ||
            object_key(reinterpret_cast<UObject*>(context.pawn->GetWorld())) != scan_.world_key) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=world_or_pawn_changed phase=pre_rpc");
            return;
        }

        UObject* object{};
        UObject* component{};
        double distance_meters{};
        const char* target_reason{"unknown"};
        if (!revalidate_candidate(context, scan_.best, &object, &component, &distance_meters, &target_reason)) {
            ++rejected_target_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason={} phase=target_revalidation", target_reason));
            return;
        }

        auto** target_object = context.interaction_receiver
            ->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetObject"));
        auto** target_component = context.interaction_receiver
            ->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetComponent"));
        if (!target_object || !target_component) {
            ++rejected_target_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=receiver_target_properties_missing");
            return;
        }
        if ((*target_object && *target_object != object) ||
            (*target_component && *target_component != component)) {
            ++rejected_target_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason=receiver_target_busy existing_object=0x{:X} existing_component=0x{:X}",
                                      object_key(*target_object), object_key(*target_component)));
            return;
        }

        const auto ownership = dsnap::transient_target_ownership(*target_object == nullptr,
                                                                 *target_component == nullptr);
        if (!assign_transient_targets_guarded(target_object, target_component, object, component,
                                              ownership.target_object, ownership.target_component)) {
            bool ignored_object_cleared{};
            bool ignored_component_cleared{};
            static_cast<void>(clear_transient_targets_guarded(
                target_object, target_component, object, component, ownership.target_object,
                ownership.target_component, &ignored_object_cleared, &ignored_component_cleared));
            ++faults_;
            logger_.write(dsnap::LogAudience::User, "CANARY_FAULT",
                          "structured exception while assigning transient targets; RPC not invoked");
            return;
        }

        logger_.write(dsnap::LogAudience::User, "TARGET_VALIDATED",
                      std::format("source=current_world_scan target_object=0x{:X} target_object_name={} "
                                  "target_component=0x{:X} target_component_name={} receiver=0x{:X} "
                                  "distance_meters={:.3f} transient_object={} transient_component={}",
                                  object_key(object), object_full_name(object), object_key(component),
                                  object_full_name(component), object_key(context.interaction_receiver),
                                  distance_meters, ownership.target_object, ownership.target_component));

        const bool invoked = process_pickup_event_guarded(context.interaction_receiver, pickup_function_);
        bool target_object_cleared{};
        bool target_component_cleared{};
        const bool cleanup_ok = clear_transient_targets_guarded(
            target_object, target_component, object, component, ownership.target_object,
            ownership.target_component, &target_object_cleared, &target_component_cleared);
        if (!invoked || !cleanup_ok) {
            ++faults_;
            logger_.write(dsnap::LogAudience::User, "CANARY_FAULT",
                          std::format("rpc_returned={} cleanup_returned={} no_retry=1", invoked, cleanup_ok));
            return;
        }
        ++rpc_invocations_;

        logger_.write(dsnap::LogAudience::User, "ACTION_RPC_INVOKED",
                      std::format("receiver=0x{:X} function=Server_RunInteractV2 target_object=0x{:X} "
                                  "target_component=0x{:X} target_object_cleared={} target_component_cleared={} "
                                  "visible_collection=pending_owner_confirmation",
                                  object_key(context.interaction_receiver), object_key(object), object_key(component),
                                  target_object_cleared, target_component_cleared));
    }

    [[nodiscard]] static bool assign_transient_targets_guarded(UObject** target_object,
                                                                UObject** target_component,
                                                                UObject* object,
                                                                UObject* component,
                                                                bool assign_object,
                                                                bool assign_component) noexcept {
#if defined(_MSC_VER)
        __try {
#endif
            if (!target_object || !target_component || !object || !component) return false;
            if (assign_object) *target_object = object;
            if (assign_component) *target_component = component;
            return (!assign_object || *target_object == object) &&
                   (!assign_component || *target_component == component);
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    }

    [[nodiscard]] static bool process_pickup_event_guarded(UObject* receiver,
                                                            UFunction* function) noexcept {
#if defined(_MSC_VER)
        __try {
#endif
            if (!receiver || !function) return false;
            receiver->ProcessEvent(function, nullptr);
            return true;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    }

    [[nodiscard]] static bool clear_transient_targets_guarded(UObject** target_object,
                                                               UObject** target_component,
                                                               UObject* object,
                                                               UObject* component,
                                                               bool owns_object,
                                                               bool owns_component,
                                                               bool* object_cleared,
                                                               bool* component_cleared) noexcept {
#if defined(_MSC_VER)
        __try {
#endif
            if (object_cleared) *object_cleared = false;
            if (component_cleared) *component_cleared = false;
            if (!target_object || !target_component) return false;
            if (dsnap::should_clear_transient_target(owns_object, *target_object == object)) {
                *target_object = nullptr;
                if (object_cleared) *object_cleared = true;
            }
            if (dsnap::should_clear_transient_target(owns_component, *target_component == component)) {
                *target_component = nullptr;
                if (component_cleared) *component_cleared = true;
            }
            return true;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
    }

    void process_scan_batch_unsafe(UEngine* engine) {
        if (!scan_.active) return;
        const auto now = Clock::now();
        if (now - scan_.started_at > kScanTimeout) {
            ++rejected_scan_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason=scan_timeout examined={} upper_bound={} drop_objects={} eligible={}",
                                      scan_.objects_examined, scan_.upper_bound, scan_.drop_objects,
                                      scan_.eligible_objects));
            cancel_scan();
            return;
        }

        PlayerContext context{};
        FVector player_location{};
        if (!resolve_player_context(engine, &context) || !context.pawn ||
            weak_key(FWeakObjectPtr{context.pawn}) != weak_key(scan_.pawn) ||
            object_key(reinterpret_cast<UObject*>(context.pawn->GetWorld())) != scan_.world_key ||
            !read_location_guarded(this, context.pawn, &player_location)) {
            ++rejected_context_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED", "reason=world_or_pawn_changed phase=scan");
            cancel_scan();
            return;
        }

        const auto batch_started = Clock::now();
        std::int32_t examined_this_pulse{};
        while (scan_.next_index < scan_.upper_bound && examined_this_pulse < kMaxObjectsPerPulse) {
            const auto index = scan_.next_index++;
            ++examined_this_pulse;
            ++scan_.objects_examined;
            auto* item = UObjectArray::IndexToObject(index);
            if (item && item->IsValid(false) && !item->IsUnreachable()) {
                auto* object = item->GetUObject();
                Candidate candidate{};
                bool is_drop{};
                if (object && probe_candidate_guarded(this, object, &context, &player_location,
                                                      &candidate, &is_drop)) {
                    ++scan_.eligible_objects;
                    if (candidate.distance_meters < scan_.best.distance_meters) scan_.best = candidate;
                }
                if (is_drop) ++scan_.drop_objects;
            }
            if ((examined_this_pulse & 127) == 0 && Clock::now() - batch_started >= kScanBatchBudget) break;
        }

        if (now >= scan_.next_progress_log && scan_.next_index < scan_.upper_bound) {
            logger_.write(dsnap::LogAudience::Debug, "SCAN_PROGRESS",
                          std::format("next_index={} upper_bound={} examined={} drop_objects={} eligible={}",
                                      scan_.next_index, scan_.upper_bound, scan_.objects_examined,
                                      scan_.drop_objects, scan_.eligible_objects));
            scan_.next_progress_log = now + std::chrono::milliseconds{500};
        }

        if (scan_.next_index < scan_.upper_bound) return;

        ++scans_completed_;
        objects_examined_.fetch_add(scan_.objects_examined, std::memory_order_relaxed);
        drop_objects_.fetch_add(scan_.drop_objects, std::memory_order_relaxed);
        eligible_objects_.fetch_add(scan_.eligible_objects, std::memory_order_relaxed);
        logger_.write(dsnap::LogAudience::User, "SCAN_COMPLETED",
                      std::format("examined={} drop_objects={} eligible={} elapsed_ms={}",
                                  scan_.objects_examined, scan_.drop_objects, scan_.eligible_objects,
                                  std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                                       scan_.started_at).count()));
        if (scan_.eligible_objects == 0 || !scan_.best.object.Get() || !scan_.best.component.Get()) {
            ++rejected_target_;
            logger_.write(dsnap::LogAudience::User, "CANARY_REJECTED",
                          std::format("reason=no_eligible_drop radius_meters={:.2f} drop_objects={}",
                                      configuration_result_.value.radius_meters, scan_.drop_objects));
            cancel_scan();
            return;
        }

        invoke_selected_unsafe(engine);
        cancel_scan();
    }

    [[nodiscard]] static bool process_scan_batch_guarded(NativeAutoPickup* self, UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try { self->process_scan_batch_unsafe(engine); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#else
        self->process_scan_batch_unsafe(engine);
        return true;
#endif
    }

    void cancel_scan() noexcept {
        scan_ = {};
        scan_active_public_.store(false, std::memory_order_release);
    }

    void engine_tick_post(UEngine* engine) noexcept {
        if (!game_thread()) return;
        if (test_requests_.load(std::memory_order_acquire) != 0 &&
            test_requests_.fetch_sub(1, std::memory_order_acq_rel) != 0) {
            if (!start_scan_guarded(this, engine)) {
                ++faults_;
                logger_.write(dsnap::LogAudience::User, "CANARY_FAULT",
                              "structured exception while starting scan; request discarded");
                cancel_scan();
            }
        }
        if (scan_.active && !process_scan_batch_guarded(this, engine)) {
            ++faults_;
            logger_.write(dsnap::LogAudience::User, "CANARY_FAULT",
                          "structured exception during scan/action; scan cancelled and no retry scheduled");
            cancel_scan();
        }
    }

    void reset_world(const char* source) noexcept {
        test_requests_.store(0, std::memory_order_release);
        const bool scan_was_active = scan_.active;
        cancel_scan();
        ++world_resets_;
        logger_.write(dsnap::LogAudience::User, "WORLD_RESET",
                      std::format("source={} pending_request=cleared scan_cancelled={}", source,
                                  scan_was_active));
    }

    void unregister_callbacks() noexcept {
        for (auto* id : {&engine_tick_callback_id_, &world_reset_callback_id_}) {
            if (*id != Hook::ERROR_ID) {
                Hook::UnregisterCallback(*id);
                *id = Hook::ERROR_ID;
            }
        }
    }

    void abandon_callback_ids() noexcept {
        engine_tick_callback_id_ = Hook::ERROR_ID;
        world_reset_callback_id_ = Hook::ERROR_ID;
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};

    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    dsnap::FingerprintResult fingerprint_result_{};
    ProcessShutdownProbe shutdown_probe_{};
    dsnap::QualifiedReleaseEdge f9_input_;

    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};

    UClass* actor_class_{};
    UClass* drop_item_class_{};
    UClass* interactable_class_{};
    UFunction* location_function_{};
    UFunction* pickup_function_{};
    DWORD game_thread_id_{};
    Clock::time_point next_perf_log_{};
    ScanState scan_{};

    std::atomic<bool> shutting_down_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> scan_active_public_{};
    std::atomic<std::uint64_t> test_requests_{};
    std::atomic<std::uint64_t> attempts_{};
    std::atomic<std::uint64_t> scans_started_{};
    std::atomic<std::uint64_t> scans_completed_{};
    std::atomic<std::uint64_t> objects_examined_{};
    std::atomic<std::uint64_t> drop_objects_{};
    std::atomic<std::uint64_t> eligible_objects_{};
    std::atomic<std::uint64_t> rpc_invocations_{};
    std::atomic<std::uint64_t> rejected_context_{};
    std::atomic<std::uint64_t> rejected_scan_{};
    std::atomic<std::uint64_t> rejected_target_{};
    std::atomic<std::uint64_t> faults_{};
    std::atomic<std::uint64_t> world_resets_{};
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
        native_mod->prepare_for_abandoned_host_unload();
        return;
    }
    delete native_mod;
}
}
