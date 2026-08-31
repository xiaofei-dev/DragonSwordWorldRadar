#include "radar_visibility_hub.hpp"

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/UClass.hpp>
#include <Unreal/FProperty.hpp>
#else
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#endif
#include <Unreal/FText.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include "ue4ss_compat.hpp"
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <windows.h>

namespace dsnwr {
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

struct BoolParameters {
    bool value{};
};

struct InputModeGameAndUiParameters {
    UObject* player_controller{};
    UObject* widget_to_focus{};
    std::uint8_t mouse_lock_mode{};
    bool hide_cursor_during_capture{};
    bool flush_input{};
};

struct InputModeGameOnlyParameters {
    UObject* player_controller{};
    bool flush_input{};
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
static_assert(sizeof(BoolParameters) == 1);
static_assert(sizeof(InputModeGameAndUiParameters) == 24);
static_assert(sizeof(InputModeGameOnlyParameters) == 16);

constexpr std::uint8_t kVisible = 0;
constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
constexpr std::int32_t kViewportZOrder = 2'000'000'100;
constexpr double kReferencePanelWidth = 620.0;
constexpr double kReferencePanelHeight = 526.0;

constexpr LinearColor kPanelFrame{
    25.0F / 255.0F, 46.0F / 255.0F, 57.0F / 255.0F, 0.98F};
constexpr LinearColor kPanelBackground{
    6.0F / 255.0F, 13.0F / 255.0F, 18.0F / 255.0F, 0.97F};
constexpr LinearColor kPanelAccent{
    108.0F / 255.0F, 216.0F / 255.0F, 226.0F / 255.0F, 1.0F};
constexpr LinearColor kHeaderBackground{
    10.0F / 255.0F, 25.0F / 255.0F, 35.0F / 255.0F, 0.98F};
constexpr LinearColor kContentBackground{
    8.0F / 255.0F, 18.0F / 255.0F, 25.0F / 255.0F, 0.94F};
constexpr LinearColor kBaseRow{
    13.0F / 255.0F, 27.0F / 255.0F, 35.0F / 255.0F, 0.36F};
constexpr LinearColor kAlternateRow{
    20.0F / 255.0F, 39.0F / 255.0F, 48.0F / 255.0F, 0.42F};
constexpr LinearColor kRowDivider{
    36.0F / 255.0F, 62.0F / 255.0F, 73.0F / 255.0F, 0.70F};
constexpr LinearColor kCompactHeader{
    46.0F / 255.0F, 153.0F / 255.0F, 178.0F / 255.0F, 0.32F};
constexpr LinearColor kWorldHeader{
    62.0F / 255.0F, 159.0F / 255.0F, 112.0F / 255.0F, 0.32F};
constexpr LinearColor kToggleFrame{
    36.0F / 255.0F, 57.0F / 255.0F, 67.0F / 255.0F, 1.0F};
constexpr LinearColor kCompactEnabled{
    72.0F / 255.0F, 205.0F / 255.0F, 1.0F, 1.0F};
constexpr LinearColor kWorldEnabled{
    101.0F / 255.0F, 226.0F / 255.0F, 154.0F / 255.0F, 1.0F};
constexpr LinearColor kCloseButton{
    86.0F / 255.0F, 20.0F / 255.0F, 25.0F / 255.0F, 0.96F};

struct RowDefinition {
    const wchar_t* label{};
    RadarVisibilityCategory category{RadarVisibilityCategory::Clock};
};

constexpr std::array<RowDefinition, 7> kRows{{
    {L"CLOCK", RadarVisibilityCategory::Clock},
    {L"TREASURE", RadarVisibilityCategory::Treasure},
    {L"BOSS", RadarVisibilityCategory::Boss},
    {L"ASSAULT", RadarVisibilityCategory::Assault},
    {L"MINI-GAMES", RadarVisibilityCategory::MiniGames},
    {L"AREA QUESTS", RadarVisibilityCategory::AreaQuests},
    {L"BIRD EGGS", RadarVisibilityCategory::BirdEggs},
}};

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

void set_visibility(
    UObject* widget, UFunction* function, std::uint8_t visibility) {
    VisibilityParameters parameters{visibility};
    widget->ProcessEvent(function, &parameters);
}

void set_slot_vector(
    UObject* slot, UFunction* function, double x, double y) {
    VectorParameters parameters{{x, y}};
    slot->ProcessEvent(function, &parameters);
}

void set_slot_z_order(
    UObject* slot, UFunction* function, std::int32_t value) {
    ZOrderParameters parameters{value};
    slot->ProcessEvent(function, &parameters);
}

void set_brush_color(
    UObject* border, UFunction* function, const LinearColor& color) {
    BrushColorParameters parameters{color};
    border->ProcessEvent(function, &parameters);
}

[[nodiscard]] FProperty* find_function_property(
    UFunction* function, const wchar_t* name) noexcept {
    if (!function || !name) {
        return nullptr;
    }
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function)) {
        if (property->GetName() == name) {
            return property;
        }
    }
    return nullptr;
}

void set_text(
    UObject* text_block, UFunction* function, FProperty* text_property,
    const wchar_t* value) {
    TextParameters parameters{value};
    text_block->ProcessEvent(function, &parameters);
    // RE-UE4SS models FText as a layout wrapper without a C++ destructor.
    // SetText copies the value, so release this reflected temporary explicitly.
    text_property->DestroyValue_InContainer(&parameters);
}

void set_render_opacity(
    UObject* widget, UFunction* function, float value) {
    ScalarParameters parameters{value};
    widget->ProcessEvent(function, &parameters);
}

