// DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1 targeting pinned RE-UE4SS v3.0.1.
//
// Version 1.3.1 preserves the owner-accepted automatic action, mounted Rider,
// fish, and closed target policy while enforcing one in-flight injection,
// bounded exact-candidate recovery, a physical toggle-key edge, and bounded
// interval diagnostics. A read-only, exact-UFunction post observer releases the
// in-flight gate when the game consumes the injected interaction input. Owned
// range PAKs author both interaction capsules and
// DropItemActor overlap spheres; native runtime range multiplication is disabled
// so those authored values can never be applied twice.
// There is no UObject/Actor scan, cross-World gameplay-object cache, direct
// interaction RPC, target-field access, Windows input, or worker.

#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/pe_runtime.hpp>
#include <dsnap/selector_resolver.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#include "status_toast_renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Mod/CppUserModBase.hpp>
#pragma warning(disable : 4251 4324 5038)
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Zydis/Zydis.h>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <windows.h>
#include <psapi.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

extern "C" IMAGE_DOS_HEADER __ImageBase;

constexpr auto kVersion = STR("1.3.1");
constexpr auto kLabel = "DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1";
constexpr bool kNativeDropItemRangeBridgeEnabled = false;
constexpr auto kDropItemClassPath = STR("/Script/DS.DropItemActor");
constexpr auto kInteractableClassPath = STR("/Script/DS.DInteractableComponent");
constexpr auto kSphereComponentClassPath = STR("/Script/Engine.SphereComponent");
constexpr auto kSetSphereRadiusFunctionPath = STR("/Script/Engine.SphereComponent:SetSphereRadius");
constexpr auto kServerRunInteractFunctionPath =
    STR("/Script/DS.DInteractableComponent:Server_RunInteractV2");
constexpr auto kSetInteractUiFunctionPath =
    STR("/Script/DS.DInteractableComponent:SetInteractUIV2");
constexpr auto kEnhancedPlayerInputClassPath = STR("/Script/EnhancedInput.EnhancedPlayerInput");
constexpr auto kInputActionClassPath = STR("/Script/EnhancedInput.InputAction");
constexpr auto kEnhancedActionMappingStructPath = STR("/Script/EnhancedInput.EnhancedActionKeyMapping");
constexpr auto kKeyStructPath = STR("/Script/InputCore.Key");
constexpr auto kEnhancedInputSubsystemClassPath = STR("/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem");
constexpr auto kSubsystemLibraryClassPath = STR("/Script/Engine.SubsystemBlueprintLibrary");
constexpr auto kGetLocalPlayerSubsystemFunctionPath =
    STR("/Script/Engine.SubsystemBlueprintLibrary:GetLocalPlayerSubSystemFromPlayerController");
constexpr auto kInjectInputVectorFunctionPath =
    STR("/Script/EnhancedInput.EnhancedInputSubsystemInterface:InjectInputVectorForAction");
constexpr std::string_view kSelectorResolutionPolicy =
    "runtime_reflection_dual_caller_rel32_consensus_fail_closed";
constexpr std::size_t kReflectedExecDecodeLimit = 96;
constexpr auto kPulseInterval = std::chrono::milliseconds{25};
constexpr auto kActiveScanInterval = std::chrono::milliseconds{0};
constexpr auto kIdleScanInterval = std::chrono::milliseconds{33};
constexpr auto kTransientBackoff = std::chrono::milliseconds{500};
constexpr auto kFaultBackoff = std::chrono::milliseconds{1000};
constexpr auto kPostPickupCooldown = std::chrono::milliseconds{0};
constexpr auto kWorldSettleDelay = std::chrono::milliseconds{1500};
constexpr auto kReadinessProbeInterval = std::chrono::milliseconds{250};
constexpr auto kRuntimeInitializationRetryInterval = std::chrono::milliseconds{250};
constexpr auto kRuntimeInitializationTimeout = std::chrono::seconds{30};
constexpr auto kSelectorAttemptBudget = std::chrono::milliseconds{250};
constexpr auto kSlowTickThreshold = std::chrono::microseconds{2000};
constexpr auto kSlowTickLogInterval = std::chrono::seconds{1};
constexpr std::uint32_t kMaxSelectorCallsPerWindow = 1;
constexpr std::uint8_t kRequiredInteractableValue = 2;
constexpr std::int32_t kMaxEnhancedActionMappings = 512;
constexpr std::int32_t kMaxReflectedMappingStructSize = 4096;
constexpr std::int32_t kMaxFunctionParameterBytes = 4096;
constexpr std::uint32_t kMaxPlayerContextDebugEventsPerActivation = 32;
constexpr std::uint32_t kMaxSelectorDebugEventsPerActivation = 64;
constexpr std::uint32_t kMaxDeferredDebugEventsPerActivation = 32;

struct NativeSelectorPair {
    UObject* actor{};
    UObject* component{};
};
static_assert(sizeof(NativeSelectorPair) == 16);

enum class RuntimeInitializationState : std::uint8_t {
    Pending,
    Ready,
    Failed,
};

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

template <typename Callback>
struct ScopeExit {
    Callback callback;

    ~ScopeExit() noexcept { callback(); }
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
        *output += elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0;
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

using NativeSelectorFunction = void(__fastcall*)(UObject*, NativeSelectorPair*, UObject*);

struct LoadedMainText {
    std::uintptr_t module_base{};
    std::span<const std::byte> mapped_image{};
    dsnap::PeRuntimeImageLayout image{};
    dsnap::ExecutableTextView text{};
    const char* error{"main_module_text_unresolved"};

    [[nodiscard]] bool valid() const noexcept {
        return module_base != 0 && !mapped_image.empty() && !text.bytes.empty();
    }
};

struct SelectorCapability {
    std::uintptr_t module_base{};
    std::uintptr_t address{};
    std::uintptr_t selector_rva{};
    std::uintptr_t server_address{};
    std::uintptr_t server_call_site{};
    std::uintptr_t ui_address{};
    std::uintptr_t ui_implementation_call_site{};
    std::uintptr_t ui_call_site{};
    std::uint32_t server_virtual_slot_offset{};
    std::size_t server_dispatch_candidates{};
    std::size_t ui_implementation_call_candidates{};
    std::size_t server_selector_candidates{};
    std::size_t ui_selector_candidates{};
    std::string_view status{"not_attempted"};
    std::string_view anchor_source{"none"};

    [[nodiscard]] bool resolved() const noexcept {
        return address != 0 && status == "resolved";
    }
};

struct ReflectionContractReport {
    bool required_valid{};
    std::string_view required_failure{"not_evaluated"};
    std::int32_t get_subsystem_parameter_bytes{-1};
    std::int32_t inject_parameter_bytes{-1};
    std::int32_t server_parameter_bytes{-1};
    std::uint32_t server_function_flags{};
    std::int32_t interactable_properties_size{-1};
    std::int32_t target_object_offset{-1};
    std::int32_t target_component_offset{-1};
    bool ui_schema_advisory_valid{};
    std::int32_t ui_parameter_bytes{-1};
    std::uint32_t ui_function_flags{};
    bool ui_has_return{};
    bool ui_interact_actor_present{};
    bool ui_interact_actor_exact_object{};
    std::int32_t ui_interact_actor_offset{-1};
    std::int32_t ui_interact_actor_element_size{-1};
    std::int32_t ui_interact_actor_array_dim{-1};
};

[[nodiscard]] LoadedMainText load_main_text_unsafe() noexcept {
    LoadedMainText result{};
    const auto module = GetModuleHandleW(nullptr);
    if (!module) {
        result.error = "main_module_unavailable";
        return result;
    }
    MODULEINFO module_information{};
    if (K32GetModuleInformation(GetCurrentProcess(), module, &module_information,
                                sizeof(module_information)) == FALSE ||
        module_information.lpBaseOfDll != module || module_information.SizeOfImage == 0) {
        result.error = "main_module_information_unavailable";
        return result;
    }
    const auto module_base = reinterpret_cast<std::uintptr_t>(module_information.lpBaseOfDll);
    const auto mapped_image = std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(module_information.lpBaseOfDll),
        static_cast<std::size_t>(module_information.SizeOfImage),
    };
    dsnap::PeRuntimeImageLayout image{};
    if (!dsnap::pe_runtime_succeeded(
            dsnap::parse_pe32_plus_loaded_image(mapped_image, module_base, &image))) {
        result.error = "pe_runtime_layout_invalid";
        return result;
    }
    result.module_base = module_base;
    result.mapped_image = mapped_image;
    result.image = image;
    result.text = {
        image.text_begin,
        std::span<const std::uint8_t>{
            reinterpret_cast<const std::uint8_t*>(image.text_begin), image.text_size},
    };
    result.error = "none";
    return result;
}

[[nodiscard]] LoadedMainText load_main_text_guarded() noexcept {
#if defined(_MSC_VER)
    __try {
        return load_main_text_unsafe();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LoadedMainText result{};
        result.error = "pe_header_access_fault";
        return result;
    }
#else
    return load_main_text_unsafe();
#endif
}

[[nodiscard]] bool read_virtual_function_guarded(UObject* object,
                                                 std::uint32_t slot_offset,
                                                 std::uintptr_t* output) noexcept {
    if (!object || !output || slot_offset % sizeof(void*) != 0) return false;
    *output = 0;
#if defined(_MSC_VER)
    __try {
#endif
        const auto vtable = *reinterpret_cast<const std::uintptr_t*>(object);
        if (vtable == 0 || slot_offset > std::numeric_limits<std::uintptr_t>::max() - vtable) {
            return false;
        }
        *output = *reinterpret_cast<const std::uintptr_t*>(vtable + slot_offset);
        return *output != 0;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *output = 0;
        return false;
    }
#endif
}

[[nodiscard]] bool executable_page(std::uintptr_t address) noexcept {
    if (address == 0) return false;
    MEMORY_BASIC_INFORMATION information{};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &information,
                     sizeof(information)) != sizeof(information) ||
        information.State != MEM_COMMIT || (information.Protect & PAGE_GUARD) != 0 ||
        (information.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }
    const auto protection = information.Protect & 0xFFU;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

[[nodiscard]] bool decode_instruction_addresses(
    const dsnap::ExecutableTextView& text,
    std::uintptr_t start,
    std::size_t scan_limit,
    std::vector<std::uintptr_t>* addresses) {
    if (!addresses || scan_limit == 0 || !text.contains(start)) return false;
    addresses->clear();
    const auto available = text.bytes.size() - static_cast<std::size_t>(start - text.address);
    const auto bytes = text.at(start, std::min(scan_limit, available));
    if (bytes.empty()) return false;
    ZydisDecoder decoder{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                                       ZYDIS_STACK_WIDTH_64))) {
        return false;
    }
    std::size_t offset{};
    while (offset < bytes.size()) {
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const auto decode_size = std::min<std::size_t>(16, bytes.size() - offset);
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bytes.data() + offset,
                                                 decode_size, &instruction, operands)) ||
            instruction.length == 0 || instruction.length > bytes.size() - offset) {
            addresses->clear();
            return false;
        }
        addresses->push_back(start + offset);
        offset += instruction.length;
    }
    return !addresses->empty();
}

[[nodiscard]] bool decode_leaf_thunk_addresses(
    const dsnap::ExecutableTextView& text,
    std::uintptr_t start,
    std::size_t scan_limit,
    std::vector<std::uintptr_t>* addresses,
    std::size_t* decoded_size) {
    if (!addresses || !decoded_size || scan_limit == 0 || !text.contains(start)) return false;
    addresses->clear();
    *decoded_size = 0;
    const auto available = text.bytes.size() - static_cast<std::size_t>(start - text.address);
    const auto bytes = text.at(start, std::min(scan_limit, available));
    if (bytes.empty()) return false;
    ZydisDecoder decoder{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                                       ZYDIS_STACK_WIDTH_64))) {
        return false;
    }
    std::size_t offset{};
    while (offset < bytes.size()) {
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const auto decode_size = std::min<std::size_t>(16, bytes.size() - offset);
        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bytes.data() + offset,
                                                 decode_size, &instruction, operands)) ||
            instruction.length == 0 || instruction.length > bytes.size() - offset) {
            addresses->clear();
            return false;
        }
        addresses->push_back(start + offset);
        offset += instruction.length;
        if (instruction.mnemonic == ZYDIS_MNEMONIC_JMP ||
            instruction.mnemonic == ZYDIS_MNEMONIC_RET) {
            *decoded_size = offset;
            return true;
        }
    }
    addresses->clear();
    return false;
}

[[nodiscard]] bool decode_runtime_function_addresses(
    const LoadedMainText& loaded,
    std::uintptr_t implementation,
    std::vector<std::uintptr_t>* addresses,
    std::size_t* scan_limit,
    dsnap::X64RuntimeFragmentCollection* fragments = nullptr) {
    if (!loaded.valid() || !addresses || !scan_limit ||
        !loaded.text.contains(implementation)) {
        return false;
    }
    addresses->clear();
    *scan_limit = 0;
    if (fragments) *fragments = {};
    dsnap::X64RuntimeFragmentCollection runtime_function{};
    if (!dsnap::pe_runtime_succeeded(dsnap::collect_x64_chained_runtime_fragments(
            loaded.mapped_image, loaded.module_base, implementation, &runtime_function)) ||
        runtime_function.fragment_count == 0 ||
        runtime_function.canonical_root.begin != implementation) {
        return false;
    }
    std::vector<std::uintptr_t> fragment_addresses{};
    std::uintptr_t maximum_end = implementation;
    for (std::size_t index = 0; index < runtime_function.fragment_count; ++index) {
        const auto& fragment = runtime_function.fragments[index];
        if (fragment.begin < implementation || fragment.end <= fragment.begin ||
            !loaded.text.contains(fragment.begin,
                                  static_cast<std::size_t>(fragment.end - fragment.begin)) ||
            !decode_instruction_addresses(loaded.text, fragment.begin,
                                          static_cast<std::size_t>(fragment.end - fragment.begin),
                                          &fragment_addresses)) {
            addresses->clear();
            return false;
        }
        if (!addresses->empty() && fragment_addresses.front() <= addresses->back()) {
            addresses->clear();
            return false;
        }
        addresses->insert(addresses->end(), fragment_addresses.begin(), fragment_addresses.end());
        maximum_end = std::max(maximum_end, fragment.end);
    }
    if (addresses->empty() || addresses->front() != implementation || maximum_end <= implementation) {
        addresses->clear();
        return false;
    }
    *scan_limit = static_cast<std::size_t>(maximum_end - implementation);
    if (fragments) *fragments = runtime_function;
    return true;
}

struct ReflectedVirtualFunction {
    std::uintptr_t implementation{};
    std::uint32_t virtual_slot_offset{};
    std::size_t dispatch_candidates{};
    std::string_view status{"not_attempted"};

    [[nodiscard]] bool resolved() const noexcept {
        return implementation != 0 && status == "resolved";
    }
};

struct ReflectedDirectFunction {
    std::uintptr_t implementation{};
    std::uintptr_t implementation_call_site{};
    std::size_t implementation_call_candidates{};
    std::string_view status{"not_attempted"};

    [[nodiscard]] bool resolved() const noexcept {
        return implementation != 0 && implementation_call_site != 0 && status == "resolved";
    }
};

[[nodiscard]] ReflectedVirtualFunction resolve_reflected_virtual_function(
    const LoadedMainText& loaded,
    UFunction* function,
    UObject* class_default_object) {
    ReflectedVirtualFunction result{};
    if (!loaded.valid() || !function || !class_default_object) {
        result.status = "reflection_anchor_unavailable";
        return result;
    }
    const auto reflected_exec = function->GetFuncPtr();
    static_assert(sizeof(reflected_exec) == sizeof(std::uintptr_t));
    const auto exec_address = std::bit_cast<std::uintptr_t>(reflected_exec);
    if (!loaded.text.contains(exec_address) || !executable_page(exec_address)) {
        result.status = "reflection_exec_outside_game_text";
        return result;
    }
    std::vector<std::uintptr_t> exec_instructions{};
    std::size_t exec_decode_size{};
    if (!decode_leaf_thunk_addresses(loaded.text, exec_address,
                                     kReflectedExecDecodeLimit,
                                     &exec_instructions, &exec_decode_size)) {
        result.status = "reflection_exec_decode_failed";
        return result;
    }
    const auto dispatch = dsnap::resolve_virtual_dispatch_slot(
        loaded.text, exec_address, exec_instructions, exec_decode_size);
    result.dispatch_candidates = dispatch.candidates;
    if (!dispatch.resolved()) {
        result.status = dsnap::selector_resolver_status_name(dispatch.status);
        return result;
    }
    result.virtual_slot_offset = dispatch.slot_offset;
    std::uintptr_t virtual_entry{};
    if (!read_virtual_function_guarded(class_default_object, dispatch.slot_offset,
                                       &virtual_entry) ||
        !loaded.text.contains(virtual_entry) || !executable_page(virtual_entry)) {
        result.status = "vtable_entry_outside_game_text";
        return result;
    }
    const auto implementation = dsnap::follow_direct_jump_chain(loaded.text, virtual_entry);
    if (!implementation.resolved()) {
        result.status = dsnap::selector_resolver_status_name(implementation.status);
        return result;
    }
    if (!executable_page(implementation.address)) {
        result.status = "implementation_page_not_executable";
        return result;
    }
    result.implementation = implementation.address;
    result.status = "resolved";
    return result;
}

