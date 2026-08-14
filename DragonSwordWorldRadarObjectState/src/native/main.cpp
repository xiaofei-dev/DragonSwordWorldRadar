#include <dswros/object_state.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Input/KeyDef.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <windows.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

constexpr auto kVersion = STR("0.5.0-dev2-native-presence");
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kPositionInterval = std::chrono::milliseconds{33};
constexpr auto kDiscoveryInterval = std::chrono::milliseconds{250};
constexpr auto kActivationStability = std::chrono::milliseconds{750};
constexpr auto kTransitionCooldown = std::chrono::milliseconds{1500};
constexpr double kDiscoveryRadius = 3500.0;
constexpr std::uint32_t kSharedMagic = 0x534F5344U;
constexpr std::uint32_t kSharedVersion = 1;
constexpr std::size_t kEventCapacity = 128;

extern "C" IMAGE_DOS_HEADER __ImageBase;

[[nodiscard]] bool pin_own_module_for_process_lifetime() noexcept {
    HMODULE module{};
    return GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&__ImageBase), &module) != FALSE;
}

std::filesystem::path mod_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path()
        / "ue4ss" / "Mods" / "DragonSwordWorldRadarObjectState";
}

void append_log(const std::string& event, const std::string& detail) noexcept {
    try {
        const auto directory = mod_directory() / "runtime" / "logs";
        std::filesystem::create_directories(directory);
        std::ofstream output{directory / "DragonSwordWorldRadarObjectState.Native.log", std::ios::app};
        output << event << ' ' << detail << '\n';
    } catch (...) {
    }
}

#pragma pack(push, 1)
struct SharedEvent {
    std::uint64_t sequence{};
    std::uint32_t activation{};
    std::uint32_t epoch{};
    std::uint32_t kind{};
    std::uint32_t reserved{};
    std::int64_t id{};
    std::int64_t timestamp_ms{};
};

struct SharedState {
    volatile LONG sequence{};
    std::uint32_t magic{kSharedMagic};
    std::uint32_t version{kSharedVersion};
    std::uint32_t byte_size{sizeof(SharedState)};
    std::uint32_t flags{};
    std::uint32_t activation{};
    std::uint32_t epoch{};
    std::uint32_t reserved{};
    std::uint64_t sample_sequence{};
    std::int64_t sample_timestamp_ms{};
    double player_x{};
    double player_y{};
    double player_z{};
    std::uint64_t event_head{};
    std::uint64_t position_samples{};
    std::uint64_t actor_begin_callbacks{};
    std::uint64_t actor_end_callbacks{};
    std::uint64_t find_all_calls{};
    std::uint64_t find_all_total_us{};
    std::uint64_t find_all_max_us{};
    SharedEvent events[kEventCapacity]{};
};
#pragma pack(pop)

static_assert(offsetof(SharedState, events) == 128);
static_assert(sizeof(SharedEvent) == 40);
static_assert(sizeof(SharedState) == 5248);

