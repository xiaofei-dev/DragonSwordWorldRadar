#pragma once

#include <Unreal/FWeakObjectPtr.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace RC::Unreal {
class FObjectPropertyBase;
class FNumericProperty;
class FProperty;
class UClass;
class UFunction;
class UObject;
}

namespace dsnwr {

inline constexpr std::size_t kCompactUmgMarkerCapacity = 80;
inline constexpr std::size_t kCompactUmgMarkerPieceCount = 4;
inline constexpr std::size_t kCompactUmgHeightPieceCount = 6;
inline constexpr std::size_t kCompactUmgHeightChannelCount = 2;
inline constexpr std::size_t kCompactClockDigitCount = 4;
inline constexpr std::size_t kCompactClockSegmentCount = 7;
inline constexpr std::size_t kCompactClockColonPieceCount = 2;
inline constexpr std::size_t kCompactClockPhasePieceCount = 10;

enum class CompactUmgMarkerKind : std::uint8_t {
    TreasureOther,
    TreasureMiniGame,
    TreasureMap,
    TreasurePuzzle,
    Boss,
    Assault,
    Fly,
    Mole,
    Wave,
    AreaQuest,
    BirdEgg,
};

enum class CompactUmgHeightChannel : std::uint8_t {
    Treasure,
    AreaQuest,
};

struct CompactUmgMarker {
    double normalized_x{};
    double normalized_y{};
    double reference_size{12.0};
    CompactUmgMarkerKind kind{CompactUmgMarkerKind::TreasureOther};
    bool show_height{};
    double height_angle_degrees{};
};

using CompactUmgMarkerArray =
    std::array<CompactUmgMarker, kCompactUmgMarkerCapacity>;

enum class CompactUmgRendererState : std::uint32_t {
    Uninitialized,
    Ready,
    Attached,
    Suppressed,
    Disabled,
    Faulted,
};

class CompactUmgRenderer final {
public:
    // Resolves immutable class/function metadata. This does not create a widget.
    void initialize() noexcept;

    // Starts one activation generation. Any prior host is detached first and
    // exactly one subsequent attach_once call is permitted for this generation.
    void begin_activation() noexcept;

    // Performs the only discovery and allocation transaction for an activation.
    // The expected owning player must be the freshly resolved current-world
    // controller; it is compared but never retained. The optional minimap layer
    // is a create-listener weak candidate. Discovery falls back to one bounded
    // FindFirstOf only when no valid event candidate is available. A failed call
    // cannot be retried until the next begin_activation call.
    [[nodiscard]] bool attach_once(
        RC::Unreal::UObject* expected_owning_player,
        RC::Unreal::FWeakObjectPtr minimap_layer_candidate = {}) noexcept;

    // Collapsed removes the host from layout and paint without destroying the
    // fixed pool. Updates are ignored while suppression is active.
    void set_menu_suppressed(bool suppressed) noexcept;

    // Reads the live game minimap scale through the cached DLayerMiniMap weak
    // root. The same low-frequency call also samples viewport geometry and
    // reflows the retained host only after a resolution, window-mode, or DPI
    // change. Nested objects are never retained across ticks.
    [[nodiscard]] bool read_minimap_scale(double& scale) noexcept;

    // Low-frequency binding step. Coordinates are relative to the player anchor
    // selected by the caller. Rebinding resets the panel translation to zero.
    // This uses only cached functions, scalar state, and the preallocated pool;
    // it performs no lookup, allocation, file work, or logging.
    void rebind(const CompactUmgMarkerArray& markers, std::size_t count) noexcept;

    // High-frequency motion step. Positive normalized deltas move the existing
    // root Canvas along the corresponding screen axis. This avoids both
    // viewport layout invalidation and Blueprint host transform contention. A
    // successful call performs exactly one root-Canvas ProcessEvent on the
    // proven single-host movement tree. No marker child is touched.
    void translate(double normalized_dx, double normalized_dy) noexcept;

    // Scalar-only height motion step. Each category owns six fixed pointer
    // pieces in one of two preallocated Canvases, allowing the nearest treasure
    // and nearest area quest to retain independent simultaneous pointers.
    void update_height_pointer(
        CompactUmgHeightChannel channel,
        double height_angle_degrees) noexcept;

    // Uses the already captured numeric world-time baseline. The hot call is
    // scalar-only unless the displayed game minute changes; no UObject is read.
    void update_world_clock(bool available, std::uint32_t seconds) noexcept;

    // Used by F8 and travel. The host is removed once and every weak handle is
    // reset; child widgets are released through the host's reflected ownership.
    void detach() noexcept;

