#include <dsnap/pe_runtime.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <span>
#include <vector>

namespace {

struct RuntimeEntry {
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t unwind{};
};

class PeFixture {
public:
    static constexpr std::uintptr_t kBase = 0x140000000ULL;
    static constexpr std::size_t kNtOffset = 0x80;
    static constexpr std::size_t kOptionalOffset = kNtOffset + 4 + 20;
    static constexpr std::size_t kOptionalSize = 0xF0;
    static constexpr std::size_t kSectionTable = kOptionalOffset + kOptionalSize;
    static constexpr std::uint32_t kImageSize = 0x6000;
    static constexpr std::uint32_t kTextRva = 0x1000;
    static constexpr std::uint32_t kTextSize = 0x1000;
    static constexpr std::uint32_t kExceptionRva = 0x3000;
    static constexpr std::uint32_t kXdataRva = 0x4000;

    PeFixture() : bytes(kImageSize) {
        write_u16(0, 0x5A4D);
        write_u32(0x3C, static_cast<std::uint32_t>(kNtOffset));
        write_u32(kNtOffset, 0x00004550);
        write_u16(kNtOffset + 4, 0x8664);
        write_u16(kNtOffset + 6, 3);
        write_u16(kNtOffset + 4 + 16, static_cast<std::uint16_t>(kOptionalSize));
        write_u16(kNtOffset + 4 + 18, 0x0022);

        write_u16(kOptionalOffset, 0x020B);
        write_u64(kOptionalOffset + 24, kBase);
        write_u32(kOptionalOffset + 32, 0x1000);
        write_u32(kOptionalOffset + 36, 0x200);
        write_u32(kOptionalOffset + 56, kImageSize);
        write_u32(kOptionalOffset + 60, 0x400);
        write_u32(kOptionalOffset + 108, 16);

        write_section(0, ".text", kTextRva, kTextSize, 0x60000020);
        write_section(1, ".pdata", kExceptionRva, 0x1000, 0x40000040);
        write_section(2, ".xdata", kXdataRva, 0x1000, 0x40000040);

        const RuntimeEntry root{0x1100, 0x1180, 0x4000};
        const RuntimeEntry first{0x1200, 0x1240, 0x4020};
        const RuntimeEntry second{0x1280, 0x12C0, 0x4040};
        set_runtime_entries({root, first, second});
        write_plain_unwind(root.unwind);
        write_chained_unwind(first.unwind, root);
        write_chained_unwind(second.unwind, first);
    }

    void write_u8(std::size_t offset, std::uint8_t value) {
        bytes.at(offset) = static_cast<std::byte>(value);
    }

    void write_u16(std::size_t offset, std::uint16_t value) {
        for (std::size_t index = 0; index < 2; ++index) {
            write_u8(offset + index, static_cast<std::uint8_t>(value >> (index * 8U)));
        }
    }

    void write_u32(std::size_t offset, std::uint32_t value) {
        for (std::size_t index = 0; index < 4; ++index) {
            write_u8(offset + index, static_cast<std::uint8_t>(value >> (index * 8U)));
        }
    }

    void write_u64(std::size_t offset, std::uint64_t value) {
        for (std::size_t index = 0; index < 8; ++index) {
            write_u8(offset + index, static_cast<std::uint8_t>(value >> (index * 8U)));
        }
    }

    void write_section(std::size_t index,
                       const char* name,
                       std::uint32_t virtual_address,
                       std::uint32_t virtual_size,
                       std::uint32_t characteristics) {
        const auto offset = kSectionTable + index * 40;
        for (std::size_t name_index = 0; name_index < 8; ++name_index) write_u8(offset + name_index, 0);
        for (std::size_t name_index = 0; name[name_index] != '\0' && name_index < 8; ++name_index) {
            write_u8(offset + name_index, static_cast<std::uint8_t>(name[name_index]));
        }
        write_u32(offset + 8, virtual_size);
        write_u32(offset + 12, virtual_address);
        write_u32(offset + 16, virtual_size);
        write_u32(offset + 20, 0x400 + static_cast<std::uint32_t>(index) * 0x200);
        write_u32(offset + 36, characteristics);
    }

    void set_runtime_entries(std::initializer_list<RuntimeEntry> entries) {
        set_runtime_entries(std::vector<RuntimeEntry>{entries});
    }

    void set_runtime_entries(const std::vector<RuntimeEntry>& entries) {
        std::fill(bytes.begin() + kExceptionRva, bytes.begin() + kExceptionRva + 0x1000,
                  std::byte{});
        std::size_t index{};
        for (const auto& entry : entries) {
            const auto offset = kExceptionRva + index * 12;
            write_u32(offset, entry.begin);
            write_u32(offset + 4, entry.end);
            write_u32(offset + 8, entry.unwind);
            ++index;
        }
        write_u32(kOptionalOffset + 112 + 3 * 8, kExceptionRva);
        write_u32(kOptionalOffset + 112 + 3 * 8 + 4,
                  static_cast<std::uint32_t>(entries.size() * 12));
    }

