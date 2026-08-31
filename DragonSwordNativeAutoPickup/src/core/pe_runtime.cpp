#include <dsnap/pe_runtime.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>

namespace dsnap {
namespace {

constexpr std::uint16_t kDosSignature = 0x5A4D;
constexpr std::uint32_t kNtSignature = 0x00004550;
constexpr std::uint16_t kAmd64Machine = 0x8664;
constexpr std::uint16_t kPe32PlusMagic = 0x020B;
constexpr std::uint16_t kExecutableImage = 0x0002;
constexpr std::uint32_t kCodeSection = 0x00000020;
constexpr std::uint32_t kExecuteSection = 0x20000000;
constexpr std::size_t kDosLfanewOffset = 0x3C;
constexpr std::size_t kCoffHeaderSize = 20;
constexpr std::size_t kSectionHeaderSize = 40;
constexpr std::size_t kPe32PlusDataDirectoryOffset = 112;
constexpr std::size_t kExceptionDirectoryIndex = 3;
constexpr std::size_t kDataDirectorySize = 8;
constexpr std::size_t kRuntimeFunctionSize = 12;
constexpr std::size_t kMaxPeSections = 96;
constexpr std::size_t kMaxRuntimeFunctionEntries = 1U << 20U;
constexpr std::size_t kMaxChainDepth = 32;
constexpr std::uint8_t kUnwindVersion = 1;
constexpr std::uint8_t kUnwindFlagExceptionHandler = 0x1;
constexpr std::uint8_t kUnwindFlagTerminationHandler = 0x2;
constexpr std::uint8_t kUnwindFlagChainInfo = 0x4;
constexpr std::uint8_t kKnownUnwindFlags = kUnwindFlagExceptionHandler |
    kUnwindFlagTerminationHandler | kUnwindFlagChainInfo;

struct SectionRange {
    std::uint32_t begin{};
    std::uint32_t end{};
};

struct RuntimeFunctionEntry {
    std::uint32_t begin_rva{};
    std::uint32_t end_rva{};
    std::uint32_t unwind_info_rva{};
};

struct RootResolution {
    std::size_t root_index{};
    bool all_fragments_in_text{};
};

template <typename Integer>
[[nodiscard]] bool read_little(std::span<const std::byte> bytes,
                               std::size_t offset,
                               Integer* output) noexcept {
    static_assert(std::is_unsigned_v<Integer>);
    if (!output || offset > bytes.size() || sizeof(Integer) > bytes.size() - offset) return false;
    Integer value{};
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        const auto byte = static_cast<Integer>(std::to_integer<std::uint8_t>(bytes[offset + index]));
        value |= static_cast<Integer>(byte << (index * 8U));
    }
    *output = value;
    return true;
}

[[nodiscard]] bool checked_add(std::size_t left,
                               std::size_t right,
                               std::size_t* output) noexcept {
    if (!output || left > std::numeric_limits<std::size_t>::max() - right) return false;
    *output = left + right;
    return true;
}

[[nodiscard]] bool checked_multiply(std::size_t left,
                                    std::size_t right,
                                    std::size_t* output) noexcept {
    if (!output || (left != 0 && right > std::numeric_limits<std::size_t>::max() / left)) return false;
    *output = left * right;
    return true;
}

[[nodiscard]] bool range_fits(std::size_t offset,
                              std::size_t size,
                              std::size_t limit) noexcept {
    return offset <= limit && size <= limit - offset;
}

[[nodiscard]] bool rva_range_fits(std::uint32_t rva,
                                  std::uint32_t size,
                                  const PeRuntimeImageLayout& image,
                                  std::size_t mapped_size) noexcept {
    const auto begin = static_cast<std::uint64_t>(rva);
    const auto end = begin + static_cast<std::uint64_t>(size);
    return end <= image.size_of_image && end <= mapped_size;
}

[[nodiscard]] bool checked_absolute(std::uintptr_t module_base,
                                    std::uint32_t rva,
                                    std::uintptr_t* output) noexcept {
    if (!output || module_base > std::numeric_limits<std::uintptr_t>::max() - rva) return false;
    *output = module_base + rva;
    return true;
}

[[nodiscard]] bool exact_text_name(std::span<const std::byte> image,
                                   std::size_t section_offset) noexcept {
    constexpr std::array<std::uint8_t, 8> expected{
        '.', 't', 'e', 'x', 't', 0, 0, 0,
    };
    if (!range_fits(section_offset, expected.size(), image.size())) return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (std::to_integer<std::uint8_t>(image[section_offset + index]) != expected[index]) return false;
    }
    return true;
}

