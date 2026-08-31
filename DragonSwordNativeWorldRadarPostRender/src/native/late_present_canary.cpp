#include "late_present_canary.hpp"

#include <dswros/render_projection.hpp>

#include <polyhook2/Detour/x64Detour.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

#include <windows.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace dsnwr {
namespace {

constexpr std::uint32_t kExpectedGameTimestamp = 0x691B0D98U;
constexpr std::uint32_t kExpectedGameImageSize = 0x09EB2000U;
constexpr std::uint32_t kExpectedUe4ssTimestamp = 0x6A76FCA3U;
constexpr std::uint32_t kExpectedUe4ssImageSize = 0x00FCA000U;
constexpr std::size_t kPresentSlot = 8;
constexpr std::size_t kResizeBuffersSlot = 13;
constexpr std::size_t kExecuteCommandListsSlot = 10;

struct ModuleImage {
    std::uint32_t timestamp{};
    std::uint32_t image_size{};
    bool valid{};
};

struct CrossRects {
    RECT values[2]{};
};

[[nodiscard]] CrossRects cross_rects(double x_value, double y_value,
                                     double scale) noexcept {
    const long x = static_cast<long>(std::lround(x_value));
    const long y = static_cast<long>(std::lround(y_value));
    const long half = static_cast<long>(std::max(5.0, 8.0 * scale));
    const long thickness = static_cast<long>(std::max(3.0, 4.0 * scale));
    const long low = thickness / 2;
    const long high = thickness - low;
    return {{{x - half, y - low, x + half, y + high},
             {x - low, y - half, x + high, y + half}}};
}

[[nodiscard]] ModuleImage module_image(HMODULE module) noexcept {
    if (!module) return {};
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) return {};
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base + static_cast<std::uintptr_t>(dos->e_lfanew));
    if (nt->Signature != IMAGE_NT_SIGNATURE
        || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC
        || nt->OptionalHeader.SizeOfImage == 0) {
        return {};
    }
    return {nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage, true};
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

} // namespace

std::atomic<LatePresentCanary*> LatePresentCanary::active_{};
std::atomic<ID3D12CommandQueue*> LatePresentCanary::captured_direct_queue_{};

LatePresentCanaryConfig LatePresentCanaryConfig::load(const std::filesystem::path& path) noexcept {
    LatePresentCanaryConfig result{};
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
            if (key == "late_present_hook_enabled") {
                result.hook_enabled = parse_bool(value, result.hook_enabled);
            } else if (key == "late_present_canary_enabled") {
                result.canary_enabled = parse_bool(value, result.canary_enabled);
            } else if (key == "late_present_relative_marker_enabled") {
                result.relative_marker_enabled = parse_bool(value, result.relative_marker_enabled);
            }
        }
    } catch (...) {
    }
    if (!result.hook_enabled) {
        result.canary_enabled = false;
        result.relative_marker_enabled = false;
    }
    return result;
}

LatePresentCanary::LatePresentCanary(LatePresentCanaryConfig config) noexcept : config_(config) {
    state_.store(config_.hook_enabled ? LatePresentState::waiting : LatePresentState::disabled,
                 std::memory_order_relaxed);
}

LatePresentCanary::~LatePresentCanary() = default;

void LatePresentCanary::initialize() noexcept {
    if (!config_.hook_enabled) return;
    if (!compatible_build()) {
        state_.store(LatePresentState::incompatible_build, std::memory_order_release);
        return;
    }
    LatePresentCanary* expected{};
    if (!active_.compare_exchange_strong(expected, this, std::memory_order_acq_rel)) {
        state_.store(LatePresentState::hook_failed, std::memory_order_release);
        return;
    }
    void* present{};
    void* resize_buffers{};
    void* execute_command_lists{};
    if (!discover_targets(&present, &resize_buffers, &execute_command_lists)) {
        state_.store(LatePresentState::discovery_failed, std::memory_order_release);
        return;
    }
    try {
        execute_hook_ = std::make_unique<PLH::x64Detour>(
            reinterpret_cast<std::uint64_t>(execute_command_lists),
            reinterpret_cast<std::uint64_t>(&execute_command_lists_detour),
            &execute_trampoline_);
        resize_hook_ = std::make_unique<PLH::x64Detour>(
            reinterpret_cast<std::uint64_t>(resize_buffers),
            reinterpret_cast<std::uint64_t>(&resize_buffers_detour),
            &resize_trampoline_);
        present_hook_ = std::make_unique<PLH::x64Detour>(
            reinterpret_cast<std::uint64_t>(present),
            reinterpret_cast<std::uint64_t>(&present_detour),
            &present_trampoline_);
        if (!execute_hook_->hook()) {
            state_.store(LatePresentState::hook_failed, std::memory_order_release);
            return;
        }
        if (!resize_hook_->hook()) {
            execute_hook_->unHook();
            state_.store(LatePresentState::hook_failed, std::memory_order_release);
            return;
        }
        if (!present_hook_->hook()) {
            resize_hook_->unHook();
            execute_hook_->unHook();
            state_.store(LatePresentState::hook_failed, std::memory_order_release);
            return;
        }
        state_.store(LatePresentState::installed, std::memory_order_release);
    } catch (...) {
        state_.store(LatePresentState::hook_failed, std::memory_order_release);
    }
}