void set_checked(UObject* check_box, UFunction* function, bool value) {
    BoolParameters parameters{value};
    check_box->ProcessEvent(function, &parameters);
}

[[nodiscard]] bool is_checked(UObject* check_box, UFunction* function) {
    BoolParameters parameters{};
    check_box->ProcessEvent(function, &parameters);
    return parameters.value;
}

[[nodiscard]] bool write_object_property(
    UObject* object, const wchar_t* property_name, UObject* value) {
    if (!object) {
        return false;
    }
    auto* property = CastField<FObjectPropertyBase>(
        object->GetPropertyByNameInChain(property_name));
    void* address =
        object->GetValuePtrByPropertyNameInChain(property_name);
    if (!property || !address) {
        return false;
    }
    property->SetObjectPropertyValue(address, value);
    return property->GetObjectPropertyValue(address) == value;
}

[[nodiscard]] bool read_cursor_visible(
    UObject* controller, bool& visible) {
    if (!controller) {
        return false;
    }
    auto* property = CastField<FBoolProperty>(
        controller->GetPropertyByNameInChain(L"bShowMouseCursor"));
    void* address = controller->GetValuePtrByPropertyNameInChain(
        L"bShowMouseCursor");
    if (!property || !address) {
        return false;
    }
    visible = property->GetPropertyValue(address);
    return true;
}

[[nodiscard]] bool write_cursor_visible(
    UObject* controller, bool visible) {
    if (!controller) {
        return false;
    }
    auto* property = CastField<FBoolProperty>(
        controller->GetPropertyByNameInChain(L"bShowMouseCursor"));
    void* address = controller->GetValuePtrByPropertyNameInChain(
        L"bShowMouseCursor");
    if (!property || !address) {
        return false;
    }
    property->SetPropertyValue(address, visible);
    return property->GetPropertyValue(address) == visible;
}

[[nodiscard]] RadarVisibilityMaskWord sanitize_masks(
    RadarVisibilityMaskWord masks) noexcept {
    return pack_radar_visibility_masks(
        compact_radar_visibility_mask(masks),
        world_radar_visibility_mask(masks));
}

} // namespace

void RadarVisibilityHub::initialize() noexcept {
    user_widget_class_ = find<UClass>(L"/Script/UMG.UserWidget");
    widget_tree_class_ = find<UClass>(L"/Script/UMG.WidgetTree");
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    border_class_ = find<UClass>(L"/Script/UMG.Border");
    text_block_class_ = find<UClass>(L"/Script/UMG.TextBlock");
    check_box_class_ = find<UClass>(L"/Script/UMG.CheckBox");
    widget_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    widget_layout_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");

    create_widget_ =
        find<UFunction>(L"/Script/UMG.WidgetBlueprintLibrary:Create");
    get_owning_player_ =
        find<UFunction>(L"/Script/UMG.Widget:GetOwningPlayer");
    get_viewport_size_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:GetViewportSize");
    get_viewport_scale_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:GetViewportScale");
    add_to_viewport_ =
        find<UFunction>(L"/Script/UMG.UserWidget:AddToViewport");
    add_child_to_canvas_ =
        find<UFunction>(L"/Script/UMG.CanvasPanel:AddChildToCanvas");
    set_slot_position_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetPosition");
    set_slot_size_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetSize");
    set_slot_alignment_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetAlignment");
    set_slot_z_order_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:SetZOrder");
    set_visibility_ =
        find<UFunction>(L"/Script/UMG.Widget:SetVisibility");
    set_brush_color_ =
        find<UFunction>(L"/Script/UMG.Border:SetBrushColor");
    set_text_ = find<UFunction>(L"/Script/UMG.TextBlock:SetText");
    set_text_value_property_ = CastField<FTextProperty>(
        find_function_property(set_text_, L"InText"));
    set_render_opacity_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderOpacity");
    set_render_scale_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderScale");
    set_render_pivot_ = find<UFunction>(
        L"/Script/UMG.Widget:SetRenderTransformPivot");
    set_is_checked_ =
        find<UFunction>(L"/Script/UMG.CheckBox:SetIsChecked");
    is_checked_ = find<UFunction>(L"/Script/UMG.CheckBox:IsChecked");
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
    set_input_mode_game_and_ui_ = find<UFunction>(
        L"/Script/UMG.WidgetBlueprintLibrary:SetInputMode_GameAndUIEx");
    set_input_mode_game_only_ = find<UFunction>(
        L"/Script/UMG.WidgetBlueprintLibrary:SetInputMode_GameOnly");

    abi_failure_mask_ = 0;
    const auto require_parameters =
        [this](std::uint32_t bit, UFunction* function, std::int32_t size) {
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
    require_parameters(1U << 14U, set_is_checked_, 1);
    require_parameters(1U << 15U, is_checked_, 1);
    require_parameters(1U << 16U, set_position_in_viewport_, 17);
    require_parameters(1U << 17U, set_alignment_in_viewport_, 16);
    require_parameters(1U << 18U, set_desired_size_in_viewport_, 16);
    require_parameters(1U << 19U, force_layout_prepass_, 0);
    require_parameters(1U << 20U, remove_from_parent_, 0);
    require_parameters(1U << 21U, set_input_mode_game_and_ui_, 19);
    require_parameters(1U << 22U, set_input_mode_game_only_, 9);
    require_parameters(1U << 25U, set_render_scale_, 16);
    require_parameters(1U << 26U, set_render_pivot_, 16);
    if (!set_text_value_property_
        || set_text_value_property_->GetOffset_Internal() != 0
        || set_text_value_property_->GetSize()
            != static_cast<std::int32_t>(sizeof(FText))) {
        abi_failure_mask_ |= 1U << 24U;
    }
    if (!user_widget_class_ || !widget_tree_class_
        || !canvas_panel_class_ || !border_class_
        || !text_block_class_ || !check_box_class_
        || !widget_blueprint_library_.Get()
        || !widget_layout_library_.Get()) {
        abi_failure_mask_ |= 1U << 23U;
    }

    reset_runtime_handles();
    last_failure_ = 0;
    state_ = abi_failure_mask_ == 0
        ? RadarVisibilityHubState::Ready
        : RadarVisibilityHubState::Disabled;
}

RadarVisibilityHubResult RadarVisibilityHub::toggle(
    UObject* current_controller,
    RadarVisibilityMaskWord current_masks,
    AreaQuestDisplayMode current_area_quest_mode,
    AssaultDisplayMode current_assault_mode) noexcept {
    if (state_ == RadarVisibilityHubState::Faulted
        && current_controller && abi_failure_mask_ == 0) {
        const std::uint64_t faults_before_detach = fault_count_;
        detach_guarded(current_controller);
        // F6 is the only recovery trigger. The previous guarded failure has
        // already discarded every weak runtime handle; allow one fresh open
        // only when this cleanup added no fault and the immutable ABI is valid.
        if (state_ == RadarVisibilityHubState::Faulted
            && fault_count_ == faults_before_detach
            && abi_failure_mask_ == 0) {
            state_ = RadarVisibilityHubState::Ready;
            last_failure_ = 0;
        }
    }
    if (state_ == RadarVisibilityHubState::Open) {
        const RadarVisibilityMaskWord current = source_masks_;
        detach_guarded(current_controller);
        if (state_ == RadarVisibilityHubState::Faulted) {
            return {
                RadarVisibilityHubAction::Rejected,
                current,
                false,
                last_failure_};
        }
        ++close_count_;
        return {
            RadarVisibilityHubAction::Closed,
            current,
            false,
            0};
    }
    if (state_ != RadarVisibilityHubState::Ready
        || !current_controller) {
        const std::uint32_t failure = state_ == RadarVisibilityHubState::Ready
            ? 1U : (abi_failure_mask_ ? abi_failure_mask_ : 2U);
        last_failure_ = failure;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitize_masks(current_masks),
            false,
            failure};
    }
    return open_guarded(
        current_controller, current_masks, current_area_quest_mode,
        current_assault_mode);
}