[[nodiscard]] bool ranges_overlap(std::uint32_t left_begin,
                                  std::uint32_t left_end,
                                  std::uint32_t right_begin,
                                  std::uint32_t right_end) noexcept {
    return left_begin < right_end && right_begin < left_end;
}

[[nodiscard]] bool entry_in_text(const RuntimeFunctionEntry& entry,
                                 const PeRuntimeImageLayout& image) noexcept {
    const auto text_end = static_cast<std::uint64_t>(image.text_rva) + image.text_size;
    return entry.begin_rva >= image.text_rva && entry.end_rva <= text_end;
}

[[nodiscard]] bool runtime_entry_at(std::span<const std::byte> mapped_image,
                                    const PeRuntimeImageLayout& image,
                                    std::size_t index,
                                    RuntimeFunctionEntry* output) noexcept {
    if (!output || index >= image.runtime_function_count) return false;
    std::size_t entry_delta{};
    std::size_t entry_offset{};
    if (!checked_multiply(index, kRuntimeFunctionSize, &entry_delta) ||
        !checked_add(image.exception_directory_rva, entry_delta, &entry_offset) ||
        !range_fits(entry_offset, kRuntimeFunctionSize, mapped_image.size())) {
        return false;
    }
    return read_little(mapped_image, entry_offset, &output->begin_rva) &&
           read_little(mapped_image, entry_offset + 4, &output->end_rva) &&
           read_little(mapped_image, entry_offset + 8, &output->unwind_info_rva);
}

[[nodiscard]] PeRuntimeStatus validate_runtime_function_table(
    std::span<const std::byte> mapped_image,
    const PeRuntimeImageLayout& image) noexcept {
    RuntimeFunctionEntry previous{};
    for (std::size_t index = 0; index < image.runtime_function_count; ++index) {
        RuntimeFunctionEntry entry{};
        if (!runtime_entry_at(mapped_image, image, index, &entry) ||
            entry.begin_rva >= entry.end_rva || entry.end_rva > image.size_of_image ||
            (entry.unwind_info_rva & 0x3U) != 0 ||
            !rva_range_fits(entry.unwind_info_rva, 4, image, mapped_image.size())) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        if (index != 0 &&
            (entry.begin_rva <= previous.begin_rva || entry.begin_rva < previous.end_rva)) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        previous = entry;
    }
    return PeRuntimeStatus::Success;
}

