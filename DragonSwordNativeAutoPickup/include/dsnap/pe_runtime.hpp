#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace dsnap {

inline constexpr std::size_t kMaxX64RuntimeFragments = 64;

enum class PeRuntimeStatus : std::uint8_t {
    Success,
    InvalidArgument,
    InvalidDosHeader,
    InvalidNtHeaders,
    UnsupportedImage,
    InvalidOptionalHeader,
    InvalidSectionTable,
    MissingTextSection,
    AmbiguousTextSection,
    InvalidTextSection,
    InvalidExceptionDirectory,
    InvalidRuntimeFunctionTable,
    AddressOutsideText,
    RuntimeFunctionNotFound,
    RuntimeFunctionAmbiguous,
    InvalidUnwindInfo,
    ChainTargetNotFound,
    ChainTargetAmbiguous,
    ChainLoop,
    ChainDepthExceeded,
    FragmentOutsideText,
    FragmentCapacityExceeded,
};

[[nodiscard]] constexpr bool pe_runtime_succeeded(PeRuntimeStatus status) noexcept {
    return status == PeRuntimeStatus::Success;
}

struct PeRuntimeImageLayout {
    std::uintptr_t module_base{};
    std::uintptr_t image_end{};
    std::uintptr_t text_begin{};
    std::uintptr_t text_end{};
    std::uint32_t size_of_image{};
    std::uint32_t text_rva{};
    std::uint32_t text_size{};
    std::uint32_t exception_directory_rva{};
    std::uint32_t exception_directory_size{};
    std::size_t runtime_function_count{};
};

struct X64RuntimeFunctionFragment {
    std::uintptr_t begin{};
    std::uintptr_t end{};
    std::uint32_t begin_rva{};
    std::uint32_t end_rva{};
    std::uint32_t unwind_info_rva{};
};

struct X64RuntimeFragmentCollection {
    PeRuntimeImageLayout image{};
    X64RuntimeFunctionFragment canonical_root{};
    std::array<X64RuntimeFunctionFragment, kMaxX64RuntimeFragments> fragments{};
    std::size_t fragment_count{};
    std::size_t containing_fragment_index{};
};

// Parses a complete in-memory PE32+ image. File-layout bytes are intentionally
// unsupported: section and data-directory RVAs are interpreted directly into
// mapped_image. On every failure, output is reset to its all-zero state.
[[nodiscard]] PeRuntimeStatus parse_pe32_plus_loaded_image(
    std::span<const std::byte> mapped_image,
    std::uintptr_t module_base,
    PeRuntimeImageLayout* output) noexcept;

// Locates the unique x64 RUNTIME_FUNCTION containing address, follows bounded
// UNW_FLAG_CHAININFO records to its canonical root, and returns every sorted
// table fragment with that same root. Every returned fragment and every link in
// its chain must remain inside the exact executable-code .text section. On
// every failure, output is reset to its all-zero state.
[[nodiscard]] PeRuntimeStatus collect_x64_chained_runtime_fragments(
    std::span<const std::byte> mapped_image,
    std::uintptr_t module_base,
    std::uintptr_t address,
    X64RuntimeFragmentCollection* output) noexcept;

} // namespace dsnap