RadarVisibilityHubResult RadarVisibilityHub::open_guarded(
    UObject* current_controller,
    RadarVisibilityMaskWord current_masks,
    AreaQuestDisplayMode current_area_quest_mode,
    AssaultDisplayMode current_assault_mode) noexcept {
#if defined(_MSC_VER)
    __try {
        RadarVisibilityHubResult result =
            open_unsafe(
                current_controller, current_masks,
                current_area_quest_mode, current_assault_mode);
        if (result.action == RadarVisibilityHubAction::Rejected
            && host_.Get()) {
            detach_guarded(current_controller);
        }
        return result;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 100;
        state_ = RadarVisibilityHubState::Faulted;
        detach_guarded(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            sanitize_masks(current_masks),
            false,
            last_failure_};
    }
#else
    try {
        RadarVisibilityHubResult result =
            open_unsafe(
                current_controller, current_masks,
                current_area_quest_mode, current_assault_mode);
        if (result.action == RadarVisibilityHubAction::Rejected
            && host_.Get()) {
            detach_guarded(current_controller);
        }
        return result;
    } catch (...) {
        ++fault_count_;
        last_failure_ = 100;
        state_ = RadarVisibilityHubState::Faulted;
        detach_guarded(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            sanitize_masks(current_masks),
            false,
            last_failure_};
    }
#endif
}

