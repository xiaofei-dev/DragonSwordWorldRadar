#pragma once

#include <Unreal/FWeakObjectPtr.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace RC::Unreal {
class UClass;
class FProperty;
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

using RadarVisibilityMaskWord = std::uint16_t;

inline constexpr std::uint8_t kRadarVisibilityAllCategories = 0x7FU;
inline constexpr std::uint8_t kRadarVisibilityWorldCategories = 0x3EU;

[[nodiscard]] constexpr std::uint8_t radar_visibility_bit(
    RadarVisibilityCategory category) noexcept {
    return static_cast<std::uint8_t>(
        1U << static_cast<std::uint8_t>(category));
}

[[nodiscard]] constexpr RadarVisibilityMaskWord pack_radar_visibility_masks(
    std::uint8_t compact, std::uint8_t world) noexcept {
    return static_cast<RadarVisibilityMaskWord>(
        (compact & kRadarVisibilityAllCategories)
        | static_cast<RadarVisibilityMaskWord>(
            world & kRadarVisibilityWorldCategories) << 8U);
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

inline constexpr RadarVisibilityMaskWord kDefaultRadarVisibilityMasks =
    pack_radar_visibility_masks(
        kRadarVisibilityAllCategories,
        kRadarVisibilityWorldCategories);

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

enum class AreaQuestDisplayMode : std::uint8_t {
    Available,
    AllUnfinished,
};

enum class AssaultDisplayMode : std::uint8_t {
    Current,
    All,
};

// One value carries both renderer masks so a UI change cannot expose a
// partially updated category set to the owner. The owner should publish
// packed_masks in one game-thread transaction when action is Applied.
struct RadarVisibilityHubResult {
    RadarVisibilityHubAction action{RadarVisibilityHubAction::None};
    RadarVisibilityMaskWord packed_masks{kDefaultRadarVisibilityMasks};
    bool changed{};
    std::uint32_t failure{};
    AreaQuestDisplayMode area_quest_mode{AreaQuestDisplayMode::Available};
    AssaultDisplayMode assault_mode{AssaultDisplayMode::Current};
};

class RadarVisibilityHub final {
public:
    // Resolves immutable reflected metadata only. No widget is created and no
    // controller or world object is retained.
    void initialize() noexcept;

    // Opens a transient, game-owned UMG panel, or treats a second toggle as
    // Close. This must run on the game thread with the freshly resolved
    // current-world player controller. The controller is never retained.
    [[nodiscard]] RadarVisibilityHubResult toggle(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode) noexcept;

    // Poll only while the panel is open. Closed calls return immediately and
    // perform no UObject work. Each changed selection returns both masks in
    // one packed value without closing the panel; X closes it.
    [[nodiscard]] RadarVisibilityHubResult service_open_panel(
        RC::Unreal::UObject* current_controller) noexcept;

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

private:
    static constexpr std::size_t kColumnCount = 2;
    static constexpr std::size_t kCategoryCount =
        static_cast<std::size_t>(RadarVisibilityCategory::Count);

    [[nodiscard]] RadarVisibilityHubResult open_guarded(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode) noexcept;
    [[nodiscard]] RadarVisibilityHubResult open_unsafe(
        RC::Unreal::UObject* current_controller,
        RadarVisibilityMaskWord current_masks,
        AreaQuestDisplayMode current_area_quest_mode,
        AssaultDisplayMode current_assault_mode);
    [[nodiscard]] RadarVisibilityHubResult service_guarded(
        RC::Unreal::UObject* current_controller) noexcept;
    [[nodiscard]] RadarVisibilityHubResult service_unsafe(
        RC::Unreal::UObject* current_controller);
    void detach_guarded(RC::Unreal::UObject* current_controller) noexcept;
    void detach_unsafe(RC::Unreal::UObject* current_controller);
    void reset_runtime_handles() noexcept;

    RC::Unreal::UClass* user_widget_class_{};
    RC::Unreal::UClass* widget_tree_class_{};
    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* border_class_{};
    RC::Unreal::UClass* text_block_class_{};
    RC::Unreal::UClass* check_box_class_{};
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
    RC::Unreal::UFunction* set_render_opacity_{};
    RC::Unreal::UFunction* set_render_scale_{};
    RC::Unreal::UFunction* set_render_pivot_{};
    RC::Unreal::UFunction* set_is_checked_{};
    RC::Unreal::UFunction* is_checked_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::UFunction* set_input_mode_game_and_ui_{};
    RC::Unreal::UFunction* set_input_mode_game_only_{};
    RC::Unreal::FWeakObjectPtr widget_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr widget_layout_library_{};
    RC::Unreal::FWeakObjectPtr host_{};
    RC::Unreal::FWeakObjectPtr widget_tree_{};
    RC::Unreal::FWeakObjectPtr root_panel_{};
    using ControlHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kCategoryCount>;
    std::array<ControlHandles, kColumnCount> controls_{};
    std::array<ControlHandles, kColumnCount> enabled_visuals_{};
    RC::Unreal::FWeakObjectPtr area_mode_available_control_{};
    RC::Unreal::FWeakObjectPtr area_mode_all_control_{};
    RC::Unreal::FWeakObjectPtr area_mode_available_visual_{};
    RC::Unreal::FWeakObjectPtr area_mode_all_visual_{};
    RC::Unreal::FWeakObjectPtr assault_mode_current_control_{};
    RC::Unreal::FWeakObjectPtr assault_mode_all_control_{};
    RC::Unreal::FWeakObjectPtr assault_mode_current_visual_{};
    RC::Unreal::FWeakObjectPtr assault_mode_all_visual_{};
    RC::Unreal::FWeakObjectPtr close_control_{};

    RadarVisibilityHubState state_{RadarVisibilityHubState::Uninitialized};
    RadarVisibilityMaskWord source_masks_{kDefaultRadarVisibilityMasks};
    RadarVisibilityMaskWord pending_masks_{kDefaultRadarVisibilityMasks};
    AreaQuestDisplayMode source_area_quest_mode_{
        AreaQuestDisplayMode::Available};
    AreaQuestDisplayMode pending_area_quest_mode_{
        AreaQuestDisplayMode::Available};
    AssaultDisplayMode source_assault_mode_{AssaultDisplayMode::Current};
    AssaultDisplayMode pending_assault_mode_{AssaultDisplayMode::Current};
    bool owns_input_mode_{};
    bool previous_cursor_visible_{};
    std::uint64_t open_count_{};
    std::uint64_t apply_count_{};
    std::uint64_t close_count_{};
    std::uint64_t detach_count_{};
    std::uint64_t fault_count_{};
    std::uint32_t last_failure_{};
    std::uint32_t abi_failure_mask_{};
};

} // namespace dsnwr
