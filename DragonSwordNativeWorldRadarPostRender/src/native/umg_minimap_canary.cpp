#include "umg_minimap_canary.hpp"

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#pragma warning(pop)

#include <algorithm>
#include <cmath>

#include <windows.h>

namespace dsnwr {
namespace {

using namespace RC::Unreal;

struct Vector2D {
    double x{};
    double y{};
};

static_assert(sizeof(Vector2D) == 16);

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

} // namespace

void UmgMiniMapCanary::initialize() noexcept {
    widget_blueprint_library_ = find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    widget_layout_library_ = find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");
    create_widget_ = find<UFunction>(L"/Script/UMG.WidgetBlueprintLibrary:Create");
    get_owning_player_ = find<UFunction>(L"/Script/UMG.Widget:GetOwningPlayer");
    get_viewport_size_ = find<UFunction>(L"/Script/UMG.WidgetLayoutLibrary:GetViewportSize");
    add_to_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:AddToViewport");
    set_position_in_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:SetPositionInViewport");
    set_alignment_in_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:SetAlignmentInViewport");
    set_desired_size_in_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:SetDesiredSizeInViewport");
    set_desired_size_ = find<UFunction>(L"/Script/UMG.Image:SetDesiredSizeOverride");
    set_render_opacity_ = find<UFunction>(L"/Script/UMG.Widget:SetRenderOpacity");
    set_visibility_ = find<UFunction>(L"/Script/UMG.Widget:SetVisibility");
    invalidate_layout_ = find<UFunction>(L"/Script/UMG.Widget:InvalidateLayoutAndVolatility");
    force_layout_prepass_ = find<UFunction>(L"/Script/UMG.Widget:ForceLayoutPrepass");
    remove_from_parent_ = find<UFunction>(L"/Script/UMG.Widget:RemoveFromParent");
    abi_failure_mask_ = 0;
    const auto require_parameters = [this](std::uint32_t bit, UFunction* function, std::int32_t size) {
        if (!function || function->GetParmsSize() != size) abi_failure_mask_ |= bit;
    };
    require_parameters(1U << 0U, create_widget_, 32);
    require_parameters(1U << 1U, get_owning_player_, 8);
    require_parameters(1U << 2U, get_viewport_size_, 24);
    require_parameters(1U << 3U, add_to_viewport_, 4);
    // Unreal's reflected ParmsSize stops at the final bool byte. The local
    // aligned C++ buffer is intentionally larger and remains safe to pass.
    require_parameters(1U << 4U, set_position_in_viewport_, 17);
    require_parameters(1U << 5U, set_alignment_in_viewport_, 16);
    require_parameters(1U << 6U, set_desired_size_in_viewport_, 16);
    require_parameters(1U << 7U, set_desired_size_, 16);
    require_parameters(1U << 8U, set_render_opacity_, 4);
    require_parameters(1U << 9U, set_visibility_, 1);
    require_parameters(1U << 10U, invalidate_layout_, 0);
    require_parameters(1U << 11U, force_layout_prepass_, 0);
    require_parameters(1U << 12U, remove_from_parent_, 0);
    const bool functions_ready = widget_blueprint_library_ && widget_layout_library_ && create_widget_
        && get_owning_player_ && get_viewport_size_ && add_to_viewport_
        && set_position_in_viewport_ && set_alignment_in_viewport_
        && set_desired_size_in_viewport_ && set_desired_size_
        && set_render_opacity_ && set_visibility_
        && invalidate_layout_ && force_layout_prepass_ && remove_from_parent_;
    state_ = functions_ready && abi_failure_mask_ == 0
        ? UmgMiniMapState::Ready : UmgMiniMapState::Disabled;
}

void UmgMiniMapCanary::begin_activation() noexcept {
    detach_guarded();
    attach_attempted_ = false;
    menu_suppressed_ = false;
    last_attach_failure_ = 0;
    if (state_ != UmgMiniMapState::Disabled && state_ != UmgMiniMapState::Faulted) {
        state_ = UmgMiniMapState::Ready;
    }
}

void UmgMiniMapCanary::set_runtime_enabled(bool enabled) noexcept {
    if (runtime_enabled_ == enabled) return;
    runtime_enabled_ = enabled;
    if (!enabled) {
        detach_guarded();
        return;
    }
}

void UmgMiniMapCanary::set_menu_suppressed(bool suppressed) noexcept {
    if (menu_suppressed_ == suppressed) return;
    menu_suppressed_ = suppressed;
    UObject* marker = marker_.Get();
    if (!marker || !set_visibility_) return;
#if defined(_MSC_VER)
    __try {
        struct VisibilityParameters { std::uint8_t visibility{}; }
            visibility{suppressed ? static_cast<std::uint8_t>(1)
                                  : static_cast<std::uint8_t>(3)};
        marker->ProcessEvent(set_visibility_, &visibility);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        state_ = UmgMiniMapState::Faulted;
        runtime_enabled_ = false;
        detach_guarded();
    }
#else
    try {
        struct VisibilityParameters { std::uint8_t visibility{}; }
            visibility{suppressed ? static_cast<std::uint8_t>(1)
                                  : static_cast<std::uint8_t>(3)};
        marker->ProcessEvent(set_visibility_, &visibility);
    } catch (...) {
        ++fault_count_;
        state_ = UmgMiniMapState::Faulted;
        runtime_enabled_ = false;
        detach_guarded();
    }
#endif
}

void UmgMiniMapCanary::update(bool marker_valid, double normalized_x, double normalized_y) noexcept {
    if (!runtime_enabled_ || menu_suppressed_ || !marker_valid || state_ == UmgMiniMapState::Disabled
        || state_ == UmgMiniMapState::Faulted) return;
    if (!marker_.Get()) {
        if (attach_attempted_) return;
        attach_attempted_ = true;
        ++attach_attempt_count_;
        if (!attach_guarded()) return;
    }
    if (!update_guarded(normalized_x, normalized_y)) {
        ++fault_count_;
        state_ = UmgMiniMapState::Faulted;
        detach_guarded();
    }
}

bool UmgMiniMapCanary::attach_guarded() noexcept {
#if defined(_MSC_VER)
    __try {
        return attach_unsafe();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        state_ = UmgMiniMapState::Faulted;
        last_attach_failure_ = 100;
        return false;
    }
#else
    try { return attach_unsafe(); } catch (...) {
        ++fault_count_;
        state_ = UmgMiniMapState::Faulted;
        last_attach_failure_ = 100;
        return false;
    }
#endif
}

bool UmgMiniMapCanary::attach_unsafe() {
    UObject* layer = UObjectGlobals::FindFirstOf(L"DLayerMiniMap");
    if (!layer) { last_attach_failure_ = 1; return false; }
    UObject* layer_map = read_object_property(layer, L"LayerMap");
    if (!layer_map) { last_attach_failure_ = 2; return false; }
    UObject* player_icon = read_object_property(layer_map, L"PlayerIconWidget");
    if (!player_icon) { last_attach_failure_ = 3; return false; }
    UObject* source_image = read_object_property(player_icon, L"PlayerIcon_MiniMap");
    if (!source_image) { last_attach_failure_ = 4; return false; }

    struct OwningPlayerParameters { UObject* return_value{}; } owning_player{};
    player_icon->ProcessEvent(get_owning_player_, &owning_player);
    if (!owning_player.return_value) { last_attach_failure_ = 5; return false; }

    struct CreateParameters {
        UObject* world_context_object{};
        UClass* widget_type{};
        UObject* owning_player{};
        UObject* return_value{};
    } create{layer, player_icon->GetClassPrivate(), owning_player.return_value};
    widget_blueprint_library_->ProcessEvent(create_widget_, &create);
    UObject* marker = create.return_value;
    if (!marker) { last_attach_failure_ = 6; return false; }

    UObject* marker_image = read_object_property(marker, L"PlayerIcon_MiniMap");
    if (!marker_image) { last_attach_failure_ = 7; return false; }
    if (!copy_property(source_image, marker_image, L"Brush")
        || !copy_property(source_image, marker_image, L"ColorAndOpacity")) {
        last_attach_failure_ = 8;
        return false;
    }

    struct ViewportSizeParameters { UObject* world_context_object{}; Vector2D return_value{}; }
        viewport_size{layer};
    widget_layout_library_->ProcessEvent(get_viewport_size_, &viewport_size);
    if (!std::isfinite(viewport_size.return_value.x)
        || !std::isfinite(viewport_size.return_value.y)
        || viewport_size.return_value.x < 640.0 || viewport_size.return_value.y < 360.0) {
        last_attach_failure_ = 9;
        return false;
    }
    viewport_width_ = viewport_size.return_value.x;
    viewport_height_ = viewport_size.return_value.y;
    display_scale_ = std::max(0.25, std::min(viewport_width_ / 2560.0, viewport_height_ / 1440.0));

    struct AddToViewportParameters { std::int32_t z_order{}; } add_to_viewport{10000};
    marker->ProcessEvent(add_to_viewport_, &add_to_viewport);
    struct VectorParameters { Vector2D value{}; } alignment{{0.5, 0.5}};
    marker->ProcessEvent(set_alignment_in_viewport_, &alignment);
    struct DesiredViewportSizeParameters { Vector2D size{}; }
        desired_viewport_size{{44.0 * display_scale_, 44.0 * display_scale_}};
    marker->ProcessEvent(set_desired_size_in_viewport_, &desired_viewport_size);
    struct DesiredSizeParameters { Vector2D size{}; }
        desired_size{{44.0 * display_scale_, 44.0 * display_scale_}};
    marker_image->ProcessEvent(set_desired_size_, &desired_size);
    struct OpacityParameters { float opacity{}; } opacity{1.0F};
    marker->ProcessEvent(set_render_opacity_, &opacity);
    marker_image->ProcessEvent(set_render_opacity_, &opacity);
    struct VisibilityParameters { std::uint8_t visibility{}; } visibility{3};
    marker->ProcessEvent(set_visibility_, &visibility);
    marker_image->ProcessEvent(set_visibility_, &visibility);
    marker->ProcessEvent(invalidate_layout_, nullptr);
    marker->ProcessEvent(force_layout_prepass_, nullptr);

    layer_ = layer;
    marker_ = marker;
    last_attach_failure_ = 0;
    ++attach_count_;
    state_ = UmgMiniMapState::Attached;
    return true;
}

bool UmgMiniMapCanary::update_guarded(double normalized_x, double normalized_y) noexcept {
#if defined(_MSC_VER)
    __try {
        update_unsafe(normalized_x, normalized_y);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try { update_unsafe(normalized_x, normalized_y); return true; } catch (...) { return false; }
#endif
}

void UmgMiniMapCanary::update_unsafe(double normalized_x, double normalized_y) {
    UObject* marker = marker_.Get();
    if (!marker || !layer_.Get() || viewport_width_ <= 0.0 || viewport_height_ <= 0.0) {
        throw 0;
    }
    const double half_extent = 132.0 * display_scale_;
    const double center_x = viewport_width_ - (40.0 + 180.0) * display_scale_;
    const double center_y = (37.0 + 180.0) * display_scale_;
    const double x = center_x + std::clamp(normalized_x, -1.0, 1.0) * half_extent;
    const double y = center_y + std::clamp(normalized_y, -1.0, 1.0) * half_extent;
    struct PositionParameters { Vector2D position{}; bool remove_dpi_scale{}; }
        position{{x, y}, true};
    marker->ProcessEvent(set_position_in_viewport_, &position);
    ++update_count_;
}

void UmgMiniMapCanary::detach() noexcept {
    runtime_enabled_ = false;
    detach_guarded();
}

void UmgMiniMapCanary::detach_guarded() noexcept {
#if defined(_MSC_VER)
    __try { detach_unsafe(); } __except (EXCEPTION_EXECUTE_HANDLER) { ++fault_count_; }
#else
    try { detach_unsafe(); } catch (...) { ++fault_count_; }
#endif
}

void UmgMiniMapCanary::detach_unsafe() {
    if (UObject* marker = marker_.Get(); marker && remove_from_parent_) {
        marker->ProcessEvent(remove_from_parent_, nullptr);
    }
    layer_ = nullptr;
    marker_ = nullptr;
    viewport_width_ = 0.0;
    viewport_height_ = 0.0;
    display_scale_ = 1.0;
    menu_suppressed_ = false;
    if (state_ == UmgMiniMapState::Attached) state_ = UmgMiniMapState::Ready;
}

UObject* UmgMiniMapCanary::read_object_property(UObject* object, const wchar_t* property_name) {
    if (!object) return nullptr;
    auto* property = CastField<FObjectPropertyBase>(object->GetPropertyByNameInChain(property_name));
    void* value = object->GetValuePtrByPropertyNameInChain(property_name);
    return property && value ? property->GetObjectPropertyValue(value) : nullptr;
}

bool UmgMiniMapCanary::copy_property(
    UObject* source, UObject* destination, const wchar_t* property_name) {
    if (!source || !destination) return false;
    FProperty* source_property = source->GetPropertyByNameInChain(property_name);
    FProperty* destination_property = destination->GetPropertyByNameInChain(property_name);
    if (!source_property || !destination_property
        || source_property->GetSize() != destination_property->GetSize()) return false;
    void* source_value = source->GetValuePtrByPropertyNameInChain(property_name);
    void* destination_value = destination->GetValuePtrByPropertyNameInChain(property_name);
    if (!source_value || !destination_value) return false;
    destination_property->CopyCompleteValue(destination_value, source_value);
    return true;
}

} // namespace dsnwr
