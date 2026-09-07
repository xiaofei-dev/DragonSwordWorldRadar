#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace dswros {

enum class HeightIndicatorCategory : std::uint8_t {
    Treasure,
    AreaQuest,
    Mole,
    Count,
};

using HeightIndicatorMask = std::uint8_t;

inline constexpr HeightIndicatorMask kHeightIndicatorTreasure = 0x01U;
inline constexpr HeightIndicatorMask kHeightIndicatorAreaQuest = 0x02U;
inline constexpr HeightIndicatorMask kHeightIndicatorMole = 0x04U;
inline constexpr HeightIndicatorMask kHeightIndicatorAll = 0x07U;
inline constexpr HeightIndicatorMask kDefaultHeightIndicatorMask =
    kHeightIndicatorAll;

[[nodiscard]] constexpr HeightIndicatorMask height_indicator_bit(
    HeightIndicatorCategory category) noexcept {
    return static_cast<HeightIndicatorMask>(
        1U << static_cast<std::uint8_t>(category));
}

[[nodiscard]] constexpr bool height_indicator_enabled(
    HeightIndicatorMask mask,
    HeightIndicatorCategory category) noexcept {
    return (mask & height_indicator_bit(category)) != 0U;
}

enum class RadarLanguagePreference : std::uint8_t {
    Auto,
    English,
    Japanese,
    Korean,
    SimplifiedChinese,
    TraditionalChinese,
    French,
    German,
    SpanishSpain,
    Russian,
    Thai,
    PortugueseBrazil,
    Count,
};

enum class RadarUiLanguage : std::uint8_t {
    English,
    Japanese,
    Korean,
    SimplifiedChinese,
    TraditionalChinese,
    French,
    German,
    SpanishSpain,
    Russian,
    Thai,
    PortugueseBrazil,
    Count,
};

// The game ships four system composite-font assets. Keep this pure mapping
// separate from the F6 UObject lookup so language selection remains testable
// without a live engine. Korean and all Latin/Cyrillic languages use the
// Common system font; both Chinese variants share TCSystem.
enum class RadarUiFontFamily : std::uint8_t {
    Common,
    TraditionalChinese,
    Japanese,
    Thai,
    Count,
};

inline constexpr std::size_t kRadarLanguagePreferenceCount =
    static_cast<std::size_t>(RadarLanguagePreference::Count);
inline constexpr std::size_t kRadarUiLanguageCount =
    static_cast<std::size_t>(RadarUiLanguage::Count);

[[nodiscard]] constexpr RadarUiFontFamily radar_ui_font_family(
    RadarUiLanguage language) noexcept {
    switch (language) {
    case RadarUiLanguage::SimplifiedChinese:
    case RadarUiLanguage::TraditionalChinese:
        return RadarUiFontFamily::TraditionalChinese;
    case RadarUiLanguage::Japanese:
        return RadarUiFontFamily::Japanese;
    case RadarUiLanguage::Thai:
        return RadarUiFontFamily::Thai;
    case RadarUiLanguage::English:
    case RadarUiLanguage::Korean:
    case RadarUiLanguage::French:
    case RadarUiLanguage::German:
    case RadarUiLanguage::SpanishSpain:
    case RadarUiLanguage::Russian:
    case RadarUiLanguage::PortugueseBrazil:
    case RadarUiLanguage::Count:
        return RadarUiFontFamily::Common;
    }
    return RadarUiFontFamily::Common;
}

[[nodiscard]] constexpr std::string_view radar_ui_language_id(
    RadarUiLanguage language) noexcept {
    switch (language) {
    case RadarUiLanguage::English: return "en";
    case RadarUiLanguage::Japanese: return "ja";
    case RadarUiLanguage::Korean: return "ko";
    case RadarUiLanguage::SimplifiedChinese: return "zh-hans";
    case RadarUiLanguage::TraditionalChinese: return "zh-hant";
    case RadarUiLanguage::French: return "fr";
    case RadarUiLanguage::German: return "de";
    case RadarUiLanguage::SpanishSpain: return "es-es";
    case RadarUiLanguage::Russian: return "ru";
    case RadarUiLanguage::Thai: return "th";
    case RadarUiLanguage::PortugueseBrazil: return "pt-br";
    case RadarUiLanguage::Count: break;
    }
    return "en";
}

