#include <dsnap/selector_resolver.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <optional>

namespace dsnap {

bool ExecutableTextView::contains(std::uintptr_t candidate, std::size_t size) const noexcept {
    if (address == 0 || bytes.empty() || candidate < address) return false;
    const auto offset = candidate - address;
    return offset <= bytes.size() && size <= bytes.size() - static_cast<std::size_t>(offset);
}

std::span<const std::uint8_t> ExecutableTextView::at(std::uintptr_t candidate,
                                                     std::size_t size) const noexcept {
    if (!contains(candidate, size)) return {};
    return bytes.subspan(static_cast<std::size_t>(candidate - address), size);
}

namespace {

template <typename ValueType>
[[nodiscard]] std::optional<ValueType> read_value(const ExecutableTextView& text,
                                                   std::uintptr_t address) noexcept {
    const auto source = text.at(address, sizeof(ValueType));
    if (source.size() != sizeof(ValueType)) return std::nullopt;
    ValueType value{};
    std::memcpy(&value, source.data(), sizeof(value));
    return value;
}

[[nodiscard]] std::optional<std::uintptr_t> add_signed(std::uintptr_t base,
                                                       std::int64_t displacement) noexcept {
    if (displacement >= 0) {
        const auto amount = static_cast<std::uint64_t>(displacement);
        if (amount > std::numeric_limits<std::uintptr_t>::max() - base) return std::nullopt;
        return base + static_cast<std::uintptr_t>(amount);
    }
    const auto amount = static_cast<std::uint64_t>(-(displacement + 1)) + 1;
    if (amount > base) return std::nullopt;
    return base - static_cast<std::uintptr_t>(amount);
}

[[nodiscard]] std::optional<std::uintptr_t> relative_target(
    const ExecutableTextView& text,
    std::uintptr_t instruction,
    std::size_t instruction_size,
    std::size_t displacement_offset,
    std::size_t displacement_size) noexcept {
    if (displacement_size == sizeof(std::int32_t)) {
        const auto displacement = read_value<std::int32_t>(text, instruction + displacement_offset);
        if (!displacement) return std::nullopt;
        return add_signed(instruction + instruction_size, *displacement);
    }
    if (displacement_size == sizeof(std::int8_t)) {
        const auto displacement = read_value<std::int8_t>(text, instruction + displacement_offset);
        if (!displacement) return std::nullopt;
        return add_signed(instruction + instruction_size, *displacement);
    }
    return std::nullopt;
}

[[nodiscard]] bool valid_slot(std::int64_t value) noexcept {
    return value >= 0 && value <= 0x4000 && value % static_cast<std::int64_t>(sizeof(void*)) == 0;
}

[[nodiscard]] std::optional<std::uint32_t> dispatch_slot_at(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) noexcept {
    if (offset + 3 > bytes.size() || bytes[offset] != 0x48 || bytes[offset + 1] != 0x8B ||
        bytes[offset + 2] != 0x01) {
        return std::nullopt;
    }
    auto cursor = offset + 3;
    if (cursor < bytes.size() && bytes[cursor] == 0x48) ++cursor;
    if (cursor + 3 > bytes.size() || bytes[cursor] != 0xFF) return std::nullopt;
    if (bytes[cursor + 1] == 0xA0) {
        if (cursor + 6 > bytes.size()) return std::nullopt;
        std::int32_t displacement{};
        std::memcpy(&displacement, bytes.data() + cursor + 2, sizeof(displacement));
        if (!valid_slot(displacement)) return std::nullopt;
        return static_cast<std::uint32_t>(displacement);
    }
    if (bytes[cursor + 1] == 0x60) {
        const auto displacement = static_cast<std::int8_t>(bytes[cursor + 2]);
        if (!valid_slot(displacement)) return std::nullopt;
        return static_cast<std::uint32_t>(displacement);
    }
    return std::nullopt;
}

[[nodiscard]] bool register_saved_from_rcx(std::span<const std::uint8_t> bytes,
                                           std::uintptr_t scan_address,
                                           std::span<const std::uintptr_t> addresses,
                                           std::uint8_t expected_register) noexcept {
    if (expected_register == 1 || expected_register == 4 || expected_register == 12) return false;
    for (const auto instruction_address : addresses) {
        if (instruction_address < scan_address ||
            instruction_address >= scan_address + bytes.size()) {
            continue;
        }
        const auto offset = static_cast<std::size_t>(instruction_address - scan_address);
        if (offset + 3 > bytes.size()) continue;
        const auto rex = bytes[offset];
        if ((rex & 0xF8U) != 0x48U) continue;
        const auto opcode = bytes[offset + 1];
        const auto modrm = bytes[offset + 2];
        if ((modrm & 0xC0U) != 0xC0U) continue;
        const auto rex_r = static_cast<std::uint8_t>((rex & 0x04U) ? 8U : 0U);
        const auto rex_b = static_cast<std::uint8_t>((rex & 0x01U) ? 8U : 0U);
        const auto reg = static_cast<std::uint8_t>(((modrm >> 3U) & 7U) + rex_r);
        const auto rm = static_cast<std::uint8_t>((modrm & 7U) + rex_b);
        if ((opcode == 0x8B && reg == expected_register && rm == 1) ||
            (opcode == 0x89 && reg == 1 && rm == expected_register)) {
            return true;
        }
    }
    return false;
}

struct CallSetup {
    std::uint8_t receiver_register{};
    std::int32_t pair_stack_offset{};
    std::size_t setup_start{};
};

struct RegisterMove {
    std::uint8_t destination{};
    std::uint8_t source{};
};

struct UiCallSetup {
    std::uint8_t receiver_register{};
    std::uint8_t third_argument_register{};
    std::int32_t pair_stack_offset{};
    std::size_t setup_start{};
};

[[nodiscard]] bool instruction_addresses_valid(
    const ExecutableTextView& text,
    std::uintptr_t start,
    std::size_t scan_size,
    std::span<const std::uintptr_t> addresses) noexcept {
    if (addresses.empty() || addresses.front() != start || !text.contains(start) || scan_size == 0) {
        return false;
    }
    const auto start_offset = start - text.address;
    const auto available = text.bytes.size() - static_cast<std::size_t>(start_offset);
    const auto bounded_size = std::min(scan_size, available);
    std::uintptr_t previous{};
    for (const auto address : addresses) {
        if (!text.contains(address) || address < start) return false;
        const auto offset = static_cast<std::size_t>(address - start);
        if (offset >= bounded_size || (previous != 0 && address <= previous)) return false;
        previous = address;
    }
    return true;
}

[[nodiscard]] bool is_instruction_boundary(
    std::span<const std::uintptr_t> addresses,
    std::uintptr_t address) noexcept {
    return std::binary_search(addresses.begin(), addresses.end(), address);
}

[[nodiscard]] std::optional<CallSetup> call_setup_before(std::span<const std::uint8_t> bytes,
                                                         std::uintptr_t scan_address,
                                                         std::size_t call_offset,
                                                         std::span<const std::uintptr_t> addresses) noexcept {
    if (call_offset < 11 || call_offset + 5 > bytes.size()) return std::nullopt;
    const auto mov_offset = call_offset - 3;
    if (!is_instruction_boundary(addresses, scan_address + mov_offset)) return std::nullopt;
    const auto rex = bytes[mov_offset];
    const auto modrm = bytes[mov_offset + 2];
    if ((rex != 0x48 && rex != 0x49) || bytes[mov_offset + 1] != 0x8B ||
        (modrm & 0xC0U) != 0xC0U || ((modrm >> 3U) & 7U) != 1U) {
        return std::nullopt;
    }
    const auto receiver_register = static_cast<std::uint8_t>((modrm & 7U) + ((rex & 1U) ? 8U : 0U));

    std::size_t lea_offset{};
    std::int32_t pair_offset{};
    if (mov_offset >= 5 && bytes[mov_offset - 5] == 0x48 &&
        bytes[mov_offset - 4] == 0x8D && bytes[mov_offset - 3] == 0x54 &&
        bytes[mov_offset - 2] == 0x24) {
        lea_offset = mov_offset - 5;
        pair_offset = static_cast<std::int8_t>(bytes[mov_offset - 1]);
    } else if (mov_offset >= 8 && bytes[mov_offset - 8] == 0x48 &&
               bytes[mov_offset - 7] == 0x8D && bytes[mov_offset - 6] == 0x94 &&
               bytes[mov_offset - 5] == 0x24) {
        lea_offset = mov_offset - 8;
        std::memcpy(&pair_offset, bytes.data() + mov_offset - 4, sizeof(pair_offset));
    } else {
        return std::nullopt;
    }
    if (!is_instruction_boundary(addresses, scan_address + lea_offset) ||
        !is_instruction_boundary(addresses, scan_address + lea_offset - 3)) {
        return std::nullopt;
    }
    if (pair_offset < 0 || pair_offset > 0x400 || pair_offset % 8 != 0) return std::nullopt;
    if (lea_offset < 3 || bytes[lea_offset - 3] != 0x45 ||
        (bytes[lea_offset - 2] != 0x31 && bytes[lea_offset - 2] != 0x33) ||
        bytes[lea_offset - 1] != 0xC0) {
        return std::nullopt;
    }
    return CallSetup{receiver_register, pair_offset, lea_offset - 3};
}

[[nodiscard]] std::optional<RegisterMove> register_move_at(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) noexcept {
    if (offset + 3 > bytes.size()) return std::nullopt;
    const auto rex = bytes[offset];
    if ((rex & 0xF8U) != 0x48U) return std::nullopt;
    const auto opcode = bytes[offset + 1];
    if (opcode != 0x8B && opcode != 0x89) return std::nullopt;
    const auto modrm = bytes[offset + 2];
    if ((modrm & 0xC0U) != 0xC0U) return std::nullopt;

    const auto rex_r = static_cast<std::uint8_t>((rex & 0x04U) ? 8U : 0U);
    const auto rex_b = static_cast<std::uint8_t>((rex & 0x01U) ? 8U : 0U);
    const auto reg = static_cast<std::uint8_t>(((modrm >> 3U) & 7U) + rex_r);
    const auto rm = static_cast<std::uint8_t>((modrm & 7U) + rex_b);
    if (opcode == 0x8B) return RegisterMove{reg, rm};
    return RegisterMove{rm, reg};
}

[[nodiscard]] std::optional<UiCallSetup> ui_call_setup_before(
    std::span<const std::uint8_t> bytes,
    std::uintptr_t scan_address,
    std::size_t call_offset,
    std::span<const std::uintptr_t> addresses) noexcept {
    if (call_offset < 11 || call_offset + 5 > bytes.size()) return std::nullopt;

    const auto receiver_move_offset = call_offset - 3;
    if (!is_instruction_boundary(addresses, scan_address + receiver_move_offset)) {
        return std::nullopt;
    }
    const auto receiver_move = register_move_at(bytes, receiver_move_offset);
    if (!receiver_move || receiver_move->destination != 1 ||
        receiver_move->source == 1 || receiver_move->source == 4) {
        return std::nullopt;
    }

    std::size_t lea_offset{};
    std::int32_t pair_offset{};
    if (receiver_move_offset >= 5 && bytes[receiver_move_offset - 5] == 0x48 &&
        bytes[receiver_move_offset - 4] == 0x8D &&
        bytes[receiver_move_offset - 3] == 0x54 &&
        bytes[receiver_move_offset - 2] == 0x24) {
        lea_offset = receiver_move_offset - 5;
        pair_offset = static_cast<std::int8_t>(bytes[receiver_move_offset - 1]);
    } else if (receiver_move_offset >= 8 &&
               bytes[receiver_move_offset - 8] == 0x48 &&
               bytes[receiver_move_offset - 7] == 0x8D &&
               bytes[receiver_move_offset - 6] == 0x94 &&
               bytes[receiver_move_offset - 5] == 0x24) {
        lea_offset = receiver_move_offset - 8;
        std::memcpy(&pair_offset, bytes.data() + receiver_move_offset - 4,
                    sizeof(pair_offset));
    } else {
        return std::nullopt;
    }
    if (!is_instruction_boundary(addresses, scan_address + lea_offset) ||
        pair_offset < 0 || pair_offset > 0x400 || pair_offset % 8 != 0 ||
        lea_offset < 3) {
        return std::nullopt;
    }

    const auto third_argument_offset = lea_offset - 3;
    if (!is_instruction_boundary(addresses, scan_address + third_argument_offset)) {
        return std::nullopt;
    }
    const auto third_argument_move = register_move_at(bytes, third_argument_offset);
    if (!third_argument_move || third_argument_move->destination != 8 ||
        third_argument_move->source == 8 || third_argument_move->source == 4) {
        return std::nullopt;
    }

    return UiCallSetup{
        receiver_move->source,
        third_argument_move->source,
        pair_offset,
        third_argument_offset,
    };
}

struct MemoryOperand {
    std::uint8_t opcode{};
    std::uint8_t base_register{};
    std::int32_t displacement{};
    std::size_t size{};
};

[[nodiscard]] std::optional<MemoryOperand> memory_operand_at(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) noexcept {
    if (offset + 4 > bytes.size()) return std::nullopt;
    const auto rex = bytes[offset];
    if ((rex & 0xF8U) != 0x48U) return std::nullopt;
    const auto opcode = bytes[offset + 1];
    if (opcode != 0x8B && opcode != 0x89) return std::nullopt;
    const auto modrm = bytes[offset + 2];
    const auto mode = static_cast<std::uint8_t>((modrm >> 6U) & 3U);
    if (mode != 1 && mode != 2) return std::nullopt;
    auto cursor = offset + 3;
    auto base_low = static_cast<std::uint8_t>(modrm & 7U);
    if (base_low == 4) {
        if (cursor >= bytes.size()) return std::nullopt;
        const auto sib = bytes[cursor++];
        if (((sib >> 3U) & 7U) != 4U) return std::nullopt;
        base_low = static_cast<std::uint8_t>(sib & 7U);
    }
    const auto base_register = static_cast<std::uint8_t>(base_low + ((rex & 1U) ? 8U : 0U));
    std::int32_t displacement{};
    if (mode == 1) {
        if (cursor >= bytes.size()) return std::nullopt;
        displacement = static_cast<std::int8_t>(bytes[cursor++]);
    } else {
        if (cursor + sizeof(std::int32_t) > bytes.size()) return std::nullopt;
        std::memcpy(&displacement, bytes.data() + cursor, sizeof(displacement));
        cursor += sizeof(displacement);
    }
    return MemoryOperand{opcode, base_register, displacement, cursor - offset};
}

[[nodiscard]] bool selector_contract_valid_at(const ExecutableTextView& text,
                                              std::uintptr_t target) noexcept {
    const auto prologue = text.at(target, 15);
    constexpr std::array<std::uint8_t, 15> expected_prologue{
        0x48, 0x8B, 0xC4, 0x4C, 0x89, 0x40, 0x18, 0x48,
        0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08,
    };
    if (prologue.size() != expected_prologue.size() ||
        !std::equal(prologue.begin(), prologue.end(), expected_prologue.begin())) {
        return false;
    }
    const auto body = text.at(target, 160);
    if (body.empty()) return false;
    constexpr std::array<std::uint8_t, 9> zero_pair_a{
        0x33, 0xC0, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08,
    };
    constexpr std::array<std::uint8_t, 9> zero_pair_b{
        0x31, 0xC0, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08,
    };
    return std::search(body.begin(), body.end(), zero_pair_a.begin(), zero_pair_a.end()) != body.end() ||
           std::search(body.begin(), body.end(), zero_pair_b.begin(), zero_pair_b.end()) != body.end();
}

struct SelectorContractMatch {
    std::uintptr_t address{};
    std::size_t count{};
};

[[nodiscard]] SelectorContractMatch unique_selector_contract(
    const ExecutableTextView& text) noexcept {
    SelectorContractMatch result{};
    constexpr std::array<std::uint8_t, 15> prefix{
        0x48, 0x8B, 0xC4, 0x4C, 0x89, 0x40, 0x18, 0x48,
        0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08,
    };
    if (text.bytes.size() < prefix.size()) return result;
    for (std::size_t offset = 0; offset <= text.bytes.size() - prefix.size(); ++offset) {
        if (text.bytes[offset] != prefix.front() ||
            !std::equal(prefix.begin(), prefix.end(),
                        text.bytes.begin() + static_cast<std::ptrdiff_t>(offset))) {
            continue;
        }
        const auto address = text.address + offset;
        if (!selector_contract_valid_at(text, address)) continue;
        ++result.count;
        result.address = address;
        if (result.count > 1) return result;
    }
    return result;
}

[[nodiscard]] bool post_call_contract_matches(std::span<const std::uint8_t> bytes,
                                               std::uintptr_t scan_address,
                                               std::size_t call_offset,
                                               const CallSetup& setup,
                                               std::span<const std::uintptr_t> addresses,
                                               std::int32_t object_offset,
                                               std::int32_t component_offset) noexcept {
    const auto end = std::min(bytes.size(), call_offset + 5 + 112);
    bool object_read{};
    bool object_write{};
    bool component_write{};
    bool pair_first_read{};
    bool pair_second_read{};
    for (const auto instruction_address : addresses) {
        if (instruction_address < scan_address + call_offset + 5 ||
            instruction_address >= scan_address + end) {
            continue;
        }
        const auto offset = static_cast<std::size_t>(instruction_address - scan_address);
        const auto operand = memory_operand_at(bytes.first(end), offset);
        if (!operand) continue;
        if (operand->base_register == setup.receiver_register) {
            if (operand->displacement == object_offset) {
                object_read = object_read || operand->opcode == 0x8B;
                object_write = object_write || operand->opcode == 0x89;
            } else if (operand->displacement == component_offset) {
                component_write = component_write || operand->opcode == 0x89;
            }
        } else if (operand->base_register == 4 && operand->opcode == 0x8B) {
            pair_first_read = pair_first_read || operand->displacement == setup.pair_stack_offset;
            pair_second_read = pair_second_read ||
                operand->displacement == setup.pair_stack_offset + static_cast<std::int32_t>(sizeof(void*));
        }
    }
    return object_read && object_write && component_write && pair_first_read && pair_second_read;
}

[[nodiscard]] bool post_ui_call_pair_reads(
    std::span<const std::uint8_t> bytes,
    std::uintptr_t scan_address,
    std::size_t call_offset,
    const UiCallSetup& setup,
    std::span<const std::uintptr_t> addresses) noexcept {
    const auto first_post_call = call_offset + 5;
    const auto end = std::min(bytes.size(), first_post_call + 112);
    std::optional<std::size_t> first_read{};
    std::optional<std::size_t> second_read{};
    for (const auto instruction_address : addresses) {
        if (instruction_address < scan_address + first_post_call ||
            instruction_address >= scan_address + end) {
            continue;
        }
        const auto offset = static_cast<std::size_t>(instruction_address - scan_address);
        const auto operand = memory_operand_at(bytes.first(end), offset);
        if (!operand || operand->opcode != 0x8B || operand->base_register != 4) continue;
        if (operand->displacement == setup.pair_stack_offset && !first_read) {
            first_read = offset;
        } else if (operand->displacement ==
                       setup.pair_stack_offset + static_cast<std::int32_t>(sizeof(void*)) &&
                   !second_read) {
            second_read = offset;
        }
    }
    return first_read && second_read && *first_read < *second_read;
}

[[nodiscard]] bool callee_saved_gpr(std::uint8_t value) noexcept {
    return value == 3 || value == 5 || value == 6 || value == 7 ||
        (value >= 12 && value <= 15);
}

struct TerminalEpilogueInstruction {
    std::size_t size{};
    bool returns{};
};

[[nodiscard]] std::optional<std::size_t> callee_saved_restore_size(
    std::span<const std::uint8_t> bytes) noexcept {
    if (bytes.size() < 3) return std::nullopt;
    const auto rex = bytes[0];
    if ((rex & 0xF8U) != 0x48U || (rex & 0x02U) != 0 || bytes[1] != 0x8B) {
        return std::nullopt;
    }
    const auto modrm = bytes[2];
    const auto mode = static_cast<std::uint8_t>((modrm >> 6U) & 3U);
    if (mode == 3) return std::nullopt;
    const auto destination = static_cast<std::uint8_t>(
        ((modrm >> 3U) & 7U) + ((rex & 0x04U) ? 8U : 0U));
    if (!callee_saved_gpr(destination)) return std::nullopt;

    auto cursor = std::size_t{3};
    const auto rm_low = static_cast<std::uint8_t>(modrm & 7U);
    std::uint8_t base_register{};
    if (rm_low == 4) {
        if (cursor >= bytes.size()) return std::nullopt;
        const auto sib = bytes[cursor++];
        if (((sib >> 3U) & 7U) != 4U) return std::nullopt;
        base_register = static_cast<std::uint8_t>(
            (sib & 7U) + ((rex & 0x01U) ? 8U : 0U));
    } else {
        base_register = static_cast<std::uint8_t>(
            rm_low + ((rex & 0x01U) ? 8U : 0U));
    }
    if (base_register != 4 && base_register != 5) return std::nullopt;
    if (mode == 0 && base_register == 5) return std::nullopt;

    const auto displacement_size = mode == 1 ? std::size_t{1} :
        (mode == 2 ? sizeof(std::int32_t) : std::size_t{0});
    if (displacement_size > bytes.size() - cursor) return std::nullopt;
    return cursor + displacement_size;
}

[[nodiscard]] std::optional<TerminalEpilogueInstruction>
terminal_epilogue_instruction_at(const ExecutableTextView& text,
                                 std::uintptr_t address) noexcept {
    const auto bytes = text.at(address, std::min<std::size_t>(8,
        text.bytes.size() - static_cast<std::size_t>(address - text.address)));
    if (bytes.empty()) return std::nullopt;

    if (bytes[0] == 0xC3) return TerminalEpilogueInstruction{1, true};
    if (bytes[0] == 0xC9) return TerminalEpilogueInstruction{1, false};
    if (bytes[0] >= 0x58 && bytes[0] <= 0x5F &&
        callee_saved_gpr(static_cast<std::uint8_t>(bytes[0] - 0x58))) {
        return TerminalEpilogueInstruction{1, false};
    }
    if (bytes.size() >= 2 && bytes[0] == 0x41 &&
        bytes[1] >= 0x58 && bytes[1] <= 0x5F &&
        callee_saved_gpr(static_cast<std::uint8_t>(bytes[1] - 0x58 + 8))) {
        return TerminalEpilogueInstruction{2, false};
    }
    if (bytes.size() >= 4 && bytes[0] == 0x48 && bytes[1] == 0x83 &&
        bytes[2] == 0xC4 && bytes[3] != 0 && bytes[3] < 0x80 &&
        bytes[3] % sizeof(void*) == 0) {
        return TerminalEpilogueInstruction{4, false};
    }
    if (bytes.size() >= 7 && bytes[0] == 0x48 && bytes[1] == 0x81 &&
        bytes[2] == 0xC4) {
        std::int32_t amount{};
        std::memcpy(&amount, bytes.data() + 3, sizeof(amount));
        if (amount > 0 && amount % static_cast<std::int32_t>(sizeof(void*)) == 0) {
            return TerminalEpilogueInstruction{7, false};
        }
    }
    if (bytes.size() >= 3 &&
        ((bytes[0] == 0x48 && bytes[1] == 0x8B && bytes[2] == 0xE5) ||
         (bytes[0] == 0x48 && bytes[1] == 0x89 && bytes[2] == 0xEC))) {
        return TerminalEpilogueInstruction{3, false};
    }
    if (bytes.size() >= 4 && bytes[0] == 0x48 && bytes[1] == 0x8D &&
        bytes[2] == 0x65) {
        return TerminalEpilogueInstruction{4, false};
    }
    if (bytes.size() >= 7 && bytes[0] == 0x48 && bytes[1] == 0x8D &&
        bytes[2] == 0xA5) {
        return TerminalEpilogueInstruction{7, false};
    }
    const auto restore_size = callee_saved_restore_size(bytes);
    if (restore_size) return TerminalEpilogueInstruction{*restore_size, false};
    return std::nullopt;
}

[[nodiscard]] bool terminal_call_has_bounded_epilogue(
    const ExecutableTextView& text,
    std::span<const std::uintptr_t> instruction_addresses,
    std::uintptr_t call_site,
    std::uintptr_t fragment_end) noexcept {
    const auto call_position = std::lower_bound(instruction_addresses.begin(),
                                                instruction_addresses.end(), call_site);
    if (call_position == instruction_addresses.end() || *call_position != call_site ||
        call_site > std::numeric_limits<std::uintptr_t>::max() - 5 ||
        call_site + 5 > fragment_end) {
        return false;
    }
    auto expected = call_site + 5;
    for (auto position = call_position + 1;
         position != instruction_addresses.end(); ++position) {
        if (*position != expected || *position >= fragment_end) return false;
        const auto instruction = terminal_epilogue_instruction_at(text, *position);
        if (!instruction) return false;
        if (*position > std::numeric_limits<std::uintptr_t>::max() - instruction->size) {
            return false;
        }
        const auto instruction_end = *position + instruction->size;
        if (instruction_end > fragment_end) return false;
        if (instruction->returns) return instruction_end == fragment_end;
        expected = instruction_end;
    }
    return false;
}

} // namespace

DirectRel32CallCollectionResult collect_direct_rel32_calls(
    const ExecutableTextView& text,
    std::uintptr_t implementation_entry,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit) noexcept {
    if (scan_limit == 0 || text.bytes.empty()) {
        return {SelectorResolverStatus::InvalidInput, {}, 0};
    }
    if (!text.contains(implementation_entry)) {
        return {SelectorResolverStatus::AddressOutsideText, {}, 0};
    }
    if (implementation_entry >
        std::numeric_limits<std::uintptr_t>::max() - scan_limit) {
        return {SelectorResolverStatus::InvalidInput, {}, 0};
    }
    const auto available = text.bytes.size() -
        static_cast<std::size_t>(implementation_entry - text.address);
    if (scan_limit > available) {
        return {SelectorResolverStatus::InvalidInput, {}, 0};
    }
    const auto bounded_size = scan_limit;
    if (!instruction_addresses_valid(text, implementation_entry, bounded_size,
                                     instruction_addresses)) {
        return {SelectorResolverStatus::InvalidInput, {}, 0};
    }

    const auto bytes = text.at(implementation_entry, bounded_size);
    DirectRel32CallCollectionResult result{};
    for (std::size_t instruction_index = 0;
         instruction_index < instruction_addresses.size(); ++instruction_index) {
        const auto instruction_address = instruction_addresses[instruction_index];
        const auto offset = static_cast<std::size_t>(instruction_address - implementation_entry);
        if (bytes[offset] != 0xE8) continue;
        if (offset + 5 > bytes.size()) {
            return {SelectorResolverStatus::InvalidInput, {}, 0};
        }
        if (instruction_index + 1 >= instruction_addresses.size() ||
            instruction_address > std::numeric_limits<std::uintptr_t>::max() - 5 ||
            instruction_addresses[instruction_index + 1] != instruction_address + 5) {
            return {SelectorResolverStatus::InvalidInput, {}, 0};
        }
        const auto target = relative_target(text, instruction_address, 5, 1,
                                            sizeof(std::int32_t));
        if (!target || !text.contains(*target)) {
            return {SelectorResolverStatus::DirectCallTargetInvalid, {}, 0};
        }
        if (result.count == result.calls.size()) {
            return {SelectorResolverStatus::DirectCallCapacityExceeded, {}, 0};
        }
        result.calls[result.count++] = DirectRel32Call{instruction_address, *target};
    }
    if (result.count == 0) {
        return {SelectorResolverStatus::DirectCallNotFound, {}, 0};
    }
    result.status = SelectorResolverStatus::Resolved;
    return result;
}

TerminalRel32CallResult select_unique_terminal_rel32_call(
    const ExecutableTextView& text,
    std::uintptr_t implementation_entry,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit) noexcept {
    if (scan_limit == 0 || implementation_entry >
        std::numeric_limits<std::uintptr_t>::max() - scan_limit) {
        return {SelectorResolverStatus::InvalidInput, {}, 0};
    }
    const auto fragment_end = implementation_entry + scan_limit;
    const auto calls = collect_direct_rel32_calls(
        text, implementation_entry, instruction_addresses, scan_limit);
    if (!calls.resolved()) {
        const auto status = calls.status == SelectorResolverStatus::DirectCallNotFound
            ? SelectorResolverStatus::TerminalCallNotFound
            : calls.status;
        return {status, {}, 0};
    }

    DirectRel32Call selected{};
    std::size_t candidates{};
    for (std::size_t index = 0; index < calls.count; ++index) {
        if (!terminal_call_has_bounded_epilogue(text, instruction_addresses,
                                                calls.calls[index].call_site,
                                                fragment_end)) {
            continue;
        }
        ++candidates;
        selected = calls.calls[index];
    }
    if (candidates == 0) {
        return {SelectorResolverStatus::TerminalCallNotFound, {}, 0};
    }
    if (candidates != 1) {
        return {SelectorResolverStatus::TerminalCallAmbiguous, {}, candidates};
    }
    return {SelectorResolverStatus::Resolved, selected, candidates};
}

VirtualDispatchResult resolve_virtual_dispatch_slot(const ExecutableTextView& text,
                                                     std::uintptr_t reflected_exec_thunk,
                                                     std::span<const std::uintptr_t> instruction_addresses,
                                                     std::size_t scan_limit) noexcept {
    if (scan_limit == 0 || text.bytes.empty()) {
        return {SelectorResolverStatus::InvalidInput, 0, 0};
    }
    if (!text.contains(reflected_exec_thunk)) {
        return {SelectorResolverStatus::AddressOutsideText, 0, 0};
    }
    const auto available = text.bytes.size() - static_cast<std::size_t>(reflected_exec_thunk - text.address);
    const auto bounded_size = std::min(scan_limit, available);
    if (!instruction_addresses_valid(text, reflected_exec_thunk, bounded_size,
                                     instruction_addresses)) {
        return {SelectorResolverStatus::InvalidInput, 0, 0};
    }
    const auto bytes = text.at(reflected_exec_thunk, bounded_size);
    std::uint32_t selected_slot{};
    std::size_t raw_matches{};
    for (const auto instruction_address : instruction_addresses) {
        const auto offset = static_cast<std::size_t>(instruction_address - reflected_exec_thunk);
        const auto slot = dispatch_slot_at(bytes, offset);
        if (!slot) continue;
        ++raw_matches;
        selected_slot = *slot;
    }
    if (raw_matches == 0) return {SelectorResolverStatus::DispatchNotFound, 0, 0};
    if (raw_matches != 1) return {SelectorResolverStatus::DispatchAmbiguous, 0, raw_matches};
    return {SelectorResolverStatus::Resolved, selected_slot, raw_matches};
}

JumpFollowResult follow_direct_jump_chain(const ExecutableTextView& text,
                                          std::uintptr_t entry,
                                          std::uint32_t max_jumps) noexcept {
    constexpr std::size_t kVisitedCapacity = 8;
    if (max_jumps == 0 || max_jumps >= kVisitedCapacity || text.bytes.empty()) {
        return {SelectorResolverStatus::InvalidInput, 0, 0};
    }
    if (!text.contains(entry)) {
        return {SelectorResolverStatus::AddressOutsideText, 0, 0};
    }
    std::array<std::uintptr_t, kVisitedCapacity> visited{};
    std::size_t visited_count{};
    auto current = entry;
    for (std::uint32_t jumps = 0; jumps <= max_jumps; ++jumps) {
        if (!text.contains(current)) {
            return {SelectorResolverStatus::JumpTargetInvalid, 0, jumps};
        }
        if (std::find(visited.begin(), visited.begin() + static_cast<std::ptrdiff_t>(visited_count), current) !=
            visited.begin() + static_cast<std::ptrdiff_t>(visited_count)) {
            return {SelectorResolverStatus::JumpTargetInvalid, 0, jumps};
        }
        if (visited_count < visited.size()) visited[visited_count++] = current;
        const auto opcode = read_value<std::uint8_t>(text, current);
        if (!opcode || (*opcode != 0xE9 && *opcode != 0xEB)) {
            return {SelectorResolverStatus::Resolved, current, jumps};
        }
        if (jumps == max_jumps) {
            return {SelectorResolverStatus::JumpLimitExceeded, 0, jumps};
        }
        const auto target = *opcode == 0xE9
            ? relative_target(text, current, 5, 1, sizeof(std::int32_t))
            : relative_target(text, current, 2, 1, sizeof(std::int8_t));
        if (!target || !text.contains(*target)) {
            return {SelectorResolverStatus::JumpTargetInvalid, 0, jumps + 1};
        }
        current = *target;
    }
    return {SelectorResolverStatus::JumpLimitExceeded, 0, max_jumps};
}

SelectorCallResult resolve_selector_call(const ExecutableTextView& text,
                                         std::uintptr_t server_run_interact,
                                         std::span<const std::uintptr_t> instruction_addresses,
                                         std::int32_t target_object_offset,
                                         std::int32_t target_component_offset,
                                         std::size_t scan_limit) noexcept {
    if (scan_limit == 0 || text.bytes.empty()) {
        return {SelectorResolverStatus::InvalidInput, 0, 0, 0};
    }
    if (!text.contains(server_run_interact)) {
        return {SelectorResolverStatus::AddressOutsideText, 0, 0, 0};
    }
    if (target_object_offset < 0 || target_component_offset < 0 ||
        target_component_offset != target_object_offset + static_cast<std::int32_t>(sizeof(void*)) ||
        target_object_offset % static_cast<std::int32_t>(alignof(void*)) != 0) {
        return {SelectorResolverStatus::PropertyLayoutInvalid, 0, 0, 0};
    }
    const auto available = text.bytes.size() - static_cast<std::size_t>(server_run_interact - text.address);
    const auto bounded_size = std::min(scan_limit, available);
    if (!instruction_addresses_valid(text, server_run_interact, bounded_size,
                                     instruction_addresses)) {
        return {SelectorResolverStatus::InvalidInput, 0, 0, 0};
    }
    const auto bytes = text.at(server_run_interact, bounded_size);
    std::size_t structural_candidates{};
    std::uintptr_t structural_call{};
    for (const auto instruction_address : instruction_addresses) {
        const auto offset = static_cast<std::size_t>(instruction_address - server_run_interact);
        if (offset + 5 > bytes.size()) continue;
        if (bytes[offset] != 0xE8) continue;
        const auto setup = call_setup_before(bytes, server_run_interact, offset,
                                             instruction_addresses);
        if (!setup) continue;
        const auto prologue_limit = std::min<std::size_t>(setup->setup_start, 96);
        if (!register_saved_from_rcx(bytes.first(prologue_limit), server_run_interact,
                                     instruction_addresses, setup->receiver_register)) {
            continue;
        }
        if (!post_call_contract_matches(bytes, server_run_interact, offset, *setup,
                                        instruction_addresses,
                                        target_object_offset, target_component_offset)) {
            continue;
        }
        ++structural_candidates;
        structural_call = server_run_interact + offset;
    }
    if (structural_candidates == 0) return {SelectorResolverStatus::SelectorNotFound, 0, 0, 0};
    if (structural_candidates != 1) {
        return {SelectorResolverStatus::SelectorAmbiguous, 0, 0, structural_candidates};
    }
    const auto raw_target = relative_target(text, structural_call, 5, 1, sizeof(std::int32_t));
    if (!raw_target || !text.contains(*raw_target)) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 1};
    }
    const auto followed = follow_direct_jump_chain(text, *raw_target);
    if (!followed.resolved()) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 1};
    }
    const auto selector_contract = unique_selector_contract(text);
    if (selector_contract.count == 0) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 0};
    }
    if (selector_contract.count != 1) {
        return {SelectorResolverStatus::SelectorAmbiguous, 0, 0, selector_contract.count};
    }
    if (selector_contract.address != followed.address) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, selector_contract.count};
    }
    return {SelectorResolverStatus::Resolved, followed.address, structural_call, 1};
}

