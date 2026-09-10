#pragma once

#include <dswros/radar_preferences.hpp>
#include <dswros/radar_confirmation.hpp>
#include <dswros/hub_viewport_layout.hpp>

#include <Unreal/FWeakObjectPtr.hpp>

#include <array>
#include <cstddef>
#include <chrono>
#include <cstdint>
#include <filesystem>

namespace RC::Unreal {
class UClass;
class FProperty;
class FStructProperty;
class UFunction;
class UObject;
}

namespace dsnwr {

enum class RadarVisibilityCategory : std::uint8_t {
    Clock,
    Treasure,
    Boss,
    Assault,
    MiniGames,
    AreaQuests,
    BirdEggs,
    Count,
};

using RadarVisibilityMaskWord = std::uint32_t;

inline constexpr std::uint8_t kRadarVisibilityAllCategories = 0x7FU;
inline constexpr std::uint8_t kRadarVisibilityWorldCategories = 0x3EU;
inline constexpr std::uint8_t kRadarVisibilitySceneCategories = 0x32U;

[[nodiscard]] constexpr std::uint8_t radar_visibility_bit(
    RadarVisibilityCategory category) noexcept {
    return static_cast<std::uint8_t>(
        1U << static_cast<std::uint8_t>(category));
}

[[nodiscard]] constexpr RadarVisibilityMaskWord pack_radar_visibility_masks(
    std::uint8_t compact, std::uint8_t world,
    std::uint8_t scene = 0U) noexcept {
    return static_cast<RadarVisibilityMaskWord>(
        (compact & kRadarVisibilityAllCategories)
        | static_cast<RadarVisibilityMaskWord>(
            world & kRadarVisibilityWorldCategories) << 8U
        | static_cast<RadarVisibilityMaskWord>(
            scene & kRadarVisibilitySceneCategories) << 16U);
}

[[nodiscard]] constexpr std::uint8_t compact_radar_visibility_mask(
    RadarVisibilityMaskWord masks) noexcept {
    return static_cast<std::uint8_t>(
        masks & kRadarVisibilityAllCategories);
}

[[nodiscard]] constexpr std::uint8_t world_radar_visibility_mask(
    RadarVisibilityMaskWord masks) noexcept {
    return static_cast<std::uint8_t>(
        (masks >> 8U) & kRadarVisibilityWorldCategories);
}

[[nodiscard]] constexpr std::uint8_t scene_radar_visibility_mask(
    RadarVisibilityMaskWord masks) noexcept {
    return static_cast<std::uint8_t>(
        (masks >> 16U) & kRadarVisibilitySceneCategories);
}

inline constexpr RadarVisibilityMaskWord kDefaultRadarVisibilityMasks =
    pack_radar_visibility_masks(
        kRadarVisibilityAllCategories,
        kRadarVisibilityWorldCategories,
        kRadarVisibilitySceneCategories);

static_assert(sizeof(RadarVisibilityMaskWord) == 4U);
static_assert(scene_radar_visibility_mask(kDefaultRadarVisibilityMasks) == kRadarVisibilitySceneCategories);
static_assert(pack_radar_visibility_masks(0xFFU, 0xFFU, 0xFFU) == 0x323E7FU);
static_assert(compact_radar_visibility_mask(
    pack_radar_visibility_masks(0x40U, 0x04U, 0x20U)) == 0x40U);
static_assert(world_radar_visibility_mask(
    pack_radar_visibility_masks(0x40U, 0x04U, 0x20U)) == 0x04U);
static_assert(scene_radar_visibility_mask(
    pack_radar_visibility_masks(0x40U, 0x04U, 0x20U)) == 0x20U);

enum class RadarVisibilityHubState : std::uint32_t {
    Uninitialized,
    Ready,
    Open,
    Disabled,
    Faulted,
};

enum class RadarVisibilityHubAction : std::uint8_t {
    None,
    Opened,
    Applied,
    Closed,
    Rejected,
};

enum class RadarModStatus : std::uint8_t {
    Off,
    On,
    Fault,
};

enum class RadarVisibilityHubCommand : std::uint8_t {
    None,
    EnableMod,
    DisableMod,
    OpenBugReport,
    OpenEndorsement,
};

[[nodiscard]] constexpr RadarVisibilityHubCommand
radar_mod_action_for_status(RadarModStatus status) noexcept {
    return status == RadarModStatus::On
        ? RadarVisibilityHubCommand::DisableMod
        : RadarVisibilityHubCommand::EnableMod;
}

enum class RadarVisibilityHubFontSource : std::uint8_t {
    DTextBlockInheritedDefault,
    DTextBlockClassDefaultObject,
    TextBlockFallback,
    DTextBlockRenderScaleFallback,
    TextBlockRenderScaleFallback,
};

enum class AreaQuestDisplayMode : std::uint8_t {
    Available,
    AllUnfinished,
};

enum class AssaultDisplayMode : std::uint8_t {
    Current,
    All,
};

// One value carries all three renderer masks so a UI change cannot expose a
// partially updated category set to the owner. The owner should publish
// packed_masks in one game-thread transaction when action is Applied.
struct RadarVisibilityHubResult {
    RadarVisibilityHubAction action{RadarVisibilityHubAction::None};
    RadarVisibilityMaskWord packed_masks{kDefaultRadarVisibilityMasks};
    bool changed{};
    std::uint32_t failure{};
    AreaQuestDisplayMode area_quest_mode{AreaQuestDisplayMode::Available};
    AssaultDisplayMode assault_mode{AssaultDisplayMode::Current};
    dswros::HeightIndicatorMask height_indicators{
        dswros::kDefaultHeightIndicatorMask};
    dswros::RadarLanguagePreference language{
        dswros::RadarLanguagePreference::Auto};
    RadarVisibilityHubCommand command{RadarVisibilityHubCommand::None};
    dswros::SceneDisplaySettings scene_settings{};
    bool global_reset_requested{};
};

class RadarVisibilityHub final {
public:
    // Resolves immutable reflected metadata only. No widget is created and no
    // controller or world object is retained.
    void initialize(std::filesystem::path text_overlay_root) noexcept;

