#include <dswros/area_quest_visibility.hpp>
#include <dswros/compact_menu_state.hpp>
#include <dswros/compact_render_model.hpp>
#include <dswros/diagnostics_config.hpp>
#include <dswros/diagnostic_log_format.hpp>
#include <dswros/object_state.hpp>
#include <dswros/owner_pointer_pattern.hpp>
#include <dswros/render_projection.hpp>
#include <dswros/radar_localization.hpp>
#include <dswros/radar_visibility_hub_policy.hpp>
#include <dswros/save_key_field_policy.hpp>
#include <dswros/visibility_config.hpp>
#include <dswros/world_map_session_policy.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

std::size_t allocation_count{};
std::size_t assertion_count{};

} // namespace

void* operator new(std::size_t size) {
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        ++allocation_count;
        return memory;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        ++allocation_count;
        return memory;
    }
    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

namespace {

static_assert(
    dswros::LiveMarkerPresenceGate::kMissingDebounceMilliseconds == 400,
    "live runtime markers must retain the 400 ms disappearance debounce");

void require(bool condition, const char* message) {
    ++assertion_count;
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

bool near(double left, double right, double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

const dswros::CompactTreasureMarker* marker_by_id(
    std::span<const dswros::CompactTreasureMarker> markers,
    std::int64_t id) {
    for (const dswros::CompactTreasureMarker& marker : markers) {
        if (marker.id == id) {
            return &marker;
        }
    }
    return nullptr;
}

dswros::ObjectStateTracker tracker() {
    dswros::ObjectStateTracker value;
    value.set_catalog({
        {1001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
        {1002, "TreasureBox02_C", {8000.0, 9000.0, 1000.0}},
    });
    value.reset(7, 11);
    return value;
}

template <typename Value>
void write_pe_value(
    std::vector<std::uint8_t>& image,
    std::size_t offset,
    Value value) {
    require(offset <= image.size() && image.size() - offset >= sizeof(Value),
            "synthetic PE write must stay in bounds");
    std::memcpy(image.data() + offset, &value, sizeof(Value));
}

void write_owner_pointer_pattern(
    std::vector<std::uint8_t>& image,
    std::size_t raw_offset,
    std::uint32_t instruction_rva,
    std::uint32_t target_rva) {
    constexpr std::array<std::uint8_t, 24> pattern{
        0x48, 0x8B, 0x0D, 0, 0, 0, 0,
        0xE8, 0, 0, 0, 0,
        0x8B, 0xC7, 0x48, 0x8B, 0x5C, 0x24,
        0x40, 0x48, 0x8B, 0x6C, 0x24, 0x50};
    require(raw_offset <= image.size()
                && image.size() - raw_offset >= pattern.size(),
            "synthetic owner-pointer pattern must stay in bounds");
    std::memcpy(image.data() + raw_offset, pattern.data(), pattern.size());
    const auto displacement = static_cast<std::int32_t>(
        static_cast<std::int64_t>(target_rva)
        - static_cast<std::int64_t>(instruction_rva + 7U));
    write_pe_value(image, raw_offset + 3U, displacement);
    write_pe_value(image, raw_offset + 8U, std::int32_t{0x10});
}

std::vector<std::uint8_t> synthetic_owner_pointer_pe() {
    std::vector<std::uint8_t> image(0x800U, 0U);
    write_pe_value(image, 0U, std::uint16_t{0x5A4D});
    write_pe_value(image, 0x3CU, std::uint32_t{0x80});
    write_pe_value(image, 0x80U, std::uint32_t{0x00004550});
    write_pe_value(image, 0x84U, std::uint16_t{0x8664});
    write_pe_value(image, 0x86U, std::uint16_t{1});
    write_pe_value(image, 0x94U, std::uint16_t{0xF0});
    write_pe_value(image, 0x96U, std::uint16_t{0x0002});
    write_pe_value(image, 0x98U, std::uint16_t{0x020B});
    write_pe_value(image, 0xD0U, std::uint32_t{0x4000});
    constexpr std::size_t section = 0x188U;
    write_pe_value(image, section + 8U, std::uint32_t{0x400});
    write_pe_value(image, section + 12U, std::uint32_t{0x1000});
    write_pe_value(image, section + 16U, std::uint32_t{0x400});
    write_pe_value(image, section + 20U, std::uint32_t{0x200});
    write_pe_value(image, section + 36U, std::uint32_t{0x60000020});
    write_owner_pointer_pattern(image, 0x240U, 0x1040U, 0x3000U);
    return image;
}

} // namespace

int main() {
    {
        using FontSource = dswros::RadarVisibilityHubFontPlanSource;
        using TextKind = dswros::RadarVisibilityHubTextWidgetKind;
        using Attempt = dswros::RadarVisibilityHubTextPrepareAttempt;

        const std::size_t allocations_before = allocation_count;
        constexpr std::array optional_font_details{
            1U << 5U, 1U << 6U, 1U << 7U, 1U << 8U, 1U << 12U};
        for (const std::uint32_t detail : optional_font_details) {
            require(!dswros::radar_visibility_hub_font_detail_is_fatal(
                        detail),
                    "every optional F6 game-font detail must remain nonfatal");
        }
        require(!dswros::radar_visibility_hub_font_detail_is_fatal(
                    (1U << 5U) | (1U << 6U) | (1U << 7U) | (1U << 8U)
                        | (1U << 12U)),
                "combined optional F6 font details including the reflected layout contract must not disable the Hub");
        constexpr std::array essential_hub_details{
            1U << 0U, 1U << 1U, 1U << 2U, 1U << 3U,
            1U << 4U, 1U << 9U, 1U << 10U, 1U << 11U};
        for (const std::uint32_t detail : essential_hub_details) {
            require(dswros::radar_visibility_hub_font_detail_is_fatal(
                        detail),
                    "every essential F6 widget detail must remain fatal");
        }

        constexpr dswros::RadarVisibilityHubFontPlan inherited_plan =
            dswros::radar_visibility_hub_font_plan(true, false);
        static_assert(inherited_plan.use_game_text_widget);
        static_assert(!inherited_plan.copy_game_default_font);
        static_assert(
            inherited_plan.source == FontSource::DTextBlockInheritedDefault);
        std::array<TextKind, 2> inherited_attempts{};
        std::size_t inherited_attempt_count{};
        const auto inherited_result =
            dswros::prepare_radar_visibility_hub_text_with_fallback(
                inherited_plan.use_game_text_widget,
                [&inherited_attempts, &inherited_attempt_count](TextKind kind) {
                    inherited_attempts[inherited_attempt_count++] = kind;
                    return Attempt{
                        kind == TextKind::GameTextBlock,
                        kind == TextKind::GameTextBlock ? 0U : 7U};
                });
        require(inherited_result.prepared
                    && inherited_result.selected == TextKind::GameTextBlock
                    && inherited_result.game_attempted
                    && !inherited_result.base_attempted
                    && inherited_attempt_count == 1U
                    && inherited_attempts[0] == TextKind::GameTextBlock,
                "an inherited DTextBlock font must be allowed to prepare without a compatible CDO font");

        constexpr dswros::RadarVisibilityHubFontPlan cdo_plan =
            dswros::radar_visibility_hub_font_plan(true, true);
        require(cdo_plan.use_game_text_widget
                    && cdo_plan.copy_game_default_font
                    && cdo_plan.source
                        == FontSource::DTextBlockClassDefaultObject,
                "a compatible DTextBlock CDO font must select and copy the CDO source");

        std::array<TextKind, 2> base_only_attempts{};
        std::size_t base_only_attempt_count{};
        const auto base_only_result =
            dswros::prepare_radar_visibility_hub_text_with_fallback(
                false,
                [&base_only_attempts, &base_only_attempt_count](TextKind kind) {
                    base_only_attempts[base_only_attempt_count++] = kind;
                    return Attempt{
                        kind == TextKind::BaseTextBlock,
                        kind == TextKind::BaseTextBlock ? 0U : 1U};
                });
        require(base_only_result.prepared
                    && base_only_result.selected == TextKind::BaseTextBlock
                    && !base_only_result.game_attempted
                    && base_only_result.base_attempted
                    && base_only_attempt_count == 1U
                    && base_only_attempts[0] == TextKind::BaseTextBlock,
                "a disabled game-font path must attempt only the base TextBlock");

        std::array<TextKind, 2> fallback_attempts{};
        std::size_t fallback_attempt_count{};
        const auto fallback_result =
            dswros::prepare_radar_visibility_hub_text_with_fallback(
                true,
                [&fallback_attempts, &fallback_attempt_count](TextKind kind) {
                    fallback_attempts[fallback_attempt_count++] = kind;
                    return kind == TextKind::GameTextBlock
                        ? Attempt{false, 3U}
                        : Attempt{true, 0U};
                });
        require(fallback_result.prepared
                    && fallback_result.selected == TextKind::BaseTextBlock
                    && fallback_result.game_attempted
                    && fallback_result.base_attempted
                    && fallback_result.fallback_reason == 3U
                    && fallback_result.terminal_failure == 0U
                    && fallback_attempt_count == 2U
                    && fallback_attempts[0] == TextKind::GameTextBlock
                    && fallback_attempts[1] == TextKind::BaseTextBlock,
                "a failed game DTextBlock must retry the base TextBlock in order and preserve its fallback reason");

        std::array<TextKind, 2> rejected_attempts{};
        std::size_t rejected_attempt_count{};
        const auto rejected_result =
            dswros::prepare_radar_visibility_hub_text_with_fallback(
                true,
                [&rejected_attempts, &rejected_attempt_count](TextKind kind) {
                    rejected_attempts[rejected_attempt_count++] = kind;
                    return kind == TextKind::GameTextBlock
                        ? Attempt{false, 2U}
                        : Attempt{false, 6U};
                });
        require(!rejected_result.prepared
                    && rejected_result.selected == TextKind::BaseTextBlock
                    && rejected_result.game_attempted
                    && rejected_result.base_attempted
                    && rejected_result.fallback_reason == 2U
                    && rejected_result.terminal_failure == 6U
                    && rejected_attempt_count == 2U
                    && rejected_attempts[0] == TextKind::GameTextBlock
                    && rejected_attempts[1] == TextKind::BaseTextBlock,
                "F6 text preparation may reject only after both game and base TextBlocks fail");

        constexpr auto centered_line =
            dswros::center_radar_visibility_hub_text_slot(
                100.0, 26.0, 18.0);
        static_assert(centered_line.centered);
        static_assert(centered_line.top == 104.0);
        static_assert(centered_line.height == 18.0);
        require(centered_line.centered
                    && near(centered_line.top, 104.0)
                    && near(centered_line.height, 18.0)
                    && near(
                        centered_line.top + centered_line.height * 0.5,
                        100.0 + 26.0 * 0.5),
                "F6 text must preserve the authored vertical center while shrinking to its real Slate line height");
        const auto scaled_centered_line =
            dswros::center_radar_visibility_hub_text_slot(
                150.0, 39.0, 27.0);
        require(scaled_centered_line.centered
                    && near(scaled_centered_line.top, 156.0)
                    && near(scaled_centered_line.height, 27.0),
                "F6 desired-size centering must remain scale independent");
        const auto oversized_line =
            dswros::center_radar_visibility_hub_text_slot(
                100.0, 26.0, 27.0);
        require(!oversized_line.centered
                    && near(oversized_line.top, 100.0)
                    && near(oversized_line.height, 26.0),
                "an oversized desired line must preserve the authored F6 slot geometry");
        const std::array invalid_desired_heights{
            0.0,
            -1.0,
            std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN(),
        };
        for (const double desired_height : invalid_desired_heights) {
            const auto invalid_line =
                dswros::center_radar_visibility_hub_text_slot(
                    100.0, 26.0, desired_height);
            require(!invalid_line.centered
                        && near(invalid_line.top, 100.0)
                        && near(invalid_line.height, 26.0),
                    "an invalid desired line height must preserve the authored F6 slot geometry");
        }
        const auto invalid_authored_line =
            dswros::center_radar_visibility_hub_text_slot(
                100.0, 0.0, 18.0);
        require(!invalid_authored_line.centered
                    && near(invalid_authored_line.top, 100.0)
                    && near(invalid_authored_line.height, 0.0),
                "an invalid authored line box must remain unchanged");
        require(allocation_count == allocations_before,
                "F6 text preparation and centering policy checks must not allocate");
    }
    {
        constexpr std::string_view current_config{
            "# Public defaults\n"
            "[radar]\n"
            "clock=true\n"
            "treasure=true\n"
            "boss=true\n"
            "assault=true\n"
            "mini_games=true\n"
            "area_quests=true\n"
            "bird_eggs=true\n"
            "[map]\n"
            "treasure=true\n"
            "boss=true\n"
            "assault=true\n"
            "mini_games=true\n"
            "area_quests=true\n"
            "[modes]\n"
            "area_quests=available\n"
            "assault=available\n"};
        const std::size_t allocations_before = allocation_count;
        const auto current = dswros::parse_visibility_config(current_config);
        require(current
                    && current.format
                        == dswros::VisibilityConfigFormat::Sectioned
                    && dswros::compact_visibility_mask(current.settings)
                        == 0x7FU
                    && dswros::world_visibility_mask(current.settings)
                        == 0x3EU
                    && current.settings.height_treasure
                    && current.settings.height_area_quests
                    && current.settings.height_mole
                    && current.settings.language
                        == dswros::RadarLanguagePreference::Auto,
                "the 2.1.1 sectioned visibility defaults must migrate exactly");
        require(allocation_count == allocations_before,
                "visibility parsing must not allocate");

        dswros::VisibilityConfigSettings customized{};
        customized.radar_clock = false;
        customized.radar_assault = false;
        customized.map_boss = false;
        customized.area_quest_mode =
            dswros::VisibilityAreaQuestMode::All;
        customized.assault_mode = dswros::VisibilityAssaultMode::All;
        customized.height_treasure = false;
        customized.height_area_quests = true;
        customized.height_mole = true;
        customized.language = dswros::RadarLanguagePreference::Thai;
        const std::string serialized =
            dswros::format_visibility_config(customized);
        const auto round_trip =
            dswros::parse_visibility_config(serialized);
        require(round_trip
                    && !round_trip.settings.radar_clock
                    && !round_trip.settings.radar_assault
                    && !round_trip.settings.map_boss
                    && round_trip.settings.area_quest_mode
                        == dswros::VisibilityAreaQuestMode::All
                    && round_trip.settings.assault_mode
                        == dswros::VisibilityAssaultMode::All
                    && !round_trip.settings.height_treasure
                    && round_trip.settings.height_area_quests
                    && round_trip.settings.height_mole
                    && round_trip.settings.language
                        == dswros::RadarLanguagePreference::Thai,
                "F6 visibility, height, and language output must round-trip");

        for (std::size_t index = 0;
             index < dswros::kRadarLanguagePreferenceCount; ++index) {
            const auto expected =
                static_cast<dswros::RadarLanguagePreference>(index);
            dswros::RadarLanguagePreference parsed{};
            require(dswros::parse_radar_language_preference(
                        dswros::radar_language_preference_id(expected),
                        parsed)
                        && parsed == expected,
                    "every public language ID must parse exactly");
        }
        require(dswros::resolve_radar_ui_language(
                    dswros::RadarLanguagePreference::Auto,
                    dswros::RadarUiLanguage::Russian)
                    == dswros::RadarUiLanguage::Russian
                    && dswros::resolve_radar_ui_language(
                        dswros::RadarLanguagePreference::PortugueseBrazil,
                        dswros::RadarUiLanguage::Japanese)
                        == dswros::RadarUiLanguage::PortugueseBrazil,
                "AUTO must follow the game while a manual language overrides it");
        for (std::size_t index = 0;
             index < dswros::kRadarUiLanguageCount; ++index) {
            const auto ui_language =
                static_cast<dswros::RadarUiLanguage>(index);
            const auto manual_preference =
                dswros::explicit_radar_language_preference(ui_language);
            require(
                manual_preference
                        != dswros::RadarLanguagePreference::Auto
                    && dswros::explicit_radar_ui_language(
                           manual_preference)
                        == ui_language,
                "every visible language choice must map to one explicit persisted preference");
        }
        require(
            dswros::radar_ui_font_family(
                dswros::RadarUiLanguage::SimplifiedChinese)
                    == dswros::RadarUiFontFamily::TraditionalChinese
                && dswros::radar_ui_font_family(
                    dswros::RadarUiLanguage::TraditionalChinese)
                    == dswros::RadarUiFontFamily::TraditionalChinese
                && dswros::radar_ui_font_family(
                    dswros::RadarUiLanguage::Japanese)
                    == dswros::RadarUiFontFamily::Japanese
                && dswros::radar_ui_font_family(
                    dswros::RadarUiLanguage::Thai)
                    == dswros::RadarUiFontFamily::Thai
                && dswros::radar_ui_font_family(
                    dswros::RadarUiLanguage::Korean)
                    == dswros::RadarUiFontFamily::Common
                && dswros::radar_ui_font_family(
                    dswros::RadarUiLanguage::SpanishSpain)
                    == dswros::RadarUiFontFamily::Common,
            "visible languages must select the game's four proven system-font families");
        require(
            dswros::resolve_explicit_radar_language_preference(
                dswros::RadarLanguagePreference::Auto,
                dswros::RadarUiLanguage::Korean)
                    == dswros::RadarLanguagePreference::Korean
                && dswros::resolve_explicit_radar_language_preference(
                    dswros::RadarLanguagePreference::Thai,
                    dswros::RadarUiLanguage::English)
                    == dswros::RadarLanguagePreference::Thai,
            "legacy AUTO must migrate to the detected explicit language while manual choices remain authoritative");
        constexpr std::array expected_game_languages{
            dswros::RadarUiLanguage::Korean,
            dswros::RadarUiLanguage::English,
            dswros::RadarUiLanguage::Japanese,
            dswros::RadarUiLanguage::SimplifiedChinese,
            dswros::RadarUiLanguage::TraditionalChinese,
            dswros::RadarUiLanguage::German,
            dswros::RadarUiLanguage::French,
            dswros::RadarUiLanguage::SpanishSpain,
            dswros::RadarUiLanguage::PortugueseBrazil,
            dswros::RadarUiLanguage::Russian,
            dswros::RadarUiLanguage::Thai,
        };
        for (std::size_t index = 0;
             index < expected_game_languages.size(); ++index) {
            dswros::RadarUiLanguage mapped{};
            require(dswros::radar_ui_language_from_game_setting(
                        index, mapped)
                        && mapped == expected_game_languages[index],
                    "every EBILanguageType LanguageText value must map exactly");
        }
        dswros::RadarUiLanguage unsupported_game_language{};
        require(!dswros::radar_ui_language_from_game_setting(
                    11U, unsupported_game_language),
                "invalid LanguageText values must use the culture fallback");
        require(
            dswros::radar_ui_language_from_culture(L"en-US")
                    == dswros::RadarUiLanguage::English
                && dswros::radar_ui_language_from_culture(L"ja-JP")
                    == dswros::RadarUiLanguage::Japanese
                && dswros::radar_ui_language_from_culture(L"ko_KR")
                    == dswros::RadarUiLanguage::Korean
                && dswros::radar_ui_language_from_culture(L"zh-Hans-CN")
                    == dswros::RadarUiLanguage::SimplifiedChinese
                && dswros::radar_ui_language_from_culture(L"zh-TW")
                    == dswros::RadarUiLanguage::TraditionalChinese
                && dswros::radar_ui_language_from_culture(L"fr-FR")
                    == dswros::RadarUiLanguage::French
                && dswros::radar_ui_language_from_culture(L"de-DE")
                    == dswros::RadarUiLanguage::German
                && dswros::radar_ui_language_from_culture(L"es-ES")
                    == dswros::RadarUiLanguage::SpanishSpain
                && dswros::radar_ui_language_from_culture(L"ru-RU")
                    == dswros::RadarUiLanguage::Russian
                && dswros::radar_ui_language_from_culture(L"th-TH")
                    == dswros::RadarUiLanguage::Thai
                && dswros::radar_ui_language_from_culture(L"pt-BR")
                    == dswros::RadarUiLanguage::PortugueseBrazil
                && dswros::radar_ui_language_from_culture(L"unsupported")
                    == dswros::RadarUiLanguage::English,
            "game culture tags must map to all supported UI languages and fall back to English");
        for (std::size_t index = 0;
             index < dswros::kRadarUiLanguageCount; ++index) {
            const auto& text = dswros::radar_localized_text(
                static_cast<dswros::RadarUiLanguage>(index));
            const auto present = [](const wchar_t* value) noexcept {
                return value && value[0] != L'\0';
            };
            require(present(text.language_name) && present(text.title)
                        && present(text.language)
                        && present(text.automatic)
                        && present(text.marker_visibility)
                        && present(text.radar) && present(text.map)
                        && present(text.height_indicators)
                        && present(text.radar_only)
                        && present(text.filter_modes)
                        && present(text.available) && present(text.all)
                        && present(text.close) && present(text.status)
                        && present(text.status_off)
                        && present(text.status_on)
                        && present(text.status_fault)
                        && present(text.enable_mod)
                        && present(text.disable_mod)
                        && present(text.retry_mod)
                        && present(text.bug_report),
                    "every supported UI language must define all shared labels");
            for (const wchar_t* category : text.marker_categories) {
                require(present(category),
                        "every language must define all marker category labels");
            }
            for (const wchar_t* category : text.height_categories) {
                require(present(category),
                        "every language must define all height category labels");
            }
        }
        require(
            dswros::kRadarUiLanguageCount + 1U
                    == dswros::kRadarLanguagePreferenceCount
                && std::wstring_view(dswros::radar_localized_text(
                    dswros::RadarUiLanguage::SimplifiedChinese)
                        .language_name)
                    == L"简体中文",
            "the F6 selector must expose only the eleven explicit languages and exclude the legacy AUTO value");
        const auto& english_controls = dswros::radar_localized_text(
            dswros::RadarUiLanguage::English);
        require(
            std::wstring_view(english_controls.status_off) == L"OFF"
                && std::wstring_view(english_controls.status_on) == L"ON"
                && std::wstring_view(english_controls.status_fault)
                    == L"FAULT"
                && std::wstring_view(english_controls.enable_mod)
                    == L"ENABLE"
                && std::wstring_view(english_controls.disable_mod)
                    == L"DISABLE"
                && std::wstring_view(english_controls.retry_mod)
                    == L"RETRY"
                && std::wstring_view(english_controls.bug_report)
                    == L"BUG REPORT",
            "the F6 status card must expose complete OFF/ON/FAULT controls and a report action");

        const auto legacy1 = dswros::parse_visibility_config(
            "; schema 1\ncompact_mask=63\nworld_mask=62\n");
        require(legacy1
                    && legacy1.format
                        == dswros::VisibilityConfigFormat::LegacySchema1
                    && dswros::compact_visibility_mask(legacy1.settings)
                        == 0x7FU,
                "schema 1 visibility must migrate the bird-egg default");
        const auto legacy2 = dswros::parse_visibility_config(
            "schema_version=2\ncompact_mask=5\nworld_mask=6\n");
        require(legacy2
                    && legacy2.format
                        == dswros::VisibilityConfigFormat::LegacySchema2
                    && dswros::compact_visibility_mask(legacy2.settings) == 5U
                    && dswros::world_visibility_mask(legacy2.settings) == 6U,
                "schema 2 visibility masks must remain readable");
        const auto legacy3 = dswros::parse_visibility_config(
            "schema_version=3\ncompact_mask=7\nworld_mask=10\n"
            "area_quest_mode=all\n");
        require(legacy3
                    && legacy3.settings.area_quest_mode
                        == dswros::VisibilityAreaQuestMode::All,
                "schema 3 area-quest mode must remain readable");
        const auto legacy4 = dswros::parse_visibility_config(
            "schema_version=4\ncompact_mask=127\nworld_mask=62\n"
            "area_quest_mode=available\nassault_mode=current\n");
        require(legacy4
                    && legacy4.format
                        == dswros::VisibilityConfigFormat::LegacySchema4
                    && legacy4.settings.assault_mode
                        == dswros::VisibilityAssaultMode::Available,
                "schema 4 current assault mode must remain readable");

        require(!dswros::parse_visibility_config(
                    "[radar]\nclock=true\n"),
                "incomplete sectioned visibility must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config} + "clock=true\n"),
                "a key in the wrong section must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config} + "[unknown]\n"),
                "unknown visibility sections must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config}
                        + "[height_arrows]\ntreasure=true\n"
                          "area_quests=false\nmole=false\n"),
                "one new section without interface settings must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config}
                        + "[height_arrows]\ntreasure=true\n"
                          "area_quests=false\nmole=false\n"
                          "[interface]\nlanguage=unknown\n"),
                "unknown interface languages must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config}
                        + "[height_arrows]\ntreasure=true\n"
                          "area_quests=false\n"
                          "[interface]\nlanguage=auto\n"),
                "incomplete height settings must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\nclock=TRUE\n"),
                "non-canonical visibility booleans must fail closed");
        require(!dswros::parse_visibility_config(
                    "compact_mask=127\nworld_mask=62\n[radar]\n"),
                "mixed legacy and sectioned visibility must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=5\ncompact_mask=127\nworld_mask=62\n"),
                "unsupported legacy visibility schemas must fail closed");
        require(!dswros::parse_visibility_config(
                    "SCHEMA_VERSION=4\ncompact_mask=127\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "uppercase legacy visibility keys must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=4\nCOMPACT_MASK=127\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "mixed-case legacy visibility keys must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=4\ncompact_mask=128\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "out-of-range legacy masks must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\rclock=true\n"),
                "bare carriage returns must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=1\ncompact_mask=63\nworld_mask=62\r"),
                "a trailing bare carriage return must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\r\nclock=true\n"),
                "mixed visibility line endings must fail closed");
        const auto bom_legacy = dswros::parse_visibility_config(
            "\xEF\xBB\xBFschema_version=4\ncompact_mask=127\n"
            "world_mask=62\narea_quest_mode=available\n"
            "assault_mode=available\n");
        require(bom_legacy
                    && bom_legacy.format
                        == dswros::VisibilityConfigFormat::LegacySchema4,
                "legacy UTF-8 BOM visibility must remain readable");
        require(!dswros::parse_visibility_config(
                    std::string(dswros::kMaximumVisibilityConfigBytes + 1U,
                                'x')),
                "oversized visibility input must fail closed");
    }
    {
        auto image = synthetic_owner_pointer_pe();
        const std::size_t allocations_before = allocation_count;
        const auto unique = dswros::resolve_owner_pointer_rva(image);
        require(unique.status == dswros::OwnerPointerPatternStatus::Unique
                    && unique.rva == 0x3000U
                    && unique.match_count == 1U,
                "one executable-section owner-pointer signature must resolve");
        require(allocation_count == allocations_before,
                "owner-pointer resolution must not allocate");

        auto missing = image;
        missing[0x240U] = 0U;
        require(dswros::resolve_owner_pointer_rva(missing).status
                    == dswros::OwnerPointerPatternStatus::NotFound,
                "a missing owner-pointer signature must fail closed");

        auto ambiguous = image;
        write_owner_pointer_pattern(
            ambiguous, 0x280U, 0x1080U, 0x3010U);
        const auto ambiguous_result =
            dswros::resolve_owner_pointer_rva(ambiguous);
        require(ambiguous_result.status
                    == dswros::OwnerPointerPatternStatus::Ambiguous
                    && ambiguous_result.match_count == 2U,
                "multiple owner-pointer signatures must fail closed");

        auto invalid_target = synthetic_owner_pointer_pe();
        write_owner_pointer_pattern(
            invalid_target, 0x240U, 0x1040U, 0x5000U);
        require(dswros::resolve_owner_pointer_rva(invalid_target).status
                    == dswros::OwnerPointerPatternStatus::TargetOutOfRange,
                "an out-of-image owner-pointer target must fail closed");

        auto valid_plus_invalid = synthetic_owner_pointer_pe();
        write_owner_pointer_pattern(
            valid_plus_invalid, 0x280U, 0x1080U, 0x5000U);
        const auto valid_plus_invalid_result =
            dswros::resolve_owner_pointer_rva(valid_plus_invalid);
        require(valid_plus_invalid_result.status
                    == dswros::OwnerPointerPatternStatus::Unique
                    && valid_plus_invalid_result.rva == 0x3000U
                    && valid_plus_invalid_result.match_count == 1U,
                "an invalid incidental signature must not make one valid owner ambiguous");

        auto raw_padding = synthetic_owner_pointer_pe();
        write_pe_value(raw_padding, 0x188U + 8U, std::uint32_t{0x80U});
        write_owner_pointer_pattern(
            raw_padding, 0x300U, 0x1100U, 0x3020U);
        const auto raw_padding_result =
            dswros::resolve_owner_pointer_rva(raw_padding);
        require(raw_padding_result.status
                    == dswros::OwnerPointerPatternStatus::Unique
                    && raw_padding_result.rva == 0x3000U
                    && raw_padding_result.match_count == 1U,
                "raw section padding beyond VirtualSize must not create ambiguity");

        image[0] = 0U;
        require(dswros::resolve_owner_pointer_rva(image).status
                    == dswros::OwnerPointerPatternStatus::InvalidImage,
                "a malformed game image must fail closed");
    }
    {
        require(dswros::is_scannable_save_key_field_offset(
                    dswros::kLegacySaveKeyFieldOffset),
                "the established save-key field must remain in the bounded scan");
        require(dswros::is_scannable_save_key_field_offset(0x140U),
                "an aligned nearby compatibility field must be scannable");
        require(dswros::kCompatibilitySaveKeyFieldOffset == 0x128U
                    && dswros::is_scannable_save_key_field_offset(
                        dswros::kCompatibilitySaveKeyFieldOffset),
                "the verified 1.0.11 save-key field must remain an explicit fast path");
        require(!dswros::is_scannable_save_key_field_offset(0x143U),
                "misaligned save-key fields must fail closed");
        require(!dswros::is_scannable_save_key_field_offset(
                    dswros::kMaximumSaveKeyFieldOffset
                    + dswros::kSaveKeyFieldStride),
                "save-key fields beyond the bounded owner window must fail closed");
        require(dswros::has_save_key_candidate_budget(0U)
                    && dswros::has_save_key_candidate_budget(
                        dswros::kMaximumSaveKeyCandidates - 1U)
                    && !dswros::has_save_key_candidate_budget(
                        dswros::kMaximumSaveKeyCandidates),
                "all owner routes must share one bounded 24-candidate budget");
        require(dswros::is_plausible_save_key_descriptor(
                    0x10000U, 33, 40),
                "a bounded FString-shaped key descriptor must be accepted");
        require(!dswros::is_plausible_save_key_descriptor(0U, 33, 40),
                "a null save-key buffer must fail closed");
        require(!dswros::is_plausible_save_key_descriptor(
                    0x10000U, 33, 32),
                "a save-key length larger than capacity must fail closed");
        require(!dswros::is_plausible_save_key_descriptor(
                    0x10000U, dswros::kMaximumSaveKeyCharacters + 1,
                    dswros::kMaximumSaveKeyCharacters + 1),
                "an oversized save-key descriptor must fail closed");
        require(dswros::should_retry_packaged_owner_with_runtime_pattern(
                    true, true, false, false),
                "a full activation must replace a stale packaged owner after database key validation fails");
        require(!dswros::should_retry_packaged_owner_with_runtime_pattern(
                    false, true, false, false),
                "a narrow reconciliation request must never scan the executable");
        require(!dswros::should_retry_packaged_owner_with_runtime_pattern(
                    true, false, false, false),
                "a non-packaged owner route must not recursively rescan");
        require(!dswros::should_retry_packaged_owner_with_runtime_pattern(
                    true, true, true, false),
                "a database-validated packaged owner must remain the fast path");
        require(!dswros::should_retry_packaged_owner_with_runtime_pattern(
                    true, true, false, true),
                "owner pattern resolution must remain one-shot per process");
    }
    require(dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "# Read once at native startup.\n"
                "debug_logging = true\n")
                == std::optional<bool>{true},
            "the documented diagnostics true value must be accepted");
    require(dswros::parse_event_log_enabled(
                "; Public release default\r\n"
                "\r\n"
                "[diagnostics]\r\n"
                "debug_logging=false\r\n")
                == std::optional<bool>{false},
            "comments, blank lines, and CRLF must be accepted");
    require(dswros::parse_event_log_enabled("event_log_enabled=true\n")
                == std::optional<bool>{true},
            "the exact legacy diagnostics true value must remain accepted");
    require(dswros::parse_event_log_enabled("event_log_enabled=false\r\n")
                == std::optional<bool>{false},
            "the exact legacy diagnostics false value must remain accepted");
    require(!dswros::parse_event_log_enabled(""),
            "missing diagnostics input must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\ndebug_logging=TRUE\n"),
            "non-canonical diagnostics values must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "debug_logging=true\n"
                "debug_logging=false\n"),
            "duplicate diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\nunknown=true\n"),
            "unknown diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled("debug_logging=true\n"),
            "a sectionless current diagnostics key must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "debug_logging=true\n"
                "event_log_enabled=true\n"),
            "mixed current and legacy diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[other]\ndebug_logging=true\n"),
            "unknown diagnostics sections must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\rdebug_logging=true\n"),
            "a lone carriage return must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\r\ndebug_logging=true\n"),
            "mixed CRLF and LF diagnostics must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\ndebug_logging=true\r\n"),
            "mixed LF and CRLF diagnostics must fail closed in either order");
    require(!dswros::parse_event_log_enabled(
                std::string(dswros::kMaximumDiagnosticsConfigBytes + 1, 'x')),
            "oversized diagnostics input must fail closed");
    {
        std::array<char, dswros::kMaximumDiagnosticLogMetadataBytes> output{};
        const auto allocations_before = allocation_count;
        const auto written = dswros::format_diagnostic_log_metadata(
            output, 42U, 1780000000123ULL, 9876U);
        require(std::string_view{output.data(), written}
                    == " seq=42 utc_ms=1780000000123 elapsed_ms=9876",
                "diagnostic events must carry stable sequence and time fields");
        require(allocation_count == allocations_before,
                "diagnostic event metadata formatting must not allocate");
        std::array<char, 8> undersized{};
        require(dswros::format_diagnostic_log_metadata(
                    undersized, 1U, 2U, 3U) == 0U,
                "diagnostic metadata formatting must fail closed on capacity");
    }
    {
        using dswros::AreaQuestEligibilityProof;
        using dswros::AreaQuestState;
        require(!dswros::area_quest_compact_visible(AreaQuestState::None),
                "compact area quests must hide NONE");
        require(dswros::area_quest_compact_visible(AreaQuestState::Acceptable),
                "compact area quests must show ACCEPTABLE");
        require(dswros::area_quest_compact_visible(AreaQuestState::Progress),
                "compact area quests must show PROGRESS");
        require(!dswros::area_quest_compact_visible(AreaQuestState::End)
                    && !dswros::area_quest_compact_visible(
                        AreaQuestState::Fail)
                    && !dswros::area_quest_compact_visible(
                        AreaQuestState::Unknown),
                "compact area quests must fail closed for terminal or unknown states");

        require(dswros::area_quest_completion_transition(
                    AreaQuestState::Progress, AreaQuestState::End),
                "progress-to-end must prove a completed active task");
        require(!dswros::area_quest_completion_transition(
                    AreaQuestState::Progress, AreaQuestState::Fail)
                    && !dswros::area_quest_completion_transition(
                        AreaQuestState::Acceptable, AreaQuestState::End),
                "fail and non-progress transitions must not latch completion");
        require(dswros::area_quest_witnessed_completion_transition(
                    true, AreaQuestState::Fail, AreaQuestState::End)
                    && dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Acceptable,
                        AreaQuestState::End)
                    && dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::None,
                        AreaQuestState::End),
                "an exact quest identity may accept end over a stale snapshot");
        require(!dswros::area_quest_witnessed_completion_transition(
                    false, AreaQuestState::Fail, AreaQuestState::End)
                    && !dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Progress,
                        AreaQuestState::Fail)
                    && !dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Progress,
                        AreaQuestState::Progress),
                "witnessed completion must require exact identity and end");
        require(dswros::area_quest_active_sample_may_rearm(0, 0, true)
                    && dswros::area_quest_active_sample_may_rearm(1, 1, true),
                "a current scan with a settled inactive boundary may rearm a repeatable task");
        require(!dswros::area_quest_active_sample_may_rearm(0, 1, true)
                    && !dswros::area_quest_active_sample_may_rearm(2, 1, true)
                    && !dswros::area_quest_active_sample_may_rearm(1, 1, false),
                "stale or not-yet-settled active samples must not overwrite exact completion");
        require(dswros::area_quest_inactive_sample_arms_reactivation(
                    AreaQuestState::None)
                    && dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::End)
                    && !dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::Fail)
                    && !dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::Progress),
                "only a proven inactive NONE or END sample may arm repeatable reactivation");
        bool completion_latched = true;
        bool reactivation_armed = false;
        if (dswros::area_quest_active_sample_may_rearm(
                0, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(completion_latched,
                "scan start then exact completion then stale progress must preserve completion");
        if (dswros::area_quest_active_sample_may_rearm(
                1, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(completion_latched,
                "post-completion stale progress must remain hidden before an inactive boundary");
        reactivation_armed =
            dswros::area_quest_inactive_sample_arms_reactivation(
                AreaQuestState::End);
        if (dswros::area_quest_active_sample_may_rearm(
                1, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(!completion_latched,
                "inactive boundary then later active state may rearm a repeatable task");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "a proven eligible unfinished quest must be globally visible");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, true, false, false,
                    AreaQuestEligibilityProof::Unknown),
                "an unfinished quest without prerequisite proof must fail closed");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, false, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "NONE without a completion snapshot must remain hidden");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, false, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "UNKNOWN without a completion snapshot must remain hidden");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, true, false,
                    AreaQuestEligibilityProof::Eligible),
                "a completed saved quest must remain hidden");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::Acceptable, false, true, true,
                    AreaQuestEligibilityProof::Unknown)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Progress, false, true, true,
                        AreaQuestEligibilityProof::Unknown),
                "strict runtime eligibility must remain visible on the world map");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::End, true, false, false,
                    AreaQuestEligibilityProof::Eligible)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Fail, true, false, false,
                        AreaQuestEligibilityProof::Eligible),
                "unconfirmed terminal samples must not hide an unfinished proven quest");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::End, true, false, true,
                    AreaQuestEligibilityProof::Eligible)
                    && !dswros::area_quest_world_map_visible(
                        AreaQuestState::Fail, true, false, true,
                        AreaQuestEligibilityProof::Eligible),
            "a confirmed runtime completion must hide the quest");

        require(dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 0, 1)
                && dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 1, 2),
            "an exact requested save row must grow beyond its verified per-ID baseline");
        require(!dswros::accept_area_quest_save_confirmation(
                    false, true, false, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, false, false, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, true, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, false, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true,
                    dswros::kUnknownAreaQuestSaveCompletionCount, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 1, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 2, 1),
            "save confirmation must reject unrequested, unavailable, ambiguous, mismatched, unknown-baseline, unchanged, and regressed rows");
        require(dswros::area_quest_completion_generation_may_arm(false)
                && !dswros::area_quest_completion_generation_may_arm(true),
            "a failed area-quest generation must suppress repeated same-cycle witnesses");
        require(dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::None)
                && dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::End)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::Fail)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    false, AreaQuestState::None),
            "only a locked generation followed by NONE or END may arm reactivation");
        require(dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Acceptable)
                && dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    false, true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, false, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::None)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::End)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Fail),
            "a failed area-quest generation must unlock only after an inactive NONE/END sample followed by a fresh active sample");
        require(dswros::retry_area_quest_save_confirmation(
                    true, false, 1, 3)
                && dswros::retry_area_quest_save_confirmation(
                    true, false, 2, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    false, false, 1, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    true, true, 1, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    true, false, 3, 3),
            "area-quest save fallback must be positive-only and bounded to three event-driven attempts");
        require(dswros::accept_treasure_save_confirmation(
                    true, true, true)
                && !dswros::accept_treasure_save_confirmation(
                    false, true, true)
                && !dswros::accept_treasure_save_confirmation(
                    true, false, true)
                && !dswros::accept_treasure_save_confirmation(
                    true, true, false)
                && dswros::retry_treasure_save_confirmation(
                    true, false, 1, 2)
                && !dswros::retry_treasure_save_confirmation(
                    true, true, 1, 2)
                && !dswros::retry_treasure_save_confirmation(
                    true, false, 2, 2),
            "underwater save confirmation must require an exact positive requested bit and allow only one bounded retry");
        require(dswros::accept_mounted_treasure_interactor(
                    true, true, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    false, true, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, false, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, true, false, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, true, true, false),
            "mounted underwater treasure completion must require the mount-only receiver, exact current Rider, same world, and nearby exact receiver");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::Acceptable, true, true, true,
                    AreaQuestEligibilityProof::Unknown)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Progress, true, true, true,
                        AreaQuestEligibilityProof::Unknown),
                "current active state must override an earlier repeatable completion");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, false, true,
                    AreaQuestEligibilityProof::Eligible),
                "a completion observed in this activation must override an unfinished snapshot");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, true, false, true,
                    AreaQuestEligibilityProof::Eligible),
                "an unavailable local object must not revive a completion observed in this activation");
        require(dswros::area_quest_visible_for_display_mode(
                    false, true, AreaQuestState::None,
                    true, false, false)
                    && !dswros::area_quest_visible_for_display_mode(
                        false, false, AreaQuestState::None,
                        true, false, false),
                "AVAILABLE mode must preserve prerequisite-proven visibility exactly");
        require(dswros::area_quest_visible_for_display_mode(
                    true, false, AreaQuestState::None,
                    true, false, false)
                    && dswros::area_quest_visible_for_display_mode(
                        true, false, AreaQuestState::Unknown,
                        false, false, false),
                "ALL mode must expose unfinished tasks even when prerequisite or snapshot proof is unavailable");
        require(!dswros::area_quest_visible_for_display_mode(
                    true, true, AreaQuestState::None,
                    true, true, false)
                    && !dswros::area_quest_visible_for_display_mode(
                        true, true, AreaQuestState::None,
                        false, false, true),
                "ALL mode must still hide saved or runtime-confirmed completions");
        require(dswros::area_quest_visible_for_display_mode(
                    true, false, AreaQuestState::Progress,
                    true, true, true),
                "a currently active repeatable task must override an older completion cycle in ALL mode");
    }
    {
        auto value = tracker();
        const auto id = value.observe({4, 8}, "TreasureBox01_C", {1001.0, 1998.0, 2999.0});
        require(id && *id == 1001, "exact class and nearby coordinate must match");
        const auto event = value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 7, 11);
        require(event && event->kind == dswros::EventKind::TreasureOpened && event->id == 1001,
                "destroyed observed nearby treasure must publish opened event");
    }
    {
        dswros::DisappearanceConfirmation gate;
        require(!gate.sample(false, true), "first missing sample must not complete");
        require(gate.sample(false, true), "second missing sample must complete");
        gate.reset();
        require(!gate.sample(false, true), "reset must clear missing history");
        require(!gate.sample(true, true), "reappearance must cancel missing history");
        require(!gate.sample(false, false), "invalid context must fail closed");
        gate.mark_eligible_end();
        require(gate.sample(false, true), "eligible EndPlay plus missing probe must complete");
    }
    {
        dswros::UnobservedEncounterEndEvidence evidence{
            true, false, false, true, true, true, true, true, true,
            true, false};
        require(dswros::accept_unobserved_encounter_end(evidence),
                "destroyed exact nearby available encounter must recover");
        auto rejected = evidence;
        rejected.destroyed = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "streaming removal must not recover an unobserved encounter");
        rejected = evidence;
        rejected.exact_catalog_class = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "non-target actor class must not recover an encounter");
        rejected = evidence;
        rejected.player_near_actor = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "distant actor destruction must not recover an encounter");
        rejected.weak_identity_valid = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "invalid weak identity must not recover an encounter");
        rejected = evidence;
        rejected.activity_suppressed = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "suppressed activity must fail closed");
        rejected = evidence;
        rejected.transition_active = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "travel destruction must fail closed");
        rejected = evidence;
        rejected.encounter_available = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "cooling-down encounter must not recover twice");
        rejected = evidence;
        rejected.duplicate_observation = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "existing positive observation must own completion");
        require(dswros::encounter_available_now(
                    true, true, false, 0, 1000),
                "encounter without cooldown must be available");
        require(!dswros::encounter_available_now(
                    true, false, false, 0, 1000),
                "expired time-window encounter must reject completion");
        require(!dswros::encounter_available_now(
                    false, true, false, 0, 1000),
                "unready encounter state must reject completion");
        require(dswros::encounter_activity_edge_requires_reset(true, true),
                "activity suppression entry must reset encounter observations");
        require(!dswros::encounter_activity_edge_requires_reset(false, true),
                "unchanged activity suppression must not repeat encounter cleanup");
        require(!dswros::encounter_activity_edge_requires_reset(true, false),
                "open-world return must preserve fresh encounter candidates");
        require(!dswros::encounter_available_now(
                    true, true, true, 1100, 1000),
                "future cooldown must reject duplicate completion");
        require(!dswros::encounter_cooldown_write_allowed(
                    true, 1100, 1000),
                "future cooldown must reject duplicate cooldown write");
        require(dswros::encounter_available_now(
                    true, true, true, 1000, 1000),
                "expired cooldown must allow a new completion");
        require(!dswros::encounter_visible_for_display_mode(
                    true, true, false, false, false, 0, 1000),
                "AVAILABLE Assault display must preserve the live time window");
        require(dswros::encounter_visible_for_display_mode(
                    true, true, true, false, false, 0, 1000),
                "ALL Assault display must expose an out-of-window static record");
        require(dswros::encounter_visible_for_display_mode(
                    true, true, true, false, true, 1100, 1000),
                "ALL Assault display must include cooling-down Assaults");
        require(dswros::encounter_visible_for_display_mode(
                    false, true, true, false, true, 1100, 1000),
                "ALL Assault display must expose the static catalog before save state is ready");
        require(!dswros::encounter_visible_for_display_mode(
                    true, false, true, false, false, 0, 1000),
                "ALL Assault display must not bypass a Boss time condition");
        require(!dswros::encounter_visible_for_display_mode(
                    false, true, false, true, false, 0, 1000),
                "AVAILABLE Assault display must fail closed before state is ready");
        require(dswros::encounter_cooldown_write_allowed(
                    true, 1000, 1000),
                "expired cooldown must allow a new cooldown write");
        require(dswros::merge_encounter_cooldown(1100, 1050) == 1100,
                "an older save snapshot must not roll back a newer runtime cooldown");
        require(dswros::merge_encounter_cooldown(1100, 1200) == 1200,
                "a newer save snapshot must advance the effective cooldown");
    }
    {
        dswros::EncounterDisappearanceEvidence evidence{
            true, false, true, true};
        require(dswros::accept_encounter_disappearance(evidence),
                "a visible encounter may complete after disappearing while the player remains nearby");
        auto rejected = evidence;
        rejected.player_near_last_actor = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "leaving encounter streaming range must never count as a defeat");
        rejected = evidence;
        rejected.visible_seen = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "an unseen encounter placeholder must not count as a defeat");
        rejected = evidence;
        rejected.lifecycle_valid = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "travel and suppression must invalidate disappearance evidence");
        rejected = evidence;
        rejected.visible_seen = false;
        rejected.destroyed_end = true;
        require(dswros::accept_encounter_disappearance(rejected),
                "an exact nearby Destroyed EndPlay remains positive evidence");
        require(dswros::preserve_observed_encounter_removal(
                    true, true, true),
                "an observed nearby encounter removal must retain its numeric confirmation gate");
        require(!dswros::preserve_observed_encounter_removal(
                    false, true, true),
                "an unobserved streaming removal must remain insufficient evidence");
        require(!dswros::preserve_observed_encounter_removal(
                    true, true, false),
                "travel must reject retained encounter removal evidence");
        require(!dswros::encounter_cursor_context_allowed(true, false),
                "a live weak disappearance must reset while a menu owns the cursor");
        require(dswros::encounter_cursor_context_allowed(true, true),
                "an exact ended numeric observation must survive opening the map");
        require(dswros::encounter_cursor_context_allowed(false, false),
                "normal gameplay must allow ordinary encounter observation");

        dswros::EncounterDeathNotificationEvidence death_evidence{
            true, true, true, true, true, true, true};
        require(dswros::accept_encounter_death_notification(death_evidence),
                "an exact observed nearby monster death notification must complete");
        auto rejected_death = death_evidence;
        rejected_death.lifecycle_valid = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "travel or suppression must reject a death notification");
        rejected_death = death_evidence;
        rejected_death.encounter_currently_available = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unavailable encounter must reject a duplicate death notification");
        rejected_death = death_evidence;
        rejected_death.exact_observed_actor = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unobserved receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.exact_catalog_class = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a non-catalog receiver class must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.monster_character = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a non-monster receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.visible_seen = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unseen receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.player_near_actor = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a death outside the observed streaming context must fail closed");

        require(dswros::encounter_death_process_is_terminal(3)
                    && !dswros::encounter_death_process_is_terminal(0)
                    && !dswros::encounter_death_process_is_terminal(1)
                    && !dswros::encounter_death_process_is_terminal(2)
                    && !dswros::encounter_death_process_is_terminal(4),
                "only the exact death-process End state may publish completion");

        const dswros::PendingEncounterDeathConsumptionContext
            normal_death_handoff{false, true, true, false, false, true};
        require(dswros::can_consume_pending_encounter_death(
                    normal_death_handoff),
                "a valid active context must consume an accepted encounter death bit");
        auto deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.transition_active = true;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "travel must defer ordinary accepted encounter death consumption");
        deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.activity_suppressed = true;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "activity suppression must defer ordinary accepted encounter death consumption");
        deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.player_position_valid = false;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "an invalid player position must defer ordinary accepted encounter death consumption");
        dswros::PendingEncounterDeathConsumptionContext boundary_death_handoff{
            true, false, true, true, true, false};
        require(dswros::can_consume_pending_encounter_death(
                    boundary_death_handoff),
                "an authoritative boundary must preserve a bit after Engine Tick disables itself");
        boundary_death_handoff.encounter_state_ready = false;
        require(!dswros::can_consume_pending_encounter_death(
                    boundary_death_handoff),
                "an uninitialized encounter state must reject even boundary consumption");

        dswros::EncounterDisappearanceConfirmation fast_end_gate;
        require(!fast_end_gate.sample(true, true, false, 100),
                "one visible sample must not arm ordinary encounter confirmation");
        fast_end_gate.arm_observed_end_fallback(200);
        for (std::uint32_t sample = 1;
             sample < dswros::EncounterDisappearanceConfirmation::kRequiredMissingSamples;
             ++sample) {
            require(!fast_end_gate.sample(
                        false, true, false,
                        200 + static_cast<std::int64_t>(sample) * 250),
                    "observed EndPlay fallback must retain the ten-second gate");
        }
        require(fast_end_gate.sample(false, true, false, 10'200),
                "observed EndPlay fallback must complete after forty missing samples and ten seconds");

        dswros::EncounterDisappearanceConfirmation returned_gate;
        returned_gate.arm_observed_end_fallback(1'000);
        require(!returned_gate.sample(false, true, false, 1'250),
                "observed EndPlay fallback must begin as missing");
        require(!returned_gate.sample(true, true, false, 1'500),
                "a returned encounter must cancel the missing sequence");
        require(!returned_gate.pending(),
                "a returned encounter must clear pending EndPlay evidence");

        dswros::EncounterDisappearanceConfirmation gate;
        require(!gate.sample(true, true, false, 0),
                "the first visible sample must not arm encounter fallback");
        require(!gate.sample(true, true, false, 250),
                "short visibility must not arm encounter fallback");
        require(!gate.sample(true, true, false, 500),
                "short visibility must remain unarmed");
        for (std::uint32_t sample = 0; sample < 50; ++sample) {
            require(!gate.sample(false, true, false,
                        750 + static_cast<std::int64_t>(sample) * 250),
                    "an encounter that was not stably present must not complete");
        }
        require(!gate.sample(false, false, false, 20'000),
                "context reset must clear the unarmed history");
        for (std::int64_t now = 21'000; now <= 22'000; now += 250) {
            require(!gate.sample(true, true, false, now),
                    "stable presence only arms the encounter fallback");
        }
        for (std::int64_t now = 22'250; now < 32'250; now += 250) {
            require(!gate.sample(false, true, false, now),
                    "less than ten missing seconds must fail closed");
        }
        require(gate.sample(false, true, false, 32'250),
                "ten seconds plus enough missing probes while nearby may complete an encounter");
        require(!gate.sample(true, true, false, 32'500),
                "reappearance must reset missing encounter evidence");
        require(!gate.sample(false, true, false, 32'750),
                "a fresh disappearance must restart confirmation");
        require(!gate.sample(false, false, false, 33'000),
                "leaving range must cancel a pending encounter disappearance");
        require(!gate.pending(),
                "a rejected out-of-range disappearance must not accumulate");
        require(gate.sample(false, true, true, 33'250),
                "an exact nearby Destroyed EndPlay may complete immediately");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, false, false, {1000.0, 2000.0, 3000.0}, 7, 11),
                "streaming removal must not publish opened event");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, true, {1000.0, 2000.0, 3000.0}, 7, 11),
                "transition destruction must fail closed");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 8, 11),
                "stale activation must not publish");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {10000.0, 2000.0, 3000.0}, 7, 11),
                "distant destruction must not publish");
    }
    {
        auto value = tracker();
        require(!value.observe({4, 8}, "TreasureBox02_C", {1000.0, 2000.0, 3000.0}),
                "wrong exact class must not match");
        require(!value.observe({4, 8}, "TreasureBox01_C", {2000.0, 2000.0, 3000.0}),
                "coordinate outside match radius must not match");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {2001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {2002, "TreasureBox01_C", {1000.0, 2000.0, 3500.0}},
            {2003, "TreasureBox01_C", {1181.0, 2013.0, 3500.0}},
        });
        value.reset(7, 11);
        const auto upper = value.observe(
            {20, 40}, "TreasureBox01_C", {1000.0, 2000.0, 3500.0});
        const auto adjacent = value.observe(
            {21, 41}, "TreasureBox01_C", {1181.0, 2013.0, 3500.0});
        const auto lower = value.observe(
            {22, 42}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0});
        require(upper && *upper == 2002,
                "XY-overlapping treasure actors must resolve by exact 3D position");
        require(adjacent && *adjacent == 2003,
                "adjacent same-class treasure actors must retain distinct IDs");
        require(lower && *lower == 2001,
                "the lower XY-overlapping treasure must retain its own ID");
        require(value.observed_count() == 3,
                "adjacent treasure weak identities must remain independently observed");
        const auto upper_end = value.end(
            {20, 40}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        const auto adjacent_end = value.end(
            {21, 41}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        const auto lower_end = value.end(
            {22, 42}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        require(upper_end && upper_end->id == 2002,
                "the upper treasure lifecycle must publish only its own ID");
        require(adjacent_end && adjacent_end->id == 2003,
                "the adjacent treasure lifecycle must publish only its own ID");
        require(lower_end && lower_end->id == 2001,
                "the lower treasure lifecycle must publish only its own ID");
        require(value.observed_count() == 0,
                "completed adjacent treasure observations must not accumulate");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {3001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {3002, "TreasureBox01_C", {1000.0, 2000.0, 3500.0}},
            {3003, "TreasureBox02_C", {1181.0, 2013.0, 3500.0}},
        });
        value.reset(7, 11);
        const auto lower = value.observe_reported_id(
            {30, 50}, "TreasureBox01_C",
            {1000.0, 2000.0, 3000.0}, 3001);
        const auto upper = value.observe_reported_id(
            {31, 51}, "TreasureBox01_C",
            {1000.0, 2000.0, 3500.0}, 3002);
        require(lower && *lower == 3001 && upper && *upper == 3002,
                "runtime ObjectID plus class and 3D position must keep overlapping treasures distinct");
        require(!value.observe_reported_id(
                    {32, 52}, "TreasureBox01_C",
                    {1181.0, 2013.0, 3500.0}, 3003),
                "runtime ObjectID class conflict must fail closed without spatial fallback");
        require(!value.observe_reported_id(
                    {33, 53}, "TreasureBox01_C",
                    {1000.0, 2000.0, 3000.0}, 9999),
                "unknown runtime ObjectID must fail closed");
        require(!value.observe_reported_id(
                    {34, 54}, "TreasureBox01_C",
                    {2000.0, 2000.0, 3000.0}, 3001),
                "runtime ObjectID outside the catalog 3D bound must fail closed");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {4001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {4001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
        });
        value.reset(7, 11);
        require(!value.observe_reported_id(
                    {40, 60}, "TreasureBox01_C",
                    {1000.0, 2000.0, 3000.0}, 4001),
                "duplicate reported treasure IDs must fail closed");
    }
    {
        auto value = tracker();
        const auto nearby = value.nearby_classes({900.0, 2000.0, 3000.0}, 3000.0);
        require(nearby.size() == 1 && nearby.front() == "TreasureBox01_C",
                "catch-up must request only classes near the player");
    }
    {
        auto value = tracker();
        const auto nearest = value.nearest_point({1200.0, 2100.0, 3000.0}, 10000.0);
        require(nearest && nearest->id == 1001,
                "native render target selection must return the closest catalog point");
        require(!value.nearest_point({50000.0, 50000.0, 50000.0}, 1000.0),
                "native render target selection must respect the bounded radius");
        const auto annulus = value.nearest_planar_point_in_annulus(
            {1000.0, 2000.0, 3000.0}, 3000.0, 20000.0);
        require(annulus && annulus->id == 1002,
                "validation target selection must avoid markers hidden at the player center");
        require(!value.nearest_planar_point_in_annulus(
                    {1000.0, 2000.0, 3000.0}, 10000.0, 20000.0),
                "validation target selection must enforce both annulus bounds");
    }
    {
        const auto center = dswros::project_compact_radar_point(
            {1000.0, 2000.0, 0.0}, {1000.0, 2000.0, 0.0}, 20000.0, 2560, 1440);
        require(center && center->x == 2340.0 && center->y == 217.0
                    && center->display_scale == 1.0,
                "native compact projection must preserve accepted 2560x1440 geometry");
        const auto east_edge = dswros::project_compact_radar_point(
            {0.0, 0.0, 0.0}, {20000.0, 0.0, 0.0}, 20000.0, 2560, 1440);
        require(east_edge && east_edge->x == 2510.0 && east_edge->y == 217.0,
                "native compact projection must place the positive-X radius at the right edge");
        require(!dswros::project_compact_radar_point(
                    {0.0, 0.0, 0.0}, {20001.0, 0.0, 0.0}, 20000.0, 2560, 1440),
                "native compact projection must reject points outside the configured radius");
    }
    {
        const auto fullscreen = dswros::calculate_compact_viewport_layout(
            3840.0, 2160.0, 1.5, 1.0);
        require(fullscreen && near(fullscreen->display_scale, 1.5)
                    && near(fullscreen->umg_unit_scale, 1.0)
                    && near(fullscreen->host_origin_x, 3146.625)
                    && near(fullscreen->host_origin_y, -37.875)
                    && near(fullscreen->host_render_scale, 1.0),
                "compact fullscreen layout must preserve the accepted physical geometry");

        const auto windowed = dswros::calculate_compact_viewport_layout(
            2560.0, 1600.0, 1.0, 1.0);
        require(windowed && near(windowed->display_scale, 1.0)
                    && near(windowed->umg_unit_scale, 1.0)
                    && near(windowed->host_origin_x, 2097.75)
                    && near(windowed->host_origin_y, -25.25)
                    && near(windowed->host_render_scale, 1.0),
                "a retained compact host must move back on-screen after a window-mode resize");

        const auto native_ultrawide =
            dswros::calculate_compact_viewport_layout(
                3440.0, 1440.0, 1.0, 1.0);
        require(native_ultrawide
                    && near(native_ultrawide->display_scale, 1.0)
                    && near(native_ultrawide->host_origin_x, 2977.75)
                    && near(native_ultrawide->host_origin_y, -25.25),
                "native 21:9 must use the live game viewport instead of desktop geometry");

        const auto ultrawide_content_viewport =
            dswros::calculate_compact_viewport_layout(
                3840.0, 1600.0, 1.0, 1.0);
        require(ultrawide_content_viewport
                    && near(ultrawide_content_viewport->display_scale, 10.0 / 9.0)
                    && near(ultrawide_content_viewport->host_origin_x,
                            3326.388888888889)
                    && near(ultrawide_content_viewport->host_origin_y,
                            -28.055555555556)
                    && near(ultrawide_content_viewport->host_render_scale,
                            10.0 / 9.0),
                "a true 3840x1600 game viewport must preserve its compact HUD geometry without implying a desktop-sized viewport");

        const auto ultrawide_window =
            dswros::calculate_compact_viewport_layout(
                2560.0, 1080.0, 1.0, 1.0);
        require(ultrawide_window
                    && near(ultrawide_window->display_scale, 0.75)
                    && near(ultrawide_window->host_origin_x, 2213.3125)
                    && near(ultrawide_window->host_origin_y, -18.9375),
                "windowed 21:9 must reflow from its live client viewport");

        const auto dpi_resize = dswros::calculate_compact_viewport_layout(
            1920.0, 1080.0, 1.25, 1.0);
        require(dpi_resize && near(dpi_resize->display_scale, 0.75)
                    && near(dpi_resize->umg_unit_scale, 0.6)
                    && near(dpi_resize->host_render_scale, 0.6),
                "compact reflow must correct a retained host for live DPI changes");
        require(!dswros::calculate_compact_viewport_layout(
                    1920.0, 1080.0, 0.0, 1.0),
                "compact viewport layout must reject an invalid DPI scale");
    }
    {
        require(dswros::compact_time_phase(5U * 3600U + 59U * 60U)
                        == dswros::CompactTimePhase::Night
                    && dswros::compact_time_phase(6U * 3600U)
                        == dswros::CompactTimePhase::Morning
                    && dswros::compact_time_phase(12U * 3600U)
                        == dswros::CompactTimePhase::Afternoon
                    && dswros::compact_time_phase(18U * 3600U)
                        == dswros::CompactTimePhase::Evening
                    && dswros::compact_time_phase(21U * 3600U)
                        == dswros::CompactTimePhase::Night
                    && dswros::compact_time_phase(30U * 3600U)
                        == dswros::CompactTimePhase::Morning,
                "the four presentation bands must retain exact boundaries and day wrapping");
    }
    {
        const dswros::WorldMapAtlasPlacement native_atlas{
            -410.0, -275.0, 3601.246, 2048.0};
        for (const double dpi : {1.0, 1.25, 1.5}) {
            const auto viewport_atlas =
                dswros::calculate_world_map_viewport_placement(
                    native_atlas,
                    {240.0 * dpi, 120.0 * dpi, 3840.0, 2160.0,
                     0.8 * dpi, 0.8 * dpi},
                    {0.0, 0.0, 3840.0, 2160.0, dpi, dpi});
            require(viewport_atlas
                        && near(viewport_atlas->left, -88.0)
                        && near(viewport_atlas->top, -100.0)
                        && near(viewport_atlas->width, 2880.9968)
                        && near(viewport_atlas->height, 1638.4),
                    "4K viewport conversion must apply 100, 125, and 150 percent DPI exactly once");
        }

        const auto ultrawide =
            dswros::calculate_world_map_viewport_placement(
                {125.0, -64.0, 2048.0, 2048.0},
                {330.0, 210.0, 3440.0, 1440.0, 0.75, 0.75},
                {90.0, 45.0, 3440.0, 1440.0, 1.0, 1.0});
        const auto sixteen_ten =
            dswros::calculate_world_map_viewport_placement(
                {125.0, -64.0, 2048.0, 2048.0},
                {200.0, 110.0, 2560.0, 1600.0, 1.25, 1.25},
                {40.0, 30.0, 2560.0, 1600.0, 1.25, 1.25});
        require(ultrawide && near(ultrawide->left, 333.75)
                    && near(ultrawide->top, 117.0)
                    && near(ultrawide->width, 1536.0)
                    && near(ultrawide->height, 1536.0)
                    && sixteen_ten && near(sixteen_ten->left, 253.0)
                    && near(sixteen_ten->top, 0.0)
                    && near(sixteen_ten->width, 2048.0)
                    && near(sixteen_ten->height, 2048.0),
                "21:9 and 16:10 placement must honor live scale, translation, and nonzero viewport origins");

        const dswros::WorldMapAtlasPlacement affine_atlas{
            -123.5, 42.25, 3072.0, 2048.0};
        const dswros::WorldMapSlateGeometry affine_native{
            512.0, 256.0, 4000.0, 3000.0, 1.25, 0.8};
        const dswros::WorldMapSlateGeometry affine_viewport{
            128.0, -64.0, 3840.0, 2160.0, 0.75, 0.5};
        const auto affine_base =
            dswros::calculate_world_map_viewport_placement(
                affine_atlas, affine_native, affine_viewport);
        require(affine_base
                    && near(affine_base->left, 306.166666666667)
                    && near(affine_base->top, 707.6)
                    && near(affine_base->width, 5120.0)
                    && near(affine_base->height, 3276.8),
                "viewport conversion must preserve the exact native-to-viewport affine relation");

        const auto common_dpi =
            dswros::calculate_world_map_viewport_placement(
                affine_atlas,
                {affine_native.absolute_left * 1.5,
                 affine_native.absolute_top * 1.5,
                 affine_native.local_width,
                 affine_native.local_height,
                 affine_native.absolute_scale_x * 1.5,
                 affine_native.absolute_scale_y * 1.5},
                {affine_viewport.absolute_left * 1.5,
                 affine_viewport.absolute_top * 1.5,
                 affine_viewport.local_width,
                 affine_viewport.local_height,
                 affine_viewport.absolute_scale_x * 1.5,
                 affine_viewport.absolute_scale_y * 1.5});
        require(common_dpi && affine_base
                    && near(common_dpi->left, affine_base->left)
                    && near(common_dpi->top, affine_base->top)
                    && near(common_dpi->width, affine_base->width)
                    && near(common_dpi->height, affine_base->height),
                "a common desktop DPI transform must cancel rather than being applied twice");

        const auto common_translation =
            dswros::calculate_world_map_viewport_placement(
                affine_atlas,
                {affine_native.absolute_left + 731.25,
                 affine_native.absolute_top - 418.75,
                 affine_native.local_width,
                 affine_native.local_height,
                 affine_native.absolute_scale_x,
                 affine_native.absolute_scale_y},
                {affine_viewport.absolute_left + 731.25,
                 affine_viewport.absolute_top - 418.75,
                 affine_viewport.local_width,
                 affine_viewport.local_height,
                 affine_viewport.absolute_scale_x,
                 affine_viewport.absolute_scale_y});
        require(common_translation && affine_base
                    && near(common_translation->left, affine_base->left)
                    && near(common_translation->top, affine_base->top)
                    && near(common_translation->width, affine_base->width)
                    && near(common_translation->height, affine_base->height),
                "a common absolute desktop translation must not move the viewport-local atlas");

        const auto transient_geometry =
            dswros::calculate_world_map_viewport_placement(
                affine_atlas,
                {affine_native.absolute_left + 24.0,
                 affine_native.absolute_top - 18.0,
                 affine_native.local_width,
                 affine_native.local_height,
                 affine_native.absolute_scale_x * 0.9,
                 affine_native.absolute_scale_y * 1.1},
                affine_viewport);
        const auto restored_geometry =
            dswros::calculate_world_map_viewport_placement(
                affine_atlas, affine_native, affine_viewport);
        require(transient_geometry && affine_base && restored_geometry
                    && (!near(transient_geometry->left, affine_base->left)
                        || !near(transient_geometry->top, affine_base->top)
                        || !near(transient_geometry->width, affine_base->width)
                        || !near(transient_geometry->height, affine_base->height))
                    && near(restored_geometry->left, affine_base->left)
                    && near(restored_geometry->top, affine_base->top)
                    && near(restored_geometry->width, affine_base->width)
                    && near(restored_geometry->height, affine_base->height),
                "an A-B-A live-geometry sequence must return exactly to the original viewport placement without retained drift");

        require(affine_base
                    && near(
                        affine_viewport.absolute_left
                            + affine_base->left
                                * affine_viewport.absolute_scale_x,
                        affine_native.absolute_left
                            + affine_atlas.left
                                * affine_native.absolute_scale_x)
                    && near(
                        affine_viewport.absolute_top
                            + affine_base->top
                                * affine_viewport.absolute_scale_y,
                        affine_native.absolute_top
                            + affine_atlas.top
                                * affine_native.absolute_scale_y),
                "the calculated viewport placement must reconstruct the witnessed absolute atlas origin");
        require(affine_base
                    && near(
                        affine_viewport.absolute_left
                            + (affine_base->left + affine_base->width)
                                * affine_viewport.absolute_scale_x,
                        affine_native.absolute_left
                            + (affine_atlas.left + affine_atlas.width)
                                * affine_native.absolute_scale_x)
                    && near(
                        affine_viewport.absolute_top
                            + (affine_base->top + affine_base->height)
                                * affine_viewport.absolute_scale_y,
                        affine_native.absolute_top
                            + (affine_atlas.top + affine_atlas.height)
                                * affine_native.absolute_scale_y),
                "the calculated viewport placement must reconstruct the witnessed absolute atlas bottom-right extent");

        const auto nonuniform_geometry =
            dswros::calculate_world_map_viewport_placement(
                {-100.0, 50.0, 200.0, 300.0},
                {-320.0, 180.0, 1920.0, 1080.0, 1.5, 0.75},
                {-640.0, -360.0, 3840.0, 2160.0, 1.25, 0.5});
        require(nonuniform_geometry
                    && near(nonuniform_geometry->left, 136.0)
                    && near(nonuniform_geometry->top, 1155.0)
                    && near(nonuniform_geometry->width, 240.0)
                    && near(nonuniform_geometry->height, 450.0),
                "viewport conversion must preserve independent X and Y Slate scales and negative viewport origins");

        const auto nan = std::numeric_limits<double>::quiet_NaN();
        require(!dswros::calculate_world_map_viewport_placement(
                    {0.0, 0.0, 2048.0, 2048.0},
                    {0.0, 0.0, 3000.0, 3000.0, nan, 1.0},
                    {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0,
                         std::numeric_limits<double>::infinity(), 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 0.0, 1.0}),
                "viewport conversion must fail closed for NaN, infinity, and zero scale");

        require(!dswros::calculate_world_map_viewport_placement(
                    {0.0, 0.0, -1.0, 2048.0},
                    {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                    {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 0.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 0.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 0.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, -1.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, -1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, -1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 0.0, 2160.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {0.0, 0.0, 2048.0, 2048.0},
                        {0.0, 0.0, 3000.0, 3000.0, 1.0, 1.0},
                        {0.0, 0.0, 3840.0, -1.0, 1.0, 1.0})
                    && !dswros::calculate_world_map_viewport_placement(
                        {std::numeric_limits<double>::max(), 0.0,
                         2048.0, 2048.0},
                        {std::numeric_limits<double>::max(), 0.0,
                         3000.0, 3000.0,
                         std::numeric_limits<double>::max(), 1.0},
                        {0.0, 0.0, 3840.0, 2160.0, 1.0, 1.0}),
                "viewport conversion must reject non-positive atlas or geometry extents, non-positive native or viewport scales, and arithmetic overflow");
    }
    {
        const auto subpixel = dswros::world_map_viewport_transform_changed(
            {-88.0, -100.0, 2880.0, 1638.0},
            {-87.6, -100.4, 2880.4, 1637.6});
        const auto translated =
            dswros::world_map_viewport_transform_changed(
                {-88.0, -100.0, 2880.0, 1638.0},
                {-87.4, -100.0, 2880.0, 1638.0});
        const auto exact_tolerance =
            dswros::world_map_viewport_transform_changed(
                {-88.0, -100.0, 2880.0, 1638.0},
                {-87.5, -100.0, 2880.0, 1638.0});
        const auto above_tolerance =
            dswros::world_map_viewport_transform_changed(
                {-88.0, -100.0, 2880.0, 1638.0},
                {-87.499, -100.0, 2880.0, 1638.0});
        require(subpixel && !*subpixel && translated && *translated
                    && exact_tolerance && !*exact_tolerance
                    && above_tolerance && *above_tolerance
                    && !dswros::world_map_viewport_transform_changed(
                        {-88.0, -100.0, 0.0, 1638.0},
                        {-88.0, -100.0, 2880.0, 1638.0})
                    && !dswros::world_map_viewport_transform_changed(
                        {-88.0, -100.0, 2880.0, 1638.0},
                        {-88.0, -100.0, 2880.0, 1638.0}, -0.1)
                    && !dswros::world_map_viewport_transform_changed(
                        {-88.0, -100.0, 2880.0, 1638.0},
                        {-88.0, -100.0, 2880.0, 1638.0},
                        std::numeric_limits<double>::quiet_NaN()),
                "viewport transform synchronization must suppress sub-half-unit jitter and fail closed on invalid state");
    }
    {
        const auto unchanged = dswros::world_map_geometry_maximum_delta(
            {1500.0, 1500.0, 3000.0, 3000.0},
            {1500.25, 1499.75, 3000.0, 3000.0});
        const auto logged_parent_extent_change =
            dswros::world_map_geometry_maximum_delta(
                {2460.347, 2001.596, 3000.0, 3000.0},
                {3145.620, 2559.683, 3840.0, 3840.0});
        require(unchanged && near(*unchanged, 0.25)
                    && logged_parent_extent_change
                    && near(*logged_parent_extent_change, 840.0)
                    && !dswros::world_map_geometry_maximum_delta(
                        {1500.0, 1500.0, 0.0, 3000.0},
                        {1500.0, 1500.0, 3000.0, 3000.0}),
                "world-map geometry drift must distinguish sub-pixel stability from the logged 3000-to-3840 parent-local reflow");
    }
    {
        using CandidateTransition =
            dswros::WorldMapCandidateTransition;
        struct CandidateCase final {
            dswros::WorldMapCandidateEvent event;
            CandidateTransition expected;
        };
        constexpr std::array candidate_cases{
            CandidateCase{{false, false, false, false},
                          CandidateTransition::RetainIdentity},
            CandidateCase{{true, false, false, false},
                          CandidateTransition::ReplaceIdentity},
            CandidateCase{{true, true, false, false},
                          CandidateTransition::RetainIdentity},
            CandidateCase{{true, false, true, false},
                          CandidateTransition::ReplaceAndBeginOpenSession},
            CandidateCase{{true, false, true, true},
                          CandidateTransition::ReplaceAndBeginOpenSession},
            CandidateCase{{true, true, true, false},
                          CandidateTransition::BeginOpenSession},
            CandidateCase{{true, true, true, true},
                          CandidateTransition::DuplicateOpenEvent},
        };
        const std::size_t allocations_before = allocation_count;
        for (const CandidateCase& test : candidate_cases) {
            require(dswros::world_map_candidate_transition(test.event)
                        == test.expected,
                    "candidate listener and SetWorldMapImage events must retain, replace, begin, or deduplicate the world-map session exactly");
        }

        std::uint64_t candidate_serial = 7U;
        std::uint32_t open_session_count{};
        std::uint32_t replacement_count{};
        const CandidateTransition first_open =
            dswros::world_map_candidate_transition(
                {true, true, true, false});
        open_session_count +=
            dswros::world_map_open_session_started(first_open) ? 1U : 0U;
        for (std::uint32_t index = 0; index < 20U; ++index) {
            const CandidateTransition duplicate =
                dswros::world_map_candidate_transition(
                    {true, true, true, true});
            open_session_count +=
                dswros::world_map_open_session_started(duplicate) ? 1U : 0U;
            replacement_count +=
                dswros::world_map_candidate_replaced(duplicate) ? 1U : 0U;
        }
        require(open_session_count == 1U && replacement_count == 0U
                    && candidate_serial == 7U,
                "a same-layer SetWorldMapImage burst must start one session without replacing identity or resetting its serial");

        const std::uint64_t first_a =
            dswros::next_world_map_candidate_serial(0U);
        const std::uint64_t b =
            dswros::next_world_map_candidate_serial(first_a);
        const std::uint64_t second_a =
            dswros::next_world_map_candidate_serial(b);
        require(first_a == 1U && b == 2U && second_a == 3U
                    && !dswros::world_map_serial_matches(
                        true, second_a, first_a)
                    && dswros::world_map_serial_matches(
                        true, second_a, second_a)
                    && dswros::next_world_map_candidate_serial(
                        std::numeric_limits<std::uint64_t>::max()) == 1U,
                "A-B-A candidate reuse and serial rollover must not revive stale open evidence");
        require(allocation_count == allocations_before,
                "world-map candidate-session policy must not allocate");
    }
    {
        const std::size_t allocations_before = allocation_count;
        require(dswros::world_map_listener_open_probe_allowed(
                    true, false, false, true)
                    && !dswros::world_map_listener_open_probe_allowed(
                        false, false, false, true)
                    && !dswros::world_map_listener_open_probe_allowed(
                        true, true, false, true)
                    && !dswros::world_map_listener_open_probe_allowed(
                        true, false, true, true)
                    && !dswros::world_map_listener_open_probe_allowed(
                        true, false, false, false),
                "only a post-activation exact-current-world listener candidate may arm a world-map opening probe");
        require(allocation_count == allocations_before,
                "world-map listener opening policy must not allocate");
    }
    {
        using VisibilityAction = dswros::WorldMapVisibilityAction;
        using VisibilitySample = dswros::WorldMapVisibilitySample;
        struct VisibilityCase final {
            dswros::WorldMapVisibilityContext context;
            VisibilitySample sample;
            VisibilityAction expected_action;
            bool expected_recovery_edge;
        };
        constexpr dswros::WorldMapVisibilityContext new_open{
            true, true, true, true, false, false, 0U, 1'000U};
        constexpr std::array visibility_cases{
            VisibilityCase{
                {true, true, true, true, false, false, 999U, 1'000U},
                VisibilitySample::Hidden,
                VisibilityAction::Preserve,
                false},
            VisibilityCase{
                {true, true, true, true, false, false, 1'000U, 1'000U},
                VisibilitySample::Hidden,
                VisibilityAction::CloseSession,
                false},
            VisibilityCase{new_open, VisibilitySample::Unknown,
                           VisibilityAction::Preserve, false},
            VisibilityCase{new_open, VisibilitySample::Visible,
                           VisibilityAction::ConfirmVisible, true},
            VisibilityCase{
                {true, true, true, true, true, true, 2'000U, 1'000U},
                VisibilitySample::Visible,
                VisibilityAction::ConfirmVisible,
                false},
            VisibilityCase{
                {true, true, true, true, true, false, 2'000U, 1'000U},
                VisibilitySample::Visible,
                VisibilityAction::ConfirmVisible,
                true},
            VisibilityCase{
                {true, true, true, true, true, true, 1U, 1'000U},
                VisibilitySample::Hidden,
                VisibilityAction::CloseSession,
                false},
            VisibilityCase{
                {true, false, true, true, false, false, 2'000U, 1'000U},
                VisibilitySample::Visible,
                VisibilityAction::Ignore,
                false},
        };
        const std::size_t allocations_before = allocation_count;
        for (const VisibilityCase& test : visibility_cases) {
            const dswros::WorldMapVisibilityDecision decision =
                dswros::decide_world_map_visibility(
                    test.context, test.sample);
            require(decision.action == test.expected_action
                        && decision.recovery_edge
                            == test.expected_recovery_edge,
                    "world-map visibility policy must preserve transition gaps, close only at the grace boundary, and identify compact false-to-true recovery");
        }
        require(allocation_count == allocations_before,
                "world-map visibility policy must not allocate");
    }
    {
        using Gate = dswros::WorldMapWorkGate;
        using Sample = dswros::WorldMapVisibilitySample;
        struct WorkCase final {
            dswros::WorldMapWorkContext context;
            Gate expected;
        };
        constexpr std::array work_cases{
            WorkCase{{false, true, true, true, true,
                       Sample::Visible, 0U, 3U}, Gate::Ignore},
            WorkCase{{true, false, false, true, false,
                       Sample::Unknown, 0U, 3U}, Gate::Ignore},
            WorkCase{{true, true, true, true, true,
                       Sample::Unknown, 0U, 3U}, Gate::Hold},
            WorkCase{{true, true, false, true, true,
                       Sample::Visible, 0U, 3U}, Gate::Hold},
            WorkCase{{true, true, true, true, true,
                       Sample::Hidden, 0U, 3U}, Gate::CloseSession},
            WorkCase{{true, true, true, true, true,
                       Sample::Visible, 3U, 3U}, Gate::Exhausted},
            WorkCase{{true, true, true, true, true,
                       Sample::Visible, 0U, 0U}, Gate::Exhausted},
            WorkCase{{true, true, true, true, true,
                       Sample::Visible, 2U, 3U}, Gate::ConsumeAttempt},
        };
        std::uint32_t consumed_attempts{};
        const std::size_t allocations_before = allocation_count;
        for (const WorkCase& test : work_cases) {
            const Gate gate = dswros::world_map_work_gate(test.context);
            require(gate == test.expected,
                    "world-map work must hold unknown readiness and consume budget only for a confirmed-visible live session");
            consumed_attempts += gate == Gate::ConsumeAttempt ? 1U : 0U;
        }
        require(consumed_attempts == 1U,
                "only the confirmed-visible work-gate case may consume an attempt");
        require(allocation_count == allocations_before,
                "world-map work-gate policy must not allocate");
    }
    {
        using F7Action = dswros::WorldMapF7RearmAction;
        using Sample = dswros::WorldMapVisibilitySample;
        const std::size_t allocations_before = allocation_count;
        require(dswros::world_map_f7_rearm_action(
                    true, true, true, true, Sample::Unknown)
                    == F7Action::Preserve
                    && dswros::world_map_f7_rearm_action(
                        true, true, true, true, Sample::Visible)
                        == F7Action::Rearm,
                "F7 must preserve an unknown visibility session and rearm when that same exact layer becomes visible");

        struct F8Case final {
            bool candidate_available;
            bool exact_candidate_live;
            bool current_world;
            bool serial_bound_open_evidence;
            bool expected;
        };
        constexpr std::array f8_cases{
            F8Case{true, true, true, true, true},
            F8Case{false, true, true, true, false},
            F8Case{true, false, true, true, false},
            F8Case{true, true, false, true, false},
            F8Case{true, true, true, false, false},
        };
        for (const F8Case& test : f8_cases) {
            require(dswros::preserve_world_map_evidence_on_f8(
                        test.candidate_available,
                        test.exact_candidate_live,
                        test.current_world,
                        test.serial_bound_open_evidence) == test.expected,
                    "F8 evidence preservation must depend only on exact live current-world serial-bound evidence, never renderer suspension state");
        }
        require(allocation_count == allocations_before,
                "world-map F7 and F8 lifecycle policy must not allocate");
    }
    {
        using Failure = dswros::WorldMapTransformObservationFailure;
        using Action =
            dswros::WorldMapTransformObservationFailureAction;
        using Stage = dswros::WorldMapTransformSyncStage;
        const auto classify = [](Failure failure, bool same_layer,
                                 bool has_valid_transform) {
            return dswros::classify_world_map_transform_observation_failure(
                failure, same_layer, has_valid_transform);
        };
        require(
            classify(Failure::NativeCanvasUnavailable, false, false)
                    == Action::RetryHidden
                && classify(Failure::GeometryUnavailable, true, false)
                    == Action::RetryHidden
                && classify(Failure::NativeCanvasUnavailable, true, true)
                    == Action::RetainLastValid
                && classify(Failure::GeometryUnavailable, true, true)
                    == Action::RetainLastValid
                && classify(Failure::NativeCanvasUnavailable, false, true)
                    == Action::RetryHidden
                && classify(Failure::OwnedHostInvalid, true, true)
                    == Action::Fault
                && classify(Failure::AbiInvalid, true, true)
                    == Action::Fault
                && classify(Failure::RuntimeFault, true, true)
                    == Action::Fault,
            "same-layer transient world-map observation gaps must retain the last verified transform while ownership, ABI, and runtime failures remain terminal");
        require(
            dswros::world_map_transform_failure_for_stage(
                Stage::NativeCanvasObservation)
                    == Failure::NativeCanvasUnavailable
                && dswros::world_map_transform_failure_for_stage(
                    Stage::GeometryObservation)
                    == Failure::GeometryUnavailable
                && dswros::world_map_transform_failure_for_stage(
                    Stage::OwnedHostValidation)
                    == Failure::OwnedHostInvalid
                && dswros::world_map_transform_failure_for_stage(
                    Stage::AbiValidation)
                    == Failure::AbiInvalid
                && dswros::world_map_transform_failure_for_stage(
                    Stage::OwnedHostApplication)
                    == Failure::RuntimeFault,
            "guarded transform failures must retain observation-stage gaps but classify owned-host and ABI stages as terminal");
        require(
            dswros::classify_world_map_owned_host_application_failure(
                true, true, true) == Action::RetainLastValid
                && dswros::classify_world_map_owned_host_application_failure(
                    false, true, true) == Action::Fault
                && dswros::classify_world_map_owned_host_application_failure(
                    true, false, true) == Action::Fault
                && dswros::classify_world_map_owned_host_application_failure(
                    true, true, false) == Action::Fault,
            "a transient owned-host application exception may retain only an exact live layer with a prior verified transform and revalidated Mod-owned hosts");

        bool four_gate_truth_table_matches = true;
        for (std::uint32_t mask = 0; mask < 16U; ++mask) {
            const bool target = dswros::world_map_host_visibility_target({
                (mask & 1U) != 0U,
                (mask & 2U) != 0U,
                (mask & 4U) != 0U,
                (mask & 8U) != 0U,
            });
            four_gate_truth_table_matches =
                four_gate_truth_table_matches && target == (mask == 15U);
        }
        require(four_gate_truth_table_matches,
            "world-map hosts must be visible only when content intent, runtime visibility, attachment, and transform readiness are all true");

        std::optional<bool> applied_visibility;
        std::size_t visibility_write_count{};
        constexpr std::array<bool, 5> visibility_targets{
            false, false, true, true, false};
        for (const bool target : visibility_targets) {
            if (dswros::world_map_host_visibility_write_required(
                    applied_visibility, target)) {
                ++visibility_write_count;
                applied_visibility = target;
            }
        }
        require(visibility_write_count == 3U && applied_visibility
                    && !*applied_visibility,
            "world-map host visibility writes must publish unknown, edge, and reverse-edge states exactly once");

        const auto retry_later_visibility =
            dswros::world_map_transform_visibility_policy(
                Action::RetryHidden);
        const auto retained_visibility =
            dswros::world_map_transform_visibility_policy(
                Action::RetainLastValid);
        const auto fault_visibility =
            dswros::world_map_transform_visibility_policy(Action::Fault);
        require(retry_later_visibility.retain_host
                    && !retry_later_visibility.transform_ready
                    && retry_later_visibility.force_collapsed
                    && retained_visibility.retain_host
                    && retained_visibility.transform_ready
                    && !retained_visibility.force_collapsed
                    && !fault_visibility.retain_host
                    && !fault_visibility.transform_ready
                    && fault_visibility.force_collapsed,
            "RetryLater must retain one collapsed host, Retained must keep the last verified transform, and hard failures must detach");

        dswros::WorldMapHostVisibilityInputs lifecycle{
            true, true, true, true};
        std::optional<bool> lifecycle_applied{true};
        std::size_t retry_collapse_writes{};
        std::size_t retained_restore_writes{};
        const auto apply_lifecycle_policy = [&lifecycle, &lifecycle_applied](
                dswros::WorldMapTransformVisibilityPolicy policy) {
            lifecycle.attached = policy.retain_host;
            lifecycle.transform_ready = policy.transform_ready;
            const bool target = !policy.force_collapsed
                && dswros::world_map_host_visibility_target(lifecycle);
            const bool write =
                dswros::world_map_host_visibility_write_required(
                    lifecycle_applied, target);
            if (write) {
                lifecycle_applied = target;
            }
            return write;
        };
        retry_collapse_writes +=
            apply_lifecycle_policy(retry_later_visibility) ? 1U : 0U;
        retry_collapse_writes +=
            apply_lifecycle_policy(retry_later_visibility) ? 1U : 0U;
        retained_restore_writes +=
            apply_lifecycle_policy(retained_visibility) ? 1U : 0U;
        retained_restore_writes +=
            apply_lifecycle_policy(retained_visibility) ? 1U : 0U;
        require(retry_collapse_writes == 1U
                    && retained_restore_writes == 1U
                    && lifecycle.content_intent && lifecycle.runtime_allowed
                    && lifecycle.attached && lifecycle.transform_ready
                    && lifecycle_applied && *lifecycle_applied,
            "repeated RetryLater must collapse once without clearing durable intent, then repeated Retained must restore once without detaching");

        // Regression for the logged 2.2.0 sequence: a valid attachment remains
        // live across a fourth-sample geometry exception and continues on the
        // fifth sample. The transient must not detach, fault, or clear markers.
        constexpr std::array<Stage, 5> samples{
            Stage::None, Stage::None, Stage::None,
            Stage::GeometryObservation, Stage::None};
        bool attached = true;
        bool valid_transform = true;
        std::size_t marker_count = 1632;
        std::uint32_t detach_count = 0;
        std::uint32_t fault_count = 0;
        std::uint32_t retained_count = 0;
        for (const auto sample : samples) {
            if (sample == Stage::None) {
                continue;
            }
            const auto action = classify(
                dswros::world_map_transform_failure_for_stage(sample),
                attached, valid_transform);
            if (action == Action::RetainLastValid) {
                ++retained_count;
                continue;
            }
            if (action == Action::Fault) {
                attached = false;
                marker_count = 0;
                ++detach_count;
                ++fault_count;
            }
        }
        require(attached && valid_transform && marker_count == 1632
                    && detach_count == 0 && fault_count == 0
                    && retained_count == 1,
            "Updated-Unchanged-Unchanged-transient-Unchanged must preserve the attached atlas and its marker payload");

        // Once attachment verifies native parent A, transient empty or B
        // observations must not switch the coordinate witness. A real layer
        // replacement is handled by a separate attachment lifecycle.
        constexpr std::array<std::int32_t, 4> observed_parent_ids{
            1, 0, 2, 1};
        std::int32_t retained_parent_id = 1;
        std::uint32_t parent_switch_count = 0;
        for (const auto observed_parent_id : observed_parent_ids) {
            if (observed_parent_id == retained_parent_id) {
                continue;
            }
            const auto action = classify(
                Failure::NativeCanvasUnavailable, true, true);
            require(action == Action::RetainLastValid,
                "same-layer empty or alternate native-parent observations must retain the verified coordinate witness");
            (void)observed_parent_id;
        }
        require(retained_parent_id == 1 && parent_switch_count == 0,
            "the attached world-map coordinate witness must not oscillate A-B-A during layout settlement");
    }
    {
        const auto anchor_only =
            dswros::world_map_parent_extent_maximum_delta(
                {1500.0, 1500.0, 3440.0, 3440.0},
                {2200.0, 900.0, 3440.0, 3440.0});
        const auto delayed_extent =
            dswros::world_map_parent_extent_maximum_delta(
                {1500.0, 1500.0, 3440.0, 3440.0},
                {1500.0, 1500.0, 3656.742, 3440.0});
        require(anchor_only && near(*anchor_only, 0.0)
                    && delayed_extent
                    && near(*delayed_extent, 216.742)
                    && !dswros::world_map_parent_extent_maximum_delta(
                        {1500.0, 1500.0, 0.0, 3440.0},
                        {1500.0, 1500.0, 3656.742, 3440.0}),
                "same-parent correction must ignore normal pan or zoom anchors while detecting the logged delayed Canvas extent change");
    }
    {
        // CD41 put atlas_left=-320.7263 and atlas_width=2870.7947 on the
        // native-parent slot, inflating a 3000-wide Canvas to 3191.521 and
        // starting an attach/rebuild loop. A single observation of that exact
        // drift must retain the current payload; only a consecutive stable
        // sample may qualify the bounded rebuild.
        constexpr double inflated_parent_width =
            2870.7947 - (-320.7263);
        require(near(inflated_parent_width, 3191.521),
            "the CD41 outer-atlas regression fixture must reproduce the logged 3191.521 parent width");

        bool sample_valid{};
        dswros::WorldMapGeometrySample retained{};
        double maximum_delta{-1.0};
        const dswros::WorldMapGeometrySample inflated_geometry{
            1500.0, 1500.0, inflated_parent_width, 3000.0};
        const auto first = dswros::observe_world_map_geometry_sample(
            sample_valid, retained, inflated_geometry, maximum_delta);
        const auto second = dswros::observe_world_map_geometry_sample(
            sample_valid, retained, inflated_geometry, maximum_delta);
        require(first == dswros::WorldMapGeometryStabilityResult::Seeded
                    && second
                        == dswros::WorldMapGeometryStabilityResult::Stable,
            "the first CD41-style extent drift must retain the visible payload and only the second matching successful sample may request rebuild");

        sample_valid = false;
        retained = {};
        maximum_delta = -1.0;
        const auto after_reset = dswros::observe_world_map_geometry_sample(
            sample_valid, retained, inflated_geometry, maximum_delta);
        require(after_reset
                    == dswros::WorldMapGeometryStabilityResult::Seeded,
            "a cleared extent-stability sample must not reuse a prior observation to request rebuild");
    }
    {
        bool sample_valid{};
        dswros::WorldMapGeometrySample retained{};
        double maximum_delta{-1.0};
        const auto seeded = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {1940.0, 720.0, 3440.0, 1440.0}, maximum_delta);
        require(seeded
                    == dswros::WorldMapGeometryStabilityResult::Seeded
                    && sample_valid && near(maximum_delta, 0.0)
                    && near(retained.player_canvas_x, 1940.0)
                    && near(retained.parent_width, 3440.0),
                "the first valid world-map geometry sample must seed numeric state without accepting attachment");

        const auto replaced = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.0, 720.0, 3440.0, 1440.0}, maximum_delta);
        require(replaced
                    == dswros::WorldMapGeometryStabilityResult::Replaced
                    && near(maximum_delta, 300.0)
                    && near(retained.player_canvas_x, 2240.0),
                "a changed second world-map geometry sample must replace numeric state without accepting attachment");

        const auto stable = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.25, 719.75, 3440.25, 1439.75}, maximum_delta);
        require(stable
                    == dswros::WorldMapGeometryStabilityResult::Stable
                    && near(maximum_delta, 0.25)
                    && std::string_view{
                        dswros::world_map_geometry_stability_name(stable)}
                        == "stable",
                "the third existing attempt may accept geometry stable within half a Slate unit");

        const auto invalid = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.0, 720.0, 0.0, 1440.0}, maximum_delta);
        require(invalid
                    == dswros::WorldMapGeometryStabilityResult::None
                    && near(retained.parent_width, 3440.25),
                "an invalid geometry observation must not overwrite the retained numeric sample");
    }
    {
        require(dswros::world_map_attach_failure_retryable(3U)
                    && dswros::world_map_attach_failure_retryable(9U)
                    && dswros::world_map_attach_failure_retryable(24U)
                    && !dswros::world_map_attach_failure_retryable(2U)
                    && !dswros::world_map_attach_failure_retryable(10U)
                    && !dswros::world_map_attach_failure_retryable(100U),
                "world-map attachment must use one retryable-failure policy for widget and Slate geometry readiness");
        require(!dswros::world_map_attach_attempt_is_terminal(false, 24U),
                "a failed Slate-geometry attachment must leave the current-session retry latch open");
        require(!dswros::world_map_attach_attempt_is_terminal(false, 3U)
                    && dswros::world_map_attach_attempt_is_terminal(false, 2U)
                    && dswros::world_map_attach_attempt_is_terminal(false, 10U)
                    && dswros::world_map_attach_attempt_is_terminal(true, 24U),
                "world-map attachment must latch only success or a non-retryable failure");
    }
    {
        const auto standard_anchor =
            dswros::validate_world_map_canvas_anchor(
                1280.0, 720.0, 2560.0, 1440.0, 3000.0);
        require(standard_anchor && near(standard_anchor->x, 1280.0)
                    && near(standard_anchor->y, 720.0),
                "world-map geometry validation must accept a live 16:9 Canvas extent");

        const auto ultrawide_anchor =
            dswros::validate_world_map_canvas_anchor(
                1940.0, 720.0, 3440.0, 1440.0, 3000.0);
        require(ultrawide_anchor && near(ultrawide_anchor->x, 1940.0)
                    && near(ultrawide_anchor->y, 720.0),
                "world-map geometry validation must accept a live 21:9 Canvas extent and offset");

        const auto sixteen_ten_anchor =
            dswros::validate_world_map_canvas_anchor(
                1280.0, 900.0, 2560.0, 1600.0, 3000.0);
        require(sixteen_ten_anchor && near(sixteen_ten_anchor->x, 1280.0)
                    && near(sixteen_ten_anchor->y, 900.0),
                "world-map geometry validation must accept a live 16:10 Canvas extent and offset");

        const auto arbitrary_positive_extent =
            dswros::validate_world_map_canvas_anchor(
                0.5, 0.5, 1.0, 1.0, 3000.0);
        require(arbitrary_positive_extent
                    && near(arbitrary_positive_extent->x, 0.5)
                    && near(arbitrary_positive_extent->y, 0.5),
                "world-map geometry validation must not impose an authored-size assumption on a finite positive Canvas");

        require(!dswros::validate_world_map_canvas_anchor(
                    -200.0, 720.0, 3440.0, 1440.0, 3000.0),
                "world-map geometry validation must reject an implausible stale player anchor");
        require(!dswros::validate_world_map_canvas_anchor(
                    3500.0, 720.0, 3440.0, 1440.0, 3000.0),
                "world-map geometry validation must reject a point outside its exact native Canvas");
        require(!dswros::validate_world_map_canvas_anchor(
                    0.0, 0.0, 0.0, 1440.0, 3000.0),
                "world-map geometry validation must reject a non-positive parent extent");

        const auto same_parent = dswros::retain_world_map_atlas_placement(
            {-410.0, -275.0, 2048.0, 2048.0},
            {2460.347, 2001.596, 3000.0, 3000.0},
            {2460.347, 2001.596, 3000.0, 3000.0});
        require(same_parent && near(same_parent->left, -410.0)
                    && near(same_parent->top, -275.0)
                    && near(same_parent->width, 2048.0)
                    && near(same_parent->height, 2048.0),
                "same-anchor world-map validation must preserve atlas bounds");

        // Exact 5C632820 runtime fixture. The native parent identity and
        // 3000x3000 extent never changed, but six PlayerIcon observations
        // moved while zoom animated. Those sibling-anchor samples must all
        // retain the one placement authored at attach.
        constexpr dswros::WorldMapAtlasPlacement logged_placement{
            -320.726335, 118.721024, 2870.794736, 2341.547368};
        constexpr dswros::WorldMapGeometrySample logged_attach_geometry{
            1189.152, 2005.302, 3000.0, 3000.0};
        constexpr std::array<dswros::WorldMapGeometrySample, 6>
            logged_zoom_geometry{{
                {1177.0187, 1941.7708, 3000.0, 3000.0},
                {1189.1520, 2005.3020, 3000.0, 3000.0},
                {1180.8932, 1962.0581, 3000.0, 3000.0},
                {1189.1520, 2005.3020, 3000.0, 3000.0},
                {1195.1682, 1945.7388, 3000.0, 3000.0},
                {1211.9684, 1779.4022, 3000.0, 3000.0},
            }};
        for (const auto& observed : logged_zoom_geometry) {
            const auto retained =
                dswros::retain_world_map_atlas_placement(
                    logged_placement, logged_attach_geometry, observed);
            require(retained
                        && near(retained->left, logged_placement.left)
                        && near(retained->top, logged_placement.top)
                        && near(retained->width, logged_placement.width)
                        && near(retained->height, logged_placement.height),
                "5C632820 same-parent zoom anchors must never move the retained inner atlas slot");
        }

        require(!dswros::retain_world_map_atlas_placement(
                    {-410.0, -275.0, 2048.0, 2048.0},
                    {2460.347, 2001.596, 3000.0, 3000.0},
                    {3145.620, 2559.683, 3840.0, 3840.0}),
                "a 3000-to-3840 parent-local extent change must request a fresh atlas instead of scaling marker glyphs");

        require(!dswros::retain_world_map_atlas_placement(
                    {-410.0, -275.0, 0.0, 2048.0},
                    {2460.347, 2001.596, 3000.0, 3000.0},
                    {2480.347, 1991.596, 3000.0, 3000.0})
                    && !dswros::retain_world_map_atlas_placement(
                        {-410.0, -275.0, 2048.0, 2048.0},
                        {2460.347, 2001.596, 3000.0, 3000.0},
                        {std::numeric_limits<double>::infinity(),
                         1991.596, 3000.0, 3000.0}),
                "world-map placement retention must reject invalid retained bounds or anchors");

        const auto player = dswros::project_world_map_point(
            3145.620, 2559.683,
            {100000.0, 200000.0, 0.0},
            {100000.0, 200000.0, 0.0},
            570000.0, 3840.0, 3840.0);
        require(player && near(player->x, 3145.620)
                    && near(player->y, 2559.683),
                "world-map projection must preserve the logged 21:9 player anchor in the exact 3840-square parent-local space");

        const auto sixteen_nine_offset = dswros::project_world_map_point(
            2460.347, 2001.596,
            {100000.0, 200000.0, 0.0},
            {157000.0, 86000.0, 0.0},
            570000.0, 3000.0, 3000.0);
        require(sixteen_nine_offset
                    && near(sixteen_nine_offset->x, 2760.347)
                    && near(sixteen_nine_offset->y, 1401.596),
                "the logged 16:9 parent-local extent must scale both world deltas by 3000");

        const auto twenty_one_nine_offset = dswros::project_world_map_point(
            3145.620, 2559.683,
            {100000.0, 200000.0, 0.0},
            {157000.0, 86000.0, 0.0},
            570000.0, 3840.0, 3840.0);
        require(twenty_one_nine_offset
                    && near(twenty_one_nine_offset->x, 3529.620)
                    && near(twenty_one_nine_offset->y, 1791.683),
                "the logged 21:9 parent-local extent must scale world deltas by 3840 instead of authored ui_size 3000");
        require(!dswros::project_world_map_point(
                    1940.0, 720.0,
                    {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                    0.0, 3840.0, 3840.0)
                    && !dswros::project_world_map_point(
                        1940.0, 720.0,
                        {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                        570000.0, 0.0, 3840.0),
                "world-map projection must reject invalid map dimensions");
    }
    {
        dswros::CompactRenderModel model;
        std::array<dswros::CompactTreasureMarker, 1> output{};
        const std::array<std::uint8_t, 0> eligibility{};
        const auto result = model.refresh(
            {0.0, 0.0, 0.0}, true, 12500.0, eligibility, output);
        require(result.status == dswros::CompactRefreshStatus::NotInitialized,
                "compact refresh must fail closed before catalog initialization");

        const std::array<dswros::CompactTreasureCatalogEntry, 1> invalid{{
            {1, 100, {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}, true},
        }};
        require(!model.initialize(invalid) && !model.initialized(),
                "compact catalog initialization must reject invalid coordinates");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 6> catalog{{
            {1, 100, {0.0, -12500.0, 0.0}, false,
             dswros::CompactTreasureKind::Puzzle},
            {2, 100, {0.0, -12500.1, 0.0}, false},
            {3, 200, {100.0, 0.0, 0.0}, false},
            {4, 100, {200.0, 0.0, 0.0}, false},
            {5, 100, {22500.0, 0.0, 0.0}, false},
            {6, 100, {22500.1, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 6> eligibility{{1, 1, 1, 0, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 80> output{};
        require(model.initialize(catalog),
                "compact catalog initialization must accept finite entries");

        const auto compact = model.refresh(
            {0.0, 0.0, 0.0}, false, 12500.0, eligibility, output);
        require(compact.ok() && compact.count == 1 && output[0].id == 1,
                "125 metre planar boundary must be inclusive and filter map and eligibility");
        require(near(output[0].normalized_x, 0.0)
                    && near(output[0].normalized_y, -1.0),
                "compact output must expose normalized XY coordinates");
        require(output[0].kind
                    == dswros::CompactTreasureKind::Puzzle,
                "compact output must preserve the install-time treasure kind");

        const auto expanded = model.refresh(
            {0.0, 0.0, 0.0}, false, 22500.0, eligibility, output);
        require(expanded.ok() && expanded.count == 3,
                "225 metre planar boundary must be inclusive");
        require(marker_by_id(
                    {output.data(), expanded.count}, 5) != nullptr
                    && marker_by_id(
                        {output.data(), expanded.count}, 6) == nullptr,
                "points immediately beyond the 225 metre boundary must be excluded");
        require(marker_by_id(
                    {output.data(), expanded.count}, 3) == nullptr,
                "compact selection must accept only map_id 100");
        require(marker_by_id(
                    {output.data(), expanded.count}, 4) == nullptr,
                "compact selection must honor caller eligibility");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 3> catalog{{
            {10, 100, {300.0, 400.0, 0.0}, false},
            {11, 100, {0.0, 500.0, 0.0}, false},
            {12, 100, {1000.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 3> eligibility{{1, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 3> output{};
        std::array<dswros::CompactTreasureMarker, 3> repeated{};
        require(model.initialize(catalog),
                "tie-order catalog must initialize");
        const auto first = model.refresh(
            {0.0, 0.0, 0.0}, false, 2000.0, eligibility, output);
        const auto second = model.refresh(
            {0.0, 0.0, 0.0}, false, 2000.0, eligibility, repeated);
        require(first.ok() && first.count == 3
                    && output[0].id == 10 && output[1].id == 11,
                "equal 3D ranks must retain deterministic catalog order");
        require(second.ok() && repeated[0].id == output[0].id
                    && repeated[1].id == output[1].id
                    && repeated[2].id == output[2].id,
                "compact refresh ordering must be stable across samples");
        std::size_t nearest_count = 0;
        for (std::size_t index = 0; index < first.count; ++index) {
            nearest_count += output[index].nearest ? 1U : 0U;
        }
        require(nearest_count == 1 && output[0].nearest,
                "exactly the closest selected treasure must be marked nearest");
        require(!output[1].nearest,
                "the second closest treasure must not receive nearest emphasis");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 2> catalog{{
            {20, 100, {100.0, 0.0, 850.0}, true},
            {21, 100, {50.0, 0.0, 1000.0}, true},
        }};
        const std::array<std::uint8_t, 2> eligibility{{1, 1}};
        std::array<dswros::CompactTreasureMarker, 2> output{};
        require(model.initialize(catalog),
                "3D rank catalog must initialize");
        const auto result = model.refresh(
            {0.0, 0.0, 1000.0}, true, 1000.0, eligibility, output);
        require(result.ok() && result.count == 2 && output[0].id == 20,
                "3D nearest rank must compare treasure Z with player Z minus 150");
        require(near(output[0].ranking_distance_squared, 10000.0),
                "adjusted player Z must contribute correctly to 3D distance");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 5> catalog{{
            {30, 100, {10.0, 0.0, 950.0}, true},
            {31, 100, {20.0, 0.0, 951.0}, true},
            {32, 100, {30.0, 0.0, 1.0e12}, true},
            {33, 100, {40.0, 0.0, -1.0e12}, true},
            {34, 100, {50.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 5> eligibility{{1, 1, 1, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 5> output{};
        require(model.initialize(catalog),
                "height catalog must initialize");
        const auto result = model.refresh(
            {0.0, 0.0, 1000.0}, true, 100.0, eligibility, output);
        require(result.ok() && result.count == 5,
                "height test entries must remain inside the planar range");
        const auto markers = std::span<const dswros::CompactTreasureMarker>(
            output.data(), result.count);
        const auto* dead_zone = marker_by_id(markers, 30);
        const auto* above = marker_by_id(markers, 31);
        const auto* upper_clamp = marker_by_id(markers, 32);
        const auto* lower_clamp = marker_by_id(markers, 33);
        const auto* no_height = marker_by_id(markers, 34);
        require(dead_zone && dead_zone->height_available
                    && near(dead_zone->height_target_z, 950.0)
                    && near(dead_zone->height_delta, 0.0)
                    && near(dead_zone->height_angle_degrees, 0.0),
                "height delta at the 100 unit deadzone boundary must collapse to zero");
        const double expected_above_angle =
            -std::atan(1.0 / dswros::CompactRenderModel::kHeightSensitivity)
            * 57.2957795130823208768;
        require(above && near(above->height_delta, 101.0)
                    && near(above->height_angle_degrees, expected_above_angle),
                "height angle outside the deadzone must use the accepted atan mapping");
        require(upper_clamp
                    && near(upper_clamp->height_angle_degrees, -85.0),
                "positive height delta angle must clamp at negative 85 degrees");
        require(lower_clamp
                    && near(lower_clamp->height_angle_degrees, 85.0),
                "negative height delta angle must clamp at positive 85 degrees");
        require(no_height && !no_height->height_available
                    && near(no_height->height_delta, 0.0)
                    && near(no_height->height_angle_degrees, 0.0),                "entries without Z must not synthesize height output");
        require(near(
                    dswros::CompactRenderModel::height_angle_from_delta(100.0),
                    0.0)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(101.0),
                        expected_above_angle)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(-101.0),
                        -expected_above_angle)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(
                            std::numeric_limits<double>::quiet_NaN()),
                        0.0),
                "scalar height updates must be continuous at the deadzone and fail closed");
        require(
            dswros::mini_game_height_indicator_shape(
                dswros::CompactRenderModel::
                    mini_game_height_angle_from_delta(500.0))
                    == dswros::MiniGameHeightIndicatorShape::Hidden
                && dswros::mini_game_height_indicator_shape(
                    dswros::CompactRenderModel::
                        mini_game_height_angle_from_delta(501.0))
                    == dswros::MiniGameHeightIndicatorShape::Above
                && dswros::mini_game_height_indicator_shape(
                    dswros::CompactRenderModel::
                        mini_game_height_angle_from_delta(-501.0))
                    == dswros::MiniGameHeightIndicatorShape::Below
                && dswros::mini_game_height_indicator_shape(
                    dswros::CompactRenderModel::
                        mini_game_height_angle_from_delta(
                            std::numeric_limits<double>::quiet_NaN()))
                    == dswros::MiniGameHeightIndicatorShape::Hidden,
            "shared Fly/Mole/Wave height state must hide inside the 500-unit deadzone, map higher targets to an up triangle, map lower targets to a down triangle, and fail closed");
        require(
            near(dswros::CompactRenderModel::comparable_player_z(1000.0),
                 850.0)
                && std::isnan(
                    dswros::CompactRenderModel::comparable_player_z(
                        std::numeric_limits<double>::quiet_NaN())),
            "all compact height channels must share the explicit player-root calibration and fail closed for non-finite input");
        dswros::AreaQuestHeightProfile single_band{};
        single_band.bands[0] = {1000.0, 1000.0};
        single_band.band_count = 1;
        dswros::AreaQuestHeightProfile two_bands{};
        two_bands.bands[0] = {5150.0, 5218.0};
        two_bands.bands[1] = {6620.0, 6620.0};
        two_bands.band_count = 2;
        const auto marker_selected_band =
            dswros::area_quest_height_profile_for_marker(
                two_bands, 6576.0);
        dswros::AreaQuestHeightProfile tied_bands{};
        tied_bands.bands[0] = {0.0, 0.0};
        tied_bands.bands[1] = {1000.0, 1000.0};
        tied_bands.band_count = 2;
        dswros::AreaQuestHeightProfile unavailable{};
        require(
            dswros::area_quest_height_indicator_shape(
                single_band, 500.0)
                    == dswros::AreaQuestHeightIndicatorShape::Aligned
                && dswros::area_quest_height_indicator_shape(
                       single_band, 1500.0)
                    == dswros::AreaQuestHeightIndicatorShape::Aligned
                && dswros::area_quest_height_indicator_shape(
                       single_band, 499.0)
                    == dswros::AreaQuestHeightIndicatorShape::Above
                && dswros::area_quest_height_indicator_shape(
                       single_band, 1501.0)
                    == dswros::AreaQuestHeightIndicatorShape::Below
                && dswros::area_quest_height_indicator_shape(
                       two_bands, 4000.0)
                    == dswros::AreaQuestHeightIndicatorShape::Above
                && dswros::area_quest_height_indicator_shape(
                       two_bands, 5150.0)
                    == dswros::AreaQuestHeightIndicatorShape::Aligned
                && dswros::area_quest_height_indicator_shape(
                       two_bands, 5900.0)
                    == dswros::AreaQuestHeightIndicatorShape::Unavailable
                && marker_selected_band.band_count == 1
                && near(marker_selected_band.bands[0].minimum_z, 6620.0)
                && near(marker_selected_band.bands[0].maximum_z, 6620.0)
                && dswros::area_quest_height_indicator_shape(
                       marker_selected_band, 6120.0)
                    == dswros::AreaQuestHeightIndicatorShape::Aligned
                && dswros::area_quest_height_indicator_shape(
                       marker_selected_band, 6119.0)
                    == dswros::AreaQuestHeightIndicatorShape::Above
                && !dswros::area_quest_height_profile_valid(
                    dswros::area_quest_height_profile_for_marker(
                        tied_bands, 500.0))
                && dswros::area_quest_height_indicator_shape(
                       two_bands, 6620.0)
                    == dswros::AreaQuestHeightIndicatorShape::Aligned
                && dswros::area_quest_height_indicator_shape(
                       two_bands, 7200.0)
                    == dswros::AreaQuestHeightIndicatorShape::Below
                && dswros::area_quest_height_indicator_shape(
                       unavailable, 0.0)
                    == dswros::AreaQuestHeightIndicatorShape::Unavailable
                && dswros::area_quest_height_indicator_shape(
                       single_band,
                       std::numeric_limits<double>::quiet_NaN())
                    == dswros::AreaQuestHeightIndicatorShape::Unavailable,
            "Area Quest height UI must include the +/-500-unit boundaries, select the uniquely nearest task-actor band from the authored marker Z, and fail closed for a tied or missing source");
    }
    {
        std::array<dswros::CompactTreasureMarker, 3> candidates{};
        candidates[0].id = 1;
        candidates[0].ranking_distance_squared = 1000.0 * 1000.0;
        candidates[1].id = 2;
        candidates[1].ranking_distance_squared = 1050.0 * 1050.0;
        candidates[2].id = 3;
        candidates[2].ranking_distance_squared = 1400.0 * 1400.0;
        const auto retained = dswros::choose_compact_nearest(
            candidates, 2, 100.0);
        require(retained.index == 1 && retained.retained_non_best,
                "one-metre nearest hysteresis must retain a near-tied target");
        const auto switched = dswros::choose_compact_nearest(
            candidates, 2, 20.0);
        require(switched.index == 0 && !switched.retained_non_best,
                "a clearly closer target must bypass nearest hysteresis");
        const auto missing = dswros::choose_compact_nearest(
            candidates, 99, 100.0);
        require(missing.index == 0 && !missing.retained_non_best,
                "an unavailable retained target must switch immediately");
        const auto exact_boundary = dswros::choose_compact_nearest(
            candidates, 2, 50.0);
        require(exact_boundary.index == 1
                    && exact_boundary.retained_non_best,
                "the exact nearest-switch boundary must retain the target");
        const auto beyond_boundary = dswros::choose_compact_nearest(
            candidates, 2, 49.999);
        require(beyond_boundary.index == 0
                    && !beyond_boundary.retained_non_best,
                "a target beyond the nearest-switch boundary must switch");

        std::array<dswros::CompactTreasureMarker, 3> ordered{};
        ordered[0].id = 1;
        ordered[0].nearest = true;
        ordered[0].ranking_distance_squared = 1000.0 * 1000.0;
        ordered[1].id = 2;
        ordered[1].ranking_distance_squared = 1020.0 * 1020.0;
        ordered[2].id = 3;
        ordered[2].ranking_distance_squared = 1040.0 * 1040.0;
        require(dswros::promote_compact_nearest(ordered, 2),
                "a valid retained target must be promoted");
        require(ordered[0].id == 3 && ordered[0].nearest
                    && ordered[1].id == 1 && !ordered[1].nearest
                    && ordered[2].id == 2 && !ordered[2].nearest,
                "promotion must preserve the remaining distance order");
        require(!dswros::promote_compact_nearest(ordered, ordered.size()),
                "an invalid promotion index must fail closed");
    }
    {
        dswros::CompactRenderModel model;
        std::vector<dswros::CompactTreasureCatalogEntry> catalog;
        catalog.reserve(100);
        for (std::int64_t id = 1; id <= 100; ++id) {
            catalog.push_back({id, 100,
                               {static_cast<double>(id), 0.0, 0.0}, false});
        }
        const std::vector<std::uint8_t> eligibility(100, 1);
        std::array<dswros::CompactTreasureMarker, 80> output{};
        require(model.initialize(catalog),
                "maximum-selection catalog must initialize");
        const std::size_t allocations_before = allocation_count;
        const auto result = model.refresh(
            {0.0, 0.0, 0.0}, false, 200.0, eligibility, output);
        const std::size_t allocations_after = allocation_count;
        require(result.ok() && result.count == 80,
                "compact refresh must cap selected treasures at caller capacity 80");
        require(output.front().id == 1 && output.back().id == 80,
                "bounded selection must retain the nearest 80 treasures");
        require(allocations_after == allocations_before,
                "compact refresh must perform no dynamic allocation");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 1> catalog{{
            {1, 100, {10.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 1> eligibility{{1}};
        std::array<dswros::CompactTreasureMarker, 1> output{};
        std::array<dswros::CompactTreasureMarker, 81> oversized{};
        require(model.initialize(catalog),
                "invalid-input catalog must initialize");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 0.0, eligibility, output).status
                    == dswros::CompactRefreshStatus::InvalidInput,
                "zero compact radius must fail closed");
        require(model.refresh(
                    {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0},
                    false, 100.0, eligibility, output).status
                    == dswros::CompactRefreshStatus::InvalidInput,
                "non-finite player coordinates must fail closed");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0,
                    std::span<const std::uint8_t>{}, output).status
                    == dswros::CompactRefreshStatus::InvalidEligibility,
                "eligibility size must match the initialized catalog");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0, eligibility,
                    std::span<dswros::CompactTreasureMarker>{}).status
                    == dswros::CompactRefreshStatus::InvalidOutputCapacity,
                "zero output capacity must fail closed");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0, eligibility,
                    oversized).status
                    == dswros::CompactRefreshStatus::InvalidOutputCapacity,
                "output capacity above 80 must fail closed");
    }
    {
        const std::size_t allocations_before = allocation_count;
        const dswros::CompactMenuState visible{
            true, true, false, false, false, false};
        require(!dswros::compact_render_suppressed(visible),
                "a valid gameplay surface with an enabled category must remain visible");
        require(dswros::compact_render_suppressed({
                    false, true, false, false, false, false}),
                "zero enabled compact categories must suppress rendering");
        require(dswros::compact_render_suppressed({
                    true, false, false, false, false, false}),
                "an invalid player position must suppress rendering");
        require(dswros::compact_render_suppressed({
                    true, true, true, false, false, false}),
                "a cursor-owning menu must suppress rendering");
        require(dswros::compact_render_suppressed({
                    true, true, false, true, false, false}),
                "a visible world map must suppress rendering without a mouse cursor");
        require(dswros::compact_render_suppressed({
                    true, true, false, false, true, false}),
                "a paused game must suppress rendering without a mouse cursor");
        require(dswros::compact_render_suppressed({
                    true, true, false, false, false, true}),
                "a confirmed non-open-world activity must suppress rendering");
        require(allocation_count == allocations_before,
                "compact menu-state classification must not allocate");
    }
    {
        dswros::LiveMarkerPresenceGate gate;
        const std::size_t allocations_before = allocation_count;
        require(gate.sample(true, 1'000) && gate.visible(),
                "a live runtime marker must become visible immediately");
        require(gate.sample(false, 1'100) && gate.missing_pending(),
                "the first missing edge must start the debounce window");
        require(gate.sample(false, 1'499),
                "a runtime marker must survive the first 399 missing milliseconds");
        require(!gate.sample(false, 1'500) && !gate.visible(),
                "a runtime marker must disappear after 400 continuous missing milliseconds");
        require(!gate.sample(false, 2'000),
                "a marker that was never visible must remain absent");
        require(gate.sample(true, 3'000),
                "a new live sample must reactivate the marker");
        require(gate.sample(false, 3'050),
                "a second missing window must arm independently");
        require(gate.sample(true, 3'200) && !gate.missing_pending(),
                "a recovered live object must cancel the missing window");
        require(gate.sample(false, 3'250),
                "a later missing edge must start a fresh window");
        require(gate.sample(false, 3'100),
                "a backwards clock sample must fail safe by restarting the window");
        require(gate.sample(false, 3'499),
                "a restarted missing window must retain the marker for 399 milliseconds");
        require(!gate.sample(false, 3'500),
                "a restarted missing window must expire at exactly 400 milliseconds");
        require(gate.sample(true, 4'000),
                "a marker must reactivate after a completed disappearance window");
        gate.reset();
        require(!gate.visible() && !gate.missing_pending(),
                "an explicit lifecycle reset must clear all presence state");
        require(allocation_count == allocations_before,
                "live marker presence sampling must not allocate");
    }
    {
        const std::size_t allocations_before = allocation_count;
        require(dswros::bird_egg_interaction_available(2, 2),
                "the exact enum-backed On and NormalGather values must be available");
        require(!dswros::bird_egg_interaction_available(1, 2),
                "an Off bird-egg interaction switch must be unavailable");
        require(!dswros::bird_egg_interaction_available(2, 4),
                "a non-NormalGather bird-egg interaction type must be unavailable");
        require(dswros::bird_egg_runtime_present(
                    true, true, true, true),
                "an exact same-world interactable bird egg must be present");
        require(!dswros::bird_egg_runtime_present(
                    true, true, false, false),
                "an unknown bird-egg interaction state must fail closed");
        require(!dswros::bird_egg_runtime_present(
                    true, true, true, false),
                "a non-interactable bird egg must be absent");
        require(!dswros::bird_egg_runtime_present(
                    false, true, true, true),
                "an invalid bird-egg weak identity must be absent");
        require(!dswros::bird_egg_runtime_present(
                    true, false, true, true),
                "a bird egg from another world must be absent");
        require(allocation_count == allocations_before,
                "bird-egg enum-state classification must not allocate");
        require(dswros::is_main_menu_world_identity(
                    "World /Game/Title/TitleMap/DS_Title.DS_Title"),
                "the exact title world must be a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Maps/TransitionMap/EmptyTransitionMap.EmptyTransitionMap"),
                "the transition map must not become a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Art/Environment_Art/Maps/World/world_01_Main_WP/world_01_main_WP.World_01_Main_WP"),
                "the open world must not become a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Title/OtherMap.OtherMap"),
                "a broad title-path match must fail closed");
        require(allocation_count == allocations_before,
                "bird-egg and title-world policy checks must not allocate");
    }
    std::cout << "NATIVE_STATE_TESTS_OK assertions="
              << assertion_count << '\n';
    return 0;
}
