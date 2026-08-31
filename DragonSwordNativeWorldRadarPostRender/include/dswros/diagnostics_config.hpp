#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace dswros {

constexpr std::size_t kMaximumDiagnosticsConfigBytes = 2048;

[[nodiscard]] constexpr std::string_view trim_diagnostics_token(
    std::string_view token) noexcept {
    while (!token.empty()
           && (token.front() == ' ' || token.front() == '\t')) {
        token.remove_prefix(1);
    }
    while (!token.empty()
           && (token.back() == ' ' || token.back() == '\t')) {
        token.remove_suffix(1);
    }
    return token;
}

// The current user-facing document follows the same small INI style as the
// Auto Pickup Mod: comments, blank lines, one named section, and one explicit
// debug_logging key. Unknown sections/keys, duplicates, non-canonical booleans,
// lone carriage returns, and oversized input still fail closed. The exact
// legacy event_log_enabled=<bool> line remains accepted so an update never
// silently discards an existing user's choice.
[[nodiscard]] constexpr std::optional<bool> parse_event_log_enabled(
    std::string_view document) noexcept {
    if (document.empty()
        || document.size() > kMaximumDiagnosticsConfigBytes) {
        return std::nullopt;
    }

    // Match installer validation exactly: accept LF or CRLF documents, but
    // never a mixture of the two and never a bare carriage return.
    bool saw_crlf{};
    bool saw_lf{};
    for (std::size_t index{}; index < document.size(); ++index) {
        if (document[index] == '\r') {
            if (index + 1U >= document.size()
                || document[index + 1U] != '\n') {
                return std::nullopt;
            }
            saw_crlf = true;
            ++index;
        } else if (document[index] == '\n') {
            saw_lf = true;
        }
        if (saw_crlf && saw_lf) {
            return std::nullopt;
        }
    }

    auto legacy = document;
    if (legacy.ends_with("\r\n")) {
        legacy.remove_suffix(2);
    } else if (legacy.ends_with('\n')) {
        legacy.remove_suffix(1);
    }
    if (legacy == "event_log_enabled=true") {
        return true;
    }
    if (legacy == "event_log_enabled=false") {
        return false;
    }

    bool section_seen{};
    bool value_seen{};
    bool enabled{};
    std::size_t offset{};
    while (offset < document.size()) {
        const auto line_end = document.find('\n', offset);
        const auto count = line_end == std::string_view::npos
            ? document.size() - offset
            : line_end - offset;
        auto line = document.substr(offset, count);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (line.find('\r') != std::string_view::npos) {
            return std::nullopt;
        }
        line = trim_diagnostics_token(line);
        if (!line.empty() && line.front() != '#' && line.front() != ';') {
            if (line == "[diagnostics]") {
                if (section_seen || value_seen) {
                    return std::nullopt;
                }
                section_seen = true;
            } else {
                const auto equals = line.find('=');
                if (!section_seen || value_seen
                    || equals == std::string_view::npos
                    || trim_diagnostics_token(line.substr(0, equals))
                        != "debug_logging") {
                    return std::nullopt;
                }
                const auto value = trim_diagnostics_token(
                    line.substr(equals + 1U));
                if (value == "true") {
                    enabled = true;
                } else if (value == "false") {
                    enabled = false;
                } else {
                    return std::nullopt;
                }
                value_seen = true;
            }
        }
        if (line_end == std::string_view::npos) {
            break;
        }
        offset = line_end + 1U;
    }
    return section_seen && value_seen
        ? std::optional<bool>{enabled}
        : std::nullopt;
}

} // namespace dswros