    // Opens a transient, game-owned UMG panel, or treats a second toggle as
    // Close. This must run on the game thread with the freshly resolved
    // current-world player controller. The controller is never retained.
    [[nodiscard]] RadarVisibilityHubResult toggle(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode,
        dswros::HeightIndicatorMask current_height_indicators =
            dswros::kDefaultHeightIndicatorMask,
        dswros::RadarLanguagePreference current_language =
            dswros::RadarLanguagePreference::Auto,
        dswros::RadarUiLanguage detected_game_language =
            dswros::RadarUiLanguage::English,
        RadarModStatus current_mod_status = RadarModStatus::Off,
        dswros::SceneDisplaySettings current_scene_settings = {}) noexcept;

    // Poll only while the panel is open. Closed calls return immediately and
    // perform no UObject work. Each changed selection returns all masks in
    // one packed value without closing the panel; X closes it.
    [[nodiscard]] RadarVisibilityHubResult service_open_panel(
        RC::Unreal::UObject* current_controller,
        RadarModStatus current_mod_status) noexcept;

    // Samples every control (including a slider's last drag value) before
    // closing. Used by both F6 and the already-consumed Escape request.
    [[nodiscard]] RadarVisibilityHubResult close(
        RC::Unreal::UObject* current_controller,
        RadarModStatus current_mod_status) noexcept;

    // Escape dismisses an active confirmation only. F6, focus loss and Travel
    // retain their explicit full-close path and always discard pending intent.
    [[nodiscard]] RadarVisibilityHubResult escape(
        RC::Unreal::UObject* current_controller,
        RadarModStatus current_mod_status) noexcept;
    [[nodiscard]] bool confirmation_open() const noexcept {
        return is_open() && confirmation_.active();
    }

