#include "status_toast_renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#pragma warning(pop)

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <windows.h>

namespace dsnap::ue4ss {
namespace {

using namespace RC::Unreal;

struct Vector2D {
    double x{};
    double y{};
};

struct CreateWidgetParameters {
    UObject* world_context_object{};
    UClass* widget_type{};
    UObject* owning_player{};
    UObject* return_value{};
};

struct ObjectReturnParameters {
    UObject* return_value{};
};

struct ViewportSizeParameters {
    UObject* world_context_object{};
    Vector2D return_value{};
};

struct ViewportScaleParameters {
    UObject* world_context_object{};
    float return_value{};
};

struct AddToViewportParameters {
    std::int32_t z_order{};
};

struct AddChildToCanvasParameters {
    UObject* content{};
    UObject* return_value{};
};

struct VectorParameters {
    Vector2D value{};
};

struct PositionInViewportParameters {
    Vector2D position{};
    bool remove_dpi_scale{};
};

struct ZOrderParameters {
    std::int32_t value{};
};

struct VisibilityParameters {
    std::uint8_t visibility{};
};

struct LinearColor {
    float red{};
    float green{};
    float blue{};
    float alpha{};
};

struct BrushColorParameters {
    LinearColor color{};
};

struct TextParameters {
    FText text{};