    [[nodiscard]] CompactUmgRendererState state() const noexcept { return state_; }
    [[nodiscard]] std::uint64_t attach_attempt_count() const noexcept {
        return attach_attempt_count_;
    }
    [[nodiscard]] std::uint64_t attach_count() const noexcept { return attach_count_; }
    [[nodiscard]] std::uint64_t detach_count() const noexcept { return detach_count_; }
    [[nodiscard]] std::uint64_t rebind_count() const noexcept { return rebind_count_; }
    [[nodiscard]] std::uint64_t translation_count() const noexcept {
        return translation_count_;
    }
    [[nodiscard]] std::uint64_t height_transform_count() const noexcept {
        return height_transform_count_;
    }
    [[nodiscard]] std::uint64_t height_transform_skip_count() const noexcept {
        return height_transform_skip_count_;
    }
    [[nodiscard]] std::uint64_t fault_count() const noexcept { return fault_count_; }
    [[nodiscard]] std::uint64_t suppression_change_count() const noexcept {
        return suppression_change_count_;
    }
    [[nodiscard]] std::uint64_t input_overflow_count() const noexcept {
        return input_overflow_count_;
    }
    [[nodiscard]] std::size_t active_marker_count() const noexcept {
        return active_marker_count_;
    }
    [[nodiscard]] std::uint32_t last_attach_failure() const noexcept {
        return last_attach_failure_;
    }
    [[nodiscard]] std::uint32_t abi_failure_mask() const noexcept {
        return abi_failure_mask_;
    }
    [[nodiscard]] double viewport_width() const noexcept {
        return viewport_width_;
    }
    [[nodiscard]] double viewport_height() const noexcept {
        return viewport_height_;
    }
    [[nodiscard]] double viewport_dpi_scale() const noexcept {
        return viewport_dpi_scale_;
    }
    [[nodiscard]] double display_scale() const noexcept {
        return display_scale_;
    }
    [[nodiscard]] double umg_unit_scale() const noexcept {
        return umg_unit_scale_;
    }

private:
    [[nodiscard]] bool attach_guarded(
        RC::Unreal::UObject* expected_owning_player,
        RC::Unreal::FWeakObjectPtr minimap_layer_candidate) noexcept;
    [[nodiscard]] bool attach_unsafe(
        RC::Unreal::UObject* expected_owning_player,
        RC::Unreal::FWeakObjectPtr minimap_layer_candidate);
    [[nodiscard]] bool rebind_guarded(
        const CompactUmgMarkerArray& markers, std::size_t count) noexcept;
    [[nodiscard]] bool rebind_unsafe(
        const CompactUmgMarkerArray& markers, std::size_t count);
    [[nodiscard]] bool translate_guarded(
        double normalized_dx, double normalized_dy) noexcept;
    [[nodiscard]] bool translate_unsafe(
        double normalized_dx, double normalized_dy);
    [[nodiscard]] bool update_height_pointer_guarded(
        CompactUmgHeightChannel channel,
        double height_angle_degrees) noexcept;
    [[nodiscard]] bool update_height_pointer_unsafe(
        CompactUmgHeightChannel channel,
        double height_angle_degrees);
    [[nodiscard]] bool update_world_clock_guarded(
        bool available, std::uint32_t seconds) noexcept;
    [[nodiscard]] bool update_world_clock_unsafe(
        bool available, std::uint32_t seconds);
    [[nodiscard]] bool apply_height_pointer_transform_unsafe(
        std::size_t channel,
        double height_angle_degrees,
        bool force);
    [[nodiscard]] bool set_menu_suppressed_guarded(bool suppressed) noexcept;
    [[nodiscard]] bool set_menu_suppressed_unsafe(bool suppressed);
    [[nodiscard]] bool read_minimap_scale_guarded(double& scale) noexcept;
    [[nodiscard]] bool read_minimap_scale_unsafe(double& scale);
    [[nodiscard]] bool refresh_viewport_layout_unsafe(
        RC::Unreal::UObject* player_icon);
    [[nodiscard]] bool resolve_minimap_scale_schema(
        RC::Unreal::UObject* map_overlay);
    void detach_guarded() noexcept;
    void detach_unsafe();
    void reset_runtime_handles() noexcept;