[[nodiscard]] constexpr std::string_view radar_language_preference_id(
    RadarLanguagePreference language) noexcept {
    switch (language) {
    case RadarLanguagePreference::Auto: return "auto";
    case RadarLanguagePreference::English: return "en";
    case RadarLanguagePreference::Japanese: return "ja";
    case RadarLanguagePreference::Korean: return "ko";
    case RadarLanguagePreference::SimplifiedChinese: return "zh-hans";
    case RadarLanguagePreference::TraditionalChinese: return "zh-hant";
    case RadarLanguagePreference::French: return "fr";
    case RadarLanguagePreference::German: return "de";
    case RadarLanguagePreference::SpanishSpain: return "es-es";
    case RadarLanguagePreference::Russian: return "ru";
    case RadarLanguagePreference::Thai: return "th";
    case RadarLanguagePreference::PortugueseBrazil: return "pt-br";
    case RadarLanguagePreference::Count: break;
    }
    return {};
}

[[nodiscard]] constexpr bool parse_radar_language_preference(
    std::string_view value,
    RadarLanguagePreference& output) noexcept {
    for (std::size_t index = 0; index < kRadarLanguagePreferenceCount;
         ++index) {
        const auto candidate = static_cast<RadarLanguagePreference>(index);
        if (value == radar_language_preference_id(candidate)) {
            output = candidate;
            return true;
        }
    }
    return false;
}

[[nodiscard]] constexpr RadarUiLanguage explicit_radar_ui_language(
    RadarLanguagePreference preference) noexcept {
    switch (preference) {
    case RadarLanguagePreference::Japanese:
        return RadarUiLanguage::Japanese;
    case RadarLanguagePreference::Korean:
        return RadarUiLanguage::Korean;
    case RadarLanguagePreference::SimplifiedChinese:
        return RadarUiLanguage::SimplifiedChinese;
    case RadarLanguagePreference::TraditionalChinese:
        return RadarUiLanguage::TraditionalChinese;
    case RadarLanguagePreference::French:
        return RadarUiLanguage::French;
    case RadarLanguagePreference::German:
        return RadarUiLanguage::German;
    case RadarLanguagePreference::SpanishSpain:
        return RadarUiLanguage::SpanishSpain;
    case RadarLanguagePreference::Russian:
        return RadarUiLanguage::Russian;
    case RadarLanguagePreference::Thai:
        return RadarUiLanguage::Thai;
    case RadarLanguagePreference::PortugueseBrazil:
        return RadarUiLanguage::PortugueseBrazil;
    case RadarLanguagePreference::Auto:
    case RadarLanguagePreference::English:
    case RadarLanguagePreference::Count:
        return RadarUiLanguage::English;
    }
    return RadarUiLanguage::English;
}

// Converts an explicit language into its persisted manual preference.
// Follow-game mode is a separate choice, never a detected language to persist.
[[nodiscard]] constexpr RadarLanguagePreference
explicit_radar_language_preference(RadarUiLanguage language) noexcept {
    switch (language) {
    case RadarUiLanguage::Japanese:
        return RadarLanguagePreference::Japanese;
    case RadarUiLanguage::Korean:
        return RadarLanguagePreference::Korean;
    case RadarUiLanguage::SimplifiedChinese:
        return RadarLanguagePreference::SimplifiedChinese;
    case RadarUiLanguage::TraditionalChinese:
        return RadarLanguagePreference::TraditionalChinese;
    case RadarUiLanguage::French:
        return RadarLanguagePreference::French;
    case RadarUiLanguage::German:
        return RadarLanguagePreference::German;
    case RadarUiLanguage::SpanishSpain:
        return RadarLanguagePreference::SpanishSpain;
    case RadarUiLanguage::Russian:
        return RadarLanguagePreference::Russian;
    case RadarUiLanguage::Thai:
        return RadarLanguagePreference::Thai;
    case RadarUiLanguage::PortugueseBrazil:
        return RadarLanguagePreference::PortugueseBrazil;
    case RadarUiLanguage::English:
    case RadarUiLanguage::Count:
        return RadarLanguagePreference::English;
    }
    return RadarLanguagePreference::English;
}

// AUTO is the first persistent preference, followed by the eleven languages.
// Popup labels, hit targets, highlights and raster overlays share this order.
[[nodiscard]] constexpr RadarLanguagePreference radar_language_choice(
    std::size_t index) noexcept {
    return index < kRadarLanguagePreferenceCount
        ? static_cast<RadarLanguagePreference>(index)
        : RadarLanguagePreference::Auto;
}

[[nodiscard]] constexpr std::size_t radar_language_choice_index(
    RadarLanguagePreference preference) noexcept {
    return static_cast<std::size_t>(preference) < kRadarLanguagePreferenceCount
        ? static_cast<std::size_t>(preference) : 0;
}

[[nodiscard]] constexpr RadarUiLanguage retain_detected_radar_language(
    RadarUiLanguage previous, RadarUiLanguage sample) noexcept {
    if (static_cast<std::size_t>(sample) < kRadarUiLanguageCount) {
        return sample;
    }
    return static_cast<std::size_t>(previous) < kRadarUiLanguageCount
        ? previous : RadarUiLanguage::English;
}

