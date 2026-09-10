#include "scene_umg_renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/UClass.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/FString.hpp>
#else
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Core/Containers/FString.hpp>
#endif
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#if !defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/Property/FEnumProperty.hpp>
#endif
#include "ue4ss_compat.hpp"
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include <windows.h>

namespace dsnwr {
namespace {
using namespace RC::Unreal;

struct Vec2 { double x{}; double y{}; };
struct CreateParams {
    UObject* context{}; UClass* type{}; UObject* owner{}; UObject* result{};
};
struct ObjectResult { UObject* result{}; };
struct AddCanvas { UObject* child{}; UObject* result{}; };
struct ViewportSize { UObject* context{}; Vec2 result{}; };
struct ViewportScale { UObject* context{}; float result{}; };
struct ViewportPosition { Vec2 position{}; bool remove_dpi{}; };
struct Color { float r{}; float g{}; float b{}; float a{1.0F}; };
struct SetBrushFromTexture { UObject* texture{}; bool match_size{}; };
struct ImportFile {
    UObject* context{}; FString filename{}; UObject* result{};
    ImportFile(UObject* owner, const wchar_t* path) : context(owner), filename(path) {}
};

static_assert(sizeof(Vec2) == 16);
static_assert(sizeof(dswros::Position) == 24);
static_assert(sizeof(CreateParams) == 32);
static_assert(sizeof(AddCanvas) == 16);
static_assert(sizeof(ViewportSize) == 24);
static_assert(sizeof(Color) == 16);
static_assert(sizeof(SetBrushFromTexture) == 16);
static_assert(sizeof(ImportFile) == 32);
static_assert(sizeof(FText) == 24);
static_assert(alignof(FText) <= 8);
// RE-UE4SS's wrapper owns engine-managed storage through reflected teardown,
// as in the F6 hub. A changed SDK ownership contract must fail compilation.
static_assert(std::is_trivially_destructible_v<FText>);

constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
// Below compact Radar and the interactive F6 hub, above normal world HUD.
constexpr std::int32_t kSceneZOrder = 1'999'999'900;
// Generated pixels retain the SG-04 32 x 36 group and center (16,16).
constexpr std::array<const wchar_t*, 6> kMarkerTextureFiles{{
    L"treasure-other.tga", L"treasure-mini-game.tga", L"treasure-map.tga",
    L"treasure-puzzle.tga", L"area-quest.tga", L"mini-game.tga",
}};
static_assert(static_cast<std::size_t>(SceneUmgMarkerKind::MiniGame) + 1
              == kMarkerTextureFiles.size());

template <typename T> T* find(const wchar_t* name) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, name);
}

FProperty* field(UFunction* function, const wchar_t* name) {
    if (!function) return nullptr;
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function)) {
        if (property->GetName() == name) return property;
    }
    return nullptr;
}

bool field_span(UFunction* function, FProperty* property, std::int32_t size) {
    return function && property && property->GetOffset_Internal() >= 0
        && property->GetSize() == size
        && property->GetOffset_Internal() + size <= function->GetParmsSize();
}

bool write_object(UObject* object, const wchar_t* name, UObject* value) {
    if (!object) return false;
    auto* property = CastField<FObjectPropertyBase>(
        object->GetPropertyByNameInChain(name));
    void* address = object->GetValuePtrByPropertyNameInChain(name);
    if (!property || !address) return false;
    property->SetObjectPropertyValue(address, value);
    return property->GetObjectPropertyValue(address) == value;
}

void vector_call(UObject* object, UFunction* function, double x, double y) {
    Vec2 value{x, y};
    object->ProcessEvent(function, &value);
}

void visibility(UObject* object, UFunction* function, bool show) {
    std::uint8_t value = show ? kHitTestInvisible : kCollapsed;
    object->ProcessEvent(function, &value);
}


} // namespace

void SceneUmgRenderer::set_display_settings(dswros::SceneDisplaySettings settings) noexcept {
    settings = dswros::normalize_scene_display_settings(settings);
    if (settings_.distance_mode != settings.distance_mode) focus_state_ = {};
    if (settings_.range_meters != settings.range_meters
        || settings_.marker_limit != settings.marker_limit
        || settings_.distance_mode != settings.distance_mode)
        distance_refresh_at_ = 0;
    settings_ = settings;
}

void SceneUmgRenderer::initialize(std::filesystem::path marker_asset_root) noexcept {
    if (state_ != SceneUmgRendererState::Uninitialized) return;
    marker_asset_root_ = std::move(marker_asset_root);
    state_ = run_guarded(Operation::Initialize)
        ? SceneUmgRendererState::Ready : SceneUmgRendererState::Disabled;
}

