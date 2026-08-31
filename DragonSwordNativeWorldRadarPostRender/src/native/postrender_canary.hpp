#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace RC::Unreal {
class UObject;
class UFunction;
}

namespace dsnwr {

struct PostRenderCanaryConfig {
    bool hook_enabled{};
    bool draw_enabled{};
    bool relative_marker_enabled{};
    std::size_t vtable_slot{112};

    static PostRenderCanaryConfig load(const std::filesystem::path& path) noexcept;
};

enum class PostRenderHookState : std::uint32_t {
    disabled,
    waiting_for_viewport,
    installed,
    shutting_down,
    incompatible_build,
    invalid_draw_metadata,
    invalid_target,
    lost_ownership,
    capacity_exhausted,
};

class PostRenderCanary final {
public:
    explicit PostRenderCanary(PostRenderCanaryConfig config) noexcept;
    ~PostRenderCanary() = default;

    PostRenderCanary(const PostRenderCanary&) = delete;
    PostRenderCanary& operator=(const PostRenderCanary&) = delete;

    void initialize() noexcept;
    void observe_engine(RC::Unreal::UObject* engine) noexcept;
    void set_runtime_enabled(bool enabled) noexcept;
    void publish_relative_marker(std::uint64_t source_sequence, bool valid,
                                 double player_x, double player_y,
                                 double target_x, double target_y,
                                 double world_radius) noexcept;
    void shutdown_for_process_lifetime() noexcept;

    [[nodiscard]] PostRenderHookState state() const noexcept;
    [[nodiscard]] std::uint64_t callback_count() const noexcept;
    [[nodiscard]] std::uint64_t draw_count() const noexcept;
    [[nodiscard]] std::uint64_t fault_count() const noexcept;

private:
    using PostRenderFn = void(__fastcall*)(void*, RC::Unreal::UObject*);

    struct HookRecord {
        std::atomic<void**> slot_address{};
        std::atomic<void*> original{};
        std::atomic<void*> vtable_identity{};
        std::atomic<bool> published{};
    };

    struct DrawLineMetadata {
        RC::Unreal::UFunction* function{};
        std::uint16_t parameters_size{};
        std::uint16_t point_a_offset{};
        std::uint16_t point_b_offset{};
        std::uint16_t thickness_offset{};
        std::uint16_t color_offset{};
        std::uint16_t canvas_size_x_offset{};
        std::uint16_t canvas_size_y_offset{};
        bool valid{};
    };

    struct AtomicRelativeMarker {
        std::atomic<std::uint64_t> sequence{};
        std::atomic<std::uint64_t> source_sequence{};
        std::atomic<std::uint64_t> player_x{};
        std::atomic<std::uint64_t> player_y{};
        std::atomic<std::uint64_t> target_x{};
        std::atomic<std::uint64_t> target_y{};
        std::atomic<std::uint64_t> world_radius{};
        std::atomic<bool> valid{};
    };

    struct RelativeMarkerSnapshot {
        std::uint64_t source_sequence{};
        double player_x{};
        double player_y{};
        double target_x{};
        double target_y{};
        double world_radius{};
        bool valid{};
    };

    static constexpr std::size_t kMaxHookRecords = 4;
    static constexpr std::size_t kMaximumParameterBytes = 128;
    static constexpr std::uint32_t kMaximumDrawFaults = 1;

    static void __fastcall detour(void* viewport, RC::Unreal::UObject* canvas) noexcept;
    [[nodiscard]] PostRenderFn original_for(void* viewport) const noexcept;
    void draw_guarded(RC::Unreal::UObject* canvas) noexcept;
    void draw_canary(RC::Unreal::UObject* canvas);
    void draw_line(RC::Unreal::UObject* canvas, double ax, double ay,
                   double bx, double by, float thickness,
                   float red, float green, float blue, float alpha);
    [[nodiscard]] bool read_relative_marker(RelativeMarkerSnapshot* output) const noexcept;
    [[nodiscard]] bool initialize_draw_metadata() noexcept;
    [[nodiscard]] bool install_for_viewport(RC::Unreal::UObject* viewport) noexcept;
    [[nodiscard]] bool compatible_build() const noexcept;
    void restore_owned_slots() noexcept;
    void fail(PostRenderHookState state) noexcept;

    PostRenderCanaryConfig config_{};
    DrawLineMetadata draw_line_{};
    AtomicRelativeMarker relative_marker_{};
    std::array<HookRecord, kMaxHookRecords> hooks_{};
    std::atomic<PostRenderHookState> state_{PostRenderHookState::disabled};
    std::atomic<bool> runtime_enabled_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<std::uint32_t> in_flight_{};
    std::atomic<std::uint64_t> callback_count_{};
    std::atomic<std::uint64_t> draw_count_{};
    std::atomic<std::uint64_t> fault_count_{};
    std::chrono::steady_clock::time_point next_install_attempt_{};
    std::uint32_t install_attempts_{};
    std::uint32_t viewport_wait_attempts_{};

    static std::atomic<PostRenderCanary*> active_;
};

} // namespace dsnwr
