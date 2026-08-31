// OWNER_AUTHORIZED_PROMPT_TARGET_AUTO_PICKUP targeting pinned RE-UE4SS v3.0.1.
//
// Version 1.5 follows the game's own ground-loot UI target path. The
// game tells UDDropItemButtonUserWidget which UDInteractableComponent is active;
// this Mod retains only the target's weak identity. EngineTick revalidates the
// target component, Actor Outer, current World, interaction state, and current
// player receiver before invoking the manually observed zero-parameter
// Server_RunInteractV2 contract. F9 toggles the event-driven path. There is no
// UObject scan, synthetic input, native detour, or background worker.

#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
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
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#pragma warning(pop)

#include <atomic>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <mutex>
#include <string>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

extern "C" IMAGE_DOS_HEADER __ImageBase;

constexpr auto kVersion = STR("1.5.0-prompt-target-auto-pickup");
constexpr auto kLabel = "OWNER_AUTHORIZED_PROMPT_TARGET_AUTO_PICKUP";
constexpr auto kCurrentGameSha256 = "85E0F6BAFF78940C53451A282559A9378F6541E24B1CC81CD8CAF1556204A52E";
constexpr auto kActorClassPath = STR("/Script/Engine.Actor");
constexpr auto kWidgetClassPath = STR("/Script/DSClient.DDropItemButtonUserWidget");
constexpr auto kInteractableClassPath = STR("/Script/DS.DInteractableComponent");
constexpr auto kVisibilityFunctionPath =
    STR("/Script/DSClient.DDropItemButtonUserWidget:UpdateButtonVisibilityByComponent");
constexpr auto kPickupFunctionPath = STR("/Script/DS.DInteractableComponent:Server_RunInteractV2");
constexpr auto kPulseInterval = std::chrono::milliseconds{50};
constexpr auto kF9Debounce = std::chrono::milliseconds{500};
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

[[nodiscard]] std::uint64_t pack_weak_identity(const FWeakObjectPtr& weak) noexcept {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(weak.ObjectIndex)) << 32U) |
           static_cast<std::uint32_t>(weak.ObjectSerialNumber);
}

class NativeAutoPickup final : public CppUserModBase {
public:
    NativeAutoPickup()
        : callback_gate_(next_generation_.fetch_add(1, std::memory_order_relaxed) + 1),
          configuration_result_(dsnap::load_configuration(mod_directory() / "config.ini")),
          logger_(mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.log",
                  mod_directory() / "runtime" / "logs" / "DragonSwordNativeAutoPickup.Debug.log"),
          shutdown_probe_(resolve_process_shutdown_probe()) {
        ModName = STR("DragonSwordNativeAutoPickup");
        ModVersion = kVersion;
        ModDescription = STR("Prompt-target native ordinary ground-loot auto pickup");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "START",
                      std::format("label={} active=0 generation={}", kLabel, callback_gate_.generation()));
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
        const bool process_shutdown = shutdown_probe_ && shutdown_probe_() != FALSE;
        const bool registry_available = !process_shutdown && UnrealInitializer::StaticStorage::bIsInitialized;
        if (registry_available) {
            unregister_callbacks();
            static_cast<void>(logger_.flush_all());
        } else {
            visibility_hook_ = {};
            engine_tick_callback_id_ = Hook::ERROR_ID;
            world_reset_callback_id_ = Hook::ERROR_ID;
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
        active_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
    }