bool SceneUmgRenderer::initialize_unsafe() {
    last_failure_ = 1;
    user_widget_class_ = find<UClass>(L"/Script/UMG.UserWidget");
    widget_tree_class_ = find<UClass>(L"/Script/UMG.WidgetTree");
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    canvas_slot_class_ = find<UClass>(L"/Script/UMG.CanvasPanelSlot");
    image_class_ = find<UClass>(L"/Script/UMG.Image");
    blueprint_library_ = find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    layout_library_ = find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");
    rendering_library_ = find<UObject>(L"/Script/Engine.Default__KismetRenderingLibrary");
    if (!user_widget_class_ || !widget_tree_class_ || !canvas_panel_class_
        || !canvas_slot_class_ || !image_class_ || !blueprint_library_.Get()
        || !layout_library_.Get() || !rendering_library_.Get()
        || marker_asset_root_.empty()) return false;
    const auto resolve = [](UFunction*& target, const wchar_t* name,
                            std::int32_t parameter_size) {
        target = find<UFunction>(name);
        return target && target->GetParmsSize() == parameter_size;
    };
    bool valid = true;
    valid &= resolve(create_, L"/Script/UMG.WidgetBlueprintLibrary:Create", 32);
    valid &= resolve(owning_player_, L"/Script/UMG.Widget:GetOwningPlayer", 8);
    valid &= resolve(viewport_size_, L"/Script/UMG.WidgetLayoutLibrary:GetViewportSize", 24);
    valid &= resolve(viewport_scale_, L"/Script/UMG.WidgetLayoutLibrary:GetViewportScale", 12);
    valid &= resolve(add_viewport_, L"/Script/UMG.UserWidget:AddToViewport", 4);
    valid &= resolve(add_canvas_, L"/Script/UMG.CanvasPanel:AddChildToCanvas", 16);
    valid &= resolve(position_, L"/Script/UMG.CanvasPanelSlot:SetPosition", 16);
    valid &= resolve(render_translation_, L"/Script/UMG.Widget:SetRenderTranslation", 16);
    valid &= resolve(force_volatile_, L"/Script/UMG.Widget:ForceVolatile", 1);
    valid &= resolve(size_, L"/Script/UMG.CanvasPanelSlot:SetSize", 16);
    valid &= resolve(alignment_, L"/Script/UMG.CanvasPanelSlot:SetAlignment", 16);
    valid &= resolve(visible_, L"/Script/UMG.Widget:SetVisibility", 1);
    valid &= resolve(set_brush_from_texture_, L"/Script/UMG.Image:SetBrushFromTexture", 9);
    valid &= resolve(import_file_as_texture_, L"/Script/Engine.KismetRenderingLibrary:ImportFileAsTexture2D", 32);
    valid &= resolve(viewport_position_, L"/Script/UMG.UserWidget:SetPositionInViewport", 17);
    valid &= resolve(viewport_alignment_, L"/Script/UMG.UserWidget:SetAlignmentInViewport", 16);
    valid &= resolve(viewport_desired_, L"/Script/UMG.UserWidget:SetDesiredSizeInViewport", 16);
    valid &= resolve(remove_, L"/Script/UMG.Widget:RemoveFromParent", 0);
    last_failure_ = 2;
    if (!valid) return false;
    auto* volatile_flag = CastField<FBoolProperty>(field(force_volatile_, L"bForce"));
    if (!volatile_flag || volatile_flag->GetOffset_Internal() != 0) return false;
    // Optional batch path: preserve the existing engine projection if the
    // camera metadata is unavailable on another game/runtime build.
    camera_location_ = find<UFunction>(L"/Script/Engine.PlayerCameraManager:GetCameraLocation");
    camera_rotation_ = find<UFunction>(L"/Script/Engine.PlayerCameraManager:GetCameraRotation");
    for (auto** function : {&camera_location_, &camera_rotation_}) {
        auto* result = field(*function, L"ReturnValue");
        if (!*function || (*function)->GetParmsSize() != 24
            || !CastField<FStructProperty>(result)
            || !field_span(*function, result, 24) || result->GetOffset_Internal() != 0)
            *function = nullptr;
    }
    // Import and brush packets are checked once before any texture operation.
    FProperty* brush_texture = field(set_brush_from_texture_, L"Texture");
    auto* match_size = CastField<FBoolProperty>(
        field(set_brush_from_texture_, L"bMatchSize"));
    FProperty* import_context = field(import_file_as_texture_, L"WorldContextObject");
    FProperty* import_filename{};
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(import_file_as_texture_)) {
        if (CastField<FStrProperty>(property) && property->GetOffset_Internal() == 8) {
            import_filename = property;
            break;
        }
    }
    FProperty* import_result = field(import_file_as_texture_, L"ReturnValue");
    if (!CastField<FObjectPropertyBase>(brush_texture)
        || !field_span(set_brush_from_texture_, brush_texture, 8)
        || brush_texture->GetOffset_Internal() != 0
        || !field_span(set_brush_from_texture_, match_size, 1)
        || match_size->GetOffset_Internal() != 8
        || !CastField<FObjectPropertyBase>(import_context)
        || !field_span(import_file_as_texture_, import_context, 8)
        || import_context->GetOffset_Internal() != 0
        || !CastField<FStrProperty>(import_filename)
        || !field_span(import_file_as_texture_, import_filename, 16)
        || import_filename->GetOffset_Internal() != 8
        || !CastField<FObjectPropertyBase>(import_result)
        || !field_span(import_file_as_texture_, import_result, 8)
        || import_result->GetOffset_Internal() != 24) return false;

    project_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:ProjectWorldLocationToWidgetPosition");
    last_failure_ = 3;
    if (!project_ || project_->GetParmsSize() <= 0
        || project_->GetParmsSize() > 128) return false;
    FProperty* controller = field(project_, L"PlayerController");
    FProperty* world = field(project_, L"WorldLocation");
    FProperty* screen = field(project_, L"ScreenPosition");
    projection_relative_ = CastField<FBoolProperty>(
        field(project_, L"bPlayerViewportRelative"));
    projection_return_ = CastField<FBoolProperty>(field(project_, L"ReturnValue"));
    if (!CastField<FObjectPropertyBase>(controller)
        || !CastField<FStructProperty>(world) || !CastField<FStructProperty>(screen)
        || !field_span(project_, controller, 8) || !field_span(project_, world, 24)
        || !field_span(project_, screen, 16)
        || !field_span(project_, projection_relative_, 1)
        || !field_span(project_, projection_return_, 1)) return false;
    project_controller_offset_ = static_cast<std::size_t>(controller->GetOffset_Internal());
    project_world_offset_ = static_cast<std::size_t>(world->GetOffset_Internal());
    project_screen_offset_ = static_cast<std::size_t>(screen->GetOffset_Internal());
    project_relative_offset_ = static_cast<std::size_t>(projection_relative_->GetOffset_Internal());
    project_return_offset_ = static_cast<std::size_t>(projection_return_->GetOffset_Internal());
    // Reflected signatures are validated once. Runtime packets are fixed,
    // zeroed stack storage; no property lookup occurs during scene updates.
    last_failure_ = 0;
    return true;
}

