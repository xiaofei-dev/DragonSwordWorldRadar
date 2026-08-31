#include "postrender_canary.hpp"

#include <dswros/render_projection.hpp>

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

#include <windows.h>

namespace dsnwr {
namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;

constexpr std::uint32_t kExpectedGameTimestamp = 0x691B0D98U;
constexpr std::uint32_t kExpectedGameImageSize = 0x09EB2000U;
constexpr std::uint32_t kExpectedUe4ssTimestamp = 0x6A76FCA3U;
constexpr std::uint32_t kExpectedUe4ssImageSize = 0x00FCA000U;
constexpr auto kDrawLineFunction = STR("/Script/Engine.Canvas:K2_DrawLine");
constexpr auto kCanvasClass = STR("/Script/Engine.Canvas");

struct Vec2 {
    double x{};
    double y{};
};

struct LinearColor {
    float red{};
    float green{};
    float blue{};
    float alpha{};
};

static_assert(sizeof(Vec2) == 16);
static_assert(sizeof(LinearColor) == 16);

struct ModuleImage {
    std::uintptr_t begin{};
    std::uintptr_t end{};
    std::uint32_t timestamp{};
    std::uint32_t image_size{};
    bool valid{};
};

[[nodiscard]] ModuleImage module_image(HMODULE module) noexcept {
    if (!module) return {};
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) return {};
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + static_cast<std::uintptr_t>(dos->e_lfanew));
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return {};
    const auto size = nt->OptionalHeader.SizeOfImage;
    if (size == 0) return {};
    return {base, base + size, nt->FileHeader.TimeDateStamp, size, true};
}

[[nodiscard]] bool executable_pointer(void* pointer) noexcept {
    if (!pointer) return false;
    MEMORY_BASIC_INFORMATION information{};
    if (VirtualQuery(pointer, &information, sizeof(information)) != sizeof(information)
        || information.State != MEM_COMMIT || (information.Protect & PAGE_GUARD) != 0) {
        return false;
    }
    const DWORD protection = information.Protect & 0xFFU;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ
        || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

[[nodiscard]] bool module_contains(const ModuleImage& image, void* pointer) noexcept {
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    return image.valid && address >= image.begin && address < image.end;
}

[[nodiscard]] std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] bool parse_bool(std::string_view value, bool fallback) noexcept {
    if (value == "true" || value == "1") return true;
    if (value == "false" || value == "0") return false;
    return fallback;
}

[[nodiscard]] bool property_layout(UStruct* owner, const wchar_t* name,
                                   std::size_t expected_size, std::uint16_t* offset) noexcept {
    if (!owner || !offset) return false;
    auto* property = owner->GetPropertyByNameInChain(name);
    if (!property || property->GetElementSize() != static_cast<int32_t>(expected_size)) return false;
    const int32_t raw_offset = property->GetOffset_Internal();
    if (raw_offset < 0 || raw_offset > 0xFFFF) return false;
    *offset = static_cast<std::uint16_t>(raw_offset);
    return true;
}

template <typename T, std::size_t N>
void write_parameter(std::array<std::byte, N>& buffer,
                     std::uint16_t offset, const T& value) noexcept {
    std::memcpy(buffer.data() + offset, &value, sizeof(T));
}

} // namespace

std::atomic<PostRenderCanary*> PostRenderCanary::active_{};

PostRenderCanaryConfig PostRenderCanaryConfig::load(const std::filesystem::path& path) noexcept {
    PostRenderCanaryConfig result{};
    try {
        std::ifstream input{path};
        std::string line;
        while (std::getline(input, line)) {
            const auto comment = line.find_first_of("#;");
            if (comment != std::string::npos) line.resize(comment);
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;
            const std::string key = trim(line.substr(0, separator));
            const std::string value = trim(line.substr(separator + 1));
            if (key == "postrender_hook_enabled") result.hook_enabled = parse_bool(value, result.hook_enabled);
            else if (key == "postrender_canary_enabled") result.draw_enabled = parse_bool(value, result.draw_enabled);
            else if (key == "postrender_relative_marker_enabled") {
                result.relative_marker_enabled = parse_bool(value, result.relative_marker_enabled);
            }
            else if (key == "postrender_vtable_slot") {
                std::size_t slot{};
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), slot);
                if (parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size()) result.vtable_slot = slot;
            }
        }
    } catch (...) {
    }
    if (result.vtable_slot != 112) {
        result.hook_enabled = false;
        result.draw_enabled = false;
        result.relative_marker_enabled = false;
    }
    if (!result.hook_enabled) {
        result.draw_enabled = false;
        result.relative_marker_enabled = false;
    }
    return result;
}

