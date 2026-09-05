#pragma once

#include "dswros/radar_preferences.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace dswros {

inline constexpr std::size_t kMaximumVisibilityConfigBytes = 4096U;

enum class VisibilityConfigFormat : std::uint8_t {
    Sectioned,
    LegacySchema1,
    LegacySchema2,
    LegacySchema3,
    LegacySchema4,
};

enum class VisibilityConfigParseStatus : std::uint8_t {
    Success,
    Empty,
    TooLarge,
    Utf8Bom,
    InvalidLineEnding,
    InvalidSection,
    DuplicateSection,
    KeyOutsideSection,
    MixedFormats,
    UnknownKey,
    DuplicateKey,
    InvalidValue,
    MissingKey,
    UnsupportedLegacySchema,
};

enum class VisibilityAreaQuestMode : std::uint8_t {
    Available,
    All,
};

enum class VisibilityAssaultMode : std::uint8_t {
    Available,
    All,
};

struct VisibilityConfigSettings {
    bool radar_clock{true};
    bool radar_treasure{true};
    bool radar_boss{true};
    bool radar_assault{true};
    bool radar_mini_games{true};
    bool radar_area_quests{true};
    bool radar_bird_eggs{true};

    bool map_treasure{true};
    bool map_boss{true};
    bool map_assault{true};
    bool map_mini_games{true};
    bool map_area_quests{true};

    VisibilityAreaQuestMode area_quest_mode{
        VisibilityAreaQuestMode::Available};
    VisibilityAssaultMode assault_mode{VisibilityAssaultMode::Available};

    bool height_treasure{true};
    bool height_area_quests{true};
    bool height_mole{true};
    RadarLanguagePreference language{RadarLanguagePreference::Auto};
};

struct VisibilityConfigParseResult {
    VisibilityConfigParseStatus status{VisibilityConfigParseStatus::Empty};
    VisibilityConfigFormat format{VisibilityConfigFormat::Sectioned};
    VisibilityConfigSettings settings{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return status == VisibilityConfigParseStatus::Success;
    }
};

namespace detail {

[[nodiscard]] constexpr std::string_view trim_ascii(
    std::string_view value) noexcept {
    while (!value.empty()
           && (value.front() == ' ' || value.front() == '\t')) {
        value.remove_prefix(1U);
    }
    while (!value.empty()
           && (value.back() == ' ' || value.back() == '\t')) {
        value.remove_suffix(1U);
    }
    return value;
}

[[nodiscard]] constexpr bool parse_boolean(
    std::string_view value, bool& output) noexcept {
    if (value == "true") {
        output = true;
        return true;
    }
    if (value == "false") {
        output = false;
        return true;
    }
    return false;
}

[[nodiscard]] constexpr bool parse_u32(
    std::string_view value, std::uint32_t& output) noexcept {
    if (value.empty()) {
        return false;
    }
    std::uint32_t parsed{};
    for (const char character : value) {
        if (character < '0' || character > '9') {
            return false;
        }
        const std::uint32_t digit = static_cast<std::uint32_t>(
            character - '0');
        if (parsed > (UINT32_MAX - digit) / 10U) {
            return false;
        }
        parsed = parsed * 10U + digit;
    }
    output = parsed;
    return true;
}

} // namespace detail