void SceneUmgRenderer::begin_activation() noexcept {
    if (state_ == SceneUmgRendererState::Disabled
        || state_ == SceneUmgRendererState::Uninitialized) return;
    const bool detached = run_guarded(Operation::Detach);
    reset_handles();
    if (!detached) {
        state_ = SceneUmgRendererState::Faulted;
        return;
    }
    activation_ = true;
    attach_attempted_ = false;
    state_ = SceneUmgRendererState::Ready;
    last_failure_ = 0;
}

bool SceneUmgRenderer::attach_once(UObject* controller) noexcept {
    if (settings_.range_meters == 0 || settings_.marker_limit == 0) return false;
    if (!activation_ || attach_attempted_
        || state_ != SceneUmgRendererState::Ready) {
        return state_ == SceneUmgRendererState::Attached
            || state_ == SceneUmgRendererState::Suppressed;
    }
    attach_attempted_ = true;
    if (!run_guarded(Operation::Attach, controller)) {
        fail(last_failure_ ? last_failure_ : 10);
        return false;
    }
    state_ = suppressed_ ? SceneUmgRendererState::Suppressed
                         : SceneUmgRendererState::Attached;
    last_failure_ = 0;
    return true;
}

bool SceneUmgRenderer::attach_unsafe(UObject* controller) {
    last_failure_ = 10;
    UObject* library = blueprint_library_.Get();
    if (!controller || !library) return false;
    CreateParams create{controller, user_widget_class_, controller};
    library->ProcessEvent(create_, &create);
    UObject* host = create.result;
    if (!host) return false;
    // Record ownership immediately so every later failure detaches this host.
    host_ = host;
    ObjectResult owner{};
    host->ProcessEvent(owning_player_, &owner);
    if (owner.result != controller) return false;
    owner_ = controller;
    camera_manager_property_ = CastField<FObjectPropertyBase>(
        controller->GetPropertyByNameInChain(L"PlayerCameraManager"));
    if (camera_manager_property_ && camera_manager_property_->GetSize() != 8)
        camera_manager_property_ = nullptr;
    UObject* world = reinterpret_cast<UObject*>(controller->GetWorld());
    if (!world) return false;
    owner_world_ = world;
    UObject* tree = UObjectGlobals::NewObject<UObject>(host, widget_tree_class_);
    UObject* root = tree ? UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_)
                         : nullptr;
    last_failure_ = 11;
    if (!tree || !root || !write_object(host, L"WidgetTree", tree)
        || !write_object(tree, L"RootWidget", root)) return false;
    tree_ = tree;
    visibility(host, visible_, false);
    visibility(root, visible_, true);

    const auto add = [this](UObject* parent, UObject* child, double x, double y,
                            double w, double h, bool centered) -> UObject* {
        if (!parent || !child) return nullptr;
        AddCanvas parameters{child};
        parent->ProcessEvent(add_canvas_, &parameters);
        UObject* slot = parameters.result;
        if (!slot || !slot->IsA(canvas_slot_class_)) return nullptr;
        vector_call(slot, position_, x, y);
        vector_call(slot, size_, w, h);
        vector_call(slot, alignment_, centered ? 0.5 : 0.0, centered ? 0.5 : 0.0);
        return slot;
    };
    // Retain all six textures through reflected Image Brushes. Inactive kinds
    // have a collapsed keeper in this same tree, so no texture import or GC
    // ownership repair is needed on the render path or a category transition.
    static_assert(kMarkerTextureFiles.size() == kMarkerTextureCount);
    UObject* rendering_library = rendering_library_.Get();
    if (!rendering_library) return false;
    for (std::size_t i = 0; i < kMarkerTextureCount; ++i) {
        last_failure_ = 13;
        UObject* keeper = UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (!keeper || !add(root, keeper, 0, 0, 0, 0, false)) return false;
        texture_keepers_[i] = keeper;
        visibility(keeper, visible_, false);
        if (i == 0) {
            image_brush_property_ = CastField<FStructProperty>(
                keeper->GetPropertyByNameInChain(L"Brush"));
            if (!image_brush_property_ || !image_brush_property_->GetStruct())
                return false;
            brush_resource_property_ = nullptr;
            for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(
                    image_brush_property_->GetStruct())) {
                if (property->GetName() == L"ResourceObject") {
                    brush_resource_property_ = CastField<FObjectPropertyBase>(property);
                    break;
                }
            }
            if (!brush_resource_property_ || brush_resource_property_->GetSize() != 8
                || brush_resource_property_->GetOffset_Internal() < 0
                || brush_resource_property_->GetOffset_Internal() + 8
                    > image_brush_property_->GetSize()) return false;
        }
        const auto path = (marker_asset_root_ / kMarkerTextureFiles[i]).wstring();
        ImportFile imported{controller, path.c_str()};
        rendering_library->ProcessEvent(import_file_as_texture_, &imported);
        ++texture_import_count_;
        if (!imported.result || !bind_marker_texture_unsafe(keeper, imported.result))
            return false;
        marker_textures_[i] = imported.result;
    }
    last_failure_ = 12;
    for (std::size_t i = 0; i < kSceneUmgMarkerCapacity; ++i) {
        UObject* group = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
        UObject* slot = add(root, group, 0, 0, 32, 36, false);
        if (!group || !slot) return false;
        groups_[i] = group;
        group_slots_[i] = slot;
#if !defined(DSNWRPR_UE4SS_STABLE_ROOT)
        // Apply before Slate construction; moving screen markers should not
        // hop between integer pixels. Older builds without this enum keep
        // their normal snapping behavior instead of writing an unknown field.
        auto* snapping = CastField<FEnumProperty>(group->GetPropertyByNameInChain(L"PixelSnapping"));
        if (snapping && snapping->GetSize() == 1 && snapping->GetEnum()
            && snapping->GetEnum()->GetNameByValue(1).ToString() == L"EWidgetPixelSnapping::Disabled") {
            auto* value = snapping->ContainerPtrToValuePtr<std::uint8_t>(group);
            if (value) *value = 1;
        }
#endif
        // Only these tiny moving subtrees are volatile. The menu, minimap and
        // other HUD widgets retain their normal layout/invalidation caches.
        bool moving = true;
        group->ProcessEvent(force_volatile_, &moving);
        visibility(group, visible_, false);
        UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (!image || !add(group, image, 0, 0, 32, 36, false)
            || !bind_marker_texture_unsafe(image, marker_textures_[0].Get())) return false;
        marker_images_[i] = image;
        visibility(image, visible_, true);
        displayed_kinds_[i] = SceneUmgMarkerKind::TreasureOther;
        style_valid_[i] = true;
    }

    std::int32_t z_order = kSceneZOrder;
    host->ProcessEvent(add_viewport_, &z_order);
    vector_call(host, viewport_alignment_, 0, 0);
    ViewportPosition viewport_position{{0, 0}, false};
    host->ProcessEvent(viewport_position_, &viewport_position);
    // First successful update establishes the current viewport size and only
    // then reveals the host. An attached-but-empty scene never flashes at 0,0.
    return true;
}