PostRenderCanary::PostRenderCanary(PostRenderCanaryConfig config) noexcept : config_(config) {
    state_.store(config_.hook_enabled ? PostRenderHookState::waiting_for_viewport
                                     : PostRenderHookState::disabled,
                 std::memory_order_relaxed);
}

void PostRenderCanary::initialize() noexcept {
    if (!config_.hook_enabled) return;
    if (!compatible_build()) {
        fail(PostRenderHookState::incompatible_build);
        return;
    }
    if (!initialize_draw_metadata()) {
        fail(PostRenderHookState::invalid_draw_metadata);
        return;
    }
    PostRenderCanary* expected = nullptr;
    if (!active_.compare_exchange_strong(expected, this, std::memory_order_acq_rel)) {
        fail(PostRenderHookState::lost_ownership);
    }
}

void PostRenderCanary::observe_engine(UObject* engine) noexcept {
    if (!config_.hook_enabled || shutting_down_.load(std::memory_order_acquire)) return;
    const auto current_state = state_.load(std::memory_order_acquire);
    if (current_state == PostRenderHookState::installed || current_state != PostRenderHookState::waiting_for_viewport) return;
    const auto now = Clock::now();
    if (now < next_install_attempt_) return;
    next_install_attempt_ = now + std::chrono::seconds{1};
    if (!engine) {
        if (++viewport_wait_attempts_ >= 60) fail(PostRenderHookState::invalid_target);
        return;
    }
#if defined(_MSC_VER)
    __try {
#endif
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        if (!viewport) {
            if (++viewport_wait_attempts_ >= 60) fail(PostRenderHookState::invalid_target);
            return;
        }
        if (install_attempts_ >= 3) {
            fail(PostRenderHookState::invalid_target);
            return;
        }
        ++install_attempts_;
        if (viewport && install_for_viewport(viewport)) {
            state_.store(PostRenderHookState::installed, std::memory_order_release);
        }
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fail(PostRenderHookState::invalid_target);
    }
#endif
}

void PostRenderCanary::set_runtime_enabled(bool enabled) noexcept {
    runtime_enabled_.store(config_.draw_enabled && enabled && !shutting_down_.load(std::memory_order_acquire),
                           std::memory_order_release);
}

void PostRenderCanary::publish_relative_marker(std::uint64_t source_sequence, bool valid,
                                               double player_x, double player_y,
                                               double target_x, double target_y,
                                               double world_radius) noexcept {
    const bool finite = std::isfinite(player_x) && std::isfinite(player_y)
        && std::isfinite(target_x) && std::isfinite(target_y)
        && std::isfinite(world_radius) && world_radius > 0.0;
    const auto start = relative_marker_.sequence.fetch_add(1, std::memory_order_acq_rel) + 1U;
    relative_marker_.source_sequence.store(source_sequence, std::memory_order_relaxed);
    relative_marker_.player_x.store(std::bit_cast<std::uint64_t>(player_x), std::memory_order_relaxed);
    relative_marker_.player_y.store(std::bit_cast<std::uint64_t>(player_y), std::memory_order_relaxed);
    relative_marker_.target_x.store(std::bit_cast<std::uint64_t>(target_x), std::memory_order_relaxed);
    relative_marker_.target_y.store(std::bit_cast<std::uint64_t>(target_y), std::memory_order_relaxed);
    relative_marker_.world_radius.store(std::bit_cast<std::uint64_t>(world_radius), std::memory_order_relaxed);
    relative_marker_.valid.store(valid && finite, std::memory_order_relaxed);
    relative_marker_.sequence.store(start + 1U, std::memory_order_release);
}

