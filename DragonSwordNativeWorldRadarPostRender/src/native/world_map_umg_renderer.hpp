#pragma once

#include <Unreal/FWeakObjectPtr.hpp>

#include <dswros/render_projection.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace RC::Unreal {
class UClass;
class UFunction;
class UObject;
}

namespace dsnwr {

// The current catalog needs 1,785 slots, while the validated treasure loader
// deliberately permits up to 2,500 rows. Reserve enough fixed storage for
// that full loader boundary plus every current non-treasure catalog and future
// bounded growth without allocating during a map session.
inline constexpr std::size_t kWorldMapUmgMarkerCapacity = 4096;
// Give every glyph 50% more linear raster detail without changing the exact
// parent-local geometry, marker centers, projection, or two-layer ownership.
inline constexpr std::uint16_t kWorldMapAtlasTextureSize = 3072;
inline constexpr std::size_t kWorldMapAtlasLayerCount = 2;

enum class WorldMapUmgMarkerTone : std::uint8_t {
    White,
    Green,
    Orange,
    Blue,
};

enum class WorldMapUmgMarkerKind : std::uint8_t {
    Treasure,
    Boss,
    Assault,
    Fly,
    Mole,
    Wave,
    AreaQuest,
};

struct WorldMapUmgMarker {
    std::int64_t id{};
    double world_x{};
    double world_y{};
    WorldMapUmgMarkerTone tone{WorldMapUmgMarkerTone::White};
    WorldMapUmgMarkerKind kind{WorldMapUmgMarkerKind::Treasure};
    bool visible{true};
};

using WorldMapUmgMarkerArray =
    std::array<WorldMapUmgMarker, kWorldMapUmgMarkerCapacity>;

enum class WorldMapUmgRendererState : std::uint32_t {
    Uninitialized,
    Ready,
    Attached,
    Suspended,
    Disabled,
    Faulted,
};

enum class WorldMapUmgPaintOwnerStatus : std::uint32_t {
    Unresolved,
    Missing,
    HostClass,
    BaseClass,
    UnexpectedOwner,
};

enum class WorldMapLayeringRefreshResult : std::uint32_t {
    Updated,
    RetryLater,
    Retained,
    Faulted,
    Unchanged,
};

class WorldMapUmgRenderer final {
public:
    // Resolves immutable reflected metadata only. Runtime objects are created
    // only in an explicit map-widget session.
    void initialize(std::filesystem::path atlas_cache_path) noexcept;

    // Starts one bounded activation generation. A host suspended by F8 remains
    // parent-owned so the same open map can restore it without rebuilding.
    void begin_activation() noexcept;

    // Opens one new bounded map-widget session. Any prior attached or suspended
    // host is fully detached before the new session begins.
    void begin_map_session() noexcept;

    [[nodiscard]] bool detect_current_map_id(
        RC::Unreal::UObject* current_layer,
        std::int32_t& map_id) const noexcept;

    [[nodiscard]] bool attached_to(
        RC::Unreal::UObject* current_layer,
        std::int32_t map_id) const noexcept;

    // Returns true only when the attached renderer owns this exact weak layer
    // identity. Replacement candidates can exist briefly before their map data
    // is ready; they must never be mistaken for corruption of the old host.
    [[nodiscard]] bool attached_layer_matches(
        RC::Unreal::UObject* current_layer) const noexcept;

    // Reads the game-owned map Canvas geometry and projects the immutable
    // atlas bounds into the independent viewport hosts. The native map tree is
    // a read-only witness and is never made an owner of Mod widgets or slots.
    [[nodiscard]] WorldMapLayeringRefreshResult sync_viewport_transform(
        RC::Unreal::UObject* current_layer) noexcept;

    // Compatibility wrapper for callers that have not yet dropped the legacy
    // map-world arguments. They are intentionally ignored: transform refresh
    // never rebuilds the atlas or reprojects marker world coordinates.
    [[nodiscard]] WorldMapLayeringRefreshResult refresh_layering(
        RC::Unreal::UObject* current_layer,
        bool allow_tree_mutation,
        double player_world_x,
        double player_world_y) noexcept;

