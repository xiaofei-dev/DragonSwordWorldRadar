#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace dswros {

inline constexpr std::size_t kMaximumHotkeyConfigBytes = 4096U;

struct HotkeySettings {
    std::uint8_t settings{0x75}; // F6
    std::uint8_t enable{0x76};   // F7
    std::uint8_t disable{0x77};  // F8
};

enum class HotkeyConfigStatus : std::uint8_t {
    Success, Missing, Unreadable, Invalid, DuplicateBinding, TooLarge
};

struct HotkeyConfigResult {
    HotkeySettings settings{};
    HotkeyConfigStatus status{HotkeyConfigStatus::Invalid};
};

[[nodiscard]] constexpr std::string_view hotkey_trim(std::string_view value) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
    return value;
}

// Same key families as AutoPickup; case-insensitive names are convenient in INI files.
// Codes match UE4SS Input::Key and Windows virtual-key codes, not scan codes.
[[nodiscard]] constexpr std::optional<std::uint8_t> parse_radar_hotkey(std::string_view value) noexcept {
    value = hotkey_trim(value);
    if (value.empty() || value.size() > 8) return std::nullopt;
    std::array<char, 8> storage{};
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        storage[i] = c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
    }
    const std::string_view key{storage.data(), value.size()};
    if (key.size() == 1 && ((key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= '0' && key[0] <= '9')))
        return static_cast<std::uint8_t>(key[0]);
    if ((key.size() == 2 || key.size() == 3) && key[0] == 'F' && key[1] >= '1' && key[1] <= '9') {
        unsigned number = static_cast<unsigned>(key[1] - '0');
        if (key.size() == 3) {
            if (key[2] < '0' || key[2] > '9') return std::nullopt;
            number = number * 10 + static_cast<unsigned>(key[2] - '0');
        }
        if (number <= 24) return static_cast<std::uint8_t>(0x6F + number);
    }
    if (key.size() == 4 && key.substr(0, 3) == "NUM" && key[3] >= '0' && key[3] <= '9')
        return static_cast<std::uint8_t>(0x60 + key[3] - '0');
    constexpr std::array<std::string_view, 7> names{
        "HOME", "END", "PAGEUP", "PAGEDOWN", "INSERT", "DELETE", "SPACE"};
    constexpr std::array<std::uint8_t, 7> codes{0x24, 0x23, 0x21, 0x22, 0x2D, 0x2E, 0x20};
    for (std::size_t i = 0; i < names.size(); ++i) if (key == names[i]) return codes[i];
    return std::nullopt;
}

// Parse transactionally: one bad/missing/duplicate field cannot partly rebind controls.
// A rejected document returns all three defaults, leaving the disable key reachable.
[[nodiscard]] constexpr HotkeyConfigResult parse_hotkey_config(std::string_view text) noexcept {
    if (text.size() > kMaximumHotkeyConfigBytes) return {{}, HotkeyConfigStatus::TooLarge};
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
    bool section = false;
    unsigned seen = 0;
    HotkeySettings pending{};
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r' && end != std::string_view::npos) line.remove_suffix(1);
        if (line.find('\r') != std::string_view::npos || line.find('\0') != std::string_view::npos) return {};
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        line = hotkey_trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;
        if (line == "[hotkeys]") {
            if (section) return {};
            section = true;
            continue;
        }
        const auto equal = line.find('=');
        if (!section || equal == std::string_view::npos) return {};
        const auto name = hotkey_trim(line.substr(0, equal));
        const auto key = parse_radar_hotkey(line.substr(equal + 1));
        if (!key) return {};
        unsigned bit = 0;
        if (name == "settings_hotkey") { bit = 1; pending.settings = *key; }
        else if (name == "enable_hotkey") { bit = 2; pending.enable = *key; }
        else if (name == "disable_hotkey") { bit = 4; pending.disable = *key; }
        else return {};
        if ((seen & bit) != 0) return {};
        seen |= bit;
    }
    if (!section || seen != 7) return {};
    if (pending.settings == pending.enable || pending.settings == pending.disable || pending.enable == pending.disable)
        return {{}, HotkeyConfigStatus::DuplicateBinding};
    return {pending, HotkeyConfigStatus::Success};
}

[[nodiscard]] constexpr std::string_view hotkey_config_status_name(HotkeyConfigStatus status) noexcept {
    switch (status) {
    case HotkeyConfigStatus::Success: return "configured";
    case HotkeyConfigStatus::Missing: return "missing_defaults";
    case HotkeyConfigStatus::Unreadable: return "unreadable_defaults";
    case HotkeyConfigStatus::DuplicateBinding: return "duplicate_binding_defaults";
    case HotkeyConfigStatus::TooLarge: return "too_large_defaults";
    default: return "invalid_defaults";
    }
}
} // namespace dswros
