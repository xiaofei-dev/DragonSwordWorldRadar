#pragma once

#include <Unreal/FWeakObjectPtr.hpp>

#include <dswros/render_projection.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace RC::Unreal {
class UClass;
class UFunction;
class UObject;
}

namespace dsnwr {

inline constexpr std::size_t kWorldMapUmgMarkerCapacity = 1785;
inline constexpr std::uint16_t kWorldMapAtlasTextureSize = 2048;
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
    Restacked,
    Reparented,
    RetryLater,
    ParentChanged,
    Faulted,
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

    // Reinsert the two retained host slots after every game-native child on a
    // real map-image update. The game can rebuild dungeon icons at the same
    // maximum Canvas Z while changing zoom tiers, so Z alone is insufficient.
    // A missing native witness is retryable. A changed Canvas reuses the
    // retained atlas only after fresh parent-local geometry is available.
    [[nodiscard]] WorldMapLayeringRefreshResult refresh_layering(
        RC::Unreal::UObject* current_layer,
        bool allow_tree_mutation) noexcept;

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

    // F7 restores a suspended host only after exact layer, slot, parent,
    // content, image, and texture identity checks. Failure detaches fail-closed.
    [[nodiscard]] bool resume_suspended(
        RC::Unreal::UObject* current_layer) noexcept;

    // Travel, map replacement, and shutdown always use complete removal.
    void detach() noexcept;

    [[nodiscard]] WorldMapUmgRendererState state() const noexcept {
        return state_;
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
    [[nodiscard]] std::uint32_t last_map_data_source() const noexcept {
        return last_map_data_source_;
    }
    [[nodiscard]] std::uint32_t last_attach_failure() const noexcept {
        return last_attach_failure_;
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
    [[nodiscard]] WorldMapLayeringRefreshResult restack_hosts_unsafe(
        RC::Unreal::UObject* current_layer,
        bool allow_tree_mutation);
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
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* clear_children_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::UFunction* request_retainer_render_{};
    RC::Unreal::FWeakObjectPtr widget_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr kismet_rendering_library_{};
    RC::Unreal::FWeakObjectPtr slate_blueprint_library_{};
    RC::Unreal::FWeakObjectPtr layer_{};
    RC::Unreal::FWeakObjectPtr retainer_box_{};
    RC::Unreal::FWeakObjectPtr native_parent_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount> hosts_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        widget_trees_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        root_panels_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        native_parent_slots_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_images_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_image_slots_{};
    std::array<RC::Unreal::FWeakObjectPtr, kWorldMapAtlasLayerCount>
        atlas_textures_{};
    std::array<std::filesystem::path, kWorldMapAtlasLayerCount>
        atlas_cache_paths_{};

    WorldMapUmgRendererState state_{WorldMapUmgRendererState::Uninitialized};
    WorldMapUmgPaintOwnerStatus paint_owner_status_{
        WorldMapUmgPaintOwnerStatus::Unresolved};
    bool activation_active_{};
    bool attach_attempted_{};
    bool map_data_lookup_attempted_{};
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
    std::uint32_t last_map_data_source_{};
    std::uint32_t last_attach_failure_{};
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