[[nodiscard]] constexpr VisibilityConfigParseResult parse_visibility_config(
    std::string_view text) noexcept {
    VisibilityConfigParseResult result{};
    if (text.empty()) {
        return result;
    }
    if (text.size() > kMaximumVisibilityConfigBytes) {
        result.status = VisibilityConfigParseStatus::TooLarge;
        return result;
    }
    // Existing schema 1-4 files could be written by Windows tooling with a
    // UTF-8 BOM. Treat it as an encoding marker so an update never preserves
    // bytes that the runtime would then silently ignore.
    if (text.size() >= 3U
        && static_cast<unsigned char>(text[0]) == 0xEFU
        && static_cast<unsigned char>(text[1]) == 0xBBU
        && static_cast<unsigned char>(text[2]) == 0xBFU) {
        text.remove_prefix(3U);
        if (text.empty()) {
            return result;
        }
    }

    enum class Section : std::uint8_t {
        None,
        Radar,
        Map,
        Modes,
        HeightArrows,
        Interface,
    };
    Section section{Section::None};
    std::uint8_t seen_sections{};
    std::uint8_t radar_keys{};
    std::uint8_t map_keys{};
    std::uint8_t mode_keys{};
    std::uint8_t height_arrow_keys{};
    std::uint8_t interface_keys{};
    bool saw_sectioned{};
    bool saw_legacy{};
    bool saw_crlf{};
    bool saw_lf{};

    std::uint8_t legacy_keys{};
    std::uint32_t legacy_schema{1U};
    bool legacy_schema_explicit{};
    std::uint32_t legacy_compact{};
    std::uint32_t legacy_world{};
    VisibilityAreaQuestMode legacy_area_mode{
        VisibilityAreaQuestMode::Available};
    VisibilityAssaultMode legacy_assault_mode{
        VisibilityAssaultMode::Available};

    std::size_t cursor{};
    while (cursor < text.size()) {
        const std::size_t newline = text.find('\n', cursor);
        const std::size_t end = newline == std::string_view::npos
            ? text.size()
            : newline;
        std::string_view line = text.substr(cursor, end - cursor);
        cursor = newline == std::string_view::npos ? text.size() : end + 1U;
        if (newline != std::string_view::npos) {
            if (!line.empty() && line.back() == '\r') {
                saw_crlf = true;
            } else {
                saw_lf = true;
            }
            if (saw_crlf && saw_lf) {
                result.status = VisibilityConfigParseStatus::InvalidLineEnding;
                return result;
            }
        }
        if (!line.empty() && line.back() == '\r') {
            if (newline == std::string_view::npos) {
                result.status =
                    VisibilityConfigParseStatus::InvalidLineEnding;
                return result;
            }
            line.remove_suffix(1U);
        }
        if (line.find('\r') != std::string_view::npos) {
            result.status = VisibilityConfigParseStatus::InvalidLineEnding;
            return result;
        }
        line = detail::trim_ascii(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;
        }

        if (line.front() == '[') {
            if (saw_legacy) {
                result.status = VisibilityConfigParseStatus::MixedFormats;
                return result;
            }
            saw_sectioned = true;
            std::uint8_t section_bit{};
            if (line == "[radar]") {
                section = Section::Radar;
                section_bit = 0x01U;
            } else if (line == "[map]") {
                section = Section::Map;
                section_bit = 0x02U;
            } else if (line == "[modes]") {
                section = Section::Modes;
                section_bit = 0x04U;
            } else if (line == "[height_arrows]") {
                section = Section::HeightArrows;
                section_bit = 0x08U;
            } else if (line == "[interface]") {
                section = Section::Interface;
                section_bit = 0x10U;
            } else {
                result.status = VisibilityConfigParseStatus::InvalidSection;
                return result;
            }
            if ((seen_sections & section_bit) != 0U) {
                result.status = VisibilityConfigParseStatus::DuplicateSection;
                return result;
            }
            seen_sections = static_cast<std::uint8_t>(
                seen_sections | section_bit);
            continue;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string_view::npos
            || line.find('=', separator + 1U) != std::string_view::npos) {
            result.status = VisibilityConfigParseStatus::InvalidValue;
            return result;
        }
        const std::string_view key = detail::trim_ascii(
            line.substr(0U, separator));
        const std::string_view value = detail::trim_ascii(
            line.substr(separator + 1U));
        if (key.empty() || value.empty()) {
            result.status = VisibilityConfigParseStatus::InvalidValue;
            return result;
        }

        if (saw_sectioned) {
            if (section == Section::None) {
                result.status = VisibilityConfigParseStatus::KeyOutsideSection;
                return result;
            }
            bool* target{};
            std::uint8_t key_bit{};
            std::uint8_t* key_set{};
            if (section == Section::Radar) {
                key_set = &radar_keys;
                if (key == "clock") {
                    key_bit = 0x01U;
                    target = &result.settings.radar_clock;
                } else if (key == "treasure") {
                    key_bit = 0x02U;
                    target = &result.settings.radar_treasure;
                } else if (key == "boss") {
                    key_bit = 0x04U;
                    target = &result.settings.radar_boss;
                } else if (key == "assault") {
                    key_bit = 0x08U;
                    target = &result.settings.radar_assault;
                } else if (key == "mini_games") {
                    key_bit = 0x10U;
                    target = &result.settings.radar_mini_games;
                } else if (key == "area_quests") {
                    key_bit = 0x20U;
                    target = &result.settings.radar_area_quests;
                } else if (key == "bird_eggs") {
                    key_bit = 0x40U;
                    target = &result.settings.radar_bird_eggs;
                }
            } else if (section == Section::Map) {
                key_set = &map_keys;
                if (key == "treasure") {
                    key_bit = 0x01U;
                    target = &result.settings.map_treasure;
                } else if (key == "boss") {
                    key_bit = 0x02U;
                    target = &result.settings.map_boss;
                } else if (key == "assault") {
                    key_bit = 0x04U;
                    target = &result.settings.map_assault;
                } else if (key == "mini_games") {
                    key_bit = 0x08U;
                    target = &result.settings.map_mini_games;
                } else if (key == "area_quests") {
                    key_bit = 0x10U;
                    target = &result.settings.map_area_quests;
                }
            } else if (section == Section::Modes) {
                key_set = &mode_keys;
                if (key == "area_quests") {
                    key_bit = 0x01U;
                    if (value == "available") {
                        result.settings.area_quest_mode =
                            VisibilityAreaQuestMode::Available;
                    } else if (value == "all") {
                        result.settings.area_quest_mode =
                            VisibilityAreaQuestMode::All;
                    } else {
                        result.status = VisibilityConfigParseStatus::InvalidValue;
                        return result;
                    }
                } else if (key == "assault") {
                    key_bit = 0x02U;
                    if (value == "available") {
                        result.settings.assault_mode =
                            VisibilityAssaultMode::Available;
                    } else if (value == "all") {
                        result.settings.assault_mode =
                            VisibilityAssaultMode::All;
                    } else {
                        result.status = VisibilityConfigParseStatus::InvalidValue;
                        return result;
                    }
                }
            } else if (section == Section::HeightArrows) {
                key_set = &height_arrow_keys;
                if (key == "treasure") {
                    key_bit = 0x01U;
                    target = &result.settings.height_treasure;
                } else if (key == "area_quests") {
                    key_bit = 0x02U;
                    target = &result.settings.height_area_quests;
                } else if (key == "mole") {
                    key_bit = 0x04U;
                    target = &result.settings.height_mole;
                }
            } else if (section == Section::Interface) {
                key_set = &interface_keys;
                if (key == "language") {
                    key_bit = 0x01U;
                    if (!parse_radar_language_preference(
                            value, result.settings.language)) {
                        result.status =
                            VisibilityConfigParseStatus::InvalidValue;
                        return result;
                    }
                }
            }
            if (key_bit == 0U || key_set == nullptr) {
                result.status = VisibilityConfigParseStatus::UnknownKey;
                return result;
            }
            if ((*key_set & key_bit) != 0U) {
                result.status = VisibilityConfigParseStatus::DuplicateKey;
                return result;
            }
            *key_set = static_cast<std::uint8_t>(*key_set | key_bit);
            if (target != nullptr && !detail::parse_boolean(value, *target)) {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
            continue;
        }

        saw_legacy = true;
        std::uint8_t key_bit{};
        if (key == "schema_version") {
            key_bit = 0x01U;
            if (!detail::parse_u32(value, legacy_schema)) {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
            legacy_schema_explicit = true;
        } else if (key == "compact_mask") {
            key_bit = 0x02U;
            if (!detail::parse_u32(value, legacy_compact)) {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
        } else if (key == "world_mask") {
            key_bit = 0x04U;
            if (!detail::parse_u32(value, legacy_world)) {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
        } else if (key == "area_quest_mode") {
            key_bit = 0x08U;
            if (value == "available") {
                legacy_area_mode = VisibilityAreaQuestMode::Available;
            } else if (value == "all") {
                legacy_area_mode = VisibilityAreaQuestMode::All;
            } else {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
        } else if (key == "assault_mode") {
            key_bit = 0x10U;
            if (value == "available" || value == "current") {
                legacy_assault_mode = VisibilityAssaultMode::Available;
            } else if (value == "all") {
                legacy_assault_mode = VisibilityAssaultMode::All;
            } else {
                result.status = VisibilityConfigParseStatus::InvalidValue;
                return result;
            }
        } else {
            result.status = VisibilityConfigParseStatus::UnknownKey;
            return result;
        }
        if ((legacy_keys & key_bit) != 0U) {
            result.status = VisibilityConfigParseStatus::DuplicateKey;
            return result;
        }
        legacy_keys = static_cast<std::uint8_t>(legacy_keys | key_bit);
    }

    if (saw_sectioned) {
        const bool old_sectioned = seen_sections == 0x07U;
        const bool current_sectioned = seen_sections == 0x1FU
            && height_arrow_keys == 0x07U
            && interface_keys == 0x01U;
        if ((!old_sectioned && !current_sectioned)
            || radar_keys != 0x7FU || map_keys != 0x1FU
            || mode_keys != 0x03U) {
            result.status = VisibilityConfigParseStatus::MissingKey;
            return result;
        }
        result.status = VisibilityConfigParseStatus::Success;
        result.format = VisibilityConfigFormat::Sectioned;
        return result;
    }
    if (!saw_legacy) {
        result.status = VisibilityConfigParseStatus::Empty;
        return result;
    }
    if (legacy_schema < 1U || legacy_schema > 4U) {
        result.status = VisibilityConfigParseStatus::UnsupportedLegacySchema;
        return result;
    }
    if (!legacy_schema_explicit && legacy_schema != 1U) {
        result.status = VisibilityConfigParseStatus::UnsupportedLegacySchema;
        return result;
    }
    const std::uint8_t required_keys = legacy_schema >= 4U
        ? 0x1FU
        : legacy_schema == 3U
            ? 0x0FU
            : legacy_schema == 1U && !legacy_schema_explicit
                ? 0x06U
                : 0x07U;
    if (legacy_keys != required_keys
        || legacy_compact > 0x7FU
        || legacy_world > 0x3EU
        || (legacy_world & ~0x3EU) != 0U) {
        result.status = VisibilityConfigParseStatus::MissingKey;
        return result;
    }

    if (legacy_schema < 2U && legacy_compact == 0x3FU) {
        legacy_compact |= 0x40U;
    }
    result.settings.radar_clock = (legacy_compact & 0x01U) != 0U;
    result.settings.radar_treasure = (legacy_compact & 0x02U) != 0U;
    result.settings.radar_boss = (legacy_compact & 0x04U) != 0U;
    result.settings.radar_assault = (legacy_compact & 0x08U) != 0U;
    result.settings.radar_mini_games = (legacy_compact & 0x10U) != 0U;
    result.settings.radar_area_quests = (legacy_compact & 0x20U) != 0U;
    result.settings.radar_bird_eggs = (legacy_compact & 0x40U) != 0U;
    result.settings.map_treasure = (legacy_world & 0x02U) != 0U;
    result.settings.map_boss = (legacy_world & 0x04U) != 0U;
    result.settings.map_assault = (legacy_world & 0x08U) != 0U;
    result.settings.map_mini_games = (legacy_world & 0x10U) != 0U;
    result.settings.map_area_quests = (legacy_world & 0x20U) != 0U;
    result.settings.area_quest_mode = legacy_area_mode;
    result.settings.assault_mode = legacy_assault_mode;
    result.status = VisibilityConfigParseStatus::Success;
    result.format = legacy_schema == 1U
        ? VisibilityConfigFormat::LegacySchema1
        : legacy_schema == 2U
            ? VisibilityConfigFormat::LegacySchema2
            : legacy_schema == 3U
                ? VisibilityConfigFormat::LegacySchema3
                : VisibilityConfigFormat::LegacySchema4;
    return result;
}

[[nodiscard]] constexpr std::uint8_t compact_visibility_mask(
    const VisibilityConfigSettings& settings) noexcept {
    return static_cast<std::uint8_t>(
        (settings.radar_clock ? 0x01U : 0U)
        | (settings.radar_treasure ? 0x02U : 0U)
        | (settings.radar_boss ? 0x04U : 0U)
        | (settings.radar_assault ? 0x08U : 0U)
        | (settings.radar_mini_games ? 0x10U : 0U)
        | (settings.radar_area_quests ? 0x20U : 0U)
        | (settings.radar_bird_eggs ? 0x40U : 0U));
}

[[nodiscard]] constexpr std::uint8_t world_visibility_mask(
    const VisibilityConfigSettings& settings) noexcept {
    return static_cast<std::uint8_t>(
        (settings.map_treasure ? 0x02U : 0U)
        | (settings.map_boss ? 0x04U : 0U)
        | (settings.map_assault ? 0x08U : 0U)
        | (settings.map_mini_games ? 0x10U : 0U)
        | (settings.map_area_quests ? 0x20U : 0U));
}

[[nodiscard]] inline std::string format_visibility_config(
    const VisibilityConfigSettings& settings) {
    const auto boolean = [](bool value) noexcept {
        return value ? "true" : "false";
    };
    std::string output;
    output.reserve(768U);
    output += "# Radar visibility. Press F6 to change these options in game.\n";
    output += "# Changes apply immediately and are saved to this file.\n\n";
    output += "[radar]\n";
    output += "clock=";
    output += boolean(settings.radar_clock);
    output += "\ntreasure=";
    output += boolean(settings.radar_treasure);
    output += "\nboss=";
    output += boolean(settings.radar_boss);
    output += "\nassault=";
    output += boolean(settings.radar_assault);
    output += "\nmini_games=";
    output += boolean(settings.radar_mini_games);
    output += "\narea_quests=";
    output += boolean(settings.radar_area_quests);
    output += "\nbird_eggs=";
    output += boolean(settings.radar_bird_eggs);
    output += "\n\n[map]\n";
    output += "treasure=";
    output += boolean(settings.map_treasure);
    output += "\nboss=";
    output += boolean(settings.map_boss);
    output += "\nassault=";
    output += boolean(settings.map_assault);
    output += "\nmini_games=";
    output += boolean(settings.map_mini_games);
    output += "\narea_quests=";
    output += boolean(settings.map_area_quests);
    output += "\n\n[modes]\n";
    output += "# available: show only currently eligible entries; all: show every unfinished entry.\n";
    output += "area_quests=";
    output += settings.area_quest_mode == VisibilityAreaQuestMode::All
        ? "all"
        : "available";
    output += "\nassault=";
    output += settings.assault_mode == VisibilityAssaultMode::All
        ? "all"
        : "available";
    output += "\n\n[height_arrows]\n";
    output += "# Compact-radar height indicators. These do not hide markers.\n";
    output += "treasure=";
    output += boolean(settings.height_treasure);
    output += "\narea_quests=";
    output += boolean(settings.height_area_quests);
    output += "\nmole=";
    output += boolean(settings.height_mole);
    output += "\n\n[interface]\n";
    output += "# auto follows the game's text language on each F7 activation\n";
    output += "# and each actual F6 opening.\n";
    output += "language=";
    output += radar_language_preference_id(settings.language);
    output += '\n';
    return output;
}

[[nodiscard]] constexpr const char* visibility_config_format_name(
    VisibilityConfigFormat format) noexcept {
    switch (format) {
    case VisibilityConfigFormat::Sectioned:
        return "sectioned";
    case VisibilityConfigFormat::LegacySchema1:
        return "legacy_schema_1";
    case VisibilityConfigFormat::LegacySchema2:
        return "legacy_schema_2";
    case VisibilityConfigFormat::LegacySchema3:
        return "legacy_schema_3";
    case VisibilityConfigFormat::LegacySchema4:
        return "legacy_schema_4";
    }
    return "unknown";
}

[[nodiscard]] constexpr const char* visibility_config_status_name(
    VisibilityConfigParseStatus status) noexcept {
    switch (status) {
    case VisibilityConfigParseStatus::Success:
        return "success";
    case VisibilityConfigParseStatus::Empty:
        return "empty_or_missing";
    case VisibilityConfigParseStatus::TooLarge:
        return "too_large";
    case VisibilityConfigParseStatus::Utf8Bom:
        return "utf8_bom";
    case VisibilityConfigParseStatus::InvalidLineEnding:
        return "invalid_line_ending";
    case VisibilityConfigParseStatus::InvalidSection:
        return "invalid_section";
    case VisibilityConfigParseStatus::DuplicateSection:
        return "duplicate_section";
    case VisibilityConfigParseStatus::KeyOutsideSection:
        return "key_outside_section";
    case VisibilityConfigParseStatus::MixedFormats:
        return "mixed_formats";
    case VisibilityConfigParseStatus::UnknownKey:
        return "unknown_key";
    case VisibilityConfigParseStatus::DuplicateKey:
        return "duplicate_key";
    case VisibilityConfigParseStatus::InvalidValue:
        return "invalid_value";
    case VisibilityConfigParseStatus::MissingKey:
        return "missing_or_incompatible_key";
    case VisibilityConfigParseStatus::UnsupportedLegacySchema:
        return "unsupported_legacy_schema";
    }
    return "unknown";
}

} // namespace dswros