    void on_unreal_init() override {
        if (!configuration_result_.valid()) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "invalid configuration");
            return;
        }

        actor_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kActorClassPath);
        widget_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kWidgetClassPath);
        interactable_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInteractableClassPath);
        visibility_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kVisibilityFunctionPath);
        pickup_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kPickupFunctionPath);
        if (!validate_reflection_contract()) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "prompt-target/RPC reflection contract is missing or changed");
            return;
        }

        const auto generation = callback_gate_.generation();
        try {
            visibility_hook_ = UObjectGlobals::RegisterHook(
                visibility_function_,
                [generation](UnrealScriptFunctionCallableContext& context, void*) {
                    if (auto* self = current_instance(generation)) self->visibility_pre(context);
                },
                 [](UnrealScriptFunctionCallableContext&, void*) {}, nullptr);
        } catch (...) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "pickup-button visibility hook registration failed");
            return;
        }
        if (visibility_hook_.first == Hook::ERROR_ID || visibility_hook_.second == Hook::ERROR_ID) {
            visibility_hook_ = {};
            logger_.write(dsnap::LogAudience::User, "DISABLED", "pickup-button visibility hook returned an error id");
            return;
        }

        engine_tick_callback_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                if (auto* self = current_instance(generation)) self->engine_tick_post(engine);
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("PromptTargetPickup")});
        world_reset_callback_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});

        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID) {
            unregister_callbacks();
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required native callback registration failed");
            return;
        }

        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey=F9 mode=toggle source=game_pickup_prompt_component "
                                  "action=validated_Server_RunInteractV2 pulse_ms={} object_scans=0 SendInput=0",
                                  kLabel, kPulseInterval.count()));
        logger_.write(dsnap::LogAudience::User, "AUTO_PICKUP_AVAILABLE", "press F9 once to enable");
    }

    void on_update() override {
        static_cast<void>(logger_.flush());
        if (shutting_down_.load(std::memory_order_acquire)) return;

        if (!fingerprint_applied_) {
            fingerprint_result_ = dsnap::verify_build_fingerprint(binary_directory());
            fingerprint_applied_ = true;
            const bool exact_game = fingerprint_result_.game_sha256 == kCurrentGameSha256;
            build_trusted_.store(fingerprint_result_.trusted && exact_game, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                          std::format("trusted={} exact_current_build={} game={} ue4ss={} error={}",
                                      fingerprint_result_.trusted, exact_game, fingerprint_result_.game_sha256,
                                      fingerprint_result_.ue4ss_sha256, fingerprint_result_.error));
            logger_.write(dsnap::LogAudience::User,
                          build_trusted_.load(std::memory_order_acquire) ? "AUTO_PICKUP_AVAILABLE" : "PASSIVE_ONLY",
                          build_trusted_.load(std::memory_order_acquire) ? "press F9 to start prompt-target pickup"
                                                                        : "unknown current game or UE4SS fingerprint");
            if (build_trusted_.load(std::memory_order_acquire) && configuration_result_.value.automatic_pickup &&
                configuration_result_.value.enabled_on_launch) {
                toggle_requested_.store(true, std::memory_order_release);
            }
            static_cast<void>(logger_.flush_all());
        }

        const bool down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (down && !f9_was_down_) {
            const auto now = Clock::now();
            if (last_f9_edge_ == Clock::time_point{} || now - last_f9_edge_ >= kF9Debounce) {
                toggle_requested_.store(true, std::memory_order_release);
                last_f9_edge_ = now;
            }
        }
        f9_was_down_ = down;

        const auto now = Clock::now();
        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                          std::format("visibility_events={} visible_events={} hidden_events={} "
                                      "action_attempts={} rpc_invocations={} action_failures={} "
                                      "world_resets={} active={} target_visible={} pending={} object_scans=0",
                                      visibility_events_.load(), visible_events_.load(), hidden_events_.load(),
                                      action_attempts_.load(), rpc_invocations_.load(), action_failures_.load(),
                                      world_resets_.load(), active_.load(), target_visible_.load(),
                                      pickup_pending_.load()));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