class SharedPublisher {
public:
    SharedPublisher() {
        const auto name = std::format(L"Local\\DragonSwordWorldRadarObjectState.NativeState.{}", GetCurrentProcessId());
        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                      static_cast<DWORD>(sizeof(SharedState)), name.c_str());
        if (!mapping_) return;
        state_ = static_cast<SharedState*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState)));
        if (!state_) {
            CloseHandle(mapping_);
            mapping_ = nullptr;
            return;
        }
        ZeroMemory(state_, sizeof(*state_));
        state_->magic = kSharedMagic;
        state_->version = kSharedVersion;
        state_->byte_size = sizeof(SharedState);
    }

    ~SharedPublisher() {
        if (state_) UnmapViewOfFile(state_);
        if (mapping_) CloseHandle(mapping_);
    }

    [[nodiscard]] bool ready() const noexcept { return state_ != nullptr; }

    void publish_status(bool enabled, bool position_valid, bool transition, bool catchup_complete,
                        std::uint32_t activation, std::uint32_t epoch,
                        const dswros::Position& position, std::uint64_t sample_sequence) noexcept {
        if (!state_) return;
        begin_write();
        state_->flags = (enabled ? 1U : 0U) | (position_valid ? 2U : 0U)
            | (transition ? 4U : 0U) | (catchup_complete ? 8U : 0U);
        state_->activation = activation;
        state_->epoch = epoch;
        state_->sample_sequence = sample_sequence;
        state_->sample_timestamp_ms = monotonic_milliseconds();
        state_->player_x = position.x;
        state_->player_y = position.y;
        state_->player_z = position.z;
        state_->position_samples = position_samples_;
        state_->actor_begin_callbacks = actor_begin_callbacks_;
        state_->actor_end_callbacks = actor_end_callbacks_;
        state_->find_all_calls = find_all_calls_;
        state_->find_all_total_us = find_all_total_us_;
        state_->find_all_max_us = find_all_max_us_;
        end_write();
    }

    void publish_event(dswros::EventKind kind, std::int64_t id,
                       std::uint32_t activation, std::uint32_t epoch) noexcept {
        if (!state_) return;
        begin_write();
        const std::uint64_t sequence = ++event_head_;
        SharedEvent& event = state_->events[(sequence - 1U) % kEventCapacity];
        event = SharedEvent{sequence, activation, epoch, static_cast<std::uint32_t>(kind), 0, id,
                            unix_milliseconds()};
        state_->event_head = event_head_;
        end_write();
    }

    void record_position_sample() noexcept { ++position_samples_; }
    void record_begin() noexcept { ++actor_begin_callbacks_; }
    void record_end() noexcept { ++actor_end_callbacks_; }
    void record_find(std::uint64_t elapsed_us) noexcept {
        ++find_all_calls_;
        find_all_total_us_ += elapsed_us;
        find_all_max_us_ = std::max(find_all_max_us_, elapsed_us);
    }

private:
    static std::int64_t monotonic_milliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
    }
    static std::int64_t unix_milliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void begin_write() noexcept {
        InterlockedIncrement(&state_->sequence);
        MemoryBarrier();
    }
    void end_write() noexcept {
        MemoryBarrier();
        InterlockedIncrement(&state_->sequence);
    }

    HANDLE mapping_{};
    SharedState* state_{};
    std::uint64_t event_head_{};
    std::uint64_t position_samples_{};
    std::uint64_t actor_begin_callbacks_{};
    std::uint64_t actor_end_callbacks_{};
    std::uint64_t find_all_calls_{};
    std::uint64_t find_all_total_us_{};
    std::uint64_t find_all_max_us_{};
};

struct EncounterSpec {
    std::int64_t id{};
    std::string class_name{};
    dswros::Position position{};
};

struct ObservedRuntimeObject {
    FWeakObjectPtr weak{};
    dswros::EventKind kind{};
    std::int64_t id{};
    dswros::Position position{};
    std::uint32_t activation{};
    std::uint32_t epoch{};
    dswros::DisappearanceConfirmation disappearance{};
};

[[nodiscard]] double distance_squared(const dswros::Position& left, const dswros::Position& right) noexcept {
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    const double dz = left.z - right.z;
    return dx * dx + dy * dy + dz * dz;
}

[[nodiscard]] std::vector<std::string> split_tab(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t begin{};
    while (begin <= line.size()) {
        const auto end = line.find('\t', begin);
        fields.push_back(line.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1U;
    }
    return fields;
}

std::vector<dswros::CatalogPoint> load_catalog(const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) throw std::runtime_error{"treasure actor catalog is missing"};
    std::vector<dswros::CatalogPoint> result;
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line)) {
        const auto fields = split_tab(line);
        if (fields.size() != 5U) throw std::runtime_error{"treasure actor catalog row is malformed"};
        dswros::CatalogPoint point{};
        point.id = std::stoll(fields[0]);
        point.class_name = fields[1];
        point.position = {std::stod(fields[2]), std::stod(fields[3]), std::stod(fields[4])};
        if (point.id <= 0 || point.class_name.empty()) throw std::runtime_error{"treasure actor catalog identity is invalid"};
        result.push_back(std::move(point));
    }
    if (result.size() < 1600U || result.size() > 2500U) {
        throw std::runtime_error{"treasure actor catalog count is outside the accepted range"};
    }
    return result;
}