void LatePresentCanary::set_runtime_enabled(bool enabled) noexcept {
    runtime_enabled_.store(config_.canary_enabled && enabled
                               && !shutting_down_.load(std::memory_order_acquire),
                           std::memory_order_release);
}

void LatePresentCanary::publish_relative_marker(std::uint64_t source_sequence, bool valid,
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

void LatePresentCanary::shutdown_for_process_lifetime() noexcept {
    shutting_down_.store(true, std::memory_order_release);
    runtime_enabled_.store(false, std::memory_order_release);
    state_.store(LatePresentState::shutting_down, std::memory_order_release);
    for (std::uint32_t attempt = 0; attempt < 200
         && in_flight_.load(std::memory_order_acquire) != 0; ++attempt) {
        std::this_thread::yield();
    }
    // The DLL and owner are pinned until process exit. The detours remain
    // chained but custom drawing is permanently disabled, avoiding a late
    // entry into an unloaded module during UE4SS hot-unload.
}

LatePresentState LatePresentCanary::state() const noexcept {
    return state_.load(std::memory_order_acquire);
}

std::uint64_t LatePresentCanary::callback_count() const noexcept {
    return callback_count_.load(std::memory_order_relaxed);
}

std::uint64_t LatePresentCanary::draw_count() const noexcept {
    return draw_count_.load(std::memory_order_relaxed);
}

std::uint64_t LatePresentCanary::fault_count() const noexcept {
    return fault_count_.load(std::memory_order_relaxed);
}

long __stdcall LatePresentCanary::present_detour(IDXGISwapChain* swap_chain,
                                                 unsigned int sync_interval,
                                                 unsigned int flags) noexcept {
    auto* owner = active_.load(std::memory_order_acquire);
    if (!owner || owner->present_trampoline_ == 0) return DXGI_ERROR_INVALID_CALL;
    owner->in_flight_.fetch_add(1, std::memory_order_acq_rel);
    owner->callback_count_.fetch_add(1, std::memory_order_relaxed);
    if (owner->runtime_enabled_.load(std::memory_order_acquire)
        && !owner->shutting_down_.load(std::memory_order_acquire)) {
        owner->draw_guarded(swap_chain);
    }
    const auto original = reinterpret_cast<PresentFn>(owner->present_trampoline_);
    const long result = original(swap_chain, sync_interval, flags);
    owner->in_flight_.fetch_sub(1, std::memory_order_acq_rel);
    return result;
}

long __stdcall LatePresentCanary::resize_buffers_detour(
    IDXGISwapChain* swap_chain, unsigned int buffer_count,
    unsigned int width, unsigned int height, int format,
    unsigned int flags) noexcept {
    auto* owner = active_.load(std::memory_order_acquire);
    if (!owner || owner->resize_trampoline_ == 0) return DXGI_ERROR_INVALID_CALL;
    owner->release_render_target(swap_chain);
    const auto original = reinterpret_cast<ResizeBuffersFn>(owner->resize_trampoline_);
    return original(swap_chain, buffer_count, width, height, format, flags);
}

void __stdcall LatePresentCanary::execute_command_lists_detour(
    ID3D12CommandQueue* queue, unsigned int count,
    ID3D12CommandList* const* command_lists) noexcept {
    auto* owner = active_.load(std::memory_order_acquire);
    if (!owner || owner->execute_trampoline_ == 0) return;
    if (queue && queue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT
        && !captured_direct_queue_.load(std::memory_order_acquire)) {
        queue->AddRef();
        ID3D12CommandQueue* expected{};
        if (!captured_direct_queue_.compare_exchange_strong(
                expected, queue, std::memory_order_acq_rel)) {
            queue->Release();
        }
    }
    const auto original = reinterpret_cast<ExecuteCommandListsFn>(owner->execute_trampoline_);
    original(queue, count, command_lists);
}

void LatePresentCanary::draw_guarded(IDXGISwapChain* swap_chain) noexcept {
    if (!swap_chain || fault_count_.load(std::memory_order_acquire) != 0) return;
#if defined(_MSC_VER)
    __try {
        draw(swap_chain);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
        state_.store(LatePresentState::render_failed, std::memory_order_release);
    }
#else
    try {
        draw(swap_chain);
    } catch (...) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
        state_.store(LatePresentState::render_failed, std::memory_order_release);
    }
#endif
}