[[nodiscard]] PeRuntimeStatus parse_internal(std::span<const std::byte> mapped_image,
                                             std::uintptr_t module_base,
                                             PeRuntimeImageLayout* output) noexcept {
    if (!output || mapped_image.empty() || module_base == 0) return PeRuntimeStatus::InvalidArgument;

    std::uint16_t dos_signature{};
    std::uint32_t nt_offset_u32{};
    if (!read_little(mapped_image, 0, &dos_signature) || dos_signature != kDosSignature ||
        !read_little(mapped_image, kDosLfanewOffset, &nt_offset_u32) ||
        (nt_offset_u32 & 0x80000000U) != 0) {
        return PeRuntimeStatus::InvalidDosHeader;
    }
    const auto nt_offset = static_cast<std::size_t>(nt_offset_u32);
    if (nt_offset < 64 || !range_fits(nt_offset, 4 + kCoffHeaderSize, mapped_image.size())) {
        return PeRuntimeStatus::InvalidDosHeader;
    }

    std::uint32_t nt_signature{};
    if (!read_little(mapped_image, nt_offset, &nt_signature) || nt_signature != kNtSignature) {
        return PeRuntimeStatus::InvalidNtHeaders;
    }
    const auto coff_offset = nt_offset + 4;
    std::uint16_t machine{};
    std::uint16_t number_of_sections{};
    std::uint16_t optional_header_size{};
    std::uint16_t characteristics{};
    if (!read_little(mapped_image, coff_offset, &machine) ||
        !read_little(mapped_image, coff_offset + 2, &number_of_sections) ||
        !read_little(mapped_image, coff_offset + 16, &optional_header_size) ||
        !read_little(mapped_image, coff_offset + 18, &characteristics)) {
        return PeRuntimeStatus::InvalidNtHeaders;
    }
    if (machine != kAmd64Machine || (characteristics & kExecutableImage) == 0) {
        return PeRuntimeStatus::UnsupportedImage;
    }
    if (number_of_sections == 0 || number_of_sections > kMaxPeSections) {
        return PeRuntimeStatus::InvalidSectionTable;
    }

    std::size_t optional_offset{};
    if (!checked_add(coff_offset, kCoffHeaderSize, &optional_offset) ||
        !range_fits(optional_offset, optional_header_size, mapped_image.size())) {
        return PeRuntimeStatus::InvalidOptionalHeader;
    }
    const auto required_optional_size = kPe32PlusDataDirectoryOffset +
        (kExceptionDirectoryIndex + 1) * kDataDirectorySize;
    if (optional_header_size < required_optional_size) {
        return PeRuntimeStatus::InvalidOptionalHeader;
    }

    std::uint16_t optional_magic{};
    std::uint32_t section_alignment{};
    std::uint32_t size_of_image{};
    std::uint32_t size_of_headers{};
    std::uint32_t number_of_directories{};
    if (!read_little(mapped_image, optional_offset, &optional_magic) ||
        !read_little(mapped_image, optional_offset + 32, &section_alignment) ||
        !read_little(mapped_image, optional_offset + 56, &size_of_image) ||
        !read_little(mapped_image, optional_offset + 60, &size_of_headers) ||
        !read_little(mapped_image, optional_offset + 108, &number_of_directories)) {
        return PeRuntimeStatus::InvalidOptionalHeader;
    }
    const auto directory_capacity =
        (static_cast<std::size_t>(optional_header_size) - kPe32PlusDataDirectoryOffset) /
        kDataDirectorySize;
    if (optional_magic != kPe32PlusMagic || section_alignment == 0 || size_of_image == 0 ||
        size_of_headers == 0 || size_of_headers > size_of_image ||
        size_of_image > mapped_image.size() || number_of_directories <= kExceptionDirectoryIndex ||
        number_of_directories > directory_capacity ||
        module_base > std::numeric_limits<std::uintptr_t>::max() - size_of_image) {
        return PeRuntimeStatus::InvalidOptionalHeader;
    }

    std::size_t section_table_offset{};
    std::size_t section_table_size{};
    std::size_t section_table_end{};
    if (!checked_add(optional_offset, optional_header_size, &section_table_offset) ||
        !checked_multiply(number_of_sections, kSectionHeaderSize, &section_table_size) ||
        !checked_add(section_table_offset, section_table_size, &section_table_end) ||
        section_table_end > size_of_headers ||
        !range_fits(section_table_offset, section_table_size, mapped_image.size())) {
        return PeRuntimeStatus::InvalidSectionTable;
    }

    std::array<SectionRange, kMaxPeSections> section_ranges{};
    std::size_t section_range_count{};
    std::size_t text_name_count{};
    std::uint32_t text_rva{};
    std::uint32_t text_size{};
    std::uint32_t text_characteristics{};
    for (std::size_t index = 0; index < number_of_sections; ++index) {
        const auto section_offset = section_table_offset + index * kSectionHeaderSize;
        std::uint32_t virtual_size{};
        std::uint32_t virtual_address{};
        std::uint32_t raw_size{};
        std::uint32_t section_characteristics{};
        if (!read_little(mapped_image, section_offset + 8, &virtual_size) ||
            !read_little(mapped_image, section_offset + 12, &virtual_address) ||
            !read_little(mapped_image, section_offset + 16, &raw_size) ||
            !read_little(mapped_image, section_offset + 36, &section_characteristics)) {
            return PeRuntimeStatus::InvalidSectionTable;
        }
        const auto mapped_size = virtual_size != 0 ? virtual_size : raw_size;
        if (mapped_size != 0) {
            const auto range_end = static_cast<std::uint64_t>(virtual_address) + mapped_size;
            if (virtual_address < size_of_headers || range_end > size_of_image ||
                range_end > mapped_image.size()) {
                return PeRuntimeStatus::InvalidSectionTable;
            }
            section_ranges[section_range_count++] = {
                virtual_address,
                static_cast<std::uint32_t>(range_end),
            };
        }
        if (exact_text_name(mapped_image, section_offset)) {
            ++text_name_count;
            text_rva = virtual_address;
            text_size = virtual_size;
            text_characteristics = section_characteristics;
        }
    }
    if (text_name_count == 0) return PeRuntimeStatus::MissingTextSection;
    if (text_name_count != 1) return PeRuntimeStatus::AmbiguousTextSection;
    if (text_size == 0 || text_rva < size_of_headers ||
        (text_characteristics & kCodeSection) == 0 ||
        (text_characteristics & kExecuteSection) == 0 ||
        static_cast<std::uint64_t>(text_rva) + text_size > size_of_image) {
        return PeRuntimeStatus::InvalidTextSection;
    }
    for (std::size_t left = 0; left < section_range_count; ++left) {
        for (std::size_t right = left + 1; right < section_range_count; ++right) {
            if (ranges_overlap(section_ranges[left].begin, section_ranges[left].end,
                               section_ranges[right].begin, section_ranges[right].end)) {
                return PeRuntimeStatus::InvalidSectionTable;
            }
        }
    }

    const auto exception_directory_offset = optional_offset + kPe32PlusDataDirectoryOffset +
        kExceptionDirectoryIndex * kDataDirectorySize;
    std::uint32_t exception_rva{};
    std::uint32_t exception_size{};
    if (!read_little(mapped_image, exception_directory_offset, &exception_rva) ||
        !read_little(mapped_image, exception_directory_offset + 4, &exception_size) ||
        exception_rva == 0 || exception_size == 0 ||
        exception_size % kRuntimeFunctionSize != 0) {
        return PeRuntimeStatus::InvalidExceptionDirectory;
    }

    PeRuntimeImageLayout parsed{};
    parsed.module_base = module_base;
    parsed.image_end = module_base + size_of_image;
    parsed.size_of_image = size_of_image;
    parsed.text_rva = text_rva;
    parsed.text_size = text_size;
    parsed.exception_directory_rva = exception_rva;
    parsed.exception_directory_size = exception_size;
    parsed.runtime_function_count = exception_size / kRuntimeFunctionSize;
    if (parsed.runtime_function_count == 0 ||
        parsed.runtime_function_count > kMaxRuntimeFunctionEntries ||
        !rva_range_fits(exception_rva, exception_size, parsed, mapped_image.size()) ||
        ranges_overlap(text_rva, text_rva + text_size,
                       exception_rva, exception_rva + exception_size) ||
        !checked_absolute(module_base, text_rva, &parsed.text_begin) ||
        !checked_absolute(module_base, text_rva + text_size, &parsed.text_end)) {
        return PeRuntimeStatus::InvalidExceptionDirectory;
    }

    const auto table_status = validate_runtime_function_table(mapped_image, parsed);
    if (!pe_runtime_succeeded(table_status)) return table_status;
    *output = parsed;
    return PeRuntimeStatus::Success;
}