    explicit TextParameters(const wchar_t* value) : text(value) {}
};

struct ScalarParameters {
    float value{};
};

static_assert(sizeof(Vector2D) == 16);
static_assert(sizeof(CreateWidgetParameters) == 32);
static_assert(sizeof(ObjectReturnParameters) == 8);
static_assert(sizeof(ViewportSizeParameters) == 24);
static_assert(sizeof(ViewportScaleParameters) == 16);
static_assert(sizeof(AddToViewportParameters) == 4);
static_assert(sizeof(AddChildToCanvasParameters) == 16);
static_assert(sizeof(VectorParameters) == 16);
static_assert(sizeof(PositionInViewportParameters) == 24);
static_assert(sizeof(ZOrderParameters) == 4);
static_assert(sizeof(VisibilityParameters) == 1);
static_assert(sizeof(LinearColor) == 16);
static_assert(sizeof(BrushColorParameters) == 16);
static_assert(sizeof(FText) == 24);
static_assert(sizeof(TextParameters) == 24);
static_assert(sizeof(ScalarParameters) == 4);

constexpr std::uint8_t kVisible = 0;
constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
constexpr std::int32_t kViewportZOrder = 2'000'000'050;
constexpr double kReferenceWidth = 360.0;
constexpr double kReferenceHeight = 82.0;
constexpr double kReferenceViewportWidth = 2560.0;
constexpr double kReferenceViewportHeight = 1440.0;
constexpr double kReferenceTop = 44.0;

constexpr LinearColor kShadow{0.0F, 0.0F, 0.0F, 0.28F};
constexpr LinearColor kFrame{
    80.0F / 255.0F, 111.0F / 255.0F, 126.0F / 255.0F, 0.66F};
constexpr LinearColor kBackground{
    8.0F / 255.0F, 20.0F / 255.0F, 27.0F / 255.0F, 0.72F};
constexpr LinearColor kInnerGlass{
    4.0F / 255.0F, 10.0F / 255.0F, 14.0F / 255.0F, 0.32F};
constexpr LinearColor kTopHighlight{1.0F, 1.0F, 1.0F, 0.13F};
constexpr LinearColor kStarting{
    244.0F / 255.0F, 184.0F / 255.0F, 70.0F / 255.0F, 1.0F};
constexpr LinearColor kEnabled{
    82.0F / 255.0F, 218.0F / 255.0F, 157.0F / 255.0F, 1.0F};
constexpr LinearColor kDisabled{
    128.0F / 255.0F, 158.0F / 255.0F, 176.0F / 255.0F, 1.0F};
constexpr LinearColor kUnavailable{
    235.0F / 255.0F, 103.0F / 255.0F, 111.0F / 255.0F, 1.0F};

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

void set_visibility(UObject* widget, UFunction* function,
                    std::uint8_t visibility) {
    VisibilityParameters parameters{visibility};
    widget->ProcessEvent(function, &parameters);
}

void set_vector(UObject* object, UFunction* function, double x, double y) {
    VectorParameters parameters{{x, y}};
    object->ProcessEvent(function, &parameters);
}

void set_z_order(UObject* object, UFunction* function, std::int32_t value) {
    ZOrderParameters parameters{value};
    object->ProcessEvent(function, &parameters);
}

void set_brush_color(UObject* border, UFunction* function,
                     const LinearColor& color) {
    BrushColorParameters parameters{color};
    border->ProcessEvent(function, &parameters);
}

void set_text(UObject* text_block, UFunction* function,
              FTextProperty* text_property, const wchar_t* value) {
    TextParameters parameters{value};
    text_block->ProcessEvent(function, &parameters);
    text_property->DestroyValue_InContainer(&parameters);
}

[[nodiscard]] FProperty* find_function_property(
    UFunction* function, const wchar_t* name) noexcept {
    if (!function || !name) return nullptr;
    for (FProperty* property : TFieldRange<FProperty>(
             function,
             EFieldIterationFlags::IncludeSuper
                 | EFieldIterationFlags::IncludeDeprecated)) {
        if (property->GetName() == name) return property;
    }
    return nullptr;
}

[[nodiscard]] bool write_object_property(
    UObject* object, const wchar_t* name, UObject* value) {
    if (!object) return false;
    auto* property = CastField<FObjectPropertyBase>(
        object->GetPropertyByNameInChain(name));
    void* address = object->GetValuePtrByPropertyNameInChain(name);
    if (!property || !address) return false;
    property->SetObjectPropertyValue(address, value);
    return property->GetObjectPropertyValue(address) == value;
}

[[nodiscard]] UObject* named_object(UObject* owner, const wchar_t* name) {
    if (!owner) return nullptr;
    auto** value = owner->GetValuePtrByPropertyNameInChain<UObject*>(name);
    return value ? *value : nullptr;
}

[[nodiscard]] UObject* resolve_local_controller(UEngine* engine) {
    if (!engine) return nullptr;
    auto** viewport_value =
        engine->GetValuePtrByPropertyNameInChain<UObject*>(L"GameViewport");
    UObject* viewport = viewport_value ? *viewport_value : nullptr;
    UObject* game_instance = named_object(viewport, L"GameInstance");
    auto* players = game_instance
        ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(
              L"LocalPlayers")
        : nullptr;
    if (!players || !players->IsValidIndex(0) || !players->GetData()) {
        return nullptr;
    }
    UObject* local_player =
        static_cast<UObject* const*>(players->GetData())[0];
    UObject* controller = named_object(local_player, L"PlayerController");
    if (!controller || named_object(controller, L"Player") != local_player) {
        return nullptr;
    }
    return controller;
}

[[nodiscard]] const wchar_t* message_for(StatusToastKind kind) noexcept {
    switch (kind) {
    case StatusToastKind::Starting:
        return L"STARTING...";
    case StatusToastKind::Enabled:
        return L"ENABLED";
    case StatusToastKind::Disabled:
        return L"DISABLED";
    case StatusToastKind::Unavailable:
        return L"NOT READY";
    case StatusToastKind::None:
    default:
        return L"";
    }
}

[[nodiscard]] const LinearColor& accent_for(StatusToastKind kind) noexcept {
    switch (kind) {
    case StatusToastKind::Starting:
        return kStarting;
    case StatusToastKind::Enabled:
        return kEnabled;
    case StatusToastKind::Disabled:
        return kDisabled;
    case StatusToastKind::Unavailable:
        return kUnavailable;
    case StatusToastKind::None:
    default:
        return kDisabled;
    }
}

[[nodiscard]] LinearColor with_alpha(const LinearColor& color,
                                     float alpha) noexcept {
    return {color.red, color.green, color.blue, alpha};
}

} // namespace

void StatusToastRenderer::initialize() noexcept {
    user_widget_class_ = find<UClass>(L"/Script/UMG.UserWidget");
    widget_tree_class_ = find<UClass>(L"/Script/UMG.WidgetTree");
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    border_class_ = find<UClass>(L"/Script/UMG.Border");
    text_block_class_ = find<UClass>(L"/Script/UMG.TextBlock");
    widget_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    widget_layout_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");

    create_widget_ =
        find<UFunction>(L"/Script/UMG.WidgetBlueprintLibrary:Create");
    get_owning_player_ = find<UFunction>(L"/Script/UMG.Widget:GetOwningPlayer");
    get_viewport_size_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:GetViewportSize");
    get_viewport_scale_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:GetViewportScale");
    add_to_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:AddToViewport");
    add_child_to_canvas_ =
        find<UFunction>(L"/Script/UMG.CanvasPanel:AddChildToCanvas");
    set_slot_position_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetPosition");
    set_slot_size_ = find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetSize");
    set_slot_alignment_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetAlignment");
    set_slot_z_order_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetZOrder");
    set_visibility_ = find<UFunction>(L"/Script/UMG.Widget:SetVisibility");
    set_brush_color_ = find<UFunction>(L"/Script/UMG.Border:SetBrushColor");
    set_text_ = find<UFunction>(L"/Script/UMG.TextBlock:SetText");
    set_text_value_property_ = CastField<FTextProperty>(
        find_function_property(set_text_, L"InText"));
    set_render_opacity_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderOpacity");
    set_render_translation_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderTranslation");
    set_render_scale_ = find<UFunction>(L"/Script/UMG.Widget:SetRenderScale");
    set_render_pivot_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderTransformPivot");
    set_position_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetPositionInViewport");
    set_alignment_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetAlignmentInViewport");
    set_desired_size_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetDesiredSizeInViewport");
    force_layout_prepass_ =
        find<UFunction>(L"/Script/UMG.Widget:ForceLayoutPrepass");
    remove_from_parent_ =
        find<UFunction>(L"/Script/UMG.Widget:RemoveFromParent");

    abi_failure_mask_ = 0;
    const auto require_parameters = [this](std::uint32_t bit,
                                            UFunction* function,
                                            std::int32_t size) {
        if (!function || function->GetParmsSize() != size) {
            abi_failure_mask_ |= bit;
        }
    };
    require_parameters(1U << 0U, create_widget_, 32);
    require_parameters(1U << 1U, get_owning_player_, 8);
    require_parameters(1U << 2U, get_viewport_size_, 24);
    require_parameters(1U << 3U, get_viewport_scale_, 12);
    require_parameters(1U << 4U, add_to_viewport_, 4);
    require_parameters(1U << 5U, add_child_to_canvas_, 16);
    require_parameters(1U << 6U, set_slot_position_, 16);
    require_parameters(1U << 7U, set_slot_size_, 16);
    require_parameters(1U << 8U, set_slot_alignment_, 16);
    require_parameters(1U << 9U, set_slot_z_order_, 4);
    require_parameters(1U << 10U, set_visibility_, 1);
    require_parameters(1U << 11U, set_brush_color_, 16);
    require_parameters(1U << 12U, set_text_, 24);
    require_parameters(1U << 13U, set_render_opacity_, 4);
    require_parameters(1U << 14U, set_render_translation_, 16);
    require_parameters(1U << 15U, set_render_scale_, 16);
    require_parameters(1U << 16U, set_render_pivot_, 16);
    require_parameters(1U << 17U, set_position_in_viewport_, 17);
    require_parameters(1U << 18U, set_alignment_in_viewport_, 16);
    require_parameters(1U << 19U, set_desired_size_in_viewport_, 16);
    require_parameters(1U << 20U, force_layout_prepass_, 0);
    require_parameters(1U << 21U, remove_from_parent_, 0);
    if (!set_text_value_property_
        || set_text_value_property_->GetOffset_Internal() != 0
        || set_text_value_property_->GetSize()
            != static_cast<std::int32_t>(sizeof(FText))) {
        abi_failure_mask_ |= 1U << 22U;
    }
    if (!user_widget_class_ || !widget_tree_class_ || !canvas_panel_class_
        || !border_class_ || !text_block_class_ || !widget_blueprint_library_
        || !widget_layout_library_) {
        abi_failure_mask_ |= 1U << 23U;
    }
    reset_runtime_handles();
    state_ = abi_failure_mask_ == 0
        ? StatusToastRendererState::Ready
        : StatusToastRendererState::Disabled;
}

void StatusToastRenderer::notify(StatusToastKind kind,
                                 Clock::time_point now) noexcept {
    timeline_.notify(kind, now);
}

void StatusToastRenderer::tick(UEngine* engine, Clock::time_point now) noexcept {
    if (state_ == StatusToastRendererState::Disabled
        || state_ == StatusToastRendererState::Faulted
        || state_ == StatusToastRendererState::Uninitialized) {
        return;
    }
    tick_guarded(engine, timeline_.frame(now));
}

void StatusToastRenderer::tick_guarded(
    UEngine* engine, const StatusToastFrame& frame) noexcept {
#if defined(_MSC_VER)
    __try {
        tick_unsafe(engine, frame);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 100;
        state_ = StatusToastRendererState::Faulted;
        timeline_.clear();
        reset_runtime_handles();
    }
#else
    try {
        tick_unsafe(engine, frame);
    } catch (...) {
        ++fault_count_;
        last_failure_ = 100;
        state_ = StatusToastRendererState::Faulted;
        timeline_.clear();
        reset_runtime_handles();
    }
#endif
}

void StatusToastRenderer::tick_unsafe(
    UEngine* engine, const StatusToastFrame& frame) {
    UObject* host = host_.Get();
    if (!frame.visible) {
        if (host && host_visible_) {
            set_visibility(host, set_visibility_, kCollapsed);
            host_visible_ = false;
        }
        return;
    }

    UObject* controller = resolve_local_controller(engine);
    if (!controller) {
        last_failure_ = 1;
        return;
    }
    if (host) {
        ObjectReturnParameters owner{};
        host->ProcessEvent(get_owning_player_, &owner);
        if (owner.return_value != controller) {
            host->ProcessEvent(remove_from_parent_, nullptr);
            reset_runtime_handles();
            host = nullptr;
        }
    }
    if (!host) {
        if (attach_attempted_revision_ == frame.revision) return;
        attach_attempted_revision_ = frame.revision;
        if (!attach(engine, controller)) return;
    }
    host = host_.Get();
    if (!host) {
        last_failure_ = 2;
        return;
    }

    if (rendered_revision_ != frame.revision && !set_message(frame)) {
        ++fault_count_;
        state_ = StatusToastRendererState::Faulted;
        timeline_.clear();
        reset_runtime_handles();
        return;
    }
    if (!host_visible_) {
        set_visibility(host, set_visibility_, kHitTestInvisible);
        host_visible_ = true;
    }
    ScalarParameters opacity{static_cast<float>(frame.opacity)};
    host->ProcessEvent(set_render_opacity_, &opacity);
    set_vector(host, set_render_translation_, 0.0, frame.vertical_offset);
    const double scale = 0.985 + 0.015 * frame.opacity;
    set_vector(host, set_render_scale_, scale, scale);
}

bool StatusToastRenderer::attach(UEngine* engine, UObject* controller) {
    if (!engine || !controller || !widget_blueprint_library_
        || !widget_layout_library_) {
        last_failure_ = 3;
        return false;
    }
    ViewportSizeParameters viewport_size{controller};
    widget_layout_library_->ProcessEvent(get_viewport_size_, &viewport_size);
    ViewportScaleParameters viewport_scale{controller};
    widget_layout_library_->ProcessEvent(get_viewport_scale_, &viewport_scale);
    if (!std::isfinite(viewport_size.return_value.x)
        || !std::isfinite(viewport_size.return_value.y)
        || viewport_size.return_value.x < 640.0
        || viewport_size.return_value.y < 360.0
        || !std::isfinite(viewport_scale.return_value)
        || viewport_scale.return_value < 0.1F
        || viewport_scale.return_value > 10.0F) {
        last_failure_ = 4;
        return false;
    }
    const double display_scale = std::clamp(
        std::min(viewport_size.return_value.x / kReferenceViewportWidth,
                 viewport_size.return_value.y / kReferenceViewportHeight),
        0.65, 2.25);
    const double unit_scale =
        display_scale / static_cast<double>(viewport_scale.return_value);

    CreateWidgetParameters create{
        controller, user_widget_class_, controller, nullptr};
    widget_blueprint_library_->ProcessEvent(create_widget_, &create);
    UObject* host = create.return_value;
    if (!host) {
        last_failure_ = 5;
        return false;
    }
    ObjectReturnParameters owner{};
    host->ProcessEvent(get_owning_player_, &owner);
    if (owner.return_value != controller) {
        last_failure_ = 6;
        return false;
    }

    UObject* tree = UObjectGlobals::NewObject<UObject>(host, widget_tree_class_);
    UObject* root = tree
        ? UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_)
        : nullptr;
    if (!tree || !root || !write_object_property(host, L"WidgetTree", tree)
        || !write_object_property(tree, L"RootWidget", root)) {
        last_failure_ = 7;
        return false;
    }