    // Normal F8/shutdown close. Supplying the current controller allows the
    // hub to restore cursor/input state without retaining it between frames.
    void detach(RC::Unreal::UObject* current_controller = nullptr) noexcept;

    // Travel-safe fail-closed release. This intentionally calls no UObject
    // method because the previous world's widgets may already be stale.
    void release_for_travel() noexcept;

    [[nodiscard]] RadarVisibilityHubState state() const noexcept {
        return state_;
    }
    [[nodiscard]] bool is_open() const noexcept {
        return state_ == RadarVisibilityHubState::Open;
    }
    [[nodiscard]] bool owns_gameplay_cursor() const noexcept {
        return is_open() && owns_input_mode_;
    }
    [[nodiscard]] std::uint64_t open_count() const noexcept {
        return open_count_;
    }
    [[nodiscard]] std::uint64_t apply_count() const noexcept {
        return apply_count_;
    }
    [[nodiscard]] std::uint64_t close_count() const noexcept {
        return close_count_;
    }
    [[nodiscard]] std::uint64_t detach_count() const noexcept {
        return detach_count_;
    }
    [[nodiscard]] std::uint64_t fault_count() const noexcept {
        return fault_count_;
    }
    [[nodiscard]] std::uint32_t last_failure() const noexcept {
        return last_failure_;
    }
    [[nodiscard]] std::uint32_t abi_failure_mask() const noexcept {
        return abi_failure_mask_;
    }
    [[nodiscard]] std::uint32_t font_abi_detail_mask() const noexcept {
        return font_abi_detail_mask_;
    }
    [[nodiscard]] RadarVisibilityHubFontSource font_source() const noexcept {
        return font_source_;
    }
    [[nodiscard]] std::uint32_t font_fallback_reason() const noexcept {
        return font_fallback_reason_;
    }
    [[nodiscard]] std::uint32_t text_runtime_failure() const noexcept {
        return text_runtime_failure_;
    }
    [[nodiscard]] std::uint32_t text_overlay_failure() const noexcept {
        return text_overlay_failure_;
    }
    [[nodiscard]] bool text_overlay_active() const noexcept {
        return text_overlay_active_;
    }
    [[nodiscard]] bool font_fallback_active() const noexcept {
        return font_source_
            != RadarVisibilityHubFontSource::DTextBlockClassDefaultObject;
    }

private:
    static constexpr std::size_t kColumnCount = 3;
    static constexpr std::size_t kCategoryCount =
        static_cast<std::size_t>(RadarVisibilityCategory::Count);
    static constexpr std::size_t kHeightIndicatorCount =
        static_cast<std::size_t>(dswros::HeightIndicatorCategory::Count);
    static constexpr std::size_t kLanguageChoiceCount =
        dswros::kRadarLanguagePreferenceCount;
    static constexpr std::size_t kLanguagePopupDecorationCount =
        kLanguageChoiceCount + 3U;
    static constexpr std::size_t kLanguageFontCount =
        static_cast<std::size_t>(dswros::RadarUiFontFamily::Count);

    enum class LocalizedTextSlot : std::size_t {
        Title,
        Close,
        Language,
        LanguageValue,
        MarkerVisibility,
        Radar,
        Map,
        HeightIndicators,
        RadarOnly,
        FilterModes,
        AreaQuestMode,
        AssaultMode,
        AreaAvailable,
        AreaAll,
        AssaultAvailable,
        AssaultAll,
        StatusLabel,
        StatusValue,
        StatusAction,
        BugReport,
        SceneSettings,
        SceneRange,
        SceneLimit,
        SceneDistance,
        SceneDistanceOff,
        SceneDistanceCentral,
        SceneDistanceNearest,
        SceneDistanceAll,
        SceneTreasure,
        SceneAreaQuest,
        SceneMiniGame,
        GlobalReset,
        MarkerAll,
        Count,
    };
    static constexpr std::size_t kLocalizedTextCount =
        static_cast<std::size_t>(LocalizedTextSlot::Count);

