#pragma once

#include <Unreal/FWeakObjectPtr.hpp>
#include <dswros/scene_marker_model.hpp>
#include <dswros/scene_projection.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace RC::Unreal {
class UClass;
class UFunction;
class UObject;
class FBoolProperty;
class FTextProperty;
class FStructProperty;
class FObjectPropertyBase;
}

namespace dsnwr {

using SceneUmgMarkerKind = dswros::SceneMarkerKind;
using SceneUmgMiniGameKind = dswros::SceneMiniGameKind;
using SceneUmgMarker = dswros::SceneMarker;
inline constexpr std::size_t kSceneUmgMarkerCapacity = dswros::kSceneMarkerCapacity;
using SceneUmgMarkerArray = std::array<SceneUmgMarker, kSceneUmgMarkerCapacity>;

enum class SceneUmgRendererState : std::uint8_t {
    Uninitialized, Ready, Attached, Suppressed, Disabled, Faulted,
};

class SceneUmgRenderer final {
public:
    SceneUmgRenderer() = default;
    SceneUmgRenderer(const SceneUmgRenderer&) = delete;
    SceneUmgRenderer& operator=(const SceneUmgRenderer&) = delete;
    SceneUmgRenderer(SceneUmgRenderer&&) = delete;
    SceneUmgRenderer& operator=(SceneUmgRenderer&&) = delete;
    // Metadata only. Off never allocates a widget or projects a position.
    void initialize(std::filesystem::path marker_asset_root) noexcept;
    void begin_activation() noexcept;
    [[nodiscard]] bool attach_once(RC::Unreal::UObject* current_controller) noexcept;
    void set_menu_suppressed(bool suppressed) noexcept;
    // Pure values only; the next update applies visibility/resource changes.
    void set_display_settings(dswros::SceneDisplaySettings settings) noexcept;
    void update(RC::Unreal::UObject* current_controller, dswros::Position player,
                std::span<const SceneUmgMarker> markers) noexcept;
    void detach() noexcept;
    void release_for_travel() noexcept { detach(); }
    // Object-array shutdown: no weak resolution and no Unreal calls.
    void abandon_runtime_handles() noexcept;

    [[nodiscard]] SceneUmgRendererState state() const noexcept { return state_; }
    [[nodiscard]] std::uint32_t last_failure() const noexcept { return last_failure_; }
    [[nodiscard]] std::uint64_t fault_count() const noexcept { return fault_count_; }
    [[nodiscard]] std::uint64_t projection_count() const noexcept { return projection_count_; }
    [[nodiscard]] std::uint64_t batch_frame_count() const noexcept { return batch_frame_count_; }
    [[nodiscard]] std::uint64_t batch_fallback_count() const noexcept { return batch_fallback_count_; }
    [[nodiscard]] std::uint64_t position_update_count() const noexcept { return position_update_count_; }
    [[nodiscard]] std::uint64_t position_reuse_count() const noexcept { return position_reuse_count_; }
    [[nodiscard]] std::uint64_t glyph_bind_count() const noexcept { return glyph_bind_count_; }
    [[nodiscard]] std::uint64_t texture_import_count() const noexcept { return texture_import_count_; }
    [[nodiscard]] std::size_t active_marker_count() const noexcept { return active_count_; }
    [[nodiscard]] std::uint32_t last_text_failure() const noexcept { return last_text_failure_; }

private:
    enum class Operation : std::uint8_t { Initialize, Attach, Update, Hide, Detach };
    [[nodiscard]] bool run_guarded(Operation operation,
        RC::Unreal::UObject* controller = nullptr, dswros::Position player = {},
        std::span<const SceneUmgMarker> markers = {}) noexcept;
    [[nodiscard]] bool run_unsafe(Operation operation, RC::Unreal::UObject* controller,
        dswros::Position player, std::span<const SceneUmgMarker> markers);
    [[nodiscard]] bool initialize_unsafe();
    [[nodiscard]] bool attach_unsafe(RC::Unreal::UObject* controller);
    [[nodiscard]] bool bind_marker_texture_unsafe(
        RC::Unreal::UObject* image, RC::Unreal::UObject* texture);
    [[nodiscard]] bool update_unsafe(RC::Unreal::UObject* controller,
        dswros::Position player, std::span<const SceneUmgMarker> markers);
    [[nodiscard]] dswros::SceneProjectedPoint project_engine_unsafe(
        RC::Unreal::UObject* controller, dswros::Position position);
    [[nodiscard]] dswros::SceneFrameProjection capture_projection_unsafe(
        RC::Unreal::UObject* controller, double scale);
    [[nodiscard]] bool ensure_distance_label_unsafe(std::size_t slot);
    [[nodiscard]] bool set_distance_text_unsafe(std::size_t slot, std::uint16_t meters);
    [[nodiscard]] bool update_distance_label_guarded(std::size_t slot, bool show,
        const dswros::SceneVisibleMarker& marker, const dswros::SceneCandidate& candidate,
        double width, bool refresh) noexcept;
    [[nodiscard]] bool update_distance_label_unsafe(std::size_t slot, bool show,
        const dswros::SceneVisibleMarker& marker, const dswros::SceneCandidate& candidate,
        double width, bool refresh);
    void disable_distance_labels() noexcept;
    void release_distance_cache_unsafe();
    void reset_handles() noexcept;
    void fail(std::uint32_t code) noexcept;