bool SceneUmgRenderer::bind_marker_texture_unsafe(UObject* image, UObject* texture) {
    if (!image || !texture || !image_brush_property_ || !brush_resource_property_)
        return false;
    SetBrushFromTexture brush{texture, false};
    image->ProcessEvent(set_brush_from_texture_, &brush);
    void* value = image_brush_property_->ContainerPtrToValuePtr<void>(image);
    void* resource = value
        ? brush_resource_property_->ContainerPtrToValuePtr<void>(value) : nullptr;
    if (!resource || brush_resource_property_->GetObjectPropertyValue(resource) != texture)
        return false;
    ++glyph_bind_count_;
    return true;
}

void SceneUmgRenderer::set_menu_suppressed(bool suppressed) noexcept {
    if (suppressed) focus_state_ = {};
    if (suppressed_ == suppressed) return;
    suppressed_ = suppressed;
    previous_visible_count_ = 0;
    if (state_ != SceneUmgRendererState::Attached
        && state_ != SceneUmgRendererState::Suppressed) return;
    if (suppressed && !run_guarded(Operation::Hide)) {
        fail(20);
        return;
    }
    // Unsuppression waits for a fresh projected frame instead of briefly
    // exposing positions left over from before the menu/camera transition.
    state_ = suppressed ? SceneUmgRendererState::Suppressed
                        : SceneUmgRendererState::Attached;
}

