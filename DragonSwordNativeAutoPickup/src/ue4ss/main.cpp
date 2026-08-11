// OWNER_AUTHORIZED_RUNTIME_CANARY targeting pinned RE-UE4SS v3.0.1.
// Runtime behavior remains unaccepted until owner gameplay validation.

#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#pragma warning(disable : 4251 5038)
#include <Input/KeyDef.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>
#pragma warning(disable : 4251 5038)
#include <UE4SSProgram.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#pragma warning(pop)

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
#include <unordered_map>
#include <vector>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

constexpr auto kVersion = STR("0.3.2-relocated-runtime-canary");
constexpr auto kCanaryLabel = "OWNER_AUTHORIZED_RUNTIME_CANARY";
constexpr auto kDropItemClass = STR("/Script/DS.DropItemActor");
constexpr auto kPlayerCharacterClass = STR("/Script/DS.DsPlayerCharacter");
constexpr auto kPlayerControllerClass = STR("/Script/DS.DsPlayerController");
constexpr auto kInteractionFunction = STR("/Script/DS.DInteractableComponent:Server_InputInteractKeyAction");
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kPulseFunction = STR("/Script/Engine.PlayerController:ServerRecvClientInputFrame");
constexpr auto kPulseInterval = std::chrono::milliseconds{150};
constexpr std::uint8_t kRequiredInteractableValue = 2;
constexpr std::size_t kCandidatesPerPulse = 8;
constexpr std::size_t kMaxParameterBytes = 4096;

std::filesystem::path binary_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path mod_directory() {
    return binary_directory() / "ue4ss" / "Mods" / "DragonSwordNativeAutoPickup";
}

struct CandidateState {
    FWeakObjectPtr weak{};
    Clock::time_point next_attempt{};
    std::uint32_t failures{};
};

struct CandidateSnapshot {
    std::int32_t index{-1};
    FWeakObjectPtr weak{};
};

enum class GateResult { Eligible, RetryLater, PermanentReject, Invalid };

struct GateOutput {
    GateResult result{GateResult::Invalid};
    UObject* component{};
    double distance_meters{};
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
        ModDescription = STR("Owner-authorized native input-frame DropItemActor pickup canary");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "START",
                      std::format("label={} armed=0 active=0 generation={}", kCanaryLabel, callback_gate_.generation()));
        for (const auto& error : configuration_result_.errors) {
            logger_.write(dsnap::LogAudience::Debug, "CONFIG_REJECTED", error);
        }
    }

    ~NativeAutoPickup() override {
        shutting_down_.store(true, std::memory_order_release);
        callback_gate_.invalidate();
        active_.store(false, std::memory_order_release);
        if (listeners_registered_) {
            UObjectArray::RemoveUObjectCreateListener(this);
            UObjectArray::RemoveUObjectDeleteListener(this);
            listeners_registered_ = false;
        }
        if (pulse_hook_registered_ && pulse_function_) {
            pulse_function_->UnregisterHook(pulse_hook_post_);
            pulse_hook_registered_ = false;
            pulse_function_ = nullptr;
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
        player_character_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kPlayerCharacterClass);
        player_controller_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kPlayerControllerClass);
        interaction_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kInteractionFunction);
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunction);
        pulse_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kPulseFunction);
        if (!drop_item_class_ || !player_character_class_ || !player_controller_class_ ||
            !interaction_function_ || !location_function_ || !pulse_function_) {
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required class or UFunction metadata is missing");
            return;
        }

        UObjectArray::AddUObjectCreateListener(this);
        UObjectArray::AddUObjectDeleteListener(this);
        listeners_registered_ = true;
        pulse_hook_post_ = pulse_function_->RegisterPostHook(&input_frame_post, this);
        pulse_hook_registered_ = true;

        const auto generation = callback_gate_.generation();
        Hook::RegisterInitGameStatePreCallback([generation](AGameModeBase*) {
            if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
        });
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F9, [generation] {
            if (auto* self = current_instance(generation)) self->toggle_armed();
        });
        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey=F9 pulse=ServerRecvClientInputFrame pulse_ms={} "
                                  "candidates=weak_lifecycle max_queue={} generation={}",
                                  kCanaryLabel, kPulseInterval.count(), configuration_result_.value.max_queue, generation));
    }

    void on_update() override {
        // Event-loop callback: plain atomics, future completion, and logging only.
        if (shutting_down_.load(std::memory_order_acquire)) return;
        if (!fingerprint_applied_ &&
            fingerprint_future_.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
            const auto result = fingerprint_future_.get();
            fingerprint_applied_ = true;
            build_trusted_.store(result.trusted, std::memory_order_release);
            if (!result.trusted) active_.store(false, std::memory_order_release);
            logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                          std::format("trusted={} game={} ue4ss={} error={}", result.trusted, result.game_sha256,
                                      result.ue4ss_sha256, result.error));
            if (!result.trusted) logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY", "unknown build fingerprint");
        }

        const auto now = Clock::now();
        if (next_perf_log_ == Clock::time_point{}) {
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        } else if (now >= next_perf_log_) {
            logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                          std::format("label={} create_callbacks={} exact_captures={} delete_callbacks={} "
                                      "raw_hook_callbacks={} accepted_pulses={} throttle_rejects={} gate_rejections={} "
                                      "seh_rejections={} actions={} action_failures={} world_resets={} candidates={} "
                                      "armed={} world_ready={} trusted={} active={} dropped_logs={}",
                                      kCanaryLabel, create_callbacks_.load(), exact_captures_.load(), delete_callbacks_.load(),
                                      raw_hook_callbacks_.load(), accepted_pulses_.load(), throttle_rejects_.load(),
                                      gate_rejections_.load(), seh_rejections_.load(), actions_.load(),
                                      action_failures_.load(), world_resets_.load(), candidate_count(), armed_.load(),
                                      world_ready_.load(), build_trusted_.load(), active_.load(), logger_.dropped_messages()));
            next_perf_log_ = now + std::chrono::seconds{configuration_result_.value.perf_log_interval_seconds};
        }
    }

    void NotifyUObjectCreated(const UObjectBase* object, int32 index) override {
        ++create_callbacks_;
        if (!object || shutting_down_.load(std::memory_order_acquire)) return;
        FWeakObjectPtr weak{};
        if (!capture_exact_drop_guarded(object, &weak)) return;
        CandidateState candidate{};
        candidate.weak = weak;
        const std::scoped_lock lock{candidates_mutex_};
        if (candidates_.size() >= configuration_result_.value.max_queue && !candidates_.contains(index)) {
            ++queue_rejections_;
            return;
        }
        candidates_[index] = candidate;
        ++exact_captures_;
    }

    void NotifyUObjectDeleted(const UObjectBase*, int32 index) override {
        ++delete_callbacks_;
        const std::scoped_lock lock{candidates_mutex_};
        candidates_.erase(index);
    }

    void OnUObjectArrayShutdown() override { listeners_registered_ = false; }