    RC::Unreal::UClass* user_widget_class_{};
    RC::Unreal::UClass* widget_tree_class_{};
    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* canvas_slot_class_{};
    RC::Unreal::UClass* image_class_{};
    RC::Unreal::UClass* text_block_class_{};
    RC::Unreal::UFunction* create_{};
    RC::Unreal::UFunction* owning_player_{};
    RC::Unreal::UFunction* viewport_size_{};
    RC::Unreal::UFunction* viewport_scale_{};
    RC::Unreal::UFunction* project_{};
    RC::Unreal::UFunction* add_viewport_{};
    RC::Unreal::UFunction* add_canvas_{};
    RC::Unreal::UFunction* position_{};
    RC::Unreal::UFunction* render_translation_{};
    RC::Unreal::UFunction* force_volatile_{};
    RC::Unreal::UFunction* camera_location_{};
    RC::Unreal::UFunction* camera_rotation_{};
    RC::Unreal::FObjectPropertyBase* camera_manager_property_{};
    RC::Unreal::UFunction* size_{};
    RC::Unreal::UFunction* alignment_{};
    RC::Unreal::UFunction* visible_{};
    RC::Unreal::UFunction* set_brush_from_texture_{};
    RC::Unreal::UFunction* import_file_as_texture_{};
    RC::Unreal::FStructProperty* image_brush_property_{};
    RC::Unreal::FObjectPropertyBase* brush_resource_property_{};
    RC::Unreal::UFunction* viewport_position_{};
    RC::Unreal::UFunction* viewport_alignment_{};
    RC::Unreal::UFunction* viewport_desired_{};
    RC::Unreal::UFunction* remove_{};
    RC::Unreal::UFunction* set_text_{};
    RC::Unreal::UFunction* text_render_scale_{};
    RC::Unreal::UFunction* text_render_pivot_{};
    RC::Unreal::UFunction* text_shadow_color_{};
    RC::Unreal::UFunction* text_shadow_offset_{};
    RC::Unreal::FTextProperty* text_value_property_{};
    bool text_metadata_ready_{};
    RC::Unreal::FWeakObjectPtr blueprint_library_{};
    RC::Unreal::FWeakObjectPtr layout_library_{};
    RC::Unreal::FWeakObjectPtr rendering_library_{};
    std::filesystem::path marker_asset_root_{};
    RC::Unreal::FBoolProperty* projection_relative_{};
    RC::Unreal::FBoolProperty* projection_return_{};
    std::size_t project_controller_offset_{};
    std::size_t project_world_offset_{};
    std::size_t project_screen_offset_{};
    std::size_t project_relative_offset_{};
    std::size_t project_return_offset_{};