std::vector<EncounterSpec> load_encounter_catalog(const std::filesystem::path& directory) {
    std::vector<EncounterSpec> result;
    for (const auto& [name, expected] : std::array<std::pair<const char*, std::size_t>, 2>{{
             {"boss-actors.tsv", 9U}, {"assault-actors.tsv", 40U}}}) {
        std::ifstream input{directory / name};
        if (!input) throw std::runtime_error{std::string{name} + " is missing"};
        std::string line;
        std::getline(input, line);
        if (line != "Id\tClassName\tX\tY\tZ") throw std::runtime_error{std::string{name} + " header is invalid"};
        const auto before = result.size();
        while (std::getline(input, line)) {
            const auto fields = split_tab(line);
            if (fields.size() != 5U) throw std::runtime_error{std::string{name} + " row is malformed"};
            EncounterSpec spec{std::stoll(fields[0]), fields[1],
                               {std::stod(fields[2]), std::stod(fields[3]), std::stod(fields[4])}};
            if (spec.id <= 0 || spec.class_name.empty()) throw std::runtime_error{std::string{name} + " identity is invalid"};
            result.push_back(std::move(spec));
        }
        if (result.size() - before != expected) throw std::runtime_error{std::string{name} + " count is invalid"};
    }
    return result;
}

class NativeObjectState final : public CppUserModBase {
public:
    NativeObjectState()
        : instance_generation_(next_instance_generation_.fetch_add(1, std::memory_order_relaxed) + 1U) {
        ModName = STR("DragonSwordWorldRadarObjectState");
        ModVersion = kVersion;
        ModDescription = STR("Native coordinate and object-state provider with one-shot save synchronization");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        try {
            tracker_.set_catalog(load_catalog(mod_directory() / "data" / "generated" / "treasure-actors.tsv"));
            encounter_catalog_ = load_encounter_catalog(mod_directory() / "data" / "generated");
            catalog_ready_ = true;
        } catch (const std::exception& exception) {
            append_log("CATALOG_DISABLED", exception.what());
        }
        append_log("START", std::format("version=0.5.0-dev2-native-presence treasure_catalog={} encounter_catalog={} shared_memory={}",
                                         tracker_.catalog_count(), encounter_catalog_.size(), publisher_.ready()));
    }

    ~NativeObjectState() override {
        shutting_down_.store(true, std::memory_order_release);
        enabled_ = false;
        auto* expected = this;
        instance_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        unregister_callbacks();
    }

    void on_unreal_init() override {
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunction);
        const auto generation = instance_generation_;
        engine_tick_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                if (auto* self = current(generation)) self->engine_tick(engine);
            }, {false, false, STR("DragonSwordWorldRadarObjectState"), STR("NativeState")});
        begin_play_id_ = Hook::RegisterBeginPlayPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor) {
                if (auto* self = current(generation)) self->actor_begin(actor);
            }, {false, false, STR("DragonSwordWorldRadarObjectState"), STR("ActorBegin")});
        end_play_id_ = Hook::RegisterEndPlayPreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor, EEndPlayReason reason) {
                if (auto* self = current(generation)) self->actor_end(actor, reason);
            }, {false, false, STR("DragonSwordWorldRadarObjectState"), STR("ActorEnd")});
        transition_pre_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current(generation)) self->transition_begin();
            }, {false, false, STR("DragonSwordWorldRadarObjectState"), STR("Transition")});
        transition_post_id_ = Hook::RegisterInitGameStatePostCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current(generation)) self->transition_end();
            }, {false, false, STR("DragonSwordWorldRadarObjectState"), STR("Transition")});
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F7, [generation] {
            if (auto* self = current(generation)) self->f7_requests_.fetch_add(1, std::memory_order_release);
        });
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F8, [generation] {
            if (auto* self = current(generation)) self->f8_requests_.fetch_add(1, std::memory_order_release);
        });
        if (!location_function_ || engine_tick_id_ == Hook::ERROR_ID || begin_play_id_ == Hook::ERROR_ID
            || end_play_id_ == Hook::ERROR_ID || transition_pre_id_ == Hook::ERROR_ID
            || transition_post_id_ == Hook::ERROR_ID || !publisher_.ready()) {
            enabled_ = false;
            append_log("DISABLED", "required native metadata, callback, or shared-memory mapping is unavailable");
            return;
        }
        publish(false);
        append_log("READY", "hotkeys=F7,F8 coordinate_ms=33 discovery=nearby_exact_class_once_per_activation sql=overlay_one_shot_only");
    }