private:
    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    static void input_frame_post(UnrealScriptFunctionCallableContext context, void* custom_data) {
        auto* self = static_cast<NativeAutoPickup*>(custom_data);
        if (!self || current_instance(self->callback_gate_.generation()) != self || !context.Context) return;
        ++self->raw_hook_callbacks_;
        try {
            if (!handle_input_frame_guarded(self, context.Context)) ++self->seh_rejections_;
        } catch (...) {
            ++self->gate_rejections_;
        }
    }

    static bool handle_input_frame_guarded(NativeAutoPickup* self, UObject* controller) noexcept {
        bool ok{};
#if defined(_MSC_VER)
        __try {
            self->handle_input_frame(controller);
            ok = true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { ok = false; }
#else
        self->handle_input_frame(controller);
        ok = true;
#endif
        return ok;
    }

    void handle_input_frame(UObject* controller) {
        if (!controller || !controller->IsA(player_controller_class_)) {
            ++gate_rejections_;
            return;
        }
        const auto now = Clock::now();
        if (next_pulse_due_ != Clock::time_point{} && now < next_pulse_due_) {
            ++throttle_rejects_;
            return;
        }
        next_pulse_due_ = now + kPulseInterval;

        auto** pawn_value = controller->GetValuePtrByPropertyNameInChain<UObject*>(STR("Pawn"));
        auto* player = pawn_value ? *pawn_value : nullptr;
        if (!player || !player->IsA(player_character_class_)) {
            ++gate_rejections_;
            return;
        }
        auto** controller_value = player->GetValuePtrByPropertyNameInChain<UObject*>(STR("Controller"));
        if (!controller_value || *controller_value != controller) {
            ++gate_rejections_;
            return;
        }
        FVector player_location{};
        if (!read_location_guarded(player, &player_location)) {
            ++gate_rejections_;
            return;
        }

        ++accepted_pulses_;
        world_ready_.store(true, std::memory_order_release);
        refresh_active("valid_input_frame_pulse");
        if (active_.load(std::memory_order_acquire)) process_candidates(player, player_location);
    }

    void process_candidates(UObject* player, const FVector& player_location) {
        const auto now = Clock::now();
        if (now < next_global_action_) return;
        std::vector<CandidateSnapshot> due{};
        due.reserve(kCandidatesPerPulse);
        {
            const std::scoped_lock lock{candidates_mutex_};
            for (auto& [index, candidate] : candidates_) {
                if (candidate.next_attempt > now) continue;
                candidate.next_attempt = now + std::chrono::milliseconds{configuration_result_.value.retry_backoff_ms};
                due.push_back(CandidateSnapshot{index, candidate.weak});
                if (due.size() >= kCandidatesPerPulse) break;
            }
        }

        for (const auto& candidate : due) {
            auto* owner = candidate.weak.Get();
            if (!owner) { erase_candidate(candidate.index); continue; }
            GateOutput gate{};
            if (!evaluate_candidate_guarded(this, owner, &player_location, &gate)) {
                ++seh_rejections_;
                record_failure(candidate.index, now);
                continue;
            }
            if (gate.result == GateResult::Invalid || gate.result == GateResult::PermanentReject) {
                erase_candidate(candidate.index);
                ++gate_rejections_;
                continue;
            }
            if (gate.result == GateResult::RetryLater) continue;

            next_global_action_ = now + std::chrono::milliseconds{configuration_result_.value.action_interval_ms};
            if (invoke_interaction_guarded(this, gate.component, owner, player)) {
                ++actions_;
                erase_candidate(candidate.index);
                logger_.write(dsnap::LogAudience::Debug, "AUTO_PICKUP",
                              std::format("object_index={} distance_meters={:.3f} key_action=13", candidate.index,
                                          gate.distance_meters));
            } else {
                ++action_failures_;
                record_failure(candidate.index, now);
            }
            break;
        }
    }

    static bool capture_exact_drop_guarded(const UObjectBase* object, FWeakObjectPtr* weak) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            auto* self = instance_.load(std::memory_order_acquire);
            auto* unreal_object = std::bit_cast<UObject*>(object);
            if (self && unreal_object->GetClassPrivate() == self->drop_item_class_) {
                *weak = unreal_object;
                captured = true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) { captured = false; }
#else
        auto* self = instance_.load(std::memory_order_acquire);
        auto* unreal_object = std::bit_cast<UObject*>(object);
        if (self && unreal_object->GetClassPrivate() == self->drop_item_class_) {
            *weak = unreal_object;
            captured = true;
        }
#endif
        return captured;
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
        if (!owner || owner->GetClassPrivate() != drop_item_class_) return {GateResult::Invalid, nullptr, 0.0};
        auto** component_value = owner->GetValuePtrByPropertyNameInChain<UObject*>(STR("InteractComponent"));
        auto* component = component_value ? *component_value : nullptr;
        if (!component) return {GateResult::RetryLater, nullptr, 0.0};
        if (component->GetOuterPrivate() != owner) return {GateResult::PermanentReject, nullptr, 0.0};
        auto* interactable = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractableValue"));
        auto* interact_type = component->GetValuePtrByPropertyName<std::uint8_t>(STR("InteractTypeValue"));
        if (!interactable || !interact_type) return {GateResult::RetryLater, nullptr, 0.0};
        if (*interactable != kRequiredInteractableValue || *interact_type != dsnap::kDropItemInteractType) {
            return {GateResult::PermanentReject, nullptr, 0.0};
        }

        FVector owner_location{};
        if (!read_location_guarded(owner, &owner_location)) return {GateResult::RetryLater, nullptr, 0.0};
        const auto dx = owner_location.X() - player_location.X();
        const auto dy = owner_location.Y() - player_location.Y();
        const auto dz = owner_location.Z() - player_location.Z();
        const auto distance_squared = dx * dx + dy * dy + dz * dz;
        const auto radius_world = configuration_result_.value.radius_meters * 100.0;
        if (!std::isfinite(distance_squared)) return {GateResult::Invalid, nullptr, 0.0};
        if (distance_squared > radius_world * radius_world) return {GateResult::RetryLater, nullptr, 0.0};
        return {GateResult::Eligible, component, std::sqrt(distance_squared) / 100.0};
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

    static bool invoke_interaction_guarded(NativeAutoPickup* self, UObject* component,
                                           UObject* owner, UObject* player) noexcept {
        bool invoked{};
#if defined(_MSC_VER)
        __try { invoked = self->invoke_interaction_unsafe(component, owner, player); }
        __except (EXCEPTION_EXECUTE_HANDLER) { invoked = false; }
#else
        invoked = self->invoke_interaction_unsafe(component, owner, player);
#endif
        return invoked;
    }

    bool invoke_interaction_unsafe(UObject* component, UObject* owner, UObject* player) {
        if (!component || !owner || !player || !interaction_function_) return false;
        const auto parameter_bytes = static_cast<std::size_t>(interaction_function_->GetParmsSize());
        if (parameter_bytes == 0 || parameter_bytes > kMaxParameterBytes) return false;
        alignas(std::uint64_t) std::array<std::byte, kMaxParameterBytes> storage{};
        void* parameters = storage.data();
        bool target_written{};
        bool player_written{};
        bool key_written{};
        for (auto* property : interaction_function_->ForEachProperty()) {
            if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm)) continue;
            auto* value = property->ContainerPtrToValuePtr<void>(parameters);
            if (!value) return false;
            const auto name = property->GetName();
            if (name == STR("TargetActor")) {
                *static_cast<UObject**>(value) = owner;
                target_written = true;
            } else if (name == STR("InActor")) {
                *static_cast<UObject**>(value) = player;
                player_written = true;
            } else if (name == STR("KeyAction")) {
                *static_cast<std::uint8_t*>(value) = dsnap::kDropItemKeyActionEnumCandidate;
                key_written = true;
            }
        }
        if (!target_written || !player_written || !key_written) return false;
        component->ProcessEvent(interaction_function_, parameters);
        return true;
    }

    void record_failure(std::int32_t index, Clock::time_point now) {
        const std::scoped_lock lock{candidates_mutex_};
        const auto found = candidates_.find(index);
        if (found == candidates_.end()) return;
        ++found->second.failures;
        if (found->second.failures > configuration_result_.value.max_retries) {
            candidates_.erase(found);
            return;
        }
        found->second.next_attempt = now +
            std::chrono::milliseconds{configuration_result_.value.retry_backoff_ms * found->second.failures};
    }

    void erase_candidate(std::int32_t index) {
        const std::scoped_lock lock{candidates_mutex_};
        candidates_.erase(index);
    }

    [[nodiscard]] std::size_t candidate_count() const {
        const std::scoped_lock lock{candidates_mutex_};
        return candidates_.size();
    }

    void toggle_armed() {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        const bool armed = !armed_.load(std::memory_order_acquire);
        armed_.store(armed, std::memory_order_release);
        if (!armed) active_.store(false, std::memory_order_release);
        else refresh_active("hotkey");
        logger_.write(dsnap::LogAudience::User, "ARMED_CHANGED",
                      std::format("armed={} trusted={} world_ready={} active={}", armed, build_trusted_.load(),
                                  world_ready_.load(), active_.load()));
    }

    void refresh_active(const char* reason) {
        const bool desired = armed_.load(std::memory_order_acquire) &&
                             build_trusted_.load(std::memory_order_acquire) &&
                             world_ready_.load(std::memory_order_acquire);
        const bool previous = active_.exchange(desired, std::memory_order_acq_rel);
        if (previous != desired) {
            logger_.write(dsnap::LogAudience::User, "ACTIVE_CHANGED", std::format("active={} reason={}", desired, reason));
        }
    }

    void reset_world(const char* source) {
        active_.store(false, std::memory_order_release);
        world_ready_.store(false, std::memory_order_release);
        next_pulse_due_ = {};
        next_global_action_ = {};
        {
            const std::scoped_lock lock{candidates_mutex_};
            candidates_.clear();
        }
        ++world_resets_;
        logger_.write(dsnap::LogAudience::Debug, "WORLD_RESET",
                      std::format("source={} active=0 candidates=0 armed={}", source, armed_.load()));
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};
    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::ConfigurationResult configuration_result_{};
    dsnap::AsyncLogger logger_;
    std::future<dsnap::FingerprintResult> fingerprint_future_;
    mutable std::mutex candidates_mutex_{};
    std::unordered_map<std::int32_t, CandidateState> candidates_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<bool> armed_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> world_ready_{};
    std::atomic<bool> active_{};
    std::atomic<std::uint64_t> create_callbacks_{};
    std::atomic<std::uint64_t> exact_captures_{};
    std::atomic<std::uint64_t> delete_callbacks_{};
    std::atomic<std::uint64_t> queue_rejections_{};
    std::atomic<std::uint64_t> raw_hook_callbacks_{};
    std::atomic<std::uint64_t> accepted_pulses_{};
    std::atomic<std::uint64_t> throttle_rejects_{};
    std::atomic<std::uint64_t> gate_rejections_{};
    std::atomic<std::uint64_t> seh_rejections_{};
    std::atomic<std::uint64_t> actions_{};
    std::atomic<std::uint64_t> action_failures_{};
    std::atomic<std::uint64_t> world_resets_{};
    UClass* drop_item_class_{};
    UClass* player_character_class_{};
    UClass* player_controller_class_{};
    UFunction* interaction_function_{};
    UFunction* location_function_{};
    UFunction* pulse_function_{};
    Clock::time_point next_pulse_due_{};
    Clock::time_point next_global_action_{};
    Clock::time_point next_perf_log_{};
    CallbackId pulse_hook_post_{};
    bool listeners_registered_{};
    bool pulse_hook_registered_{};
    bool fingerprint_applied_{};
};

} // namespace

#define DSNAP_API __declspec(dllexport)
extern "C" {
DSNAP_API RC::CppUserModBase* start_mod() { return new NativeAutoPickup(); }
DSNAP_API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