[[nodiscard]] PeRuntimeStatus find_exact_runtime_entry(
    std::span<const std::byte> mapped_image,
    const PeRuntimeImageLayout& image,
    const RuntimeFunctionEntry& target,
    std::size_t* output_index) noexcept {
    if (!output_index) return PeRuntimeStatus::InvalidArgument;
    std::size_t first{};
    std::size_t last = image.runtime_function_count;
    while (first < last) {
        const auto middle = first + (last - first) / 2;
        RuntimeFunctionEntry entry{};
        if (!runtime_entry_at(mapped_image, image, middle, &entry)) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        if (entry.begin_rva < target.begin_rva) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    if (first >= image.runtime_function_count) return PeRuntimeStatus::ChainTargetNotFound;
    RuntimeFunctionEntry found{};
    if (!runtime_entry_at(mapped_image, image, first, &found)) {
        return PeRuntimeStatus::InvalidRuntimeFunctionTable;
    }
    if (found.begin_rva != target.begin_rva || found.end_rva != target.end_rva ||
        found.unwind_info_rva != target.unwind_info_rva) {
        return PeRuntimeStatus::ChainTargetNotFound;
    }
    if (first + 1 < image.runtime_function_count) {
        RuntimeFunctionEntry next{};
        if (!runtime_entry_at(mapped_image, image, first + 1, &next)) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        if (next.begin_rva == target.begin_rva && next.end_rva == target.end_rva &&
            next.unwind_info_rva == target.unwind_info_rva) {
            return PeRuntimeStatus::ChainTargetAmbiguous;
        }
    }
    *output_index = first;
    return PeRuntimeStatus::Success;
}

[[nodiscard]] PeRuntimeStatus chained_entry_from_unwind(
    std::span<const std::byte> mapped_image,
    const PeRuntimeImageLayout& image,
    const RuntimeFunctionEntry& entry,
    bool* chained,
    RuntimeFunctionEntry* chain_target) noexcept {
    if (!chained || !chain_target ||
        !rva_range_fits(entry.unwind_info_rva, 4, image, mapped_image.size())) {
        return PeRuntimeStatus::InvalidUnwindInfo;
    }
    const auto unwind_offset = static_cast<std::size_t>(entry.unwind_info_rva);
    const auto version_and_flags = std::to_integer<std::uint8_t>(mapped_image[unwind_offset]);
    const auto version = static_cast<std::uint8_t>(version_and_flags & 0x7U);
    const auto flags = static_cast<std::uint8_t>(version_and_flags >> 3U);
    const auto unwind_code_count = std::to_integer<std::uint8_t>(mapped_image[unwind_offset + 2]);
    if (version != kUnwindVersion || (flags & static_cast<std::uint8_t>(~kKnownUnwindFlags)) != 0 ||
        ((flags & kUnwindFlagChainInfo) != 0 &&
         (flags & (kUnwindFlagExceptionHandler | kUnwindFlagTerminationHandler)) != 0)) {
        return PeRuntimeStatus::InvalidUnwindInfo;
    }

    const auto aligned_code_count = (static_cast<std::size_t>(unwind_code_count) + 1U) & ~std::size_t{1};
    std::size_t code_bytes{};
    std::size_t trailer_offset{};
    if (!checked_multiply(aligned_code_count, 2, &code_bytes) ||
        !checked_add(unwind_offset, 4, &trailer_offset) ||
        !checked_add(trailer_offset, code_bytes, &trailer_offset) ||
        trailer_offset > image.size_of_image || trailer_offset > mapped_image.size()) {
        return PeRuntimeStatus::InvalidUnwindInfo;
    }

    if ((flags & kUnwindFlagChainInfo) != 0) {
        if (!range_fits(trailer_offset, kRuntimeFunctionSize, image.size_of_image) ||
            !range_fits(trailer_offset, kRuntimeFunctionSize, mapped_image.size()) ||
            !read_little(mapped_image, trailer_offset, &chain_target->begin_rva) ||
            !read_little(mapped_image, trailer_offset + 4, &chain_target->end_rva) ||
            !read_little(mapped_image, trailer_offset + 8, &chain_target->unwind_info_rva) ||
            chain_target->begin_rva >= chain_target->end_rva ||
            chain_target->end_rva > image.size_of_image ||
            (chain_target->unwind_info_rva & 0x3U) != 0 ||
            !rva_range_fits(chain_target->unwind_info_rva, 4, image, mapped_image.size())) {
            return PeRuntimeStatus::InvalidUnwindInfo;
        }
        *chained = true;
        return PeRuntimeStatus::Success;
    }

    if ((flags & (kUnwindFlagExceptionHandler | kUnwindFlagTerminationHandler)) != 0 &&
        (!range_fits(trailer_offset, 4, image.size_of_image) ||
         !range_fits(trailer_offset, 4, mapped_image.size()))) {
        return PeRuntimeStatus::InvalidUnwindInfo;
    }
    *chained = false;
    *chain_target = {};
    return PeRuntimeStatus::Success;
}

[[nodiscard]] PeRuntimeStatus resolve_canonical_root(
    std::span<const std::byte> mapped_image,
    const PeRuntimeImageLayout& image,
    std::size_t start_index,
    RootResolution* output) noexcept {
    if (!output || start_index >= image.runtime_function_count) {
        return PeRuntimeStatus::InvalidArgument;
    }
    std::array<std::size_t, kMaxChainDepth> visited{};
    std::size_t visited_count{};
    std::size_t current_index = start_index;
    bool all_in_text = true;

    while (true) {
        for (std::size_t index = 0; index < visited_count; ++index) {
            if (visited[index] == current_index) return PeRuntimeStatus::ChainLoop;
        }
        if (visited_count >= visited.size()) return PeRuntimeStatus::ChainDepthExceeded;
        visited[visited_count++] = current_index;

        RuntimeFunctionEntry current{};
        if (!runtime_entry_at(mapped_image, image, current_index, &current)) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        all_in_text = all_in_text && entry_in_text(current, image);

        bool chained{};
        RuntimeFunctionEntry chain_target{};
        const auto unwind_status = chained_entry_from_unwind(
            mapped_image, image, current, &chained, &chain_target);
        if (!pe_runtime_succeeded(unwind_status)) return unwind_status;
        if (!chained) {
            *output = {current_index, all_in_text};
            return PeRuntimeStatus::Success;
        }

        std::size_t target_index{};
        const auto target_status = find_exact_runtime_entry(
            mapped_image, image, chain_target, &target_index);
        if (!pe_runtime_succeeded(target_status)) return target_status;
        current_index = target_index;
    }
}

[[nodiscard]] bool make_fragment(const RuntimeFunctionEntry& entry,
                                 const PeRuntimeImageLayout& image,
                                 X64RuntimeFunctionFragment* output) noexcept {
    if (!output) return false;
    X64RuntimeFunctionFragment fragment{};
    fragment.begin_rva = entry.begin_rva;
    fragment.end_rva = entry.end_rva;
    fragment.unwind_info_rva = entry.unwind_info_rva;
    if (!checked_absolute(image.module_base, entry.begin_rva, &fragment.begin) ||
        !checked_absolute(image.module_base, entry.end_rva, &fragment.end)) {
        return false;
    }
    *output = fragment;
    return true;
}

} // namespace