    [[nodiscard]] RadarVisibilityHubResult open_guarded(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode,
        dswros::HeightIndicatorMask current_height_indicators,
        dswros::RadarLanguagePreference current_language,
        dswros::RadarUiLanguage detected_game_language,
        RadarModStatus current_mod_status,
        dswros::SceneDisplaySettings current_scene_settings) noexcept;
    [[nodiscard]] RadarVisibilityHubResult open_unsafe(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode,
        dswros::HeightIndicatorMask current_height_indicators,
        dswros::RadarLanguagePreference current_language,
        dswros::RadarUiLanguage detected_game_language,
        RadarModStatus current_mod_status,
        dswros::SceneDisplaySettings current_scene_settings);
    [[nodiscard]] RadarVisibilityHubResult service_guarded(
        RC::Unreal::UObject* current_controller,
        RadarModStatus current_mod_status, bool force_close = false,
        bool cancel_confirmation = false) noexcept;
    [[nodiscard]] RadarVisibilityHubResult service_unsafe(
        RC::Unreal::UObject* current_controller,
        RadarModStatus current_mod_status, bool force_close,
        bool cancel_confirmation);
    void detach_guarded(RC::Unreal::UObject* current_controller) noexcept;
    void detach_unsafe(RC::Unreal::UObject* current_controller);
    [[nodiscard]] bool refresh_localized_text_unsafe();
    [[nodiscard]] bool refresh_mod_status_unsafe(
        RadarModStatus current_mod_status);
    [[nodiscard]] bool set_language_popup_visibility_unsafe(bool visible);
    void refresh_guide_unsafe();
    void set_guide_visibility_unsafe(bool visible);
    [[nodiscard]] bool recenter_native_text_unsafe();
    [[nodiscard]] bool refresh_packaged_text_overlay_unsafe(
        RC::Unreal::UObject* world_context);
    [[nodiscard]] bool refresh_language_value_overlay_unsafe(
        RC::Unreal::UObject* world_context);
    [[nodiscard]] bool ensure_language_popup_overlay_unsafe(
        RC::Unreal::UObject* world_context);
    [[nodiscard]] RC::Unreal::UObject* import_text_overlay_unsafe(
        RC::Unreal::UObject* world_context,
        const std::filesystem::path& path);
    [[nodiscard]] bool apply_text_overlay_unsafe(
        RC::Unreal::UObject* image,
        RC::Unreal::UObject* texture);
    [[nodiscard]] bool set_native_text_visibility_unsafe(bool visible);
    void resolve_language_fonts_once_unsafe();
    [[nodiscard]] RC::Unreal::UObject* language_font_unsafe(
        dswros::RadarUiLanguage language) const noexcept;
    [[nodiscard]] bool apply_language_font_unsafe(
        RC::Unreal::UObject* text_block,
        dswros::RadarUiLanguage language) noexcept;
    void reset_runtime_handles() noexcept;
    void bind_tooltip_unsafe(RC::Unreal::UObject* control, std::uint8_t tooltip_id,
                             RC::Unreal::UObject* tree, double unit_scale);
    void create_tooltip_content_unsafe(std::size_t index, RC::Unreal::UObject* tree);
    void refresh_tooltips_unsafe();
    [[nodiscard]] bool set_confirmation_visibility_unsafe(
        bool visible, bool guard_background = false);
    [[nodiscard]] bool refresh_confirmation_text_unsafe();
    [[nodiscard]] bool refresh_numeric_text_unsafe();
    [[nodiscard]] bool apply_viewport_layout_unsafe(
        const dswros::HubViewportLayout& layout);
    [[nodiscard]] bool refresh_viewport_layout_unsafe(
        RC::Unreal::UObject* controller, bool& changed);
    [[nodiscard]] bool configure_chip_nine_slice_unsafe(RC::Unreal::UObject* image, double unit_scale);