    const auto add_widget = [this, root, unit_scale](
        UObject* widget, double x, double y, double width, double height,
        std::int32_t z_order) -> bool {
        if (!widget) return false;
        AddChildToCanvasParameters add{widget};
        root->ProcessEvent(add_child_to_canvas_, &add);
        UObject* slot = add.return_value;
        if (!slot) return false;
        set_vector(slot, set_slot_position_, x * unit_scale, y * unit_scale);
        set_vector(slot, set_slot_size_, width * unit_scale,
                   height * unit_scale);
        set_vector(slot, set_slot_alignment_, 0.0, 0.0);
        set_z_order(slot, set_slot_z_order_, z_order);
        return true;
    };
    const auto add_border = [this, tree, &add_widget](
        double x, double y, double width, double height, std::int32_t z_order,
        const LinearColor& color) -> UObject* {
        UObject* border =
            UObjectGlobals::NewObject<UObject>(tree, border_class_);
        if (!border) return nullptr;
        set_brush_color(border, set_brush_color_, color);
        set_visibility(border, set_visibility_, kHitTestInvisible);
        return add_widget(border, x, y, width, height, z_order)
            ? border
            : nullptr;
    };
    const auto add_text = [this, tree, &add_widget, unit_scale](
        const wchar_t* text, double x, double y, double width, double height,
        std::int32_t z_order, double scale_value) -> UObject* {
        UObject* block =
            UObjectGlobals::NewObject<UObject>(tree, text_block_class_);
        if (!block) return nullptr;
        set_text(block, set_text_, set_text_value_property_, text);
        set_vector(block, set_render_pivot_, 0.0, 0.0);
        const double text_scale = unit_scale * scale_value;
        set_vector(block, set_render_scale_, text_scale, text_scale);
        set_visibility(block, set_visibility_, kHitTestInvisible);
        return add_widget(block, x, y, width, height, z_order)
            ? block
            : nullptr;
    };