    // Durable content policy. This survives renderer host lifecycles; callers
    // update it only when expanded-map content is enabled or disabled.
    void set_content_visibility_intent(bool enabled) noexcept;

    // Transient live-layer gate. New attachments, suspension, detachment, and
    // faults reset it to false; callers publish true only after validating the
    // exact current-world layer and its native visibility.
    void publish_runtime_visibility(bool visible) noexcept;

    [[nodiscard]] bool attach_once(
        RC::Unreal::UObject* current_layer,
        RC::Unreal::UObject* expected_owning_player,
        const WorldMapUmgMarkerArray& markers,
        std::size_t marker_count,
        double player_world_x,
        double player_world_y,
        RC::Unreal::UObject* current_map_data = nullptr) noexcept;

    // F8 collapses both atlas hosts without removing them from the live map
    // tree. Collapsed UMG content performs no layout or paint work.
    void suspend() noexcept;

    // F7 restores a suspended host only after exact owned payload validation
    // and a fresh read-only viewport transform sync.
    [[nodiscard]] bool resume_suspended(
        RC::Unreal::UObject* current_layer) noexcept;

    // Travel, map replacement, and shutdown always use complete removal.
    void detach() noexcept;

    // UObject-array shutdown path. Clears retained weak identities without
    // dereferencing the destroyed registry or mutating the widget tree.
    void abandon_runtime_handles() noexcept;