[[nodiscard]] ReflectedDirectFunction resolve_reflected_direct_function(
    const LoadedMainText& loaded,
    UFunction* function) {
    ReflectedDirectFunction result{};
    if (!loaded.valid() || !function) {
        result.status = "reflection_anchor_unavailable";
        return result;
    }
    const auto reflected_exec = function->GetFuncPtr();
    static_assert(sizeof(reflected_exec) == sizeof(std::uintptr_t));
    const auto exec_address = std::bit_cast<std::uintptr_t>(reflected_exec);
    if (!loaded.text.contains(exec_address) || !executable_page(exec_address)) {
        result.status = "reflection_exec_outside_game_text";
        return result;
    }

    std::vector<std::uintptr_t> exec_instructions{};
    std::size_t exec_scan_limit{};
    dsnap::X64RuntimeFragmentCollection exec_runtime_function{};
    if (!decode_runtime_function_addresses(loaded, exec_address, &exec_instructions,
                                            &exec_scan_limit, &exec_runtime_function)) {
        result.status = "reflection_exec_runtime_function_decode_failed";
        return result;
    }

    dsnap::DirectRel32Call implementation_call{};
    for (std::size_t fragment_index = 0;
         fragment_index < exec_runtime_function.fragment_count;
         ++fragment_index) {
        const auto& fragment = exec_runtime_function.fragments[fragment_index];
        const auto first = std::lower_bound(exec_instructions.begin(), exec_instructions.end(),
                                            fragment.begin);
        const auto last = std::lower_bound(exec_instructions.begin(), exec_instructions.end(),
                                           fragment.end);
        if (first == last || *first != fragment.begin) {
            result.status = "reflection_exec_fragment_instruction_bounds_invalid";
            return result;
        }
        const auto first_index = static_cast<std::size_t>(
            std::distance(exec_instructions.begin(), first));
        const auto instruction_count = static_cast<std::size_t>(std::distance(first, last));
        const auto fragment_instructions = std::span<const std::uintptr_t>{
            exec_instructions.data() + first_index,
            instruction_count,
        };
        const auto terminal = dsnap::select_unique_terminal_rel32_call(
            loaded.text, fragment.begin, fragment_instructions,
            static_cast<std::size_t>(fragment.end - fragment.begin));
        if (terminal.status == dsnap::SelectorResolverStatus::TerminalCallNotFound) {
            continue;
        }
        if (terminal.status == dsnap::SelectorResolverStatus::TerminalCallAmbiguous) {
            result.implementation_call_candidates += terminal.candidates;
            continue;
        }
        if (!terminal.resolved()) {
            result.status = dsnap::selector_resolver_status_name(terminal.status);
            return result;
        }
        result.implementation_call_candidates += terminal.candidates;
        implementation_call = terminal.call;
    }
    if (result.implementation_call_candidates == 0) {
        result.status = "terminal_call_not_found";
        return result;
    }
    if (result.implementation_call_candidates != 1) {
        result.status = "terminal_call_ambiguous";
        return result;
    }

    if (implementation_call.call_site == 0 || implementation_call.target == 0 ||
        implementation_call.target == exec_address) {
        result.status = "direct_implementation_call_target_invalid";
        return result;
    }
    const auto implementation = dsnap::follow_direct_jump_chain(
        loaded.text, implementation_call.target);
    if (!implementation.resolved()) {
        result.status = dsnap::selector_resolver_status_name(implementation.status);
        return result;
    }
    if (implementation.address == exec_address ||
        !executable_page(implementation_call.call_site) ||
        !executable_page(implementation.address)) {
        result.status = "direct_implementation_page_not_executable";
        return result;
    }
    result.implementation = implementation.address;
    result.implementation_call_site = implementation_call.call_site;
    result.status = "resolved";
    return result;
}

[[nodiscard]] bool invoke_selector_address_guarded(std::uintptr_t address,
                                                   UObject* receiver,
                                                   NativeSelectorPair* output,
                                                   bool* faulted) noexcept {
    if (faulted) *faulted = false;
    if (address == 0 || !receiver || !output) return false;
    const auto selector = reinterpret_cast<NativeSelectorFunction>(address);
#if defined(_MSC_VER)
    __try {
#endif
        selector(receiver, output, nullptr);
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
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(weak.ObjectIndex)) << 32U) |
           static_cast<std::uint32_t>(weak.ObjectSerialNumber);
}