    UObject* shadow = add_border(
        4.0, 6.0, kReferenceWidth - 8.0, kReferenceHeight - 8.0, 0, kShadow);
    UObject* frame = add_border(
        0.0, 0.0, kReferenceWidth - 4.0, kReferenceHeight - 6.0, 1, kFrame);
    UObject* background = add_border(
        1.0, 1.0, kReferenceWidth - 6.0, kReferenceHeight - 8.0, 2,
        kBackground);
    UObject* inner_glass = add_border(
        7.0, 7.0, kReferenceWidth - 18.0, kReferenceHeight - 20.0, 3,
        kInnerGlass);
    UObject* highlight = add_border(
        8.0, 7.0, kReferenceWidth - 20.0, 1.0, 4, kTopHighlight);
    UObject* glow = add_border(
        7.0, 8.0, 12.0, kReferenceHeight - 22.0, 4,
        with_alpha(kStarting, 0.14F));
    UObject* accent = add_border(
        7.0, 8.0, 3.0, kReferenceHeight - 22.0, 5, kStarting);
    UObject* indicator = add_border(20.0, 18.0, 7.0, 7.0, 6, kStarting);
    UObject* status_rule = add_border(
        10.0, kReferenceHeight - 12.0, kReferenceWidth - 24.0, 2.0, 5,
        with_alpha(kStarting, 0.38F));
    UObject* title = add_text(
        L"AUTO PICKUP", 36.0, 10.0, 270.0, 22.0, 7, 0.40);
    UObject* status = add_text(
        L"STARTING...", 20.0, 34.0, 310.0, 32.0, 7, 0.66);
    if (!shadow || !frame || !background || !inner_glass || !highlight
        || !glow || !accent || !indicator || !status_rule || !title || !status) {
        last_failure_ = 8;
        return false;
    }