    RC::Unreal::UClass* user_widget_class_{};
    RC::Unreal::UClass* widget_class_{};
    RC::Unreal::UClass* widget_tree_class_{};
    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* border_class_{};
    RC::Unreal::UClass* text_block_class_{};
    RC::Unreal::UClass* game_text_block_class_{};
    RC::Unreal::UClass* check_box_class_{};
    RC::Unreal::UClass* image_class_{};
    RC::Unreal::UClass* slider_class_{};
    RC::Unreal::UClass* size_box_class_{};
    RC::Unreal::UClass* scroll_box_class_{};
    RC::Unreal::UFunction* add_child_to_panel_{};
    RC::Unreal::UFunction* set_scroll_offset_{};
    RC::Unreal::UFunction* get_scroll_offset_{};
    RC::Unreal::UFunction* set_scroll_orientation_{};
    RC::Unreal::UFunction* set_scrollbar_visibility_{};
    RC::Unreal::UFunction* set_tool_tip_{};
    RC::Unreal::UFunction* set_tool_tip_text_{};
    RC::Unreal::FProperty* tool_tip_text_property_{};
    RC::Unreal::UFunction* set_content_{};
    RC::Unreal::UFunction* set_width_override_{};
    RC::Unreal::UFunction* set_height_override_{};
    RC::Unreal::UFunction* set_clipping_{};
    RC::Unreal::UFunction* set_image_brush_{};
    RC::Unreal::FStructProperty* image_brush_property_{};
    RC::Unreal::FStructProperty* set_image_brush_value_property_{};
    RC::Unreal::FStructProperty* brush_margin_property_{};
    RC::Unreal::FStructProperty* brush_image_size_property_{};
    RC::Unreal::FProperty* brush_draw_as_property_{};
    std::array<RC::Unreal::FProperty*, 6> brush_box_metrics_{};
    bool chip_nine_slice_abi_available_{};
    RC::Unreal::UFunction* set_slider_value_{};
    RC::Unreal::UFunction* get_slider_value_{};
    RC::Unreal::UFunction* set_slider_min_{};
    RC::Unreal::UFunction* set_slider_max_{};
    RC::Unreal::UFunction* set_slider_step_{};
    RC::Unreal::UFunction* set_slider_bar_color_{};
    RC::Unreal::UFunction* set_slider_handle_color_{};
    RC::Unreal::FProperty* text_block_font_property_{};
    RC::Unreal::FProperty* force_apply_language_font_property_{};
    RC::Unreal::UFunction* create_widget_{};
    RC::Unreal::UFunction* get_owning_player_{};
    RC::Unreal::UFunction* get_viewport_size_{};
    RC::Unreal::UFunction* get_viewport_scale_{};
    RC::Unreal::UFunction* add_to_viewport_{};
    RC::Unreal::UFunction* add_child_to_canvas_{};
    RC::Unreal::UFunction* set_slot_position_{};
    RC::Unreal::UFunction* set_slot_size_{};
    RC::Unreal::UFunction* set_slot_alignment_{};
    RC::Unreal::UFunction* set_slot_z_order_{};
    RC::Unreal::UFunction* set_visibility_{};
    RC::Unreal::UFunction* set_brush_color_{};
    RC::Unreal::UFunction* set_text_{};
    RC::Unreal::FProperty* set_text_value_property_{};
    RC::Unreal::UFunction* set_justification_{};
    RC::Unreal::UFunction* set_render_opacity_{};
    RC::Unreal::UFunction* set_render_scale_{};
    RC::Unreal::UFunction* set_render_pivot_{};
    RC::Unreal::UFunction* set_font_{};
    RC::Unreal::FStructProperty* set_font_value_property_{};
    RC::Unreal::UFunction* set_is_checked_{};
    RC::Unreal::UFunction* is_checked_{};
    RC::Unreal::UFunction* set_is_enabled_{};
    RC::Unreal::FProperty* set_is_enabled_value_property_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* get_desired_size_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::UFunction* set_input_mode_game_and_ui_{};
    RC::Unreal::UFunction* set_input_mode_game_only_{};
    RC::Unreal::UFunction* set_brush_from_texture_{};
    RC::Unreal::UFunction* import_file_as_texture_{};
    RC::Unreal::FWeakObjectPtr widget_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr widget_layout_library_{};
    RC::Unreal::FWeakObjectPtr kismet_rendering_library_{};
    RC::Unreal::FWeakObjectPtr game_text_block_default_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLanguageFontCount>
        language_fonts_{};
    RC::Unreal::FWeakObjectPtr host_{};
    RC::Unreal::FWeakObjectPtr widget_tree_{};
    RC::Unreal::FWeakObjectPtr root_panel_{};
    RC::Unreal::FWeakObjectPtr page_panel_{};
    RC::Unreal::FWeakObjectPtr page_slot_{};
    RC::Unreal::FWeakObjectPtr body_scroll_{};
    RC::Unreal::FWeakObjectPtr settings_body_{};
    RC::Unreal::FWeakObjectPtr body_size_{};
    RC::Unreal::FWeakObjectPtr guide_body_{};
    RC::Unreal::FWeakObjectPtr guide_control_{};
    RC::Unreal::FWeakObjectPtr guide_fallback_text_{};
    RC::Unreal::FWeakObjectPtr guide_texture_{};
    std::array<RC::Unreal::FWeakObjectPtr, 3> guide_images_{};
    std::array<RC::Unreal::FWeakObjectPtr, 2> guide_label_canvases_{};
    dswros::RadarUiLanguage guide_language_{dswros::RadarUiLanguage::Count};
    bool guide_open_{};
    bool guide_ready_{};
    float settings_scroll_offset_{};
    float guide_scroll_offset_{};
    RC::Unreal::FWeakObjectPtr body_scroll_slot_{};
    RC::Unreal::FWeakObjectPtr footer_slot_{};
    RC::Unreal::FWeakObjectPtr modal_slot_{};
    RC::Unreal::FWeakObjectPtr confirmation_dim_slot_{};
    RC::Unreal::FWeakObjectPtr confirmation_blocker_slot_{};
    RC::Unreal::FWeakObjectPtr popup_dim_slot_{};
    RC::Unreal::FWeakObjectPtr popup_dismiss_slot_{};
    double authored_unit_scale_{1.0};
    dswros::HubViewportLayout viewport_layout_{};
    std::chrono::steady_clock::time_point viewport_check_after_{};
    struct TooltipRecord {
        RC::Unreal::FWeakObjectPtr control{};
        RC::Unreal::FWeakObjectPtr content{};
        RC::Unreal::FWeakObjectPtr image{};
        RC::Unreal::FWeakObjectPtr image_slot{};
        std::uint8_t id{};
        double unit_scale{1.0};
    };
    static constexpr std::size_t kMaximumTooltipCount = 64;
    std::array<TooltipRecord, kMaximumTooltipCount> tooltips_{};
    // 51 settings targets plus the seven marker-row names share this fixed pool.
    static_assert(58U <= kMaximumTooltipCount);
    std::size_t tooltip_count_{};
    RC::Unreal::FWeakObjectPtr tooltip_atlas_{};
    dswros::RadarUiLanguage tooltip_atlas_language_{dswros::RadarUiLanguage::Count};
    bool tooltip_atlas_attempted_{};
    bool tooltip_widget_abi_available_{};
    RC::Unreal::FWeakObjectPtr main_text_overlay_image_{};
    std::array<RC::Unreal::FWeakObjectPtr, 3> main_text_overlay_images_{};
    RC::Unreal::FWeakObjectPtr popup_text_overlay_image_{};
    RC::Unreal::FWeakObjectPtr language_value_overlay_image_{};
    RC::Unreal::FWeakObjectPtr main_text_overlay_texture_{};
    RC::Unreal::FWeakObjectPtr popup_text_overlay_texture_{};
    // French and Spanish names only. The active Image brush owns its texture;
    // these bounded weak slots never keep an old transient tree alive.
    std::array<RC::Unreal::FWeakObjectPtr, 2> language_value_overlay_textures_{};
    std::array<std::uint32_t, 2> language_value_overlay_failures_{};
    using ControlHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kCategoryCount>;
    std::array<ControlHandles, kColumnCount> controls_{};
    std::array<ControlHandles, kColumnCount> enabled_visuals_{};
    std::array<RC::Unreal::FWeakObjectPtr, 2> column_all_controls_{};
    std::array<RC::Unreal::FWeakObjectPtr, 2> column_all_visuals_{};
    std::array<bool, 2> column_all_selected_{};
    using HeightControlHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kHeightIndicatorCount>;
    HeightControlHandles height_controls_{};
    HeightControlHandles height_enabled_visuals_{};
    RC::Unreal::FWeakObjectPtr language_dropdown_control_{};
    RC::Unreal::FWeakObjectPtr global_reset_control_{};
    RC::Unreal::FWeakObjectPtr language_popup_dismiss_control_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLanguagePopupDecorationCount>
        language_popup_decorations_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLanguageChoiceCount>
        language_choice_controls_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLanguageChoiceCount>
        language_choice_selected_visuals_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLanguageChoiceCount>
        language_choice_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, kLocalizedTextCount>
        localized_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCategoryCount>
        category_label_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, kHeightIndicatorCount>
        height_label_texts_{};
    RC::Unreal::FWeakObjectPtr area_mode_available_control_{};
    RC::Unreal::FWeakObjectPtr area_mode_all_control_{};
    RC::Unreal::FWeakObjectPtr area_mode_available_visual_{};
    RC::Unreal::FWeakObjectPtr area_mode_all_visual_{};
    RC::Unreal::FWeakObjectPtr assault_mode_current_control_{};
    RC::Unreal::FWeakObjectPtr assault_mode_all_control_{};
    RC::Unreal::FWeakObjectPtr assault_mode_current_visual_{};
    RC::Unreal::FWeakObjectPtr assault_mode_all_visual_{};
    RC::Unreal::FWeakObjectPtr mod_status_visual_{};
    RC::Unreal::FWeakObjectPtr mod_action_visual_{};
    RC::Unreal::FWeakObjectPtr mod_action_control_{};
    RC::Unreal::FWeakObjectPtr bug_report_control_{};
    RC::Unreal::FWeakObjectPtr endorsement_control_{};
    RC::Unreal::FWeakObjectPtr close_control_{};
    struct ConfirmationTextRecord {
        RC::Unreal::FWeakObjectPtr native_text{};
        RC::Unreal::FWeakObjectPtr canvas{};
        RC::Unreal::FWeakObjectPtr image{};
        RC::Unreal::FWeakObjectPtr image_slot{};
        double scale{1.0};
    };
    // Endorse entry, title, body, Yes and No. Images share the tooltip atlas.
    std::array<ConfirmationTextRecord, 5> confirmation_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, 5> confirmation_decorations_{};
    RC::Unreal::FWeakObjectPtr confirmation_blocker_{};
    RC::Unreal::FWeakObjectPtr confirmation_yes_control_{};
    RC::Unreal::FWeakObjectPtr confirmation_no_control_{};
    dswros::RadarConfirmation confirmation_{};
    bool confirmation_text_ready_{};
    bool confirmation_dismiss_guard_{};
    struct NumericTextRecord {
        RC::Unreal::FWeakObjectPtr canvas{};
        RC::Unreal::FWeakObjectPtr slot{};
        RC::Unreal::FWeakObjectPtr image{};
        RC::Unreal::FWeakObjectPtr image_slot{};
        double authored_x{};
        double authored_y{};
    };
    std::array<NumericTextRecord, 15> numeric_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, 3> placeholder_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, 2> scene_sliders_{};
    std::array<RC::Unreal::FWeakObjectPtr, 2> scene_value_texts_{};
    std::array<RC::Unreal::FWeakObjectPtr, 4> scene_distance_controls_{};
    std::array<RC::Unreal::FWeakObjectPtr, 4> scene_distance_visuals_{};
    dswros::SceneDisplaySettings source_scene_settings_{};
    dswros::SceneDisplaySettings pending_scene_settings_{};

