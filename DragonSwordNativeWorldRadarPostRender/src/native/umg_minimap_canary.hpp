#pragma once

#include <Unreal/FWeakObjectPtr.hpp>

#include <cstdint>

namespace RC::Unreal {
class UObject;
class UClass;
class UFunction;
}

namespace dsnwr {

enum class UmgMiniMapState : std::uint32_t {
    Uninitialized,
    Ready,
    Attached,
    Disabled,
    Faulted,
};

class UmgMiniMapCanary final {
public:
    void initialize() noexcept;
    void begin_activation() noexcept;
    void set_runtime_enabled(bool enabled) noexcept;
    void set_menu_suppressed(bool suppressed) noexcept;
    void update(bool marker_valid, double normalized_x, double normalized_y) noexcept;
    void detach() noexcept;

    [[nodiscard]] UmgMiniMapState state() const noexcept { return state_; }
    [[nodiscard]] std::uint64_t attach_attempt_count() const noexcept { return attach_attempt_count_; }
    [[nodiscard]] std::uint64_t attach_count() const noexcept { return attach_count_; }
    [[nodiscard]] std::uint64_t update_count() const noexcept { return update_count_; }
    [[nodiscard]] std::uint64_t fault_count() const noexcept { return fault_count_; }
    [[nodiscard]] std::uint32_t last_attach_failure() const noexcept { return last_attach_failure_; }
    [[nodiscard]] std::uint32_t abi_failure_mask() const noexcept { return abi_failure_mask_; }

private:
    [[nodiscard]] bool attach_guarded() noexcept;
    [[nodiscard]] bool attach_unsafe();
    [[nodiscard]] bool update_guarded(double normalized_x, double normalized_y) noexcept;
    void update_unsafe(double normalized_x, double normalized_y);
    void detach_guarded() noexcept;
    void detach_unsafe();
    [[nodiscard]] static RC::Unreal::UObject* read_object_property(
        RC::Unreal::UObject* object, const wchar_t* property_name);
    [[nodiscard]] static bool copy_property(
        RC::Unreal::UObject* source, RC::Unreal::UObject* destination,
        const wchar_t* property_name);

    RC::Unreal::UObject* widget_blueprint_library_{};
    RC::Unreal::UObject* widget_layout_library_{};
    RC::Unreal::UFunction* create_widget_{};
    RC::Unreal::UFunction* get_owning_player_{};
    RC::Unreal::UFunction* get_viewport_size_{};
    RC::Unreal::UFunction* add_to_viewport_{};
    RC::Unreal::UFunction* set_position_in_viewport_{};
    RC::Unreal::UFunction* set_alignment_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_in_viewport_{};
    RC::Unreal::UFunction* set_desired_size_{};
    RC::Unreal::UFunction* set_render_opacity_{};
    RC::Unreal::UFunction* set_visibility_{};
    RC::Unreal::UFunction* invalidate_layout_{};
    RC::Unreal::UFunction* force_layout_prepass_{};
    RC::Unreal::UFunction* remove_from_parent_{};
    RC::Unreal::FWeakObjectPtr layer_{};
    RC::Unreal::FWeakObjectPtr marker_{};
    double viewport_width_{};
    double viewport_height_{};
    double display_scale_{1.0};
    UmgMiniMapState state_{UmgMiniMapState::Uninitialized};
    bool runtime_enabled_{};
    bool attach_attempted_{};
    bool menu_suppressed_{};
    std::uint64_t attach_attempt_count_{};
    std::uint64_t attach_count_{};
    std::uint64_t update_count_{};
    std::uint64_t fault_count_{};
    std::uint32_t last_attach_failure_{};
    std::uint32_t abi_failure_mask_{};
};

} // namespace dsnwr
