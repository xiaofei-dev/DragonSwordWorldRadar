// Independent native position provider for DragonSwordNativeWorldRadar.
// No gameplay UObject is retained across samples or world transitions.

#pragma warning(push)
#include <Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Input/KeyDef.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <UE4SSProgram.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/TArray.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#pragma warning(pop)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

constexpr auto kVersion = STR("0.1.0-medium-poc");
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kPulseFunction = STR("/Script/Engine.PlayerCameraManager:BlueprintUpdateCamera");
constexpr std::uint64_t kAllMolesMask = (std::uint64_t{1} << 34U) - 1U;

struct Configuration {
    bool enabled_on_launch{};
    int sample_interval_ms{250};
    int recovery_cooldown_ms{1000};
    int activation_stability_ms{750};
    double radar_radius{22500.0};
    bool show_treasures{true};
    bool show_bosses{true};
    bool show_moles{true};
    bool show_height{true};
    bool show_treasure_types{};
    double text_scale{1.0};
};

struct Position {
    double x{};
    double y{};
    double z{};
};

std::filesystem::path binary_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path mod_directory() {
    return binary_directory() / "Mods" / "DragonSwordNativeWorldRadar";
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::optional<bool> parse_bool(std::string_view value) {
    if (value == "true") return true;
    if (value == "false") return false;
    return std::nullopt;
}

Configuration load_configuration(const std::filesystem::path& path) {
    Configuration result{};
    std::ifstream input{path};
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line.substr(0, line.find_first_of("#;")));
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));
        try {
            if (key == "enabled_on_launch") {
                if (const auto parsed = parse_bool(value)) result.enabled_on_launch = *parsed;
            } else if (key == "sample_interval_ms") {
                result.sample_interval_ms = std::clamp(std::stoi(value), 250, 2000);
            } else if (key == "recovery_cooldown_ms") {
                result.recovery_cooldown_ms = std::clamp(std::stoi(value), 500, 5000);
            } else if (key == "activation_stability_ms") {
                result.activation_stability_ms = std::clamp(std::stoi(value), 250, 5000);
            } else if (key == "radar_radius_world_units") {
                result.radar_radius = std::clamp(std::stod(value), 5000.0, 100000.0);
            } else if (key == "text_scale") {
                result.text_scale = std::clamp(std::stod(value), 0.5, 2.0);
            } else if (key == "show_treasures") {
                if (const auto parsed = parse_bool(value)) result.show_treasures = *parsed;
            } else if (key == "show_bosses") {
                if (const auto parsed = parse_bool(value)) result.show_bosses = *parsed;
            } else if (key == "show_moles") {
                if (const auto parsed = parse_bool(value)) result.show_moles = *parsed;
            } else if (key == "show_height") {
                if (const auto parsed = parse_bool(value)) result.show_height = *parsed;
            } else if (key == "show_treasure_types") {
                if (const auto parsed = parse_bool(value)) result.show_treasure_types = *parsed;
            }
        } catch (...) {
            // Each malformed optional setting fails closed to its bounded default.
        }
    }
    return result;
}

void append_log(const std::string& event, const std::string& detail) {
    try {
        const auto directory = mod_directory() / "runtime" / "logs";
        std::filesystem::create_directories(directory);
        std::ofstream output{directory / "DragonSwordNativeWorldRadar.Native.log", std::ios::app};
        output << event << " " << detail << '\n';
    } catch (...) {
        // Logging cannot participate in gameplay stability.
    }
}

bool write_all(HANDLE file, const std::string& payload) noexcept {
    DWORD written{};
    return WriteFile(file, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr) != FALSE
        && written == payload.size();
}

bool write_slot(const std::filesystem::path& path, const std::string& payload) noexcept {
    const auto file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    const bool ok = write_all(file, payload);
    CloseHandle(file);
    return ok;
}

