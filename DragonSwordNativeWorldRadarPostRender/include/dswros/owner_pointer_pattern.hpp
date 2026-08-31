#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace dswros {

enum class OwnerPointerPatternStatus : std::uint8_t {
    Unique,
    InvalidImage,
    NotFound,
    Ambiguous,
    TargetOutOfRange,
};

struct OwnerPointerPatternResult {
    OwnerPointerPatternStatus status{OwnerPointerPatternStatus::InvalidImage};
    std::uint64_t rva{};
    std::uint32_t match_count{};

    [[nodiscard]] bool success() const noexcept {
        return status == OwnerPointerPatternStatus::Unique;
    }
};

namespace detail {

template <typename Value>
[[nodiscard]] bool read_pe_value(
    std::span<const std::uint8_t> image,
    std::size_t offset,
    Value& value) noexcept {
    if (offset > image.size() || image.size() - offset < sizeof(Value)) {
        return false;
    }
    std::memcpy(&value, image.data() + offset, sizeof(Value));
    return true;
}

[[nodiscard]] inline bool matches_owner_pointer_pattern(
    const std::uint8_t* bytes) noexcept {
    return bytes[0] == 0x48 && bytes[1] == 0x8B && bytes[2] == 0x0D
        && bytes[7] == 0xE8
        && bytes[12] == 0x8B && bytes[13] == 0xC7
        && bytes[14] == 0x48 && bytes[15] == 0x8B
        && bytes[16] == 0x5C && bytes[17] == 0x24
        && bytes[18] == 0x40 && bytes[19] == 0x48
        && bytes[20] == 0x8B && bytes[21] == 0x6C
        && bytes[22] == 0x24 && bytes[23] == 0x50;
}

} // namespace detail

// Resolves the process-global save-owner pointer from an on-disk PE32+ image.
// The signature is accepted only when exactly one executable-section match
// produces an RVA inside SizeOfImage. The function allocates no memory.
[[nodiscard]] inline OwnerPointerPatternResult resolve_owner_pointer_rva(
    std::span<const std::uint8_t> image) noexcept {
    constexpr std::size_t kDosPeOffset = 0x3CU;
    constexpr std::size_t kPatternSize = 24U;
    constexpr std::uint32_t kPeSignature = 0x00004550U;
    constexpr std::uint16_t kAmd64Machine = 0x8664U;
    constexpr std::uint16_t kPe32PlusMagic = 0x020BU;
    constexpr std::uint32_t kExecutableSection = 0x20000000U;

    std::uint16_t dos_magic{};
    std::uint32_t pe_offset_u32{};
    if (!detail::read_pe_value(image, 0U, dos_magic)
        || dos_magic != 0x5A4DU
        || !detail::read_pe_value(image, kDosPeOffset, pe_offset_u32)) {
        return {};
    }
    const std::size_t pe_offset = pe_offset_u32;
    std::uint32_t signature{};
    std::uint16_t machine{};
    std::uint16_t section_count{};
    std::uint16_t optional_size{};
    if (!detail::read_pe_value(image, pe_offset, signature)
        || signature != kPeSignature
        || !detail::read_pe_value(image, pe_offset + 4U, machine)
        || machine != kAmd64Machine
        || !detail::read_pe_value(image, pe_offset + 6U, section_count)
        || section_count == 0U || section_count > 96U
        || !detail::read_pe_value(image, pe_offset + 20U, optional_size)
        || optional_size < 60U) {
        return {};
    }
    const std::size_t optional_offset = pe_offset + 24U;
    std::uint16_t optional_magic{};
    std::uint32_t size_of_image{};
    if (!detail::read_pe_value(image, optional_offset, optional_magic)
        || optional_magic != kPe32PlusMagic
        || !detail::read_pe_value(
            image, optional_offset + 56U, size_of_image)
        || size_of_image == 0U) {
        return {};
    }
    if (optional_offset > std::numeric_limits<std::size_t>::max()
            - optional_size) {
        return {};
    }
    const std::size_t section_table = optional_offset + optional_size;
    constexpr std::size_t kSectionHeaderSize = 40U;
    if (section_table > image.size()
        || section_count > (image.size() - section_table) / kSectionHeaderSize) {
        return {};
    }

    OwnerPointerPatternResult result{
        OwnerPointerPatternStatus::NotFound, 0U, 0U};
    bool out_of_range_target{};
    for (std::size_t section_index = 0;
         section_index < section_count; ++section_index) {
        const std::size_t header = section_table
            + section_index * kSectionHeaderSize;
        std::uint32_t virtual_address{};
        std::uint32_t raw_size{};
        std::uint32_t raw_offset{};
        std::uint32_t characteristics{};
        if (!detail::read_pe_value(image, header + 12U, virtual_address)
            || !detail::read_pe_value(image, header + 16U, raw_size)
            || !detail::read_pe_value(image, header + 20U, raw_offset)
            || !detail::read_pe_value(
                image, header + 36U, characteristics)) {
            return {};
        }
        if ((characteristics & kExecutableSection) == 0U
            || raw_size < kPatternSize) {
            continue;
        }
        const std::size_t raw_begin = raw_offset;
        if (raw_begin > image.size()
            || raw_size > image.size() - raw_begin) {
            return {};
        }
        const std::size_t raw_end = raw_begin + raw_size;
        for (std::size_t offset = raw_begin;
             offset <= raw_end - kPatternSize; ++offset) {
            if (!detail::matches_owner_pointer_pattern(
                    image.data() + offset)) {
                continue;
            }
            ++result.match_count;
            if (result.match_count > 1U) {
                result.status = OwnerPointerPatternStatus::Ambiguous;
                result.rva = 0U;
                return result;
            }
            std::int32_t displacement{};
            if (!detail::read_pe_value(image, offset + 3U, displacement)) {
                return {};
            }
            const std::uint64_t instruction_rva =
                static_cast<std::uint64_t>(virtual_address)
                + static_cast<std::uint64_t>(offset - raw_begin);
            const std::int64_t target = static_cast<std::int64_t>(
                instruction_rva + 7U) + displacement;
            if (target <= 0
                || static_cast<std::uint64_t>(target) >= size_of_image) {
                out_of_range_target = true;
                continue;
            }
            result.rva = static_cast<std::uint64_t>(target);
        }
    }
    if (result.match_count == 0U) {
        return result;
    }
    if (out_of_range_target || result.rva == 0U) {
        result.status = OwnerPointerPatternStatus::TargetOutOfRange;
        result.rva = 0U;
        return result;
    }
    result.status = OwnerPointerPatternStatus::Unique;
    return result;
}

} // namespace dswros