void SceneUmgRenderer::update(UObject* controller, dswros::Position player,
                             std::span<const SceneUmgMarker> markers) noexcept {
    if (!activation_ || suppressed_ || state_ != SceneUmgRendererState::Attached)
        return;
    if (settings_.range_meters == 0 || settings_.marker_limit == 0) {
        focus_state_ = {};
        selection_ = {};
        if (host_shown_ && !run_guarded(Operation::Hide)) fail(20);
        return;
    }
    if (!run_guarded(Operation::Update, controller, player, markers))
        fail(last_failure_ ? last_failure_ : 21);
}

bool SceneUmgRenderer::ensure_distance_label_unsafe(std::size_t slot) {
    if (slot >= distance_labels_.size()) return false;
    if (distance_labels_[slot].Get()) return true;
    if (!text_metadata_ready_) {
        // Distance Off never enters this path, resolves font/text metadata,
        // or creates a TextBlock/FText. Cache this class/function ABI once.
        text_block_class_ = find<UClass>(L"/Script/UMG.TextBlock");
        set_text_ = find<UFunction>(L"/Script/UMG.TextBlock:SetText");
        text_value_property_ = CastField<FTextProperty>(field(set_text_, L"InText"));
        text_render_scale_ = find<UFunction>(L"/Script/UMG.Widget:SetRenderScale");
        text_render_pivot_ = find<UFunction>(L"/Script/UMG.Widget:SetRenderTransformPivot");
        if (!text_block_class_ || !set_text_ || set_text_->GetParmsSize() != 24
            || !field_span(set_text_, text_value_property_, 24)
            || text_value_property_->GetOffset_Internal() != 0
            || !text_render_scale_ || text_render_scale_->GetParmsSize() != 16
            || !text_render_pivot_ || text_render_pivot_->GetParmsSize() != 16)
            return false;
        text_shadow_color_ = find<UFunction>(L"/Script/UMG.TextBlock:SetShadowColorAndOpacity");
        text_shadow_offset_ = find<UFunction>(L"/Script/UMG.TextBlock:SetShadowOffset");
        if (text_shadow_color_ && text_shadow_color_->GetParmsSize() != 16)
            text_shadow_color_ = nullptr;
        if (text_shadow_offset_ && text_shadow_offset_->GetParmsSize() != 16)
            text_shadow_offset_ = nullptr;
        text_metadata_ready_ = true;
    }
    UObject* tree = tree_.Get();
    UObject* parent = groups_[slot].Get();
    if (!tree || !parent) return false;
    UObject* label = UObjectGlobals::NewObject<UObject>(tree, text_block_class_);
    if (!label) return false;
    distance_labels_[slot] = label;
    AddCanvas add{label};
    parent->ProcessEvent(add_canvas_, &add);
    if (!add.result || !add.result->IsA(canvas_slot_class_)) return false;
    distance_slots_[slot] = add.result;
    vector_call(add.result, alignment_, 0, 0);
    vector_call(add.result, size_, 130, 32);
    vector_call(add.result, position_, 27, 8);
    // This is the F6 hub's already-used base UMG font/render-scale fallback:
    // inherit the complete engine Font, use only Latin digits and "m", and
    // resize visually without assuming any FSlateFontInfo memory layout.
    vector_call(label, text_render_pivot_, 0, 0);
    vector_call(label, text_render_scale_, 0.50, 0.50);
    if (text_shadow_color_) {
        Color shadow{0, 0, 0, 1};
        label->ProcessEvent(text_shadow_color_, &shadow);
    }
    if (text_shadow_offset_) vector_call(label, text_shadow_offset_, 2, 2);
    visibility(label, visible_, false);
    displayed_distances_[slot] = 1001;
    distance_shown_[slot] = false;
    distance_on_left_[slot] = false;
    return true;
}

bool SceneUmgRenderer::set_distance_text_unsafe(std::size_t slot, std::uint16_t meters) {
    if (slot >= distance_labels_.size() || meters >= distance_cache_.size()
        || !text_value_property_) return false;
    UObject* label = distance_labels_[slot].Get();
    if (!label) return false;
    auto& entry = distance_cache_[meters];
    if (!distance_cache_initialized_[meters]) {
        const auto text = dswros::scene_distance_text(meters);
        // Construct an owning engine FText in its final stable storage. The
        // reflected SetText parameter borrows it; SetText copies into Slate.
        ::new (entry.parameters.data()) FText(text.data());
        distance_cache_initialized_[meters] = true;
    }
    label->ProcessEvent(set_text_, entry.parameters.data());
    displayed_distances_[slot] = meters;
    return true;
}

bool SceneUmgRenderer::update_distance_label_guarded(std::size_t slot, bool show,
    const dswros::SceneVisibleMarker& marker, const dswros::SceneCandidate& candidate,
    double width, bool refresh) noexcept {
#if defined(_MSC_VER)
    __try {
        return update_distance_label_unsafe(slot, show, marker, candidate, width, refresh);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_text_failure_ = 100;
        return false;
    }
#else
    try {
        return update_distance_label_unsafe(slot, show, marker, candidate, width, refresh);
    } catch (...) {
        ++fault_count_;
        last_text_failure_ = 100;
        return false;
    }
#endif
}