void LatePresentCanary::draw(IDXGISwapChain* swap_chain) {
    if (!accept_swap_chain(swap_chain)) return;
    if (!ensure_render_target(swap_chain)) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
        state_.store(LatePresentState::render_failed, std::memory_order_release);
        return;
    }

    RelativeMarkerSnapshot marker{};
    const RelativeMarkerSnapshot* marker_pointer{};
    if (config_.relative_marker_enabled && read_relative_marker(&marker) && marker.valid) {
        marker_pointer = &marker;
    }
    const bool d3d11_drawn = backend_ == 1U && draw_d3d11(marker_pointer);
    const auto d3d12_result = backend_ == 2U
        ? draw_d3d12(marker_pointer) : D3d12DrawResult::failed;
    if (d3d11_drawn || d3d12_result == D3d12DrawResult::drawn) {
        draw_count_.fetch_add(1, std::memory_order_relaxed);
    } else if (backend_ == 1U
               || (backend_ == 2U && d3d12_result == D3d12DrawResult::failed
                   && d3d12_command_list_)) {
        fault_count_.fetch_add(1, std::memory_order_acq_rel);
        runtime_enabled_.store(false, std::memory_order_release);
        state_.store(LatePresentState::render_failed, std::memory_order_release);
    }
}

bool LatePresentCanary::accept_swap_chain(IDXGISwapChain* swap_chain) noexcept {
    if (accepted_swap_chain_) return accepted_swap_chain_ == swap_chain;
    DXGI_SWAP_CHAIN_DESC description{};
    if (FAILED(swap_chain->GetDesc(&description)) || !description.OutputWindow) return false;
    DWORD process_id{};
    GetWindowThreadProcessId(description.OutputWindow, &process_id);
    RECT client{};
    if (process_id != GetCurrentProcessId() || !GetClientRect(description.OutputWindow, &client)
        || client.right - client.left < 800 || client.bottom - client.top < 600) {
        return false;
    }
    accepted_swap_chain_ = swap_chain;
    return true;
}

bool LatePresentCanary::ensure_render_target(IDXGISwapChain* swap_chain) noexcept {
    if (backend_ == 1U && render_target_ && context_ && device_) return true;
    if (backend_ == 2U) return ensure_d3d12_render_target(swap_chain);
    release_all_render_resources();
    if (ensure_d3d11_render_target(swap_chain)) {
        backend_ = 1U;
        return true;
    }
    if (ensure_d3d12_render_target(swap_chain)) {
        backend_ = 2U;
        return true;
    }
    return false;
}

bool LatePresentCanary::ensure_d3d11_render_target(IDXGISwapChain* swap_chain) noexcept {
    ID3D11Device* device{};
    if (FAILED(swap_chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device)))
        || !device) {
        return false;
    }
    ID3D11DeviceContext* base_context{};
    device->GetImmediateContext(&base_context);
    ID3D11DeviceContext1* context{};
    if (!base_context
        || FAILED(base_context->QueryInterface(__uuidof(ID3D11DeviceContext1),
                                               reinterpret_cast<void**>(&context)))) {
        if (base_context) base_context->Release();
        device->Release();
        return false;
    }
    base_context->Release();
    ID3D11Texture2D* buffer{};
    if (FAILED(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                     reinterpret_cast<void**>(&buffer))) || !buffer) {
        context->Release();
        device->Release();
        return false;
    }
    D3D11_TEXTURE2D_DESC description{};
    buffer->GetDesc(&description);
    ID3D11RenderTargetView* render_target{};
    const long result = device->CreateRenderTargetView(buffer, nullptr, &render_target);
    buffer->Release();
    if (FAILED(result) || !render_target || description.Width == 0 || description.Height == 0) {
        if (render_target) render_target->Release();
        context->Release();
        device->Release();
        return false;
    }
    device_ = device;
    context_ = context;
    render_target_ = render_target;
    buffer_width_ = description.Width;
    buffer_height_ = description.Height;
    return true;
}