    struct TextLayoutRecord {
        RC::Unreal::FWeakObjectPtr widget{};
        RC::Unreal::FWeakObjectPtr slot{};
        bool allow_desired_size_centering{};
        double authored_x{};
        double authored_y{};
        double authored_width{};
        double authored_height{};
    };
    static constexpr std::size_t kMaximumTextLayoutRecordCount = 80;
    std::array<TextLayoutRecord, kMaximumTextLayoutRecordCount>
        text_layout_records_{};
    std::size_t text_layout_record_count_{};

    RadarVisibilityHubState state_{RadarVisibilityHubState::Uninitialized};
    RadarVisibilityMaskWord source_masks_{kDefaultRadarVisibilityMasks};
    RadarVisibilityMaskWord pending_masks_{kDefaultRadarVisibilityMasks};
    AreaQuestDisplayMode source_area_quest_mode_{
        AreaQuestDisplayMode::Available};
    AreaQuestDisplayMode pending_area_quest_mode_{
        AreaQuestDisplayMode::Available};
    AssaultDisplayMode source_assault_mode_{AssaultDisplayMode::Current};
    AssaultDisplayMode pending_assault_mode_{AssaultDisplayMode::Current};
    dswros::HeightIndicatorMask source_height_indicators_{
        dswros::kDefaultHeightIndicatorMask};
    dswros::HeightIndicatorMask pending_height_indicators_{
        dswros::kDefaultHeightIndicatorMask};
    dswros::RadarLanguagePreference source_language_{
        dswros::RadarLanguagePreference::Auto};
    dswros::RadarLanguagePreference pending_language_{
        dswros::RadarLanguagePreference::Auto};
    dswros::RadarUiLanguage detected_game_language_{
        dswros::RadarUiLanguage::English};
    dswros::RadarUiLanguage resolved_ui_language_{
        dswros::RadarUiLanguage::English};
    RadarModStatus displayed_mod_status_{RadarModStatus::Off};
    dswros::RadarUiLanguage text_overlay_language_{
        dswros::RadarUiLanguage::Count};
    RadarModStatus text_overlay_status_{RadarModStatus::Off};
    std::filesystem::path text_overlay_root_{};
    bool language_dropdown_expanded_{};
    RadarVisibilityHubFontSource font_source_{
        RadarVisibilityHubFontSource::DTextBlockInheritedDefault};
    std::uint32_t font_fallback_reason_{};
    std::uint32_t text_runtime_failure_{};
    std::uint32_t text_overlay_failure_{};
    bool text_overlay_active_{};
    bool owns_input_mode_{};
    bool previous_cursor_visible_{};
    std::uint64_t open_count_{};
    std::uint64_t apply_count_{};
    std::uint64_t close_count_{};
    std::uint64_t detach_count_{};
    std::uint64_t fault_count_{};
    std::uint32_t last_failure_{};
    std::uint32_t abi_failure_mask_{};
    std::uint32_t font_abi_detail_mask_{};
    bool font_layout_abi_available_{};
};

} // namespace dsnwr