[[nodiscard]] constexpr RadarUiLanguage resolve_radar_ui_language(
    RadarLanguagePreference preference,
    RadarUiLanguage detected_game_language) noexcept {
    if (preference == RadarLanguagePreference::Auto) {
        return static_cast<std::size_t>(detected_game_language)
                < kRadarUiLanguageCount
            ? detected_game_language
            : RadarUiLanguage::English;
    }
    return explicit_radar_ui_language(preference);
}

// DGameUserSettings.LanguageText stores EBILanguageType as an integer. Keep
// this mapping independent from RadarUiLanguage's display ordering so a game
// update cannot silently turn Japanese into Korean (or vice versa).
[[nodiscard]] constexpr bool radar_ui_language_from_game_setting(
    std::uint64_t value,
    RadarUiLanguage& output) noexcept {
    switch (value) {
    case 0U: output = RadarUiLanguage::Korean; return true;
    case 1U: output = RadarUiLanguage::English; return true;
    case 2U: output = RadarUiLanguage::Japanese; return true;
    case 3U: output = RadarUiLanguage::SimplifiedChinese; return true;
    case 4U: output = RadarUiLanguage::TraditionalChinese; return true;
    case 5U: output = RadarUiLanguage::German; return true;
    case 6U: output = RadarUiLanguage::French; return true;
    case 7U: output = RadarUiLanguage::SpanishSpain; return true;
    case 8U: output = RadarUiLanguage::PortugueseBrazil; return true;
    case 9U: output = RadarUiLanguage::Russian; return true;
    case 10U: output = RadarUiLanguage::Thai; return true;
    default: return false;
    }
}

[[nodiscard]] constexpr wchar_t ascii_lower(wchar_t value) noexcept {
    return value >= L'A' && value <= L'Z'
        ? static_cast<wchar_t>(value + (L'a' - L'A'))
        : value;
}

[[nodiscard]] constexpr bool culture_tag_starts_with(
    std::wstring_view culture,
    std::wstring_view prefix) noexcept {
    if (culture.size() < prefix.size()) {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        const wchar_t left = culture[index] == L'_'
            ? L'-' : ascii_lower(culture[index]);
        const wchar_t right = prefix[index] == L'_'
            ? L'-' : ascii_lower(prefix[index]);
        if (left != right) {
            return false;
        }
    }
    return culture.size() == prefix.size()
        || culture[prefix.size()] == L'-'
        || culture[prefix.size()] == L'_';
}

// Maps the culture tag returned by UE's internationalization library to the
// exact interface-language set shipped by the game. Unsupported, malformed,
// and empty values are unknown, not an authoritative English selection.
[[nodiscard]] constexpr RadarUiLanguage sample_radar_ui_language_from_culture(
    std::wstring_view culture) noexcept {
    if (culture_tag_starts_with(culture, L"ja")) {
        return RadarUiLanguage::Japanese;
    }
    if (culture_tag_starts_with(culture, L"ko")) {
        return RadarUiLanguage::Korean;
    }
    if (culture_tag_starts_with(culture, L"zh-hant")
        || culture_tag_starts_with(culture, L"zh-tw")
        || culture_tag_starts_with(culture, L"zh-hk")
        || culture_tag_starts_with(culture, L"zh-mo")) {
        return RadarUiLanguage::TraditionalChinese;
    }
    if (culture_tag_starts_with(culture, L"zh")) {
        return RadarUiLanguage::SimplifiedChinese;
    }
    if (culture_tag_starts_with(culture, L"fr")) {
        return RadarUiLanguage::French;
    }
    if (culture_tag_starts_with(culture, L"de")) {
        return RadarUiLanguage::German;
    }
    if (culture_tag_starts_with(culture, L"es")) {
        return RadarUiLanguage::SpanishSpain;
    }
    if (culture_tag_starts_with(culture, L"ru")) {
        return RadarUiLanguage::Russian;
    }
    if (culture_tag_starts_with(culture, L"th")) {
        return RadarUiLanguage::Thai;
    }
    if (culture_tag_starts_with(culture, L"pt")) {
        return RadarUiLanguage::PortugueseBrazil;
    }
    if (culture_tag_starts_with(culture, L"en")) {
        return RadarUiLanguage::English;
    }
    return RadarUiLanguage::Count;
}

// Stateless callers without a last-known language retain an English fallback.
[[nodiscard]] constexpr RadarUiLanguage radar_ui_language_from_culture(
    std::wstring_view culture) noexcept {
    return retain_detected_radar_language(
        RadarUiLanguage::English, sample_radar_ui_language_from_culture(culture));
}

} // namespace dswros