    RC::Unreal::FWeakObjectPtr host_{};
    RC::Unreal::FWeakObjectPtr owner_{};
    RC::Unreal::FWeakObjectPtr owner_world_{};
    RC::Unreal::FWeakObjectPtr tree_{};
    std::array<RC::Unreal::FWeakObjectPtr, kSceneUmgMarkerCapacity> groups_{};
    std::array<RC::Unreal::FWeakObjectPtr, kSceneUmgMarkerCapacity> group_slots_{};
    // One fixed Image per marker replaces the twelve Border glyph pieces.
    // Six collapsed keeper Images own the textures through reflected Brushes;
    // inactive kinds remain GC-safe without raw UObject or AddToRoot ownership.
    static constexpr std::size_t kMarkerTextureCount = 6;
    std::array<RC::Unreal::FWeakObjectPtr, kSceneUmgMarkerCapacity> marker_images_{};
    std::array<RC::Unreal::FWeakObjectPtr, kMarkerTextureCount> marker_textures_{};
    std::array<RC::Unreal::FWeakObjectPtr, kMarkerTextureCount> texture_keepers_{};
    std::array<SceneUmgMarkerKind, kSceneUmgMarkerCapacity> displayed_kinds_{};
    std::array<bool, kSceneUmgMarkerCapacity> style_valid_{};
    std::array<bool, kSceneUmgMarkerCapacity> shown_{};
    struct SubmittedPosition { double x{}; double y{}; };
    std::array<SubmittedPosition, kSceneUmgMarkerCapacity> submitted_positions_{};
    std::array<bool, kSceneUmgMarkerCapacity> position_valid_{};
    std::array<RC::Unreal::FWeakObjectPtr, kSceneUmgMarkerCapacity> distance_labels_{};
    std::array<RC::Unreal::FWeakObjectPtr, kSceneUmgMarkerCapacity> distance_slots_{};
    std::array<std::uint16_t, kSceneUmgMarkerCapacity> displayed_distances_{};
    std::array<dswros::SceneFocusIdentity, kSceneUmgMarkerCapacity> distance_identities_{};
    std::array<bool, kSceneUmgMarkerCapacity> distance_shown_{};
    std::array<bool, kSceneUmgMarkerCapacity> distance_pending_{};
    std::array<bool, kSceneUmgMarkerCapacity> distance_on_left_{};
    struct DistanceText {
        alignas(8) std::array<std::byte, 24> parameters{};
    };
    // Lazily intern only displayed integer distances, never a 1001-value
    // startup warm-up. The reflected owning FText is destroyed on detach.
    std::array<DistanceText, 1001> distance_cache_{};
    std::array<bool, 1001> distance_cache_initialized_{};
    std::uint64_t distance_refresh_at_{};
    std::uint8_t distance_widget_budget_{};
    std::uint8_t distance_value_budget_{};
    bool distance_failed_{};
    std::uint32_t last_text_failure_{};
    bool host_shown_{};
    bool activation_{};
    bool attach_attempted_{};
    bool suppressed_{};
    double width_{};
    double height_{};
    dswros::SceneSelection selection_{};
    dswros::SceneFocusState focus_state_{};
    std::array<dswros::SceneFocusIdentity, kSceneUmgMarkerCapacity> previous_visible_{};
    std::size_t previous_visible_count_{};
    dswros::SceneDisplaySettings settings_{};
    SceneUmgRendererState state_{SceneUmgRendererState::Uninitialized};
    std::uint32_t last_failure_{};
    std::uint64_t fault_count_{};
    std::uint64_t projection_count_{};
    std::uint64_t batch_frame_count_{};
    std::uint64_t batch_fallback_count_{};
    std::uint64_t position_update_count_{};
    std::uint64_t position_reuse_count_{};
    std::uint64_t glyph_bind_count_{};
    std::uint64_t texture_import_count_{};
    std::size_t active_count_{};
};

} // namespace dsnwr