bool LatePresentCanary::ensure_d3d12_render_target(IDXGISwapChain* swap_chain) noexcept {
    if (d3d12_command_list_ && d3d12_fence_ && d3d12_rtv_heap_
        && d3d12_buffer_count_ > 0 && d3d12_queue_) {
        return true;
    }
    ID3D12Device* detected_device{};
    if (FAILED(swap_chain->GetDevice(__uuidof(ID3D12Device),
                                     reinterpret_cast<void**>(&detected_device)))
        || !detected_device) {
        return false;
    }
    ID3D12CommandQueue* detected_queue = captured_direct_queue_.load(std::memory_order_acquire);
    if (!detected_queue) {
        detected_device->Release();
        backend_ = 2U;
        return true;
    }
    ID3D12Device* queue_device{};
    if (FAILED(detected_queue->GetDevice(__uuidof(ID3D12Device),
                                         reinterpret_cast<void**>(&queue_device)))
        || !queue_device) {
        detected_device->Release();
        return false;
    }
    IUnknown* detected_identity{};
    IUnknown* queue_identity{};
    detected_device->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&detected_identity));
    queue_device->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&queue_identity));
    const bool same_device = detected_identity && queue_identity
        && detected_identity == queue_identity;
    if (detected_identity) detected_identity->Release();
    if (queue_identity) queue_identity->Release();
    queue_device->Release();
    if (!same_device) {
        detected_device->Release();
        return false;
    }

    IDXGISwapChain3* swap_chain3{};
    if (FAILED(swap_chain->QueryInterface(__uuidof(IDXGISwapChain3),
                                          reinterpret_cast<void**>(&swap_chain3)))
        || !swap_chain3) {
        detected_device->Release();
        return false;
    }
    DXGI_SWAP_CHAIN_DESC description{};
    if (FAILED(swap_chain->GetDesc(&description)) || description.BufferCount == 0
        || description.BufferCount
            > static_cast<std::uint32_t>(d3d12_buffers_.size())) {
        swap_chain3->Release();
        detected_device->Release();
        return false;
    }

    d3d12_device_ = detected_device;
    d3d12_queue_ = detected_queue;
    d3d12_queue_->AddRef();
    swap_chain3_ = swap_chain3;
    d3d12_buffer_count_ = description.BufferCount;

    D3D12_DESCRIPTOR_HEAP_DESC heap_description{};
    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_description.NumDescriptors = d3d12_buffer_count_;
    if (FAILED(d3d12_device_->CreateDescriptorHeap(
            &heap_description, __uuidof(ID3D12DescriptorHeap),
            reinterpret_cast<void**>(&d3d12_rtv_heap_)))) {
        release_d3d12_resources(false);
        return false;
    }
    d3d12_rtv_increment_ = d3d12_device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto handle = d3d12_rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    for (std::uint32_t index = 0; index < d3d12_buffer_count_; ++index) {
        if (FAILED(swap_chain->GetBuffer(index, __uuidof(ID3D12Resource),
                                         reinterpret_cast<void**>(&d3d12_buffers_[index])))
            || FAILED(d3d12_device_->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator),
                reinterpret_cast<void**>(&d3d12_allocators_[index])))) {
            release_d3d12_resources(false);
            return false;
        }
        d3d12_device_->CreateRenderTargetView(d3d12_buffers_[index], nullptr, handle);
        handle.ptr += d3d12_rtv_increment_;
    }
    D3D12_RESOURCE_DESC buffer_description = d3d12_buffers_[0]->GetDesc();
    buffer_width_ = static_cast<std::uint32_t>(buffer_description.Width);
    buffer_height_ = buffer_description.Height;
    if (buffer_width_ == 0 || buffer_height_ == 0
        || FAILED(d3d12_device_->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12_allocators_[0], nullptr,
            __uuidof(ID3D12GraphicsCommandList),
            reinterpret_cast<void**>(&d3d12_command_list_)))
        || FAILED(d3d12_command_list_->Close())
        || FAILED(d3d12_device_->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence),
            reinterpret_cast<void**>(&d3d12_fence_)))) {
        release_d3d12_resources(false);
        return false;
    }
    d3d12_fence_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!d3d12_fence_event_) {
        release_d3d12_resources(false);
        return false;
    }
    return true;
}