    host_ = host;
    glow_ = glow;
    accent_ = accent;
    indicator_ = indicator;
    status_rule_ = status_rule;
    status_text_ = status;
    rendered_revision_ = 0;
    host_visible_ = false;

    set_visibility(host, set_visibility_, kCollapsed);
    AddToViewportParameters add_to_viewport{kViewportZOrder};
    host->ProcessEvent(add_to_viewport_, &add_to_viewport);
    set_vector(host, set_alignment_in_viewport_, 0.5, 0.0);
    set_vector(host, set_render_pivot_, 0.5, 0.0);
    set_vector(host, set_desired_size_in_viewport_,
               kReferenceWidth * unit_scale, kReferenceHeight * unit_scale);
    PositionInViewportParameters position{{
        viewport_size.return_value.x * 0.5,
        kReferenceTop * display_scale}, true};
    host->ProcessEvent(set_position_in_viewport_, &position);
    host->ProcessEvent(force_layout_prepass_, nullptr);

    ++attach_count_;
    last_failure_ = 0;
    state_ = StatusToastRendererState::Attached;
    return true;
}

bool StatusToastRenderer::set_message(const StatusToastFrame& frame) {
    UObject* glow = glow_.Get();
    UObject* accent = accent_.Get();
    UObject* indicator = indicator_.Get();
    UObject* status_rule = status_rule_.Get();
    UObject* status = status_text_.Get();
    if (!glow || !accent || !indicator || !status_rule || !status) {
        last_failure_ = 9;
        return false;
    }
    const LinearColor& color = accent_for(frame.kind);
    set_brush_color(glow, set_brush_color_, with_alpha(color, 0.14F));
    set_brush_color(accent, set_brush_color_, color);
    set_brush_color(indicator, set_brush_color_, color);
    set_brush_color(status_rule, set_brush_color_, with_alpha(color, 0.38F));
    set_text(status, set_text_, set_text_value_property_, message_for(frame.kind));
    rendered_revision_ = frame.revision;
    return true;
}