void PostRenderCanary::shutdown_for_process_lifetime() noexcept {
    shutting_down_.store(true, std::memory_order_release);
    runtime_enabled_.store(false, std::memory_order_release);
    state_.store(PostRenderHookState::shutting_down, std::memory_order_release);
    restore_owned_slots();
    for (std::uint32_t attempt = 0; attempt < 200 && in_flight_.load(std::memory_order_acquire) != 0; ++attempt) {
        std::this_thread::yield();
    }
    // Keep the owner published for the pinned module's process lifetime. A
    // thread may have fetched the detour address immediately before slot
    // restoration and enter after the bounded in-flight observation. That
    // late callback must still find its original function. Custom drawing is
    // already permanently disabled above.
}

PostRenderHookState PostRenderCanary::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

std::uint64_t PostRenderCanary::callback_count() const noexcept {
    return callback_count_.load(std::memory_order_relaxed);
}

std::uint64_t PostRenderCanary::draw_count() const noexcept {
    return draw_count_.load(std::memory_order_relaxed);
}

std::uint64_t PostRenderCanary::fault_count() const noexcept {
    return fault_count_.load(std::memory_order_relaxed);
}

void __fastcall PostRenderCanary::detour(void* viewport, UObject* canvas) noexcept {
    auto* owner = active_.load(std::memory_order_acquire);
    if (!owner) return;
    owner->in_flight_.fetch_add(1, std::memory_order_acq_rel);
    const auto original = owner->original_for(viewport);
    if (!original) {
        owner->fail(PostRenderHookState::lost_ownership);
        owner->in_flight_.fetch_sub(1, std::memory_order_acq_rel);
        return;
    }

    original(viewport, canvas);
    owner->callback_count_.fetch_add(1, std::memory_order_relaxed);
    if (owner->runtime_enabled_.load(std::memory_order_acquire)
        && !owner->shutting_down_.load(std::memory_order_acquire)) {
        owner->draw_guarded(canvas);
    }
    owner->in_flight_.fetch_sub(1, std::memory_order_acq_rel);
}

PostRenderCanary::PostRenderFn PostRenderCanary::original_for(void* viewport) const noexcept {
    if (!viewport) return nullptr;
    auto* vtable = *reinterpret_cast<void***>(viewport);
    for (const auto& record : hooks_) {
        if (record.published.load(std::memory_order_acquire)
            && record.vtable_identity.load(std::memory_order_relaxed) == vtable) {
            return reinterpret_cast<PostRenderFn>(record.original.load(std::memory_order_relaxed));
        }
    }
    return nullptr;
}

void PostRenderCanary::draw_guarded(UObject* canvas) noexcept {
    if (!canvas || fault_count_.load(std::memory_order_acquire) >= kMaximumDrawFaults) return;
#if defined(_MSC_VER)
    __try {
        draw_canary(canvas);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
    }
#else
    try {
        draw_canary(canvas);
    } catch (...) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
    }
#endif
}

