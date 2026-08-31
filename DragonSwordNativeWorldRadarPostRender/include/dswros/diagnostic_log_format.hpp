#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace dswros {

// Enough for three full-width uint64 values and their fixed field names.
constexpr std::size_t kMaximumDiagnosticLogMetadataBytes = 96U;

[[nodiscard]] inline std::size_t format_diagnostic_log_metadata(
    std::span<char> output,
    std::uint64_t sequence,
    std::uint64_t utc_unix_ms,
    std::uint64_t elapsed_ms) noexcept {
    std::size_t offset{};
    const auto append_text = [&output, &offset](
                                 std::string_view value) noexcept {
        if (value.size() > output.size() - offset) {
            return false;
        }
        for (const char character : value) {
            output[offset++] = character;
        }
        return true;
    };
    const auto append_number = [&output, &offset](
                                   std::uint64_t value) noexcept {
        if (offset >= output.size()) {
            return false;
        }
        const auto converted = std::to_chars(
            output.data() + offset, output.data() + output.size(), value);
        if (converted.ec != std::errc{}) {
            return false;
        }
        offset = static_cast<std::size_t>(converted.ptr - output.data());
        return true;
    };
    if (!append_text(" seq=") || !append_number(sequence)
        || !append_text(" utc_ms=") || !append_number(utc_unix_ms)
        || !append_text(" elapsed_ms=") || !append_number(elapsed_ms)) {
        return 0;
    }
    return offset;
}

} // namespace dswros