private:
    static NativeObjectState* current(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->instance_generation_ == generation
            && !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] bool game_thread() noexcept {
        const DWORD id = GetCurrentThreadId();
        if (game_thread_id_ == 0) game_thread_id_ = id;
        return game_thread_id_ == id;
    }

    void engine_tick(UEngine* engine) noexcept {
        if (!game_thread()) return;
#if defined(_MSC_VER)
        __try { engine_tick_unsafe(engine); }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            enabled_ = false;
            transition_active_ = true;
            publish(false);
        }
#else
        engine_tick_unsafe(engine);
#endif
    }

    void engine_tick_unsafe(UEngine* engine) {
        if (f8_requests_.exchange(0, std::memory_order_acq_rel) != 0) disable();
        if (f7_requests_.exchange(0, std::memory_order_acq_rel) != 0) activate();
        if (!enabled_ || transition_active_) return;
        const auto now = Clock::now();
        if (now < stable_after_) return;
        if (now >= next_position_) {
            next_position_ = now + kPositionInterval;
            dswros::Position position{};
            if (read_player_position(engine, &position)) {
                player_ = position;
                position_valid_ = true;
                ++sample_sequence_;
                publisher_.record_position_sample();
                publish(true);
            } else {
                position_valid_ = false;
                publish(false);
            }
        }
        if (position_valid_ && now >= next_discovery_) {
            next_discovery_ = now + kDiscoveryInterval;
            probe_observed_objects();
            schedule_nearby_classes();
            process_one_class();
        }
    }

    void activate() {
        enabled_ = true;
        transition_active_ = false;
        position_valid_ = false;
        ++activation_;
        ++epoch_;
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        pending_classes_.clear();
        scheduled_classes_.clear();
        scanned_classes_.clear();
        stable_after_ = Clock::now() + kActivationStability;
        next_position_ = stable_after_;
        next_discovery_ = stable_after_;
        publish(false);
        append_log("F7_ACTIVATED", std::format("activation={} epoch={} save_sync=one_shot_requested_by_overlay", activation_, epoch_));
    }

    void disable() {
        enabled_ = false;
        position_valid_ = false;
        ++epoch_;
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        pending_classes_.clear();
        scheduled_classes_.clear();
        scanned_classes_.clear();
        publish(false);
        append_log("F8_DISABLED", std::format("activation={} epoch={} object_work=stopped", activation_, epoch_));
    }

    void transition_begin() {
        if (!game_thread()) return;
        transition_active_ = true;
        position_valid_ = false;
        ++epoch_;
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        pending_classes_.clear();
        scheduled_classes_.clear();
        scanned_classes_.clear();
        publish(false);
    }

    void transition_end() {
        if (!game_thread()) return;
        transition_active_ = false;
        stable_after_ = Clock::now() + kTransitionCooldown;
        next_position_ = stable_after_;
        next_discovery_ = stable_after_;
        publish(false);
    }

    [[nodiscard]] bool read_player_position(UEngine* engine, dswros::Position* output) {
        if (!engine || !output || !location_function_) return false;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        auto** game_instance_value = viewport
            ? viewport->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameInstance")) : nullptr;
        auto* game_instance = game_instance_value ? *game_instance_value : nullptr;
        auto* players = game_instance
            ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers")) : nullptr;
        if (!players || !players->IsValidIndex(0) || !players->GetData()) return false;
        auto* local_player = static_cast<UObject* const*>(players->GetData())[0];
        auto** controller_value = local_player
            ? local_player->GetValuePtrByPropertyNameInChain<UObject*>(STR("PlayerController")) : nullptr;
        auto* controller = controller_value ? *controller_value : nullptr;
        auto** pawn_value = controller
            ? controller->GetValuePtrByPropertyNameInChain<UObject*>(STR("Pawn")) : nullptr;
        auto* pawn = pawn_value ? *pawn_value : nullptr;
        if (!pawn) return false;
        return read_actor_position(pawn, output);
    }

    [[nodiscard]] bool read_actor_position(UObject* actor, dswros::Position* output) {
        if (!actor || !output || !location_function_) return false;
        struct Parameters { FVector return_value{}; } parameters{};
        actor->ProcessEvent(location_function_, &parameters);
        *output = {parameters.return_value.X(), parameters.return_value.Y(), parameters.return_value.Z()};
        return std::isfinite(output->x) && std::isfinite(output->y) && std::isfinite(output->z);
    }

    void actor_begin(AActor* actor) noexcept {
        if (!enabled_ || transition_active_ || !game_thread() || !actor) return;
        publisher_.record_begin();
#if defined(_MSC_VER)
        __try { actor_begin_unsafe(actor); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        actor_begin_unsafe(actor);
#endif
    }

    void actor_begin_unsafe(AActor* actor) {
        auto* object_class = actor->GetClassPrivate();
        if (!object_class) return;
        const std::string class_name = to_string(object_class->GetName());
        if (!is_target_class(class_name)) return;
        dswros::Position position{};
        if (!read_actor_position(actor, &position)) return;
        FWeakObjectPtr weak{actor};
        const dswros::WeakIdentity identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        if (is_treasure_class(class_name)) {
            if (const auto id = tracker_.observe(identity, class_name, position)) {
                bind_observed(weak, identity, dswros::EventKind::TreasureOpened, *id, position);
            }
        }
        observe_encounter(weak, identity, class_name, position);
    }

    void actor_end(AActor* actor, EEndPlayReason reason) noexcept {
        if (!enabled_ || !game_thread() || !actor) return;
        publisher_.record_end();
#if defined(_MSC_VER)
        __try { actor_end_unsafe(actor, reason); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        actor_end_unsafe(actor, reason);
#endif
    }

    void actor_end_unsafe(AActor* actor, EEndPlayReason reason) {
        FWeakObjectPtr weak{actor};
        const dswros::WeakIdentity identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        static_cast<void>(tracker_.end(identity, false, transition_active_, player_, activation_, epoch_));
        const auto found = observed_objects_.find(identity.packed());
        if (found == observed_objects_.end()) return;
        const bool eligible = reason == EEndPlayReason::Destroyed || reason == EEndPlayReason::RemovedFromWorld;
        if (!eligible || transition_active_ || found->second.activation != activation_ || found->second.epoch != epoch_) {
            observed_objects_.erase(found);
            return;
        }
        found->second.disappearance.mark_eligible_end();
    }

    void observe_encounter(FWeakObjectPtr weak, dswros::WeakIdentity identity, const std::string& class_name,
                           const dswros::Position& position) {
        for (const auto& spec : encounter_catalog_) {
            if (class_name == spec.class_name && distance_squared(position, spec.position) <= 600.0 * 600.0) {
                bind_observed(weak, identity, dswros::EventKind::EncounterDefeated, spec.id, spec.position);
                return;
            }
        }
    }

    void bind_observed(FWeakObjectPtr weak, dswros::WeakIdentity identity, dswros::EventKind kind,
                       std::int64_t id, const dswros::Position& position) {
        for (auto it = observed_objects_.begin(); it != observed_objects_.end();) {
            if (it->second.kind == kind && it->second.id == id && it->first != identity.packed()) {
                it = observed_objects_.erase(it);
            } else ++it;
        }
        observed_objects_[identity.packed()] = {weak, kind, id, position, activation_, epoch_, {}};
    }

    void probe_observed_objects() {
        for (auto it = observed_objects_.begin(); it != observed_objects_.end();) {
            auto& observed = it->second;
            const bool context_valid = !transition_active_ && position_valid_
                && observed.activation == activation_ && observed.epoch == epoch_
                && distance_squared(player_, observed.position) <= 3000.0 * 3000.0;
            const bool present = observed.weak.Get() != nullptr;
            if (observed.disappearance.sample(present, context_valid)) {
                publisher_.publish_event(observed.kind, observed.id, activation_, epoch_);
                append_log(observed.kind == dswros::EventKind::TreasureOpened
                    ? "TREASURE_OPENED_NATIVE" : "ENCOUNTER_DEFEATED_NATIVE",
                    std::format("activation={} epoch={} id={} evidence=positive_observation_then_two_missing_samples",
                                activation_, epoch_, observed.id));
                it = observed_objects_.erase(it);
            } else if (!context_valid && !present) {
                it = observed_objects_.erase(it);
            } else ++it;
        }
    }

    void schedule_nearby_classes() {
        if (catalog_ready_) {
            for (auto& name : tracker_.nearby_classes(player_, kDiscoveryRadius)) schedule(std::move(name));
        }
        for (const auto& spec : encounter_catalog_) {
            if (distance_squared(player_, spec.position) <= kDiscoveryRadius * kDiscoveryRadius) {
                schedule(spec.class_name);
            }
        }
    }

    void schedule(std::string name) {
        if (scanned_classes_.contains(name) || !scheduled_classes_.insert(name).second) return;
        pending_classes_.push_back(std::move(name));
    }

    void process_one_class() {
        if (pending_classes_.empty()) return;
        std::string class_name = std::move(pending_classes_.front());
        pending_classes_.erase(pending_classes_.begin());
        scheduled_classes_.erase(class_name);
        scanned_classes_.insert(class_name);
        const auto started = Clock::now();
        std::vector<UObject*> objects;
        UObjectGlobals::FindAllOf(class_name, objects);
        for (auto* object : objects) {
            auto* actor = static_cast<AActor*>(object);
            if (actor) actor_begin_unsafe(actor);
        }
        const auto elapsed_us = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count());
        publisher_.record_find(elapsed_us);
        publish(position_valid_);
        append_log("NEARBY_CLASS_CATCHUP", std::format("activation={} class={} objects={} elapsed_us={}",
                                                        activation_, class_name, objects.size(), elapsed_us));
    }

    [[nodiscard]] static bool is_treasure_class(const std::string& name) {
        static const std::unordered_set<std::string> names{
            "TreasureBox01_C", "TreasureBox02_C", "TreasureBox02_Mount_C", "TreasureBox03_C",
            "TreasureBox03_Mount_C", "TreasureBox03_OnlyFront_C", "TreasureBox04Key_C",
            "TreasureBox04_C", "TreasureBox05_C", "TreasureBox05_Mount_C", "TreasureBox06_C"};
        return names.contains(name);
    }

    [[nodiscard]] bool is_target_class(const std::string& name) const {
        if (is_treasure_class(name)) return true;
        return std::any_of(encounter_catalog_.begin(), encounter_catalog_.end(),
                           [&name](const auto& spec) { return name == spec.class_name; });
    }

    void publish(bool position_valid) noexcept {
        publisher_.publish_status(enabled_, enabled_ && position_valid, transition_active_,
                                  pending_classes_.empty(), activation_, epoch_, player_, sample_sequence_);
    }

    void unregister_callbacks() noexcept {
        for (auto* id : {&engine_tick_id_, &begin_play_id_, &end_play_id_, &transition_pre_id_, &transition_post_id_}) {
            if (*id != Hook::ERROR_ID) {
                Hook::UnregisterCallback(*id);
                *id = Hook::ERROR_ID;
            }
        }
    }

    inline static std::atomic<NativeObjectState*> instance_{};
    inline static std::atomic<std::uint64_t> next_instance_generation_{};
    const std::uint64_t instance_generation_{};
    SharedPublisher publisher_{};
    dswros::ObjectStateTracker tracker_{};
    std::vector<EncounterSpec> encounter_catalog_{};
    std::unordered_map<std::uint64_t, ObservedRuntimeObject> observed_objects_{};
    std::vector<std::string> pending_classes_{};
    std::unordered_set<std::string> scheduled_classes_{};
    std::unordered_set<std::string> scanned_classes_{};
    UFunction* location_function_{};
    dswros::Position player_{};
    Clock::time_point stable_after_{};
    Clock::time_point next_position_{};
    Clock::time_point next_discovery_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<std::uint32_t> f7_requests_{};
    std::atomic<std::uint32_t> f8_requests_{};
    std::uint64_t sample_sequence_{};
    std::uint32_t activation_{};
    std::uint32_t epoch_{};
    DWORD game_thread_id_{};
    bool catalog_ready_{};
    bool enabled_{};
    bool transition_active_{};
    bool position_valid_{};
    Hook::GlobalCallbackId engine_tick_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId begin_play_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId end_play_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId transition_pre_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId transition_post_id_{Hook::ERROR_ID};
};

} // namespace

#define DSWROS_API __declspec(dllexport)
extern "C" {
DSWROS_API RC::CppUserModBase* start_mod() {
    if (!pin_own_module_for_process_lifetime()) return nullptr;
    return new NativeObjectState();
}
DSWROS_API void uninstall_mod(RC::CppUserModBase* mod) {
    delete static_cast<NativeObjectState*>(mod);
}
}