bool LatePresentCanary::draw_d3d11(const RelativeMarkerSnapshot* marker) {
    const float cyan[4]{0.05F, 0.9F, 1.0F, 1.0F};
    const D3D11_RECT canary[2]{{88, 62, 120, 66}, {102, 48, 106, 80}};
    context_->ClearView(render_target_, cyan, canary, 2);
    if (marker) {
        const auto projected = dswros::project_compact_radar_point(
            {marker->player_x, marker->player_y, 0.0},
            {marker->target_x, marker->target_y, 0.0}, marker->world_radius,
            static_cast<int>(buffer_width_), static_cast<int>(buffer_height_));
        if (projected) {
            const auto rectangles = cross_rects(projected->x, projected->y,
                                                projected->display_scale);
            const float yellow[4]{1.0F, 0.82F, 0.1F, 1.0F};
            context_->ClearView(render_target_, yellow, rectangles.values, 2);
        }
    }
    return true;
}

LatePresentCanary::D3d12DrawResult LatePresentCanary::draw_d3d12(
    const RelativeMarkerSnapshot* marker) {
    if (!d3d12_command_list_ || !d3d12_fence_ || !d3d12_queue_
        || !swap_chain3_ || !d3d12_fence_event_) {
        return D3d12DrawResult::failed;
    }
    const std::uint32_t index = swap_chain3_->GetCurrentBackBufferIndex();
    if (index >= d3d12_buffer_count_ || !d3d12_buffers_[index]
        || !d3d12_allocators_[index]) {
        return D3d12DrawResult::failed;
    }
    const std::uint64_t pending = d3d12_frame_fences_[index];
    if (pending != 0 && d3d12_fence_->GetCompletedValue() < pending) {
        return D3d12DrawResult::skipped;
    }
    if (FAILED(d3d12_allocators_[index]->Reset())
        || FAILED(d3d12_command_list_->Reset(d3d12_allocators_[index], nullptr))) {
        return D3d12DrawResult::failed;
    }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = d3d12_buffers_[index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    d3d12_command_list_->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        d3d12_rtv_heap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<std::size_t>(index) * d3d12_rtv_increment_;
    const float cyan[4]{0.05F, 0.9F, 1.0F, 1.0F};
    const D3D12_RECT canary[2]{{88, 62, 120, 66}, {102, 48, 106, 80}};
    d3d12_command_list_->ClearRenderTargetView(handle, cyan, 2, canary);
    if (marker) {
        const auto projected = dswros::project_compact_radar_point(
            {marker->player_x, marker->player_y, 0.0},
            {marker->target_x, marker->target_y, 0.0}, marker->world_radius,
            static_cast<int>(buffer_width_), static_cast<int>(buffer_height_));
        if (projected) {
            const auto rectangles = cross_rects(projected->x, projected->y,
                                                projected->display_scale);
            const float yellow[4]{1.0F, 0.82F, 0.1F, 1.0F};
            d3d12_command_list_->ClearRenderTargetView(
                handle, yellow, 2, rectangles.values);
        }
    }
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    d3d12_command_list_->ResourceBarrier(1, &barrier);
    if (FAILED(d3d12_command_list_->Close())) return D3d12DrawResult::failed;
    ID3D12CommandList* lists[]{d3d12_command_list_};
    d3d12_queue_->ExecuteCommandLists(1, lists);
    const std::uint64_t fence_value = ++d3d12_next_fence_;
    if (FAILED(d3d12_queue_->Signal(d3d12_fence_, fence_value))) {
        return D3d12DrawResult::failed;
    }
    d3d12_frame_fences_[index] = fence_value;
    return D3d12DrawResult::drawn;
}

void LatePresentCanary::release_render_target(IDXGISwapChain* swap_chain) noexcept {
    if (swap_chain != accepted_swap_chain_) return;
    if (render_target_) {
        render_target_->Release();
        render_target_ = nullptr;
    }
    release_d3d12_resources(true);
    backend_ = 0;
    buffer_width_ = 0;
    buffer_height_ = 0;
}

void LatePresentCanary::release_all_render_resources() noexcept {
    if (render_target_) render_target_->Release();
    if (context_) context_->Release();
    if (device_) device_->Release();
    render_target_ = nullptr;
    context_ = nullptr;
    device_ = nullptr;
    release_d3d12_resources(false);
    backend_ = 0;
    buffer_width_ = 0;
    buffer_height_ = 0;
}

void LatePresentCanary::release_d3d12_resources(bool wait_for_gpu) noexcept {
    if (wait_for_gpu && d3d12_queue_ && d3d12_fence_ && d3d12_fence_event_) {
        const std::uint64_t value = ++d3d12_next_fence_;
        if (SUCCEEDED(d3d12_queue_->Signal(d3d12_fence_, value))
            && d3d12_fence_->GetCompletedValue() < value
            && SUCCEEDED(d3d12_fence_->SetEventOnCompletion(
                value, static_cast<HANDLE>(d3d12_fence_event_)))) {
            WaitForSingleObject(static_cast<HANDLE>(d3d12_fence_event_), 2000);
        }
    }
    if (d3d12_command_list_) d3d12_command_list_->Release();
    for (auto*& allocator : d3d12_allocators_) {
        if (allocator) allocator->Release();
        allocator = nullptr;
    }
    for (auto*& buffer : d3d12_buffers_) {
        if (buffer) buffer->Release();
        buffer = nullptr;
    }
    if (d3d12_rtv_heap_) d3d12_rtv_heap_->Release();
    if (d3d12_fence_) d3d12_fence_->Release();
    if (swap_chain3_) swap_chain3_->Release();
    if (d3d12_queue_) d3d12_queue_->Release();
    if (d3d12_device_) d3d12_device_->Release();
    if (d3d12_fence_event_) CloseHandle(static_cast<HANDLE>(d3d12_fence_event_));
    d3d12_command_list_ = nullptr;
    d3d12_rtv_heap_ = nullptr;
    d3d12_fence_ = nullptr;
    swap_chain3_ = nullptr;
    d3d12_queue_ = nullptr;
    d3d12_device_ = nullptr;
    d3d12_fence_event_ = nullptr;
    d3d12_frame_fences_.fill(0);
    d3d12_next_fence_ = 0;
    d3d12_buffer_count_ = 0;
    d3d12_rtv_increment_ = 0;
}

bool LatePresentCanary::read_relative_marker(RelativeMarkerSnapshot* output) const noexcept {
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

bool LatePresentCanary::compatible_build() const noexcept {
    const auto game = module_image(GetModuleHandleW(nullptr));
    const auto ue4ss = module_image(GetModuleHandleW(L"UE4SS.dll"));
    return game.valid && ue4ss.valid
        && game.timestamp == kExpectedGameTimestamp && game.image_size == kExpectedGameImageSize
        && ue4ss.timestamp == kExpectedUe4ssTimestamp && ue4ss.image_size == kExpectedUe4ssImageSize;
}

bool LatePresentCanary::discover_targets(void** present, void** resize_buffers,
                                         void** execute_command_lists) noexcept {
    if (!present || !resize_buffers || !execute_command_lists) return false;
    HWND window = CreateWindowExW(0, L"STATIC", L"DSNWRPR_DXGI_DISCOVERY", WS_POPUP,
                                  0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return false;
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = 1;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = window;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* swap_chain{};
    ID3D11Device* device{};
    ID3D11DeviceContext* context{};
    const D3D_FEATURE_LEVEL requested[]{D3D_FEATURE_LEVEL_11_0};
    const long result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        requested, 1, D3D11_SDK_VERSION, &description,
        &swap_chain, &device, nullptr, &context);
    if (SUCCEEDED(result) && swap_chain) {
        auto** vtable = *reinterpret_cast<void***>(swap_chain);
        *present = vtable[kPresentSlot];
        *resize_buffers = vtable[kResizeBuffersSlot];
    }
    if (context) context->Release();
    if (device) device->Release();
    if (swap_chain) swap_chain->Release();
    ID3D12Device* device12{};
    ID3D12CommandQueue* queue12{};
    const long d3d12_result = D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device),
        reinterpret_cast<void**>(&device12));
    if (SUCCEEDED(d3d12_result) && device12) {
        D3D12_COMMAND_QUEUE_DESC queue_description{};
        queue_description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if (SUCCEEDED(device12->CreateCommandQueue(
                &queue_description, __uuidof(ID3D12CommandQueue),
                reinterpret_cast<void**>(&queue12))) && queue12) {
            auto** queue_vtable = *reinterpret_cast<void***>(queue12);
            *execute_command_lists = queue_vtable[kExecuteCommandListsSlot];
        }
    }
    if (queue12) queue12->Release();
    if (device12) device12->Release();
    DestroyWindow(window);
    return SUCCEEDED(result) && SUCCEEDED(d3d12_result)
        && executable_pointer(*present) && executable_pointer(*resize_buffers)
        && executable_pointer(*execute_command_lists);
}

} // namespace dsnwr