bool launch_overlay_host() {
    const auto host = mod_directory() / "host" / "DragonSwordNativeWorldRadar.Host.ps1";
    if (!std::filesystem::exists(host)) {
        append_log("HOST_MISSING", host.string());
        return false;
    }
    std::wstring command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + host.wstring()
        + L"\" -ModDir \"" + mod_directory().wstring() + L"\" -GamePid " + std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const auto created = CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    if (!created) {
        append_log("HOST_START_FAILED", std::to_string(GetLastError()));
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    append_log("HOST_STARTED", std::to_string(process.dwProcessId));
    return true;
}

class NativeWorldRadar final : public CppUserModBase {
public:
    NativeWorldRadar()
        : generation_(next_generation_.fetch_add(1, std::memory_order_relaxed) + 1),
          config_(load_configuration(mod_directory() / "config.ini")),
          enabled_(config_.enabled_on_launch) {
        ModName = STR("DragonSwordNativeWorldRadar");
        ModVersion = kVersion;
        ModDescription = STR("Independent native position provider with the proven external radar renderer");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        try {
            std::filesystem::create_directories(mod_directory() / "runtime" / "bridge");
        } catch (...) {}
        append_log("START", std::format("generation={} enabled={}", generation_, enabled_));
    }

    ~NativeWorldRadar() override {
        shutting_down_.store(true, std::memory_order_release);
        if (pulse_hook_registered_ && pulse_function_) {
            pulse_function_->UnregisterHook(pulse_hook_pre_);
            pulse_hook_registered_ = false;
            pulse_function_ = nullptr;
        }
        publish_disabled();
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        append_log("STOP", std::format("generation={}", generation_));
    }

    void on_unreal_init() override {
        game_thread_id_ = GetCurrentThreadId();
        pulse_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kPulseFunction);
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunction);
        if (pulse_function_) {
            pulse_hook_pre_ = pulse_function_->RegisterPreHook(&camera_pulse_pre, this);
            pulse_hook_registered_ = true;
        } else {
            append_log("PULSE_HOOK_MISSING", "function=BlueprintUpdateCamera");
        }

        const auto generation = generation_;
        Hook::RegisterInitGameStatePreCallback([generation](AGameModeBase*) {
            if (auto* self = current_instance(generation)) self->begin_world_transition("InitGameStatePre");
        });
        Hook::RegisterInitGameStatePostCallback([generation](AGameModeBase*) {
            if (auto* self = current_instance(generation)) self->finish_world_transition("InitGameStatePost");
        });
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F10, [generation] {
            if (auto* self = current_instance(generation)) {
                self->toggle_requests_.fetch_add(1, std::memory_order_release);
                append_log("TOGGLE_QUEUED", "hotkey=F10 dispatch=event_loop");
            }
        });

        const auto now = Clock::now();
        stable_after_ = now + std::chrono::milliseconds{config_.activation_stability_ms};
        next_sample_ = stable_after_;
        publish_disabled();
        launch_overlay_host();
        append_log("READY", std::format("hotkey=F10 pulse=BlueprintUpdateCamera pulse_interval_ms={} continuous_uobject_scans=0 engine_find_scans=0",
                                        config_.sample_interval_ms));
    }