private:
    struct VisibilityParameters {
        UObject* component{};
        bool active{};
        bool valid{};
    };

    struct PlayerContext {
        UObject* controller{};
        UObject* pawn{};
        UObject* interaction_receiver{};
        const char* receiver_source{"none"};
    };

    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] bool function_owner_is(UFunction* function, UClass* expected) const noexcept {
        if (!function || !expected) return false;
        auto* outer = function->GetOuterPrivate();
        return outer && outer->IsA(UClass::StaticClass()) &&
               static_cast<UClass*>(outer)->IsChildOf(expected);
    }

    [[nodiscard]] bool validate_reflection_contract() const noexcept {
        if (!actor_class_ || !widget_class_ || !interactable_class_ || !visibility_function_ ||
            !pickup_function_) return false;
        if (!function_owner_is(visibility_function_, widget_class_) ||
            !function_owner_is(pickup_function_, interactable_class_) || pickup_function_->GetParmsSize() != 0) return false;

        bool found_component{};
        bool found_active{};
        for (auto* property : TFieldRange<FProperty>(visibility_function_, EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm)) continue;
            if (property->GetName() == STR("InteractableComponent") && property->IsA<FObjectProperty>()) {
                found_component = true;
            } else if (property->GetName() == STR("Active") && property->IsA<FBoolProperty>()) {
                found_active = true;
            } else {
                return false;
            }
        }
        return found_component && found_active;
    }

    [[nodiscard]] VisibilityParameters read_visibility_parameters_unsafe(
        UnrealScriptFunctionCallableContext& context) const noexcept {
        VisibilityParameters result{};
        auto* parameters = context.TheStack.Locals();
        if (!parameters) return result;
        bool found_component{};
        bool found_active{};
        for (auto* property : TFieldRange<FProperty>(visibility_function_, EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm) ||
                property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm)) continue;
            auto* value = property->ContainerPtrToValuePtr<void>(parameters);
            if (!value) return {};
            if (property->GetName() == STR("InteractableComponent") && property->IsA<FObjectProperty>()) {
                result.component = static_cast<FObjectPropertyBase*>(property)->GetObjectPropertyValue(value);
                found_component = true;
            } else if (property->GetName() == STR("Active") && property->IsA<FBoolProperty>()) {
                result.active = static_cast<FBoolProperty*>(property)->GetPropertyValue(value);
                found_active = true;
            }
        }
        result.valid = found_component && found_active;
        return result;
    }

    void visibility_pre(UnrealScriptFunctionCallableContext& context) noexcept {
        ++visibility_events_;
        bool accepted{};
#if defined(_MSC_VER)
        __try { accepted = visibility_pre_unsafe(context); }
        __except (EXCEPTION_EXECUTE_HANDLER) { accepted = false; ++visibility_faults_; }
#else
        accepted = visibility_pre_unsafe(context);
#endif
        if (!accepted) return;
    }

    bool visibility_pre_unsafe(UnrealScriptFunctionCallableContext& context) {
        auto* widget = context.Context;
        if (!widget || !widget->IsA(widget_class_)) return false;
        const auto parameters = read_visibility_parameters_unsafe(context);
        if (!parameters.valid) return false;

        if (!parameters.active || !parameters.component || !parameters.component->IsA(interactable_class_)) {
            const std::scoped_lock lock{target_mutex_};
            if (!parameters.component || pack_weak_identity(FWeakObjectPtr{parameters.component}) == component_identity_) {
                target_visible_.store(false, std::memory_order_release);
                pickup_pending_.store(false, std::memory_order_release);
                widget_.Reset();
                component_.Reset();
                component_identity_ = 0;
                ++hidden_events_;
            }
            return true;
        }

        const FWeakObjectPtr widget_weak{widget};
        const FWeakObjectPtr component_weak{parameters.component};
        const auto identity = pack_weak_identity(component_weak);
        {
            const std::scoped_lock lock{target_mutex_};
            widget_ = widget_weak;
            component_ = component_weak;
            if (identity != component_identity_ || !target_visible_.load(std::memory_order_acquire)) {
                component_identity_ = identity;
                pickup_pending_.store(true, std::memory_order_release);
            }
            target_visible_.store(true, std::memory_order_release);
        }
        ++visible_events_;
        logger_.write(dsnap::LogAudience::Debug, "PROMPT_TARGET_CAPTURED",
                      std::format("component=0x{:X} active=1 pending={} source=game_pickup_prompt",
                                  identity, pickup_pending_.load()));
        return true;
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

    [[nodiscard]] static std::string object_full_name(UObject* object) {
        return object ? to_string(object->GetFullName()) : "null";
    }

    [[nodiscard]] static std::string object_class_name(UObject* object) {
        auto* object_class = object ? object->GetClassPrivate() : nullptr;
        return object_class ? to_string(object_class->GetPathName()) : "null";
    }

    [[nodiscard]] bool game_window_is_foreground() const noexcept {
        const auto foreground = GetForegroundWindow();
        if (!foreground) return false;
        DWORD process_id{};
        static_cast<void>(GetWindowThreadProcessId(foreground, &process_id));
        return process_id == GetCurrentProcessId();
    }
    void engine_tick_post(UEngine* engine) noexcept {
        const auto now = Clock::now();
        if (next_pulse_due_ != Clock::time_point{} && now < next_pulse_due_) return;
        next_pulse_due_ = now + kPulseInterval;

        if (toggle_requested_.exchange(false, std::memory_order_acq_rel)) {
            const bool can_enable = build_trusted_.load(std::memory_order_acquire) &&
                                    configuration_result_.value.automatic_pickup;
            const bool next = can_enable && !active_.load(std::memory_order_acquire);
            active_.store(next, std::memory_order_release);
            if (next && target_visible_.load(std::memory_order_acquire)) {
                pickup_pending_.store(true, std::memory_order_release);
            }
            logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                          std::format("state={} trusted={} target_visible={}", next ? "On" : "Off", can_enable,
                                      target_visible_.load()));
        }

        if (!active_.load(std::memory_order_acquire) ||
            !pickup_pending_.exchange(false, std::memory_order_acq_rel)) return;

        ++action_attempts_;
        const char* reason{"unknown"};
        bool faulted{};
        const bool invoked = invoke_pickup_guarded(this, engine, &reason, &faulted);
        if (invoked) {
            ++rpc_invocations_;
        } else {
            ++action_failures_;
            logger_.write(dsnap::LogAudience::User, "ACTION_REJECTED",
                          std::format("reason={} faulted={} component_identity={}", reason, faulted,
                                      current_component_identity()));
        }
    }

    bool invoke_pickup_unsafe(UEngine* engine, const char** reason) {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!game_window_is_foreground()) return reject("game_not_foreground");

        FWeakObjectPtr widget_weak{};
        FWeakObjectPtr component_weak{};
        std::uint64_t identity{};
        {
            const std::scoped_lock lock{target_mutex_};
            if (!target_visible_.load(std::memory_order_acquire)) return reject("prompt_not_visible");
            widget_weak = widget_;
            component_weak = component_;
            identity = component_identity_;
        }

        auto* widget = widget_weak.Get();
        auto* component = component_weak.Get();
        if (!widget || !component || !widget->IsA(widget_class_) ||
            !component->IsA(interactable_class_) || pack_weak_identity(component_weak) != identity) {
            return reject("prompt_weak_identity_invalid");
        }
        auto* owner = component->GetOuterPrivate();
        if (!owner || !owner->IsA(actor_class_) || component->GetOuterPrivate() != owner) {
            return reject("prompt_owner_invalid");
        }

        auto* interactable = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"));
        auto* interact_type = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"));
        if (!interactable || !interact_type) return reject("prompt_state_fields_missing");
        if (*interactable != kRequiredInteractableValue || *interact_type != dsnap::kDropItemInteractType) {
            logger_.write(dsnap::LogAudience::Debug, "PROMPT_TARGET_REJECTED",
                          std::format("reason=state_mismatch owner={} interactable={} interact_type={}",
                                      object_full_name(owner), *interactable, *interact_type));
            return reject("prompt_state_mismatch");
        }

        PlayerContext context{};
        const char* receiver_reason{"unknown"};
        if (!resolve_player_context(engine, &context)) return reject("player_context_unavailable");
        if (!validate_receiver(context, &receiver_reason)) return reject(receiver_reason);
        auto* world = context.pawn->GetWorld();
        if (!world || owner->GetWorld() != world || component->GetWorld() != world) {
            return reject("prompt_world_mismatch");
        }

        auto** target_object = context.interaction_receiver
            ->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetObject"));
        auto** target_component = context.interaction_receiver
            ->GetValuePtrByPropertyNameInChain<UObject*>(STR("ExecuteTargetComponent"));
        if (!target_object || !target_component) return reject("receiver_target_properties_missing");
        if ((*target_object && *target_object != owner) ||
            (*target_component && *target_component != component)) {
            return reject("receiver_target_busy");
        }

        const auto ownership = dsnap::transient_target_ownership(*target_object == nullptr,
                                                                 *target_component == nullptr);
        if (!assign_transient_targets_guarded(target_object, target_component, owner, component,
                                              ownership.target_object, ownership.target_component)) {
            bool ignored_object_cleared{};
            bool ignored_component_cleared{};
            static_cast<void>(clear_transient_targets_guarded(
                target_object, target_component, owner, component,
                ownership.target_object, ownership.target_component,
                &ignored_object_cleared, &ignored_component_cleared));
            return reject("transient_target_assignment_fault");
        }

        logger_.write(dsnap::LogAudience::User, "TARGET_VALIDATED",
                      std::format("source=game_pickup_prompt owner=0x{:X} owner_name={} owner_class={} "
                                  "component=0x{:X} receiver=0x{:X} receiver_source={} "
                                  "transient_object={} transient_component={}",
                                  pack_weak_identity(FWeakObjectPtr{owner}), object_full_name(owner),
                                  object_class_name(owner), identity,
                                  pack_weak_identity(FWeakObjectPtr{context.interaction_receiver}),
                                  context.receiver_source, ownership.target_object, ownership.target_component));

        const bool rpc_invoked = process_pickup_event_guarded(context.interaction_receiver,
                                                              pickup_function_);

        bool object_cleared{};
        bool component_cleared{};
        if (!clear_transient_targets_guarded(target_object, target_component, owner, component,
                                             ownership.target_object, ownership.target_component,
                                             &object_cleared, &component_cleared)) {
            return reject("transient_target_cleanup_fault");
        }
        if (!rpc_invoked) return reject("pickup_rpc_fault");

        logger_.write(dsnap::LogAudience::User, "ACTION_RPC_INVOKED",
                      std::format("receiver=0x{:X} function=Server_RunInteractV2 owner=0x{:X} component=0x{:X} "
                                  "target_object_cleared={} target_component_cleared={} "
                                  "visible_collection=pending_owner_confirmation",
                                  pack_weak_identity(FWeakObjectPtr{context.interaction_receiver}),
                                  pack_weak_identity(FWeakObjectPtr{owner}), identity,
                                  object_cleared, component_cleared));
        if (reason) *reason = "invoked";
        return true;
    }

    static bool invoke_pickup_guarded(NativeAutoPickup* self,
                                      UEngine* engine,
                                      const char** reason,
                                      bool* faulted) noexcept {
        bool invoked{};
#if defined(_MSC_VER)
        __try { invoked = self->invoke_pickup_unsafe(engine, reason); }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            invoked = false;
            if (reason) *reason = "guarded_runtime_fault";
            if (faulted) *faulted = true;
        }