void PostRenderCanary::draw_canary(UObject* canvas) {
    draw_line(canvas, 48.0, 48.0, 80.0, 48.0, 3.0F, 0.1F, 1.0F, 0.25F, 0.9F);

    RelativeMarkerSnapshot marker{};
    if (config_.relative_marker_enabled && read_relative_marker(&marker) && marker.valid) {
        // Phase 2 visibility witness. If this unobstructed yellow cross is
        // visible while the projected cross is hidden, the numeric snapshot
        // and K2 draw path are valid and the compact minimap is composited
        // above GameViewportClient::PostRender.
        constexpr double witness_x = 112.0;
        constexpr double witness_y = 64.0;
        constexpr double witness_half = 8.0;
        draw_line(canvas, witness_x - witness_half, witness_y,
                  witness_x + witness_half, witness_y,
                  3.0F, 1.0F, 0.82F, 0.1F, 1.0F);
        draw_line(canvas, witness_x, witness_y - witness_half,
                  witness_x, witness_y + witness_half,
                  3.0F, 1.0F, 0.82F, 0.1F, 1.0F);
        std::int32_t canvas_width{};
        std::int32_t canvas_height{};
        const auto* bytes = reinterpret_cast<const std::byte*>(canvas);
        std::memcpy(&canvas_width, bytes + draw_line_.canvas_size_x_offset, sizeof(canvas_width));
        std::memcpy(&canvas_height, bytes + draw_line_.canvas_size_y_offset, sizeof(canvas_height));
        const auto projected = dswros::project_compact_radar_point(
            {marker.player_x, marker.player_y, 0.0},
            {marker.target_x, marker.target_y, 0.0},
            marker.world_radius,
            canvas_width,
            canvas_height);
        if (projected) {
            const double half = std::max(4.0, 7.0 * projected->display_scale);
            const float thickness = static_cast<float>(std::max(2.0, 3.0 * projected->display_scale));
            draw_line(canvas, projected->x - half, projected->y,
                      projected->x + half, projected->y,
                      thickness, 1.0F, 0.82F, 0.1F, 1.0F);
            draw_line(canvas, projected->x, projected->y - half,
                      projected->x, projected->y + half,
                      thickness, 1.0F, 0.82F, 0.1F, 1.0F);
        }
    }
    draw_count_.fetch_add(1, std::memory_order_relaxed);
}

void PostRenderCanary::draw_line(UObject* canvas, double ax, double ay,
                                 double bx, double by, float thickness,
                                 float red, float green, float blue, float alpha) {
    std::array<std::byte, kMaximumParameterBytes> parameters{};
    const Vec2 point_a{ax, ay};
    const Vec2 point_b{bx, by};
    const LinearColor color{red, green, blue, alpha};
    write_parameter(parameters, draw_line_.point_a_offset, point_a);
    write_parameter(parameters, draw_line_.point_b_offset, point_b);
    write_parameter(parameters, draw_line_.thickness_offset, thickness);
    write_parameter(parameters, draw_line_.color_offset, color);
    canvas->ProcessEvent(draw_line_.function, parameters.data());
}

bool PostRenderCanary::read_relative_marker(RelativeMarkerSnapshot* output) const noexcept {
    if (!output) return false;
    for (std::uint32_t attempt = 0; attempt < 2; ++attempt) {
        const auto before = relative_marker_.sequence.load(std::memory_order_acquire);
        if ((before & 1U) != 0) continue;
        RelativeMarkerSnapshot candidate{};
        candidate.source_sequence = relative_marker_.source_sequence.load(std::memory_order_relaxed);
        candidate.player_x = std::bit_cast<double>(relative_marker_.player_x.load(std::memory_order_relaxed));
        candidate.player_y = std::bit_cast<double>(relative_marker_.player_y.load(std::memory_order_relaxed));
        candidate.target_x = std::bit_cast<double>(relative_marker_.target_x.load(std::memory_order_relaxed));
        candidate.target_y = std::bit_cast<double>(relative_marker_.target_y.load(std::memory_order_relaxed));
        candidate.world_radius = std::bit_cast<double>(relative_marker_.world_radius.load(std::memory_order_relaxed));
        candidate.valid = relative_marker_.valid.load(std::memory_order_relaxed);
        const auto after = relative_marker_.sequence.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0) {
            *output = candidate;
            return true;
        }
    }
    return false;
}

