#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext1;
struct ID3D11RenderTargetView;
struct ID3D12CommandAllocator;
struct ID3D12CommandList;
struct ID3D12CommandQueue;
struct ID3D12DescriptorHeap;
struct ID3D12Device;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct IDXGISwapChain3;

namespace PLH {
class x64Detour;
}

namespace dsnwr {

struct LatePresentCanaryConfig {
    bool hook_enabled{};
    bool canary_enabled{};
    bool relative_marker_enabled{};

    static LatePresentCanaryConfig load(const std::filesystem::path& path) noexcept;
};

enum class LatePresentState : std::uint32_t {
    disabled,
    waiting,
    installed,
    incompatible_build,
    discovery_failed,
    hook_failed,
    render_failed,
    shutting_down,
};

class LatePresentCanary final {
public:
    explicit LatePresentCanary(LatePresentCanaryConfig config) noexcept;
    ~LatePresentCanary();

    LatePresentCanary(const LatePresentCanary&) = delete;
    LatePresentCanary& operator=(const LatePresentCanary&) = delete;

    void initialize() noexcept;
    void set_runtime_enabled(bool enabled) noexcept;
    void publish_relative_marker(std::uint64_t source_sequence, bool valid,
                                 double player_x, double player_y,
                                 double target_x, double target_y,
                                 double world_radius) noexcept;
    void shutdown_for_process_lifetime() noexcept;

    [[nodiscard]] LatePresentState state() const noexcept;
    [[nodiscard]] std::uint64_t callback_count() const noexcept;
    [[nodiscard]] std::uint64_t draw_count() const noexcept;
    [[nodiscard]] std::uint64_t fault_count() const noexcept;

private:
    using PresentFn = long(__stdcall*)(IDXGISwapChain*, unsigned int, unsigned int);
    using ResizeBuffersFn = long(__stdcall*)(IDXGISwapChain*, unsigned int, unsigned int,
                                             unsigned int, int, unsigned int);
    using ExecuteCommandListsFn = void(__stdcall*)(ID3D12CommandQueue*, unsigned int,
                                                   ID3D12CommandList* const*);

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

    static long __stdcall present_detour(IDXGISwapChain* swap_chain,
                                         unsigned int sync_interval,
                                         unsigned int flags) noexcept;
    static long __stdcall resize_buffers_detour(IDXGISwapChain* swap_chain,
                                                unsigned int buffer_count,
                                                unsigned int width,
                                                unsigned int height,
                                                int format,
                                                unsigned int flags) noexcept;
    static void __stdcall execute_command_lists_detour(
        ID3D12CommandQueue* queue, unsigned int count,
        ID3D12CommandList* const* command_lists) noexcept;

    void draw_guarded(IDXGISwapChain* swap_chain) noexcept;
    void draw(IDXGISwapChain* swap_chain);
    [[nodiscard]] bool accept_swap_chain(IDXGISwapChain* swap_chain) noexcept;
    [[nodiscard]] bool ensure_render_target(IDXGISwapChain* swap_chain) noexcept;
    [[nodiscard]] bool ensure_d3d11_render_target(IDXGISwapChain* swap_chain) noexcept;
    [[nodiscard]] bool ensure_d3d12_render_target(IDXGISwapChain* swap_chain) noexcept;
    [[nodiscard]] bool draw_d3d11(const RelativeMarkerSnapshot* marker);
    enum class D3d12DrawResult : std::uint8_t { drawn, skipped, failed };
    [[nodiscard]] D3d12DrawResult draw_d3d12(const RelativeMarkerSnapshot* marker);
    void release_render_target(IDXGISwapChain* swap_chain) noexcept;
    void release_all_render_resources() noexcept;
    void release_d3d12_resources(bool wait_for_gpu) noexcept;
    [[nodiscard]] bool read_relative_marker(RelativeMarkerSnapshot* output) const noexcept;
    [[nodiscard]] bool compatible_build() const noexcept;
    [[nodiscard]] bool discover_targets(void** present, void** resize_buffers,
                                        void** execute_command_lists) noexcept;

    LatePresentCanaryConfig config_{};
    AtomicRelativeMarker relative_marker_{};
    std::unique_ptr<PLH::x64Detour> present_hook_{};
    std::unique_ptr<PLH::x64Detour> resize_hook_{};
    std::unique_ptr<PLH::x64Detour> execute_hook_{};
    std::uint64_t present_trampoline_{};
    std::uint64_t resize_trampoline_{};
    std::uint64_t execute_trampoline_{};
    IDXGISwapChain* accepted_swap_chain_{};
    ID3D11Device* device_{};
    ID3D11DeviceContext1* context_{};
    ID3D11RenderTargetView* render_target_{};
    std::uint32_t buffer_width_{};
    std::uint32_t buffer_height_{};
    IDXGISwapChain3* swap_chain3_{};
    ID3D12Device* d3d12_device_{};
    ID3D12CommandQueue* d3d12_queue_{};
    ID3D12DescriptorHeap* d3d12_rtv_heap_{};
    ID3D12GraphicsCommandList* d3d12_command_list_{};
    ID3D12Fence* d3d12_fence_{};
    void* d3d12_fence_event_{};
    std::array<ID3D12Resource*, 8> d3d12_buffers_{};
    std::array<ID3D12CommandAllocator*, 8> d3d12_allocators_{};
    std::array<std::uint64_t, 8> d3d12_frame_fences_{};
    std::uint64_t d3d12_next_fence_{};
    std::uint32_t d3d12_buffer_count_{};
    std::uint32_t d3d12_rtv_increment_{};
    std::uint32_t backend_{};
    std::atomic<LatePresentState> state_{LatePresentState::disabled};
    std::atomic<bool> runtime_enabled_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<std::uint32_t> in_flight_{};
    std::atomic<std::uint64_t> callback_count_{};
    std::atomic<std::uint64_t> draw_count_{};
    std::atomic<std::uint64_t> fault_count_{};

    static std::atomic<LatePresentCanary*> active_;
    static std::atomic<ID3D12CommandQueue*> captured_direct_queue_;
};

} // namespace dsnwr
