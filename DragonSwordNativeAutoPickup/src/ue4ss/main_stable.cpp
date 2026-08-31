// DRAGONSWORD_NATIVE_AUTO_PICKUP_1_0_1_STABLE targeting official UE4SS v3.0.1.
//
// Version 1.0.1 preserves the owner-accepted automatic action, mounted Rider,
// fish, closed target policy, and interval-only optional diagnostics.
// Stable UE4SS has no C++ EngineTick callback or removable native callbacks.
// Its event thread therefore polls only the configured Windows virtual key and
// publishes an atomic pulse. The already-installed ProcessEvent hook consumes
// that pulse on the game thread and resolves a fresh Engine through the current
// event World. There is no UObject/Actor scan, worker-thread UObject access,
// cross-World gameplay-object cache, direct interaction RPC, or range mutation.

#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Mod/CppUserModBase.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Unreal/FScriptArray.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/World.hpp>

#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#pragma warning(pop)

#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

extern "C" IMAGE_DOS_HEADER __ImageBase;

constexpr auto kVersion = STR("1.0.1");
constexpr auto kLabel = "DRAGONSWORD_NATIVE_AUTO_PICKUP_1_0_1_STABLE";
constexpr auto kCurrentGameSha256 = "85E0F6BAFF78940C53451A282559A9378F6541E24B1CC81CD8CAF1556204A52E";
constexpr auto kDropItemClassPath = STR("/Script/DS.DropItemActor");
constexpr auto kInteractableClassPath = STR("/Script/DS.DInteractableComponent");
constexpr auto kEnhancedPlayerInputClassPath = STR("/Script/EnhancedInput.EnhancedPlayerInput");
constexpr auto kInputActionClassPath = STR("/Script/EnhancedInput.InputAction");
constexpr auto kEnhancedActionMappingStructPath = STR("/Script/EnhancedInput.EnhancedActionKeyMapping");
constexpr auto kKeyStructPath = STR("/Script/InputCore.Key");
constexpr auto kEnhancedInputSubsystemClassPath = STR("/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem");
constexpr auto kSubsystemLibraryClassPath = STR("/Script/Engine.SubsystemBlueprintLibrary");
constexpr auto kEngineClassPath = STR("/Script/Engine.Engine");
constexpr auto kGetLocalPlayerSubsystemFunctionPath =
    STR("/Script/Engine.SubsystemBlueprintLibrary:GetLocalPlayerSubSystemFromPlayerController");
constexpr auto kInjectInputVectorFunctionPath =
    STR("/Script/EnhancedInput.EnhancedInputSubsystemInterface:InjectInputVectorForAction");
constexpr std::uintptr_t kNativeSelectorRva = 0x42C0160;
constexpr auto kPulseInterval = std::chrono::milliseconds{33};
constexpr auto kActiveScanInterval = std::chrono::milliseconds{33};
constexpr auto kIdleScanInterval = std::chrono::milliseconds{33};
constexpr auto kTransientBackoff = std::chrono::milliseconds{500};
constexpr auto kFaultBackoff = std::chrono::milliseconds{1000};
constexpr auto kPostPickupCooldown = std::chrono::milliseconds{33};
constexpr auto kRecentTargetCooldown = std::chrono::milliseconds{250};
constexpr auto kWorldSettleDelay = std::chrono::milliseconds{1500};
constexpr auto kSelectorAttemptBudget = std::chrono::milliseconds{250};
constexpr std::uint32_t kMaxSelectorCallsPerWindow = 1;
constexpr auto kToggleDebounce = std::chrono::milliseconds{500};
constexpr std::uint8_t kRequiredInteractableValue = 2;
constexpr std::int32_t kMaxEnhancedActionMappings = 512;
constexpr std::int32_t kMaxReflectedMappingStructSize = 4096;
constexpr std::int32_t kMaxFunctionParameterBytes = 4096;

struct NativeSelectorPair {
    UObject* actor{};
    UObject* component{};
};
static_assert(sizeof(NativeSelectorPair) == 16);

struct TimingSnapshot {
    std::uint64_t count{};
    std::uint64_t total_us{};
    std::uint64_t max_us{};

    [[nodiscard]] std::uint64_t average_us() const noexcept {
        return count == 0 ? 0 : total_us / count;
    }
};

struct AtomicTiming {
    void record(std::uint64_t microseconds) noexcept {
        count.fetch_add(1, std::memory_order_relaxed);
        total_us.fetch_add(microseconds, std::memory_order_relaxed);
        auto current_max = max_us.load(std::memory_order_relaxed);
        while (current_max < microseconds &&
               !max_us.compare_exchange_weak(current_max, microseconds,
                                             std::memory_order_relaxed,
                                             std::memory_order_relaxed)) {
        }
    }

    [[nodiscard]] TimingSnapshot take_interval() noexcept {
        return TimingSnapshot{
            count.exchange(0, std::memory_order_acq_rel),
            total_us.exchange(0, std::memory_order_acq_rel),
            max_us.exchange(0, std::memory_order_acq_rel),
        };
    }

    std::atomic<std::uint64_t> count{};
    std::atomic<std::uint64_t> total_us{};
    std::atomic<std::uint64_t> max_us{};
};

struct ScopeTiming {
    AtomicTiming* metric{};
    Clock::time_point started{};

    explicit ScopeTiming(AtomicTiming* value) noexcept
        : metric(value), started(value ? Clock::now() : Clock::time_point{}) {}

    ~ScopeTiming() noexcept {
        if (!metric) return;
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count();
        metric->record(elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0);
    }
};

struct StageTimingScope {
    std::uint64_t* output{};
    Clock::time_point started{};

    explicit StageTimingScope(std::uint64_t* value) noexcept
        : output(value), started(value ? Clock::now() : Clock::time_point{}) {}

    void finish() noexcept {
        if (!output) return;
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count();
        *output = elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0;
        output = nullptr;
    }

    ~StageTimingScope() noexcept { finish(); }
};

struct StageTimings {
    std::uint64_t context_us{};
    std::uint64_t selector_us{};
    std::uint64_t validation_us{};
    std::uint64_t action_resolution_us{};
    std::uint64_t subsystem_us{};
    std::uint64_t injection_us{};
};

using NativeSelectorFunction = void(__fastcall*)(UObject*, NativeSelectorPair*, std::uint8_t);

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
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), buffer.data(),
                                           static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path().parent_path();
}

std::filesystem::path game_input_settings_path() {
    return binary_directory().parent_path().parent_path() /
        "Saved" / "Config" / "Windows" / "GameUserInputSettings.ini";
}

std::string join_errors(const std::vector<std::string>& errors) {
    std::string output;
    for (const auto& error : errors) {
        if (!output.empty()) output += "; ";
        output += error;
    }
    return output;
}

[[nodiscard]] std::uint64_t pack_weak_identity(const FWeakObjectPtr& weak) noexcept {
    return reinterpret_cast<std::uintptr_t>(weak.Get());
}