    [[nodiscard]] static RC::Unreal::UObject* read_object_property(
        RC::Unreal::UObject* object, const wchar_t* property_name);

    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* canvas_panel_slot_class_{};
    RC::Unreal::UClass* border_class_{};
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
    RC::Unreal::UFunction* set_brush_{};
    RC::Unreal::UFunction* set_brush_color_{};
    RC::Unreal::UFunction* set_render_translation_{};
    RC::Unreal::UFunction* set_render_angle_{};
    RC::Unreal::UFunction* set_render_pivot_{};
    RC::Unreal::UFunction* set_render_scale_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::UFunction* clear_children_{};
    RC::Unreal::FWeakObjectPtr widget_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr widget_layout_library_{};
    RC::Unreal::FWeakObjectPtr minimap_layer_{};
    RC::Unreal::FWeakObjectPtr host_{};
    RC::Unreal::FWeakObjectPtr widget_tree_{};
    RC::Unreal::FWeakObjectPtr root_panel_{};
    RC::Unreal::FWeakObjectPtr host_root_panel_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactUmgHeightChannelCount>
        height_groups_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactUmgHeightChannelCount>
        height_group_slots_{};
    using ChestPieceHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kCompactUmgMarkerPieceCount>;
    std::array<ChestPieceHandles, kCompactUmgMarkerCapacity> marker_pieces_{};
    std::array<ChestPieceHandles, kCompactUmgMarkerCapacity> marker_piece_slots_{};
    using HeightPieceHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kCompactUmgHeightPieceCount>;
    std::array<HeightPieceHandles, kCompactUmgHeightChannelCount>
        height_pieces_{};
    std::array<HeightPieceHandles, kCompactUmgHeightChannelCount>
        height_piece_slots_{};
    using ClockDigitPieceHandles = std::array<
        RC::Unreal::FWeakObjectPtr, kCompactClockSegmentCount>;
    std::array<ClockDigitPieceHandles, kCompactClockDigitCount>
        clock_digit_pieces_{};
    std::array<ClockDigitPieceHandles, kCompactClockDigitCount>
        clock_digit_piece_slots_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactClockColonPieceCount>
        clock_colon_pieces_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactClockColonPieceCount>
        clock_colon_piece_slots_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactClockPhasePieceCount>
        clock_phase_pieces_{};
    std::array<RC::Unreal::FWeakObjectPtr, kCompactClockPhasePieceCount>
        clock_phase_piece_slots_{};
    RC::Unreal::FWeakObjectPtr clock_group_{};
    RC::Unreal::FWeakObjectPtr clock_group_slot_{};
    std::array<bool, kCompactUmgMarkerCapacity> marker_visible_{};
    std::array<double, kCompactUmgMarkerCapacity> marker_reference_sizes_{};
    std::array<std::uint8_t, kCompactUmgMarkerCapacity> marker_kind_codes_{};
    static constexpr std::size_t kSlateBrushBytes = 208;
    std::array<std::byte, kSlateBrushBytes> solid_brush_template_{};
    std::array<std::byte, kSlateBrushBytes> fly_outline_brush_template_{};
    std::array<std::byte, kSlateBrushBytes> area_quest_brush_template_{};
    std::array<std::byte, kSlateBrushBytes> bird_egg_brush_template_{};
    std::array<std::byte, kSlateBrushBytes> clock_phase_brush_template_{};
    bool brush_templates_ready_{};
    std::array<bool, kCompactUmgHeightChannelCount> height_visible_{};
    bool clock_visible_{};
    bool clock_minute_valid_{};
    bool clock_phase_valid_{};
    std::uint32_t clock_minute_{};
    std::uint8_t clock_phase_{0xFFU};
    std::array<std::array<bool, kCompactClockSegmentCount>,
               kCompactClockDigitCount> clock_segment_visible_{};
    std::array<bool, kCompactUmgHeightChannelCount>
        height_transform_valid_{};
    std::array<std::uint8_t, kCompactUmgHeightChannelCount>
        height_kind_codes_{{0xFFU, 0xFFU}};
    std::array<double, kCompactUmgHeightChannelCount>
        height_marker_half_widths_{};
    std::array<double, kCompactUmgHeightChannelCount>
        height_angle_degrees_{};
    std::array<double, kCompactUmgHeightChannelCount>
        height_translation_x_{};
    RC::Unreal::FProperty* render_transform_property_{};
    RC::Unreal::FProperty* scale_property_{};
    RC::Unreal::FNumericProperty* scale_x_property_{};

    double viewport_width_{};
    double viewport_height_{};
    double host_origin_x_{};
    double host_origin_y_{};
    double viewport_dpi_scale_{1.0};
    double display_scale_{1.0};
    double umg_unit_scale_{1.0};
    double host_render_scale_{1.0};
    CompactUmgRendererState state_{CompactUmgRendererState::Uninitialized};
    bool activation_active_{};
    bool attach_attempted_{};
    bool menu_suppressed_{};
    std::size_t active_marker_count_{};
    std::uint64_t attach_attempt_count_{};
    std::uint64_t attach_count_{};
    std::uint64_t detach_count_{};
    std::uint64_t rebind_count_{};
    std::uint64_t translation_count_{};
    std::uint64_t height_transform_count_{};
    std::uint64_t height_transform_skip_count_{};
    std::uint64_t fault_count_{};
    std::uint64_t suppression_change_count_{};
    std::uint64_t input_overflow_count_{};
    std::uint32_t last_attach_failure_{};
    std::uint32_t abi_failure_mask_{};
};

} // namespace dsnwr