SelectorCallResult resolve_ui_selector_call(
    const ExecutableTextView& text,
    std::uintptr_t set_interact_ui,
    std::span<const std::uintptr_t> instruction_addresses,
    std::size_t scan_limit) noexcept {
    if (scan_limit == 0 || text.bytes.empty()) {
        return {SelectorResolverStatus::InvalidInput, 0, 0, 0};
    }
    if (!text.contains(set_interact_ui)) {
        return {SelectorResolverStatus::AddressOutsideText, 0, 0, 0};
    }
    const auto available = text.bytes.size() -
        static_cast<std::size_t>(set_interact_ui - text.address);
    const auto bounded_size = std::min(scan_limit, available);
    if (!instruction_addresses_valid(text, set_interact_ui, bounded_size,
                                     instruction_addresses)) {
        return {SelectorResolverStatus::InvalidInput, 0, 0, 0};
    }

    const auto bytes = text.at(set_interact_ui, bounded_size);
    std::size_t structural_candidates{};
    std::uintptr_t structural_call{};
    for (const auto instruction_address : instruction_addresses) {
        const auto offset = static_cast<std::size_t>(instruction_address - set_interact_ui);
        if (offset + 5 > bytes.size() || bytes[offset] != 0xE8) continue;
        const auto setup = ui_call_setup_before(bytes, set_interact_ui, offset,
                                                instruction_addresses);
        if (!setup || !post_ui_call_pair_reads(bytes, set_interact_ui, offset, *setup,
                                               instruction_addresses)) {
            continue;
        }
        ++structural_candidates;
        structural_call = set_interact_ui + offset;
    }
    if (structural_candidates == 0) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 0};
    }
    if (structural_candidates != 1) {
        return {SelectorResolverStatus::SelectorAmbiguous, 0, 0,
                structural_candidates};
    }

    const auto raw_target = relative_target(text, structural_call, 5, 1,
                                            sizeof(std::int32_t));
    if (!raw_target || !text.contains(*raw_target)) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 1};
    }
    const auto followed = follow_direct_jump_chain(text, *raw_target);
    if (!followed.resolved()) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 1};
    }

    const auto selector_contract = unique_selector_contract(text);
    if (selector_contract.count == 0) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0, 0};
    }
    if (selector_contract.count != 1) {
        return {SelectorResolverStatus::SelectorAmbiguous, 0, 0,
                selector_contract.count};
    }
    if (selector_contract.address != followed.address) {
        return {SelectorResolverStatus::SelectorNotFound, 0, 0,
                selector_contract.count};
    }
    return {SelectorResolverStatus::Resolved, followed.address, structural_call, 1};
}