void StatusToastRenderer::release_for_travel() noexcept {
    timeline_.clear();
    reset_runtime_handles();
    if (state_ == StatusToastRendererState::Attached) {
        state_ = StatusToastRendererState::Ready;
    }
}

void StatusToastRenderer::shutdown_guarded() noexcept {
#if defined(_MSC_VER)
    __try {
        shutdown_unsafe();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 101;
        reset_runtime_handles();
    }
#else
    try {
        shutdown_unsafe();
    } catch (...) {
        ++fault_count_;
        last_failure_ = 101;
        reset_runtime_handles();
    }
#endif
    timeline_.clear();
}

void StatusToastRenderer::shutdown_unsafe() {
    if (UObject* host = host_.Get(); host && remove_from_parent_) {
        host->ProcessEvent(remove_from_parent_, nullptr);
    }
    reset_runtime_handles();
}

void StatusToastRenderer::reset_runtime_handles() noexcept {
    host_ = FWeakObjectPtr{};
    glow_ = FWeakObjectPtr{};
    accent_ = FWeakObjectPtr{};
    indicator_ = FWeakObjectPtr{};
    status_rule_ = FWeakObjectPtr{};
    status_text_ = FWeakObjectPtr{};
    rendered_revision_ = 0;
    attach_attempted_revision_ = 0;
    host_visible_ = false;
}

} // namespace dsnap::ue4ss