PeRuntimeStatus parse_pe32_plus_loaded_image(std::span<const std::byte> mapped_image,
                                             std::uintptr_t module_base,
                                             PeRuntimeImageLayout* output) noexcept {
    if (!output) return PeRuntimeStatus::InvalidArgument;
    *output = {};
    PeRuntimeImageLayout parsed{};
    const auto status = parse_internal(mapped_image, module_base, &parsed);
    if (!pe_runtime_succeeded(status)) return status;
    *output = parsed;
    return PeRuntimeStatus::Success;
}

PeRuntimeStatus collect_x64_chained_runtime_fragments(
    std::span<const std::byte> mapped_image,
    std::uintptr_t module_base,
    std::uintptr_t address,
    X64RuntimeFragmentCollection* output) noexcept {
    if (!output) return PeRuntimeStatus::InvalidArgument;
    *output = {};

    PeRuntimeImageLayout image{};
    const auto parse_status = parse_internal(mapped_image, module_base, &image);
    if (!pe_runtime_succeeded(parse_status)) return parse_status;
    if (address < module_base) return PeRuntimeStatus::AddressOutsideText;
    const auto address_delta = address - module_base;
    const auto text_end_rva = static_cast<std::uint64_t>(image.text_rva) + image.text_size;
    if (address_delta > std::numeric_limits<std::uint32_t>::max() ||
        address_delta < image.text_rva || address_delta >= text_end_rva) {
        return PeRuntimeStatus::AddressOutsideText;
    }
    const auto address_rva = static_cast<std::uint32_t>(address_delta);

    std::size_t containing_index{};
    std::size_t containing_count{};
    for (std::size_t index = 0; index < image.runtime_function_count; ++index) {
        RuntimeFunctionEntry entry{};
        if (!runtime_entry_at(mapped_image, image, index, &entry)) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        if (address_rva >= entry.begin_rva && address_rva < entry.end_rva) {
            containing_index = index;
            ++containing_count;
        }
    }
    if (containing_count == 0) return PeRuntimeStatus::RuntimeFunctionNotFound;
    if (containing_count != 1) return PeRuntimeStatus::RuntimeFunctionAmbiguous;

    RootResolution containing_root{};
    const auto containing_status = resolve_canonical_root(
        mapped_image, image, containing_index, &containing_root);
    if (!pe_runtime_succeeded(containing_status)) return containing_status;
    if (!containing_root.all_fragments_in_text) return PeRuntimeStatus::FragmentOutsideText;

    X64RuntimeFragmentCollection collected{};
    collected.image = image;
    for (std::size_t index = 0; index < image.runtime_function_count; ++index) {
        RootResolution resolution{};
        const auto resolution_status = resolve_canonical_root(mapped_image, image, index, &resolution);
        if (!pe_runtime_succeeded(resolution_status)) return resolution_status;
        if (resolution.root_index != containing_root.root_index) continue;
        if (!resolution.all_fragments_in_text) return PeRuntimeStatus::FragmentOutsideText;
        if (collected.fragment_count >= collected.fragments.size()) {
            return PeRuntimeStatus::FragmentCapacityExceeded;
        }
        RuntimeFunctionEntry entry{};
        if (!runtime_entry_at(mapped_image, image, index, &entry) ||
            !make_fragment(entry, image, &collected.fragments[collected.fragment_count])) {
            return PeRuntimeStatus::InvalidRuntimeFunctionTable;
        }
        ++collected.fragment_count;
    }
    if (collected.fragment_count == 0) return PeRuntimeStatus::RuntimeFunctionNotFound;

    std::sort(collected.fragments.begin(),
              collected.fragments.begin() + static_cast<std::ptrdiff_t>(collected.fragment_count),
              [](const X64RuntimeFunctionFragment& left,
                 const X64RuntimeFunctionFragment& right) {
                  return left.begin_rva < right.begin_rva;
              });

    RuntimeFunctionEntry root_entry{};
    if (!runtime_entry_at(mapped_image, image, containing_root.root_index, &root_entry) ||
        !make_fragment(root_entry, image, &collected.canonical_root)) {
        return PeRuntimeStatus::InvalidRuntimeFunctionTable;
    }

    std::size_t sorted_containing_count{};
    for (std::size_t index = 0; index < collected.fragment_count; ++index) {
        const auto& fragment = collected.fragments[index];
        if (address_rva >= fragment.begin_rva && address_rva < fragment.end_rva) {
            collected.containing_fragment_index = index;
            ++sorted_containing_count;
        }
    }
    if (sorted_containing_count == 0) return PeRuntimeStatus::RuntimeFunctionNotFound;
    if (sorted_containing_count != 1) return PeRuntimeStatus::RuntimeFunctionAmbiguous;

    *output = collected;
    return PeRuntimeStatus::Success;
}

} // namespace dsnap