std::string_view selector_resolver_status_name(SelectorResolverStatus status) noexcept {
    switch (status) {
    case SelectorResolverStatus::Resolved: return "resolved";
    case SelectorResolverStatus::InvalidInput: return "invalid_input";
    case SelectorResolverStatus::AddressOutsideText: return "address_outside_text";
    case SelectorResolverStatus::DirectCallNotFound: return "direct_call_not_found";
    case SelectorResolverStatus::DirectCallTargetInvalid: return "direct_call_target_invalid";
    case SelectorResolverStatus::DirectCallCapacityExceeded: return "direct_call_capacity_exceeded";
    case SelectorResolverStatus::TerminalCallNotFound: return "terminal_call_not_found";
    case SelectorResolverStatus::TerminalCallAmbiguous: return "terminal_call_ambiguous";
    case SelectorResolverStatus::DispatchNotFound: return "dispatch_not_found";
    case SelectorResolverStatus::DispatchAmbiguous: return "dispatch_ambiguous";
    case SelectorResolverStatus::JumpTargetInvalid: return "jump_target_invalid";
    case SelectorResolverStatus::JumpLimitExceeded: return "jump_limit_exceeded";
    case SelectorResolverStatus::PropertyLayoutInvalid: return "property_layout_invalid";
    case SelectorResolverStatus::SelectorNotFound: return "selector_not_found";
    case SelectorResolverStatus::SelectorAmbiguous: return "selector_ambiguous";
    }
    return "unknown";
}

} // namespace dsnap