bool SceneUmgRenderer::update_distance_label_unsafe(std::size_t slot, bool show,
    const dswros::SceneVisibleMarker& marker, const dswros::SceneCandidate& candidate,
    double width, bool refresh) {
    if (!show) {
        if (distance_shown_[slot]) {
            UObject* label = distance_labels_[slot].Get();
            if (!label) return false;
            visibility(label, visible_, false);
            distance_shown_[slot] = false;
        }
        distance_pending_[slot] = false;
        return true;
    }
    if (!distance_labels_[slot].Get()) {
        if (distance_widget_budget_ == 0) return true;
        --distance_widget_budget_;
        last_text_failure_ = 31;
        if (!ensure_distance_label_unsafe(slot)) return false;
    }
    const auto meters = dswros::scene_display_distance(candidate);
    if (meters >= distance_cache_.size()) return false;
    const auto& identity = distance_identities_[slot];
    const bool identity_changed = candidate.marker.id != identity.id
        || dswros::scene_identity_namespace(candidate.marker.kind)
            != dswros::scene_identity_namespace(identity.kind);
    if ((refresh || !distance_shown_[slot] || identity_changed || distance_pending_[slot])
        && displayed_distances_[slot] != meters) {
        if (!distance_cache_initialized_[meters]) {
            if (distance_value_budget_ == 0) {
                distance_pending_[slot] = true;
                // Never leave the previous target's distance on a new glyph
                // while its value waits for the next bounded creation budget.
                if (identity_changed && distance_shown_[slot]) {
                    UObject* label = distance_labels_[slot].Get();
                    if (!label) return false;
                    visibility(label, visible_, false);
                    distance_shown_[slot] = false;
                }
                return true;
            }
            --distance_value_budget_;
        }
        last_text_failure_ = 32;
        if (!set_distance_text_unsafe(slot, meters)) return false;
    }
    distance_pending_[slot] = false;
    distance_identities_[slot] = {candidate.marker.id, candidate.marker.kind};
    const bool on_left = marker.x + 78.0 > width;
    if (on_left != distance_on_left_[slot]) {
        UObject* label_slot = distance_slots_[slot].Get();
        if (!label_slot) return false;
        vector_call(label_slot, position_, on_left ? -60.0 : 27.0, 8.0);
        distance_on_left_[slot] = on_left;
    }
    if (!distance_shown_[slot]) {
        UObject* label = distance_labels_[slot].Get();
        if (!label) return false;
        visibility(label, visible_, true);
        distance_shown_[slot] = true;
    }
    last_text_failure_ = 0;
    return true;
}

void SceneUmgRenderer::disable_distance_labels() noexcept {
    const std::uint32_t failure = last_text_failure_ ? last_text_failure_ : 30;
    distance_failed_ = true;
    for (std::size_t i = 0; i < distance_shown_.size(); ++i) {
        if (!distance_shown_[i]) continue;
        (void)update_distance_label_guarded(i, false, {}, {}, 0, false);
        distance_shown_[i] = false;
    }
    last_text_failure_ = failure;
}

void SceneUmgRenderer::release_distance_cache_unsafe() {
    if (!text_value_property_) return;
    for (std::size_t i = 0; i < distance_cache_.size(); ++i) {
        if (!distance_cache_initialized_[i]) continue;
        // Clear ownership first so an exception cannot cause double teardown
        // if failure cleanup re-enters the guarded detach path.
        distance_cache_initialized_[i] = false;
        text_value_property_->DestroyValue_InContainer(distance_cache_[i].parameters.data());
    }
}

dswros::SceneProjectedPoint SceneUmgRenderer::project_engine_unsafe(
    UObject* controller, dswros::Position position) {
    alignas(16) std::array<std::byte, 128> parameters{};
    std::memcpy(parameters.data() + project_controller_offset_, &controller, 8);
    std::memcpy(parameters.data() + project_world_offset_, &position, 24);
    projection_relative_->SetPropertyValue(parameters.data() + project_relative_offset_, false);
    layout_library_.Get()->ProcessEvent(project_, parameters.data());
    ++projection_count_;
    Vec2 point{};
    std::memcpy(&point, parameters.data() + project_screen_offset_, 16);
    return {point.x, point.y,
        projection_return_->GetPropertyValue(parameters.data() + project_return_offset_)};
}

dswros::SceneFrameProjection SceneUmgRenderer::capture_projection_unsafe(
    UObject* controller, double scale) {
    if (!camera_manager_property_ || !camera_location_ || !camera_rotation_) return {};
    auto* camera_value = camera_manager_property_->ContainerPtrToValuePtr<void>(controller);
    UObject* camera = camera_value
        ? camera_manager_property_->GetObjectPropertyValue(camera_value) : nullptr;
    if (!camera || reinterpret_cast<UObject*>(camera->GetWorld()) != owner_world_.Get()) return {};
    dswros::Position origin{}, rotation{};
    camera->ProcessEvent(camera_location_, &origin);
    camera->ProcessEvent(camera_rotation_, &rotation);
    if (!dswros::valid_scene_position(origin) || !dswros::valid_scene_position(rotation)) return {};
    const auto basis = dswros::scene_camera_basis(origin, rotation);
    const auto points = dswros::scene_calibration_points(basis);
    std::array<dswros::SceneProjectedPoint, 5> samples{};
    for (std::size_t i = 0; i < points.size(); ++i)
        samples[i] = project_engine_unsafe(controller, points[i]);
    return dswros::calibrate_scene_projection(basis, samples, 0.25 / scale);
}