    [[nodiscard]] WorldMapUmgRendererState state() const noexcept {
        return state_;
    }
    [[nodiscard]] bool transform_ready() const noexcept {
        return transform_ready_;
    }
    [[nodiscard]] std::uint64_t attach_attempt_count() const noexcept {
        return attach_attempt_count_;
    }
    [[nodiscard]] std::uint64_t attach_count() const noexcept {
        return attach_count_;
    }
    [[nodiscard]] std::uint64_t detach_count() const noexcept {
        return detach_count_;
    }
    [[nodiscard]] std::uint64_t suspend_count() const noexcept {
        return suspend_count_;
    }
    [[nodiscard]] std::uint64_t resume_count() const noexcept {
        return resume_count_;
    }
    [[nodiscard]] std::uint64_t fault_count() const noexcept {
        return fault_count_;
    }
    [[nodiscard]] std::uint64_t map_data_lookup_count() const noexcept {
        return map_data_lookup_count_;
    }
    [[nodiscard]] std::uint64_t map_data_cache_hit_count() const noexcept {
        return map_data_cache_hit_count_;
    }
    [[nodiscard]] std::uint64_t atlas_build_elapsed_us() const noexcept {
        return atlas_build_elapsed_us_;
    }
    [[nodiscard]] std::uint64_t atlas_file_bytes() const noexcept {
        return atlas_file_bytes_;
    }
    [[nodiscard]] std::uint64_t attach_elapsed_us() const noexcept {
        return attach_elapsed_us_;
    }
    [[nodiscard]] std::uint64_t reparent_count() const noexcept {
        return reparent_count_;
    }
    [[nodiscard]] std::uint64_t reproject_count() const noexcept {
        return reproject_count_;
    }
    [[nodiscard]] std::uint32_t last_map_data_source() const noexcept {
        return last_map_data_source_;
    }
    [[nodiscard]] std::uint32_t last_attach_failure() const noexcept {
        return last_attach_failure_;
    }
    [[nodiscard]] dswros::WorldMapTransformSyncStage
    last_transform_sync_stage() const noexcept {
        return last_transform_sync_stage_;
    }
    [[nodiscard]] std::uint32_t abi_failure_mask() const noexcept {
        return abi_failure_mask_;
    }
    [[nodiscard]] bool retryable_not_ready() const noexcept {
        return !attach_attempted_
            && dswros::world_map_attach_failure_retryable(
                last_attach_failure_);
    }
    [[nodiscard]] std::size_t active_marker_count() const noexcept {
        return active_marker_count_;
    }
    [[nodiscard]] std::int32_t map_id() const noexcept { return map_id_; }
    [[nodiscard]] double map_dimensions() const noexcept {
        return map_dimensions_;
    }
    [[nodiscard]] double map_ui_size() const noexcept { return map_ui_size_; }
    [[nodiscard]] double map_overlay_left() const noexcept {
        return map_overlay_left_;
    }
    [[nodiscard]] double map_overlay_top() const noexcept {
        return map_overlay_top_;
    }
    [[nodiscard]] double map_overlay_zoom() const noexcept {
        return map_overlay_zoom_;
    }
    [[nodiscard]] double player_canvas_anchor_x() const noexcept {
        return player_canvas_anchor_x_;
    }
    [[nodiscard]] double player_canvas_anchor_y() const noexcept {
        return player_canvas_anchor_y_;
    }
    [[nodiscard]] double native_parent_width() const noexcept {
        return native_parent_width_;
    }
    [[nodiscard]] double native_parent_height() const noexcept {
        return native_parent_height_;
    }
    [[nodiscard]] std::uint32_t player_anchor_source() const noexcept {
        return player_anchor_source_;
    }
    [[nodiscard]] dswros::WorldMapGeometryStabilityResult
    geometry_stability_result() const noexcept {
        return geometry_stability_result_;
    }
    [[nodiscard]] double geometry_sample_max_delta() const noexcept {
        return geometry_sample_max_delta_;
    }
    [[nodiscard]] dswros::WorldMapGeometryStabilityResult
    reparent_geometry_stability_result() const noexcept {
        return reparent_geometry_stability_result_;
    }
    [[nodiscard]] double reparent_geometry_sample_max_delta() const noexcept {
        return reparent_geometry_sample_max_delta_;
    }
    [[nodiscard]] double last_reparent_anchor_delta_x() const noexcept {
        return last_reparent_anchor_delta_x_;
    }
    [[nodiscard]] double last_reparent_anchor_delta_y() const noexcept {
        return last_reparent_anchor_delta_y_;
    }
    [[nodiscard]] WorldMapUmgPaintOwnerStatus paint_owner_status() const noexcept {
        return paint_owner_status_;
    }
    [[nodiscard]] bool last_layering_parent_changed() const noexcept {
        return last_layering_parent_changed_;
    }
    [[nodiscard]] bool last_layering_geometry_changed() const noexcept {
        return last_layering_geometry_changed_;
    }
    [[nodiscard]] std::int32_t last_layering_previous_parent_index() const noexcept {
        return last_layering_previous_parent_index_;
    }
    [[nodiscard]] std::int32_t last_layering_current_parent_index() const noexcept {
        return last_layering_current_parent_index_;
    }
    [[nodiscard]] std::int32_t last_layering_previous_parent_serial() const noexcept {
        return last_layering_previous_parent_serial_;
    }
    [[nodiscard]] std::int32_t last_layering_current_parent_serial() const noexcept {
        return last_layering_current_parent_serial_;
    }
    [[nodiscard]] double last_layering_previous_parent_width() const noexcept {
        return last_layering_previous_parent_width_;
    }
    [[nodiscard]] double last_layering_previous_parent_height() const noexcept {
        return last_layering_previous_parent_height_;
    }
    [[nodiscard]] double last_layering_current_parent_width() const noexcept {
        return last_layering_current_parent_width_;
    }
    [[nodiscard]] double last_layering_current_parent_height() const noexcept {
        return last_layering_current_parent_height_;
    }

private:
    [[nodiscard]] bool attach_guarded(
        RC::Unreal::UObject* current_layer,
        RC::Unreal::UObject* expected_owning_player,
        const WorldMapUmgMarkerArray& markers,
        std::size_t marker_count,
        double player_world_x,
        double player_world_y,
        RC::Unreal::UObject* current_map_data) noexcept;
    [[nodiscard]] bool attach_unsafe(
        RC::Unreal::UObject* current_layer,
        RC::Unreal::UObject* expected_owning_player,
        const WorldMapUmgMarkerArray& markers,
        std::size_t marker_count,
        double player_world_x,
        double player_world_y,
        RC::Unreal::UObject* current_map_data);
    [[nodiscard]] bool detect_current_map_id_guarded(
        RC::Unreal::UObject* current_layer,
        std::int32_t& map_id) const noexcept;
    [[nodiscard]] bool validate_host_unsafe(
        RC::Unreal::UObject* current_layer) const;
    [[nodiscard]] bool validate_host_payload_unsafe(
        RC::Unreal::UObject* current_layer,
        RC::Unreal::UObject*& owning_player) const;
    [[nodiscard]] bool validate_host_payload_guarded(
        RC::Unreal::UObject* current_layer) const noexcept;
    [[nodiscard]] bool sync_viewport_transform_unsafe(
        RC::Unreal::UObject* current_layer,
        WorldMapLayeringRefreshResult& result,
        volatile dswros::WorldMapTransformSyncStage& stage);
    [[nodiscard]] bool reconcile_host_visibility_guarded(
        bool force_collapsed,
        volatile dswros::WorldMapTransformSyncStage& stage) noexcept;
    [[nodiscard]] bool reconcile_host_visibility_unsafe(
        bool force_collapsed,
        volatile dswros::WorldMapTransformSyncStage& stage);
    [[nodiscard]] bool apply_host_visibility_unsafe(
        bool visible,
        volatile dswros::WorldMapTransformSyncStage& stage);
    void fault_and_detach(std::uint32_t failure) noexcept;
    [[nodiscard]] bool suspend_guarded() noexcept;
    [[nodiscard]] bool resume_suspended_guarded(
        RC::Unreal::UObject* current_layer) noexcept;
    void detach_guarded() noexcept;
    void detach_unsafe();
    void reset_geometry_stability_sample() noexcept;
    void reset_reparent_geometry_stability_sample() noexcept;
    void reset_runtime_handles() noexcept;