RadarVisibilityHubResult RadarVisibilityHub::open_unsafe(
    UObject* current_controller,
    RadarVisibilityMaskWord current_masks,
    AreaQuestDisplayMode current_area_quest_mode,
    AssaultDisplayMode current_assault_mode) {
    const RadarVisibilityMaskWord sanitized = sanitize_masks(current_masks);
    source_masks_ = sanitized;
    pending_masks_ = sanitized;
    source_area_quest_mode_ = current_area_quest_mode;
    pending_area_quest_mode_ = current_area_quest_mode;
    source_assault_mode_ = current_assault_mode;
    pending_assault_mode_ = current_assault_mode;

    UObject* blueprint_library = widget_blueprint_library_.Get();
    UObject* layout_library = widget_layout_library_.Get();
    if (!blueprint_library || !layout_library) {
        last_failure_ = 3;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }

    ViewportSizeParameters viewport_size{current_controller};
    layout_library->ProcessEvent(get_viewport_size_, &viewport_size);
    ViewportScaleParameters viewport_scale{current_controller};
    layout_library->ProcessEvent(get_viewport_scale_, &viewport_scale);
    if (!std::isfinite(viewport_size.return_value.x)
        || !std::isfinite(viewport_size.return_value.y)
        || viewport_size.return_value.x < 640.0
        || viewport_size.return_value.y < 360.0
        || !std::isfinite(viewport_scale.return_value)
        || viewport_scale.return_value < 0.1F
        || viewport_scale.return_value > 10.0F) {
        last_failure_ = 4;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    const double display_scale = std::clamp(
        std::min(
            viewport_size.return_value.x / 2560.0,
            viewport_size.return_value.y / 1440.0),
        0.50,
        2.50);
    const double unit_scale =
        display_scale / static_cast<double>(viewport_scale.return_value);

    CreateWidgetParameters create{
        current_controller,
        user_widget_class_,
        current_controller,
        nullptr};
    blueprint_library->ProcessEvent(create_widget_, &create);
    UObject* host = create.return_value;
    if (!host) {
        last_failure_ = 5;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    ObjectReturnParameters owning_player{};
    host->ProcessEvent(get_owning_player_, &owning_player);
    if (owning_player.return_value != current_controller) {
        last_failure_ = 6;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }

    UObject* tree =
        UObjectGlobals::NewObject<UObject>(host, widget_tree_class_);
    UObject* root = tree
        ? UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_)
        : nullptr;
    if (!tree || !root
        || !write_object_property(host, L"WidgetTree", tree)
        || !write_object_property(tree, L"RootWidget", root)) {
        last_failure_ = 7;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }

    const auto add_widget = [this, root, unit_scale](
        UObject* widget, double x, double y, double width, double height,
        std::int32_t z_order) -> UObject* {
        if (!widget) {
            return nullptr;
        }
        AddChildToCanvasParameters add{widget};
        root->ProcessEvent(add_child_to_canvas_, &add);
        UObject* slot = add.return_value;
        if (!slot) {
            return nullptr;
        }
        set_slot_vector(
            slot, set_slot_position_, x * unit_scale, y * unit_scale);
        set_slot_vector(
            slot, set_slot_size_, width * unit_scale, height * unit_scale);
        set_slot_vector(slot, set_slot_alignment_, 0.0, 0.0);
        set_slot_z_order(slot, set_slot_z_order_, z_order);
        return slot;
    };

    const auto add_border = [this, tree, &add_widget](
        double x, double y, double width, double height,
        std::int32_t z_order, const LinearColor& color) -> UObject* {
        UObject* border =
            UObjectGlobals::NewObject<UObject>(tree, border_class_);
        if (!border) {
            return nullptr;
        }
        set_brush_color(border, set_brush_color_, color);
        set_visibility(border, set_visibility_, kHitTestInvisible);
        return add_widget(border, x, y, width, height, z_order)
            ? border : nullptr;
    };

    const auto add_text = [this, tree, &add_widget, unit_scale](
        const wchar_t* value, double x, double y,
        double width, double height, std::int32_t z_order,
        double text_scale) -> UObject* {
        UObject* text_block =
            UObjectGlobals::NewObject<UObject>(tree, text_block_class_);
        if (!text_block) {
            return nullptr;
        }
        set_text(text_block, set_text_, set_text_value_property_, value);
        VectorParameters pivot{{0.0, 0.0}};
        text_block->ProcessEvent(set_render_pivot_, &pivot);
        const double scaled_text = unit_scale * text_scale;
        VectorParameters scale{{scaled_text, scaled_text}};
        text_block->ProcessEvent(set_render_scale_, &scale);
        set_visibility(text_block, set_visibility_, kHitTestInvisible);
        return add_widget(text_block, x, y, width, height, z_order)
            ? text_block : nullptr;
    };

    if (!add_border(
            0.0, 0.0, kReferencePanelWidth, kReferencePanelHeight,
            0, kPanelFrame)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0,
            kReferencePanelHeight - 4.0, 1, kPanelBackground)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0, 62.0, 2,
            kHeaderBackground)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0, 3.0, 3,
            kPanelAccent)
        || !add_border(
            2.0, 2.0, 4.0, 62.0, 3, kPanelAccent)
        || !add_border(
            16.0, 68.0, 588.0, 442.0, 2, kContentBackground)
        || !add_border(
            346.0, 72.0, 112.0, 27.0, 3, kCompactHeader)
        || !add_border(
            474.0, 72.0, 128.0, 27.0, 3, kWorldHeader)
        || !add_text(
            L"RADAR SETTINGS", 28.0, 17.0, 310.0, 34.0, 5, 0.78)
        || !add_text(L"RADAR", 356.0, 75.0, 100.0, 24.0, 5, 0.66)
        || !add_text(L"MAP", 484.0, 75.0, 116.0, 24.0, 5, 0.66)) {
        last_failure_ = 8;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }

    std::array<std::array<UObject*, kCategoryCount>, kColumnCount>
        controls{};
    std::array<std::array<UObject*, kCategoryCount>, kColumnCount>
        enabled_visuals{};
    std::array<UObject*, 2> area_mode_controls{};
    std::array<UObject*, 2> area_mode_visuals{};
    std::array<UObject*, 2> assault_mode_controls{};
    std::array<UObject*, 2> assault_mode_visuals{};
    const std::array<double, kColumnCount> control_x{{389.0, 526.0}};
    for (std::size_t row = 0; row < kRows.size(); ++row) {
        const RowDefinition& definition = kRows[row];
        const std::size_t category_index =
            static_cast<std::size_t>(definition.category);
        const double y = 108.0 + static_cast<double>(row) * 44.0;
        const bool ends_radar_section =
            definition.category == RadarVisibilityCategory::BirdEggs;
        if (!add_border(
                20.0, y - 4.0, 580.0, 36.0, 3,
                row % 2U == 0U ? kBaseRow : kAlternateRow)
            || !add_text(
                definition.label, 32.0, y - 3.0, 245.0, 34.0, 5, 0.72)
             || !add_border(
                ends_radar_section ? 20.0 : 28.0,
                y + 32.0,
                ends_radar_section ? 580.0 : 564.0,
                ends_radar_section ? 3.0 : 1.0,
                4,
                ends_radar_section ? kPanelAccent : kRowDivider)) {
            last_failure_ = 9;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_};
        }
        for (std::size_t column = 0; column < kColumnCount; ++column) {
            if (column == 1
                && (definition.category == RadarVisibilityCategory::Clock
                    || definition.category
                        == RadarVisibilityCategory::BirdEggs)) {
                if (!add_text(
                        L"-", control_x[column] + 7.0, y - 2.0,
                        24.0, 32.0, 5, 0.72)) {
                    last_failure_ = 10;
                    return {
                        RadarVisibilityHubAction::Rejected,
                        sanitized,
                        false,
                        last_failure_};
                }
                continue;
            }
            const std::uint8_t mask = column == 0
                ? compact_radar_visibility_mask(sanitized)
                : world_radar_visibility_mask(sanitized);
            const bool enabled =
                (mask & radar_visibility_bit(definition.category)) != 0;
            UObject* outer = add_border(
                control_x[column], y, 26.0, 26.0, 10, kToggleFrame);
            UObject* inner = add_border(
                control_x[column] + 5.0, y + 5.0, 16.0, 16.0, 11,
                column == 0 ? kCompactEnabled : kWorldEnabled);
            UObject* check_box =
                UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
            if (!outer || !inner || !check_box) {
                last_failure_ = 11;
                return {
                    RadarVisibilityHubAction::Rejected,
                    sanitized,
                    false,
                    last_failure_};
            }
            set_visibility(
                inner, set_visibility_, enabled ? kVisible : kCollapsed);
            set_checked(check_box, set_is_checked_, enabled);
            set_render_opacity(check_box, set_render_opacity_, 0.01F);
            if (!add_widget(
                    check_box, control_x[column] - 5.0, y - 5.0,
                    36.0, 36.0, 12)) {
                last_failure_ = 12;
                return {
                    RadarVisibilityHubAction::Rejected,
                    sanitized,
                    false,
                    last_failure_};
            }
            controls[column][category_index] = check_box;
            enabled_visuals[column][category_index] = inner;
        }
    }

    constexpr double mode_y = 416.0;
    if (!add_border(
            20.0, mode_y - 4.0, 580.0, 36.0, 3, kAlternateRow)
        || !add_text(
            L"AREA QUEST MODE", 32.0, mode_y - 3.0,
            245.0, 34.0, 5, 0.66)
        || !add_text(
            L"AVAILABLE", 294.0, mode_y - 2.0,
            92.0, 32.0, 5, 0.48)
        || !add_text(
            L"ALL", 486.0, mode_y - 2.0,
            38.0, 32.0, 5, 0.58)
        || !add_border(
            28.0, mode_y + 32.0, 564.0, 1.0, 4, kRowDivider)) {
        last_failure_ = 16;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_,
            current_area_quest_mode};
    }
    const std::array<AreaQuestDisplayMode, 2> mode_values{{
        AreaQuestDisplayMode::Available,
        AreaQuestDisplayMode::AllUnfinished,
    }};
    for (std::size_t mode_index = 0; mode_index < mode_values.size();
         ++mode_index) {
        const bool selected = current_area_quest_mode
            == mode_values[mode_index];
        const double x = control_x[mode_index];
        UObject* outer = add_border(
            x, mode_y, 26.0, 26.0, 10, kToggleFrame);
        UObject* inner = add_border(
            x + 5.0, mode_y + 5.0, 16.0, 16.0, 11,
            mode_index == 0 ? kCompactEnabled : kWorldEnabled);
        UObject* check_box =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!outer || !inner || !check_box) {
            last_failure_ = 17;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_,
                current_area_quest_mode};
        }
        set_visibility(
            inner, set_visibility_, selected ? kVisible : kCollapsed);
        set_checked(check_box, set_is_checked_, selected);
        set_render_opacity(check_box, set_render_opacity_, 0.01F);
        if (!add_widget(
                check_box, x - 5.0, mode_y - 5.0,
                36.0, 36.0, 12)) {
            last_failure_ = 18;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_,
                current_area_quest_mode};
        }
        area_mode_controls[mode_index] = check_box;
        area_mode_visuals[mode_index] = inner;
    }

    constexpr double assault_mode_y = 460.0;
    if (!add_border(
            20.0, assault_mode_y - 4.0, 580.0, 36.0, 3, kBaseRow)
        || !add_text(
            L"ASSAULT MODE", 32.0, assault_mode_y - 3.0,
            245.0, 34.0, 5, 0.66)
        || !add_text(
            L"AVAILABLE", 294.0, assault_mode_y - 2.0,
            92.0, 32.0, 5, 0.48)
        || !add_text(
            L"ALL", 486.0, assault_mode_y - 2.0,
            38.0, 32.0, 5, 0.58)
        || !add_border(
            28.0, assault_mode_y + 32.0, 564.0, 1.0, 4,
            kRowDivider)) {
        last_failure_ = 27;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_,
            current_area_quest_mode,
            current_assault_mode};
    }
    const std::array<AssaultDisplayMode, 2> assault_mode_values{{
        AssaultDisplayMode::Current,
        AssaultDisplayMode::All,
    }};
    for (std::size_t mode_index = 0;
         mode_index < assault_mode_values.size(); ++mode_index) {
        const bool selected = current_assault_mode
            == assault_mode_values[mode_index];
        const double x = control_x[mode_index];
        UObject* outer = add_border(
            x, assault_mode_y, 26.0, 26.0, 10, kToggleFrame);
        UObject* inner = add_border(
            x + 5.0, assault_mode_y + 5.0, 16.0, 16.0, 11,
            mode_index == 0 ? kCompactEnabled : kWorldEnabled);
        UObject* check_box =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!outer || !inner || !check_box) {
            last_failure_ = 28;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_,
                current_area_quest_mode,
                current_assault_mode};
        }
        set_visibility(
            inner, set_visibility_, selected ? kVisible : kCollapsed);
        set_checked(check_box, set_is_checked_, selected);
        set_render_opacity(check_box, set_render_opacity_, 0.01F);
        if (!add_widget(
                check_box, x - 5.0, assault_mode_y - 5.0,
                36.0, 36.0, 12)) {
            last_failure_ = 29;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_,
                current_area_quest_mode,
                current_assault_mode};
        }
        assault_mode_controls[mode_index] = check_box;
        assault_mode_visuals[mode_index] = inner;
    }

    if (!add_border(568.0, 14.0, 32.0, 32.0, 20, kCloseButton)
        || !add_text(L"X", 577.0, 15.0, 22.0, 28.0, 21, 0.78)) {
        last_failure_ = 13;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    UObject* close_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    if (!close_control) {
        last_failure_ = 13;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    set_checked(close_control, set_is_checked_, false);
    set_render_opacity(close_control, set_render_opacity_, 0.01F);
    if (!add_widget(close_control, 558.0, 8.0, 50.0, 50.0, 22)) {
        last_failure_ = 13;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }

    // Publish weak handles before the first viewport/input mutation so every
    // guarded failure can release the exact transient tree without keeping a
    // controller or raw widget across frames.
    host_ = host;
    widget_tree_ = tree;
    root_panel_ = root;
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        for (std::size_t category = 0; category < kCategoryCount; ++category) {
            controls_[column][category] = controls[column][category];
            enabled_visuals_[column][category] =
                enabled_visuals[column][category];
        }
    }
    area_mode_available_control_ = area_mode_controls[0];
    area_mode_all_control_ = area_mode_controls[1];
    area_mode_available_visual_ = area_mode_visuals[0];
    area_mode_all_visual_ = area_mode_visuals[1];
    assault_mode_current_control_ = assault_mode_controls[0];
    assault_mode_all_control_ = assault_mode_controls[1];
    assault_mode_current_visual_ = assault_mode_visuals[0];
    assault_mode_all_visual_ = assault_mode_visuals[1];
    close_control_ = close_control;

    set_visibility(host, set_visibility_, kVisible);
    AddToViewportParameters add_to_viewport{kViewportZOrder};
    host->ProcessEvent(add_to_viewport_, &add_to_viewport);
    VectorParameters viewport_alignment{{0.0, 0.0}};
    host->ProcessEvent(set_alignment_in_viewport_, &viewport_alignment);
    VectorParameters desired_size{{
        kReferencePanelWidth * unit_scale,
        kReferencePanelHeight * unit_scale}};
    host->ProcessEvent(set_desired_size_in_viewport_, &desired_size);
    const double physical_width =
        kReferencePanelWidth * display_scale;
    const double physical_height =
        kReferencePanelHeight * display_scale;
    PositionInViewportParameters viewport_position{{
        (viewport_size.return_value.x - physical_width) * 0.5,
        (viewport_size.return_value.y - physical_height) * 0.5}, true};
    host->ProcessEvent(set_position_in_viewport_, &viewport_position);
    host->ProcessEvent(force_layout_prepass_, nullptr);

    if (!read_cursor_visible(
            current_controller, previous_cursor_visible_)) {
        last_failure_ = 14;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    owns_input_mode_ = !previous_cursor_visible_;
    if (owns_input_mode_) {
        InputModeGameAndUiParameters input{
            current_controller,
            host,
            0,
            false,
            false};
        blueprint_library->ProcessEvent(
            set_input_mode_game_and_ui_, &input);
        // The input-mode helper can update cursor policy. Write the explicit
        // cursor state after it so the opening transaction cannot immediately
        // hide the pointer again.
        if (!write_cursor_visible(current_controller, true)) {
            last_failure_ = 15;
            return {
                RadarVisibilityHubAction::Rejected,
                sanitized,
                false,
                last_failure_};
        }
    }

    state_ = RadarVisibilityHubState::Open;
    last_failure_ = 0;
    ++open_count_;
    return {
        RadarVisibilityHubAction::Opened,
        sanitized,
        false,
        0,
        current_area_quest_mode,
        current_assault_mode};
}

RadarVisibilityHubResult RadarVisibilityHub::service_open_panel(
    UObject* current_controller) noexcept {
    if (state_ != RadarVisibilityHubState::Open) {
        return {
            RadarVisibilityHubAction::None,
            source_masks_,
            false,
            0};
    }
    if (!current_controller) {
        return {
            RadarVisibilityHubAction::None,
            pending_masks_,
            false,
            0};
    }
    return service_guarded(current_controller);
}

RadarVisibilityHubResult RadarVisibilityHub::service_guarded(
    UObject* current_controller) noexcept {
#if defined(_MSC_VER)
    __try {
        return service_unsafe(current_controller);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 101;
        state_ = RadarVisibilityHubState::Faulted;
        detach_guarded(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
#else
    try {
        return service_unsafe(current_controller);
    } catch (...) {
        ++fault_count_;
        last_failure_ = 101;
        state_ = RadarVisibilityHubState::Faulted;
        detach_guarded(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
#endif
}

RadarVisibilityHubResult RadarVisibilityHub::service_unsafe(
    UObject* current_controller) {
    UObject* host = host_.Get();
    if (!host) {
        last_failure_ = 20;
        release_for_travel();
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    ObjectReturnParameters owning_player{};
    host->ProcessEvent(get_owning_player_, &owning_player);
    if (owning_player.return_value != current_controller) {
        last_failure_ = 21;
        // The host still owns a valid previous controller. Detach through that
        // owner so the cursor and GameOnly input mode are restored before the
        // weak handles are released; never carry either UObject across frames.
        detach_unsafe(nullptr);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }

    if (owns_input_mode_) {
        bool cursor_visible{};
        if (!read_cursor_visible(current_controller, cursor_visible)) {
            last_failure_ = 24;
            detach_unsafe(current_controller);
            return {
                RadarVisibilityHubAction::Rejected,
                source_masks_,
                false,
                last_failure_};
        }
        if (!cursor_visible) {
            // Some gameplay states restore their cursor policy on the frame
            // after F6. Reassert only while this panel is open and only after
            // observing that overwrite; the normal 50 ms service otherwise
            // remains a read-only fast path.
            if (UObject* blueprint_library = widget_blueprint_library_.Get()) {
                InputModeGameAndUiParameters input{
                    current_controller,
                    host,
                    0,
                    false,
                    false};
                blueprint_library->ProcessEvent(
                    set_input_mode_game_and_ui_, &input);
            }
            if (!write_cursor_visible(current_controller, true)) {
                last_failure_ = 25;
                detach_unsafe(current_controller);
                return {
                    RadarVisibilityHubAction::Rejected,
                    source_masks_,
                    false,
                    last_failure_};
            }
        }
    }

    UObject* close_control = close_control_.Get();
    if (!close_control) {
        last_failure_ = 23;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    if (is_checked(close_control, is_checked_)) {
        const RadarVisibilityMaskWord current = source_masks_;
        detach_unsafe(current_controller);
        ++close_count_;
        return {
            RadarVisibilityHubAction::Closed,
            current,
            false,
            0};
    }

    std::uint8_t compact = compact_radar_visibility_mask(pending_masks_);
    std::uint8_t world = world_radar_visibility_mask(pending_masks_);
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        for (std::size_t category = 0; category < kCategoryCount; ++category) {
            if (column == 1
                && (category == static_cast<std::size_t>(
                        RadarVisibilityCategory::Clock)
                    || category == static_cast<std::size_t>(
                        RadarVisibilityCategory::BirdEggs))) {
                continue;
            }
            UObject* control = controls_[column][category].Get();
            UObject* visual = enabled_visuals_[column][category].Get();
            if (!control || !visual) {
                last_failure_ = 22;
                detach_unsafe(current_controller);
                return {
                    RadarVisibilityHubAction::Rejected,
                    source_masks_,
                    false,
                    last_failure_};
            }
            const bool enabled = is_checked(control, is_checked_);
            const std::uint8_t bit = static_cast<std::uint8_t>(1U << category);
            std::uint8_t& mask = column == 0 ? compact : world;
            const bool was_enabled = (mask & bit) != 0;
            if (enabled) {
                mask = static_cast<std::uint8_t>(mask | bit);
            } else {
                mask = static_cast<std::uint8_t>(mask & ~bit);
            }
            if (enabled != was_enabled) {
                set_visibility(
                    visual, set_visibility_,
                    enabled ? kVisible : kCollapsed);
            }
        }
    }
    pending_masks_ = pack_radar_visibility_masks(compact, world);

    UObject* available_control = area_mode_available_control_.Get();
    UObject* all_control = area_mode_all_control_.Get();
    UObject* available_visual = area_mode_available_visual_.Get();
    UObject* all_visual = area_mode_all_visual_.Get();
    if (!available_control || !all_control
        || !available_visual || !all_visual) {
        last_failure_ = 26;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_,
            source_area_quest_mode_,
            source_assault_mode_};
    }
    const bool available_checked =
        is_checked(available_control, is_checked_);
    const bool all_checked = is_checked(all_control, is_checked_);
    const bool was_available = pending_area_quest_mode_
        == AreaQuestDisplayMode::Available;
    const bool was_all = !was_available;
    AreaQuestDisplayMode selected_mode = pending_area_quest_mode_;
    if (available_checked != was_available) {
        selected_mode = available_checked
            ? AreaQuestDisplayMode::Available
            : AreaQuestDisplayMode::AllUnfinished;
    } else if (all_checked != was_all) {
        selected_mode = all_checked
            ? AreaQuestDisplayMode::AllUnfinished
            : AreaQuestDisplayMode::Available;
    }
    pending_area_quest_mode_ = selected_mode;
    const bool selected_available = selected_mode
        == AreaQuestDisplayMode::Available;
    if (available_checked != selected_available) {
        set_checked(
            available_control, set_is_checked_, selected_available);
    }
    if (all_checked == selected_available) {
        set_checked(all_control, set_is_checked_, !selected_available);
    }
    set_visibility(
        available_visual, set_visibility_,
        selected_available ? kVisible : kCollapsed);
    set_visibility(
        all_visual, set_visibility_,
        selected_available ? kCollapsed : kVisible);

    UObject* assault_current_control =
        assault_mode_current_control_.Get();
    UObject* assault_all_control = assault_mode_all_control_.Get();
    UObject* assault_current_visual =
        assault_mode_current_visual_.Get();
    UObject* assault_all_visual = assault_mode_all_visual_.Get();
    if (!assault_current_control || !assault_all_control
        || !assault_current_visual || !assault_all_visual) {
        last_failure_ = 30;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_,
            source_area_quest_mode_,
            source_assault_mode_};
    }
    const bool assault_current_checked =
        is_checked(assault_current_control, is_checked_);
    const bool assault_all_checked =
        is_checked(assault_all_control, is_checked_);
    const bool was_assault_current = pending_assault_mode_
        == AssaultDisplayMode::Current;
    const bool was_assault_all = !was_assault_current;
    AssaultDisplayMode selected_assault_mode = pending_assault_mode_;
    if (assault_current_checked != was_assault_current) {
        selected_assault_mode = assault_current_checked
            ? AssaultDisplayMode::Current
            : AssaultDisplayMode::All;
    } else if (assault_all_checked != was_assault_all) {
        selected_assault_mode = assault_all_checked
            ? AssaultDisplayMode::All
            : AssaultDisplayMode::Current;
    }
    pending_assault_mode_ = selected_assault_mode;
    const bool selected_assault_current = selected_assault_mode
        == AssaultDisplayMode::Current;
    if (assault_current_checked != selected_assault_current) {
        set_checked(
            assault_current_control, set_is_checked_,
            selected_assault_current);
    }
    if (assault_all_checked == selected_assault_current) {
        set_checked(
            assault_all_control, set_is_checked_,
            !selected_assault_current);
    }
    set_visibility(
        assault_current_visual, set_visibility_,
        selected_assault_current ? kVisible : kCollapsed);
    set_visibility(
        assault_all_visual, set_visibility_,
        selected_assault_current ? kCollapsed : kVisible);

    if (pending_masks_ == source_masks_
        && pending_area_quest_mode_ == source_area_quest_mode_
        && pending_assault_mode_ == source_assault_mode_) {
        return {
            RadarVisibilityHubAction::None,
            pending_masks_,
            false,
            0,
            pending_area_quest_mode_,
            pending_assault_mode_};
    }
    const RadarVisibilityMaskWord applied = pending_masks_;
    const AreaQuestDisplayMode applied_mode = pending_area_quest_mode_;
    const AssaultDisplayMode applied_assault_mode = pending_assault_mode_;
    source_masks_ = applied;
    source_area_quest_mode_ = applied_mode;
    source_assault_mode_ = applied_assault_mode;
    ++apply_count_;
    return {
        RadarVisibilityHubAction::Applied,
        applied,
        true,
        0,
        applied_mode,
        applied_assault_mode};
}

void RadarVisibilityHub::detach(UObject* current_controller) noexcept {
    detach_guarded(current_controller);
}

void RadarVisibilityHub::detach_guarded(
    UObject* current_controller) noexcept {
#if defined(_MSC_VER)
    __try {
        detach_unsafe(current_controller);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_failure_ = 102;
        state_ = RadarVisibilityHubState::Faulted;
        reset_runtime_handles();
    }
#else
    try {
        detach_unsafe(current_controller);
    } catch (...) {
        ++fault_count_;
        last_failure_ = 102;
        state_ = RadarVisibilityHubState::Faulted;
        reset_runtime_handles();
    }
#endif
}

void RadarVisibilityHub::detach_unsafe(UObject* current_controller) {
    bool removed{};
    UObject* host = host_.Get();
    UObject* input_controller{};
    if (host && owns_input_mode_) {
        // Input mode and cursor ownership belong to the controller that
        // created this host. Prefer that owner even when the caller already
        // has a newer current controller after a silent world/controller
        // replacement. Only fall back when the weak host is no longer usable.
        ObjectReturnParameters owning_player{};
        host->ProcessEvent(get_owning_player_, &owning_player);
        input_controller = owning_player.return_value;
    }
    if (!input_controller) {
        input_controller = current_controller;
    }
    if (owns_input_mode_ && input_controller) {
        if (UObject* blueprint_library = widget_blueprint_library_.Get()) {
            InputModeGameOnlyParameters input{input_controller, false};
            blueprint_library->ProcessEvent(
                set_input_mode_game_only_, &input);
        }
        (void)write_cursor_visible(
            input_controller, previous_cursor_visible_);
    }
    if (host && remove_from_parent_) {
        host->ProcessEvent(remove_from_parent_, nullptr);
        removed = true;
    }
    if (removed) {
        ++detach_count_;
    }
    reset_runtime_handles();
    if (state_ == RadarVisibilityHubState::Open) {
        state_ = RadarVisibilityHubState::Ready;
    }
}

void RadarVisibilityHub::release_for_travel() noexcept {
    const bool was_open = state_ == RadarVisibilityHubState::Open;
    reset_runtime_handles();
    if (was_open) {
        ++detach_count_;
        state_ = RadarVisibilityHubState::Ready;
    }
}

void RadarVisibilityHub::reset_runtime_handles() noexcept {
    host_ = FWeakObjectPtr{};
    widget_tree_ = FWeakObjectPtr{};
    root_panel_ = FWeakObjectPtr{};
    for (auto& column : controls_) {
        for (auto& control : column) {
            control = FWeakObjectPtr{};
        }
    }
    for (auto& column : enabled_visuals_) {
        for (auto& visual : column) {
            visual = FWeakObjectPtr{};
        }
    }
    area_mode_available_control_ = FWeakObjectPtr{};
    area_mode_all_control_ = FWeakObjectPtr{};
    area_mode_available_visual_ = FWeakObjectPtr{};
    area_mode_all_visual_ = FWeakObjectPtr{};
    assault_mode_current_control_ = FWeakObjectPtr{};
    assault_mode_all_control_ = FWeakObjectPtr{};
    assault_mode_current_visual_ = FWeakObjectPtr{};
    assault_mode_all_visual_ = FWeakObjectPtr{};
    close_control_ = FWeakObjectPtr{};
    owns_input_mode_ = false;
    previous_cursor_visible_ = false;
}

} // namespace dsnwr
