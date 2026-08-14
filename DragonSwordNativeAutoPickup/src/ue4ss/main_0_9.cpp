// OWNER_AUTHORIZED_NATIVE_VISIBILITY_F_INPUT targeting pinned RE-UE4SS v3.0.1.
//
// Version 0.9.0 detours the game's native (non-ProcessEvent) ground-loot
// visibility function. A newly active component queues exactly one synthetic F
// key press/release for the next EngineTick, allowing the game to execute its
// complete ordinary interaction path.

#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/windows_fingerprint.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Mod/CppUserModBase.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Unreal/Hooks.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <polyhook2/Detour/x64Detour.hpp>
#pragma warning(pop)

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <string>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using ProcessShutdownProbe = BOOLEAN(NTAPI*)();
using NativeVisibilityFunction = void(__fastcall*)(UObject*, UObject*, bool);

extern "C" IMAGE_DOS_HEADER __ImageBase;

constexpr auto kVersion = STR("0.9.0-native-visibility-f-input");
constexpr auto kLabel = "OWNER_AUTHORIZED_NATIVE_VISIBILITY_F_INPUT";
constexpr auto kCurrentGameSha256 = "3DDDCEE474825310000A4CD24239AE5C8B76EF81BAC223C9F3D52565816A0CEA";
constexpr std::uintptr_t kNativeVisibilityRva = 0x61B3AC0;
constexpr std::array<std::uint8_t, 16> kNativeVisibilityPrefix{
    0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83,
    0xEC, 0x20, 0x48, 0x8B, 0x01, 0x48, 0x8B, 0xF2,
};
constexpr auto kF9ReleaseQualification = std::chrono::milliseconds{250};
constexpr auto kInputCooldown = std::chrono::milliseconds{250};

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

[[nodiscard]] bool signature_matches_guarded(const std::uint8_t* address) noexcept {
    if (!address) return false;
#if defined(_MSC_VER)
    __try {
#endif
        for (std::size_t index = 0; index < kNativeVisibilityPrefix.size(); ++index) {
            if (address[index] != kNativeVisibilityPrefix[index]) return false;
        }
        return true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#endif
}

[[nodiscard]] bool game_window_is_foreground() noexcept {
    const auto foreground = GetForegroundWindow();
    if (!foreground) return false;
    DWORD process_id{};
    static_cast<void>(GetWindowThreadProcessId(foreground, &process_id));
    return process_id == GetCurrentProcessId();
}

[[nodiscard]] bool send_f_press_release() noexcept {
    const auto scan_code = static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>('F'), MAPVK_VK_TO_VSC));
    if (scan_code == 0) return false;
    std::array<INPUT, 2> inputs{};
    for (auto& input : inputs) {
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = scan_code;
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
    }
    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) == inputs.size();
}

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
        ModDescription = STR("Native ground-loot visibility to one-shot F input");
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
            uninstall_native_detour();
            unregister_callbacks();
            static_cast<void>(logger_.flush_all());
        } else {
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

        fingerprint_result_ = dsnap::verify_build_fingerprint(binary_directory());
        const bool exact_game = fingerprint_result_.game_sha256 == kCurrentGameSha256;
        build_trusted_.store(fingerprint_result_.trusted && exact_game, std::memory_order_release);
        logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                      std::format("trusted={} exact_native_layout={} game={} ue4ss={} error={}",
                                  fingerprint_result_.trusted, exact_game, fingerprint_result_.game_sha256,
                                  fingerprint_result_.ue4ss_sha256, fingerprint_result_.error));
        if (!build_trusted_.load(std::memory_order_acquire)) {
            logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY", "unknown native function layout");
            return;
        }

        if (!install_native_detour()) {
            build_trusted_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::User, "DISABLED", "native visibility detour installation failed");
            return;
        }

        const auto generation = callback_gate_.generation();
        engine_tick_callback_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine*, float, bool) {
                if (auto* self = current_instance(generation)) self->engine_tick_post();
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("NativeVisibilityInputPulse")});
        world_reset_callback_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});

        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID) {
            uninstall_native_detour();
            unregister_callbacks();
            build_trusted_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required native callback registration failed");
            return;
        }

        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey=F9 native_rva=0x{:X} input=F_scan_code "
                                  "object_scans=0 pawn_queries=0 direct_rpc=0",
                                  kLabel, kNativeVisibilityRva));
        logger_.write(dsnap::LogAudience::User, "AUTO_PICKUP_AVAILABLE", "press F9 once to enable");
    }

    void on_update() override {
        static_cast<void>(logger_.flush());
        if (shutting_down_.load(std::memory_order_acquire)) return;

        const auto now = Clock::now();
        const bool f9_down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (f9_input_.sample(f9_down, now)) toggle_requested_.store(true, std::memory_order_release);

        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                          std::format("native_visibility_events={} active_events={} hidden_events={} "
                                      "input_requests={} input_sent={} input_rejected_foreground={} "
                                      "input_rejected_cooldown={} input_failures={} world_resets={} "
                                      "active={} target_visible={} pending={} object_scans=0",
                                      native_visibility_events_.load(), active_events_.load(), hidden_events_.load(),
                                      input_requests_.load(), input_sent_.load(), input_rejected_foreground_.load(),
                                      input_rejected_cooldown_.load(), input_failures_.load(), world_resets_.load(),
                                      active_.load(), target_visible_.load(), input_pending_.load()));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