    RC::Unreal::UClass* world_map_layer_class_{};
    RC::Unreal::UClass* world_map_data_class_{};
    RC::Unreal::UClass* overlay_class_{};
    RC::Unreal::UClass* retainer_box_class_{};
    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* canvas_panel_slot_class_{};
    RC::Unreal::UClass* map_point_icon_class_{};
    RC::Unreal::UClass* image_class_{};
    RC::Unreal::UFunction* create_widget_{};
    RC::Unreal::UFunction* get_owning_player_{};
    RC::Unreal::UFunction* get_slot_position_{};
    RC::Unreal::UFunction* get_slot_alignment_{};
    RC::Unreal::UFunction* get_cached_geometry_{};
    RC::Unreal::UFunction* get_geometry_local_size_{};
    RC::Unreal::UFunction* local_to_absolute_{};
    RC::Unreal::UFunction* absolute_to_local_{};
    RC::Unreal::UFunction* add_child_to_canvas_{};
    RC::Unreal::UFunction* set_slot_position_{};
    RC::Unreal::UFunction* set_slot_size_{};
    RC::Unreal::UFunction* set_slot_alignment_{};
    RC::Unreal::UFunction* set_slot_z_order_{};
    RC::Unreal::UFunction* set_visibility_{};
    RC::Unreal::UFunction* set_brush_from_texture_{};
    RC::Unreal::UFunction* import_file_as_texture_{};
    RC::Unreal::UFunction* clear_children_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::UFunction* add_to_viewport_{};
    RC::Unreal::UFunction* get_viewport_widget_geometry_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::FWeakObjectPtr widget_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr kismet_rendering_library_{};
    RC::Unreal::FWeakObjectPtr slate_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr widget_layout_library_{};
    RC::Unreal::FWeakObjectPtr layer_{};
    RC::Unreal::FWeakObjectPtr retainer_box_{};
    RC::Unreal::FWeakObjectPtr native_parent_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount> hosts_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        widget_trees_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        root_panels_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_images_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_image_slots_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_textures_{};
    std::array<std::filesystem::path, kWorldMapAtlasLayerCount>
        atlas_cache_paths_{};
    WorldMapUmgMarkerArray active_markers_{};