bool SceneUmgRenderer::update_unsafe(UObject* controller, dswros::Position player,
                                    std::span<const SceneUmgMarker> markers) {
    last_failure_ = 21;
    UObject* host = host_.Get();
    UObject* layout = layout_library_.Get();
    if (!host || !layout || !controller || owner_.Get() != controller) return false;
    UObject* world = reinterpret_cast<UObject*>(controller->GetWorld());
    if (!world || owner_world_.Get() != world) return false;
    ObjectResult owner{};
    host->ProcessEvent(owning_player_, &owner);
    if (owner.result != controller) return false;
    ViewportSize size{controller};
    ViewportScale scale{controller};
    layout->ProcessEvent(viewport_size_, &size);
    layout->ProcessEvent(viewport_scale_, &scale);
    if (!std::isfinite(scale.result) || scale.result < 0.1F || scale.result > 10.0F
        || !std::isfinite(size.result.x) || !std::isfinite(size.result.y)
        || size.result.x < 160.0 || size.result.y < 120.0) return false;
    const double width = size.result.x / scale.result;
    const double height = size.result.y / scale.result;
    if (std::abs(width_ - width) > 0.1 || std::abs(height_ - height) > 0.1) {
        vector_call(host, viewport_desired_, width, height);
        width_ = width;
        height_ = height;
    }

    selection_ = dswros::refresh_scene_frame_selection(player, markers, settings_);
    std::array<dswros::SceneProjectedPoint, kSceneUmgMarkerCapacity> projected{};
    // Up to 50 markers share this frame's camera and engine-calibrated axes.
    // Two depth samples verify both perspective axes. A real retained marker
    // must additionally check custom projection overrides. Any mismatch,
    // orthographic view or missing witness keeps the native path this frame.
    auto projection = selection_.count >= 8
        ? capture_projection_unsafe(controller, scale.result) : dswros::SceneFrameProjection{};
    std::size_t witness = selection_.count;
    if (projection.valid) {
        for (std::size_t i = 0; i < selection_.count; ++i) {
            const auto point = dswros::scene_display_position(selection_.values[i].marker);
            const auto predicted = projection.project(point);
            if (!predicted.in_front) continue;
            projected[i] = project_engine_unsafe(controller, point);
            witness = i;
            projection.valid = dswros::scene_projection_matches(
                predicted, projected[i], 0.25 / scale.result);
            break;
        }
        projection.valid = projection.valid && witness < selection_.count;
    }
    if (selection_.count >= 8) {
        if (projection.valid) ++batch_frame_count_;
        else ++batch_fallback_count_;
    }
    for (std::size_t i = 0; i < selection_.count; ++i) {
        const auto display_position = dswros::scene_display_position(
            selection_.values[i].marker);
        if (i == witness) continue;
        projected[i] = projection.valid ? projection.project(display_position)
            : project_engine_unsafe(controller, display_position);
    }
    const auto now = GetTickCount64();
    const auto frame = dswros::layout_scene_markers(
        selection_, projected, width, height, settings_, focus_state_, now,
        {previous_visible_.data(), previous_visible_count_});
    focus_state_ = frame.focus_state;
    previous_visible_count_ = frame.count;
    for (std::size_t i = 0; i < frame.count; ++i) {
        const auto& marker = selection_.values[frame.values[i].candidate_index].marker;
        previous_visible_[i] = {marker.id, marker.kind};
    }
    const bool refresh_distances = now >= distance_refresh_at_;
    if (refresh_distances) distance_refresh_at_ = now + 100;
    // All-mode cold starts reveal labels gradually while glyphs already work.
    // No frame creates 50 widgets or interns the entire 0..1000 text table.
    distance_widget_budget_ = 4;
    distance_value_budget_ = 8;
    last_failure_ = 22;
    for (std::size_t i = 0; i < kSceneUmgMarkerCapacity; ++i) {
        UObject* group = groups_[i].Get();
        if (!group) return false;
        const bool show = i < frame.count;
        if (show) {
            const auto& marker = frame.values[i];
            const auto kind = selection_.values[marker.candidate_index].marker.kind;
            if (!style_valid_[i] || displayed_kinds_[i] != kind) {
                const auto texture_index = static_cast<std::size_t>(kind);
                if (texture_index >= marker_textures_.size()
                    || !bind_marker_texture_unsafe(
                        marker_images_[i].Get(), marker_textures_[texture_index].Get()))
                    return false;
                displayed_kinds_[i] = kind;
                style_valid_[i] = true;
            }
            // The lifted projected point remains the glyph/focus center. The
            // chevron is a child decoration and does not move that anchor.
            // Compare with the last submitted position, not the last sample,
            // so subpixel drift accumulates rather than freezing a slow target.
            // Submit every changed subpixel position without a dead band.
            if (!position_valid_[i] || marker.x != submitted_positions_[i].x
                || marker.y != submitted_positions_[i].y) {
                // Render translation follows the camera without changing the
                // Canvas layout offsets of every marker on every frame.
                vector_call(group, render_translation_, marker.x - 16.0, marker.y - 16.0);
                submitted_positions_[i] = {marker.x, marker.y};
                position_valid_[i] = true;
                ++position_update_count_;
            } else {
                ++position_reuse_count_;
            }
        }
        const bool show_distance = !distance_failed_ && show && frame.values[i].show_distance;
        if (show_distance || distance_shown_[i]) {
            const dswros::SceneVisibleMarker marker = show ? frame.values[i]
                : dswros::SceneVisibleMarker{};
            const dswros::SceneCandidate candidate = show
                ? selection_.values[marker.candidate_index] : dswros::SceneCandidate{};
            if (!update_distance_label_guarded(i, show_distance, marker,
                                              candidate, width, refresh_distances))
                disable_distance_labels();
        }
        if (shown_[i] != show) {
            visibility(group, visible_, show);
            shown_[i] = show;
        }
    }
    active_count_ = frame.count;
    const bool show_host = frame.count != 0;
    if (host_shown_ != show_host) {
        visibility(host, visible_, show_host);
        host_shown_ = show_host;
    }
    last_failure_ = 0;
    return true;
}