private:
    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    static void __fastcall native_visibility_detour(UObject* widget, UObject* component, bool visible) noexcept {
        const auto trampoline_value = native_visibility_trampoline_.load(std::memory_order_acquire);
        if (trampoline_value != 0) {
            const auto original = std::bit_cast<NativeVisibilityFunction>(trampoline_value);
            original(widget, component, visible);
        }
        auto* self = instance_.load(std::memory_order_acquire);
        if (self && !self->shutting_down_.load(std::memory_order_acquire)) {
            self->record_native_visibility(component, visible);
        }
    }

    void record_native_visibility(UObject* component, bool visible) noexcept {
        ++native_visibility_events_;
        const auto identity = reinterpret_cast<std::uintptr_t>(component);
        if (!visible || identity == 0) {
            target_visible_.store(false, std::memory_order_release);
            current_component_.store(0, std::memory_order_release);
            input_pending_.store(false, std::memory_order_release);
            ++hidden_events_;
            return;
        }

        const auto previous_identity = current_component_.exchange(identity, std::memory_order_acq_rel);
        const bool was_visible = target_visible_.exchange(true, std::memory_order_acq_rel);
        if (identity != previous_identity || !was_visible) {
            input_pending_.store(true, std::memory_order_release);
        }
        ++active_events_;
    }

    [[nodiscard]] bool install_native_detour() noexcept {
        const auto executable = GetModuleHandleW(nullptr);
        if (!executable) return false;
        const auto address = reinterpret_cast<std::uint8_t*>(executable) + kNativeVisibilityRva;
        if (!signature_matches_guarded(address)) {
            logger_.write(dsnap::LogAudience::Debug, "NATIVE_SIGNATURE_REJECTED",
                          std::format("rva=0x{:X}", kNativeVisibilityRva));
            return false;
        }
        try {
            native_visibility_trampoline_.store(0, std::memory_order_release);
            native_detour_ = std::make_unique<PLH::x64Detour>(
                reinterpret_cast<std::uint64_t>(address),
                reinterpret_cast<std::uint64_t>(&NativeAutoPickup::native_visibility_detour),
                reinterpret_cast<std::uint64_t*>(&native_visibility_trampoline_storage_));
            if (!native_detour_->hook() || native_visibility_trampoline_storage_ == 0) {
                native_detour_.reset();
                native_visibility_trampoline_storage_ = 0;
                return false;
            }
            native_visibility_trampoline_.store(native_visibility_trampoline_storage_, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "NATIVE_DETOUR_READY",
                          std::format("rva=0x{:X} trampoline_ready=1", kNativeVisibilityRva));
            return true;
        } catch (...) {
            native_detour_.reset();
            native_visibility_trampoline_storage_ = 0;
            native_visibility_trampoline_.store(0, std::memory_order_release);
            return false;
        }
    }

    void uninstall_native_detour() noexcept {
        if (!native_detour_) return;
        try { static_cast<void>(native_detour_->unHook()); }
        catch (...) {}
        native_visibility_trampoline_.store(0, std::memory_order_release);
        native_visibility_trampoline_storage_ = 0;
        native_detour_.reset();
    }

    void engine_tick_post() noexcept {
        if (toggle_requested_.exchange(false, std::memory_order_acq_rel)) {
            const bool can_enable = build_trusted_.load(std::memory_order_acquire) &&
                                    configuration_result_.value.automatic_pickup;
            const bool next = can_enable && !active_.load(std::memory_order_acquire);
            active_.store(next, std::memory_order_release);
            if (next && target_visible_.load(std::memory_order_acquire)) {
                input_pending_.store(true, std::memory_order_release);
            }
            logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                          std::format("state={} trusted={} target_visible={}", next ? "On" : "Off", can_enable,
                                      target_visible_.load()));
        }

        const auto now = Clock::now();
        if (!active_.load(std::memory_order_acquire) ||
            !input_pending_.load(std::memory_order_acquire)) return;
        if (now < next_input_allowed_) {
            ++input_rejected_cooldown_;
            return;
        }
        if (!input_pending_.exchange(false, std::memory_order_acq_rel)) return;

        ++input_requests_;
        if (!game_window_is_foreground()) {
            ++input_rejected_foreground_;
            logger_.write(dsnap::LogAudience::Debug, "F_INPUT_REJECTED", "reason=game_not_foreground");
            return;
        }

        next_input_allowed_ = now + kInputCooldown;
        if (send_f_press_release()) {
            ++input_sent_;
            logger_.write(dsnap::LogAudience::User, "F_INPUT_SENT",
                          std::format("component=0x{:X} source=native_visibility_edge",
                                      current_component_.load(std::memory_order_acquire)));
        } else {
            ++input_failures_;
            logger_.write(dsnap::LogAudience::User, "F_INPUT_FAILED", "SendInput did not accept both keyboard records");
        }
    }

    void reset_world(const char* source) noexcept {
        active_.store(false, std::memory_order_release);
        input_pending_.store(false, std::memory_order_release);
        target_visible_.store(false, std::memory_order_release);
        current_component_.store(0, std::memory_order_release);
        ++world_resets_;
        logger_.write(dsnap::LogAudience::User, "STATE_CHANGED",
                      std::format("state=Off reason={} native_target=cleared", source));
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
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};
    inline static std::atomic<std::uint64_t> native_visibility_trampoline_{};

    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    dsnap::FingerprintResult fingerprint_result_{};
    ProcessShutdownProbe shutdown_probe_{};
    dsnap::QualifiedReleaseEdge f9_input_;

    std::unique_ptr<PLH::x64Detour> native_detour_{};
    std::uint64_t native_visibility_trampoline_storage_{};
    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};

    std::atomic<bool> shutting_down_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> active_{};
    std::atomic<bool> toggle_requested_{};
    std::atomic<bool> target_visible_{};
    std::atomic<bool> input_pending_{};
    std::atomic<std::uintptr_t> current_component_{};
    Clock::time_point next_input_allowed_{};
    Clock::time_point next_perf_log_{};

    std::atomic<std::uint64_t> native_visibility_events_{};
    std::atomic<std::uint64_t> active_events_{};
    std::atomic<std::uint64_t> hidden_events_{};
    std::atomic<std::uint64_t> input_requests_{};
    std::atomic<std::uint64_t> input_sent_{};
    std::atomic<std::uint64_t> input_rejected_foreground_{};
    std::atomic<std::uint64_t> input_rejected_cooldown_{};
    std::atomic<std::uint64_t> input_failures_{};
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