    void write_plain_unwind(std::uint32_t rva, std::uint8_t code_count = 0) {
        write_u8(rva, 1);
        write_u8(rva + 1, 0);
        write_u8(rva + 2, code_count);
        write_u8(rva + 3, 0);
    }

    void write_chained_unwind(std::uint32_t rva,
                              const RuntimeEntry& target,
                              std::uint8_t code_count = 0) {
        write_u8(rva, 0x21);
        write_u8(rva + 1, 0);
        write_u8(rva + 2, code_count);
        write_u8(rva + 3, 0);
        const auto aligned_count = (static_cast<std::uint32_t>(code_count) + 1U) & ~1U;
        const auto trailer = rva + 4U + aligned_count * 2U;
        write_u32(trailer, target.begin);
        write_u32(trailer + 4, target.end);
        write_u32(trailer + 8, target.unwind);
    }

    [[nodiscard]] std::span<const std::byte> view() const noexcept { return bytes; }

    std::vector<std::byte> bytes;
};

[[nodiscard]] bool collection_is_zero(const dsnap::X64RuntimeFragmentCollection& value) {
    if (value.image.module_base != 0 || value.image.image_end != 0 ||
        value.canonical_root.begin != 0 || value.canonical_root.end != 0 ||
        value.fragment_count != 0 || value.containing_fragment_index != 0) {
        return false;
    }
    for (const auto& fragment : value.fragments) {
        if (fragment.begin != 0 || fragment.end != 0 || fragment.begin_rva != 0 ||
            fragment.end_rva != 0 || fragment.unwind_info_rva != 0) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool rejected_and_zero(PeFixture& fixture,
                                     std::uintptr_t address) {
    dsnap::X64RuntimeFragmentCollection output{};
    output.image.module_base = 1;
    output.canonical_root.begin = 2;
    output.fragments[0].begin = 3;
    output.fragment_count = 4;
    output.containing_fragment_index = 5;
    const auto status = dsnap::collect_x64_chained_runtime_fragments(
        fixture.view(), PeFixture::kBase, address, &output);
    return !dsnap::pe_runtime_succeeded(status) && collection_is_zero(output);
}

[[nodiscard]] bool test_valid_image_and_chained_fragments() {
    PeFixture fixture{};
    dsnap::PeRuntimeImageLayout layout{};
    const auto parse_status = dsnap::parse_pe32_plus_loaded_image(
        fixture.view(), PeFixture::kBase, &layout);
    if (!dsnap::pe_runtime_succeeded(parse_status) ||
        layout.module_base != PeFixture::kBase || layout.size_of_image != PeFixture::kImageSize ||
        layout.text_rva != PeFixture::kTextRva || layout.text_size != PeFixture::kTextSize ||
        layout.text_begin != PeFixture::kBase + PeFixture::kTextRva ||
        layout.text_end != PeFixture::kBase + PeFixture::kTextRva + PeFixture::kTextSize ||
        layout.runtime_function_count != 3) {
        return false;
    }

    dsnap::X64RuntimeFragmentCollection output{};
    const auto collect_status = dsnap::collect_x64_chained_runtime_fragments(
        fixture.view(), PeFixture::kBase, PeFixture::kBase + 0x1290, &output);
    return dsnap::pe_runtime_succeeded(collect_status) && output.fragment_count == 3 &&
           output.canonical_root.begin_rva == 0x1100 &&
           output.canonical_root.end_rva == 0x1180 &&
           output.fragments[0].begin_rva == 0x1100 &&
           output.fragments[1].begin_rva == 0x1200 &&
           output.fragments[2].begin_rva == 0x1280 &&
           output.containing_fragment_index == 2 &&
           output.fragments[2].begin == PeFixture::kBase + 0x1280;
}

[[nodiscard]] bool test_invalid_headers_and_text_contract() {
    PeFixture bad_dos{};
    bad_dos.write_u16(0, 0);
    if (!rejected_and_zero(bad_dos, PeFixture::kBase + 0x1100)) return false;

    PeFixture pe32{};
    pe32.write_u16(PeFixture::kOptionalOffset, 0x010B);
    if (!rejected_and_zero(pe32, PeFixture::kBase + 0x1100)) return false;

    PeFixture duplicate_text{};
    duplicate_text.write_section(1, ".text", PeFixture::kExceptionRva, 0x1000, 0x60000020);
    if (!rejected_and_zero(duplicate_text, PeFixture::kBase + 0x1100)) return false;

    PeFixture non_executable_text{};
    non_executable_text.write_section(0, ".text", PeFixture::kTextRva,
                                      PeFixture::kTextSize, 0x40000020);
    return rejected_and_zero(non_executable_text, PeFixture::kBase + 0x1100);
}

[[nodiscard]] bool test_directory_table_and_address_rejections() {
    PeFixture bad_directory{};
    bad_directory.write_u32(PeFixture::kOptionalOffset + 112 + 3 * 8 + 4, 13);
    if (!rejected_and_zero(bad_directory, PeFixture::kBase + 0x1100)) return false;

    PeFixture overlapping_entries{};
    const RuntimeEntry root{0x1100, 0x1180, 0x4000};
    const RuntimeEntry overlap{0x1170, 0x1240, 0x4020};
    overlapping_entries.set_runtime_entries({root, overlap});
    overlapping_entries.write_plain_unwind(root.unwind);
    overlapping_entries.write_plain_unwind(overlap.unwind);
    if (!rejected_and_zero(overlapping_entries, PeFixture::kBase + 0x1178)) return false;

    PeFixture valid{};
    if (!rejected_and_zero(valid, PeFixture::kBase + PeFixture::kExceptionRva)) return false;

    dsnap::PeRuntimeImageLayout layout{};
    layout.module_base = 7;
    const auto truncated = valid.view().first(0x3500);
    const auto status = dsnap::parse_pe32_plus_loaded_image(truncated, PeFixture::kBase, &layout);
    return !dsnap::pe_runtime_succeeded(status) && layout.module_base == 0 && layout.image_end == 0;
}

[[nodiscard]] bool test_chain_rejections() {
    const RuntimeEntry root{0x1100, 0x1180, 0x4000};
    const RuntimeEntry child{0x1200, 0x1240, 0x4020};

    PeFixture missing_target{};
    const RuntimeEntry absent{0x1300, 0x1340, 0x4080};
    missing_target.set_runtime_entries({root, child});
    missing_target.write_plain_unwind(root.unwind);
    missing_target.write_chained_unwind(child.unwind, absent);
    if (!rejected_and_zero(missing_target, PeFixture::kBase + 0x1210)) return false;

    PeFixture loop{};
    loop.set_runtime_entries({root, child});
    loop.write_chained_unwind(root.unwind, child);
    loop.write_chained_unwind(child.unwind, root);
    if (!rejected_and_zero(loop, PeFixture::kBase + 0x1210)) return false;

    PeFixture out_of_text{};
    const RuntimeEntry foreign_fragment{0x3100, 0x3120, 0x4020};
    out_of_text.set_runtime_entries({root, foreign_fragment});
    out_of_text.write_plain_unwind(root.unwind);
    out_of_text.write_chained_unwind(foreign_fragment.unwind, root);
    if (!rejected_and_zero(out_of_text, PeFixture::kBase + 0x1110)) return false;

    PeFixture truncated_unwind{};
    const RuntimeEntry end_unwind_child{0x1200, 0x1240, 0x5FFC};
    truncated_unwind.set_runtime_entries({root, end_unwind_child});
    truncated_unwind.write_plain_unwind(root.unwind);
    truncated_unwind.write_u8(end_unwind_child.unwind, 0x21);
    truncated_unwind.write_u8(end_unwind_child.unwind + 1, 0);
    truncated_unwind.write_u8(end_unwind_child.unwind + 2, 0);
    truncated_unwind.write_u8(end_unwind_child.unwind + 3, 0);
    return rejected_and_zero(truncated_unwind, PeFixture::kBase + 0x1210);
}

[[nodiscard]] bool test_fragment_capacity_and_base_overflow() {
    PeFixture many{};
    const RuntimeEntry root{0x1100, 0x1108, 0x4000};
    std::vector<RuntimeEntry> entries;
    entries.push_back(root);
    many.write_plain_unwind(root.unwind);
    for (std::uint32_t index = 0; index < dsnap::kMaxX64RuntimeFragments; ++index) {
        const RuntimeEntry child{
            0x1200U + index * 0x10U,
            0x1208U + index * 0x10U,
            0x4020U + index * 0x10U,
        };
        entries.push_back(child);
        many.write_chained_unwind(child.unwind, root);
    }
    many.set_runtime_entries(entries);
    // set_runtime_entries clears only .pdata, so the .xdata chain fixtures remain intact.
    if (!rejected_and_zero(many, PeFixture::kBase + 0x1101)) return false;

    PeFixture valid{};
    dsnap::PeRuntimeImageLayout output{};
    output.module_base = 1;
    const auto overflowing_base = std::numeric_limits<std::uintptr_t>::max() - 0x1000U;
    const auto status = dsnap::parse_pe32_plus_loaded_image(valid.view(), overflowing_base, &output);
    return !dsnap::pe_runtime_succeeded(status) && output.module_base == 0 && output.image_end == 0;
}

} // namespace

namespace dsnap::tests {

[[nodiscard]] bool run_pe_runtime_tests() {
    return test_valid_image_and_chained_fragments() &&
           test_invalid_headers_and_text_contract() &&
           test_directory_table_and_address_rejections() &&
           test_chain_rejections() &&
           test_fragment_capacity_and_base_overflow();
}

} // namespace dsnap::tests

extern "C" bool dsnap_run_pe_runtime_tests() {
    return dsnap::tests::run_pe_runtime_tests();
}