#else
        invoked = self->invoke_pickup_unsafe(engine, reason);
#endif
        return invoked;
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

    [[nodiscard]] std::uint64_t current_component_identity() const noexcept {
        const std::scoped_lock lock{target_mutex_};
        return component_identity_;
    }

    void reset_world(const char* source) noexcept {
        pickup_pending_.store(false, std::memory_order_release);
        target_visible_.store(false, std::memory_order_release);
        {
            const std::scoped_lock lock{target_mutex_};
            widget_.Reset();
            component_.Reset();
            component_identity_ = 0;
        }
        ++world_resets_;
        logger_.write(dsnap::LogAudience::User, "WORLD_RESET",
                      std::format("reason={} target_cache=cleared active_preserved={}", source, active_.load()));
    }

    void unregister_callbacks() noexcept {
        if (engine_tick_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(engine_tick_callback_id_);
            engine_tick_callback_id_ = Hook::ERROR_ID;
        }
        if (world_reset_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(world_reset_callback_id_);
            world_reset_callback_id_ = Hook::ERROR_ID;
        }
        if (visibility_function_ && visibility_hook_.first != 0 && visibility_hook_.second != 0) {
            try { UObjectGlobals::UnregisterHook(visibility_function_, visibility_hook_); }
            catch (...) {}
            visibility_hook_ = {};
        }
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};

    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    dsnap::FingerprintResult fingerprint_result_{};
    ProcessShutdownProbe shutdown_probe_{};

    std::atomic<bool> shutting_down_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> active_{};
    std::atomic<bool> toggle_requested_{};
    std::atomic<bool> target_visible_{};
    std::atomic<bool> pickup_pending_{};
    bool fingerprint_applied_{};
    bool f9_was_down_{};

    mutable std::mutex target_mutex_{};
    FWeakObjectPtr widget_{};
    FWeakObjectPtr component_{};
    std::uint64_t component_identity_{};

    UClass* actor_class_{};
    UClass* widget_class_{};
    UClass* interactable_class_{};
    UFunction* visibility_function_{};
    UFunction* pickup_function_{};
    std::pair<int, int> visibility_hook_{};
    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};

    Clock::time_point last_f9_edge_{};
    Clock::time_point next_pulse_due_{};
    Clock::time_point next_perf_log_{};

    std::atomic<std::uint64_t> visibility_events_{};
    std::atomic<std::uint64_t> visible_events_{};
    std::atomic<std::uint64_t> hidden_events_{};
    std::atomic<std::uint64_t> visibility_faults_{};
    std::atomic<std::uint64_t> action_attempts_{};
    std::atomic<std::uint64_t> rpc_invocations_{};
    std::atomic<std::uint64_t> action_failures_{};
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