[[nodiscard]] dsnap::WeakObjectId weak_object_id(const FWeakObjectPtr& weak) noexcept {
    return {weak.ObjectIndex, weak.ObjectSerialNumber};
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
            status_toast_renderer_.shutdown_guarded();
            unregister_callbacks();
            cancel_window("shutdown");
            reset_action_activation("shutdown", true);
            static_cast<void>(logger_.flush_all());
        } else {
            static_cast<void>(session_events_.reset());
            automation_enabled_.store(false, std::memory_order_release);
            active_world_identity_.store(0, std::memory_order_release);
            window_active_.store(false, std::memory_order_release);
            dispatch_observer_armed_.store(false, std::memory_order_release);
            pending_action_id_visible_.store(0, std::memory_order_release);
            pending_receiver_address_.store(0, std::memory_order_release);
            dispatch_observed_action_id_.store(0, std::memory_order_release);
            pending_action_visible_.store(false, std::memory_order_release);
            engine_tick_callback_id_ = Hook::ERROR_ID;
            world_reset_callback_id_ = Hook::ERROR_ID;
            begin_play_callback_id_ = Hook::ERROR_ID;
            server_run_interact_hook_ = {};
            server_run_interact_hook_registered_ = false;
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
        static_cast<void>(session_events_.reset());
        automation_enabled_.store(false, std::memory_order_release);
        active_world_identity_.store(0, std::memory_order_release);
        window_active_.store(false, std::memory_order_release);
        dispatch_observer_armed_.store(false, std::memory_order_release);
        pending_action_id_visible_.store(0, std::memory_order_release);
        pending_receiver_address_.store(0, std::memory_order_release);
        dispatch_observed_action_id_.store(0, std::memory_order_release);
        pending_action_visible_.store(false, std::memory_order_release);
        status_toast_renderer_.release_for_travel();
    }

    void on_unreal_init() override {
        if (!configuration_result_.valid()) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            logger_.write(dsnap::LogAudience::User, "DISABLED", "invalid configuration");
            return;
        }
        if (!apply_build_fingerprint_once()) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            logger_.write(dsnap::LogAudience::User, "PASSIVE_ONLY",
                          std::format("unknown or incompatible UE4SS fingerprint error={} "
                                      "reflection_not_attempted=1",
                                      fingerprint_result_.error));
            static_cast<void>(logger_.flush_all());
            return;
        }

        const auto initialization_now = Clock::now();
        if (runtime_initialization_attempts_ == 0) {
            runtime_initialization_started_at_ = initialization_now;
            runtime_initialization_deadline_ = initialization_now + kRuntimeInitializationTimeout;
            const auto generation = callback_gate_.generation();
            engine_tick_callback_id_ = Hook::RegisterEngineTickPostCallback(
                [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                    if (auto* self = current_instance(generation)) self->engine_tick_post(engine);
                },
                {false, false, STR("DragonSwordNativeAutoPickup"), STR("BoundedSelectorWindowCanary")});
            if (engine_tick_callback_id_ == Hook::ERROR_ID) {
                runtime_initialization_state_ = RuntimeInitializationState::Failed;
                logger_.write(dsnap::LogAudience::User, "DISABLED",
                              "required bootstrap EngineTickPost callback registration failed");
                return;
            }
        }
        ++runtime_initialization_attempts_;

        drop_item_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kDropItemClassPath);
        interactable_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInteractableClassPath);
        sphere_component_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kSphereComponentClassPath);
        set_sphere_radius_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kSetSphereRadiusFunctionPath);
        enhanced_player_input_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kEnhancedPlayerInputClassPath);
        input_action_class_ = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, kInputActionClassPath);
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
            ? subsystem_library_class_->GetClassDefaultObject().Get()
            : nullptr;
        server_run_interact_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kServerRunInteractFunctionPath);
        auto* set_interact_ui_function =
            UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kSetInteractUiFunctionPath);
        auto* interactable_cdo = interactable_class_
            ? interactable_class_->GetClassDefaultObject().Get()
            : nullptr;
        const FName target_object_name{STR("ExecuteTargetObject"), FNAME_Find};
        const FName target_component_name{STR("ExecuteTargetComponent"), FNAME_Find};
        auto* target_object_property = interactable_class_
            ? exact_property<FObjectProperty>(interactable_class_->FindProperty(target_object_name))
            : nullptr;
        auto* target_component_property = interactable_class_
            ? exact_property<FObjectProperty>(interactable_class_->FindProperty(target_component_name))
            : nullptr;
        ReflectionContractReport reflection_report{};
        if (!validate_reflection_contract(server_run_interact_function_, set_interact_ui_function,
                                          interactable_cdo, target_object_property,
                                          target_component_property, &reflection_report)) {
            const bool only_interactable_cdo_pending =
                interactable_cdo == nullptr && drop_item_class_ != nullptr &&
                interactable_class_ != nullptr && enhanced_player_input_class_ != nullptr &&
                input_action_class_ != nullptr && enhanced_action_mapping_struct_ != nullptr &&
                key_struct_ != nullptr && enhanced_input_subsystem_class_ != nullptr &&
                subsystem_library_class_ != nullptr && subsystem_library_cdo_ != nullptr &&
                get_local_player_subsystem_function_ != nullptr &&
                inject_input_vector_function_ != nullptr &&
                server_run_interact_function_ != nullptr && set_interact_ui_function != nullptr &&
                target_object_property != nullptr && target_component_property != nullptr;
            if (only_interactable_cdo_pending && initialization_now < runtime_initialization_deadline_) {
                runtime_initialization_state_ = RuntimeInitializationState::Pending;
                next_runtime_initialization_attempt_ =
                    initialization_now + kRuntimeInitializationRetryInterval;
                if (runtime_initialization_attempts_ == 1) {
                    logger_.write(dsnap::LogAudience::User, "INITIALIZATION_DEFERRED",
                                  std::format("reason=interactable_cdo_not_ready retry_interval_ms={} "
                                              "timeout_ms={} fail_closed_until_ready=1",
                                              kRuntimeInitializationRetryInterval.count(),
                                              std::chrono::duration_cast<std::chrono::milliseconds>(
                                                  kRuntimeInitializationTimeout).count()));
                    log_reflection_contract_detail(reflection_report, set_interact_ui_function);
                } else if (configuration_result_.value.debug_logging &&
                           runtime_initialization_attempts_ % 8 == 0) {
                    logger_.write(dsnap::LogAudience::Debug, "INITIALIZATION_RETRY",
                                  std::format("reason=interactable_cdo_not_ready attempt={} "
                                              "elapsed_ms={}",
                                              runtime_initialization_attempts_,
                                              std::chrono::duration_cast<std::chrono::milliseconds>(
                                                  initialization_now - runtime_initialization_started_at_).count()));
                }
                return;
            }
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            logger_.write(dsnap::LogAudience::User, "DISABLED",
                          std::format("reflection contract missing or changed drop_item={} interactable={} "
                                      "enhanced_player_input={} input_action={} mapping_struct={} key_struct={} "
                                      "subsystem={} library={} "
                                      "library_cdo={} get_subsystem_function={} inject_function={} "
                                       "server_run_interact={} set_interact_ui={} interactable_cdo={} "
                                       "target_object_property={} "
                                      "target_component_property={}",
                                      drop_item_class_ != nullptr, interactable_class_ != nullptr,
                                      enhanced_player_input_class_ != nullptr, input_action_class_ != nullptr,
                                      enhanced_action_mapping_struct_ != nullptr, key_struct_ != nullptr,
                                      enhanced_input_subsystem_class_ != nullptr,
                                      subsystem_library_class_ != nullptr, subsystem_library_cdo_ != nullptr,
                                      get_local_player_subsystem_function_ != nullptr,
                                      inject_input_vector_function_ != nullptr,
                                        server_run_interact_function_ != nullptr,
                                       set_interact_ui_function != nullptr, interactable_cdo != nullptr,
                                       target_object_property != nullptr,
                                       target_component_property != nullptr));
            log_reflection_contract_detail(reflection_report, set_interact_ui_function);
            return;
        }
        log_reflection_contract_detail(reflection_report, set_interact_ui_function);
        if (!resolve_selector_capability(server_run_interact_function_, set_interact_ui_function,
                                         interactable_cdo,
                                         target_object_property->GetOffset_Internal(),
                                         target_component_property->GetOffset_Internal())) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            logger_.write(dsnap::LogAudience::User, "SELECTOR_UNAVAILABLE",
                           std::format("policy={} status={} anchor_source={} "
                                       "server_virtual_dispatch_candidates={} "
                                       "ui_implementation_call_candidates={} "
                                       "server_selector_candidates={} ui_selector_candidates={} "
                                       "fail_closed=1 automation_disabled=1",
                                       kSelectorResolutionPolicy, selector_capability_.status,
                                       selector_capability_.anchor_source,
                                       selector_capability_.server_dispatch_candidates,
                                       selector_capability_.ui_implementation_call_candidates,
                                       selector_capability_.server_selector_candidates,
                                       selector_capability_.ui_selector_candidates));
            return;
        }
        logger_.write(dsnap::LogAudience::User, "SELECTOR_RESOLVED",
                       std::format("policy={} anchor_source={} selector_rva=0x{:X} "
                                   "server_rva=0x{:X} server_call_rva=0x{:X} "
                                   "ui_exec_call_rva=0x{:X} ui_rva=0x{:X} ui_call_rva=0x{:X} "
                                   "server_virtual_slot=0x{:X} "
                                   "server_virtual_dispatch_candidates={} "
                                   "ui_implementation_call_candidates={} "
                                   "server_selector_candidates={} ui_selector_candidates={} fail_closed=1",
                                   kSelectorResolutionPolicy, selector_capability_.anchor_source,
                                   selector_capability_.selector_rva,
                                   selector_capability_.server_address - selector_capability_.module_base,
                                   selector_capability_.server_call_site - selector_capability_.module_base,
                                   selector_capability_.ui_implementation_call_site -
                                       selector_capability_.module_base,
                                   selector_capability_.ui_address - selector_capability_.module_base,
                                   selector_capability_.ui_call_site - selector_capability_.module_base,
                                   selector_capability_.server_virtual_slot_offset,
                                   selector_capability_.server_dispatch_candidates,
                                   selector_capability_.ui_implementation_call_candidates,
                                   selector_capability_.server_selector_candidates,
                                   selector_capability_.ui_selector_candidates));

        if (!register_server_run_interact_observer()) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            logger_.write(dsnap::LogAudience::User, "DISABLED",
                          "required Server_RunInteractV2 post observer registration failed");
            return;
        }

        if constexpr (kNativeDropItemRangeBridgeEnabled) {
            resolve_drop_item_range_contract();
        } else {
            drop_item_range_multiplier_ = 1;
            drop_item_range_contract_ready_ = false;
            logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_PAK_OWNED",
                          "structured_drop_assets=19 native_multiplier=disabled double_apply_prevented=1");
        }

        const auto generation = callback_gate_.generation();
        world_reset_callback_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current_instance(generation)) self->reset_world("InitGameStatePre");
            },
            {false, false, STR("DragonSwordNativeAutoPickup"), STR("WorldReset")});
        if (drop_item_range_contract_ready_) {
            begin_play_callback_id_ = Hook::RegisterBeginPlayPostCallback(
                [generation](Hook::TCallbackIterationData<void>&, AActor* actor) {
                    if (auto* self = current_instance(generation)) {
                        self->drop_item_begin_play_post(actor);
                    }
                },
                {false, false, STR("DragonSwordNativeAutoPickup"), STR("DropItemRange")});
            if (begin_play_callback_id_ == Hook::ERROR_ID) {
                drop_item_range_contract_ready_ = false;
                logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_DISABLED",
                              "reason=begin_play_callback_registration_failed core_auto_pickup_available=1");
            }
        }

        if (engine_tick_callback_id_ == Hook::ERROR_ID || world_reset_callback_id_ == Hook::ERROR_ID) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            unregister_callbacks();
            logger_.write(dsnap::LogAudience::User, "DISABLED", "required native callback registration failed");
            return;
        }

        const auto toggle_key = dsnap::parse_toggle_hotkey(configuration_result_.value.toggle_hotkey);
        if (!toggle_key) {
            runtime_initialization_state_ = RuntimeInitializationState::Failed;
            unregister_callbacks();
            logger_.write(dsnap::LogAudience::User, "DISABLED", "configured toggle hotkey is unsupported");
            return;
        }
        toggle_virtual_key_ = *toggle_key;
        register_keydown_event(static_cast<Input::Key>(*toggle_key), [generation]() noexcept {
            if (auto* self = current_instance(generation)) {
                if (!self->toggle_key_edge_.key_down()) {
                    ++self->key_repeat_rejections_;
                    return;
                }
                const auto foreground = foreground_window_state();
                if (!foreground.matched) {
                    ++self->key_events_rejected_;
                    self->logger_.write(dsnap::LogAudience::User, "TOGGLE_REJECTED",
                                        "reason=foreground_mismatch source=UE4SS_configured_keydown");
                } else if (!self->session_events_.record_key_event()) {
                    ++self->key_events_rejected_;
                    self->logger_.write(dsnap::LogAudience::User, "TOGGLE_REJECTED",
                                        "reason=playable_session_not_ready "
                                        "source=UE4SS_configured_keydown retry_after_world_load=1");
                }
            }
        });

        status_toast_renderer_.initialize();
        if (status_toast_renderer_.state()
            == dsnap::ue4ss::StatusToastRendererState::Ready) {
            logger_.write(dsnap::LogAudience::User, "STATUS_TOAST_READY",
                          "renderer=native_umg position=top_center input_mode_unchanged=1 "
                          "hit_test_invisible=1 pickup_decision_input=0");
        } else {
            logger_.write(dsnap::LogAudience::User, "STATUS_TOAST_UNAVAILABLE",
                          std::format("reason=abi_validation_failed mask=0x{:08X} "
                                      "pickup_unaffected=1",
                                      status_toast_renderer_.abi_failure_mask()));
        }

        runtime_initialization_state_ = RuntimeInitializationState::Ready;
        if (runtime_initialization_attempts_ > 1) {
            logger_.write(dsnap::LogAudience::User, "INITIALIZATION_RECOVERED",
                          std::format("reason=interactable_cdo_ready attempts={} elapsed_ms={} "
                                      "full_contract_revalidated=1",
                                      runtime_initialization_attempts_,
                                      std::chrono::duration_cast<std::chrono::milliseconds>(
                                          Clock::now() - runtime_initialization_started_at_).count()));
        }

        logger_.write(dsnap::LogAudience::User, "READY",
                      std::format("label={} hotkey={} mode=toggle_automatic selector_policy={} "
                                  "selector_rva=0x{:X} "
                                  "allowed_types=2,5,7 excluded_types=4_treasure_box "
                                  "action=enhanced_input_live_interaction_action_one_shot "
                                  "interaction_key={} interaction_key_fallback={} "
                                  "engine_pulse_ms={} active_scan_ms={} idle_scan_ms={} transient_backoff_ms={} "
                                  "post_pickup_ms={} world_settle_ms={} selector_calls_per_scan={} "
                                  "debug_logging={} perf_interval_seconds={} slow_scan_threshold_us={} "
                                  "hotkey_source=UE4SS_configured_keydown_with_atomic_physical_edge "
                                  "pending_action_policy=one_until_game_dispatch bounded_retry=1 max_attempts={} "
                                  "dispatch_observer=Server_RunInteractV2_post activation_quarantine=0 "
                                  "dispatch_attempt_policy=shared_unconfirmed_v1 "
                                  "active_cadence=engine_tick_v1 capacity_policy=wait_before_input "
                                  "debug_decision_policy=deferred_scalar_v1 "
                                  "drop_item_range_multiplier={} drop_item_range_begin_play={} "
                                  "object_scans=0 cross_world_object_cache=0 target_field_access=0 "
                                  "direct_RPC=0 SendInput=0",
                                  kLabel, configuration_result_.value.toggle_hotkey,
                                  kSelectorResolutionPolicy, selector_capability_.selector_rva,
                                  configuration_result_.value.interaction_key,
                                  configuration_result_.value.interaction_key_fallback,
                                  kPulseInterval.count(),
                                  kActiveScanInterval.count(), kIdleScanInterval.count(),
                                  kTransientBackoff.count(), kPostPickupCooldown.count(),
                                  kWorldSettleDelay.count(), kMaxSelectorCallsPerWindow,
                                  configuration_result_.value.debug_logging,
                                  configuration_result_.value.perf_log_interval_seconds,
                                  configuration_result_.value.slow_scan_threshold_us,
                                  dsnap::kMaximumAutomaticActionAttempts,
                                  drop_item_range_multiplier_, drop_item_range_contract_ready_));
        logger_.write(dsnap::LogAudience::User, "AUTOMATION_AVAILABLE",
                      std::format("automatic pickup is off at launch; press {} to enable or disable",
                                  configuration_result_.value.toggle_hotkey));
    }

    void release_toggle_key_if_up() noexcept {
        if (!toggle_key_edge_.latched()) return;
        if ((GetAsyncKeyState(static_cast<int>(toggle_virtual_key_)) & 0x8000) != 0) return;
        if (toggle_key_edge_.key_up()) ++key_release_rearms_;
    }

    void on_update() override {
        release_toggle_key_if_up();
        const auto logger_flush_started = configuration_result_.value.debug_logging
            ? Clock::now()
            : Clock::time_point{};
        const auto dequeued_messages = logger_.flush();
        if (configuration_result_.value.debug_logging && dequeued_messages != 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - logger_flush_started).count();
            logger_flush_timing_.record(elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0);
            logger_messages_dequeued_.fetch_add(dequeued_messages, std::memory_order_relaxed);
        }
        if (shutting_down_.load(std::memory_order_acquire)) return;

        if (!fingerprint_applied_ || !build_trusted_.load(std::memory_order_acquire) ||
            runtime_initialization_state_ != RuntimeInitializationState::Ready) return;

        const auto key_events = session_events_.drain_key_events();
        const auto key_event_count = key_events.count;
        if (key_event_count != 0) {
            key_events_received_.fetch_add(key_event_count, std::memory_order_relaxed);
            const auto coalesced_count = key_event_count - 1;
            if (coalesced_count != 0) {
                key_events_coalesced_.fetch_add(coalesced_count, std::memory_order_relaxed);
            }

            if (!session_events_.publish_toggle_request(key_events.generation)) {
                ++key_events_rejected_;
                logger_.write(dsnap::LogAudience::User, "TOGGLE_REJECTED",
                              "reason=session_reset source=UE4SS_configured_keydown");
            } else {
                ++toggle_requests_accepted_;
                logger_.write(dsnap::LogAudience::User, "TOGGLE_REQUESTED",
                              std::format("source=UE4SS_configured_keydown_physical_edge hotkey={} event_count={} "
                                          "coalesced={} session_generation={}",
                                          configuration_result_.value.toggle_hotkey,
                                          key_event_count, coalesced_count, key_events.generation));
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
            const auto logger_flush = logger_flush_timing_.take_interval();
            const auto slow_scans = slow_scan_count_.exchange(0, std::memory_order_acq_rel);
            if (configuration_result_.value.debug_logging) {
                logger_.write(dsnap::LogAudience::Debug, "PERF_AGGREGATE",
                              std::format("key_events_received={} key_events_coalesced={} key_events_rejected={} "
                                          "key_repeat_rejections={} key_release_rearms={} "
                                          "toggle_requests_accepted={} toggle_transitions={} automatic_scans={} "
                                          "foreground_pauses={} automatic_disables={} "
                                          "windows_started={} windows_hit={} windows_cancelled={} "
                                          "selector_attempts={} selector_no_candidate={} selector_pairs={} "
                                          "enhanced_input_attempts={} enhanced_input_injections={} "
                                          "action_invocations={} action_injection_failures={} "
                                          "pending_scan_suppressions={} action_cooldown_hits={} "
                                          "action_record_size={} action_dispatch_observations={} "
                                          "dispatch_marker_mismatches={} action_state_faults={} "
                                          "action_failures={} transient_failures={} confirmations={} "
                                          "unconfirmed_timeouts={} lifecycle_cancellations={} "
                                          "selector_faults={} world_resets={} "
                                          "automation_enabled={} window_active={} pending_action={} "
                                          "debug_context_emitted={} debug_context_suppressed={} "
                                          "debug_selector_emitted={} debug_selector_suppressed={} "
                                          "debug_deferred_emitted={} debug_deferred_suppressed={} "
                                          "diagnostic_write_failures={} "
                                          "logger_messages_dequeued={} logger_queue_peak={} logger_dropped={} "
                                          "logger_failed={} object_scans=0 cross_world_object_cache=0",
                                          key_events_received_.load(), key_events_coalesced_.load(),
                                          key_events_rejected_.load(), key_repeat_rejections_.load(),
                                          key_release_rearms_.load(), toggle_requests_accepted_.load(),
                                          toggle_transitions_.load(), automatic_scans_.load(),
                                          foreground_pauses_.load(), automatic_disables_.load(),
                                          windows_started_.load(), windows_hit_.load(), windows_cancelled_.load(),
                                          selector_attempts_.load(), selector_no_candidate_.load(), selector_pairs_.load(),
                                          enhanced_input_attempts_.load(), enhanced_input_injections_.load(),
                                          action_invocations_.load(), action_injection_failures_.load(),
                                          pending_scan_suppressions_.load(), action_cooldown_hits_.load(),
                                          action_record_size_visible_.load(std::memory_order_acquire),
                                          action_dispatch_observations_.load(),
                                          dispatch_marker_mismatches_.load(),
                                          action_state_faults_.load(),
                                          action_failures_.load(), transient_failures_.load(), confirmations_.load(),
                                          confirmation_timeouts_.load(), action_lifecycle_cancellations_.load(),
                                          selector_faults_.load(), world_resets_.load(),
                                          automation_enabled_.load(std::memory_order_acquire),
                                          window_active_.load(std::memory_order_acquire),
                                          pending_action_visible_.load(std::memory_order_acquire),
                                          debug_context_emitted_.load(), debug_context_suppressed_.load(),
                                          debug_selector_emitted_.load(), debug_selector_suppressed_.load(),
                                          debug_deferred_emitted_.load(), debug_deferred_suppressed_.load(),
                                          diagnostic_write_failures_.load(),
                                          logger_messages_dequeued_.load(), logger_.peak_queue_size(),
                                          logger_.dropped_messages(), logger_.failed_messages()));
                logger_.write(dsnap::LogAudience::Debug, "PERF_TIMING",
                              std::format("interval_seconds={} slow_threshold_us={} "
                                          "scheduler_gap_count={} scheduler_gap_avg_us={} scheduler_gap_max_us={} "
                                          "pulse_work_count={} pulse_work_avg_us={} pulse_work_max_us={} "
                                          "scan_count={} scan_avg_us={} scan_max_us={} slow_scans={} "
                                          "context_count={} context_avg_us={} context_max_us={} "
                                          "selector_count={} selector_avg_us={} selector_max_us={} "
                                          "validation_count={} validation_avg_us={} validation_max_us={} "
                                          "action_resolution_count={} action_resolution_avg_us={} action_resolution_max_us={} "
                                          "subsystem_count={} subsystem_avg_us={} subsystem_max_us={} "
                                          "injection_count={} injection_avg_us={} injection_max_us={} "
                                          "logger_flush_count={} logger_flush_avg_us={} logger_flush_max_us={}",
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
                                          injection.count, injection.average_us(), injection.max_us,
                                          logger_flush.count, logger_flush.average_us(), logger_flush.max_us));
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

    void update_status_toast(UEngine* engine) noexcept {
        status_toast_renderer_.tick(engine, Clock::now());
        const auto faults = status_toast_renderer_.fault_count();
        if (faults <= status_toast_faults_reported_) return;
        status_toast_faults_reported_ = faults;
        try {
            logger_.write(dsnap::LogAudience::User, "STATUS_TOAST_DISABLED",
                          std::format("reason=guarded_runtime_fault failure={} faults={} "
                                      "pickup_unaffected=1",
                                      status_toast_renderer_.last_failure(), faults));
        } catch (...) {
        }
    }

    void emit_selected_distance_diagnostic(std::uint64_t activation_id,
                                           std::uint64_t action_id,
                                           std::uint64_t request_id,
                                           std::uint64_t actor_identity,
                                           std::uint64_t component_identity) noexcept {
        if (!configuration_result_.value.debug_logging) return;
        try {
            logger_.write(dsnap::LogAudience::Debug, "TARGET_SELECTED_DIAGNOSTIC",
                          std::format("activation_id={} action_id={} request_id={} actor=0x{:X} "
                                      "component=0x{:X} distance_unavailable=1 "
                                      "reason=unsafe_runtime_location_read_omitted "
                                      "diagnostic_only=1 decision_input=0 post_injection=1",
                                      activation_id, action_id, request_id, actor_identity,
                                      component_identity));
        } catch (...) {
        }
    }

    void maybe_log_slow_tick(Clock::time_point started,
                             const StageTimings& timing) noexcept {
        if (!configuration_result_.value.debug_logging) return;
        const auto finished = Clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            finished - started);
        if (elapsed <= kSlowTickThreshold) return;
        if (last_slow_tick_log_ != Clock::time_point{} &&
            finished - last_slow_tick_log_ < kSlowTickLogInterval) {
            return;
        }
        last_slow_tick_log_ = finished;
        try {
            logger_.write(dsnap::LogAudience::Debug, "AUTO_TICK_SLOW",
                          std::format("total_us={} context_us={} selector_us={} validation_us={} "
                                      "action_resolution_us={} subsystem_us={} injection_us={} "
                                      "automation_enabled={} pending_action={} window_active={}",
                                      elapsed.count(), timing.context_us, timing.selector_us,
                                      timing.validation_us, timing.action_resolution_us,
                                      timing.subsystem_us, timing.injection_us,
                                      automation_enabled_.load(std::memory_order_acquire),
                                      pending_action_visible_.load(std::memory_order_acquire),
                                      window_active_.load(std::memory_order_acquire)));
        } catch (...) {
        }
    }

    void resolve_drop_item_range_contract() noexcept {
        drop_item_range_multiplier_ = 1;
        drop_item_range_contract_ready_ = false;
        std::uint32_t selected_count{};
        try {
            const auto mods_directory = binary_directory().parent_path().parent_path() /
                "Content" / "Paks" / "~mods";
            constexpr std::array<std::pair<std::wstring_view, std::uint32_t>, 5> options{{
                {L"DS_PickupRangeX3_P.pak", 3},
                {L"DS_PickupRangeX5_P.pak", 5},
                {L"DS_PickupRangeX10_P.pak", 10},
                {L"DS_PickupRangeX15_P.pak", 15},
                {L"DS_PickupRangeX20_P.pak", 20},
            }};
            for (const auto& [name, multiplier] : options) {
                std::error_code error{};
                if (!std::filesystem::is_regular_file(mods_directory / name, error) || error) continue;
                ++selected_count;
                drop_item_range_multiplier_ = multiplier;
            }
        } catch (...) {
            selected_count = 0;
            drop_item_range_multiplier_ = 0;
        }

        if (drop_item_range_multiplier_ == 0) {
            logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_DISABLED",
                          "reason=range_pak_detection_failed core_auto_pickup_available=1");
            return;
        }
        if (selected_count == 0) {
            logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_READY",
                          "multiplier=1 source=no_recognized_range_pak runtime_adjustment=not_required");
            return;
        }
        if (selected_count != 1) {
            drop_item_range_multiplier_ = 0;
            logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_DISABLED",
                          std::format("reason=multiple_recognized_range_paks count={} "
                                      "remove_all_but_one=1 core_auto_pickup_available=1",
                                      selected_count));
            return;
        }

        const FName overlap_name{STR("SphereOverlapComp"), FNAME_Find};
        const FName radius_name{STR("SphereRadius"), FNAME_Find};
        drop_item_overlap_property_ = drop_item_class_ && overlap_name != NAME_None
            ? exact_property<FObjectProperty>(drop_item_class_->FindProperty(overlap_name))
            : nullptr;
        sphere_radius_instance_property_ = sphere_component_class_ && radius_name != NAME_None
            ? exact_property<FFloatProperty>(sphere_component_class_->FindProperty(radius_name))
            : nullptr;
        set_sphere_radius_value_property_ = set_sphere_radius_function_
            ? exact_property<FFloatProperty>(
                  function_property(set_sphere_radius_function_, STR("InSphereRadius")))
            : nullptr;
        set_sphere_radius_update_property_ = set_sphere_radius_function_
            ? exact_property<FBoolProperty>(
                  function_property(set_sphere_radius_function_, STR("bUpdateOverlaps")))
            : nullptr;

        const auto parameter_bytes = set_sphere_radius_function_
            ? set_sphere_radius_function_->GetParmsSize()
            : 0;
        const auto drop_item_properties_size = drop_item_class_
            ? drop_item_class_->GetPropertiesSize()
            : 0;
        const auto sphere_properties_size = sphere_component_class_
            ? sphere_component_class_->GetPropertiesSize()
            : 0;
        const bool contract_valid = drop_item_overlap_property_ &&
            sphere_component_class_ && sphere_radius_instance_property_ &&
            set_sphere_radius_function_ && set_sphere_radius_value_property_ &&
            set_sphere_radius_update_property_ && parameter_bytes > 0 &&
            parameter_bytes <= 64 && set_sphere_radius_function_->GetReturnProperty() == nullptr &&
            set_sphere_radius_function_->HasAllFunctionFlags(FUNC_Native) &&
            drop_item_properties_size > 0 && sphere_properties_size > 0 &&
            property_range_fits(drop_item_overlap_property_,
                                static_cast<std::size_t>(drop_item_properties_size),
                                sizeof(UObject*)) &&
            property_range_fits(sphere_radius_instance_property_,
                                static_cast<std::size_t>(sphere_properties_size),
                                sizeof(float)) &&
            property_range_fits(set_sphere_radius_value_property_, parameter_bytes,
                                sizeof(float)) &&
            property_range_fits(set_sphere_radius_update_property_, parameter_bytes,
                                sizeof(bool));
        if (!contract_valid) {
            logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_DISABLED",
                          std::format("reason=reflection_contract_missing multiplier={} "
                                      "drop_property={} sphere_class={} radius_property={} "
                                      "set_radius_function={} value_parameter={} update_parameter={} "
                                      "core_auto_pickup_available=1",
                                      drop_item_range_multiplier_,
                                      drop_item_overlap_property_ != nullptr,
                                      sphere_component_class_ != nullptr,
                                      sphere_radius_instance_property_ != nullptr,
                                      set_sphere_radius_function_ != nullptr,
                                      set_sphere_radius_value_property_ != nullptr,
                                      set_sphere_radius_update_property_ != nullptr));
            return;
        }

        drop_item_range_contract_ready_ = true;
        logger_.write(dsnap::LogAudience::User, "DROP_ITEM_RANGE_READY",
                      std::format("multiplier={} source=recognized_range_pak "
                                  "target=DropItemActor.SphereOverlapComp "
                                  "application=BeginPlay_exact_reflection object_scans=0",
                                  drop_item_range_multiplier_));
    }

    [[nodiscard]] bool apply_drop_item_range_unsafe(AActor* actor, const char** reason) {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!actor || !actor->IsA(drop_item_class_)) return reject("not_drop_item");
        if (!drop_item_range_contract_ready_ || drop_item_range_multiplier_ <= 1) {
            return reject("range_contract_inactive");
        }
        auto* overlap_storage = drop_item_overlap_property_->ContainerPtrToValuePtr<void>(actor);
        auto* overlap_component = overlap_storage
            ? drop_item_overlap_property_->GetObjectPropertyValue(overlap_storage)
            : nullptr;
        if (!overlap_component || !overlap_component->IsA(sphere_component_class_) ||
            overlap_component->GetOuterPrivate() != actor ||
            !actor->GetWorld() || overlap_component->GetWorld() != actor->GetWorld()) {
            return reject("overlap_component_contract_changed");
        }
        const auto base_radius = sphere_radius_instance_property_->GetPropertyValueInContainer(
            overlap_component);
        const auto requested_radius = base_radius * static_cast<float>(drop_item_range_multiplier_);
        if (!std::isfinite(base_radius) || base_radius <= 0.0F || base_radius > 100000.0F ||
            !std::isfinite(requested_radius) || requested_radius <= base_radius ||
            requested_radius > 2000000.0F) {
            return reject("overlap_radius_out_of_bounds");
        }

        const auto actor_identity = pack_weak_identity(FWeakObjectPtr{actor});
        const auto component_identity = pack_weak_identity(FWeakObjectPtr{overlap_component});
        std::vector<std::byte> parameters(set_sphere_radius_function_->GetParmsSize());
        set_sphere_radius_value_property_->SetPropertyValueInContainer(
            parameters.data(), requested_radius);
        set_sphere_radius_update_property_->SetPropertyValueInContainer(parameters.data(), true);
        overlap_component->ProcessEvent(set_sphere_radius_function_, parameters.data());
        // PROCESS_EVENT_RETURNED_SCALAR_ONLY: no UObject access below this line.
        const auto applied_index = drop_item_range_applied_.fetch_add(
            1, std::memory_order_relaxed) + 1;
        if (configuration_result_.value.debug_logging && applied_index <= 32) {
            logger_.write(dsnap::LogAudience::Debug, "DROP_ITEM_RANGE_APPLIED",
                          std::format("actor=0x{:X} component=0x{:X} base_radius={} "
                                      "requested_radius={} multiplier={} bounded_event={}/32",
                                      actor_identity, component_identity, base_radius,
                                      requested_radius, drop_item_range_multiplier_, applied_index));
        }
        if (reason) *reason = "applied";
        return true;
    }

    [[nodiscard]] static bool apply_drop_item_range_guarded(
        NativeAutoPickup* self,
        AActor* actor,
        const char** reason,
        bool* is_drop_item) noexcept {
        if (!self || !actor || !reason || !is_drop_item) return false;
#if defined(_MSC_VER)
        __try {
#endif
            *is_drop_item = actor->IsA(self->drop_item_class_);
            if (!*is_drop_item) return false;
            return self->apply_drop_item_range_unsafe(actor, reason);
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *is_drop_item = true;
            *reason = "guarded_runtime_fault";
            return false;
        }
#endif
    }

    void drop_item_begin_play_post(AActor* actor) noexcept {
        if (!drop_item_range_contract_ready_ || !actor) return;
        const char* reason{"unknown"};
        bool is_drop_item{};
        if (apply_drop_item_range_guarded(this, actor, &reason, &is_drop_item)) return;
        if (!is_drop_item) return;
        const auto failure_index = drop_item_range_failures_.fetch_add(
            1, std::memory_order_relaxed) + 1;
        if (failure_index <= 32) {
            logger_.write(dsnap::LogAudience::Debug, "DROP_ITEM_RANGE_REJECTED",
                          std::format("reason={} bounded_event={}/32 core_auto_pickup_available=1",
                                      reason, failure_index));
        }
    }

    static NativeAutoPickup* current_instance(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->callback_gate_.accepts(generation) &&
                       !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] bool register_server_run_interact_observer() noexcept {
        if (!server_run_interact_function_ || server_run_interact_hook_registered_) return false;
        const auto generation = callback_gate_.generation();
        try {
            server_run_interact_hook_ = UObjectGlobals::RegisterHook(
                server_run_interact_function_,
                [](UnrealScriptFunctionCallableContext&, void*) {},
                [generation](UnrealScriptFunctionCallableContext& context, void*) {
                    if (auto* self = current_instance(generation)) {
                        self->observe_server_run_interact_post(context);
                    }
                },
                nullptr);
            if (server_run_interact_hook_.first == Hook::ERROR_ID ||
                server_run_interact_hook_.second == Hook::ERROR_ID) {
                try {
                    UObjectGlobals::UnregisterHook(server_run_interact_function_,
                                                   server_run_interact_hook_);
                } catch (...) {
                }
                server_run_interact_hook_ = {};
                return false;
            }
            server_run_interact_hook_registered_ = true;
            return true;
        } catch (...) {
            server_run_interact_hook_ = {};
            return false;
        }
    }

    // Exact hook hot path: raw pointer comparison and atomic publication only.
    // Logging, reflection, UObject reads, and action-state mutation stay on the
    // next EngineTickPost after the hooked game call has fully returned.
    void observe_server_run_interact_post(
        const UnrealScriptFunctionCallableContext& context) noexcept {
        if (!dispatch_observer_armed_.load(std::memory_order_acquire)) return;
        const auto receiver_address =
            pending_receiver_address_.load(std::memory_order_acquire);
        if (receiver_address == 0 ||
            reinterpret_cast<std::uintptr_t>(context.Context) != receiver_address) {
            return;
        }
        const auto action_id = pending_action_id_visible_.load(std::memory_order_acquire);
        if (action_id == 0) return;
        std::uint64_t expected{};
        static_cast<void>(dispatch_observed_action_id_.compare_exchange_strong(
            expected, action_id, std::memory_order_release, std::memory_order_relaxed));
    }

    [[nodiscard]] bool apply_build_fingerprint_once() noexcept {
        if (fingerprint_applied_) {
            return build_trusted_.load(std::memory_order_acquire);
        }
        fingerprint_applied_ = true;
        try {
            fingerprint_result_ = dsnap::verify_build_fingerprint(binary_directory());
        } catch (const std::exception& error) {
            fingerprint_result_ = {};
            fingerprint_result_.error = std::format("fingerprint_exception:{}", error.what());
        } catch (...) {
            fingerprint_result_ = {};
            fingerprint_result_.error = "fingerprint_exception:unknown";
        }
        build_trusted_.store(fingerprint_result_.trusted, std::memory_order_release);
        logger_.write(dsnap::LogAudience::Debug, "FINGERPRINT",
                      std::format("trusted={} game_hash_policy=diagnostic_only game={} ue4ss={} error={}",
                                  fingerprint_result_.trusted, fingerprint_result_.game_sha256,
                                  fingerprint_result_.ue4ss_sha256, fingerprint_result_.error));
        return fingerprint_result_.trusted;
    }

    template <typename PropertyType>
    [[nodiscard]] static PropertyType* exact_property(FProperty* property) noexcept {
        auto* typed = CastField<PropertyType>(property);
        return typed && typed->GetClass() == PropertyType::StaticClass() ? typed : nullptr;
    }

    [[nodiscard]] static FObjectProperty* exact_mapping_action_property(
        FProperty* property,
        const char** storage_kind) noexcept {
        if (auto* object_property = exact_property<FObjectProperty>(property)) {
            if (storage_kind) *storage_kind = "ObjectProperty";
            return object_property;
        }
        if (auto* object_ptr_property = exact_property<FObjectPtrProperty>(property)) {
            if (storage_kind) *storage_kind = "ObjectPtrProperty";
            return object_ptr_property;
        }
        if (storage_kind) *storage_kind = property ? "unsupported" : "missing";
        return nullptr;
    }

    [[nodiscard]] bool resolve_selector_capability(UFunction* server_run_interact_function,
                                                    UFunction* set_interact_ui_function,
                                                    UObject* interactable_cdo,
                                                    std::int32_t target_object_offset,
                                                    std::int32_t target_component_offset) noexcept {
        selector_capability_ = {};
        const auto reject = [this](std::string_view status) {
            selector_capability_.address = 0;
            selector_capability_.selector_rva = 0;
            selector_capability_.server_address = 0;
            selector_capability_.server_call_site = 0;
            selector_capability_.ui_address = 0;
            selector_capability_.ui_implementation_call_site = 0;
            selector_capability_.ui_call_site = 0;
            selector_capability_.status = status;
            return false;
        };
        try {
            const auto loaded = load_main_text_guarded();
            if (!loaded.valid()) return reject(loaded.error);
            selector_capability_.module_base = loaded.module_base;
            selector_capability_.anchor_source =
                "Server_RunInteractV2 reflected virtual implementation + "
                "SetInteractUIV2 reflected direct implementation";
            if (!server_run_interact_function || !set_interact_ui_function || !interactable_cdo) {
                return reject("reflection_anchor_unavailable");
            }
            const auto server = resolve_reflected_virtual_function(
                loaded, server_run_interact_function, interactable_cdo);
            selector_capability_.server_dispatch_candidates = server.dispatch_candidates;
            selector_capability_.server_virtual_slot_offset = server.virtual_slot_offset;
            if (!server.resolved()) return reject(server.status);
            const auto ui = resolve_reflected_direct_function(loaded, set_interact_ui_function);
            selector_capability_.ui_implementation_call_candidates =
                ui.implementation_call_candidates;
            if (!ui.resolved()) return reject(ui.status);

            std::vector<std::uintptr_t> server_instructions{};
            std::size_t server_scan_limit{};
            if (!decode_runtime_function_addresses(loaded, server.implementation,
                                                    &server_instructions,
                                                    &server_scan_limit)) {
                return reject("server_runtime_function_decode_failed");
            }
            const auto server_selector = dsnap::resolve_selector_call(
                loaded.text, server.implementation, server_instructions,
                target_object_offset, target_component_offset, server_scan_limit);
            selector_capability_.server_selector_candidates = server_selector.candidates;
            if (!server_selector.resolved()) {
                return reject(dsnap::selector_resolver_status_name(server_selector.status));
            }
            std::vector<std::uintptr_t> ui_instructions{};
            std::size_t ui_scan_limit{};
            if (!decode_runtime_function_addresses(loaded, ui.implementation,
                                                    &ui_instructions, &ui_scan_limit)) {
                return reject("ui_runtime_function_decode_failed");
            }
            const auto ui_selector = dsnap::resolve_ui_selector_call(
                loaded.text, ui.implementation, ui_instructions, ui_scan_limit);
            selector_capability_.ui_selector_candidates = ui_selector.candidates;
            if (!ui_selector.resolved()) {
                return reject(dsnap::selector_resolver_status_name(ui_selector.status));
            }
            if (server_selector.selector_address != ui_selector.selector_address) {
                return reject("selector_consensus_mismatch");
            }
            if (!executable_page(server_selector.call_site) ||
                !executable_page(ui_selector.call_site)) {
                return reject("selector_call_page_not_executable");
            }
            if (!executable_page(server_selector.selector_address)) {
                return reject("selector_target_page_not_executable");
            }
            selector_capability_.address = server_selector.selector_address;
            selector_capability_.selector_rva = server_selector.selector_address - loaded.module_base;
            selector_capability_.server_address = server.implementation;
            selector_capability_.server_call_site = server_selector.call_site;
            selector_capability_.ui_address = ui.implementation;
            selector_capability_.ui_implementation_call_site = ui.implementation_call_site;
            selector_capability_.ui_call_site = ui_selector.call_site;
            selector_capability_.status = "resolved";
            return true;
        } catch (...) {
            return reject("resolver_exception");
        }
    }

    [[nodiscard]] bool validate_reflection_contract(
        UFunction* server_run_interact_function,
        UFunction* set_interact_ui_function,
        UObject* interactable_cdo,
        FObjectProperty* target_object_property,
        FObjectProperty* target_component_property,
        ReflectionContractReport* output) const noexcept {
        ReflectionContractReport report{};
        const auto finish = [output](const ReflectionContractReport& value) noexcept {
            if (output) *output = value;
            return value.required_valid;
        };
        if (!drop_item_class_ || !interactable_class_ || !enhanced_player_input_class_ ||
            !input_action_class_ || !enhanced_action_mapping_struct_ || !key_struct_ ||
            !enhanced_input_subsystem_class_ || !subsystem_library_class_ || !subsystem_library_cdo_ ||
            !get_local_player_subsystem_function_ || !inject_input_vector_function_ ||
            !server_run_interact_function || !set_interact_ui_function ||
            !interactable_cdo || !target_object_property ||
            !target_component_property) {
            report.required_failure = "required_reflection_object_missing";
            return finish(report);
        }
        const auto get_subsystem_parameter_bytes = get_local_player_subsystem_function_->GetParmsSize();
        const auto inject_parameter_bytes = inject_input_vector_function_->GetParmsSize();
        auto* player_controller_property = exact_property<FObjectProperty>(
            function_property(get_local_player_subsystem_function_, STR("PlayerController")));
        auto* subsystem_class_property = exact_property<FClassProperty>(
            function_property(get_local_player_subsystem_function_, STR("Class")));
        auto* subsystem_return_property = exact_property<FObjectProperty>(
            get_local_player_subsystem_function_->GetReturnProperty());
        auto* action_property = exact_property<FObjectProperty>(
            function_property(inject_input_vector_function_, STR("Action")));
        auto* value_property = exact_property<FStructProperty>(
            function_property(inject_input_vector_function_, STR("Value")));
        auto* modifiers_property = exact_property<FArrayProperty>(
            function_property(inject_input_vector_function_, STR("Modifiers")));
        auto* triggers_property = exact_property<FArrayProperty>(
            function_property(inject_input_vector_function_, STR("Triggers")));
        const auto set_interact_ui_parameter_bytes = set_interact_ui_function->GetParmsSize();
        auto* interact_actor_property_any =
            function_property(set_interact_ui_function, STR("InteractActor"));
        auto* interact_actor_property = exact_property<FObjectProperty>(interact_actor_property_any);
        const auto interactable_properties_size = interactable_class_->GetPropertiesSize();
        const auto target_object_offset = target_object_property->GetOffset_Internal();
        const auto target_component_offset = target_component_property->GetOffset_Internal();
        report.get_subsystem_parameter_bytes = get_subsystem_parameter_bytes;
        report.inject_parameter_bytes = inject_parameter_bytes;
        report.server_parameter_bytes = server_run_interact_function->GetParmsSize();
        report.server_function_flags =
            static_cast<std::uint32_t>(server_run_interact_function->GetFunctionFlags());
        report.interactable_properties_size = interactable_properties_size;
        report.target_object_offset = target_object_offset;
        report.target_component_offset = target_component_offset;
        report.ui_parameter_bytes = set_interact_ui_parameter_bytes;
        report.ui_function_flags =
            static_cast<std::uint32_t>(set_interact_ui_function->GetFunctionFlags());
        report.ui_has_return = set_interact_ui_function->GetReturnProperty() != nullptr;
        report.ui_interact_actor_present = interact_actor_property_any != nullptr;
        report.ui_interact_actor_exact_object = interact_actor_property != nullptr;
        if (interact_actor_property_any) {
            report.ui_interact_actor_offset = interact_actor_property_any->GetOffset_Internal();
            report.ui_interact_actor_element_size = interact_actor_property_any->GetElementSize();
            report.ui_interact_actor_array_dim = interact_actor_property_any->GetArrayDim();
        }
        // This UFunction is a machine-code anchor only. We neither invoke it nor
        // read its parameters, so parameter-layout drift is diagnostic rather
        // than an action-safety gate. The resolver still requires bounded code,
        // one structural UI selector call, and exact Server/UI consensus.
        report.ui_schema_advisory_valid =
            set_interact_ui_function->HasAllFunctionFlags(FUNC_Native) &&
            set_interact_ui_parameter_bytes == sizeof(UObject*) &&
            set_interact_ui_function->GetReturnProperty() == nullptr &&
            interact_actor_property && interact_actor_property->GetOffset_Internal() == 0 &&
            property_range_fits(interact_actor_property, set_interact_ui_parameter_bytes,
                                sizeof(UObject*));

        if (get_subsystem_parameter_bytes <= 0 ||
            get_subsystem_parameter_bytes > kMaxFunctionParameterBytes) {
            report.required_failure = "get_subsystem_parameter_size_invalid";
        } else if (inject_parameter_bytes <= 0 ||
                   inject_parameter_bytes > kMaxFunctionParameterBytes) {
            report.required_failure = "inject_parameter_size_invalid";
        } else if (!player_controller_property || !subsystem_class_property ||
                   !subsystem_return_property || !action_property || !value_property ||
                   !modifiers_property || !triggers_property) {
            report.required_failure = "enhanced_input_property_missing_or_wrong_type";
        } else if (!property_range_fits(player_controller_property,
                                        get_subsystem_parameter_bytes, sizeof(UObject*)) ||
                   !property_range_fits(subsystem_class_property,
                                        get_subsystem_parameter_bytes, sizeof(UClass*)) ||
                   !property_range_fits(subsystem_return_property,
                                        get_subsystem_parameter_bytes, sizeof(UObject*))) {
            report.required_failure = "get_subsystem_property_layout_invalid";
        } else if (!property_range_fits(action_property, inject_parameter_bytes,
                                        sizeof(UObject*)) ||
                   !property_range_fits(value_property, inject_parameter_bytes,
                                        FVector::StaticSize()) ||
                   !property_range_fits(modifiers_property, inject_parameter_bytes,
                                        sizeof(FScriptArray)) ||
                   !property_range_fits(triggers_property, inject_parameter_bytes,
                                        sizeof(FScriptArray))) {
            report.required_failure = "inject_property_layout_invalid";
        } else if (server_run_interact_function->GetParmsSize() != 0 ||
                   server_run_interact_function->GetReturnProperty() != nullptr ||
                   !server_run_interact_function->HasAllFunctionFlags(
                       FUNC_Native | FUNC_Net | FUNC_NetServer)) {
            report.required_failure = "server_anchor_schema_invalid";
        } else if (interactable_properties_size <= 0 ||
                   !property_range_fits(
                       target_object_property,
                       static_cast<std::size_t>(interactable_properties_size), sizeof(UObject*)) ||
                   !property_range_fits(
                       target_component_property,
                       static_cast<std::size_t>(interactable_properties_size), sizeof(UObject*)) ||
                   target_component_offset !=
                       target_object_offset + static_cast<std::int32_t>(sizeof(UObject*))) {
            report.required_failure = "selector_output_property_layout_invalid";
        } else {
            report.required_valid = true;
            report.required_failure = "none";
        }
        return finish(report);
    }

    [[nodiscard]] static std::string reflected_parameter_metadata(UFunction* function) {
        if (!function) return "function_missing";
        std::string metadata{};
        for (auto* property :
             TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated)) {
            if (!property->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm)) continue;
            if (!metadata.empty()) metadata += ";";
            metadata += std::format("{}:{}:size={}:element={}:offset={}:array={}:return={}",
                                    to_string(property->GetName()),
                                    to_string(property->GetClass().GetName()),
                                    property->GetSize(), property->GetElementSize(),
                                    property->GetOffset_Internal(), property->GetArrayDim(),
                                    property->HasAnyPropertyFlags(EPropertyFlags::CPF_ReturnParm));
            if (metadata.size() >= 2048) {
                metadata.resize(2048);
                metadata += ":truncated";
                break;
            }
        }
        return metadata.empty() ? "none" : metadata;
    }

    void log_reflection_contract_detail(const ReflectionContractReport& report,
                                        UFunction* set_interact_ui_function) {
        logger_.write(
            report.required_valid ? dsnap::LogAudience::Debug : dsnap::LogAudience::User,
            "REFLECTION_CONTRACT_DETAIL",
            std::format(
                "required_valid={} required_failure={} get_subsystem_parms={} inject_parms={} "
                "server_parms={} server_flags=0x{:X} interactable_size={} "
                "target_object_offset={} target_component_offset={} "
                "ui_schema_advisory_valid={} ui_parms={} ui_flags=0x{:X} ui_has_return={} "
                "ui_interact_actor_present={} ui_interact_actor_exact_object={} "
                "ui_interact_actor_offset={} ui_interact_actor_element={} "
                "ui_interact_actor_array={} ui_parameters={}",
                report.required_valid, report.required_failure,
                report.get_subsystem_parameter_bytes, report.inject_parameter_bytes,
                report.server_parameter_bytes, report.server_function_flags,
                report.interactable_properties_size, report.target_object_offset,
                report.target_component_offset, report.ui_schema_advisory_valid,
                report.ui_parameter_bytes, report.ui_function_flags, report.ui_has_return,
                report.ui_interact_actor_present, report.ui_interact_actor_exact_object,
                report.ui_interact_actor_offset, report.ui_interact_actor_element_size,
                report.ui_interact_actor_array_dim,
                reflected_parameter_metadata(set_interact_ui_function)));
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

    [[nodiscard]] static bool write_object_function_parameter(
        UFunction* function,
        std::vector<std::byte>& parameters,
        const wchar_t* name,
        UObject* value) noexcept {
        auto* property = exact_property<FObjectProperty>(function_property(function, name));
        if (!property_range_fits(property, parameters.size(), sizeof(UObject*))) return false;
        auto* address = parameters.data() + property->GetOffset_Internal();
        property->SetObjectPropertyValue(address, value);
        return property->GetObjectPropertyValue(address) == value;
    }

    [[nodiscard]] static bool write_class_function_parameter(
        UFunction* function,
        std::vector<std::byte>& parameters,
        const wchar_t* name,
        UClass* value) noexcept {
        auto* property = exact_property<FClassProperty>(function_property(function, name));
        if (!property_range_fits(property, parameters.size(), sizeof(UClass*))) return false;
        auto* address = parameters.data() + property->GetOffset_Internal();
        property->SetObjectPropertyValue(address, value);
        return property->GetObjectPropertyValue(address) == value;
    }

    template <typename ValueType>
    [[nodiscard]] static bool write_struct_function_parameter(
        UFunction* function,
        std::vector<std::byte>& parameters,
        const wchar_t* name,
        const ValueType& value) noexcept {
        auto* property = exact_property<FStructProperty>(function_property(function, name));
        if (!property_range_fits(property, parameters.size(), sizeof(ValueType))) return false;
        std::memcpy(parameters.data() + property->GetOffset_Internal(), &value, sizeof(ValueType));
        return true;
    }

    [[nodiscard]] static UObject* named_object(UObject* owner, const wchar_t* name) {
        if (!owner) return nullptr;
        auto** value = owner->GetValuePtrByPropertyNameInChain<UObject*>(name);
        return value ? *value : nullptr;
    }

    [[nodiscard]] static std::uint64_t resolve_viewport_world_identity(UEngine* engine) noexcept {
        if (!engine) return 0;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        auto* world = viewport ? viewport->GetWorld() : nullptr;
        return world
            ? pack_weak_identity(FWeakObjectPtr{reinterpret_cast<UObject*>(world)})
            : 0;
    }

    [[nodiscard]] bool resolve_player_context(UEngine* engine,
                                              PlayerContext* output,
                                              const char** reason = nullptr) {
        const auto reject = [reason](const char* value) {
            if (reason) *reason = value;
            return false;
        };
        if (!engine || !output) return reject("engine_or_output_invalid");
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        if (!viewport_value) return reject("game_viewport_property_missing");
        if (!viewport) return reject("game_viewport_value_null");
        auto* game_instance = named_object(viewport, STR("GameInstance"));
        if (!game_instance) return reject("game_instance_unavailable");
        auto* players = game_instance
            ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers")) : nullptr;
        if (!players) return reject("local_players_property_missing");
        if (!players->IsValidIndex(0)) return reject("local_players_invalid_index");
        if (!players->GetData()) return reject("local_players_null_data");
        auto* local_player = static_cast<UObject* const*>(players->GetData())[0];
        if (!local_player) return reject("local_player_entry_null");
        auto* controller = named_object(local_player, STR("PlayerController"));
        if (!controller) return reject("local_player_controller_unavailable");
        if (named_object(controller, STR("Player")) != local_player) {
            return reject("controller_player_identity_mismatch");
        }
        auto* pawn = named_object(controller, STR("Pawn"));
        if (!pawn) return reject("controller_pawn_unavailable");
        if (named_object(pawn, STR("Controller")) != controller) {
            return reject("pawn_controller_identity_mismatch");
        }
        if (!pawn->GetWorld() || controller->GetWorld() != pawn->GetWorld()) {
            return reject("controller_pawn_world_mismatch");
        }

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
        if (reason) *reason = "success";
        return true;
    }

    [[nodiscard]] bool resolve_player_context_guarded(UEngine* engine,
                                                       PlayerContext* output,
                                                       const char** reason = nullptr) noexcept {
#if defined(_MSC_VER)
        __try {
#endif
            return resolve_player_context(engine, output, reason);
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            if (reason) *reason = "guarded_player_context_fault";
            return false;
        }
#endif
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
        FObjectProperty* action_property{};
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
        const char* action_property_storage{"unresolved"};
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
            ? resolved.mapping_property->GetStruct().Get()
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
        auto* action_property_any = resolved.mapping_struct->FindProperty(action_name);
        resolved.action_property = exact_mapping_action_property(
            action_property_any, &resolved.action_property_storage);
        resolved.key_property = CastField<FStructProperty>(resolved.mapping_struct->FindProperty(key_name));
        resolved.ignored_property = CastField<FBoolProperty>(resolved.mapping_struct->FindProperty(ignored_name));
        auto* reflected_key_struct = resolved.key_property
            ? resolved.key_property->GetStruct().Get()
            : nullptr;
        resolved.key_name_property = reflected_key_struct
            ? CastField<FNameProperty>(reflected_key_struct->FindProperty(key_name_name))
            : nullptr;
        if (!resolved.action_property) {
            *output = resolved;
            return reject("enhanced_action_mapping_action_property_class_unsupported");
        }
        if (!resolved.key_property) {
            *output = resolved;
            return reject("enhanced_action_mapping_key_property_class_unsupported");
        }
        if (!resolved.ignored_property) {
            *output = resolved;
            return reject("enhanced_action_mapping_ignored_property_class_unsupported");
        }
        if (!resolved.key_name_property) {
            *output = resolved;
            return reject("enhanced_action_mapping_key_name_property_class_unsupported");
        }
        if (reflected_key_struct != key_struct_) {
            *output = resolved;
            return reject("enhanced_action_mapping_key_struct_mismatch");
        }

        auto* action_property_class = resolved.action_property->GetPropertyClass().Get();
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

        FScriptArrayHelper_InContainer mappings{resolved.mappings_property, resolved.player_input};
        resolved.mapping_count = mappings.NumUnchecked();
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
            if (!mappings.IsValidIndex(index)) {
                *output = resolved;
                return reject("enhanced_action_mapping_index_invalid");
            }
            const auto* entry = reinterpret_cast<const std::byte*>(mappings.GetElementPtr(index));
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

    [[nodiscard]] UObject* resolve_enhanced_input_subsystem(
        UObject* controller,
        std::uint64_t expected_activation_id,
        std::uint64_t expected_world_identity,
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
        auto* return_property = exact_property<FObjectProperty>(
            get_local_player_subsystem_function_->GetReturnProperty());
        if (!return_property ||
            !property_range_fits(return_property, parameters.size(), sizeof(UObject*))) {
            return reject("get_subsystem_return_layout_rejected");
        }
        if (!write_object_function_parameter(get_local_player_subsystem_function_, parameters,
                                             STR("PlayerController"), controller) ||
            !write_class_function_parameter(get_local_player_subsystem_function_, parameters,
                                            STR("Class"), enhanced_input_subsystem_class_)) {
            return reject("get_subsystem_parameter_layout_rejected");
        }
        subsystem_library_cdo_->ProcessEvent(get_local_player_subsystem_function_, parameters.data());
        // The UFunction may synchronously reenter lifecycle callbacks. Reject
        // that scalar state change before interpreting its UObject return.
        if (shutting_down_.load(std::memory_order_acquire) ||
            !automation_enabled_.load(std::memory_order_acquire) ||
            action_activation_id_ != expected_activation_id || expected_world_identity == 0 ||
            active_world_identity_.load(std::memory_order_acquire) != expected_world_identity) {
            return reject("action_context_changed_during_subsystem_resolution");
        }
        auto* subsystem = return_property->GetObjectPropertyValue(
            parameters.data() + return_property->GetOffset_Internal());
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
        if (!write_object_function_parameter(inject_input_vector_function_, parameters, STR("Action"), action) ||
            !write_struct_function_parameter(inject_input_vector_function_, parameters, STR("Value"), pressed_value)) {
            return reject("inject_input_parameter_layout_rejected");
        }
        subsystem->ProcessEvent(inject_input_vector_function_, parameters.data());
        if (reason) *reason = "live_interaction_action_injected";
        return true;
    }

    void engine_tick_post(UEngine* engine) noexcept {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        // Do not let a nested tick consume dispatch state while the outer
        // tick is still inside ProcessEvent. Cadence is not a reentry guard.
        if (engine_tick_active_.test_and_set(std::memory_order_acquire)) return;
        ScopeExit tick_entry_guard{[this]() noexcept {
            engine_tick_active_.clear(std::memory_order_release);
        }};
        const auto tick_sequence = next_tick_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
        logger_.set_tick_sequence(tick_sequence);
        if (runtime_initialization_state_ == RuntimeInitializationState::Pending) {
            const auto now = Clock::now();
            if (now >= next_runtime_initialization_attempt_) {
                runtime_initialization_retry_in_progress_ = true;
                on_unreal_init();
                runtime_initialization_retry_in_progress_ = false;
            }
            return;
        }
        if (runtime_initialization_state_ != RuntimeInitializationState::Ready) return;
        const auto tick_started = Clock::now();
        StageTimings tick_stage_timings{};
        ScopeExit status_toast_tick{[this, engine]() noexcept {
            update_status_toast(engine);
        }};
        ScopeExit tick_diagnostic{[this, tick_started, &tick_stage_timings]() noexcept {
            maybe_log_slow_tick(tick_started, tick_stage_timings);
        }};
        const auto now = Clock::now();
        // Enabled work is frame-paced. The one-pending-action gate below still
        // prevents another input until the previous dispatch/confirmation.
        // Off-state maintenance keeps its bounded pulse; empty scans keep their
        // independent idle due-time. Debug is deliberately not a scheduling input.
        if (!automation_enabled_.load(std::memory_order_acquire) &&
            next_pulse_due_ != Clock::time_point{} && now < next_pulse_due_) return;
        if (configuration_result_.value.debug_logging && last_pulse_at_ != Clock::time_point{}) {
            const auto gap_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now - last_pulse_at_).count();
            pulse_gap_timing_.record(gap_us > 0 ? static_cast<std::uint64_t>(gap_us) : 0);
        }
        last_pulse_at_ = now;
        ScopeTiming pulse_work{configuration_result_.value.debug_logging ? &pulse_work_timing_ : nullptr};
        next_pulse_due_ = now + kPulseInterval;

        const bool enabled_before_toggle = automation_enabled_.load(std::memory_order_acquire);
        const auto current_world_identity = resolve_viewport_world_identity(engine);
        if (ready_world_identity_ != 0 && current_world_identity != ready_world_identity_) {
            reset_world(current_world_identity == 0
                            ? "viewport_world_unavailable"
                            : "playable_world_identity_changed");
            return;
        }
        if (ready_world_identity_ == 0 && now >= next_readiness_probe_due_) {
            next_readiness_probe_due_ = now + kReadinessProbeInterval;
            PlayerContext readiness_context{};
            const auto session_generation = session_events_.current_generation();
            if (current_world_identity != 0 && now >= world_settle_until_ &&
                resolve_player_context_guarded(engine, &readiness_context) &&
                validate_receiver(readiness_context, nullptr) && readiness_context.pawn &&
                pack_weak_identity(FWeakObjectPtr{
                    reinterpret_cast<UObject*>(readiness_context.pawn->GetWorld())}) == current_world_identity &&
                session_events_.mark_playable(session_generation)) {
                ready_world_identity_ = current_world_identity;
                logger_.write(dsnap::LogAudience::User, "PLAYABLE_SESSION_READY",
                              std::format("session_generation={} world_identity=0x{:016X} "
                                          "hotkey_acceptance=enabled",
                                          session_generation, ready_world_identity_));
            }
        }
        const auto active_world_identity = active_world_identity_.load(std::memory_order_acquire);
        if (enabled_before_toggle &&
            (active_world_identity == 0 || current_world_identity == 0 ||
             current_world_identity != active_world_identity)) {
            reset_world(current_world_identity == 0
                            ? "playable_world_unavailable"
                            : "playable_world_identity_changed");
            return;
        }

        const auto toggles = session_events_.drain_toggle_requests();
        const auto toggle_requests = toggles.count;
        if ((toggle_requests & 1ULL) != 0) {
            const bool enable = !automation_enabled_.load(std::memory_order_acquire);
            ++toggle_transitions_;
            if (enable) {
                status_toast_renderer_.notify(dsnap::StatusToastKind::Starting, now);
                const bool allowed = selector_capability_.resolved() &&
                                     build_trusted_.load(std::memory_order_acquire) &&
                                     configuration_result_.value.automatic_pickup;
                PlayerContext lifecycle_context{};
                const bool playable_interaction_context =
                    ready_world_identity_ != 0 && current_world_identity == ready_world_identity_ &&
                    resolve_player_context_guarded(engine, &lifecycle_context) &&
                    validate_receiver(lifecycle_context, nullptr) && lifecycle_context.pawn &&
                    pack_weak_identity(FWeakObjectPtr{
                        reinterpret_cast<UObject*>(lifecycle_context.pawn->GetWorld())}) ==
                        current_world_identity;
                if (!selector_capability_.resolved()) {
                    status_toast_renderer_.notify(dsnap::StatusToastKind::Unavailable, now);
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  std::format("reason=selector_capability_{} source=configured_hotkey_toggle "
                                              "fail_closed=1",
                                              selector_capability_.status));
                } else if (!allowed) {
                    status_toast_renderer_.notify(dsnap::StatusToastKind::Unavailable, now);
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  "reason=untrusted_or_disabled source=configured_hotkey_toggle");
                } else if (current_world_identity == 0 || !playable_interaction_context) {
                    status_toast_renderer_.notify(dsnap::StatusToastKind::Unavailable, now);
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  "reason=playable_world_unavailable source=configured_hotkey_toggle "
                                  "retry_after_world_load=1");
                } else if (!refresh_interaction_binding()) {
                    status_toast_renderer_.notify(dsnap::StatusToastKind::Unavailable, now);
                    ++action_failures_;
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_REJECTED",
                                  "reason=interaction_binding_unavailable source=configured_hotkey_toggle");
                } else {
                    cancel_window("toggle_on_reset");
                    reset_action_activation("toggle_on_reset", false);
                    ++action_activation_id_;
                    active_world_identity_.store(current_world_identity, std::memory_order_release);
                    automation_enabled_.store(true, std::memory_order_release);
                    next_auto_scan_due_ = now < world_settle_until_ ? world_settle_until_ : now;
                    status_toast_renderer_.notify(dsnap::StatusToastKind::Enabled, now);
                    logger_.write(dsnap::LogAudience::User, "AUTOMATION_ENABLED",
                                  std::format("source=configured_hotkey_toggle hotkey={} activation_id={} "
                                              "interaction_range=game_or_optional_pak "
                                              "mounted_rider_route=1 "
                                              "active_scan_ms={} idle_scan_ms={} post_pickup_ms={} "
                                              "world_settle_remaining_ms={}",
                                              configuration_result_.value.toggle_hotkey,
                                              action_activation_id_,
                                              kActiveScanInterval.count(), kIdleScanInterval.count(),
                                              kPostPickupCooldown.count(),
                                              now < world_settle_until_
                                                  ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        world_settle_until_ - now).count()
                                                  : 0));
                }
            } else {
                automation_enabled_.store(false, std::memory_order_release);
                active_world_identity_.store(0, std::memory_order_release);
                cancel_window("toggle_off");
                reset_action_activation("toggle_off", true);
                next_auto_scan_due_ = {};
                status_toast_renderer_.notify(dsnap::StatusToastKind::Disabled, now);
                logger_.write(dsnap::LogAudience::User, "AUTOMATION_DISABLED",
                              std::format("reason=configured_hotkey_toggle hotkey={} pending_action_state=cleared",
                                          configuration_result_.value.toggle_hotkey));
            }
        }

        if (!automation_enabled_.load(std::memory_order_acquire)) return;
        poll_pending_action(now);
        if (!automation_enabled_.load(std::memory_order_acquire)) return;
        if (automatic_action_.pending()) {
            ++pending_scan_suppressions_;
            return;
        }
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
        // Start the attempt budget after unrelated pulse/toggle diagnostics.
        start_window(request_id, Clock::now());

        ++window_samples_;
        const auto attempt_started = Clock::now();
        const char* reason{"unknown"};
        bool retry_no_candidate{};
        bool selector_called{};
        bool selector_faulted{};
        bool runtime_faulted{};
        const bool invoked = invoke_pickup_guarded(this, engine, active_request_id_, true,
                                                   &reason, &retry_no_candidate, &selector_called,
                                                   &selector_faulted, &runtime_faulted,
                                                   configuration_result_.value.debug_logging
                                                       ? &tick_stage_timings : nullptr);
        const auto attempt_finished = Clock::now();
        const auto attempt_us = std::chrono::duration_cast<std::chrono::microseconds>(
            attempt_finished - attempt_started).count();
        if (configuration_result_.value.debug_logging) {
            const auto bounded_attempt_us = attempt_us > 0 ? static_cast<std::uint64_t>(attempt_us) : 0;
            scan_timing_.record(bounded_attempt_us);
            if (tick_stage_timings.context_us != 0) context_timing_.record(tick_stage_timings.context_us);
            if (tick_stage_timings.selector_us != 0) selector_timing_.record(tick_stage_timings.selector_us);
            if (tick_stage_timings.validation_us != 0) validation_timing_.record(tick_stage_timings.validation_us);
            if (tick_stage_timings.action_resolution_us != 0) {
                action_resolution_timing_.record(tick_stage_timings.action_resolution_us);
            }
            if (tick_stage_timings.subsystem_us != 0) subsystem_timing_.record(tick_stage_timings.subsystem_us);
            if (tick_stage_timings.injection_us != 0) injection_timing_.record(tick_stage_timings.injection_us);
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
            close_window_state();
            ++windows_hit_;
            next_auto_scan_due_ = attempt_finished + kActiveScanInterval;
            return;
        }

        close_window_state();
        const std::string_view failure_reason{reason ? reason : "unknown"};
        if (failure_reason == "active_world_identity_changed") {
            ++windows_cancelled_;
            ++action_failures_;
            reset_world("player_world_identity_changed_during_scan");
            return;
        }
        if (failure_reason == "interaction_owner_changed") {
            ++windows_cancelled_;
            ++transient_failures_;
            return;
        }
        if (retry_no_candidate) {
            ++selector_no_candidate_;
            next_auto_scan_due_ = attempt_finished + kIdleScanInterval;
            return;
        }

        ++windows_cancelled_;
        ++action_failures_;
        const bool hard_fault = selector_faulted || runtime_faulted ||
            failure_reason == "native_selector_fault" ||
            failure_reason == "guarded_runtime_fault" ||
            failure_reason == "action_state_pending_invariant" ||
            failure_reason == "action_state_fail_closed" ||
            failure_reason == "action_candidate_identity_invalid";
        if (hard_fault) {
            if (automatic_action_.pending()) {
                disable_for_action_fault(reason ? reason : "guarded_runtime_fault");
                return;
            }
            automation_enabled_.store(false, std::memory_order_release);
            active_world_identity_.store(0, std::memory_order_release);
            ++automatic_disables_;
            next_auto_scan_due_ = {};
            status_toast_renderer_.notify(dsnap::StatusToastKind::Disabled,
                                          attempt_finished);
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
        if (configuration_result_.value.debug_logging) {
            const bool reason_changed = !deferred_reason_observed_ ||
                last_deferred_reason_ != failure_reason;
            if (reason_changed &&
                deferred_debug_events_this_activation_ < kMaxDeferredDebugEventsPerActivation) {
                ++deferred_debug_events_this_activation_;
                ++debug_deferred_emitted_;
                logger_.write(dsnap::LogAudience::Debug, "AUTO_SCAN_DEFERRED",
                              std::format("request_id={} reason={} attempt_us={} backoff_ms={} "
                                          "selector_called={} selector_faulted={} runtime_faulted={} "
                                          "change_only=1 activation_event={}/{}",
                                          request_id, failure_reason, attempt_us, backoff.count(),
                                          selector_called, selector_faulted, runtime_faulted,
                                          deferred_debug_events_this_activation_,
                                          kMaxDeferredDebugEventsPerActivation));
            } else {
                ++debug_deferred_suppressed_;
            }
            deferred_reason_observed_ = true;
            last_deferred_reason_ = failure_reason;
        }
    }

    bool invoke_pickup_unsafe(UEngine* engine,
                              std::uint64_t request_id,
                              bool first_sample,
                              const char** reason,
                              bool* retry_no_candidate,
                              bool* selector_called,
                              bool* selector_faulted,
                              StageTimings* timing) {
        // Capture only owned scalar diagnostics before either ProcessEvent
        // boundary. Formatting/enqueue must happen after the action decision,
        // otherwise Debug logging can consume the guarded selector window.
        struct DeferredContextDiagnostic {
            bool emit{};
            bool resolved{};
            bool mounted{};
            std::uint64_t pawn_identity{};
            std::uint64_t owner_identity{};
            std::uint64_t receiver_identity{};
            std::uint32_t activation_event{};
            std::array<char, 128> reason{};
            std::array<char, 128> receiver_source{};
        } context_diagnostic{};
        struct DeferredSelectorDiagnostic {
            bool emit{};
            bool actor_is_drop_item{};
            bool component_is_interactable{};
            bool relation_matches{};
            bool target_type_supported{};
            std::uint64_t actor_identity{};
            std::uint64_t component_identity{};
            std::uint64_t outer_identity{};
            std::uint64_t receiver_identity{};
            int state_value{};
            int type_value{};
            std::uint32_t activation_event{};
            std::array<char, 128> receiver_source{};
        } selector_diagnostic{};
        const auto copy_diagnostic_token = [](std::string_view token) noexcept {
            std::array<char, 128> copied{};
            const auto count = std::min(token.size(), copied.size() - 1);
            if (count != 0) std::copy_n(token.data(), count, copied.data());
            return copied;
        };

        const auto run_pickup_decision = [&]() -> bool {
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
            const char* context_reason{"unknown"};
            const bool context_resolved = resolve_player_context_guarded(
                engine, &context, &context_reason);
            if (first_sample) {
                const std::string_view context_reason_view{
                    context_reason ? context_reason : "unknown"};
                const auto pawn_identity = context.pawn
                    ? pack_weak_identity(FWeakObjectPtr{context.pawn})
                    : 0;
                const auto owner_identity = context.interaction_owner
                    ? pack_weak_identity(FWeakObjectPtr{context.interaction_owner})
                    : 0;
                const auto receiver_identity = context.interaction_receiver
                    ? pack_weak_identity(FWeakObjectPtr{context.interaction_receiver})
                    : 0;
                const std::string_view receiver_source_view{
                    context.receiver_source ? context.receiver_source : "none"};
                const bool changed = !player_context_observed_ ||
                    context_resolved != last_context_resolved_ ||
                    context_reason_view != last_player_context_reason_ ||
                    pawn_identity != last_pawn_identity_ ||
                    receiver_identity != last_receiver_identity_ ||
                    context.mounted != last_context_mounted_ ||
                    receiver_source_view != last_receiver_source_;
                if (changed) {
                    if (configuration_result_.value.debug_logging) {
                        if (context_debug_events_this_activation_ <
                            kMaxPlayerContextDebugEventsPerActivation) {
                            ++context_debug_events_this_activation_;
                            ++debug_context_emitted_;
                            context_diagnostic = DeferredContextDiagnostic{
                                true, context_resolved, context.mounted,
                                pawn_identity, owner_identity, receiver_identity,
                                context_debug_events_this_activation_,
                                copy_diagnostic_token(context_reason_view),
                                copy_diagnostic_token(receiver_source_view),
                            };
                        } else {
                            ++debug_context_suppressed_;
                        }
                    }
                    player_context_observed_ = true;
                    last_context_resolved_ = context_resolved;
                    last_player_context_reason_ = context_reason_view;
                    last_pawn_identity_ = pawn_identity;
                    last_receiver_identity_ = receiver_identity;
                    last_context_mounted_ = context.mounted;
                    last_receiver_source_ = receiver_source_view;
                }
            }
        if (!context_resolved) return reject(context_reason);
            if (!validate_receiver(context, &receiver_reason)) return reject(receiver_reason);
            const auto active_world_identity =
                active_world_identity_.load(std::memory_order_acquire);
            const auto pawn_world_identity = context.pawn && context.pawn->GetWorld()
                ? pack_weak_identity(FWeakObjectPtr{
                      reinterpret_cast<UObject*>(context.pawn->GetWorld())})
                : 0;
            if (active_world_identity == 0 || pawn_world_identity != active_world_identity) {
                return reject("active_world_identity_changed");
            }
            const auto owner_identity = context.interaction_owner
                ? pack_weak_identity(FWeakObjectPtr{context.interaction_owner})
                : 0;
            if (owner_identity == 0) return reject("interaction_owner_identity_invalid");
            if (active_interaction_owner_identity_ == 0) {
                active_interaction_owner_identity_ = owner_identity;
            } else if (active_interaction_owner_identity_ != owner_identity) {
                const auto previous_owner_identity = active_interaction_owner_identity_;
                reset_action_activation("interaction_owner_changed", true);
                ++action_activation_id_;
                active_interaction_owner_identity_ = owner_identity;
                const auto owner_settle_until = Clock::now() + kWorldSettleDelay;
                if (world_settle_until_ < owner_settle_until) {
                    world_settle_until_ = owner_settle_until;
                }
                next_auto_scan_due_ = world_settle_until_;
                logger_.write(dsnap::LogAudience::User, "ACTION_CONTEXT_RESET",
                              std::format("reason=interaction_owner_changed previous_owner=0x{:X} "
                                          "current_owner=0x{:X} activation_id={} automation_enabled=1 "
                                          "pending_and_attempt_records=cleared settle_ms={}",
                                          previous_owner_identity, owner_identity,
                                          action_activation_id_, kWorldSettleDelay.count()));
                return reject("interaction_owner_changed");
            }
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
        const auto observed_actor_identity = pack_weak_identity(FWeakObjectPtr{selected.actor});
        const auto observed_component_identity = pack_weak_identity(FWeakObjectPtr{selected.component});
        const auto observed_type_value = observed_interact_type
            ? static_cast<int>(*observed_interact_type) : -1;
        const auto observed_state_value = observed_interactable
            ? static_cast<int>(*observed_interactable) : -1;
        const bool selector_observation_changed =
            observed_actor_identity != last_observed_actor_identity_ ||
            observed_component_identity != last_observed_component_identity_ ||
            observed_type_value != last_observed_interact_type_ ||
            observed_state_value != last_observed_interactable_ ||
            target_type_supported != last_observed_supported_;
        if (selector_observation_changed && configuration_result_.value.debug_logging) {
            if (selector_debug_events_this_activation_ < kMaxSelectorDebugEventsPerActivation) {
                ++selector_debug_events_this_activation_;
                ++debug_selector_emitted_;
                const auto outer_identity = component_outer
                    ? pack_weak_identity(FWeakObjectPtr{component_outer})
                    : 0;
                const auto receiver_identity = context.interaction_receiver
                    ? pack_weak_identity(FWeakObjectPtr{context.interaction_receiver})
                    : 0;
                selector_diagnostic = DeferredSelectorDiagnostic{
                    true, actor_is_drop_item, component_is_interactable,
                    component_outer == selected.actor, target_type_supported,
                    observed_actor_identity, observed_component_identity,
                    outer_identity, receiver_identity, observed_state_value,
                    observed_type_value, selector_debug_events_this_activation_,
                    copy_diagnostic_token(context.receiver_source ? context.receiver_source : "none"),
                };
            } else {
                ++debug_selector_suppressed_;
            }
        }
        if (selector_observation_changed) {
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

        const FWeakObjectPtr actor_weak{selected.actor};
        const FWeakObjectPtr component_weak{selected.component};
        const auto actor_identity = pack_weak_identity(actor_weak);
        const auto component_identity = pack_weak_identity(component_weak);
        const auto component_object_id = weak_object_id(component_weak);
        const auto preflight_decision = automatic_action_.inspect(
            component_object_id, Clock::now(), 0.0);
        if (preflight_decision == dsnap::AutomaticActionDecision::Cooldown) {
            ++action_cooldown_hits_;
            if (retry_no_candidate) *retry_no_candidate = true;
            return reject("action_component_cooldown");
        }
        if (preflight_decision == dsnap::AutomaticActionDecision::CapacityWait) {
            // Ordinary record pressure must not inject an untrackable action
            // or permanently disable pickup. Idle cadence retries on expiry.
            if (retry_no_candidate) *retry_no_candidate = true;
            return reject("action_record_capacity_wait");
        }
        if (preflight_decision == dsnap::AutomaticActionDecision::Pending) {
            return reject("action_state_pending_invariant");
        }
        if (preflight_decision == dsnap::AutomaticActionDecision::FailClosed) {
            return reject("action_state_fail_closed");
        }
        if (preflight_decision != dsnap::AutomaticActionDecision::Ready) {
            return reject("action_candidate_identity_invalid");
        }
        const auto interact_type_value = *interact_type;
        const auto interactable_value = *interactable;
        const char* target_kind = interact_type_value == dsnap::kNormalGatherInteractType
            ? "normal_gather"
            : (interact_type_value == dsnap::kAnimalInteractType ? "animal_fish" : "drop_item");
        ++selector_pairs_;
        validation_scope.finish();

        if (Clock::now() >= window_deadline_) return reject("window_budget_expired");
        StageTimingScope action_resolution_scope{
            timing ? &timing->action_resolution_us : nullptr};
        const auto foreground = foreground_window_state();
        if (!foreground.matched) return reject("game_not_foreground");

        EnhancedActionResolution action_resolution{};
        const char* action_resolution_reason{"unknown"};
        const bool action_resolved = resolve_live_interaction_action(context, &action_resolution,
                                                                     &action_resolution_reason);
        if (!action_resolved) return reject(action_resolution_reason);
        action_resolution_scope.finish();

        // Everything needed after either ProcessEvent boundary is scalarized or
        // weak before the first boundary. No raw gameplay UObject is inspected
        // after the input injection returns.
        const FWeakObjectPtr receiver_weak{context.interaction_receiver};
        const FWeakObjectPtr player_input_weak{action_resolution.player_input};
        const FWeakObjectPtr interaction_action_weak{action_resolution.interaction_action};
        const auto receiver_identity = pack_weak_identity(receiver_weak);
        const auto receiver_address = reinterpret_cast<std::uintptr_t>(context.interaction_receiver);
        const auto player_input_identity = pack_weak_identity(player_input_weak);
        const auto interaction_action_identity = pack_weak_identity(interaction_action_weak);
        const std::string_view receiver_source_snapshot{
            context.receiver_source ? context.receiver_source : "none"};
        const bool mounted_snapshot = context.mounted;
        const std::string binding_mode_snapshot = interaction_binding_mode_;
        const std::string active_binding_keys_snapshot = action_resolution.active_binding_keys;
        const auto mapping_count_snapshot = action_resolution.mapping_count;
        const auto matched_mappings_snapshot = action_resolution.matched_mappings;
        const auto active_mappings_snapshot = action_resolution.active_mappings;
        const auto ignored_mappings_snapshot = action_resolution.ignored_mappings;
        const bool conflicting_actions_snapshot = action_resolution.conflicting_actions;
        const std::string_view action_property_storage_snapshot{
            action_resolution.action_property_storage};
        const auto expected_activation_id = action_activation_id_;
        const auto expected_world_identity = active_world_identity_.load(std::memory_order_acquire);

        StageTimingScope subsystem_scope{timing ? &timing->subsystem_us : nullptr};
        const char* subsystem_reason{"unknown"};
        auto* enhanced_input_subsystem = resolve_enhanced_input_subsystem(
            context.controller, expected_activation_id, expected_world_identity,
            &subsystem_reason);
        if (!enhanced_input_subsystem) return reject(subsystem_reason);
        subsystem_scope.finish();

        if (!automation_enabled_.load(std::memory_order_acquire) || expected_world_identity == 0 ||
            active_world_identity_.load(std::memory_order_acquire) != expected_world_identity) {
            return reject("action_context_changed_before_injection");
        }
        const FWeakObjectPtr subsystem_weak{enhanced_input_subsystem};
        const auto subsystem_identity = pack_weak_identity(subsystem_weak);
        bool weak_probe_faulted{};
        auto* live_actor = weak_get_guarded(actor_weak, &weak_probe_faulted);
        if (weak_probe_faulted) return reject("guarded_runtime_fault");
        auto* live_component = weak_get_guarded(component_weak, &weak_probe_faulted);
        if (weak_probe_faulted) return reject("guarded_runtime_fault");
        auto* live_player_input = weak_get_guarded(player_input_weak, &weak_probe_faulted);
        if (weak_probe_faulted) return reject("guarded_runtime_fault");
        auto* live_interaction_action = weak_get_guarded(interaction_action_weak, &weak_probe_faulted);
        if (weak_probe_faulted) return reject("guarded_runtime_fault");
        auto* live_subsystem = weak_get_guarded(subsystem_weak, &weak_probe_faulted);
        if (weak_probe_faulted) return reject("guarded_runtime_fault");
        if (live_actor != selected.actor || live_component != selected.component ||
            !live_player_input || !live_interaction_action || !live_subsystem) {
            return reject("action_context_changed_before_injection");
        }

        const auto injection_foreground = foreground_window_state();
        if (!injection_foreground.matched) {
            return reject("game_not_foreground_before_injection");
        }
        const auto action_decision = begin_pending_action(
            request_id, actor_weak, component_weak, actor_identity, component_identity,
            receiver_address, interact_type_value);
        if (action_decision == dsnap::AutomaticActionDecision::Pending) {
            return reject("action_state_pending_invariant");
        }
        if (action_decision == dsnap::AutomaticActionDecision::Cooldown) {
            ++action_cooldown_hits_;
            if (retry_no_candidate) *retry_no_candidate = true;
            return reject("action_component_cooldown");
        }
        if (action_decision == dsnap::AutomaticActionDecision::FailClosed) {
            return reject("action_state_fail_closed");
        }
        if (action_decision != dsnap::AutomaticActionDecision::Ready) {
            return reject("action_candidate_identity_invalid");
        }
        const auto invocation_activation_id = action_activation_id_;
        const auto invocation_action_id = pending_action_id_;
        const auto invocation_request_id = pending_request_id_;
        const auto invocation_attempt = automatic_action_.pending_attempt();
        const auto action_record_size_snapshot = automatic_action_.record_size();
        ++enhanced_input_attempts_;
        StageTimingScope injection_scope{timing ? &timing->injection_us : nullptr};
        const char* injection_reason{"unknown"};
        dispatch_observer_armed_.store(true, std::memory_order_release);
        const bool injection_succeeded = inject_live_interaction_action_once(
            live_subsystem, live_interaction_action, &injection_reason);
        if (!injection_succeeded) {
            dispatch_observer_armed_.store(false, std::memory_order_release);
            const bool pending_unchanged = action_activation_id_ == invocation_activation_id &&
                pending_action_id_ == invocation_action_id &&
                pending_request_id_ == invocation_request_id && automatic_action_.pending() &&
                automatic_action_.pending_candidate() == component_object_id;
            if (pending_unchanged) {
                static_cast<void>(automatic_action_.cancel(component_object_id));
                clear_pending_runtime_state();
            }
            ++action_injection_failures_;
            return reject(injection_reason);
        }
        // PROCESS_EVENT_RETURNED_SCALAR_ONLY: no UObject access below this line.
        injection_scope.finish();
        ++enhanced_input_injections_;
        ++action_invocations_;
        const bool pending_unchanged = action_activation_id_ == invocation_activation_id &&
            pending_action_id_ == invocation_action_id &&
            pending_request_id_ == invocation_request_id && automatic_action_.pending() &&
            automatic_action_.pending_candidate() == component_object_id;
        if (pending_unchanged) pending_action_invoked_ = true;
        if (!pending_unchanged) ++action_lifecycle_cancellations_;
        if (reason) *reason = pending_unchanged
            ? "interaction_action_injected"
            : "interaction_action_injected_lifecycle_cancelled";
        // POST_INJECTION_DIAGNOSTIC_ONLY_TRY: gameplay outcome and pending
        // ownership are final before any fallible scalar formatting/enqueue.
        // Never include ProcessEvent, UObject reads, or state transitions here.
        try {
        emit_selected_distance_diagnostic(invocation_activation_id, invocation_action_id,
                                          invocation_request_id, actor_identity,
                                          component_identity);
        logger_.write(dsnap::LogAudience::User, "PICKUP_ACTION_INVOKED",
                      std::format("activation_id={} action_id={} request_id={} actor=0x{:X} component=0x{:X} "
                                  "receiver=0x{:X} receiver_source={} mounted={} interact_type={} "
                                  "target_kind={} resolution_mode={} active_binding_keys={} "
                                  "one_shot=1 in_flight_gate=1 attempt={}/{} confirmation_window_ms={} "
                                  "target_field_access=0 direct_RPC=0 SendInput=0 success_claim=0",
                                  invocation_activation_id, invocation_action_id, invocation_request_id,
                                  actor_identity, component_identity, receiver_identity,
                                  receiver_source_snapshot, mounted_snapshot, interact_type_value, target_kind,
                                  binding_mode_snapshot, active_binding_keys_snapshot,
                                  invocation_attempt, dsnap::kMaximumAutomaticActionAttempts,
                                  dsnap::kActionConfirmationWindow.count()));
        if (configuration_result_.value.debug_logging) {
            logger_.write(dsnap::LogAudience::Debug, "ACTION_TRACE",
                          std::format("activation_id={} action_id={} selector_rva=0x{:X} actor=0x{:X} component=0x{:X} "
                                      "player_input=0x{:X} interaction_action=0x{:X} subsystem=0x{:X} "
                                      "actor_is_drop_item={} interactable={} interact_type={} target_kind={} "
                                      "mapping_count={} matched_mappings={} active_mappings={} ignored_mappings={} "
                                      "action_property_storage={} "
                                      "conflicting_actions={} foreground_first={} foreground_final={} "
                                      "pending=1 action_record_size={}",
                                      invocation_activation_id, invocation_action_id,
                                      selector_capability_.selector_rva,
                                      actor_identity, component_identity,
                                      player_input_identity, interaction_action_identity, subsystem_identity,
                                      actor_is_drop_item, interactable_value, interact_type_value, target_kind,
                                      mapping_count_snapshot, matched_mappings_snapshot,
                                      active_mappings_snapshot, ignored_mappings_snapshot,
                                      action_property_storage_snapshot, conflicting_actions_snapshot, foreground.matched,
                                      injection_foreground.matched, action_record_size_snapshot));
        }
        if (!pending_unchanged) {
            logger_.write(dsnap::LogAudience::User, "PICKUP_ACTION_CANCELLED",
                          std::format("activation_id={} action_id={} request_id={} actor=0x{:X} component=0x{:X} "
                                      "reason=lifecycle_changed_during_injection terminal=1",
                                      invocation_activation_id, invocation_action_id, invocation_request_id,
                                      actor_identity, component_identity));
        }
        } catch (const std::exception&) {
            ++diagnostic_write_failures_;
        }
        return true;
        };

        const bool decision_result = run_pickup_decision();
        // DEFERRED_PICKUP_DIAGNOSTICS_SCALAR_ONLY: both success and ordinary
        // rejection arrive here after the decision. No gameplay object survives
        // in either value-captured snapshot; a diagnostic fault cannot cause a
        // second invocation or change a previously checked window deadline.
        // POST_DECISION_DIAGNOSTIC_ONLY_TRY: include snapshot value-copy
        // allocations as well as formatting/enqueue, never the gameplay lambda.
        try {
        const auto emit_context_diagnostic = [this, request_id, snapshot = context_diagnostic]() {
            if (!snapshot.emit) return;
            logger_.write(dsnap::LogAudience::Debug, "PLAYER_CONTEXT_CHANGED",
                          std::format("request_id={} resolved={} reason={} pawn=0x{:X} "
                                      "interaction_owner=0x{:X} receiver=0x{:X} "
                                      "receiver_source={} mounted={} scalar_only=1 "
                                      "change_only=1 activation_event={}/{} decision_complete=1",
                                      request_id, snapshot.resolved, snapshot.reason.data(),
                                      snapshot.pawn_identity, snapshot.owner_identity,
                                      snapshot.receiver_identity, snapshot.receiver_source.data(),
                                      snapshot.mounted, snapshot.activation_event,
                                      kMaxPlayerContextDebugEventsPerActivation));
        };
        const auto emit_selector_diagnostic = [this, snapshot = selector_diagnostic]() {
            if (!snapshot.emit) return;
            logger_.write(dsnap::LogAudience::Debug, "SELECTOR_PAIR_OBSERVED",
                          std::format("actor=0x{:X} component=0x{:X} component_outer=0x{:X} "
                                      "receiver=0x{:X} receiver_source={} actor_is_drop_item={} "
                                      "component_is_interactable={} relation_matches={} interactable={} "
                                      "interact_type={} target_type_supported={} target_field_access=0 "
                                      "scalar_only=1 change_only=1 activation_event={}/{} decision_complete=1",
                                      snapshot.actor_identity, snapshot.component_identity,
                                      snapshot.outer_identity, snapshot.receiver_identity,
                                      snapshot.receiver_source.data(), snapshot.actor_is_drop_item,
                                      snapshot.component_is_interactable, snapshot.relation_matches,
                                      snapshot.state_value, snapshot.type_value,
                                      snapshot.target_type_supported, snapshot.activation_event,
                                      kMaxSelectorDebugEventsPerActivation));
        };
        emit_context_diagnostic();
        emit_selector_diagnostic();
        } catch (const std::exception&) {
            ++diagnostic_write_failures_;
        }
        return decision_result;
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
            self->dispatch_observer_armed_.store(false, std::memory_order_release);
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

    [[nodiscard]] bool call_native_selector_guarded(UObject* receiver,
                                                    NativeSelectorPair* output,
                                                    bool* faulted) noexcept {
        if (faulted) *faulted = false;
        if (!receiver || !output || !selector_capability_.resolved()) return false;
        const auto called = invoke_selector_address_guarded(selector_capability_.address,
                                                            receiver, output, faulted);
        if (!called && faulted && *faulted) {
            selector_capability_.address = 0;
            selector_capability_.selector_rva = 0;
            selector_capability_.server_address = 0;
            selector_capability_.server_call_site = 0;
            selector_capability_.ui_address = 0;
            selector_capability_.ui_implementation_call_site = 0;
            selector_capability_.ui_call_site = 0;
            selector_capability_.status = "faulted";
            logger_.write(dsnap::LogAudience::User, "SELECTOR_FAULTED",
                          std::format("policy={} status=faulted fail_closed=1 "
                                       "automation_will_disable=1 retry_this_process=0",
                                       kSelectorResolutionPolicy));
        }
        return called;
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

    [[nodiscard]] dsnap::AutomaticActionDecision begin_pending_action(
        std::uint64_t request_id,
        const FWeakObjectPtr& actor,
        const FWeakObjectPtr& component,
        std::uint64_t actor_identity,
        std::uint64_t component_identity,
        std::uintptr_t receiver_address,
        std::uint8_t interact_type) noexcept {
        if (receiver_address == 0) return dsnap::AutomaticActionDecision::InvalidCandidate;
        const auto decision = automatic_action_.begin(weak_object_id(component), Clock::now(), 0.0);
        if (decision != dsnap::AutomaticActionDecision::Ready) return decision;
        pending_actor_ = actor;
        pending_component_ = component;
        pending_action_id_ = next_action_id_.fetch_add(1, std::memory_order_relaxed) + 1;
        pending_request_id_ = request_id;
        pending_actor_identity_ = actor_identity;
        pending_component_identity_ = component_identity;
        pending_interact_type_ = interact_type;
        pending_action_invoked_ = false;
        dispatch_observer_armed_.store(false, std::memory_order_relaxed);
        dispatch_observed_action_id_.store(0, std::memory_order_relaxed);
        pending_receiver_address_.store(receiver_address, std::memory_order_relaxed);
        pending_action_id_visible_.store(pending_action_id_, std::memory_order_release);
        pending_action_visible_.store(true, std::memory_order_release);
        return decision;
    }

    void clear_pending_runtime_state() noexcept {
        dispatch_observer_armed_.store(false, std::memory_order_release);
        pending_action_id_visible_.store(0, std::memory_order_release);
        pending_receiver_address_.store(0, std::memory_order_relaxed);
        dispatch_observed_action_id_.store(0, std::memory_order_relaxed);
        pending_action_visible_.store(false, std::memory_order_release);
        pending_actor_.Reset();
        pending_component_.Reset();
        pending_action_id_ = 0;
        pending_request_id_ = 0;
        pending_actor_identity_ = 0;
        pending_component_identity_ = 0;
        pending_interact_type_ = 0;
        pending_action_invoked_ = false;
    }

    void reset_debug_activation_state() noexcept {
        player_context_observed_ = false;
        deferred_reason_observed_ = false;
        last_context_resolved_ = false;
        last_context_mounted_ = false;
        last_pawn_identity_ = 0;
        last_receiver_identity_ = 0;
        last_player_context_reason_ = "unset";
        last_receiver_source_ = "unset";
        last_deferred_reason_ = "unset";
        last_observed_actor_identity_ = 0;
        last_observed_component_identity_ = 0;
        last_observed_interact_type_ = -2;
        last_observed_interactable_ = -2;
        last_observed_supported_ = false;
        context_debug_events_this_activation_ = 0;
        selector_debug_events_this_activation_ = 0;
        deferred_debug_events_this_activation_ = 0;
    }

    void reset_action_activation(const char* reason, bool record_terminal) noexcept {
        if (automatic_action_.pending()) {
            if (record_terminal && pending_action_invoked_) {
                ++action_lifecycle_cancellations_;
                logger_.write(dsnap::LogAudience::User, "PICKUP_ACTION_CANCELLED",
                              std::format("activation_id={} action_id={} request_id={} actor=0x{:X} component=0x{:X} "
                                          "reason={} terminal=1",
                                          action_activation_id_, pending_action_id_, pending_request_id_, pending_actor_identity_,
                                          pending_component_identity_, reason));
            }
        }
        automatic_action_.reset_activation();
        active_interaction_owner_identity_ = 0;
        action_record_size_visible_.store(0, std::memory_order_release);
        clear_pending_runtime_state();
        reset_debug_activation_state();
    }

    void disable_for_action_fault(const char* reason) noexcept {
        const auto candidate = automatic_action_.pending_candidate();
        if (candidate.valid()) {
            static_cast<void>(automatic_action_.cancel(candidate));
            if (pending_action_invoked_) {
                ++action_lifecycle_cancellations_;
                logger_.write(dsnap::LogAudience::User, "PICKUP_ACTION_CANCELLED",
                              std::format("activation_id={} action_id={} request_id={} actor=0x{:X} component=0x{:X} "
                                          "reason={} terminal=1",
                                          action_activation_id_, pending_action_id_, pending_request_id_, pending_actor_identity_,
                                          pending_component_identity_, reason));
            }
        }
        clear_pending_runtime_state();
        automation_enabled_.store(false, std::memory_order_release);
        active_world_identity_.store(0, std::memory_order_release);
        next_auto_scan_due_ = {};
        ++automatic_disables_;
        ++action_state_faults_;
        status_toast_renderer_.notify(dsnap::StatusToastKind::Disabled,
                                      Clock::now());
        logger_.write(dsnap::LogAudience::User, "AUTOMATION_DISABLED",
                      std::format("reason={} action_state_fail_closed=1 retry_hotkey={}",
                                  reason, configuration_result_.value.toggle_hotkey));
    }

    enum class PendingComponentProbe : std::uint8_t {
        Active,
        Inactive,
        Invalidated,
        Fault,
    };

    [[nodiscard]] PendingComponentProbe probe_pending_component_guarded(UObject* actor) noexcept {
#if defined(_MSC_VER)
        __try {
#endif
            if (!actor) return PendingComponentProbe::Invalidated;
            bool component_faulted{};
            auto* component = weak_get_guarded(pending_component_, &component_faulted);
            if (component_faulted) return PendingComponentProbe::Fault;
            if (!component) return PendingComponentProbe::Invalidated;
            if (pack_weak_identity(FWeakObjectPtr{component}) != pending_component_identity_ ||
                !component->IsA(interactable_class_) || component->GetOuterPrivate() != actor) {
                return PendingComponentProbe::Fault;
            }
            auto* world = actor->GetWorld();
            if (!world || component->GetWorld() != world ||
                pack_weak_identity(FWeakObjectPtr{reinterpret_cast<UObject*>(world)}) !=
                    active_world_identity_.load(std::memory_order_acquire)) {
                return PendingComponentProbe::Fault;
            }
            auto* interactable = component->GetValuePtrByPropertyName<std::uint8_t>(
                STR("InteractableValue"));
            auto* interact_type = component->GetValuePtrByPropertyName<std::uint8_t>(
                STR("InteractTypeValue"));
            if (!interactable || !interact_type || *interact_type != pending_interact_type_) {
                return PendingComponentProbe::Fault;
            }
            return *interactable == kRequiredInteractableValue
                ? PendingComponentProbe::Active
                : PendingComponentProbe::Inactive;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return PendingComponentProbe::Fault;
        }
#endif
    }

    void poll_pending_action(Clock::time_point now) noexcept {
        if (!automatic_action_.pending()) return;
        const auto pending_candidate = automatic_action_.pending_candidate();
        const auto pending_attempt = automatic_action_.pending_attempt();
        const auto observed_action_id =
            dispatch_observed_action_id_.exchange(0, std::memory_order_acq_rel);
        // Probe real target evidence before the dispatch branch discards the
        // weak handles. A dispatch alone is not a successful pickup.
        bool actor_faulted{};
        auto* actor = weak_get_guarded(pending_actor_, &actor_faulted);
        if (actor_faulted) {
            disable_for_action_fault("pending_weak_probe_fault");
            return;
        }
        const auto component_probe = actor
            ? probe_pending_component_guarded(actor)
            : PendingComponentProbe::Invalidated;
        if (component_probe == PendingComponentProbe::Fault) {
            disable_for_action_fault("pending_component_probe_fault");
            return;
        }
        if (!actor || component_probe == PendingComponentProbe::Invalidated ||
            component_probe == PendingComponentProbe::Inactive) {
            const auto confirmed = automatic_action_.confirm(pending_candidate);
            if (!confirmed) {
                disable_for_action_fault("pending_confirm_identity_mismatch");
                return;
            }
            ++confirmations_;
            action_record_size_visible_.store(
                automatic_action_.record_size(), std::memory_order_release);
            const bool matching_dispatch = observed_action_id == pending_action_id_;
            if (matching_dispatch) {
                ++action_dispatch_observations_;
                // Preserve the already elapsed dispatch-path due. Proving
                // pickup in this poll must not add another wait.
            } else {
                if (observed_action_id != 0) ++dispatch_marker_mismatches_;
                next_auto_scan_due_ = now + kPostPickupCooldown;
            }
            const auto next_scan_delay_ms =
                next_auto_scan_due_ == Clock::time_point{} || now >= next_auto_scan_due_
                ? std::int64_t{0}
                : std::chrono::duration_cast<std::chrono::milliseconds>(
                      next_auto_scan_due_ - now).count();
            logger_.write(dsnap::LogAudience::User, "PICKUP_CONFIRMED",
                          std::format("activation_id={} action_id={} request_id={} signal={} "
                                      "actor=0x{:X} component=0x{:X} interact_type={} terminal=1 "
                                      "next_scan_ms={} dispatch_observed={} attempt_history_cleared=1",
                                      action_activation_id_, pending_action_id_, pending_request_id_,
                                      !actor ? "exact_actor_weak_identity_invalidated" :
                                          (component_probe == PendingComponentProbe::Invalidated
                                              ? "exact_component_weak_identity_invalidated"
                                              : "exact_component_interactable_state_changed"),
                                      pending_actor_identity_,
                                      pending_component_identity_, pending_interact_type_,
                                      next_scan_delay_ms, matching_dispatch));
            clear_pending_runtime_state();
            return;
        }
        if (observed_action_id != 0) {
            if (observed_action_id == pending_action_id_) {
                const auto dispatched = automatic_action_.observe_dispatch(pending_candidate, now);
                if (!dispatched) {
                    disable_for_action_fault("dispatch_observer_identity_mismatch");
                    return;
                }
                ++action_dispatch_observations_;
                action_record_size_visible_.store(
                    automatic_action_.record_size(), std::memory_order_release);
                const auto next_scan_delay_ms =
                    next_auto_scan_due_ == Clock::time_point{} || now >= next_auto_scan_due_
                    ? std::int64_t{0}
                    : std::chrono::duration_cast<std::chrono::milliseconds>(
                          next_auto_scan_due_ - now).count();
                if (configuration_result_.value.debug_logging) {
                    logger_.write(dsnap::LogAudience::Debug, "PICKUP_DISPATCH_OBSERVED",
                                  std::format(
                                      "activation_id={} action_id={} request_id={} "
                                      "actor=0x{:X} component=0x{:X} receiver=0x{:X} "
                                      "interact_type={} signal=game_Server_RunInteractV2_post "
                                      "target_match_unproven=1 pickup_success_claim=0 "
                                      "terminal_for_injection=1 same_component_reentry_ms={} "
                                      "elapsed_pulse_satisfies_post_pickup=1 next_scan_ms={} "
                                      "attempt={}/{} retry_counter_preserved=1 recovery_backoff={}",
                                      action_activation_id_, pending_action_id_, pending_request_id_,
                                      pending_actor_identity_, pending_component_identity_,
                                      pending_receiver_address_.load(std::memory_order_acquire),
                                      pending_interact_type_,
                                      (pending_attempt >= dsnap::kMaximumAutomaticActionAttempts
                                          ? dsnap::kAutomaticActionFailureBackoff
                                          : dsnap::kAutomaticActionDispatchReentryDelay).count(),
                                      next_scan_delay_ms, pending_attempt,
                                      dsnap::kMaximumAutomaticActionAttempts,
                                      pending_attempt >= dsnap::kMaximumAutomaticActionAttempts));
                }
                clear_pending_runtime_state();
                if (automatic_action_.fail_closed()) {
                    disable_for_action_fault("action_record_capacity_exhausted");
                }
                return;
            }
            ++dispatch_marker_mismatches_;
            if (configuration_result_.value.debug_logging) {
                logger_.write(dsnap::LogAudience::Debug, "DISPATCH_MARKER_IGNORED",
                              std::format("observed_action_id={} pending_action_id={} "
                                          "scheduler_effect=none",
                                          observed_action_id, pending_action_id_));
            }
        }
        const auto expired = automatic_action_.expire(now);
        if (!expired) return;
        ++confirmation_timeouts_;
        const bool retry_scheduled = !automatic_action_.fail_closed() &&
            expired->attempt_ordinal < dsnap::kMaximumAutomaticActionAttempts;
        action_record_size_visible_.store(
            automatic_action_.record_size(), std::memory_order_release);
        logger_.write(dsnap::LogAudience::User, "PICKUP_UNCONFIRMED",
                      std::format("activation_id={} action_id={} request_id={} reason=exact_component_still_interactable "
                                  "actor=0x{:X} component=0x{:X} interact_type={} elapsed_ms={} "
                                  "attempt={}/{} cycle_terminal={} automatic_retry={} retry_after_ms={} "
                                  "recovery_after_ms={} activation_quarantine=0 action_record_size={}",
                                  action_activation_id_, pending_action_id_, pending_request_id_, pending_actor_identity_,
                                  pending_component_identity_, pending_interact_type_,
                                  std::chrono::duration_cast<std::chrono::milliseconds>(
                                      now - expired->invoked_at).count(),
                                  pending_attempt, dsnap::kMaximumAutomaticActionAttempts,
                                  !retry_scheduled, retry_scheduled,
                                  retry_scheduled ? dsnap::kAutomaticActionRetryDelay.count() : 0,
                                   retry_scheduled ? 0 : dsnap::kAutomaticActionFailureBackoff.count(),
                                   automatic_action_.record_size()));
        clear_pending_runtime_state();
        // The retry/backoff belongs only to the exact Component record.  The
        // scan pulse that observed this timeout has already satisfied the
        // active cadence, so leave the elapsed due time intact: a different
        // selector result may proceed on this same EngineTick.  If the game
        // presents the cooled Component again, AutomaticActionState rejects it
        // before action resolution and the normal idle cadence schedules the
        // next scan.
        if (automatic_action_.fail_closed()) {
            disable_for_action_fault("action_record_capacity_exhausted");
        }
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
        status_toast_renderer_.release_for_travel();
        const auto session_generation = session_events_.reset();
        const bool was_enabled = automation_enabled_.exchange(false, std::memory_order_acq_rel);
        cancel_window(source);
        reset_action_activation(source, true);
        resolved_interaction_key_.clear();
        interaction_binding_mode_ = "unresolved";
        ready_world_identity_ = 0;
        active_world_identity_.store(0, std::memory_order_release);
        const auto now = Clock::now();
        world_settle_until_ = now + kWorldSettleDelay;
        next_readiness_probe_due_ = world_settle_until_;
        next_auto_scan_due_ = {};
        if (was_enabled) ++automatic_disables_;
        ++world_resets_;
        logger_.write(dsnap::LogAudience::User, "WORLD_RESET",
                      std::format("reason={} selector_observation_state=cleared "
                                  "selector_capability=process_lifetime confirmation=cleared "
                                  "interaction_binding=cleared automation_disabled=1 was_enabled={} "
                                  "reenable_required=1 settle_ms={} session_generation={} "
                                  "cross_world_object_cache=0",
                                  source, was_enabled, kWorldSettleDelay.count(), session_generation));
    }

    void unregister_callbacks() noexcept {
        // A deferred startup attempt runs inside this callback. If a later
        // registration gate fails, quiesce every partially registered runtime
        // callback but leave the current EngineTickPost callback as a permanent
        // fail-closed no-op until ordinary Mod shutdown unregisters it.
        if (!runtime_initialization_retry_in_progress_ &&
            engine_tick_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(engine_tick_callback_id_);
            engine_tick_callback_id_ = Hook::ERROR_ID;
        }
        dispatch_observer_armed_.store(false, std::memory_order_release);
        if (server_run_interact_hook_registered_ && server_run_interact_function_) {
            try {
                UObjectGlobals::UnregisterHook(server_run_interact_function_,
                                               server_run_interact_hook_);
            } catch (...) {
            }
            server_run_interact_hook_ = {};
            server_run_interact_hook_registered_ = false;
        }
        if (begin_play_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(begin_play_callback_id_);
            begin_play_callback_id_ = Hook::ERROR_ID;
        }
        if (world_reset_callback_id_ != Hook::ERROR_ID) {
            Hook::UnregisterCallback(world_reset_callback_id_);
            world_reset_callback_id_ = Hook::ERROR_ID;
        }
    }

    inline static std::atomic<NativeAutoPickup*> instance_{};
    inline static std::atomic<std::uint64_t> next_generation_{};

    dsnap::CallbackGenerationGate callback_gate_;
    dsnap::SessionToggleEventGate session_events_{};
    dsnap::ConfigurationResult configuration_result_{};
    std::string resolved_interaction_key_{};
    std::string interaction_binding_mode_{"unresolved"};
    dsnap::AsyncLogger logger_;
    dsnap::ue4ss::StatusToastRenderer status_toast_renderer_{};
    std::uint64_t status_toast_faults_reported_{};
    dsnap::FingerprintResult fingerprint_result_{};
    SelectorCapability selector_capability_{};
    ProcessShutdownProbe shutdown_probe_{};

    std::atomic<bool> shutting_down_{};
    std::atomic<bool> build_trusted_{};
    std::atomic<bool> automation_enabled_{};
    std::atomic<bool> window_active_{};
    std::atomic<std::uint64_t> next_tick_sequence_{};
    std::atomic<std::uint64_t> next_request_id_{};
    std::atomic<std::uint64_t> next_action_id_{};
    bool fingerprint_applied_{};
    RuntimeInitializationState runtime_initialization_state_{RuntimeInitializationState::Pending};
    std::uint32_t runtime_initialization_attempts_{};
    bool runtime_initialization_retry_in_progress_{};
    std::atomic<bool> pending_action_visible_{};
    dsnap::AtomicPhysicalKeyEdge toggle_key_edge_{};
    std::atomic<std::size_t> action_record_size_visible_{};
    std::uint8_t toggle_virtual_key_{};

    UClass* drop_item_class_{};
    UClass* interactable_class_{};
    UClass* sphere_component_class_{};
    UClass* enhanced_player_input_class_{};
    UClass* input_action_class_{};
    UScriptStruct* enhanced_action_mapping_struct_{};
    UScriptStruct* key_struct_{};
    UClass* enhanced_input_subsystem_class_{};
    UClass* subsystem_library_class_{};
    UObject* subsystem_library_cdo_{};
    UFunction* get_local_player_subsystem_function_{};
    UFunction* inject_input_vector_function_{};
    UFunction* server_run_interact_function_{};
    UFunction* set_sphere_radius_function_{};
    FObjectProperty* drop_item_overlap_property_{};
    FFloatProperty* sphere_radius_instance_property_{};
    FFloatProperty* set_sphere_radius_value_property_{};
    FBoolProperty* set_sphere_radius_update_property_{};
    Hook::GlobalCallbackId engine_tick_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId world_reset_callback_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId begin_play_callback_id_{Hook::ERROR_ID};
    std::pair<int, int> server_run_interact_hook_{};
    bool server_run_interact_hook_registered_{};
    std::uint32_t drop_item_range_multiplier_{1};
    bool drop_item_range_contract_ready_{};
    std::atomic<std::uint64_t> drop_item_range_applied_{};
    std::atomic<std::uint64_t> drop_item_range_failures_{};

    dsnap::AutomaticActionState automatic_action_{};
    FWeakObjectPtr pending_actor_{};
    FWeakObjectPtr pending_component_{};
    std::uint64_t pending_action_id_{};
    std::uint64_t pending_request_id_{};
    std::uint64_t pending_actor_identity_{};
    std::uint64_t pending_component_identity_{};
    std::uint8_t pending_interact_type_{};
    bool pending_action_invoked_{};
    std::atomic<bool> dispatch_observer_armed_{};
    std::atomic<std::uintptr_t> pending_receiver_address_{};
    std::atomic<std::uint64_t> pending_action_id_visible_{};
    std::atomic<std::uint64_t> dispatch_observed_action_id_{};
    bool player_context_observed_{};
    bool deferred_reason_observed_{};
    bool last_context_resolved_{};
    bool last_context_mounted_{};
    std::uint64_t last_pawn_identity_{};
    std::uint64_t last_receiver_identity_{};
    std::string_view last_player_context_reason_{"unset"};
    std::string_view last_receiver_source_{"unset"};
    std::string_view last_deferred_reason_{"unset"};
    std::uint64_t ready_world_identity_{};
    std::atomic<std::uint64_t> active_world_identity_{};
    std::uint64_t active_interaction_owner_identity_{};
    std::uint64_t last_observed_actor_identity_{};
    std::uint64_t last_observed_component_identity_{};
    int last_observed_interact_type_{-2};
    int last_observed_interactable_{-2};
    bool last_observed_supported_{};
    std::uint32_t context_debug_events_this_activation_{};
    std::uint32_t selector_debug_events_this_activation_{};
    std::uint32_t deferred_debug_events_this_activation_{};

    Clock::time_point last_pulse_at_{};
    Clock::time_point runtime_initialization_started_at_{};
    Clock::time_point runtime_initialization_deadline_{};
    Clock::time_point next_runtime_initialization_attempt_{};
    std::atomic_flag engine_tick_active_ = ATOMIC_FLAG_INIT;
    Clock::time_point next_pulse_due_{};
    Clock::time_point next_perf_log_{};
    Clock::time_point last_slow_tick_log_{};
    Clock::time_point next_auto_scan_due_{};
    Clock::time_point next_readiness_probe_due_{};
    Clock::time_point world_settle_until_{};
    Clock::time_point window_started_at_{};
    Clock::time_point window_deadline_{};

    std::uint64_t active_request_id_{};
    std::uint64_t action_activation_id_{};
    std::uint32_t window_samples_{};
    std::uint32_t window_selector_calls_{};
    std::uint32_t window_no_candidate_count_{};

    std::atomic<std::uint64_t> key_events_received_{};
    std::atomic<std::uint64_t> key_events_coalesced_{};
    std::atomic<std::uint64_t> key_events_rejected_{};
    std::atomic<std::uint64_t> key_repeat_rejections_{};
    std::atomic<std::uint64_t> key_release_rearms_{};
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
    std::atomic<std::uint64_t> action_invocations_{};
    std::atomic<std::uint64_t> action_injection_failures_{};
    std::atomic<std::uint64_t> action_lifecycle_cancellations_{};
    std::atomic<std::uint64_t> action_cooldown_hits_{};
    std::atomic<std::uint64_t> pending_scan_suppressions_{};
    std::atomic<std::uint64_t> action_dispatch_observations_{};
    std::atomic<std::uint64_t> dispatch_marker_mismatches_{};
    std::atomic<std::uint64_t> action_state_faults_{};
    std::atomic<std::uint64_t> action_failures_{};
    std::atomic<std::uint64_t> transient_failures_{};
    std::atomic<std::uint64_t> confirmations_{};
    std::atomic<std::uint64_t> confirmation_timeouts_{};
    std::atomic<std::uint64_t> selector_faults_{};
    std::atomic<std::uint64_t> world_resets_{};
    std::atomic<std::uint64_t> slow_scan_count_{};
    std::atomic<std::uint64_t> debug_context_emitted_{};
    std::atomic<std::uint64_t> debug_context_suppressed_{};
    std::atomic<std::uint64_t> debug_selector_emitted_{};
    std::atomic<std::uint64_t> debug_selector_suppressed_{};
    std::atomic<std::uint64_t> debug_deferred_emitted_{};
    std::atomic<std::uint64_t> debug_deferred_suppressed_{};
    std::atomic<std::uint64_t> diagnostic_write_failures_{};
    std::atomic<std::uint64_t> logger_messages_dequeued_{};
    AtomicTiming pulse_gap_timing_{};
    AtomicTiming pulse_work_timing_{};
    AtomicTiming scan_timing_{};
    AtomicTiming context_timing_{};
    AtomicTiming selector_timing_{};
    AtomicTiming validation_timing_{};
    AtomicTiming action_resolution_timing_{};
    AtomicTiming subsystem_timing_{};
    AtomicTiming injection_timing_{};
    AtomicTiming logger_flush_timing_{};
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