private:
    static NativeWorldRadar* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->generation_ == generation && !self->shutting_down_.load(std::memory_order_acquire)
            ? self : nullptr;
    }

    bool game_thread() const noexcept {
        return game_thread_id_ != 0 && GetCurrentThreadId() == game_thread_id_;
    }

    static void camera_pulse_pre(UnrealScriptFunctionCallableContext context, void* custom_data) {
        auto* self = static_cast<NativeWorldRadar*>(custom_data);
        if (!self || current_instance(self->generation_) != self || !context.Context) return;
        self->game_thread_id_ = GetCurrentThreadId();
        self->game_thread_pulse(context.Context);
    }

    void game_thread_pulse(UObject* camera_manager) {
        if (!game_thread() || shutting_down_.load(std::memory_order_acquire)) return;

        const auto toggle_count = toggle_requests_.exchange(0, std::memory_order_acq_rel);
        if (toggle_count != 0 && (toggle_count & 1U) != 0) apply_toggle();
        if (!enabled_) return;

        const auto now = Clock::now();
        if (now < stable_after_ || now < next_sample_) return;
        next_sample_ = now + std::chrono::milliseconds{config_.sample_interval_ms};

        Position position{};
        if (!guarded_sample(camera_manager, &position)) {
            handle_sample_failure(now);
            return;
        }

        if (!had_valid_sample_) {
            append_log("WORLD_READY", std::format("epoch={} proof=player_location", world_epoch_));
        }
        had_valid_sample_ = true;
        failure_logged_ = false;
        publish_position(position);
    }

    void apply_toggle() {
        enabled_ = !enabled_;
        ++world_epoch_;
        had_valid_sample_ = false;
        failure_logged_ = false;
        stable_after_ = Clock::now() + std::chrono::milliseconds{config_.activation_stability_ms};
        next_sample_ = stable_after_;
        publish_disabled();
        append_log("ACTIVE_CHANGED", std::format("enabled={} epoch={} stability_ms={}", enabled_, world_epoch_, config_.activation_stability_ms));
    }

    void begin_world_transition(const char* source) {
        if (!game_thread()) return;
        ++world_epoch_;
        transition_active_ = true;
        had_valid_sample_ = false;
        failure_logged_ = false;
        stable_after_ = Clock::time_point::max();
        publish_disabled();
        append_log("WORLD_TRANSITION_BEGIN", std::format("source={} epoch={}", source, world_epoch_));
    }

    void finish_world_transition(const char* source) {
        if (!game_thread()) return;
        transition_active_ = false;
        stable_after_ = Clock::now() + std::chrono::milliseconds{config_.recovery_cooldown_ms};
        next_sample_ = stable_after_;
        append_log("WORLD_TRANSITION_COOLDOWN", std::format("source={} epoch={} cooldown_ms={}", source, world_epoch_, config_.recovery_cooldown_ms));
    }

    bool sample_unsafe(UObject* camera_manager, Position* result) noexcept {
        if (!camera_manager || !result || transition_active_ || !location_function_) return false;
        auto** controller_value = camera_manager->GetValuePtrByPropertyName<UObject*>(STR("PCOwner"));
        auto* controller = controller_value ? *controller_value : nullptr;
        if (!controller) return false;
        auto** pawn_value = controller->GetValuePtrByPropertyName<UObject*>(STR("Pawn"));
        auto* pawn = pawn_value ? *pawn_value : nullptr;
        if (!pawn) return false;
        struct LocationParameters { FVector return_value{}; } parameters{};
        pawn->ProcessEvent(location_function_, &parameters);
        result->x = parameters.return_value.X();
        result->y = parameters.return_value.Y();
        result->z = parameters.return_value.Z();
        return std::isfinite(result->x) && std::isfinite(result->y) && std::isfinite(result->z);
    }

    bool guarded_sample(UObject* camera_manager, Position* result) noexcept {
#if defined(_MSC_VER)
        __try {
            return sample_unsafe(camera_manager, result);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        return sample_unsafe(camera_manager, result);
#endif
    }

    void handle_sample_failure(Clock::time_point now) {
        if (had_valid_sample_) {
            ++world_epoch_;
            publish_disabled();
        }
        had_valid_sample_ = false;
        stable_after_ = now + std::chrono::milliseconds{config_.recovery_cooldown_ms};
        next_sample_ = stable_after_;
        if (!failure_logged_) {
            append_log("SAMPLE_SUSPENDED", std::format("epoch={} retry_ms={} uobject_scans=0", world_epoch_, config_.recovery_cooldown_ms));
            failure_logged_ = true;
        }
    }

    void publish_disabled() {
        publish(Position{}, false);
    }

    void publish_position(const Position& position) {
        publish(position, true);
    }

    void publish(const Position& position, bool frame_enabled) {
        const auto sequence = ++sequence_;
        const auto timestamp_ms = std::chrono::duration<double, std::milli>(Clock::now().time_since_epoch()).count();
        const auto body = std::format(
            "5|{}|{}|{:.3f}|{}|radar|{}|{}|{}|{}|{}|{}|0|0|0|0|0|0|{:.3f}|{:.6f}|{:.6f}|{:.6f}|{}|{:.6f}|0|0|0|0|0|0|0|0|0|0|0",
            generation_, world_epoch_, timestamp_ms, frame_enabled ? 1 : 0,
            frame_enabled && config_.show_height ? 1 : 0,
            frame_enabled && config_.show_treasure_types ? 1 : 0,
            frame_enabled && config_.show_treasures ? 1 : 0,
            frame_enabled && config_.show_bosses ? 1 : 0,
            frame_enabled && config_.show_moles ? 1 : 0,
            frame_enabled && config_.show_moles ? kAllMolesMask : 0,
            config_.text_scale, position.x, position.y, position.z,
            frame_enabled ? 1 : 0, config_.radar_radius);
        const auto sequence_text = std::to_string(sequence);
        const auto payload = sequence_text + "|" + body + "|" + sequence_text + "\n";
        const auto bridge = mod_directory() / "runtime" / "bridge";
        const auto path = bridge / (sequence % 2 == 0 ? "native_radar_motion_a.dat" : "native_radar_motion_b.dat");
        if (!write_slot(path, payload) && !bridge_error_logged_) {
            append_log("BRIDGE_WRITE_FAILED", path.string());
            bridge_error_logged_ = true;
        }
    }

    inline static std::atomic<NativeWorldRadar*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};

    const std::uint64_t generation_;
    const Configuration config_;
    std::atomic<bool> shutting_down_{};
    std::atomic<std::uint32_t> toggle_requests_{};
    UFunction* pulse_function_{}; // Static function metadata resolved once.
    UFunction* location_function_{}; // Static function metadata resolved once.
    Clock::time_point next_sample_{};
    Clock::time_point stable_after_{};
    std::uint64_t sequence_{};
    std::uint64_t world_epoch_{};
    DWORD game_thread_id_{};
    CallbackId pulse_hook_pre_{};
    bool enabled_{};
    bool pulse_hook_registered_{};
    bool transition_active_{};
    bool had_valid_sample_{};
    bool failure_logged_{};
    bool bridge_error_logged_{};
};

} // namespace

#define DSNWR_API __declspec(dllexport)
extern "C" {
DSNWR_API RC::CppUserModBase* start_mod() { return new NativeWorldRadar(); }
DSNWR_API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