bool PostRenderCanary::initialize_draw_metadata() noexcept {
    try {
        auto* function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kDrawLineFunction);
        auto* canvas_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kCanvasClass);
        if (!function || !canvas_class) return false;
        const auto size = function->GetParmsSize();
        if (size == 0 || size > kMaximumParameterBytes || size > 0xFFFF) return false;
        DrawLineMetadata candidate{};
        candidate.function = function;
        candidate.parameters_size = static_cast<std::uint16_t>(size);
        candidate.valid = property_layout(function, STR("ScreenPositionA"), sizeof(Vec2), &candidate.point_a_offset)
            && property_layout(function, STR("ScreenPositionB"), sizeof(Vec2), &candidate.point_b_offset)
            && property_layout(function, STR("Thickness"), sizeof(float), &candidate.thickness_offset)
            && property_layout(function, STR("RenderColor"), sizeof(LinearColor), &candidate.color_offset)
            && property_layout(canvas_class, STR("SizeX"), sizeof(std::int32_t), &candidate.canvas_size_x_offset)
            && property_layout(canvas_class, STR("SizeY"), sizeof(std::int32_t), &candidate.canvas_size_y_offset)
            && candidate.point_a_offset + sizeof(Vec2) <= size
            && candidate.point_b_offset + sizeof(Vec2) <= size
            && candidate.thickness_offset + sizeof(float) <= size
            && candidate.color_offset + sizeof(LinearColor) <= size;
        if (!candidate.valid) return false;
        draw_line_ = candidate;
        return true;
    } catch (...) {
        return false;
    }
}

bool PostRenderCanary::install_for_viewport(UObject* viewport) noexcept {
    if (!viewport) return false;
    auto* vtable = *reinterpret_cast<void***>(viewport);
    if (!vtable) return false;
    for (const auto& record : hooks_) {
        if (record.published.load(std::memory_order_acquire)
            && record.vtable_identity.load(std::memory_order_relaxed) == vtable) return true;
    }

    HookRecord* free_record = nullptr;
    for (auto& record : hooks_) {
        if (!record.published.load(std::memory_order_acquire)) {
            free_record = &record;
            break;
        }
    }
    if (!free_record) {
        fail(PostRenderHookState::capacity_exhausted);
        return false;
    }

    void** slot = &vtable[config_.vtable_slot];
    void* original = *slot;
    const auto game = module_image(GetModuleHandleW(nullptr));
    if (!executable_pointer(original) || !module_contains(game, original)) {
        fail(PostRenderHookState::invalid_target);
        return false;
    }

    free_record->slot_address.store(slot, std::memory_order_relaxed);
    free_record->original.store(original, std::memory_order_relaxed);
    free_record->vtable_identity.store(vtable, std::memory_order_relaxed);
    free_record->published.store(true, std::memory_order_release);

    DWORD previous_protection{};
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &previous_protection)) {
        free_record->published.store(false, std::memory_order_release);
        return false;
    }
    void* observed = InterlockedCompareExchangePointer(slot, reinterpret_cast<void*>(&detour), original);
    DWORD ignored{};
    const bool protection_restored = VirtualProtect(slot, sizeof(void*), previous_protection, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
    if (observed != original || !protection_restored) {
        if (observed == original && !protection_restored) {
            InterlockedCompareExchangePointer(slot, original, reinterpret_cast<void*>(&detour));
        }
        free_record->published.store(false, std::memory_order_release);
        fail(PostRenderHookState::lost_ownership);
        return false;
    }
    return true;
}

bool PostRenderCanary::compatible_build() const noexcept {
    const auto game = module_image(GetModuleHandleW(nullptr));
    const auto ue4ss = module_image(GetModuleHandleW(L"UE4SS.dll"));
    return game.valid && ue4ss.valid
        && game.timestamp == kExpectedGameTimestamp && game.image_size == kExpectedGameImageSize
        && ue4ss.timestamp == kExpectedUe4ssTimestamp && ue4ss.image_size == kExpectedUe4ssImageSize;
}

void PostRenderCanary::restore_owned_slots() noexcept {
    for (auto& record : hooks_) {
        if (!record.published.load(std::memory_order_acquire)) continue;
        void** slot = record.slot_address.load(std::memory_order_relaxed);
        void* original = record.original.load(std::memory_order_relaxed);
        if (!slot || !original) continue;
        DWORD previous_protection{};
        if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &previous_protection)) continue;
        InterlockedCompareExchangePointer(slot, original, reinterpret_cast<void*>(&detour));
        DWORD ignored{};
        VirtualProtect(slot, sizeof(void*), previous_protection, &ignored);
        FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
    }
}

void PostRenderCanary::fail(PostRenderHookState failure) noexcept {
    runtime_enabled_.store(false, std::memory_order_release);
    state_.store(failure, std::memory_order_release);
}

} // namespace dsnwr