bool SceneUmgRenderer::run_guarded(Operation operation, UObject* controller,
    dswros::Position player, std::span<const SceneUmgMarker> markers) noexcept {
#if defined(_MSC_VER)
    __try {
        return run_unsafe(operation, controller, player, markers);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 100;
        return false;
    }
#else
    try {
        return run_unsafe(operation, controller, player, markers);
    } catch (...) {
        ++fault_count_;
        last_failure_ = 100;
        return false;
    }
#endif
}

bool SceneUmgRenderer::run_unsafe(Operation operation, UObject* controller,
    dswros::Position player, std::span<const SceneUmgMarker> markers) {
    switch (operation) {
    case Operation::Initialize: return initialize_unsafe();
    case Operation::Attach: return attach_unsafe(controller);
    case Operation::Update: return update_unsafe(controller, player, markers);
    case Operation::Hide:
        focus_state_ = {};
        if (UObject* host = host_.Get()) {
            visibility(host, visible_, false);
            host_shown_ = false;
            active_count_ = 0;
            return true;
        }
        return false;
    case Operation::Detach:
        if (UObject* host = host_.Get()) host->ProcessEvent(remove_, nullptr);
        release_distance_cache_unsafe();
        return true;
    }
    return false;
}

void SceneUmgRenderer::fail(std::uint32_t code) noexcept {
    (void)run_guarded(Operation::Detach);
    reset_handles();
    state_ = SceneUmgRendererState::Faulted;
    last_failure_ = code;
}

void SceneUmgRenderer::reset_handles() noexcept {
    host_ = FWeakObjectPtr{};
    owner_ = FWeakObjectPtr{};
    camera_manager_property_ = nullptr;
    owner_world_ = FWeakObjectPtr{};
    tree_ = FWeakObjectPtr{};
    groups_ = {};
    group_slots_ = {};
    marker_images_ = {};
    marker_textures_ = {};
    texture_keepers_ = {};
    image_brush_property_ = nullptr;
    brush_resource_property_ = nullptr;
    submitted_positions_ = {};
    position_valid_ = {};
    style_valid_ = {};
    shown_ = {};
    distance_labels_ = {};
    distance_slots_ = {};
    displayed_distances_.fill(1001);
    distance_identities_ = {};
    distance_shown_ = {};
    distance_pending_ = {};
    distance_on_left_ = {};
    distance_cache_ = {};
    distance_cache_initialized_ = {};
    distance_refresh_at_ = 0;
    distance_widget_budget_ = distance_value_budget_ = 0;
    distance_failed_ = false;
    last_text_failure_ = 0;
    focus_state_ = {};
    previous_visible_ = {};
    previous_visible_count_ = 0;
    host_shown_ = false;
    activation_ = false;
    attach_attempted_ = false;
    suppressed_ = false;
    selection_ = {};
    active_count_ = 0;
    width_ = height_ = 0;
}

void SceneUmgRenderer::detach() noexcept {
    const bool success = run_guarded(Operation::Detach);
    reset_handles();
    if (!success) state_ = SceneUmgRendererState::Faulted;
    else if (state_ == SceneUmgRendererState::Attached
             || state_ == SceneUmgRendererState::Suppressed)
        state_ = SceneUmgRendererState::Ready;
}

void SceneUmgRenderer::abandon_runtime_handles() noexcept {
    reset_handles();
    blueprint_library_ = FWeakObjectPtr{};
    layout_library_ = FWeakObjectPtr{};
    rendering_library_ = FWeakObjectPtr{};
    state_ = SceneUmgRendererState::Disabled;
}

} // namespace dsnwr