[[nodiscard]] FWeakObjectPtr make_weak(UObject* object) noexcept {
    FWeakObjectPtr weak{};
    weak = object;
    return weak;
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
        ModDescription = STR("Startup-off native-selector automatic pickup with a configurable toggle key");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "START",
                      std::format("label={} mode=startup_off_configurable_toggle generation={}",
                                  kLabel, callback_gate_.generation()));
        for (const auto& error : configuration_result_.errors) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIG_REJECTED", error);
        }
    }

    ~NativeAutoPickup() override {
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        const bool process_shutdown = shutdown_probe_ && shutdown_probe_() != FALSE;
        const bool registry_available = !process_shutdown && UnrealInitializer::StaticStorage::bIsInitialized;
        if (registry_available) {
            unregister_callbacks();
            cancel_window("shutdown");
            clear_confirmation();
            static_cast<void>(logger_.flush_all());
        } else {
            pending_key_events_.store(0, std::memory_order_release);
            pending_toggle_requests_.store(0, std::memory_order_release);
            automation_enabled_.store(false, std::memory_order_release);
            window_active_.store(false, std::memory_order_release);
            confirmation_pending_.store(false, std::memory_order_release);
            process_event_callback_registered_ = false;
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
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        pending_key_events_.store(0, std::memory_order_release);
        pending_toggle_requests_.store(0, std::memory_order_release);
        automation_enabled_.store(false, std::memory_order_release);
        window_active_.store(false, std::memory_order_release);
        confirmation_pending_.store(false, std::memory_order_release);
        game_thread_pulse_requested_.store(false, std::memory_order_release);
        process_event_callback_registered_.store(false, std::memory_order_release);
        static_cast<void>(logger_.flush_all());
    }

    void on_unreal_init() override {
        if (!configuration_result_.valid()) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "invalid configuration");
            return;
        }

        const auto toggle_key = dsnap::parse_toggle_hotkey(configuration_result_.value.toggle_hotkey);
        if (!toggle_key) {
            unregister_callbacks();
            logger_.write(dsnap::LogAudience::User, "DISABLED", "configured toggle hotkey is unsupported");
            return;
        }
        toggle_virtual_key_ = *toggle_key;

        const auto generation = callback_gate_.generation();
        Hook::RegisterProcessEventPreCallback([generation](UObject* context, UFunction*, void*) {
            if (auto* self = current_instance(generation)) {
                bool faulted{};
                static_cast<void>(process_event_pulse_guarded(self, context, &faulted));
                if (faulted) self->disable_after_process_event_fault();
            }
        });
        process_event_callback_registered_ = true;

        logger_.write(dsnap::LogAudience::User, "CALLBACK_READY",
                      std::format("label={} hotkey={} mode=toggle_automatic selector_rva=0x{:X} "
                                  "allowed_types=2,5,7 excluded_types=4_treasure_box "
                                  "action=enhanced_input_live_interaction_action_one_shot "
                                  "interaction_key={} interaction_key_fallback={} "
                                  "engine_pulse_ms={} active_scan_ms={} idle_scan_ms={} transient_backoff_ms={} "
                                  "post_pickup_ms={} world_settle_ms={} selector_calls_per_scan={} "
                                  "debug_logging={} perf_interval_seconds={} slow_scan_threshold_us={} "
                                  "hotkey_source=stable_event_thread_GetAsyncKeyState "
                                  "game_thread_source=stable_ProcessEvent object_scans=0 "
                                  "reflection_init=deferred_to_game_thread "
                                  "worker_thread_uobject_access=0 cross_world_object_cache=0 "
                                  "target_field_access=0 direct_RPC=0 SendInput=0 range_mutation=0",
                                  kLabel, configuration_result_.value.toggle_hotkey, kNativeSelectorRva,
                                  configuration_result_.value.interaction_key,
                                  configuration_result_.value.interaction_key_fallback,
                                  kPulseInterval.count(),
                                  kActiveScanInterval.count(), kIdleScanInterval.count(),
                                  kTransientBackoff.count(), kPostPickupCooldown.count(),
                                  kWorldSettleDelay.count(), kMaxSelectorCallsPerWindow,
                                  configuration_result_.value.debug_logging,
                                  configuration_result_.value.perf_log_interval_seconds,
                                  configuration_result_.value.slow_scan_threshold_us));
        logger_.write(dsnap::LogAudience::User, "AUTOMATION_AVAILABLE",
                      std::format("automatic pickup is off at launch; press {} to enable or disable",
                                  configuration_result_.value.toggle_hotkey));
    }

    void on_update() override {
        static_cast<void>(logger_.flush());
        if (shutting_down_.load(std::memory_order_acquire)) return;

        const auto update_now = Clock::now();
        if (next_game_thread_request_due_ == Clock::time_point{} ||
            update_now >= next_game_thread_request_due_) {
            game_thread_pulse_requested_.store(true, std::memory_order_release);
            next_game_thread_request_due_ = update_now + kPulseInterval;
        }

        const bool toggle_down = toggle_virtual_key_ != 0 &&
            (GetAsyncKeyState(static_cast<int>(toggle_virtual_key_)) & 0x8000) != 0;
        if (toggle_down && !toggle_key_was_down_ && foreground_window_state().matched) {
            pending_key_events_.fetch_add(1, std::memory_order_release);
            game_thread_pulse_requested_.store(true, std::memory_order_release);
        }
        toggle_key_was_down_ = toggle_down;

        if (!fingerprint_applied_) {
            fingerprint_result_ = dsnap::verify_build_fingerprint(binary_directory());
            fingerprint_applied_ = true;
            const bool exact_game = fingerprint_result_.game_sha256 == kCurrentGameSha256;
            build_trusted_.store(fingerprint_result_.trusted, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                          std::format("trusted={} exact_current_build={} game={} ue4ss={} error={}",
                                      fingerprint_result_.trusted, exact_game, fingerprint_result_.game_sha256,
                                      fingerprint_result_.ue4ss_sha256, fingerprint_result_.error));
            logger_.write(dsnap::LogAudience::User,
                          build_trusted_.load(std::memory_order_acquire) ? "AUTOMATION_AVAILABLE" : "PASSIVE_ONLY",
                          build_trusted_.load(std::memory_order_acquire)
                              ? std::format("press {} to toggle automatic pickup",
                                            configuration_result_.value.toggle_hotkey)
                              : "unknown UE4SS fingerprint");
            static_cast<void>(logger_.flush_all());
        }

        const auto key_event_count = pending_key_events_.exchange(0, std::memory_order_acq_rel);
        if (key_event_count != 0) {
            key_events_received_.fetch_add(key_event_count, std::memory_order_relaxed);
            const auto coalesced_count = key_event_count - 1;
            if (coalesced_count != 0) {
                key_events_coalesced_.fetch_add(coalesced_count, std::memory_order_relaxed);
            }

            const auto key_event_now = Clock::now();
            if (last_key_event_accepted_ != Clock::time_point{} &&
                key_event_now - last_key_event_accepted_ < kToggleDebounce) {
                ++key_events_rejected_;
                logger_.write(dsnap::LogAudience::User, "TOGGLE_REJECTED",
                              "reason=debounced source=stable_GetAsyncKeyState");
            } else {
                last_key_event_accepted_ = key_event_now;
                ++toggle_requests_accepted_;
                pending_toggle_requests_.fetch_add(1, std::memory_order_release);
                logger_.write(dsnap::LogAudience::User, "TOGGLE_REQUESTED",
                              std::format("source=stable_GetAsyncKeyState hotkey={} event_count={} coalesced={}",
                                          configuration_result_.value.toggle_hotkey,
                                          key_event_count, coalesced_count));
            }
        }

        const auto now = Clock::now();
        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            const auto pulse_gap = pulse_gap_timing_.take_interval();
            const auto pulse_work = pulse_work_timing_.take_interval();
            const auto scan = scan_timing_.take_interval();
            const auto context = context_timing_.take_interval();
            const auto selector = selector_timing_.take_interval();
            const auto validation = validation_timing_.take_interval();
            const auto action_resolution = action_resolution_timing_.take_interval();
            const auto subsystem = subsystem_timing_.take_interval();
            const auto injection = injection_timing_.take_interval();
            const auto slow_scans = slow_scan_count_.exchange(0, std::memory_order_acq_rel);
            if (configuration_result_.value.debug_logging) {
                logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                              std::format("key_events_received={} key_events_coalesced={} key_events_rejected={} "
                                          "toggle_requests_accepted={} toggle_transitions={} automatic_scans={} "
                                          "foreground_pauses={} automatic_disables={} "
                                          "windows_started={} windows_hit={} windows_cancelled={} "
                                          "selector_attempts={} selector_no_candidate={} selector_pairs={} "
                                          "enhanced_input_attempts={} enhanced_input_injections={} "
                                          "action_failures={} transient_failures={} confirmations={} "
                                          "confirmation_timeouts={} selector_faults={} world_resets={} "
                                          "automation_enabled={} window_active={} confirmation_pending={} "
                                          "logger_dropped={} object_scans=0 cross_world_object_cache=0",
                                          key_events_received_.load(), key_events_coalesced_.load(),
                                          key_events_rejected_.load(), toggle_requests_accepted_.load(),
                                          toggle_transitions_.load(), automatic_scans_.load(),
                                          foreground_pauses_.load(), automatic_disables_.load(),
                                          windows_started_.load(), windows_hit_.load(), windows_cancelled_.load(),
                                          selector_attempts_.load(), selector_no_candidate_.load(), selector_pairs_.load(),
                                          enhanced_input_attempts_.load(), enhanced_input_injections_.load(),
                                          action_failures_.load(), transient_failures_.load(), confirmations_.load(),
                                          confirmation_timeouts_.load(), selector_faults_.load(), world_resets_.load(),
                                          automation_enabled_.load(std::memory_order_acquire),
                                          window_active_.load(std::memory_order_acquire),
                                          confirmation_pending_.load(std::memory_order_acquire),
                                          logger_.dropped_messages()));
                logger_.write(dsnap::LogAudience::Debug, "PERF_TIMING",
                              std::format("interval_seconds={} slow_threshold_us={} "
                                          "pulse_gap_count={} pulse_gap_avg_us={} pulse_gap_max_us={} "
                                          "pulse_work_count={} pulse_work_avg_us={} pulse_work_max_us={} "
                                          "scan_count={} scan_avg_us={} scan_max_us={} slow_scans={} "
                                          "context_count={} context_avg_us={} context_max_us={} "
                                          "selector_count={} selector_avg_us={} selector_max_us={} "
                                          "validation_count={} validation_avg_us={} validation_max_us={} "
                                          "action_resolution_count={} action_resolution_avg_us={} action_resolution_max_us={} "
                                          "subsystem_count={} subsystem_avg_us={} subsystem_max_us={} "
                                          "injection_count={} injection_avg_us={} injection_max_us={}",
                                          configuration_result_.value.perf_log_interval_seconds,
                                          configuration_result_.value.slow_scan_threshold_us,
                                          pulse_gap.count, pulse_gap.average_us(), pulse_gap.max_us,
                                          pulse_work.count, pulse_work.average_us(), pulse_work.max_us,
                                          scan.count, scan.average_us(), scan.max_us, slow_scans,
                                          context.count, context.average_us(), context.max_us,
                                          selector.count, selector.average_us(), selector.max_us,
                                          validation.count, validation.average_us(), validation.max_us,
                                          action_resolution.count, action_resolution.average_us(), action_resolution.max_us,
                                          subsystem.count, subsystem.average_us(), subsystem.max_us,
                                          injection.count, injection.average_us(), injection.max_us));
            }
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

