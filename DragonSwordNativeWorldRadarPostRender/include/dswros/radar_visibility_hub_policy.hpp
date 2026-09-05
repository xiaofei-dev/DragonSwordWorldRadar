#pragma once

#include <cstdint>

namespace dswros {

inline constexpr std::uint32_t kRadarVisibilityHubEssentialDetailMask =
    (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U)
    | (1U << 4U) | (1U << 9U) | (1U << 10U) | (1U << 11U);
static_assert(kRadarVisibilityHubEssentialDetailMask == 0xE1FU);

[[nodiscard]] constexpr bool radar_visibility_hub_font_detail_is_fatal(
    std::uint32_t detail_mask) noexcept {
    return (detail_mask & kRadarVisibilityHubEssentialDetailMask) != 0;
}

enum class RadarVisibilityHubFontPlanSource : std::uint8_t {
    TextBlockFallback,
    DTextBlockInheritedDefault,
    DTextBlockClassDefaultObject,
};

struct RadarVisibilityHubFontPlan {
    bool use_game_text_widget{};
    bool copy_game_default_font{};
    RadarVisibilityHubFontPlanSource source{
        RadarVisibilityHubFontPlanSource::TextBlockFallback};
};

[[nodiscard]] constexpr RadarVisibilityHubFontPlan
radar_visibility_hub_font_plan(
    bool game_text_widget_usable,
    bool game_default_font_compatible) noexcept {
    if (!game_text_widget_usable) {
        return {};
    }
    if (!game_default_font_compatible) {
        return {
            true,
            false,
            RadarVisibilityHubFontPlanSource::DTextBlockInheritedDefault};
    }
    return {
        true,
        true,
        RadarVisibilityHubFontPlanSource::DTextBlockClassDefaultObject};
}

enum class RadarVisibilityHubTextWidgetKind : std::uint8_t {
    GameTextBlock,
    BaseTextBlock,
};

struct RadarVisibilityHubTextPrepareAttempt {
    bool prepared{};
    std::uint32_t failure{};
};

struct RadarVisibilityHubTextPrepareResult {
    bool prepared{};
    RadarVisibilityHubTextWidgetKind selected{
        RadarVisibilityHubTextWidgetKind::BaseTextBlock};
    bool game_attempted{};
    bool base_attempted{};
    std::uint32_t fallback_reason{};
    std::uint32_t terminal_failure{};
};

struct RadarVisibilityHubCenteredTextSlot {
    double top{};
    double height{};
    bool centered{};
};

[[nodiscard]] constexpr RadarVisibilityHubCenteredTextSlot
center_radar_visibility_hub_text_slot(
    double authored_top,
    double authored_height,
    double desired_height) noexcept {
    if (!(authored_height > 0.0) || !(desired_height > 0.0)
        || desired_height > authored_height) {
        return {authored_top, authored_height, false};
    }
    return {
        authored_top + (authored_height - desired_height) * 0.5,
        desired_height,
        true};
}

template <typename Prepare>
[[nodiscard]] RadarVisibilityHubTextPrepareResult
prepare_radar_visibility_hub_text_with_fallback(
    bool use_game_text_widget,
    Prepare&& prepare) {
    RadarVisibilityHubTextPrepareResult result{};
    if (use_game_text_widget) {
        result.game_attempted = true;
        const RadarVisibilityHubTextPrepareAttempt game = prepare(
            RadarVisibilityHubTextWidgetKind::GameTextBlock);
        if (game.prepared) {
            result.prepared = true;
            result.selected =
                RadarVisibilityHubTextWidgetKind::GameTextBlock;
            return result;
        }
        result.fallback_reason = game.failure;
    }

    result.base_attempted = true;
    const RadarVisibilityHubTextPrepareAttempt base = prepare(
        RadarVisibilityHubTextWidgetKind::BaseTextBlock);
    result.prepared = base.prepared;
    result.selected = RadarVisibilityHubTextWidgetKind::BaseTextBlock;
    result.terminal_failure = base.prepared ? 0U : base.failure;
    return result;
}

} // namespace dswros
