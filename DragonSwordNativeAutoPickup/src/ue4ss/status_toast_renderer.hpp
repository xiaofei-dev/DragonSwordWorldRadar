#pragma once

#include <dsnap/status_toast.hpp>

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#include <Unreal/FWeakObjectPtr.hpp>
#pragma warning(pop)

#include <chrono>
#include <cstdint>

namespace RC::Unreal {
class UClass;
class UEngine;
class UFunction;
class UObject;
class FTextProperty;
}

namespace dsnap::ue4ss {

enum class StatusToastRendererState : std::uint8_t {
    Uninitialized,
    Ready,
    Attached,
    Disabled,
    Faulted,
};

class StatusToastRenderer final {
public:
    using Clock = std::chrono::steady_clock;

    void initialize() noexcept;
    void notify(StatusToastKind kind, Clock::time_point now) noexcept;
    void tick(RC::Unreal::UEngine* engine, Clock::time_point now) noexcept;
    void release_for_travel() noexcept;
    void shutdown_guarded() noexcept;

    [[nodiscard]] StatusToastRendererState state() const noexcept { return state_; }
    [[nodiscard]] std::uint32_t abi_failure_mask() const noexcept { return abi_failure_mask_; }
    [[nodiscard]] std::uint32_t last_failure() const noexcept { return last_failure_; }
    [[nodiscard]] std::uint64_t fault_count() const noexcept { return fault_count_; }
    [[nodiscard]] std::uint64_t attach_count() const noexcept { return attach_count_; }

private:
    void tick_guarded(RC::Unreal::UEngine* engine,
                      const StatusToastFrame& frame) noexcept;
    void tick_unsafe(RC::Unreal::UEngine* engine,
                     const StatusToastFrame& frame);
    [[nodiscard]] bool attach(RC::Unreal::UEngine* engine,
                              RC::Unreal::UObject* controller);
    [[nodiscard]] bool set_message(const StatusToastFrame& frame);
    void reset_runtime_handles() noexcept;
    void shutdown_unsafe();

    StatusToastTimeline timeline_{};

    RC::Unreal::UClass* user_widget_class_{};
    RC::Unreal::UClass* widget_tree_class_{};
    RC::Unreal::UClass* canvas_panel_class_{};
    RC::Unreal::UClass* border_class_{};
    RC::Unreal::UClass* text_block_class_{};
    RC::Unreal::UObject* widget_blueprint_library_{};
    RC::Unreal::UObject* widget_layout_library_{};
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
    RC::Unreal::FTextProperty* set_text_value_property_{};
    RC::Unreal::UFunction* set_render_opacity_{};
    RC::Unreal::UFunction* set_render_translation_{};
    RC::Unreal::UFunction* set_render_scale_{};
    RC::Unreal::UFunction* set_render_pivot_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* remove_from_parent_{};

    RC::Unreal::FWeakObjectPtr host_{};
    RC::Unreal::FWeakObjectPtr glow_{};
    RC::Unreal::FWeakObjectPtr accent_{};
    RC::Unreal::FWeakObjectPtr indicator_{};
    RC::Unreal::FWeakObjectPtr status_rule_{};
    RC::Unreal::FWeakObjectPtr status_text_{};
    std::uint64_t rendered_revision_{};
    std::uint64_t attach_attempted_revision_{};
    bool host_visible_{};

    StatusToastRendererState state_{StatusToastRendererState::Uninitialized};
    std::uint32_t abi_failure_mask_{};
    std::uint32_t last_failure_{};
    std::uint64_t fault_count_{};
    std::uint64_t attach_count_{};
};

} // namespace dsnap::ue4ss