private:
    struct PlayerContext {
        UObject* local_player{};
        UObject* controller{};
        UObject* pawn{};
        UObject* interaction_owner{};
        UObject* interaction_receiver{};
        const char* receiver_source{"none"};
        bool mounted{};
    };

    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] static bool process_event_pulse_guarded(NativeAutoPickup* self,
                                                           UObject* context,
                                                           bool* faulted) noexcept {
        if (faulted) *faulted = false;
        if (!self) return false;
#if defined(_MSC_VER)
        __try {
#endif
            self->process_event_pulse(context);
            return true;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            if (faulted) *faulted = true;
            return false;
        }
#endif
    }

    void disable_after_process_event_fault() noexcept {
        process_event_callback_registered_.store(false, std::memory_order_release);
        game_thread_pulse_requested_.store(false, std::memory_order_release);
        automation_enabled_.store(false, std::memory_order_release);
        window_active_.store(false, std::memory_order_release);
        confirmation_pending_.store(false, std::memory_order_release);
        ++automatic_disables_;
        logger_.write(dsnap::LogAudience::User, "DISABLED",
                      "reason=stable_ProcessEvent_guard_fault fail_closed=1");
    }

    [[nodiscard]] bool initialize_reflection_contract_on_game_thread() noexcept {
        if (reflection_initialized_) return true;
        if (reflection_failed_) return false;

        drop_item_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kDropItemClassPath);
        interactable_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInteractableClassPath);
        enhanced_player_input_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kEnhancedPlayerInputClassPath);
        input_action_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInputActionClassPath);
        engine_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kEngineClassPath);
        enhanced_action_mapping_struct_ =
            UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, kEnhancedActionMappingStructPath);
        key_struct_ = UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, kKeyStructPath);
        enhanced_input_subsystem_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kEnhancedInputSubsystemClassPath);
        subsystem_library_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kSubsystemLibraryClassPath);
        get_local_player_subsystem_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kGetLocalPlayerSubsystemFunctionPath);
        inject_input_vector_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kInjectInputVectorFunctionPath);
        subsystem_library_cdo_ = subsystem_library_class_
            ? subsystem_library_class_->GetClassDefaultObject()
            : nullptr;

        if (!validate_reflection_contract()) {
            reflection_failed_ = true;
            logger_.write(dsnap::LogAudience::User, "DISABLED",
                          std::format("reflection contract missing or changed drop_item={} interactable={} "
                                      "engine={} enhanced_player_input={} input_action={} "
                                      "mapping_struct={} key_struct={} subsystem={} library={} "
                                      "library_cdo={} get_subsystem_function={} inject_function={} "
                                      "thread=game",
                                      drop_item_class_ != nullptr, interactable_class_ != nullptr,
                                      engine_class_ != nullptr, enhanced_player_input_class_ != nullptr,
                                      input_action_class_ != nullptr,
                                      enhanced_action_mapping_struct_ != nullptr, key_struct_ != nullptr,
                                      enhanced_input_subsystem_class_ != nullptr,
                                      subsystem_library_class_ != nullptr, subsystem_library_cdo_ != nullptr,
                                      get_local_player_subsystem_function_ != nullptr,
                                      inject_input_vector_function_ != nullptr));
            return false;
        }

        reflection_initialized_ = true;
        logger_.write(dsnap::LogAudience::User, "READY",
                      "reflection_contract=validated thread=game callback=stable_ProcessEvent");
        return true;
    }

    [[nodiscard]] UEngine* resolve_engine_from_event_context(UObject* context,
                                                              UObject** world_out) const noexcept {
        if (world_out) *world_out = nullptr;
        if (!context || !engine_class_) return nullptr;

        auto* world = context->GetWorld();
        if (!world) return nullptr;
        if (world_out) *world_out = world;

        auto* game_instance = named_object(world, STR("OwningGameInstance"));
        auto* players = game_instance
            ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers"))
            : nullptr;
        if (!players || !players->IsValidIndex(0) || !players->GetData()) return nullptr;

        auto* local_player = static_cast<UObject* const*>(players->GetData())[0];
        auto* viewport = named_object(local_player, STR("ViewportClient"));
        auto* engine = viewport ? viewport->GetTypedOuter(engine_class_) : nullptr;
        return engine && engine->IsA(engine_class_) ? static_cast<UEngine*>(engine) : nullptr;
    }

    void process_event_pulse(UObject* context) noexcept {
        if (shutting_down_.load(std::memory_order_acquire) ||
            !process_event_callback_registered_.load(std::memory_order_acquire) ||
            !game_thread_pulse_requested_.load(std::memory_order_acquire)) {
            return;
        }

        if (!initialize_reflection_contract_on_game_thread()) {
            game_thread_pulse_requested_.store(false, std::memory_order_release);
            return;
        }

        UObject* world{};
        auto* engine = resolve_engine_from_event_context(context, &world);
        if (!engine || !world || shutting_down_.load(std::memory_order_acquire)) return;
        if (!game_thread_pulse_requested_.exchange(false, std::memory_order_acq_rel)) return;

        const auto world_identity = reinterpret_cast<std::uintptr_t>(world);
        if (world_identity != last_world_identity_) {
            last_world_identity_ = world_identity;
            reset_world("ProcessEventWorldChanged");
        }
        engine_tick_post(engine);
    }

    [[nodiscard]] bool validate_reflection_contract() const noexcept {
        if (!drop_item_class_ || !interactable_class_ || !engine_class_ ||
            !enhanced_player_input_class_ ||
            !input_action_class_ || !enhanced_action_mapping_struct_ || !key_struct_ ||
            !enhanced_input_subsystem_class_ || !subsystem_library_class_ || !subsystem_library_cdo_ ||
            !get_local_player_subsystem_function_ || !inject_input_vector_function_) {
            return false;
        }
        const auto get_subsystem_parameter_bytes = get_local_player_subsystem_function_->GetParmsSize();
        const auto inject_parameter_bytes = inject_input_vector_function_->GetParmsSize();
        auto* player_controller_property = CastField<FObjectPropertyBase>(
            function_property(get_local_player_subsystem_function_, STR("PlayerController")));
        auto* subsystem_class_property = CastField<FClassProperty>(
            function_property(get_local_player_subsystem_function_, STR("Class")));
        auto* subsystem_return_property = CastField<FObjectPropertyBase>(
            get_local_player_subsystem_function_->GetReturnProperty());
        auto* action_property = CastField<FObjectPropertyBase>(
            function_property(inject_input_vector_function_, STR("Action")));
        auto* value_property = CastField<FStructProperty>(
            function_property(inject_input_vector_function_, STR("Value")));
        auto* modifiers_property = CastField<FArrayProperty>(
            function_property(inject_input_vector_function_, STR("Modifiers")));
        auto* triggers_property = CastField<FArrayProperty>(
            function_property(inject_input_vector_function_, STR("Triggers")));
        return get_subsystem_parameter_bytes > 0 &&
               get_subsystem_parameter_bytes <= kMaxFunctionParameterBytes &&
               inject_parameter_bytes > 0 && inject_parameter_bytes <= kMaxFunctionParameterBytes &&
               player_controller_property && subsystem_class_property && subsystem_return_property &&
               action_property && value_property && modifiers_property && triggers_property &&
               function_property_fits(get_local_player_subsystem_function_, STR("PlayerController"), sizeof(UObject*)) &&
               function_property_fits(get_local_player_subsystem_function_, STR("Class"), sizeof(UClass*)) &&
               return_property_fits(get_local_player_subsystem_function_, sizeof(UObject*)) &&
               function_property_fits(inject_input_vector_function_, STR("Action"), sizeof(UObject*)) &&
               function_property_fits(inject_input_vector_function_, STR("Value"), FVector::StaticSize()) &&
               function_property_fits(inject_input_vector_function_, STR("Modifiers"), sizeof(FScriptArray)) &&
               function_property_fits(inject_input_vector_function_, STR("Triggers"), sizeof(FScriptArray));
    }

    [[nodiscard]] static FProperty* function_property(UFunction* function, const wchar_t* name) noexcept {
        if (!function || !name) return nullptr;
        return function->FindProperty(FName{name, FNAME_Find});
    }

    [[nodiscard]] static bool property_range_fits(FProperty* property,
                                                   std::size_t parameter_size,
                                                   std::size_t expected_size) noexcept {
        if (!property || property->GetArrayDim() != 1 || property->GetOffset_Internal() < 0 ||
            property->GetElementSize() <= 0 ||
            static_cast<std::size_t>(property->GetElementSize()) != expected_size) {
            return false;
        }
        const auto offset = static_cast<std::size_t>(property->GetOffset_Internal());
        return offset <= parameter_size && expected_size <= parameter_size - offset;
    }

    [[nodiscard]] static bool function_property_fits(UFunction* function,
                                                      const wchar_t* name,
                                                      std::size_t expected_size) noexcept {
        return function && property_range_fits(function_property(function, name),
                                                function->GetParmsSize(), expected_size);
    }

    [[nodiscard]] static bool return_property_fits(UFunction* function,
                                                    std::size_t expected_size) noexcept {
        return function && property_range_fits(function->GetReturnProperty(),
                                                function->GetParmsSize(), expected_size);
    }

    template <typename ValueType>
    [[nodiscard]] static bool write_function_parameter(UFunction* function,
                                                       std::vector<std::byte>& parameters,
                                                       const wchar_t* name,
                                                       const ValueType& value) noexcept {
        auto* property = function_property(function, name);
        if (!property_range_fits(property, parameters.size(), sizeof(ValueType))) return false;
        std::memcpy(parameters.data() + property->GetOffset_Internal(), &value, sizeof(ValueType));
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

        auto* interaction_owner = pawn;
        bool mounted{};
        if (auto* rider = named_object(pawn, STR("Rider")); rider && rider->GetWorld() == pawn->GetWorld()) {
            interaction_owner = rider;
            mounted = true;
        }

        UObject* receiver = named_object(interaction_owner, STR("InteractableComponent"));
        const char* receiver_source = mounted
            ? "pawn.Rider.InteractableComponent"
            : "pawn.InteractableComponent";
        if (!receiver) {
            receiver = named_object(interaction_owner, STR("InteractionComponent"));
            receiver_source = mounted
                ? "pawn.Rider.InteractionComponent"
                : "pawn.InteractionComponent";
        }
        if (!receiver && interaction_owner != pawn) {
            receiver = named_object(pawn, STR("InteractableComponent"));
            receiver_source = "mounted_pawn.InteractableComponent_fallback";
        }
        if (!receiver && interaction_owner != pawn) {
            receiver = named_object(pawn, STR("InteractionComponent"));
            receiver_source = "mounted_pawn.InteractionComponent_fallback";
        }
        if (!receiver) {
            receiver = named_object(controller, STR("InteractableComponent"));
            receiver_source = "controller.InteractableComponent";
        }
        if (!receiver) {
            receiver = named_object(controller, STR("InteractionComponent"));
            receiver_source = "controller.InteractionComponent";
        }
        *output = {local_player, controller, pawn, interaction_owner, receiver,
                   receiver ? receiver_source : "none", mounted};
        return true;
    }

    [[nodiscard]] bool validate_receiver(const PlayerContext& context, const char** reason) const {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!context.pawn || !context.pawn->GetWorld()) return reject("pawn_world_unavailable");
        if (!context.interaction_owner ||
            context.interaction_owner->GetWorld() != context.pawn->GetWorld()) {
            return reject("interaction_owner_world_mismatch");
        }
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

    struct ForegroundWindowState {
        HWND window{};
        DWORD foreground_process_id{};
        DWORD current_process_id{};
        char window_class[256]{};
        bool process_matched{};
        bool console_class{};
        bool matched{};
    };

    [[nodiscard]] static ForegroundWindowState foreground_window_state() noexcept {
        ForegroundWindowState state{};
        state.window = GetForegroundWindow();
        state.current_process_id = GetCurrentProcessId();
        if (state.window) {
            static_cast<void>(GetWindowThreadProcessId(state.window, &state.foreground_process_id));
            static_cast<void>(GetClassNameA(state.window, state.window_class,
                                            static_cast<int>(sizeof(state.window_class))));
        }
        state.process_matched = state.window && state.foreground_process_id == state.current_process_id;
        state.console_class = std::string_view{state.window_class} == "ConsoleWindowClass";
        state.matched = state.process_matched && !state.console_class;
        return state;
    }

    struct EnhancedActionResolution {
        UObject* player_input{};
        FArrayProperty* mappings_property{};
        FStructProperty* mapping_property{};
        UScriptStruct* mapping_struct{};
        FObjectPropertyBase* action_property{};
        FStructProperty* key_property{};
        FNameProperty* key_name_property{};
        FBoolProperty* ignored_property{};
        std::int32_t mapping_count{};
        std::int32_t mapping_struct_size{};
        std::int32_t mapping_alignment{};
        std::int32_t matched_mappings{};
        std::int32_t active_mappings{};
        std::int32_t ignored_mappings{};
        std::int32_t action_property_offset{-1};
        std::int32_t key_property_offset{-1};
        std::int32_t ignored_property_offset{-1};
        std::uint8_t ignored_byte_offset{};
        std::uint8_t ignored_byte_mask{};
        std::uint8_t ignored_field_mask{};
        std::uint8_t raw_ignored_flag_byte{};
        bool automatic_mode{};
        bool conflicting_actions{};
        UObject* interaction_action{};
        std::string active_binding_keys{};
    };

    [[nodiscard]] bool use_interaction_binding_fallback(std::string failure) {
        resolved_interaction_key_ = configuration_result_.value.interaction_key_fallback;
        interaction_binding_mode_ = "auto_fallback";
        logger_.write(dsnap::LogAudience::User, "INTERACTION_BINDING_FALLBACK",
                      std::format("mode=auto_fallback resolved_key={} semantic_failure={}",
                                  resolved_interaction_key_, failure));
        return true;
    }

    [[nodiscard]] bool refresh_interaction_binding() {
        if (configuration_result_.value.interaction_key != "AUTO") {
            resolved_interaction_key_ = configuration_result_.value.interaction_key;
            interaction_binding_mode_ = "manual_key";
            logger_.write(dsnap::LogAudience::User, "INTERACTION_BINDING_RESOLVED",
                          std::format("mode=manual configured_key={} resolved_key={}",
                                      configuration_result_.value.interaction_key,
                                      resolved_interaction_key_));
            return true;
        }

        try {
            const auto path = game_input_settings_path();
            const auto started = Clock::now();
            const auto binding = dsnap::load_game_interaction_binding(path);
            const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - started).count();
            if (!binding.valid() || !binding.value.keyboard_key) {
                return use_interaction_binding_fallback(
                    std::format("source={} elapsed_us={} errors={}", path.string(), elapsed_us,
                                join_errors(binding.errors)));
            }

            resolved_interaction_key_ = *binding.value.keyboard_key;
            interaction_binding_mode_ = "auto_saved_binding";
            logger_.write(dsnap::LogAudience::User, "INTERACTION_BINDING_RESOLVED",
                          std::format("mode=auto_saved_binding source={} action=INTERACT action_type=91 "
                                      "keyboard_code={} keyboard_key={} gamepad_code={} gamepad_key={} "
                                      "elapsed_us={} reload=each_enable",
                                      path.string(), binding.value.keyboard_key_code,
                                      *binding.value.keyboard_key, binding.value.gamepad_key_code,
                                      binding.value.gamepad_key.value_or("NONE"), elapsed_us));
            return true;
        } catch (const std::exception& error) {
            return use_interaction_binding_fallback(
                std::format("exception={}", error.what()));
        } catch (...) {
            return use_interaction_binding_fallback("exception=unknown");
        }
    }

    [[nodiscard]] bool resolve_live_interaction_action(const PlayerContext& context,
                                                       EnhancedActionResolution* output,
                                                       const char** reason) const {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!output || !context.controller) return reject("enhanced_input_context_unavailable");

        EnhancedActionResolution resolved{};
        resolved.player_input = named_object(context.controller, STR("PlayerInput"));
        if (!resolved.player_input || !resolved.player_input->IsA(enhanced_player_input_class_)) {
            return reject("enhanced_player_input_unavailable");
        }

        resolved.mappings_property = CastField<FArrayProperty>(
            resolved.player_input->GetPropertyByNameInChain(STR("EnhancedActionMappings")));
        if (!resolved.mappings_property) {
            *output = resolved;
            return reject("enhanced_action_mappings_array_property_unavailable");
        }
        auto* player_input_class = resolved.player_input->GetClassPrivate();
        const auto player_input_size = player_input_class ? player_input_class->GetPropertiesSize() : 0;
        if (player_input_size <= 0 ||
            !property_range_fits(resolved.mappings_property,
                                 static_cast<std::size_t>(player_input_size), sizeof(FScriptArray))) {
            *output = resolved;
            return reject("enhanced_action_mappings_array_property_malformed");
        }

        resolved.mapping_property = CastField<FStructProperty>(resolved.mappings_property->GetInner());
        resolved.mapping_struct = resolved.mapping_property
            ? resolved.mapping_property->GetStruct()
            : nullptr;
        if (!resolved.mapping_property || !resolved.mapping_struct ||
            resolved.mapping_struct != enhanced_action_mapping_struct_) {
            *output = resolved;
            return reject("enhanced_action_mapping_struct_mismatch");
        }
        resolved.mapping_struct_size = resolved.mapping_struct->GetStructureSize();
        resolved.mapping_alignment = resolved.mapping_property->GetMinAlignment();
        if (resolved.mapping_property->GetArrayDim() != 1 ||
            resolved.mapping_struct_size <= 0 ||
            resolved.mapping_struct_size > kMaxReflectedMappingStructSize ||
            resolved.mapping_property->GetElementSize() != resolved.mapping_struct_size ||
            resolved.mapping_alignment <= 0 ||
            !std::has_single_bit(static_cast<std::uint32_t>(resolved.mapping_alignment)) ||
            resolved.mapping_struct_size % resolved.mapping_alignment != 0) {
            *output = resolved;
            return reject("enhanced_action_mapping_struct_size_malformed");
        }

        const FName action_name{STR("Action"), FNAME_Find};
        const FName key_name{STR("Key"), FNAME_Find};
        const FName ignored_name{STR("bShouldBeIgnored"), FNAME_Find};
        const FName key_name_name{STR("KeyName"), FNAME_Find};
        if (!action_name || !key_name || !ignored_name || !key_name_name) {
            *output = resolved;
            return reject("enhanced_action_mapping_field_names_unavailable");
        }
        resolved.action_property = CastField<FObjectPropertyBase>(
            resolved.mapping_struct->FindProperty(action_name));
        resolved.key_property = CastField<FStructProperty>(resolved.mapping_struct->FindProperty(key_name));
        resolved.ignored_property = CastField<FBoolProperty>(resolved.mapping_struct->FindProperty(ignored_name));
        auto* reflected_key_struct = resolved.key_property
            ? resolved.key_property->GetStruct()
            : nullptr;
        resolved.key_name_property = reflected_key_struct
            ? CastField<FNameProperty>(reflected_key_struct->FindProperty(key_name_name))
            : nullptr;
        if (!resolved.action_property || !resolved.key_property || !resolved.ignored_property ||
            !resolved.key_name_property || reflected_key_struct != key_struct_) {
            *output = resolved;
            return reject("enhanced_action_mapping_field_metadata_mismatch");
        }

        auto* action_property_class = resolved.action_property->GetPropertyClass();
        const auto key_struct_size = reflected_key_struct->GetStructureSize();
        const auto key_struct_alignment = resolved.key_property->GetMinAlignment();
        const auto key_name_alignment = resolved.key_name_property->GetMinAlignment();
        resolved.action_property_offset = resolved.action_property->GetOffset_Internal();
        resolved.key_property_offset = resolved.key_property->GetOffset_Internal();
        resolved.ignored_property_offset = resolved.ignored_property->GetOffset_Internal();
        resolved.ignored_byte_offset = resolved.ignored_property->GetByteOffset();
        resolved.ignored_byte_mask = resolved.ignored_property->GetByteMask();
        resolved.ignored_field_mask = resolved.ignored_property->GetFieldMask();
        if (!action_property_class || !action_property_class->IsChildOf(input_action_class_) ||
            key_struct_size <= 0 || resolved.key_property->GetElementSize() != key_struct_size ||
            key_struct_alignment <= 0 ||
            !std::has_single_bit(static_cast<std::uint32_t>(key_struct_alignment)) ||
            key_struct_size % key_struct_alignment != 0 ||
            key_name_alignment <= 0 ||
            !std::has_single_bit(static_cast<std::uint32_t>(key_name_alignment)) ||
            !property_range_fits(resolved.action_property,
                                 static_cast<std::size_t>(resolved.mapping_struct_size), sizeof(UObject*)) ||
            !property_range_fits(resolved.key_property,
                                 static_cast<std::size_t>(resolved.mapping_struct_size),
                                 static_cast<std::size_t>(key_struct_size)) ||
            !property_range_fits(resolved.ignored_property,
                                 static_cast<std::size_t>(resolved.mapping_struct_size), sizeof(std::uint8_t)) ||
            !property_range_fits(resolved.key_name_property,
                                 static_cast<std::size_t>(key_struct_size), sizeof(FName)) ||
            resolved.ignored_byte_offset >= resolved.ignored_property->GetElementSize() ||
            resolved.ignored_byte_mask == 0 || resolved.ignored_field_mask == 0 ||
            resolved.action_property_offset % static_cast<std::int32_t>(alignof(UObject*)) != 0 ||
            resolved.key_property_offset % key_struct_alignment != 0 ||
            resolved.key_name_property->GetOffset_Internal() % key_name_alignment != 0) {
            *output = resolved;
            return reject("enhanced_action_mapping_field_layout_malformed");
        }

        const auto* mappings = resolved.player_input->
            GetValuePtrByPropertyNameInChain<FScriptArray>(STR("EnhancedActionMappings"));
        if (!mappings) {
            *output = resolved;
            return reject("enhanced_action_mappings_array_value_unavailable");
        }
        resolved.mapping_count = mappings->Num();
        if (resolved.mapping_count < 0 || resolved.mapping_count > kMaxEnhancedActionMappings) {
            *output = resolved;
            return reject("enhanced_action_mappings_bounds_rejected");
        }
        if (resolved.mapping_count == 0) {
            *output = resolved;
            return reject("enhanced_action_mappings_empty");
        }

        resolved.automatic_mode = configuration_result_.value.interaction_key == "AUTO";
        if (resolved_interaction_key_.empty()) {
            *output = resolved;
            return reject("interaction_binding_not_resolved");
        }
        for (std::int32_t index = 0; index < resolved.mapping_count; ++index) {
            if (!mappings->IsValidIndex(index)) {
                *output = resolved;
                return reject("enhanced_action_mapping_index_invalid");
            }
            const auto* entry = reinterpret_cast<const std::byte*>(mappings->GetData()) +
                static_cast<std::size_t>(index) * static_cast<std::size_t>(resolved.mapping_struct_size);
            if (!entry || reinterpret_cast<std::uintptr_t>(entry) %
                    static_cast<std::uintptr_t>(resolved.mapping_alignment) != 0) {
                *output = resolved;
                return reject("enhanced_action_mapping_element_alignment_rejected");
            }
            const auto* key_container = resolved.key_property->ContainerPtrToValuePtr<void>(entry);
            const auto& mapped_key_name = resolved.key_name_property->GetPropertyValueInContainer(key_container);
            const auto* action_address = resolved.action_property->ContainerPtrToValuePtr<void>(entry);
            auto* action = resolved.action_property->GetObjectPropertyValue(action_address);
            if (!action || !action->IsA(input_action_class_)) continue;

            const auto mapped_key = to_string(mapped_key_name.ToString());
            const bool mapping_matches = mapped_key == resolved_interaction_key_;
            if (!mapping_matches) continue;

            const bool ignored = resolved.ignored_property->GetPropertyValueInContainer(entry);
            const auto* raw_flag_address = entry + resolved.ignored_property_offset +
                resolved.ignored_byte_offset;
            ++resolved.matched_mappings;
            if (resolved.matched_mappings == 1) {
                resolved.raw_ignored_flag_byte = std::to_integer<std::uint8_t>(*raw_flag_address);
            }
            if (ignored) {
                ++resolved.ignored_mappings;
                continue;
            }
            ++resolved.active_mappings;
            if (!resolved.interaction_action) {
                resolved.interaction_action = action;
            } else if (resolved.interaction_action != action) {
                resolved.conflicting_actions = true;
            }
            if (!mapped_key.empty() && resolved.active_binding_keys.size() < 192) {
                if (!resolved.active_binding_keys.empty()) resolved.active_binding_keys += ',';
                resolved.active_binding_keys += mapped_key;
            }
        }

        *output = resolved;
        if (resolved.matched_mappings == 0) {
            return reject(resolved.automatic_mode
                ? "interaction_action_mapping_missing"
                : "configured_interaction_key_mapping_missing");
        }
        if (resolved.active_mappings == 0) return reject("interaction_action_mapping_inactive");
        if (resolved.conflicting_actions) return reject("interaction_action_mapping_ambiguous");
        if (!resolved.interaction_action || !resolved.interaction_action->IsA(input_action_class_)) {
            return reject("live_interaction_input_action_invalid");
        }
        if (reason) {
            *reason = resolved.automatic_mode
                ? "live_interaction_action_auto_resolved"
                : "live_interaction_action_manual_key_resolved";
        }
        return true;
    }

    [[nodiscard]] UObject* resolve_enhanced_input_subsystem(UObject* controller,
                                                            const char** reason) const {
        const auto reject = [reason](const char* value) -> UObject* {
            if (reason) *reason = value;
            return nullptr;
        };
        if (!controller || !subsystem_library_cdo_ || !get_local_player_subsystem_function_ ||
            !enhanced_input_subsystem_class_) {
            return reject("enhanced_input_subsystem_contract_unavailable");
        }

        std::vector<std::byte> parameters(get_local_player_subsystem_function_->GetParmsSize());
        if (!write_function_parameter(get_local_player_subsystem_function_, parameters,
                                      STR("PlayerController"), controller) ||
            !write_function_parameter(get_local_player_subsystem_function_, parameters,
                                      STR("Class"), enhanced_input_subsystem_class_)) {
            return reject("get_subsystem_parameter_layout_rejected");
        }
        subsystem_library_cdo_->ProcessEvent(get_local_player_subsystem_function_, parameters.data());
        auto* return_property = get_local_player_subsystem_function_->GetReturnProperty();
        if (!property_range_fits(return_property, parameters.size(), sizeof(UObject*))) {
            return reject("get_subsystem_return_layout_rejected");
        }
        UObject* subsystem{};
        std::memcpy(&subsystem, parameters.data() + return_property->GetOffset_Internal(), sizeof(subsystem));
        if (!subsystem || !subsystem->IsA(enhanced_input_subsystem_class_)) {
            return reject("enhanced_input_subsystem_unavailable");
        }
        if (reason) *reason = "enhanced_input_subsystem_resolved";
        return subsystem;
    }

    [[nodiscard]] bool inject_live_interaction_action_once(UObject* subsystem,
                                                           UObject* action,
                                                           const char** reason) const {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!subsystem || !subsystem->IsA(enhanced_input_subsystem_class_)) {
            return reject("enhanced_input_subsystem_invalid");
        }
        if (!action || !action->IsA(input_action_class_)) {
            return reject("live_interaction_input_action_invalid");
        }

        std::vector<std::byte> parameters(inject_input_vector_function_->GetParmsSize());
        const FVector pressed_value{1.0, 0.0, 0.0};
        if (!write_function_parameter(inject_input_vector_function_, parameters, STR("Action"), action) ||
            !write_function_parameter(inject_input_vector_function_, parameters, STR("Value"), pressed_value)) {
            return reject("inject_input_parameter_layout_rejected");
        }
        subsystem->ProcessEvent(inject_input_vector_function_, parameters.data());
        if (reason) *reason = "live_interaction_action_injected";
        return true;
    }

    void engine_tick_post(UEngine* engine) noexcept {
        const auto now = Clock::now();
        if (next_pulse_due_ != Clock::time_point{} && now < next_pulse_due_) return;
        if (configuration_result_.value.debug_logging && last_pulse_at_ != Clock::time_point{}) {
            const auto gap_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now - last_pulse_at_).count();
            pulse_gap_timing_.record(gap_us > 0 ? static_cast<std::uint64_t>(gap_us) : 0);
        }
        last_pulse_at_ = now;
        ScopeTiming pulse_work{configuration_result_.value.debug_logging ? &pulse_work_timing_ : nullptr};
        next_pulse_due_ = now + kPulseInterval;

        poll_confirmation(now);

        const auto toggle_requests = pending_toggle_requests_.exchange(0, std::memory_order_acq_rel);
        if ((toggle_requests & 1ULL) != 0) {
            const bool enable = !automation_enabled_.load(std::memory_order_acquire);
            ++toggle_transitions_;
            if (enable) {
                const bool allowed = build_trusted_.load(std::memory_order_acquire) &&
                                     configuration_result_.value.automatic_pickup;
                if (!allowed) {
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  "reason=untrusted_or_disabled source=configured_hotkey_toggle");
                } else if (!refresh_interaction_binding()) {
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  "reason=interaction_binding_unavailable source=configured_hotkey_toggle");
                } else {
                    cancel_window("toggle_on_reset");
                    clear_confirmation();
                    automation_enabled_.store(true, std::memory_order_release);
                    next_auto_scan_due_ = now < world_settle_until_ ? world_settle_until_ : now;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_ENABLED",
                                  std::format("source=configured_hotkey_toggle hotkey={} interaction_range=game_or_optional_pak "
                                              "mounted_rider_route=1 "
                                              "active_scan_ms={} idle_scan_ms={} post_pickup_ms={} "
                                              "world_settle_remaining_ms={}",
                                              configuration_result_.value.toggle_hotkey,
                                              kActiveScanInterval.count(), kIdleScanInterval.count(),
                                              kPostPickupCooldown.count(),
                                              now < world_settle_until_
                                                  ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        world_settle_until_ - now).count()
                                                  : 0));
                }
            } else {
                automation_enabled_.store(false, std::memory_order_release);
                cancel_window("toggle_off");
                clear_confirmation();
                next_auto_scan_due_ = {};
                logger_.write(dsnap::LogAudience::User, "AUTOMATION_DISABLED",
                              std::format("reason=configured_hotkey_toggle hotkey={} pending_action_state=cleared",
                                          configuration_result_.value.toggle_hotkey));
            }
        }

        if (!automation_enabled_.load(std::memory_order_acquire)) return;
        if (now < world_settle_until_ || window_active_.load(std::memory_order_acquire)) {
            return;
        }
        if (next_auto_scan_due_ != Clock::time_point{} && now < next_auto_scan_due_) return;

        const auto foreground = foreground_window_state();
        if (!foreground.matched) {
            ++foreground_pauses_;
            next_auto_scan_due_ = now + kTransientBackoff;
            return;
        }

        const auto request_id = next_request_id_.fetch_add(1, std::memory_order_relaxed) + 1;
        ++automatic_scans_;
        start_window(request_id, now);

        ++window_samples_;
        const auto attempt_started = Clock::now();
        const char* reason{"unknown"};
        bool retry_no_candidate{};
        bool selector_called{};
        bool selector_faulted{};
        bool runtime_faulted{};
        StageTimings stage_timings{};
        const bool invoked = invoke_pickup_guarded(this, engine, active_request_id_, true,
                                                   &reason, &retry_no_candidate, &selector_called,
                                                   &selector_faulted, &runtime_faulted,
                                                   configuration_result_.value.debug_logging
                                                       ? &stage_timings : nullptr);
        const auto attempt_finished = Clock::now();
        const auto attempt_us = std::chrono::duration_cast<std::chrono::microseconds>(
            attempt_finished - attempt_started).count();
        if (configuration_result_.value.debug_logging) {
            const auto bounded_attempt_us = attempt_us > 0 ? static_cast<std::uint64_t>(attempt_us) : 0;
            scan_timing_.record(bounded_attempt_us);
            if (stage_timings.context_us != 0) context_timing_.record(stage_timings.context_us);
            if (stage_timings.selector_us != 0) selector_timing_.record(stage_timings.selector_us);
            if (stage_timings.validation_us != 0) validation_timing_.record(stage_timings.validation_us);
            if (stage_timings.action_resolution_us != 0) {
                action_resolution_timing_.record(stage_timings.action_resolution_us);
            }
            if (stage_timings.subsystem_us != 0) subsystem_timing_.record(stage_timings.subsystem_us);
            if (stage_timings.injection_us != 0) injection_timing_.record(stage_timings.injection_us);
            if (bounded_attempt_us >=
                static_cast<std::uint64_t>(configuration_result_.value.slow_scan_threshold_us)) {
                slow_scan_count_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        if (selector_called) {
            ++window_selector_calls_;
            ++selector_attempts_;
        }
        if (selector_faulted) ++selector_faults_;

        if (invoked) {
            const auto completed_request_id = active_request_id_;
            close_window_state();
            ++windows_hit_;
            next_auto_scan_due_ = attempt_finished + kActiveScanInterval;
            logger_.write(dsnap::LogAudience::User, "PICKUP_ACTION_SENT",
                          std::format("request_id={} attempt_us={} confirmation=asynchronous "
                                      "next_target_scan_ms={}",
                                      completed_request_id, attempt_us, kActiveScanInterval.count()));
            return;
        }

        close_window_state();
        if (retry_no_candidate) {
            ++selector_no_candidate_;
            next_auto_scan_due_ = attempt_finished + kIdleScanInterval;
            return;
        }

        ++windows_cancelled_;
        ++action_failures_;
        const std::string_view failure_reason{reason ? reason : "unknown"};
        const bool hard_fault = selector_faulted || runtime_faulted ||
            failure_reason == "native_selector_fault" ||
            failure_reason == "guarded_runtime_fault";
        if (hard_fault) {
            automation_enabled_.store(false, std::memory_order_release);
            ++automatic_disables_;
            next_auto_scan_due_ = {};
            logger_.write(dsnap::LogAudience::User, "AUTOMATION_DISABLED",
                          std::format("reason={} selector_faulted={} runtime_faulted={} "
                                      "fail_closed=1 retry_hotkey={}",
                                      failure_reason, selector_faulted, runtime_faulted,
                                      configuration_result_.value.toggle_hotkey));
            return;
        }

        ++transient_failures_;
        const auto backoff = failure_reason == "player_context_unavailable" ||
                             failure_reason == "pawn_world_unavailable" ||
                             failure_reason == "receiver_not_interactable_component" ||
                             failure_reason == "receiver_world_mismatch" ||
                             failure_reason == "interaction_owner_world_mismatch"
            ? kTransientBackoff
            : kFaultBackoff;
        next_auto_scan_due_ = attempt_finished + backoff;
        logger_.write(dsnap::LogAudience::Debug, "AUTO_SCAN_DEFERRED",
                      std::format("request_id={} reason={} attempt_us={} backoff_ms={} "
                                  "selector_called={} selector_faulted={} runtime_faulted={}",
                                  request_id, failure_reason, attempt_us, backoff.count(),
                                  selector_called, selector_faulted, runtime_faulted));
    }

    bool invoke_pickup_unsafe(UEngine* engine,
                              std::uint64_t request_id,
                              bool first_sample,
                              const char** reason,
                              bool* retry_no_candidate,
                              bool* selector_called,
                              bool* selector_faulted,
                              StageTimings* timing) {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (retry_no_candidate) *retry_no_candidate = false;
        if (selector_called) *selector_called = false;
        if (selector_faulted) *selector_faulted = false;
        if (timing) *timing = {};
        PlayerContext context{};
        const char* receiver_reason{"unknown"};
        {
            StageTimingScope context_scope{timing ? &timing->context_us : nullptr};
            const bool context_resolved = resolve_player_context(engine, &context);
            if (first_sample) {
                const auto pawn_identity = context.pawn
                    ? pack_weak_identity(make_weak(context.pawn))
                    : 0;
                const auto receiver_identity = context.interaction_receiver
                    ? pack_weak_identity(make_weak(context.interaction_receiver))
                    : 0;
                if (pawn_identity != last_pawn_identity_ || receiver_identity != last_receiver_identity_) {
                    if (configuration_result_.value.debug_logging) {
                        logger_.write(dsnap::LogAudience::Debug, "PLAYER_CONTEXT_CHANGED",
                                      std::format("request_id={} resolved={} pawn=0x{:X} pawn_name={} pawn_class={} "
                                                  "interaction_owner=0x{:X} interaction_owner_name={} mounted={} "
                                                  "receiver=0x{:X} receiver_source={} receiver_name={}",
                                                  request_id, context_resolved, pawn_identity,
                                                  object_full_name(context.pawn),
                                                  object_full_name(context.pawn ? context.pawn->GetClassPrivate() : nullptr),
                                                  context.interaction_owner
                                                      ? pack_weak_identity(make_weak(context.interaction_owner))
                                                      : 0,
                                                  object_full_name(context.interaction_owner), context.mounted,
                                                  receiver_identity, context.receiver_source,
                                                  object_full_name(context.interaction_receiver)));
                    }
                    last_pawn_identity_ = pawn_identity;
                    last_receiver_identity_ = receiver_identity;
                }
            }
            if (!context_resolved) return reject("player_context_unavailable");
            if (!validate_receiver(context, &receiver_reason)) return reject(receiver_reason);
        }

        NativeSelectorPair selected{};
        bool local_selector_faulted{};
        {
            StageTimingScope selector_scope{timing ? &timing->selector_us : nullptr};
            if (selector_called) *selector_called = true;
            if (!call_native_selector_guarded(context.interaction_receiver, &selected, &local_selector_faulted)) {
                if (selector_faulted) *selector_faulted = local_selector_faulted;
                return reject(local_selector_faulted ? "native_selector_fault" : "native_selector_unavailable");
            }
        }
        if (!selected.actor && !selected.component) {
            if (retry_no_candidate) *retry_no_candidate = true;
            return reject("selector_no_candidate");
        }
        if (!selected.actor || !selected.component) return reject("selector_partial_pair");
        StageTimingScope validation_scope{timing ? &timing->validation_us : nullptr};
        const bool actor_is_drop_item = selected.actor->IsA(drop_item_class_);
        const bool component_is_interactable = selected.component->IsA(interactable_class_);
        auto* component_outer = selected.component->GetOuterPrivate();
        auto* observed_interactable = component_is_interactable
            ? selected.component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"))
            : nullptr;
        auto* observed_interact_type = component_is_interactable
            ? selected.component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"))
            : nullptr;
        const bool target_type_supported = observed_interact_type &&
            dsnap::is_supported_selector_target(*observed_interact_type, actor_is_drop_item);
        const auto observed_actor_identity = pack_weak_identity(make_weak(selected.actor));
        const auto observed_component_identity = pack_weak_identity(make_weak(selected.component));
        const auto observed_type_value = observed_interact_type
            ? static_cast<int>(*observed_interact_type) : -1;
        const auto observed_state_value = observed_interactable
            ? static_cast<int>(*observed_interactable) : -1;
        if (configuration_result_.value.debug_logging &&
            (observed_actor_identity != last_observed_actor_identity_ ||
             observed_component_identity != last_observed_component_identity_ ||
             observed_type_value != last_observed_interact_type_ ||
             observed_state_value != last_observed_interactable_ ||
             target_type_supported != last_observed_supported_)) {
            logger_.write(dsnap::LogAudience::Debug, "SELECTOR_PAIR_OBSERVED",
                          std::format("actor_name={} component_name={} component_outer={} receiver_name={} "
                                      "receiver_source={} actor_is_drop_item={} component_is_interactable={} "
                                      "relation_matches={} interactable={} interact_type={} "
                                      "target_type_supported={} target_field_access=0 deduplicated=1",
                                      object_full_name(selected.actor), object_full_name(selected.component),
                                      object_full_name(component_outer), object_full_name(context.interaction_receiver),
                                      context.receiver_source, actor_is_drop_item, component_is_interactable,
                                      component_outer == selected.actor, observed_state_value,
                                      observed_type_value, target_type_supported));
            last_observed_actor_identity_ = observed_actor_identity;
            last_observed_component_identity_ = observed_component_identity;
            last_observed_interact_type_ = observed_type_value;
            last_observed_interactable_ = observed_state_value;
            last_observed_supported_ = target_type_supported;
        }

        if (!component_is_interactable) return reject("selector_component_not_interactable");
        if (selected.component == context.interaction_receiver) return reject("selector_component_is_receiver");
        if (component_outer != selected.actor) return reject("selector_relation_mismatch");

        auto* world = context.pawn->GetWorld();
        if (!world || selected.actor->GetWorld() != world || selected.component->GetWorld() != world) {
            return reject("selector_world_mismatch");
        }

        auto* interactable = selected.component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"));
        auto* interact_type = selected.component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"));
        if (!interactable || !interact_type) return reject("selector_state_fields_missing");
        if (*interactable != kRequiredInteractableValue ||
            !dsnap::is_supported_selector_target(*interact_type, actor_is_drop_item)) {
            return reject("selector_state_mismatch");
        }

        const FWeakObjectPtr actor_weak = make_weak(selected.actor);
        const FWeakObjectPtr component_weak = make_weak(selected.component);
        const auto actor_identity = pack_weak_identity(actor_weak);
        const auto component_identity = pack_weak_identity(component_weak);
        const auto now = Clock::now();
        if (actor_identity == recent_target_identity_ && now < recent_target_until_) {
            if (retry_no_candidate) *retry_no_candidate = true;
            return reject("recent_target_cooldown");
        }
        const char* target_kind = *interact_type == dsnap::kNormalGatherInteractType
            ? "normal_gather"
            : (*interact_type == dsnap::kAnimalInteractType ? "animal_fish" : "drop_item");
        ++selector_pairs_;
        logger_.write(dsnap::LogAudience::User, "SELECTOR_PAIR_VALIDATED",
                      std::format("selector_rva=0x{:X} actor=0x{:X} actor_name={} component=0x{:X} "
                                  "receiver=0x{:X} receiver_source={} actor_is_drop_item={} "
                                  "interactable={} interact_type={} target_kind={} target_field_access=0",
                                  kNativeSelectorRva, actor_identity, object_full_name(selected.actor),
                                  component_identity, pack_weak_identity(make_weak(context.interaction_receiver)),
                                     context.receiver_source, actor_is_drop_item, *interactable, *interact_type,
                                     target_kind));
        validation_scope.finish();

        if (Clock::now() >= window_deadline_) return reject("window_budget_expired");

        StageTimingScope action_resolution_scope{
            timing ? &timing->action_resolution_us : nullptr};
        const auto foreground = foreground_window_state();
        logger_.write(dsnap::LogAudience::User, "FOREGROUND_VALIDATION",
                      std::format("window=0x{:X} window_class={} foreground_pid={} current_pid={} "
                                  "process_matched={} console_class={} matched={}",
                                  reinterpret_cast<std::uintptr_t>(foreground.window),
                                  foreground.window_class,
                                  foreground.foreground_process_id, foreground.current_process_id,
                                  foreground.process_matched, foreground.console_class,
                                  foreground.matched));
        if (!foreground.matched) return reject("game_not_foreground");

        EnhancedActionResolution action_resolution{};
        const char* action_resolution_reason{"unknown"};
        const bool action_resolved = resolve_live_interaction_action(context, &action_resolution,
                                                                     &action_resolution_reason);
        logger_.write(dsnap::LogAudience::User, "ENHANCED_INPUT_ACTION_RESOLUTION",
                      std::format("resolved={} reason={} player_input={} mapping_count={} "
                                   "mapping_struct_size={} mapping_alignment={} mode={} configured_key={} "
                                   "resolved_key={} fallback_key={} "
                                  "interaction_action={} action_ptr=0x{:X} active_binding_keys={} "
                                  "matched_mappings={} active_mappings={} ignored_mappings={} "
                                  "conflicting_actions={} "
                                  "raw_ignored_flag=0x{:02X} ignored_field_mask=0x{:02X} "
                                  "ignored_byte_mask=0x{:02X} ignored_byte_offset={} "
                                  "action_offset={} key_offset={} ignored_property_offset={}",
                                  action_resolved, action_resolution_reason,
                                  object_full_name(action_resolution.player_input),
                                  action_resolution.mapping_count,
                                  action_resolution.mapping_struct_size,
                                  action_resolution.mapping_alignment,
                                   interaction_binding_mode_,
                                   configuration_result_.value.interaction_key,
                                   resolved_interaction_key_,
                                   configuration_result_.value.interaction_key_fallback,
                                  object_full_name(action_resolution.interaction_action),
                                  reinterpret_cast<std::uintptr_t>(action_resolution.interaction_action),
                                  action_resolution.active_binding_keys,
                                  action_resolution.matched_mappings,
                                  action_resolution.active_mappings,
                                  action_resolution.ignored_mappings,
                                  action_resolution.conflicting_actions,
                                  static_cast<unsigned int>(action_resolution.raw_ignored_flag_byte),
                                  static_cast<unsigned int>(action_resolution.ignored_field_mask),
                                  static_cast<unsigned int>(action_resolution.ignored_byte_mask),
                                  static_cast<unsigned int>(action_resolution.ignored_byte_offset),
                                  action_resolution.action_property_offset,
                                  action_resolution.key_property_offset,
                                  action_resolution.ignored_property_offset));
        if (!action_resolved) return reject(action_resolution_reason);
        action_resolution_scope.finish();

        StageTimingScope subsystem_scope{timing ? &timing->subsystem_us : nullptr};
        const char* subsystem_reason{"unknown"};
        auto* enhanced_input_subsystem = resolve_enhanced_input_subsystem(context.controller,
                                                                          &subsystem_reason);
        logger_.write(dsnap::LogAudience::User, "ENHANCED_INPUT_SUBSYSTEM_RESOLUTION",
                      std::format("resolved={} reason={} subsystem={} function={}",
                                  enhanced_input_subsystem != nullptr, subsystem_reason,
                                  object_full_name(enhanced_input_subsystem),
                                  object_full_name(inject_input_vector_function_)));
        if (!enhanced_input_subsystem) return reject(subsystem_reason);
        subsystem_scope.finish();

        if (confirmation_pending_.load(std::memory_order_acquire) &&
            confirmation_actor_identity_ != actor_identity) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIRMATION_SUPERSEDED",
                          std::format("previous_actor=0x{:X} next_actor=0x{:X}",
                                      confirmation_actor_identity_, actor_identity));
            clear_confirmation();
        }
        begin_confirmation(actor_weak, component_weak, actor_identity, component_identity);
        ++enhanced_input_attempts_;
        StageTimingScope injection_scope{timing ? &timing->injection_us : nullptr};
        const char* injection_reason{"unknown"};
        if (!inject_live_interaction_action_once(enhanced_input_subsystem,
                                                action_resolution.interaction_action,
                                                &injection_reason)) {
            clear_confirmation();
            return reject(injection_reason);
        }
        injection_scope.finish();
        ++enhanced_input_injections_;

        recent_target_identity_ = actor_identity;
        recent_target_until_ = Clock::now() + kRecentTargetCooldown;
        logger_.write(dsnap::LogAudience::User, "PICKUP_ENHANCED_INPUT_INJECTED",
                      std::format("receiver=0x{:X} actor=0x{:X} "
                                   "component=0x{:X} interact_type={} target_kind={} "
                                   "action={} resolution_mode={} active_binding_keys={} value=1,0,0 one_shot=1 "
                                   "continuous_injection=0 target_field_access=0 direct_RPC=0 "
                                   "SendInput=0 confirmation=asynchronous",
                                   pack_weak_identity(make_weak(context.interaction_receiver)),
                                   actor_identity, component_identity, *interact_type, target_kind,
                                   object_full_name(action_resolution.interaction_action),
                                   interaction_binding_mode_,
                                   action_resolution.active_binding_keys));
        if (reason) *reason = "live_interaction_action_injected";
        return true;
    }

    static bool invoke_pickup_guarded(NativeAutoPickup* self,
                                      UEngine* engine,
                                      std::uint64_t request_id,
                                      bool first_sample,
                                      const char** reason,
                                      bool* retry_no_candidate,
                                      bool* selector_called,
                                      bool* selector_faulted,
                                      bool* runtime_faulted,
                                      StageTimings* timing) noexcept {
        if (runtime_faulted) *runtime_faulted = false;
        bool invoked{};
#if defined(_MSC_VER)
        __try {
            invoked = self->invoke_pickup_unsafe(engine, request_id, first_sample, reason,
                                                 retry_no_candidate, selector_called, selector_faulted,
                                                 timing);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            invoked = false;
            if (reason) *reason = "guarded_runtime_fault";
            if (retry_no_candidate) *retry_no_candidate = false;
            if (runtime_faulted) *runtime_faulted = true;
        }
#else
        invoked = self->invoke_pickup_unsafe(engine, request_id, first_sample, reason,
                                             retry_no_candidate, selector_called, selector_faulted,
                                             timing);
#endif
        return invoked;
    }

    [[nodiscard]] static bool call_native_selector_guarded(UObject* receiver,
                                                            NativeSelectorPair* output,
                                                            bool* faulted) noexcept {
        if (faulted) *faulted = false;
        if (!receiver || !output) return false;
        const auto module = GetModuleHandleW(nullptr);
        if (!module) return false;
        const auto address = reinterpret_cast<std::uintptr_t>(module) + kNativeSelectorRva;
        const auto selector = reinterpret_cast<NativeSelectorFunction>(address);
#if defined(_MSC_VER)
        __try {
#endif
            selector(receiver, output, 0);
            return true;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            if (faulted) *faulted = true;
            output->actor = nullptr;
            output->component = nullptr;
            return false;
        }
#endif
    }

    [[nodiscard]] static UObject* weak_get_guarded(const FWeakObjectPtr& weak, bool* faulted) noexcept {
        if (faulted) *faulted = false;
#if defined(_MSC_VER)
        __try {
#endif
            return weak.Get();
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            if (faulted) *faulted = true;
            return nullptr;
        }
#endif
    }

    void begin_confirmation(const FWeakObjectPtr& actor,
                            const FWeakObjectPtr& component,
                            std::uint64_t actor_identity,
                            std::uint64_t component_identity) noexcept {
        confirmation_actor_ = actor;
        confirmation_component_ = component;
        confirmation_actor_identity_ = actor_identity;
        confirmation_component_identity_ = component_identity;
        confirmation_deadline_ = Clock::now() + dsnap::kActionConfirmationWindow;
        confirmation_pending_.store(true, std::memory_order_release);
    }

    void poll_confirmation(Clock::time_point now) noexcept {
        if (!confirmation_pending_.load(std::memory_order_acquire)) return;
        bool actor_faulted{};
        auto* actor = weak_get_guarded(confirmation_actor_, &actor_faulted);
        if (actor_faulted) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIRMATION_ABORTED",
                          std::format("reason=weak_probe_fault actor=0x{:X}", confirmation_actor_identity_));
            clear_confirmation();
            next_auto_scan_due_ = now + kActiveScanInterval;
            return;
        }
        if (!actor) {
            ++confirmations_;
            logger_.write(dsnap::LogAudience::User, "PICKUP_CONFIRMED",
                          std::format("signal=exact_actor_weak_identity_invalidated actor=0x{:X} component=0x{:X} "
                                      "next_scan_ms={}",
                                      confirmation_actor_identity_, confirmation_component_identity_,
                                      kPostPickupCooldown.count()));
            clear_confirmation();
            next_auto_scan_due_ = now + kPostPickupCooldown;
            return;
        }
        if (now >= confirmation_deadline_) {
            ++confirmation_timeouts_;
            logger_.write(dsnap::LogAudience::User, "PICKUP_UNCONFIRMED",
                          std::format("reason=actor_still_live actor=0x{:X} component=0x{:X} "
                                      "scheduler_blocked=0",
                                      confirmation_actor_identity_, confirmation_component_identity_));
            clear_confirmation();
        }
    }

    void clear_confirmation() noexcept {
        confirmation_pending_.store(false, std::memory_order_release);
        confirmation_actor_ = nullptr;
        confirmation_component_ = nullptr;
        confirmation_actor_identity_ = 0;
        confirmation_component_identity_ = 0;
        confirmation_deadline_ = {};
    }

    void start_window(std::uint64_t request_id, Clock::time_point now) noexcept {
        active_request_id_ = request_id;
        window_samples_ = 0;
        window_selector_calls_ = 0;
        window_no_candidate_count_ = 0;
        window_started_at_ = now;
        window_deadline_ = now + kSelectorAttemptBudget;
        window_active_.store(true, std::memory_order_release);
        ++windows_started_;
        // Empty scans are counted by PERF_AGGREGATE; do not log at the 30 Hz scan rate.
    }

    void close_window_state() noexcept {
        active_request_id_ = 0;
        window_samples_ = 0;
        window_selector_calls_ = 0;
        window_no_candidate_count_ = 0;
        window_started_at_ = {};
        window_deadline_ = {};
        window_active_.store(false, std::memory_order_release);
    }

    void cancel_active_window(const char* reason, Clock::time_point now) noexcept {
        const auto request_id = active_request_id_;
        const auto samples = window_samples_;
        const auto selector_calls = window_selector_calls_;
        const auto no_candidate_count = window_no_candidate_count_;
        const auto elapsed_ms = window_started_at_ == Clock::time_point{} ? 0 :
            std::chrono::duration_cast<std::chrono::milliseconds>(now - window_started_at_).count();
        close_window_state();
        ++windows_cancelled_;
        logger_.write(dsnap::LogAudience::User, "WINDOW_CANCELLED",
                      std::format("request_id={} reason={} samples={} selector_calls={} "
                                  "no_candidate_count={} elapsed_ms={}",
                                  request_id, reason, samples, selector_calls,
                                  no_candidate_count, elapsed_ms));
    }

    void cancel_window(const char* reason) noexcept {
        if (window_active_.load(std::memory_order_acquire)) {
            cancel_active_window(reason, Clock::now());
        }
    }

    void reset_world(const char* source) noexcept {
        pending_key_events_.store(0, std::memory_order_release);
        pending_toggle_requests_.store(0, std::memory_order_release);
        cancel_window(source);
        clear_confirmation();
        last_pawn_identity_ = 0;
        last_receiver_identity_ = 0;
        last_observed_actor_identity_ = 0;
        last_observed_component_identity_ = 0;
        last_observed_interact_type_ = -2;
        last_observed_interactable_ = -2;
        last_observed_supported_ = false;
        recent_target_identity_ = 0;
        recent_target_until_ = {};
        const auto now = Clock::now();
        world_settle_until_ = now + kWorldSettleDelay;
        next_auto_scan_due_ = automation_enabled_.load(std::memory_order_acquire)
            ? world_settle_until_
            : Clock::time_point{};
        ++world_resets_;
        logger_.write(dsnap::LogAudience::User, "WORLD_RESET",
                      std::format("reason={} selector_state=cleared confirmation=cleared "
                                  "automation_preserved={} settle_ms={} cross_world_object_cache=0",
                                  source, automation_enabled_.load(std::memory_order_acquire),
                                  kWorldSettleDelay.count()));
    }

    void unregister_callbacks() noexcept {
        // Official UE4SS v3.0.1 exposes append-only native callback vectors.
        // The module is pinned for process lifetime and callbacks consult the
        // generation-gated static instance before touching this object.
        process_event_callback_registered_.store(false, std::memory_order_release);
        game_thread_pulse_requested_.store(false, std::memory_order_release);
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};

    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    std::string resolved_interaction_key_{};
    std::string interaction_binding_mode_{"unresolved"};
    dsnap::AsyncLogger logger_;
    dsnap::FingerprintResult fingerprint_result_{};
    ProcessShutdownProbe shutdown_probe_{};

    std::atomic<bool> shutting_down_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> process_event_callback_registered_{};
    std::atomic<bool> game_thread_pulse_requested_{};
    std::atomic<std::uint64_t> pending_key_events_{};
    std::atomic<std::uint64_t> pending_toggle_requests_{};
    std::atomic<bool> automation_enabled_{};
    std::atomic<bool> window_active_{};
    std::atomic<std::uint64_t> next_request_id_{};
    bool fingerprint_applied_{};
    bool reflection_initialized_{};
    bool reflection_failed_{};
    std::atomic<bool> confirmation_pending_{};

    UClass* drop_item_class_{};
    UClass* interactable_class_{};
    UClass* engine_class_{};
    UClass* enhanced_player_input_class_{};
    UClass* input_action_class_{};
    UScriptStruct* enhanced_action_mapping_struct_{};
    UScriptStruct* key_struct_{};
    UClass* enhanced_input_subsystem_class_{};
    UClass* subsystem_library_class_{};
    UObject* subsystem_library_cdo_{};
    UFunction* get_local_player_subsystem_function_{};
    UFunction* inject_input_vector_function_{};

    FWeakObjectPtr confirmation_actor_{};
    FWeakObjectPtr confirmation_component_{};
    std::uint64_t confirmation_actor_identity_{};
    std::uint64_t confirmation_component_identity_{};
    std::uint64_t last_pawn_identity_{};
    std::uint64_t last_receiver_identity_{};
    std::uint64_t last_observed_actor_identity_{};
    std::uint64_t last_observed_component_identity_{};
    int last_observed_interact_type_{-2};
    int last_observed_interactable_{-2};
    bool last_observed_supported_{};

    std::uint64_t recent_target_identity_{};
    std::uintptr_t last_world_identity_{};
    std::uint8_t toggle_virtual_key_{};
    bool toggle_key_was_down_{};
    Clock::time_point last_key_event_accepted_{};
    Clock::time_point next_game_thread_request_due_{};
    Clock::time_point last_pulse_at_{};
    Clock::time_point next_pulse_due_{};
    Clock::time_point next_perf_log_{};
    Clock::time_point next_auto_scan_due_{};
    Clock::time_point world_settle_until_{};
    Clock::time_point confirmation_deadline_{};
    Clock::time_point window_started_at_{};
    Clock::time_point recent_target_until_{};
    Clock::time_point window_deadline_{};

    std::uint64_t active_request_id_{};
    std::uint32_t window_samples_{};
    std::uint32_t window_selector_calls_{};
    std::uint32_t window_no_candidate_count_{};

    std::atomic<std::uint64_t> key_events_received_{};
    std::atomic<std::uint64_t> key_events_coalesced_{};
    std::atomic<std::uint64_t> key_events_rejected_{};
    std::atomic<std::uint64_t> toggle_requests_accepted_{};
    std::atomic<std::uint64_t> toggle_transitions_{};
    std::atomic<std::uint64_t> automatic_scans_{};
    std::atomic<std::uint64_t> foreground_pauses_{};
    std::atomic<std::uint64_t> automatic_disables_{};
    std::atomic<std::uint64_t> windows_started_{};
    std::atomic<std::uint64_t> windows_hit_{};
    std::atomic<std::uint64_t> windows_cancelled_{};
    std::atomic<std::uint64_t> selector_attempts_{};
    std::atomic<std::uint64_t> selector_no_candidate_{};
    std::atomic<std::uint64_t> selector_pairs_{};
    std::atomic<std::uint64_t> enhanced_input_attempts_{};
    std::atomic<std::uint64_t> enhanced_input_injections_{};
    std::atomic<std::uint64_t> action_failures_{};
    std::atomic<std::uint64_t> transient_failures_{};
    std::atomic<std::uint64_t> confirmations_{};
    std::atomic<std::uint64_t> confirmation_timeouts_{};
    std::atomic<std::uint64_t> selector_faults_{};
    std::atomic<std::uint64_t> world_resets_{};
    std::atomic<std::uint64_t> slow_scan_count_{};
    AtomicTiming pulse_gap_timing_{};
    AtomicTiming pulse_work_timing_{};
    AtomicTiming scan_timing_{};
    AtomicTiming context_timing_{};
    AtomicTiming selector_timing_{};
    AtomicTiming validation_timing_{};
    AtomicTiming action_resolution_timing_{};
    AtomicTiming subsystem_timing_{};
    AtomicTiming injection_timing_{};
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
    // UE4SS v3.0.1 cannot remove native ProcessEvent callbacks. The DLL is
    // process-pinned and the small mod object is intentionally retained after
    // invalidating its generation, so an in-flight callback can never race a
    // delete or execute unloaded code.
    native_mod->prepare_for_abandoned_host_unload();
}
}