    WorldMapUmgRendererState state_{WorldMapUmgRendererState::Uninitialized};
    WorldMapUmgPaintOwnerStatus paint_owner_status_{
        WorldMapUmgPaintOwnerStatus::Unresolved};
    bool activation_active_{};
    bool attach_attempted_{};
    bool map_data_lookup_attempted_{};
    bool content_visibility_intent_{};
    bool runtime_visibility_allowed_{};
    bool transform_ready_{};
    std::optional<bool> applied_host_visibility_{};
    bool viewport_transform_valid_{};
    dswros::WorldMapAtlasPlacement viewport_placement_{};
    bool viewport_geometry_sample_valid_{};
    dswros::WorldMapGeometrySample viewport_geometry_sample_{};
    std::size_t active_marker_input_count_{};
    std::size_t active_marker_count_{};
    std::int32_t map_id_{};
    double map_dimensions_{};
    double map_ui_size_{};
    double map_overlay_left_{};
    double map_overlay_top_{};
    double map_overlay_zoom_{1.0};
    double player_canvas_anchor_x_{};
    double player_canvas_anchor_y_{};
    double native_parent_width_{};
    double native_parent_height_{};
    std::uint32_t player_anchor_source_{};
    bool geometry_sample_valid_{};
    dswros::WorldMapGeometrySample geometry_sample_{};
    dswros::WorldMapGeometryStabilityResult geometry_stability_result_{
        dswros::WorldMapGeometryStabilityResult::None};
    double geometry_sample_max_delta_{};
    bool reparent_geometry_sample_valid_{};
    dswros::WorldMapGeometrySample reparent_geometry_sample_{};
    dswros::WorldMapGeometryStabilityResult
        reparent_geometry_stability_result_{
            dswros::WorldMapGeometryStabilityResult::None};
    double reparent_geometry_sample_max_delta_{};
    std::int32_t reparent_geometry_parent_index_{-1};
    std::int32_t reparent_geometry_parent_serial_{};
    double last_reparent_anchor_delta_x_{};
    double last_reparent_anchor_delta_y_{};
    double atlas_left_{};
    double atlas_top_{};
    double atlas_width_{};
    double atlas_height_{};
    std::int32_t cached_map_id_{};
    double cached_map_dimensions_{};
    double cached_map_ui_size_{};
    std::uint64_t attach_attempt_count_{};
    std::uint64_t attach_count_{};
    std::uint64_t detach_count_{};
    std::uint64_t suspend_count_{};
    std::uint64_t resume_count_{};
    std::uint64_t fault_count_{};
    std::uint64_t map_data_lookup_count_{};
    std::uint64_t map_data_cache_hit_count_{};
    std::uint64_t atlas_build_elapsed_us_{};
    std::uint64_t atlas_file_bytes_{};
    std::uint64_t attach_elapsed_us_{};
    std::uint64_t reparent_count_{};
    std::uint64_t reproject_count_{};
    std::uint32_t last_map_data_source_{};
    std::uint32_t last_attach_failure_{};
    dswros::WorldMapTransformSyncStage last_transform_sync_stage_{
        dswros::WorldMapTransformSyncStage::None};
    std::uint32_t abi_failure_mask_{};
    bool last_layering_parent_changed_{};
    bool last_layering_geometry_changed_{};
    std::int32_t last_layering_previous_parent_index_{-1};
    std::int32_t last_layering_current_parent_index_{-1};
    std::int32_t last_layering_previous_parent_serial_{};
    std::int32_t last_layering_current_parent_serial_{};
    double last_layering_previous_parent_width_{};
    double last_layering_previous_parent_height_{};
    double last_layering_current_parent_width_{};
    double last_layering_current_parent_height_{};
};

} // namespace dsnwr
