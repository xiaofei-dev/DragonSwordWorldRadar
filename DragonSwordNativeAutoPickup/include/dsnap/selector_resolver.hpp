#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace dsnap {

struct ExecutableTextView {
    std::uintptr_t address{};
    std::span<const std::uint8_t> bytes{};

    [[nodiscard]] bool contains(std::uintptr_t candidate,
                                std::size_t size = 1) const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> at(std::uintptr_t candidate,
                                                   std::size_t size) const noexcept;
};

enum class SelectorResolverStatus : std::uint8_t {
    Resolved,
    InvalidInput,
    AddressOutsideText,
    DirectCallNotFound,
    DirectCallTargetInvalid,
    DirectCallCapacityExceeded,
    TerminalCallNotFound,
    TerminalCallAmbiguous,
    DispatchNotFound,
    DispatchAmbiguous,
    JumpTargetInvalid,
    JumpLimitExceeded,
    PropertyLayoutInvalid,
    SelectorNotFound,
    SelectorAmbiguous,
};

struct VirtualDispatchResult {
    SelectorResolverStatus status{SelectorResolverStatus::InvalidInput};
    std::uint32_t slot_offset{};
    std::size_t candidates{};

    [[nodiscard]] bool resolved() const noexcept {
        return status == SelectorResolverStatus::Resolved;
    }
};

struct JumpFollowResult {
    SelectorResolverStatus status{SelectorResolverStatus::InvalidInput};
    std::uintptr_t address{};
    std::uint32_t jumps{};

    [[nodiscard]] bool resolved() const noexcept {
        return status == SelectorResolverStatus::Resolved;
    }
};

struct SelectorCallResult {
    SelectorResolverStatus status{SelectorResolverStatus::InvalidInput};
    std::uintptr_t selector_address{};
    std::uintptr_t call_site{};
    std::size_t candidates{};

    [[nodiscard]] bool resolved() const noexcept {
        return status == SelectorResolverStatus::Resolved;
    }
};

struct DirectRel32Call {
    std::uintptr_t call_site{};
    std::uintptr_t target{};
};

inline constexpr std::size_t kDirectRel32CallCapacity = 16;

struct DirectRel32CallCollectionResult {
    SelectorResolverStatus status{SelectorResolverStatus::InvalidInput};
    std::array<DirectRel32Call, kDirectRel32CallCapacity> calls{};
    std::size_t count{};

    [[nodiscard]] bool resolved() const noexcept {
        return status == SelectorResolverStatus::Resolved;
    }
};

struct TerminalRel32CallResult {
    SelectorResolverStatus status{SelectorResolverStatus::InvalidInput};
    DirectRel32Call call{};
    std::size_t candidates{};

    [[nodiscard]] bool resolved() const noexcept {
        return status == SelectorResolverStatus::Resolved;
    }
};

[[nodiscard]] DirectRel32CallCollectionResult collect_direct_rel32_calls(
    const ExecutableTextView& text,
    std::uintptr_t implementation_entry,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit = 96) noexcept;

[[nodiscard]] TerminalRel32CallResult select_unique_terminal_rel32_call(
    const ExecutableTextView& text,
    std::uintptr_t implementation_entry,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit = 96) noexcept;

[[nodiscard]] VirtualDispatchResult resolve_virtual_dispatch_slot(
    const ExecutableTextView& text,
    std::uintptr_t reflected_exec_thunk,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit = 96) noexcept;

[[nodiscard]] JumpFollowResult follow_direct_jump_chain(
    const ExecutableTextView& text,
    std::uintptr_t entry,
    std::uint32_t max_jumps = 4) noexcept;

[[nodiscard]] SelectorCallResult resolve_selector_call(
    const ExecutableTextView& text,
    std::uintptr_t server_run_interact,
    std::span<const std::uintptr_t> instruction_addresses,
    std::int32_t target_object_offset,
    std::int32_t target_component_offset,
    std::size_t scan_limit = 0x600) noexcept;

[[nodiscard]] SelectorCallResult resolve_ui_selector_call(
    const ExecutableTextView& text,
    std::uintptr_t set_interact_ui,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit = 0x600) noexcept;

[[nodiscard]] std::string_view selector_resolver_status_name(
    SelectorResolverStatus status) noexcept;

} // namespace dsnap
