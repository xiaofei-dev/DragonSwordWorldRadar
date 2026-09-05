#pragma once

#include <cstddef>
#include <cstdint>

namespace dswros {

// The save service stored its SQLCipher key as an FString at +0x120 before
// game build 1.0.11. Keep that exact layout as the zero-regression fast path,
// then search only a small, aligned neighborhood when a compatible game update
// moves the field. Every discovered string is still required to unlock the
// active save database before it can be used.
inline constexpr std::size_t kLegacySaveKeyFieldOffset = 0x120U;
inline constexpr std::size_t kCompatibilitySaveKeyFieldOffset = 0x128U;
inline constexpr std::size_t kMinimumSaveKeyFieldOffset = 0x40U;
inline constexpr std::size_t kMaximumSaveKeyFieldOffset = 0x500U;
inline constexpr std::size_t kSaveKeyFieldStride = 8U;
inline constexpr std::int32_t kMinimumSaveKeyCharacters = 2;
inline constexpr std::int32_t kMaximumSaveKeyCharacters = 256;
inline constexpr std::int32_t kMaximumSaveKeyCapacity = 512;
inline constexpr std::uint32_t kMaximumSaveKeyCandidates = 24U;

[[nodiscard]] constexpr bool has_save_key_candidate_budget(
    std::uint32_t validated_candidates) noexcept {
    return validated_candidates < kMaximumSaveKeyCandidates;
}

[[nodiscard]] constexpr bool is_scannable_save_key_field_offset(
    std::size_t offset) noexcept {
    return offset >= kMinimumSaveKeyFieldOffset
        && offset <= kMaximumSaveKeyFieldOffset
        && (offset % kSaveKeyFieldStride) == 0U;
}

[[nodiscard]] constexpr bool is_plausible_save_key_descriptor(
    std::uintptr_t data, std::int32_t length,
    std::int32_t capacity) noexcept {
    return data != 0U
        && length >= kMinimumSaveKeyCharacters
        && length <= kMaximumSaveKeyCharacters
        && capacity >= length
        && capacity <= kMaximumSaveKeyCapacity;
}

// A packaged owner RVA is only a fast path. If it can no longer produce a key
// that authenticates the active save database, one full activation may replace
// it with the unique owner signature found in the current executable.
[[nodiscard]] constexpr bool
should_retry_packaged_owner_with_runtime_pattern(
    bool full_activation, bool packaged_owner_selected,
    bool save_key_validated, bool runtime_pattern_attempted) noexcept {
    return full_activation
        && packaged_owner_selected
        && !save_key_validated
        && !runtime_pattern_attempted;
}

} // namespace dswros
