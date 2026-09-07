#include "radar_visibility_hub.hpp"

#include <dswros/radar_localization.hpp>
#include <dswros/radar_visibility_hub_policy.hpp>

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
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>
#include <vector>

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

struct VectorReturnParameters {
    Vector2D return_value{};
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

struct ByteParameters {
    std::uint8_t value{};
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

struct SetBrushFromTextureParameters {
    UObject* texture{};
    bool match_size{};
    std::array<std::byte, 7> padding{};
};

struct ImportFileParameters {
    UObject* world_context_object{};
    FString filename{};
    UObject* return_value{};

    ImportFileParameters(UObject* context, const wchar_t* path)
        : world_context_object(context), filename(path) {}
};

static_assert(sizeof(Vector2D) == 16);
static_assert(sizeof(CreateWidgetParameters) == 32);
static_assert(sizeof(ObjectReturnParameters) == 8);
static_assert(sizeof(ViewportSizeParameters) == 24);
static_assert(sizeof(ViewportScaleParameters) == 16);
static_assert(sizeof(AddToViewportParameters) == 4);
static_assert(sizeof(AddChildToCanvasParameters) == 16);
static_assert(sizeof(VectorParameters) == 16);
static_assert(sizeof(VectorReturnParameters) == 16);
static_assert(sizeof(PositionInViewportParameters) == 24);
static_assert(sizeof(ZOrderParameters) == 4);
static_assert(sizeof(VisibilityParameters) == 1);
static_assert(sizeof(LinearColor) == 16);
static_assert(sizeof(BrushColorParameters) == 16);
static_assert(sizeof(FText) == 24);
static_assert(sizeof(TextParameters) == 24);
static_assert(sizeof(ScalarParameters) == 4);
static_assert(sizeof(BoolParameters) == 1);
static_assert(sizeof(ByteParameters) == 1);
static_assert(sizeof(InputModeGameAndUiParameters) == 24);
static_assert(sizeof(InputModeGameOnlyParameters) == 16);
static_assert(sizeof(SetBrushFromTextureParameters) == 16);
static_assert(sizeof(FString) == 16);
static_assert(sizeof(ImportFileParameters) == 32);

constexpr std::uint8_t kVisible = 0;
constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
constexpr std::uint8_t kTextLeft = 0;
constexpr std::uint8_t kTextCenter = 1;
constexpr std::int32_t kViewportZOrder = 2'000'000'100;
constexpr double kReferencePanelWidth = 680.0;
constexpr double kReferencePanelHeight = 660.0;
constexpr double kReferenceViewportWidth = 2560.0;
constexpr double kReferenceViewportHeight = 1440.0;
constexpr double kMinimumViewportMargin = 16.0;
constexpr double kTextLineHeightSafety = 1.45;
constexpr double kReferenceHubFontSize = 24.0;
constexpr double kMaximumHubFontSize = 512.0;
constexpr double kFontMetricReadbackTolerance = 0.001;
constexpr std::size_t kFontParameterCapacity = 512;
constexpr std::size_t kMaximumHubTextCount = 64;

[[nodiscard]] constexpr bool uses_packaged_text_overlay(
    dswros::RadarUiLanguage language) noexcept {
    return language == dswros::RadarUiLanguage::Korean
        || language == dswros::RadarUiLanguage::TraditionalChinese;
}

[[nodiscard]] constexpr const wchar_t* packaged_text_overlay_prefix(
    dswros::RadarUiLanguage language) noexcept {
    return language == dswros::RadarUiLanguage::Korean
        ? L"ko"
        : language == dswros::RadarUiLanguage::TraditionalChinese
            ? L"zh-hant"
            : nullptr;
}

[[nodiscard]] constexpr const wchar_t* packaged_text_overlay_status(
    RadarModStatus status) noexcept {
    return status == RadarModStatus::On
        ? L"on"
        : status == RadarModStatus::Fault ? L"fault" : L"off";
}

constexpr std::array<const wchar_t*, 4> kLanguageFontObjectNames{{
    L"DsCompositFont_CommonSystem",
    L"DsCompositFont_TCSystem",
    L"DsCompositFont_JPSystem",
    L"DsCompositFont_THSystem",
}};

struct HubTextFontRecord {
    UObject* widget{};
    UObject* slot{};
    double target_size{};
    bool allow_desired_size_centering{};
    double authored_x{};
    double authored_y{};
    double authored_width{};
    double authored_height{};
};

struct alignas(std::max_align_t) FontParameterBuffer {
    std::array<std::byte, kFontParameterCapacity> bytes{};
};

class FontCallParameters final {
public:
    explicit FontCallParameters(UFunction* function) noexcept
        : function_(function) {
        if (!function_ || function_->GetParmsSize() <= 0
            || static_cast<std::size_t>(function_->GetParmsSize())
                > buffer_.bytes.size()) {
            function_ = nullptr;
            return;
        }
        for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function_)) {
            if (property->HasAnyPropertyFlags(CPF_Parm)) {
                property->InitializeValue_InContainer(buffer_.bytes.data());
            }
        }
    }

    ~FontCallParameters() {
        if (!function_) {
            return;
        }
        for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function_)) {
            if (property->HasAnyPropertyFlags(CPF_Parm)) {
                property->DestroyValue_InContainer(buffer_.bytes.data());
            }
        }
    }

    FontCallParameters(const FontCallParameters&) = delete;
    FontCallParameters& operator=(const FontCallParameters&) = delete;

    [[nodiscard]] bool valid() const noexcept { return function_ != nullptr; }
    [[nodiscard]] void* data() noexcept { return buffer_.bytes.data(); }

private:
    UFunction* function_{};
    FontParameterBuffer buffer_{};
};

constexpr LinearColor kPanelFrame{
    45.0F / 255.0F, 76.0F / 255.0F, 90.0F / 255.0F, 0.99F};
constexpr LinearColor kPanelBackground{
    6.0F / 255.0F, 14.0F / 255.0F, 21.0F / 255.0F, 0.98F};
constexpr LinearColor kPanelAccent{
    90.0F / 255.0F, 207.0F / 255.0F, 222.0F / 255.0F, 1.0F};
constexpr LinearColor kHeaderBackground{
    11.0F / 255.0F, 27.0F / 255.0F, 38.0F / 255.0F, 0.99F};
constexpr LinearColor kContentBackground{
    9.0F / 255.0F, 20.0F / 255.0F, 29.0F / 255.0F, 0.93F};
constexpr LinearColor kSectionFrame{
    52.0F / 255.0F, 88.0F / 255.0F, 102.0F / 255.0F, 0.82F};
constexpr LinearColor kBaseRow{
    18.0F / 255.0F, 36.0F / 255.0F, 46.0F / 255.0F, 0.50F};
constexpr LinearColor kAlternateRow{
    24.0F / 255.0F, 46.0F / 255.0F, 57.0F / 255.0F, 0.58F};
constexpr LinearColor kRowDivider{
    48.0F / 255.0F, 77.0F / 255.0F, 89.0F / 255.0F, 0.62F};
constexpr LinearColor kCompactHeader{
    39.0F / 255.0F, 139.0F / 255.0F, 165.0F / 255.0F, 0.52F};
constexpr LinearColor kWorldHeader{
    58.0F / 255.0F, 153.0F / 255.0F, 111.0F / 255.0F, 0.52F};
constexpr LinearColor kToggleFrame{
    46.0F / 255.0F, 72.0F / 255.0F, 83.0F / 255.0F, 1.0F};
constexpr LinearColor kCompactEnabled{
    72.0F / 255.0F, 205.0F / 255.0F, 1.0F, 1.0F};
constexpr LinearColor kWorldEnabled{
    101.0F / 255.0F, 226.0F / 255.0F, 154.0F / 255.0F, 1.0F};
constexpr LinearColor kCloseButton{
    125.0F / 255.0F, 48.0F / 255.0F, 55.0F / 255.0F, 0.92F};
constexpr LinearColor kLanguageCard{
    18.0F / 255.0F, 43.0F / 255.0F, 56.0F / 255.0F, 0.98F};
constexpr LinearColor kLanguageSelector{
    26.0F / 255.0F, 60.0F / 255.0F, 75.0F / 255.0F, 1.0F};
constexpr LinearColor kPopupDim{
    2.0F / 255.0F, 7.0F / 255.0F, 10.0F / 255.0F, 0.82F};
constexpr LinearColor kPopupBackground{
    10.0F / 255.0F, 25.0F / 255.0F, 34.0F / 255.0F, 1.0F};
constexpr LinearColor kLanguageOption{
    24.0F / 255.0F, 48.0F / 255.0F, 59.0F / 255.0F, 1.0F};
constexpr LinearColor kLanguageSelected{
    40.0F / 255.0F, 130.0F / 255.0F, 151.0F / 255.0F, 0.94F};
constexpr LinearColor kModeOption{
    28.0F / 255.0F, 52.0F / 255.0F, 63.0F / 255.0F, 1.0F};
constexpr LinearColor kModeSelectedAvailable{
    42.0F / 255.0F, 140.0F / 255.0F, 165.0F / 255.0F, 0.82F};
constexpr LinearColor kModeSelectedAll{
    54.0F / 255.0F, 156.0F / 255.0F, 112.0F / 255.0F, 0.82F};
constexpr LinearColor kStatusCard{
    14.0F / 255.0F, 33.0F / 255.0F, 44.0F / 255.0F, 0.90F};
constexpr LinearColor kStatusOff{
    139.0F / 255.0F, 116.0F / 255.0F, 71.0F / 255.0F, 0.94F};
constexpr LinearColor kStatusOn{
    45.0F / 255.0F, 143.0F / 255.0F, 100.0F / 255.0F, 0.94F};
constexpr LinearColor kStatusFault{
    150.0F / 255.0F, 58.0F / 255.0F, 67.0F / 255.0F, 0.96F};
constexpr LinearColor kActionButton{
    38.0F / 255.0F, 111.0F / 255.0F, 139.0F / 255.0F, 0.96F};
constexpr LinearColor kBugReportButton{
    87.0F / 255.0F, 71.0F / 255.0F, 121.0F / 255.0F, 0.96F};

struct RowDefinition {
    RadarVisibilityCategory category{RadarVisibilityCategory::Clock};
};

constexpr std::array<RowDefinition, 7> kRows{{
    {RadarVisibilityCategory::Clock},
    {RadarVisibilityCategory::Treasure},
    {RadarVisibilityCategory::Boss},
    {RadarVisibilityCategory::Assault},
    {RadarVisibilityCategory::MiniGames},
    {RadarVisibilityCategory::AreaQuests},
    {RadarVisibilityCategory::BirdEggs},
}};


[[nodiscard]] dswros::HeightIndicatorMask sanitize_height_indicators(
    dswros::HeightIndicatorMask value) noexcept {
    return static_cast<dswros::HeightIndicatorMask>(
        value & dswros::kHeightIndicatorAll);
}

[[nodiscard]] dswros::RadarLanguagePreference sanitize_language(
    dswros::RadarLanguagePreference value) noexcept {
    return static_cast<std::size_t>(value)
            < dswros::kRadarLanguagePreferenceCount
        ? value
        : dswros::RadarLanguagePreference::Auto;
}

using LanguageDisplayBuffer = std::array<wchar_t, 96>;

[[nodiscard]] LanguageDisplayBuffer format_language_display(
    dswros::RadarUiLanguage resolved_language) noexcept {
    LanguageDisplayBuffer output{};
    const auto& text = dswros::radar_localized_text(resolved_language);
    std::size_t cursor{};
    const auto append = [&output, &cursor](const wchar_t* value) noexcept {
        if (!value) {
            return;
        }
        while (*value != L'\0' && cursor + 1U < output.size()) {
            output[cursor++] = *value++;
        }
        output[cursor] = L'\0';
    };
    append(text.language_name);
    return output;
}

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

template <typename Structure>
[[nodiscard]] FProperty* find_reflected_property(
    Structure* structure, const wchar_t* name) noexcept {
    if (!structure || !name) {
        return nullptr;
    }
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(structure)) {
        if (property->GetName() == name) {
            return property;
        }
    }
    return nullptr;
}

[[nodiscard]] FProperty* find_struct_field(
    UScriptStruct* structure, const wchar_t* name) noexcept {
    if (!structure || !name) {
        return nullptr;
    }
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(structure)) {
        if (property->GetName() == name) {
            return property;
        }
    }
    return nullptr;
}

[[nodiscard]] UObject* read_struct_object_property(
    UObject* object, const wchar_t* struct_name,
    const wchar_t* object_name) noexcept {
    auto* structure = CastField<FStructProperty>(
        object ? object->GetPropertyByNameInChain(struct_name) : nullptr);
    void* structure_value = structure
        ? structure->ContainerPtrToValuePtr<void>(object)
        : nullptr;
    auto* object_property = CastField<FObjectPropertyBase>(
        structure
            ? find_struct_field(structure->GetStruct(), object_name)
            : nullptr);
    void* object_value = object_property && structure_value
        ? object_property->ContainerPtrToValuePtr<void>(structure_value)
        : nullptr;
    return object_value
        ? object_property->GetObjectPropertyValue(object_value)
        : nullptr;
}

[[nodiscard]] FStructProperty* compatible_font_property(
    UObject* text_block,
    UClass* text_block_class,
    FStructProperty* expected_font_property) noexcept {
    if (!text_block || !text_block_class || !expected_font_property
        || !text_block->IsA(text_block_class)) {
        return nullptr;
    }
    // Initialization has already resolved and ABI-validated the inherited
    // TextBlock.Font property from the class chain.  Live diagnostics show
    // that the separate UObject instance lookup path can fail for both newly
    // constructed TextBlock and DTextBlock widgets in this runtime.  Reuse the
    // validated class property and keep the owner/layout checks fail-closed.
    FStructProperty* property = expected_font_property;
    UClass* property_owner = property->GetOwner<UClass>();
    UClass* actual_class = text_block->GetClassPrivate();
    if (!property_owner || !actual_class
        || !text_block_class->IsChildOf(property_owner)
        || !actual_class->IsChildOf(property_owner)
        || property->GetOffset_Internal() < 0
        || !property->IsInContainer(actual_class)
        || !property->GetStruct()
        || property->GetSize() <= 0
        || !property->ContainerPtrToValuePtr<void>(text_block)) {
        return nullptr;
    }
    return property;
}

[[nodiscard]] FNumericProperty* compatible_font_metric(
    FStructProperty* font_property, const wchar_t* metric_name) noexcept {
    UScriptStruct* font_struct = font_property
        ? font_property->GetStruct() : nullptr;
    auto* property = CastField<FNumericProperty>(
        font_struct
            ? find_reflected_property(font_struct, metric_name)
            : nullptr);
    return property
        && (property->IsInteger() || property->IsFloatingPoint())
        && property->GetSize() > 0
        && property->GetSize()
            <= static_cast<std::int32_t>(sizeof(std::int64_t))
        ? property
        : nullptr;
}

[[nodiscard]] FObjectPropertyBase* compatible_font_object_member(
    FStructProperty* font_property) noexcept {
    UScriptStruct* font_struct = font_property
        ? font_property->GetStruct() : nullptr;
    auto* property = CastField<FObjectPropertyBase>(
        font_struct
            ? find_reflected_property(font_struct, L"FontObject")
            : nullptr);
    return property && property->GetOffset_Internal() >= 0
        && property->GetSize() > 0
        && property->IsInContainer(font_struct)
        ? property
        : nullptr;
}

[[nodiscard]] bool read_font_metric(
    FNumericProperty* property, const void* address,
    double& value) noexcept {
    if (!property || !address) {
        return false;
    }
    if (property->IsFloatingPoint()) {
        value = property->GetFloatingPointPropertyValue(address);
    } else if (property->IsInteger()) {
        value = static_cast<double>(property->GetSignedIntPropertyValue(
            const_cast<void*>(address)));
    } else {
        return false;
    }
    return std::isfinite(value);
}

[[nodiscard]] bool font_metric_matches(
    double actual, double expected) noexcept {
    return std::isfinite(actual) && std::isfinite(expected)
        && std::abs(actual - expected)
            <= kFontMetricReadbackTolerance;
}

[[nodiscard]] bool write_font_metric(
    FNumericProperty* property, void* address, double value) noexcept {
    if (!property || !address || !std::isfinite(value)) {
        return false;
    }
    double expected = value;
    if (property->IsFloatingPoint()) {
        property->SetFloatingPointPropertyValue(address, value);
    } else if (property->IsInteger()) {
        const long double extended_value = static_cast<long double>(value);
        if (extended_value < static_cast<long double>(
                std::numeric_limits<std::int64_t>::min())
            || extended_value > static_cast<long double>(
                std::numeric_limits<std::int64_t>::max())) {
            return false;
        }
        const auto integer_value = static_cast<std::int64_t>(
            std::llround(value));
        if (!property->CanHoldSignedValueInternal(integer_value)) {
            return false;
        }
        property->SetIntPropertyValue(address, integer_value);
        expected = static_cast<double>(integer_value);
    } else {
        return false;
    }
    double readback{};
    return read_font_metric(property, address, readback)
        && font_metric_matches(readback, expected);
}

[[nodiscard]] bool font_object_is_compatible(
    UObject* object,
    UClass* text_block_class,
    FStructProperty* expected_font_property) noexcept {
    FStructProperty* font = compatible_font_property(
        object, text_block_class, expected_font_property);
    return font
        && compatible_font_metric(font, L"Size")
        && compatible_font_metric(font, L"LetterSpacing");
}

[[nodiscard]] bool write_bool_property(
    UObject* object, FProperty* reflected_property, bool value) noexcept {
    auto* property = CastField<FBoolProperty>(reflected_property);
    void* address = object && property
        ? property->ContainerPtrToValuePtr<void>(object)
        : nullptr;
    if (!address) {
        return false;
    }
    property->SetPropertyValue(address, value);
    return property->GetPropertyValue(address) == value;
}

[[nodiscard]] bool copy_font_preserving_layout_metrics(
    UObject* donor,
    UObject* destination,
    UClass* text_block_class,
    FStructProperty* expected_font_property) noexcept {
    FStructProperty* source_property = compatible_font_property(
        donor, text_block_class, expected_font_property);
    FStructProperty* destination_property = compatible_font_property(
        destination, text_block_class, expected_font_property);
    if (!source_property || !destination_property
        || source_property->GetStruct() != destination_property->GetStruct()
        || source_property->GetSize() != destination_property->GetSize()) {
        return false;
    }
    const void* source_value =
        source_property->ContainerPtrToValuePtr<void>(donor);
    void* destination_value =
        destination_property->ContainerPtrToValuePtr<void>(destination);
    auto* size_property = compatible_font_metric(
        destination_property, L"Size");
    auto* spacing_property = compatible_font_metric(
        destination_property, L"LetterSpacing");
    void* size_value = size_property && destination_value
        ? size_property->ContainerPtrToValuePtr<void>(destination_value)
        : nullptr;
    void* spacing_value = spacing_property && destination_value
        ? spacing_property->ContainerPtrToValuePtr<void>(destination_value)
        : nullptr;
    if (!source_value || !destination_value || !size_property
        || !spacing_property || !size_value || !spacing_value) {
        return false;
    }
    double original_size{};
    double original_spacing{};
    if (!read_font_metric(size_property, size_value, original_size)
        || !read_font_metric(
            spacing_property, spacing_value, original_spacing)) {
        return false;
    }
    destination_property->CopyCompleteValue(destination_value, source_value);
    return write_font_metric(size_property, size_value, original_size)
        && write_font_metric(
            spacing_property, spacing_value, original_spacing);
}

[[nodiscard]] bool read_font_size_raw(
    UObject* text_block,
    UClass* text_block_class,
    FStructProperty* expected_font_property,
    double& value) noexcept {
    FStructProperty* font_property = compatible_font_property(
        text_block, text_block_class, expected_font_property);
    void* font_value = font_property
        ? font_property->ContainerPtrToValuePtr<void>(text_block)
        : nullptr;
    auto* size_property = compatible_font_metric(font_property, L"Size");
    void* size_value = size_property && font_value
        ? size_property->ContainerPtrToValuePtr<void>(font_value)
        : nullptr;
    if (!size_property || !size_value) {
        return false;
    }
    return read_font_metric(size_property, size_value, value);
}

[[nodiscard]] bool read_font_size(
    UObject* text_block,
    UClass* text_block_class,
    FStructProperty* expected_font_property,
    double& value) noexcept {
    if (!read_font_size_raw(
            text_block, text_block_class, expected_font_property, value)) {
        return false;
    }
    return value > 0 && value <= kMaximumHubFontSize;
}

[[nodiscard]] bool calculate_target_font_size(
    UObject* text_block,
    UClass* text_block_class,
    FStructProperty* expected_font_property,
    double unit_scale,
    double role_scale,
    double reference_slot_height,
    double& target_size) noexcept {
    double source_size{};
    if (!read_font_size_raw(
            text_block, text_block_class, expected_font_property,
            source_size)
        || !std::isfinite(unit_scale) || unit_scale <= 0.0
        || !std::isfinite(role_scale) || role_scale <= 0.0
        || !std::isfinite(reference_slot_height)
        || reference_slot_height <= 0.0) {
        return false;
    }
    // Some game DTextBlock defaults defer their language font until the
    // widget reaches Slate and expose Font.Size == 0 during construction.
    // The structure is still writable, so seed only the bounded layout size;
    // the post-viewport prepass below will select the language font and then
    // recommit/read back this same target. Invalid nonzero defaults remain a
    // preparation failure so the caller can fall back or reject safely.
    if (source_size == 0) {
        source_size = kReferenceHubFontSize;
    } else if (source_size < 0 || source_size > kMaximumHubFontSize) {
        return false;
    }
    const double scaled_size =
        static_cast<double>(source_size) * unit_scale * role_scale;
    const double maximum_for_line_box =
        reference_slot_height * unit_scale / kTextLineHeightSafety;
    if (!std::isfinite(scaled_size) || scaled_size <= 0.0
        || !std::isfinite(maximum_for_line_box)
        || maximum_for_line_box <= 0.0) {
        return false;
    }
    const double rounded_size = std::round(scaled_size);
    const double line_box_limit = std::floor(maximum_for_line_box);
    target_size = std::clamp(
        std::min(rounded_size, std::max(1.0, line_box_limit)),
        1.0,
        kMaximumHubFontSize);
    return true;
}

[[nodiscard]] bool apply_font_size_and_commit(
    UObject* text_block,
    UClass* text_block_class,
    FStructProperty* expected_font_property,
    UFunction* set_font,
    FStructProperty* set_font_value_property,
    double target_size) noexcept {
    FStructProperty* font_property = compatible_font_property(
        text_block, text_block_class, expected_font_property);
    void* font_value = font_property
        ? font_property->ContainerPtrToValuePtr<void>(text_block)
        : nullptr;
    auto* size_property = compatible_font_metric(font_property, L"Size");
    if (!font_property || !font_value || !size_property
        || !set_font || !set_font_value_property
        || set_font_value_property->GetStruct() != font_property->GetStruct()
        || set_font_value_property->GetSize() != font_property->GetSize()
        || target_size <= 0 || target_size > kMaximumHubFontSize) {
        return false;
    }

    FontCallParameters parameters(set_font);
    void* parameter_font = parameters.valid()
        ? set_font_value_property->ContainerPtrToValuePtr<void>(
              parameters.data())
        : nullptr;
    void* parameter_size = size_property && parameter_font
        ? size_property->ContainerPtrToValuePtr<void>(parameter_font)
        : nullptr;
    if (!parameter_font || !parameter_size) {
        return false;
    }
    set_font_value_property->CopyCompleteValue(parameter_font, font_value);
    if (!write_font_metric(
            size_property, parameter_size, target_size)) {
        return false;
    }
    text_block->ProcessEvent(set_font, parameters.data());

    double committed_size{};
    return read_font_size(
               text_block, text_block_class, expected_font_property,
               committed_size)
        && font_metric_matches(committed_size, target_size);
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

[[nodiscard]] bool try_read_desired_size(
    UObject* widget, UFunction* function, Vector2D& desired_size) noexcept {
    if (!widget || !function) {
        return false;
    }
#if defined(_MSC_VER)
    __try {
        VectorReturnParameters parameters{};
        widget->ProcessEvent(function, &parameters);
        desired_size = parameters.return_value;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        VectorReturnParameters parameters{};
        widget->ProcessEvent(function, &parameters);
        desired_size = parameters.return_value;
    } catch (...) {
        return false;
    }
#endif
    return std::isfinite(desired_size.x)
        && std::isfinite(desired_size.y);
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

void RadarVisibilityHub::initialize(
    std::filesystem::path text_overlay_root) noexcept {
    text_overlay_root_ = std::move(text_overlay_root);
    user_widget_class_ = find<UClass>(L"/Script/UMG.UserWidget");
    widget_class_ = find<UClass>(L"/Script/UMG.Widget");
    widget_tree_class_ = find<UClass>(L"/Script/UMG.WidgetTree");
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    border_class_ = find<UClass>(L"/Script/UMG.Border");
    text_block_class_ = find<UClass>(L"/Script/UMG.TextBlock");
    game_text_block_class_ = find<UClass>(L"/Script/DSClient.DTextBlock");
    UObject* game_text_block_default = game_text_block_class_
        ? game_text_block_class_->GetClassDefaultObject().Get()
        : nullptr;
    if (!game_text_block_default) {
        game_text_block_default = find<UObject>(
            L"/Script/DSClient.Default__DTextBlock");
    }
    game_text_block_default_ = game_text_block_default;
    check_box_class_ = find<UClass>(L"/Script/UMG.CheckBox");
    image_class_ = find<UClass>(L"/Script/UMG.Image");
    text_block_font_property_ = text_block_class_
        ? find_reflected_property(text_block_class_, L"Font")
        : nullptr;
    force_apply_language_font_property_ = game_text_block_class_
        ? find_reflected_property(
              game_text_block_class_, L"ForceApplyLanguageFont")
        : nullptr;
    widget_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    widget_layout_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");
    kismet_rendering_library_ =
        find<UObject>(L"/Script/Engine.Default__KismetRenderingLibrary");

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
    // SetJustification is declared by UTextLayoutWidget, not UTextBlock.
    // Keep this visual-only capability optional so a future reflection rename
    // cannot disable the entire F6 settings hub.
    set_justification_ = find<UFunction>(
        L"/Script/UMG.TextLayoutWidget:SetJustification");
    if (set_justification_
        && set_justification_->GetParmsSize()
            != static_cast<std::int32_t>(sizeof(ByteParameters))) {
        set_justification_ = nullptr;
    }
    set_render_opacity_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderOpacity");
    // These two functions powered the original live-accepted F6 text path.
    // They are retained as an optional presentation fallback: reflected
    // Font.Size remains preferred, but a font-layout ABI mismatch must not
    // suppress the complete settings Hub.
    set_render_scale_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderScale");
    if (set_render_scale_
        && set_render_scale_->GetParmsSize()
            != static_cast<std::int32_t>(sizeof(VectorParameters))) {
        set_render_scale_ = nullptr;
    }
    set_render_pivot_ = find<UFunction>(
        L"/Script/UMG.Widget:SetRenderTransformPivot");
    if (set_render_pivot_
        && set_render_pivot_->GetParmsSize()
            != static_cast<std::int32_t>(sizeof(VectorParameters))) {
        set_render_pivot_ = nullptr;
    }
    set_font_ = find<UFunction>(L"/Script/UMG.TextBlock:SetFont");
    set_font_value_property_ = CastField<FStructProperty>(
        find_function_property(set_font_, L"InFontInfo"));
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
    get_desired_size_ =
        find<UFunction>(L"/Script/UMG.Widget:GetDesiredSize");
    if (get_desired_size_) {
        auto* return_property = CastField<FStructProperty>(
            find_function_property(get_desired_size_, L"ReturnValue"));
        auto* viewport_size_return_property = CastField<FStructProperty>(
            get_viewport_size_
                ? find_function_property(
                      get_viewport_size_, L"ReturnValue")
                : nullptr);
        if (get_desired_size_->GetParmsSize()
                != static_cast<std::int32_t>(
                    sizeof(VectorReturnParameters))
            || !return_property
            || !viewport_size_return_property
            || !return_property->HasAnyPropertyFlags(CPF_ReturnParm)
            || return_property->GetOffset_Internal() != 0
            || return_property->GetSize()
                != static_cast<std::int32_t>(sizeof(Vector2D))
            || !return_property->GetStruct()
            || return_property->GetStruct()
                != viewport_size_return_property->GetStruct()) {
            get_desired_size_ = nullptr;
        }
    }
    remove_from_parent_ =
        find<UFunction>(L"/Script/UMG.Widget:RemoveFromParent");
    set_input_mode_game_and_ui_ = find<UFunction>(
        L"/Script/UMG.WidgetBlueprintLibrary:SetInputMode_GameAndUIEx");
    set_input_mode_game_only_ = find<UFunction>(
        L"/Script/UMG.WidgetBlueprintLibrary:SetInputMode_GameOnly");
    set_brush_from_texture_ =
        find<UFunction>(L"/Script/UMG.Image:SetBrushFromTexture");
    import_file_as_texture_ = find<UFunction>(
        L"/Script/Engine.KismetRenderingLibrary:ImportFileAsTexture2D");

    // Packaged Korean/Traditional-Chinese text overlays are an isolated
    // presentation fallback. Keep F6 functional if this optional path is
    // unavailable, then expose an exact runtime failure code instead of
    // hiding the complete settings panel.
    if (set_brush_from_texture_
        && set_brush_from_texture_->GetParmsSize() != 9) {
        set_brush_from_texture_ = nullptr;
    }
    if (import_file_as_texture_
        && import_file_as_texture_->GetParmsSize() != 32) {
        import_file_as_texture_ = nullptr;
    }

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
    if (!set_text_value_property_
        || set_text_value_property_->GetOffset_Internal() != 0
        || set_text_value_property_->GetSize()
            != static_cast<std::int32_t>(sizeof(FText))) {
        abi_failure_mask_ |= 1U << 24U;
    }
    auto* expected_font_property = CastField<FStructProperty>(
        text_block_font_property_);
    const std::int32_t font_parameter_offset = set_font_value_property_
        ? set_font_value_property_->GetOffset_Internal()
        : -1;
    const std::int32_t font_parameter_size = set_font_value_property_
        ? set_font_value_property_->GetSize()
        : 0;
    const std::int32_t font_parameter_bytes = set_font_
        ? set_font_->GetParmsSize()
        : 0;
    const bool reflected_font_contract_invalid =
        !set_font_ || !set_font_value_property_ || !expected_font_property
        || set_font_value_property_->GetStruct()
            != expected_font_property->GetStruct()
        || font_parameter_offset < 0 || font_parameter_size <= 0
        || font_parameter_bytes <= 0
        || font_parameter_bytes
            > static_cast<std::int32_t>(kFontParameterCapacity)
        || font_parameter_offset
            > font_parameter_bytes - font_parameter_size;
    font_layout_abi_available_ = !reflected_font_contract_invalid;
    font_abi_detail_mask_ = 0;
    const auto record_font_abi_failure = [this](
        std::uint32_t bit, bool failed) noexcept {
        if (failed) {
            font_abi_detail_mask_ |= bit;
        }
    };
    record_font_abi_failure(1U << 0U, !user_widget_class_);
    record_font_abi_failure(1U << 1U, !widget_tree_class_);
    record_font_abi_failure(1U << 2U, !canvas_panel_class_);
    record_font_abi_failure(1U << 3U, !border_class_);
    record_font_abi_failure(1U << 4U, !text_block_class_);
    record_font_abi_failure(1U << 5U, !game_text_block_class_);
    record_font_abi_failure(
        1U << 6U,
        !game_text_block_class_ || !text_block_class_
            || !game_text_block_class_->IsChildOf(text_block_class_));
    record_font_abi_failure(
        1U << 7U,
        !font_object_is_compatible(
            game_text_block_default_.Get(), text_block_class_,
            expected_font_property));
    record_font_abi_failure(
        1U << 8U,
        !CastField<FBoolProperty>(force_apply_language_font_property_));
    record_font_abi_failure(
        1U << 12U, reflected_font_contract_invalid);
    record_font_abi_failure(1U << 9U, !check_box_class_);
    record_font_abi_failure(
        1U << 10U, !widget_blueprint_library_.Get());
    record_font_abi_failure(
        1U << 11U, !widget_layout_library_.Get());
    // The game-specific DTextBlock class and its language-font helpers are
    // visual enhancements.  Keep their detail bits for diagnostics, but do
    // not let a missing optional font capability disable the complete F6 Hub.
    // The base UMG widget/class/library requirements remain fail-closed here,
    // The reflected Font/SetFont contract is also optional: when it is absent
    // or incompatible, the bounded render-scale text path still presents the
    // complete Hub without reading or mutating FSlateFontInfo.
    if (dswros::radar_visibility_hub_font_detail_is_fatal(
            font_abi_detail_mask_)) {
        abi_failure_mask_ |= 1U << 23U;
    }

    for (auto& font : language_fonts_) {
        font = FWeakObjectPtr{};
    }
    reset_runtime_handles();
    last_failure_ = 0;
    text_overlay_failure_ = 0;
    state_ = abi_failure_mask_ == 0
        ? RadarVisibilityHubState::Ready
        : RadarVisibilityHubState::Disabled;
}

void RadarVisibilityHub::resolve_language_fonts_once_unsafe() {
    bool has_missing_font = false;
    for (const auto& font : language_fonts_) {
        if (!font.Get()) {
            has_missing_font = true;
            break;
        }
    }
    if (!has_missing_font) {
        return;
    }

    // The game does not expose canonical package paths for these system fonts
    // through the menu contract. MnMRadar's accepted implementation proves
    // the stable boundary instead: enumerate the loaded Font class once per
    // real F6 open while any required slot is absent or expired, and match the
    // four exact asset short names. Already resolved fonts remain untouched.
    // This never runs on panel service, render, or engine-tick paths.
    std::vector<UObject*> loaded_fonts;
    UObjectGlobals::FindAllOf(L"Font", loaded_fonts);
    for (UObject* candidate : loaded_fonts) {
        if (!candidate) {
            continue;
        }
        const auto candidate_name = candidate->GetName();
        for (std::size_t index = 0;
             index < kLanguageFontObjectNames.size(); ++index) {
            if (!language_fonts_[index].Get()
                && candidate_name == kLanguageFontObjectNames[index]) {
                language_fonts_[index] = candidate;
                break;
            }
        }
    }
}

UObject* RadarVisibilityHub::language_font_unsafe(
    dswros::RadarUiLanguage language) const noexcept {
    const std::size_t index = static_cast<std::size_t>(
        dswros::radar_ui_font_family(language));
    return index < language_fonts_.size()
        ? language_fonts_[index].Get() : nullptr;
}

bool RadarVisibilityHub::apply_language_font_unsafe(
    UObject* text_block, dswros::RadarUiLanguage language) noexcept {
    if (!font_layout_abi_available_) {
        return false;
    }
    UObject* font_object = language_font_unsafe(language);
    auto* expected_font_property = CastField<FStructProperty>(
        text_block_font_property_);
    FStructProperty* live_font_property = compatible_font_property(
        text_block, text_block_class_, expected_font_property);
    void* live_font = live_font_property
        ? live_font_property->ContainerPtrToValuePtr<void>(text_block)
        : nullptr;
    auto* font_object_property = compatible_font_object_member(
        live_font_property);
    void* live_font_object = font_object_property && live_font
        ? font_object_property->ContainerPtrToValuePtr<void>(live_font)
        : nullptr;
    if (!font_object || !live_font_property || !live_font
        || !font_object_property || !live_font_object || !set_font_
        || !set_font_value_property_
        || set_font_value_property_->GetStruct()
            != live_font_property->GetStruct()
        || set_font_value_property_->GetSize()
            != live_font_property->GetSize()) {
        return false;
    }

    FontCallParameters parameters(set_font_);
    void* parameter_font = parameters.valid()
        ? set_font_value_property_->ContainerPtrToValuePtr<void>(
              parameters.data())
        : nullptr;
    void* parameter_font_object = font_object_property && parameter_font
        ? font_object_property->ContainerPtrToValuePtr<void>(parameter_font)
        : nullptr;
    if (!parameter_font || !parameter_font_object) {
        return false;
    }

    // Copy the complete current FSlateFontInfo so Size, LetterSpacing,
    // TypefaceFontName, outline, and material state remain unchanged. Replace
    // only FontObject in the reflected SetFont parameter so UTextBlock can
    // invalidate Slate normally, then verify the live readback.
    set_font_value_property_->CopyCompleteValue(parameter_font, live_font);
    font_object_property->SetObjectPropertyValue(
        parameter_font_object, font_object);
    text_block->ProcessEvent(set_font_, parameters.data());
    if (font_object_property->GetObjectPropertyValue(live_font_object)
        != font_object) {
        return false;
    }

    // DTextBlock's language helper otherwise replaces the explicit font on
    // the next prepass with the game's current-language font. Disable it only
    // after SetFont/readback succeeds. If the property is unavailable, the
    // explicit override is not considered committed and callers retain the
    // existing best-effort fallback contract.
    if (game_text_block_class_ && text_block->IsA(game_text_block_class_)) {
        return write_bool_property(
            text_block, force_apply_language_font_property_, false);
    }
    return true;
}

RadarVisibilityHubResult RadarVisibilityHub::toggle(
    UObject* current_controller,
    RadarVisibilityMaskWord current_masks,
    AreaQuestDisplayMode current_area_quest_mode,
    AssaultDisplayMode current_assault_mode,
    dswros::HeightIndicatorMask current_height_indicators,
    dswros::RadarLanguagePreference current_language,
    dswros::RadarUiLanguage detected_game_language,
    RadarModStatus current_mod_status) noexcept {
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
            0,
            source_area_quest_mode_,
            source_assault_mode_,
            source_height_indicators_,
            source_language_};
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
        current_assault_mode, current_height_indicators, current_language,
        detected_game_language, current_mod_status);
}

void set_justification(
    UObject* text_block, UFunction* function, std::uint8_t value) {
    if (!text_block || !function) {
        return;
    }
    ByteParameters parameters{value};
    text_block->ProcessEvent(function, &parameters);
}

RadarVisibilityHubResult RadarVisibilityHub::open_guarded(
    UObject* current_controller,
    RadarVisibilityMaskWord current_masks,
    AreaQuestDisplayMode current_area_quest_mode,
    AssaultDisplayMode current_assault_mode,
    dswros::HeightIndicatorMask current_height_indicators,
    dswros::RadarLanguagePreference current_language,
    dswros::RadarUiLanguage detected_game_language,
    RadarModStatus current_mod_status) noexcept {
#if defined(_MSC_VER)
    __try {
        RadarVisibilityHubResult result =
            open_unsafe(
                current_controller, current_masks,
                current_area_quest_mode, current_assault_mode,
                current_height_indicators, current_language,
                detected_game_language, current_mod_status);
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
                current_area_quest_mode, current_assault_mode,
                current_height_indicators, current_language,
                detected_game_language, current_mod_status);
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
    AssaultDisplayMode current_assault_mode,
    dswros::HeightIndicatorMask current_height_indicators,
    dswros::RadarLanguagePreference current_language,
    dswros::RadarUiLanguage detected_game_language,
    RadarModStatus current_mod_status) {
    last_failure_ = 0;
    font_fallback_reason_ = 0;
    text_runtime_failure_ = 0;
    text_overlay_failure_ = 0;
    text_overlay_active_ = false;
    const RadarVisibilityMaskWord sanitized = sanitize_masks(current_masks);
    source_masks_ = sanitized;
    pending_masks_ = sanitized;
    source_area_quest_mode_ = current_area_quest_mode;
    pending_area_quest_mode_ = current_area_quest_mode;
    source_assault_mode_ = current_assault_mode;
    pending_assault_mode_ = current_assault_mode;
    source_height_indicators_ =
        sanitize_height_indicators(current_height_indicators);
    pending_height_indicators_ = source_height_indicators_;
    detected_game_language_ =
        static_cast<std::size_t>(detected_game_language)
                < dswros::kRadarUiLanguageCount
            ? detected_game_language
            : dswros::RadarUiLanguage::English;
    source_language_ = sanitize_language(current_language);
    pending_language_ = source_language_;
    auto* expected_font_property = CastField<FStructProperty>(
        text_block_font_property_);
    UObject* game_text_block_default = game_text_block_default_.Get();
    const bool game_text_block_class_usable = game_text_block_class_
        && text_block_class_
        && game_text_block_class_->IsChildOf(text_block_class_);
    const bool game_default_font_compatible = font_object_is_compatible(
        game_text_block_default, text_block_class_, expected_font_property);
    const dswros::RadarVisibilityHubFontPlan font_plan =
        dswros::radar_visibility_hub_font_plan(
            game_text_block_class_usable, game_default_font_compatible);
    const bool game_text_block_usable = font_plan.use_game_text_widget;
    const bool force_language_font_available = game_text_block_usable
        && CastField<FBoolProperty>(force_apply_language_font_property_);
    bool copy_game_default_font = font_plan.copy_game_default_font;
    if (font_plan.source
        == dswros::RadarVisibilityHubFontPlanSource::TextBlockFallback) {
        font_source_ = RadarVisibilityHubFontSource::TextBlockFallback;
        font_fallback_reason_ = 4;
    } else if (font_plan.source == dswros::RadarVisibilityHubFontPlanSource::
                                       DTextBlockInheritedDefault) {
        font_source_ = RadarVisibilityHubFontSource::DTextBlockInheritedDefault;
    } else {
        font_source_ =
            RadarVisibilityHubFontSource::DTextBlockClassDefaultObject;
    }
    resolved_ui_language_ = dswros::resolve_radar_ui_language(
        pending_language_, detected_game_language_);
    resolve_language_fonts_once_unsafe();
    displayed_mod_status_ = current_mod_status;

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
    const double reference_scale = std::clamp(
        std::min(
            viewport_size.return_value.x / kReferenceViewportWidth,
            viewport_size.return_value.y / kReferenceViewportHeight),
        0.50,
        2.50);
    const double fit_width = viewport_size.return_value.x
        - kMinimumViewportMargin * 2.0;
    const double fit_height = viewport_size.return_value.y
        - kMinimumViewportMargin * 2.0;
    const double fit_scale = std::min(
        fit_width / kReferencePanelWidth,
        fit_height / kReferencePanelHeight);
    const double display_scale = std::min(reference_scale, fit_scale);
    if (!std::isfinite(display_scale) || display_scale <= 0.0) {
        last_failure_ = 4;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
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

    std::array<HubTextFontRecord, kMaximumHubTextCount> text_font_records{};
    std::size_t text_font_record_count{};
    bool use_game_text_widgets = game_text_block_usable;
    const auto add_text = [this, tree, &add_widget, unit_scale,
                           &use_game_text_widgets,
                           game_text_block_class_usable,
                           force_language_font_available,
                           game_text_block_default, &copy_game_default_font,
                           expected_font_property, &text_font_records,
                           &text_font_record_count](
        const wchar_t* value, double x, double y,
        double width, double height, std::int32_t z_order,
        double role_scale,
        std::uint8_t justification = kTextLeft,
        dswros::RadarUiLanguage font_language =
            dswros::RadarUiLanguage::Count) -> UObject* {
        UObject* text_block{};
        double target_font_size{};
        const dswros::RadarUiLanguage resolved_font_language =
            static_cast<std::size_t>(font_language)
                    < dswros::kRadarUiLanguageCount
                ? font_language : resolved_ui_language_;
        const auto prepare_text_widget =
            [this, tree, unit_scale, force_language_font_available,
             game_text_block_default, &copy_game_default_font,
             expected_font_property, role_scale, height,
             resolved_font_language,
             &text_block, &target_font_size](
                UClass* widget_class, bool use_game_font,
                std::uint32_t object_failure,
                std::uint32_t sizing_failure,
                std::uint32_t commit_failure) -> bool {
                text_block = UObjectGlobals::NewObject<UObject>(
                    tree, widget_class);
                if (!text_block) {
                    text_runtime_failure_ = object_failure;
                    return false;
                }
                if (use_game_font && force_language_font_available) {
                    // This is a best-effort game-font enhancement. A missing
                    // optional bool write must not make F6 unusable.
                    (void)write_bool_property(
                        text_block, force_apply_language_font_property_, true);
                }
                if (use_game_font && copy_game_default_font) {
                    // Preserve the responsive size/spacing already inherited
                    // by the new widget while copying only the compatible game
                    // composite-font state.
                    if (!copy_font_preserving_layout_metrics(
                            game_text_block_default, text_block,
                            text_block_class_, expected_font_property)) {
                        copy_game_default_font = false;
                        font_source_ = RadarVisibilityHubFontSource::
                            DTextBlockInheritedDefault;
                    }
                }
                // A missing system-font object or optional FontObject member
                // must never reject F6. When available, this one SetFont call
                // replaces only FSlateFontInfo.FontObject and disables the
                // DTextBlock game-language override after verified readback.
                (void)apply_language_font_unsafe(
                    text_block, resolved_font_language);
                if (!font_layout_abi_available_) {
                    text_runtime_failure_ = sizing_failure;
                    return false;
                }
                if (!calculate_target_font_size(
                        text_block, text_block_class_, expected_font_property,
                        unit_scale, role_scale, height, target_font_size)) {
                    text_runtime_failure_ = sizing_failure;
                    return false;
                }
                if (!apply_font_size_and_commit(
                        text_block, text_block_class_, expected_font_property,
                        set_font_, set_font_value_property_,
                        target_font_size)) {
                    text_runtime_failure_ = commit_failure;
                    return false;
                }
                return true;
            };

        const bool initial_use_game_text_widgets = use_game_text_widgets;
        bool game_prepare_failed{};
        const dswros::RadarVisibilityHubTextPrepareResult preparation =
            dswros::prepare_radar_visibility_hub_text_with_fallback(
                initial_use_game_text_widgets,
                [&prepare_text_widget, &use_game_text_widgets,
                 initial_use_game_text_widgets, &game_prepare_failed, this](
                    dswros::RadarVisibilityHubTextWidgetKind kind) {
                    const bool use_game = kind
                        == dswros::RadarVisibilityHubTextWidgetKind::
                            GameTextBlock;
                    if (use_game) {
                        const bool prepared = prepare_text_widget(
                            game_text_block_class_, true, 1, 2, 3);
                        game_prepare_failed = !prepared;
                        return dswros::RadarVisibilityHubTextPrepareAttempt{
                            prepared, text_runtime_failure_};
                    }
                    if (initial_use_game_text_widgets
                        && game_prepare_failed) {
                        // Publish the game-path failure before the base retry.
                        // A guarded fault inside that retry must not erase the
                        // only evidence explaining why the game font was left.
                        font_fallback_reason_ = text_runtime_failure_;
                        font_source_ =
                            RadarVisibilityHubFontSource::TextBlockFallback;
                        use_game_text_widgets = false;
                    }
                    const bool prepared = prepare_text_widget(
                        text_block_class_, false, 5, 6, 7);
                    return dswros::RadarVisibilityHubTextPrepareAttempt{
                        prepared, text_runtime_failure_};
                });
        if (preparation.game_attempted && preparation.base_attempted) {
            // A game-specific widget is an optional visual enhancement.
            // Disable it for the rest of this one Hub construction after its
            // first failure; the shared policy has already retried this text
            // with the known base UMG class.
            font_fallback_reason_ = preparation.fallback_reason;
            font_source_ = RadarVisibilityHubFontSource::TextBlockFallback;
            use_game_text_widgets = false;
        }
        text_runtime_failure_ = preparation.terminal_failure;
        if (!preparation.prepared) {
            // MnMRadar and the original accepted Hub both demonstrate the
            // important boundary here: text presentation must not depend on
            // mutating reflected FSlateFontInfo metrics.  Prefer a fresh game
            // DTextBlock so the current language keeps the game's composite
            // glyph coverage, then fall back to the base UMG TextBlock. The
            // explicit system-font override remains best-effort and this path
            // scales only the rendered text inside the bounded Canvas slot.
            const std::uint32_t layout_failure = text_runtime_failure_;
            bool uses_game_render_fallback = game_text_block_class_usable;
            text_block = uses_game_render_fallback
                ? UObjectGlobals::NewObject<UObject>(
                      tree, game_text_block_class_)
                : nullptr;
            if (!text_block) {
                uses_game_render_fallback = false;
                text_block = UObjectGlobals::NewObject<UObject>(
                    tree, text_block_class_);
            }
            if (!text_block) {
                text_runtime_failure_ = 13;
                return nullptr;
            }
            if (uses_game_render_fallback
                && force_language_font_available) {
                (void)write_bool_property(
                    text_block, force_apply_language_font_property_, true);
            }
            (void)apply_language_font_unsafe(
                text_block, resolved_font_language);
            target_font_size = 0.0;
            font_source_ = uses_game_render_fallback
                ? RadarVisibilityHubFontSource::
                      DTextBlockRenderScaleFallback
                : RadarVisibilityHubFontSource::
                      TextBlockRenderScaleFallback;
            font_fallback_reason_ = layout_failure;
        }
        text_runtime_failure_ = 0;
        set_text(text_block, set_text_, set_text_value_property_, value);
        set_justification(
            text_block, set_justification_, justification);
        if (target_font_size == 0.0) {
            if (set_render_pivot_) {
                const double pivot_axis = justification == kTextCenter
                    ? 0.5
                    : 0.0;
                VectorParameters pivot{{pivot_axis, 0.5}};
                text_block->ProcessEvent(set_render_pivot_, &pivot);
            }
            if (set_render_scale_) {
                const double rendered_scale = unit_scale * role_scale;
                VectorParameters scale{{rendered_scale, rendered_scale}};
                text_block->ProcessEvent(set_render_scale_, &scale);
            }
        }
        set_visibility(text_block, set_visibility_, kHitTestInvisible);
        UObject* text_slot = add_widget(
            text_block, x, y, width, height, z_order);
        if (!text_slot) {
            text_runtime_failure_ = 8;
            return nullptr;
        }
        if (text_font_record_count >= text_font_records.size()) {
            text_runtime_failure_ = 9;
            return nullptr;
        }
        text_font_records[text_font_record_count++] = {
            text_block,
            text_slot,
            target_font_size,
            target_font_size != 0.0,
            x,
            y,
            width,
            height};
        return text_block;
    };

    const auto& localized =
        dswros::radar_localized_text(resolved_ui_language_);
    const LanguageDisplayBuffer language_display = format_language_display(
        resolved_ui_language_);
    const wchar_t* status_value = current_mod_status == RadarModStatus::On
        ? localized.status_on
        : current_mod_status == RadarModStatus::Fault
            ? localized.status_fault
            : localized.status_off;
    const wchar_t* status_action = current_mod_status == RadarModStatus::On
        ? localized.disable_mod
        : current_mod_status == RadarModStatus::Fault
            ? localized.retry_mod
            : localized.enable_mod;
    const LinearColor status_color = current_mod_status == RadarModStatus::On
        ? kStatusOn
        : current_mod_status == RadarModStatus::Fault
            ? kStatusFault
            : kStatusOff;
    std::array<UObject*, kLocalizedTextCount> localized_texts{};
    std::array<UObject*, kCategoryCount> category_label_texts{};
    std::array<UObject*, kHeightIndicatorCount> height_label_texts{};
    const auto add_localized_text =
        [&localized_texts, &add_text](
            LocalizedTextSlot slot, const wchar_t* value,
            double x, double y, double width, double height,
            std::int32_t z_order, double text_scale,
            std::uint8_t justification = kTextLeft) -> UObject* {
        UObject* widget = add_text(
            value, x, y, width, height, z_order, text_scale,
            justification);
        localized_texts[static_cast<std::size_t>(slot)] = widget;
        return widget;
    };

    if (!add_border(
            0.0, 0.0, kReferencePanelWidth, kReferencePanelHeight,
            0, kPanelFrame)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0,
            kReferencePanelHeight - 4.0, 1, kPanelBackground)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0, 60.0, 2,
            kHeaderBackground)
        || !add_border(
            2.0, 2.0, kReferencePanelWidth - 4.0, 3.0, 3,
            kPanelAccent)
        || !add_border(2.0, 2.0, 4.0, 60.0, 3, kPanelAccent)
        || !add_border(14.0, 66.0, 652.0, 584.0, 2, kSectionFrame)
        || !add_border(16.0, 68.0, 648.0, 580.0, 2, kContentBackground)
        || !add_localized_text(
            LocalizedTextSlot::Title, localized.title,
            28.0, 17.0, 360.0, 34.0, 5, 0.70)
        || !add_border(402.0, 12.0, 144.0, 36.0, 19, kToggleFrame)
        || !add_border(405.0, 15.0, 138.0, 30.0, 20,
                       kBugReportButton)
        || !add_localized_text(
            LocalizedTextSlot::BugReport, localized.bug_report,
            411.0, 17.0, 126.0, 26.0, 21, 0.40, kTextCenter)
        || !add_border(550.0, 12.0, 112.0, 36.0, 19, kToggleFrame)
        || !add_border(553.0, 15.0, 106.0, 30.0, 20, kCloseButton)
        || !add_localized_text(
            LocalizedTextSlot::Close, localized.close,
            559.0, 17.0, 94.0, 26.0, 21, 0.43, kTextCenter)) {
        last_failure_ = 8;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    if (!add_border(20.0, 70.0, 640.0, 56.0, 3, kSectionFrame)
        || !add_border(22.0, 72.0, 636.0, 52.0, 4, kLanguageCard)
        || !add_border(152.0, 72.0, 376.0, 52.0, 4, kToggleFrame)
        || !add_border(154.0, 74.0, 372.0, 48.0, 5, kLanguageSelector)
        || !add_localized_text(
            LocalizedTextSlot::Language, localized.language,
            176.0, 76.0, 328.0, 17.0, 6, 0.34, kTextCenter)
        || !add_localized_text(
            LocalizedTextSlot::LanguageValue, language_display.data(),
            200.0, 92.0, 280.0, 27.0, 6, 0.50, kTextCenter)
        || !add_text(
            L"v", 482.0, 92.0, 22.0, 25.0, 7, 0.48,
            kTextCenter)) {
        last_failure_ = 31;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    UObject* language_dropdown_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    if (!language_dropdown_control) {
        last_failure_ = 31;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    set_checked(language_dropdown_control, set_is_checked_, false);
    set_render_opacity(
        language_dropdown_control, set_render_opacity_, 0.01F);
    if (!add_widget(
            language_dropdown_control, 148.0, 68.0, 384.0, 60.0, 12)) {
        last_failure_ = 31;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    // Status is deliberately presented as a slim signal rail plus text, not
    // as another button.  Only the separate action card to its right is
    // interactive.
    UObject* mod_status_visual = add_border(
        146.0, 143.0, 5.0, 22.0, 6, status_color);
    UObject* mod_action_visual = add_border(
        429.0, 139.0, 220.0, 30.0, 6, kActionButton);
    if (!add_border(20.0, 132.0, 640.0, 44.0, 3, kSectionFrame)
        || !add_border(22.0, 134.0, 636.0, 40.0, 4, kStatusCard)
        || !add_border(426.0, 136.0, 226.0, 36.0, 5, kToggleFrame)
        || !mod_status_visual || !mod_action_visual
        || !add_localized_text(
            LocalizedTextSlot::StatusLabel, localized.status,
            32.0, 142.0, 98.0, 24.0, 7, 0.40)
        || !add_localized_text(
            LocalizedTextSlot::StatusValue, status_value,
            158.0, 142.0, 154.0, 24.0, 7, 0.40, kTextCenter)
        || !add_localized_text(
            LocalizedTextSlot::StatusAction, status_action,
            435.0, 142.0, 208.0, 24.0, 7, 0.38, kTextCenter)) {
        last_failure_ = 36;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    UObject* mod_action_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    UObject* bug_report_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    if (!mod_action_control || !bug_report_control) {
        last_failure_ = 36;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    set_checked(mod_action_control, set_is_checked_, false);
    set_checked(bug_report_control, set_is_checked_, false);
    set_render_opacity(mod_action_control, set_render_opacity_, 0.01F);
    set_render_opacity(bug_report_control, set_render_opacity_, 0.01F);
    if (!add_widget(
            mod_action_control, 424.0, 134.0, 230.0, 40.0, 12)
        || !add_widget(
            bug_report_control, 400.0, 8.0, 146.0, 50.0, 22)) {
        last_failure_ = 36;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    std::array<UObject*, kLanguagePopupDecorationCount>
        language_popup_decorations{};
    std::array<UObject*, kLanguageChoiceCount> language_choice_controls{};
    std::array<UObject*, kLanguageChoiceCount>
        language_choice_selected_visuals{};
    std::array<UObject*, kLanguageChoiceCount> language_choice_texts{};
    std::size_t popup_decoration_index{};
    const auto add_popup_decoration =
        [&language_popup_decorations, &popup_decoration_index](
            UObject* widget) -> bool {
        if (!widget
            || popup_decoration_index
                >= language_popup_decorations.size()) {
            return false;
        }
        language_popup_decorations[popup_decoration_index++] = widget;
        return true;
    };
    if (!add_popup_decoration(add_border(
            20.0, 128.0, 640.0, 520.0, 29, kPopupDim))
        || !add_popup_decoration(add_border(
            28.0, 130.0, 624.0, 182.0, 31, kPanelFrame))
        || !add_popup_decoration(add_border(
            31.0, 133.0, 618.0, 176.0, 32, kPopupBackground))) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    UObject* language_popup_dismiss_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    if (!language_popup_dismiss_control) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    set_checked(language_popup_dismiss_control, set_is_checked_, false);
    set_render_opacity(
        language_popup_dismiss_control, set_render_opacity_, 0.01F);
    if (!add_widget(
            language_popup_dismiss_control,
            20.0, 126.0, 640.0, 522.0, 30)) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    for (std::size_t index = 0; index < kLanguageChoiceCount; ++index) {
        const std::size_t column = index % 3U;
        const std::size_t row = index / 3U;
        const double x = 39.0 + static_cast<double>(column) * 202.0;
        const double y = 141.0 + static_cast<double>(row) * 40.0;
        const auto choice = dswros::radar_language_choice(index);
        const bool follow_game = choice == dswros::RadarLanguagePreference::Auto;
        const auto choice_language = dswros::explicit_radar_ui_language(choice);
        // A shared Latin label keeps this new cell readable even where the
        // game's Korean/TC fonts need the existing fixed raster fallbacks.
        const wchar_t* choice_label = follow_game ? L"AUTO (Game Language)"
            : dswros::radar_localized_text(choice_language).language_name;
        if (!add_popup_decoration(add_border(
                x, y, 190.0, 32.0, 33, kLanguageOption))) {
            last_failure_ = 35;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        language_choice_selected_visuals[index] = add_border(
            x + 2.0, y + 2.0, 186.0, 28.0, 34,
            kLanguageSelected);
        language_choice_texts[index] = add_text(
            choice_label, x + 5.0, y + 4.0, 180.0, 24.0, 35,
            0.42, kTextCenter, choice_language);
        language_choice_controls[index] =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!language_choice_selected_visuals[index]
            || !language_choice_texts[index]
            || !language_choice_controls[index]) {
            last_failure_ = 35;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        set_checked(
            language_choice_controls[index], set_is_checked_, false);
        set_render_opacity(
            language_choice_controls[index], set_render_opacity_, 0.01F);
        if (!add_widget(
                language_choice_controls[index],
                x, y, 190.0, 32.0, 40)) {
            last_failure_ = 35;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
    }
    if (popup_decoration_index != language_popup_decorations.size()) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    if (!add_localized_text(
            LocalizedTextSlot::MarkerVisibility,
            localized.marker_visibility,
            32.0, 187.0, 350.0, 22.0, 5, 0.47)
        || !add_border(420.0, 186.0, 104.0, 22.0, 3, kCompactHeader)
        || !add_localized_text(
            LocalizedTextSlot::Radar, localized.radar,
            426.0, 187.0, 92.0, 20.0, 5, 0.46, kTextCenter)
        || !add_border(550.0, 186.0, 102.0, 22.0, 3, kWorldHeader)
        || !add_localized_text(
            LocalizedTextSlot::Map, localized.map,
            556.0, 187.0, 90.0, 20.0, 5, 0.46, kTextCenter)) {
        last_failure_ = 8;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    std::array<std::array<UObject*, kCategoryCount>, kColumnCount>
        controls{};
    std::array<std::array<UObject*, kCategoryCount>, kColumnCount>
        enabled_visuals{};
    std::array<UObject*, kHeightIndicatorCount> height_controls{};
    std::array<UObject*, kHeightIndicatorCount> height_visuals{};
    std::array<UObject*, 2> area_mode_controls{};
    std::array<UObject*, 2> area_mode_visuals{};
    std::array<UObject*, 2> assault_mode_controls{};
    std::array<UObject*, 2> assault_mode_visuals{};
    const std::array<double, kColumnCount> control_x{{459.0, 588.0}};
    for (std::size_t row = 0; row < kRows.size(); ++row) {
        const RowDefinition& definition = kRows[row];
        const std::size_t category_index =
            static_cast<std::size_t>(definition.category);
        const double y = 212.0 + static_cast<double>(row) * 30.0;
        if (!add_border(
                20.0, y - 1.0, 640.0, 28.0, 3,
                row % 2U == 0U ? kBaseRow : kAlternateRow)) {
            last_failure_ = 9;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        category_label_texts[category_index] = add_text(
            localized.marker_categories[category_index],
            32.0, y, 370.0, 26.0, 5, 0.52);
        if (!category_label_texts[category_index]
            || !add_border(28.0, y + 27.0, 624.0, 1.0, 4,
                           kRowDivider)) {
            last_failure_ = 9;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        for (std::size_t column = 0; column < kColumnCount; ++column) {
            if (column == 1
                && (definition.category == RadarVisibilityCategory::Clock
                    || definition.category
                        == RadarVisibilityCategory::BirdEggs)) {
                if (!add_text(
                        L"-", control_x[column] - 1.0, y,
                        28.0, 26.0, 5, 0.58, kTextCenter)) {
                    last_failure_ = 10;
                    return {RadarVisibilityHubAction::Rejected, sanitized,
                            false, last_failure_};
                }
                continue;
            }
            const std::uint8_t mask = column == 0
                ? compact_radar_visibility_mask(sanitized)
                : world_radar_visibility_mask(sanitized);
            const bool enabled =
                (mask & radar_visibility_bit(definition.category)) != 0;
            UObject* outer = add_border(
                control_x[column], y + 3.0, 26.0, 20.0, 10, kToggleFrame);
            UObject* inner = add_border(
                control_x[column] + 4.0, y + 7.0, 18.0, 12.0, 11,
                column == 0 ? kCompactEnabled : kWorldEnabled);
            UObject* check_box =
                UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
            if (!outer || !inner || !check_box) {
                last_failure_ = 11;
                return {RadarVisibilityHubAction::Rejected, sanitized, false,
                        last_failure_};
            }
            set_visibility(
                inner, set_visibility_, enabled ? kVisible : kCollapsed);
            set_checked(check_box, set_is_checked_, enabled);
            set_render_opacity(check_box, set_render_opacity_, 0.01F);
            if (!add_widget(
                    check_box, control_x[column] - 5.0, y - 1.0,
                    36.0, 28.0, 12)) {
                last_failure_ = 12;
                return {RadarVisibilityHubAction::Rejected, sanitized, false,
                        last_failure_};
            }
            controls[column][category_index] = check_box;
            enabled_visuals[column][category_index] = inner;
        }
    }

    if (!add_border(20.0, 424.0, 640.0, 2.0, 4, kPanelAccent)
        || !add_localized_text(
            LocalizedTextSlot::HeightIndicators,
            localized.height_indicators,
            32.0, 430.0, 350.0, 22.0, 5, 0.47)
        || !add_border(518.0, 428.0, 134.0, 22.0, 3, kCompactHeader)
        || !add_localized_text(
            LocalizedTextSlot::RadarOnly, localized.radar_only,
            524.0, 430.0, 122.0, 20.0, 5, 0.40, kTextCenter)) {
        last_failure_ = 32;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    for (std::size_t index = 0; index < kHeightIndicatorCount; ++index) {
        const double y = 456.0 + static_cast<double>(index) * 30.0;
        const auto category =
            static_cast<dswros::HeightIndicatorCategory>(index);
        const bool enabled = dswros::height_indicator_enabled(
            pending_height_indicators_, category);
        if (!add_border(
                20.0, y - 1.0, 640.0, 28.0, 3,
                index % 2U == 0U ? kBaseRow : kAlternateRow)) {
            last_failure_ = 32;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        height_label_texts[index] = add_text(
            localized.height_categories[index],
            32.0, y, 500.0, 26.0, 5, 0.52);
        UObject* outer = add_border(
            control_x[1], y + 3.0, 26.0, 20.0, 10, kToggleFrame);
        UObject* inner = add_border(
            control_x[1] + 4.0, y + 7.0, 18.0, 12.0, 11,
            kCompactEnabled);
        UObject* check_box =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!height_label_texts[index] || !outer || !inner || !check_box
            || !add_border(28.0, y + 27.0, 624.0, 1.0, 4,
                           kRowDivider)) {
            last_failure_ = 32;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        set_visibility(
            inner, set_visibility_, enabled ? kVisible : kCollapsed);
        set_checked(check_box, set_is_checked_, enabled);
        set_render_opacity(check_box, set_render_opacity_, 0.01F);
        if (!add_widget(
                check_box, control_x[1] - 5.0, y - 1.0,
                36.0, 28.0, 12)) {
            last_failure_ = 32;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        height_controls[index] = check_box;
        height_visuals[index] = inner;
    }

    if (!add_border(20.0, 548.0, 640.0, 2.0, 4, kPanelAccent)
        || !add_localized_text(
            LocalizedTextSlot::FilterModes, localized.filter_modes,
            32.0, 554.0, 350.0, 22.0, 5, 0.47)) {
        last_failure_ = 16;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    const std::array<double, 2> mode_y{{580.0, 612.0}};
    const std::array<double, 2> mode_x{{330.0, 492.0}};
    const std::array<double, 2> mode_width{{154.0, 154.0}};
    const std::array<const wchar_t*, 2> mode_labels{{
        localized.marker_categories[static_cast<std::size_t>(
            RadarVisibilityCategory::AreaQuests)],
        localized.marker_categories[static_cast<std::size_t>(
            RadarVisibilityCategory::Assault)],
    }};
    const std::array<LocalizedTextSlot, 2> mode_label_slots{{
        LocalizedTextSlot::AreaQuestMode,
        LocalizedTextSlot::AssaultMode,
    }};
    const std::array<LocalizedTextSlot, 2> available_slots{{
        LocalizedTextSlot::AreaAvailable,
        LocalizedTextSlot::AssaultAvailable,
    }};
    const std::array<LocalizedTextSlot, 2> all_slots{{
        LocalizedTextSlot::AreaAll,
        LocalizedTextSlot::AssaultAll,
    }};
    for (std::size_t mode_row = 0; mode_row < mode_y.size(); ++mode_row) {
        const double y = mode_y[mode_row];
        if (!add_border(
                20.0, y - 1.0, 640.0, 28.0, 3,
                mode_row == 0U ? kAlternateRow : kBaseRow)
            || !add_localized_text(
                mode_label_slots[mode_row], mode_labels[mode_row],
                32.0, y, 280.0, 26.0, 5, 0.48)
            || !add_border(
                mode_x[0], y + 1.0, mode_width[0], 24.0, 4,
                kSectionFrame)
            || !add_border(
                mode_x[0] + 2.0, y + 3.0,
                mode_width[0] - 4.0, 20.0, 5, kModeOption)
            || !add_border(
                mode_x[1], y + 1.0, mode_width[1], 24.0, 4,
                kSectionFrame)
            || !add_border(
                mode_x[1] + 2.0, y + 3.0,
                mode_width[1] - 4.0, 20.0, 5, kModeOption)
            || !add_localized_text(
                available_slots[mode_row], localized.available,
                mode_x[0] + 4.0, y + 3.0, mode_width[0] - 8.0,
                20.0, 7, 0.37, kTextCenter)
            || !add_localized_text(
                all_slots[mode_row], localized.all,
                mode_x[1] + 4.0, y + 3.0, mode_width[1] - 8.0,
                20.0, 7, 0.37, kTextCenter)
            || !add_border(28.0, y + 27.0, 624.0, 1.0, 4,
                           kRowDivider)) {
            last_failure_ = mode_row == 0U ? 16U : 27U;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
    }

    const std::array<AreaQuestDisplayMode, 2> mode_values{{
        AreaQuestDisplayMode::Available,
        AreaQuestDisplayMode::AllUnfinished,
    }};
    for (std::size_t mode_index = 0; mode_index < mode_values.size();
         ++mode_index) {
        const bool selected = current_area_quest_mode
            == mode_values[mode_index];
        const double x = mode_x[mode_index];
        const double width = mode_width[mode_index];
        UObject* inner = add_border(
            x + 2.0, mode_y[0] + 3.0, width - 4.0, 20.0, 5,
            mode_index == 0U
                ? kModeSelectedAvailable : kModeSelectedAll);
        UObject* check_box =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!inner || !check_box) {
            last_failure_ = 17;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        set_visibility(
            inner, set_visibility_, selected ? kVisible : kCollapsed);
        set_checked(check_box, set_is_checked_, selected);
        set_render_opacity(check_box, set_render_opacity_, 0.01F);
        if (!add_widget(
                check_box, x, mode_y[0], width, 26.0, 12)) {
            last_failure_ = 18;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        area_mode_controls[mode_index] = check_box;
        area_mode_visuals[mode_index] = inner;
    }

    const std::array<AssaultDisplayMode, 2> assault_mode_values{{
        AssaultDisplayMode::Current,
        AssaultDisplayMode::All,
    }};
    for (std::size_t mode_index = 0;
         mode_index < assault_mode_values.size(); ++mode_index) {
        const bool selected = current_assault_mode
            == assault_mode_values[mode_index];
        const double x = mode_x[mode_index];
        const double width = mode_width[mode_index];
        UObject* inner = add_border(
            x + 2.0, mode_y[1] + 3.0, width - 4.0, 20.0, 5,
            mode_index == 0U
                ? kModeSelectedAvailable : kModeSelectedAll);
        UObject* check_box =
            UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!inner || !check_box) {
            last_failure_ = 28;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        set_visibility(
            inner, set_visibility_, selected ? kVisible : kCollapsed);
        set_checked(check_box, set_is_checked_, selected);
        set_render_opacity(check_box, set_render_opacity_, 0.01F);
        if (!add_widget(
                check_box, x, mode_y[1], width, 26.0, 12)) {
            last_failure_ = 29;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
        assault_mode_controls[mode_index] = check_box;
        assault_mode_visuals[mode_index] = inner;
    }

    UObject* close_control =
        UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
    if (!close_control) {
        last_failure_ = 13;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    set_checked(close_control, set_is_checked_, false);
    set_render_opacity(close_control, set_render_opacity_, 0.01F);
    if (!add_widget(close_control, 548.0, 8.0, 116.0, 50.0, 22)) {
        last_failure_ = 13;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    // The optional packaged text path owns pixels only. These images never
    // participate in hit testing, and every existing CheckBox remains above
    // the corresponding visual layer. A 2x 1360x1320 texture is displayed in
    // this unchanged 680x660 reference rectangle, so the existing viewport /
    // DPI calculation remains the sole layout authority.
    UObject* main_text_overlay_image{};
    UObject* popup_text_overlay_image{};
    if (image_class_) {
        main_text_overlay_image =
            UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (main_text_overlay_image) {
            set_visibility(
                main_text_overlay_image, set_visibility_, kCollapsed);
            if (!add_widget(
                    main_text_overlay_image, 0.0, 0.0,
                    kReferencePanelWidth, kReferencePanelHeight, 21)) {
                main_text_overlay_image = nullptr;
            }
        }
        popup_text_overlay_image =
            UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (popup_text_overlay_image) {
            set_visibility(
                popup_text_overlay_image, set_visibility_, kCollapsed);
            if (!add_widget(
                    popup_text_overlay_image, 0.0, 0.0,
                    kReferencePanelWidth, kReferencePanelHeight, 35)) {
                popup_text_overlay_image = nullptr;
            }
        }
    }

    // Publish weak handles before the first viewport/input mutation so every
    // guarded failure can release the exact transient tree without keeping a
    // controller or raw widget across frames.
    host_ = host;
    widget_tree_ = tree;
    root_panel_ = root;
    main_text_overlay_image_ = main_text_overlay_image;
    popup_text_overlay_image_ = popup_text_overlay_image;
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        for (std::size_t category = 0; category < kCategoryCount; ++category) {
            controls_[column][category] = controls[column][category];
            enabled_visuals_[column][category] =
                enabled_visuals[column][category];
        }
    }
    for (std::size_t index = 0; index < kHeightIndicatorCount; ++index) {
        height_controls_[index] = height_controls[index];
        height_enabled_visuals_[index] = height_visuals[index];
        height_label_texts_[index] = height_label_texts[index];
    }
    for (std::size_t index = 0; index < kLocalizedTextCount; ++index) {
        localized_texts_[index] = localized_texts[index];
    }
    for (std::size_t index = 0; index < kCategoryCount; ++index) {
        category_label_texts_[index] = category_label_texts[index];
    }
    language_dropdown_control_ = language_dropdown_control;
    language_popup_dismiss_control_ = language_popup_dismiss_control;
    for (std::size_t index = 0;
         index < kLanguagePopupDecorationCount; ++index) {
        language_popup_decorations_[index] =
            language_popup_decorations[index];
    }
    for (std::size_t index = 0; index < kLanguageChoiceCount; ++index) {
        language_choice_controls_[index] = language_choice_controls[index];
        language_choice_selected_visuals_[index] =
            language_choice_selected_visuals[index];
        language_choice_texts_[index] = language_choice_texts[index];
    }
    area_mode_available_control_ = area_mode_controls[0];
    area_mode_all_control_ = area_mode_controls[1];
    area_mode_available_visual_ = area_mode_visuals[0];
    area_mode_all_visual_ = area_mode_visuals[1];
    assault_mode_current_control_ = assault_mode_controls[0];
    assault_mode_all_control_ = assault_mode_controls[1];
    assault_mode_current_visual_ = assault_mode_visuals[0];
    assault_mode_all_visual_ = assault_mode_visuals[1];
    mod_status_visual_ = mod_status_visual;
    mod_action_visual_ = mod_action_visual;
    mod_action_control_ = mod_action_control;
    bug_report_control_ = bug_report_control;
    close_control_ = close_control;
    if (text_font_record_count > text_layout_records_.size()) {
        text_runtime_failure_ = 9;
        last_failure_ = 37;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    text_layout_record_count_ = text_font_record_count;
    for (std::size_t index = 0; index < text_font_record_count; ++index) {
        const HubTextFontRecord& source = text_font_records[index];
        text_layout_records_[index] = {
            source.widget,
            source.slot,
            source.allow_desired_size_centering,
            source.authored_x * unit_scale,
            source.authored_y * unit_scale,
            source.authored_width * unit_scale,
            source.authored_height * unit_scale};
    }

    // The packaged text layer is presentation-only.  Its loader records an
    // exact failure code and restores the native TextBlocks, but it must never
    // reject an otherwise functional F6 settings Hub.
    (void)refresh_packaged_text_overlay_unsafe(current_controller);
    if (!set_language_popup_visibility_unsafe(false)) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

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

    // DTextBlock may select a language-specific font while Slate constructs
    // the widget. Recommit the bounded layout size after that first prepass so
    // the selected game font is retained while its real Font.Size, rather than
    // a render-only transform, owns layout. Any missing widget, ABI mismatch,
    // or readback disagreement rejects and detaches the complete transient Hub.
    if (text_font_record_count == 0) {
        text_runtime_failure_ = 10;
        last_failure_ = 37;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }
    for (std::size_t index = 0; index < text_font_record_count; ++index) {
        const HubTextFontRecord& record = text_font_records[index];
        if (record.target_size == 0.0) {
            continue;
        }
        if (!apply_font_size_and_commit(
                record.widget, text_block_class_, expected_font_property,
                set_font_, set_font_value_property_, record.target_size)) {
            text_runtime_failure_ = 11;
            last_failure_ = 37;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
    }
    host->ProcessEvent(force_layout_prepass_, nullptr);
    for (std::size_t index = 0; index < text_font_record_count; ++index) {
        const HubTextFontRecord& record = text_font_records[index];
        if (record.target_size == 0.0) {
            continue;
        }
        double readback_size{};
        if (!read_font_size(
                record.widget, text_block_class_, expected_font_property,
                readback_size)
            || !font_metric_matches(
                readback_size, record.target_size)) {
            text_runtime_failure_ = 12;
            last_failure_ = 37;
            return {RadarVisibilityHubAction::Rejected, sanitized, false,
                    last_failure_};
        }
    }

    if (!recenter_native_text_unsafe()) {
        text_runtime_failure_ = 14;
        last_failure_ = 37;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

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
        current_assault_mode,
        source_height_indicators_,
        source_language_};
}

UObject* RadarVisibilityHub::import_text_overlay_unsafe(
    UObject* world_context,
    const std::filesystem::path& path) {
    UObject* rendering_library = kismet_rendering_library_.Get();
    if (!world_context || !rendering_library || !import_file_as_texture_
        || path.empty()) {
        return nullptr;
    }
    const std::wstring native_path = path.wstring();
    if (native_path.empty()) {
        return nullptr;
    }
    ImportFileParameters parameters(world_context, native_path.c_str());
    rendering_library->ProcessEvent(import_file_as_texture_, &parameters);
    return parameters.return_value;
}

bool RadarVisibilityHub::apply_text_overlay_unsafe(
    UObject* image, UObject* texture) {
    if (!image || !texture || !image_class_
        || !image->IsA(image_class_) || !set_brush_from_texture_) {
        return false;
    }
    SetBrushFromTextureParameters brush{texture, false};
    image->ProcessEvent(set_brush_from_texture_, &brush);
    return read_struct_object_property(
               image, L"Brush", L"ResourceObject") == texture;
}

bool RadarVisibilityHub::set_native_text_visibility_unsafe(bool visible) {
    const std::uint8_t visibility = visible
        ? kHitTestInvisible : kCollapsed;
    for (auto& handle : localized_texts_) {
        UObject* text = handle.Get();
        if (!text) {
            return false;
        }
        set_visibility(text, set_visibility_, visibility);
    }
    for (auto& handle : category_label_texts_) {
        UObject* text = handle.Get();
        if (!text) {
            return false;
        }
        set_visibility(text, set_visibility_, visibility);
    }
    for (auto& handle : height_label_texts_) {
        UObject* text = handle.Get();
        if (!text) {
            return false;
        }
        set_visibility(text, set_visibility_, visibility);
    }
    return true;
}

bool RadarVisibilityHub::refresh_packaged_text_overlay_unsafe(
    UObject* world_context) {
    UObject* image = main_text_overlay_image_.Get();
    const bool requested = uses_packaged_text_overlay(
        resolved_ui_language_);
    if (!requested) {
        text_overlay_active_ = false;
        text_overlay_failure_ = 0;
        if (image) {
            set_visibility(image, set_visibility_, kCollapsed);
        }
        return set_native_text_visibility_unsafe(true);
    }

    if (!image || !image_class_ || !kismet_rendering_library_.Get()
        || !set_brush_from_texture_ || !import_file_as_texture_) {
        text_overlay_active_ = false;
        text_overlay_failure_ = 1;
        if (image) {
            set_visibility(image, set_visibility_, kCollapsed);
        }
        return set_native_text_visibility_unsafe(true);
    }

    UObject* texture = main_text_overlay_texture_.Get();
    if (!texture || text_overlay_language_ != resolved_ui_language_
        || text_overlay_status_ != displayed_mod_status_) {
        const wchar_t* prefix = packaged_text_overlay_prefix(
            resolved_ui_language_);
        const wchar_t* status = packaged_text_overlay_status(
            displayed_mod_status_);
        std::wstring filename = prefix ? prefix : L"";
        filename += L"-";
        filename += status;
        filename += L".tga";
        texture = import_text_overlay_unsafe(
            world_context, text_overlay_root_ / filename);
        if (!texture) {
            text_overlay_active_ = false;
            text_overlay_failure_ = 2;
            set_visibility(image, set_visibility_, kCollapsed);
            return set_native_text_visibility_unsafe(true);
        }
        if (!apply_text_overlay_unsafe(image, texture)) {
            text_overlay_active_ = false;
            text_overlay_failure_ = 3;
            set_visibility(image, set_visibility_, kCollapsed);
            return set_native_text_visibility_unsafe(true);
        }
        main_text_overlay_texture_ = texture;
        text_overlay_language_ = resolved_ui_language_;
        text_overlay_status_ = displayed_mod_status_;
    }

    if (!set_native_text_visibility_unsafe(false)) {
        return false;
    }
    set_visibility(image, set_visibility_, kHitTestInvisible);
    text_overlay_active_ = true;
    text_overlay_failure_ = 0;
    return true;
}

bool RadarVisibilityHub::ensure_language_popup_overlay_unsafe(
    UObject* world_context) {
    UObject* image = popup_text_overlay_image_.Get();
    if (!image || !image_class_ || !kismet_rendering_library_.Get()
        || !set_brush_from_texture_ || !import_file_as_texture_) {
        text_overlay_failure_ = 4;
        return false;
    }
    if (popup_text_overlay_texture_.Get()) {
        return true;
    }
    UObject* texture = import_text_overlay_unsafe(
        world_context, text_overlay_root_ / L"language-popup.tga");
    if (!texture) {
        text_overlay_failure_ = 5;
        return false;
    }
    if (!apply_text_overlay_unsafe(image, texture)) {
        text_overlay_failure_ = 6;
        return false;
    }
    popup_text_overlay_texture_ = texture;
    text_overlay_failure_ = 0;
    return true;
}

bool RadarVisibilityHub::refresh_localized_text_unsafe() {
    const auto& localized =
        dswros::radar_localized_text(resolved_ui_language_);
    const LanguageDisplayBuffer language_display = format_language_display(
        resolved_ui_language_);
    const wchar_t* status_value = displayed_mod_status_ == RadarModStatus::On
        ? localized.status_on
        : displayed_mod_status_ == RadarModStatus::Fault
            ? localized.status_fault
            : localized.status_off;
    const wchar_t* status_action = displayed_mod_status_ == RadarModStatus::On
        ? localized.disable_mod
        : displayed_mod_status_ == RadarModStatus::Fault
            ? localized.retry_mod
            : localized.enable_mod;
    const std::array<const wchar_t*, kLocalizedTextCount> values{{
        localized.title,
        localized.close,
        localized.language,
        language_display.data(),
        localized.marker_visibility,
        localized.radar,
        localized.map,
        localized.height_indicators,
        localized.radar_only,
        localized.filter_modes,
        localized.marker_categories[static_cast<std::size_t>(
            RadarVisibilityCategory::AreaQuests)],
        localized.marker_categories[static_cast<std::size_t>(
            RadarVisibilityCategory::Assault)],
        localized.available,
        localized.all,
        localized.available,
        localized.all,
        localized.status,
        status_value,
        status_action,
        localized.bug_report,
    }};
    for (std::size_t index = 0; index < values.size(); ++index) {
        UObject* text = localized_texts_[index].Get();
        if (!text) {
            return false;
        }
        (void)apply_language_font_unsafe(text, resolved_ui_language_);
        set_text(text, set_text_, set_text_value_property_, values[index]);
    }
    for (std::size_t index = 0; index < kCategoryCount; ++index) {
        UObject* text = category_label_texts_[index].Get();
        if (!text) {
            return false;
        }
        (void)apply_language_font_unsafe(text, resolved_ui_language_);
        set_text(
            text, set_text_, set_text_value_property_,
            localized.marker_categories[index]);
    }
    for (std::size_t index = 0; index < kHeightIndicatorCount; ++index) {
        UObject* text = height_label_texts_[index].Get();
        if (!text) {
            return false;
        }
        (void)apply_language_font_unsafe(text, resolved_ui_language_);
        set_text(
            text, set_text_, set_text_value_property_,
            localized.height_categories[index]);
    }
    if (!refresh_packaged_text_overlay_unsafe(host_.Get())) {
        return false;
    }
    return recenter_native_text_unsafe();
}

bool RadarVisibilityHub::refresh_mod_status_unsafe(
    RadarModStatus current_mod_status) {
    UObject* status_visual = mod_status_visual_.Get();
    UObject* action_visual = mod_action_visual_.Get();
    UObject* status_text = localized_texts_[static_cast<std::size_t>(
        LocalizedTextSlot::StatusValue)].Get();
    UObject* action_text = localized_texts_[static_cast<std::size_t>(
        LocalizedTextSlot::StatusAction)].Get();
    if (!status_visual || !action_visual || !status_text || !action_text) {
        return false;
    }
    displayed_mod_status_ = current_mod_status;
    const auto& localized =
        dswros::radar_localized_text(resolved_ui_language_);
    const wchar_t* status_value = current_mod_status == RadarModStatus::On
        ? localized.status_on
        : current_mod_status == RadarModStatus::Fault
            ? localized.status_fault
            : localized.status_off;
    const wchar_t* status_action = current_mod_status == RadarModStatus::On
        ? localized.disable_mod
        : current_mod_status == RadarModStatus::Fault
            ? localized.retry_mod
            : localized.enable_mod;
    const LinearColor status_color = current_mod_status == RadarModStatus::On
        ? kStatusOn
        : current_mod_status == RadarModStatus::Fault
            ? kStatusFault
            : kStatusOff;
    set_brush_color(status_visual, set_brush_color_, status_color);
    set_brush_color(action_visual, set_brush_color_, kActionButton);
    set_text(
        status_text, set_text_, set_text_value_property_, status_value);
    set_text(
        action_text, set_text_, set_text_value_property_, status_action);
    if (!refresh_packaged_text_overlay_unsafe(host_.Get())) {
        return false;
    }
    return recenter_native_text_unsafe();
}

bool RadarVisibilityHub::recenter_native_text_unsafe() {
    UObject* host = host_.Get();
    if (!host || !force_layout_prepass_) {
        return false;
    }
    if (!get_desired_size_) {
        return true;
    }

    // Text can change height after a language/font/status update, and popup
    // choices cannot produce a reliable desired size while Collapsed. Always
    // restore each original slot first, then measure the currently visible
    // Slate line boxes. Invalid or oversized measurements leave the authored
    // geometry intact, preserving the optional fail-open contract.
    bool restored_text_slot{};
    for (std::size_t index = 0;
         index < text_layout_record_count_; ++index) {
        const TextLayoutRecord& record = text_layout_records_[index];
        UObject* widget = record.widget.Get();
        UObject* slot = record.slot.Get();
        if (!widget || !slot
            || !std::isfinite(record.authored_x)
            || !std::isfinite(record.authored_y)
            || !std::isfinite(record.authored_width)
            || !std::isfinite(record.authored_height)
            || !(record.authored_width > 0.0)
            || !(record.authored_height > 0.0)) {
            continue;
        }
        set_slot_vector(
            slot, set_slot_position_,
            record.authored_x, record.authored_y);
        set_slot_vector(
            slot, set_slot_size_,
            record.authored_width, record.authored_height);
        restored_text_slot = true;
    }
    if (!restored_text_slot) {
        return true;
    }
    host->ProcessEvent(force_layout_prepass_, nullptr);

    bool centered_text_slot{};
    for (std::size_t index = 0;
         index < text_layout_record_count_; ++index) {
        const TextLayoutRecord& record = text_layout_records_[index];
        UObject* widget = record.widget.Get();
        UObject* slot = record.slot.Get();
        Vector2D desired{};
        if (!record.allow_desired_size_centering
            || !widget || !slot
            || !try_read_desired_size(
                widget, get_desired_size_, desired)) {
            continue;
        }
        const dswros::RadarVisibilityHubCenteredTextSlot centered =
            dswros::center_radar_visibility_hub_text_slot(
                record.authored_y,
                record.authored_height,
                desired.y);
        if (!centered.centered) {
            continue;
        }
        set_slot_vector(
            slot, set_slot_position_,
            record.authored_x, centered.top);
        set_slot_vector(
            slot, set_slot_size_,
            record.authored_width, centered.height);
        centered_text_slot = true;
    }
    if (centered_text_slot) {
        host->ProcessEvent(force_layout_prepass_, nullptr);
    }
    return true;
}

bool RadarVisibilityHub::set_language_popup_visibility_unsafe(bool visible) {
    const std::uint8_t popup_visibility = visible ? kVisible : kCollapsed;
    UObject* dismiss_control = language_popup_dismiss_control_.Get();
    if (!dismiss_control) {
        return false;
    }
    set_checked(dismiss_control, set_is_checked_, false);
    set_visibility(dismiss_control, set_visibility_, popup_visibility);
    for (auto& handle : language_popup_decorations_) {
        UObject* decoration = handle.Get();
        if (!decoration) {
            return false;
        }
        set_visibility(decoration, set_visibility_, popup_visibility);
    }
    const bool packaged_popup_ready = visible
        && ensure_language_popup_overlay_unsafe(host_.Get());
    if (UObject* overlay = popup_text_overlay_image_.Get()) {
        set_visibility(
            overlay, set_visibility_,
            packaged_popup_ready ? kHitTestInvisible : kCollapsed);
    }
    const std::size_t selected_index = dswros::radar_language_choice_index(
        sanitize_language(pending_language_));
    for (std::size_t index = 0; index < kLanguageChoiceCount; ++index) {
        UObject* control = language_choice_controls_[index].Get();
        UObject* selected = language_choice_selected_visuals_[index].Get();
        UObject* text = language_choice_texts_[index].Get();
        if (!control || !selected || !text) {
            return false;
        }
        set_checked(control, set_is_checked_, false);
        set_visibility(control, set_visibility_, popup_visibility);
        const auto choice = dswros::radar_language_choice(index);
        const bool packaged_choice = choice == dswros::RadarLanguagePreference::Korean
            || choice == dswros::RadarLanguagePreference::TraditionalChinese;
        set_visibility(
            text, set_visibility_,
            visible && (!packaged_popup_ready || !packaged_choice)
                ? kHitTestInvisible : kCollapsed);
        set_visibility(
            selected, set_visibility_,
            visible && index == selected_index ? kVisible : kCollapsed);
    }
    language_dropdown_expanded_ = visible;
    return !visible || recenter_native_text_unsafe();
}

RadarVisibilityHubResult RadarVisibilityHub::service_open_panel(
    UObject* current_controller,
    RadarModStatus current_mod_status) noexcept {
    if (state_ != RadarVisibilityHubState::Open) {
        return {
            RadarVisibilityHubAction::None,
            source_masks_,
            false,
            0,
            source_area_quest_mode_,
            source_assault_mode_,
            source_height_indicators_,
            source_language_};
    }
    if (!current_controller) {
        return {
            RadarVisibilityHubAction::None,
            pending_masks_,
            false,
            0,
            pending_area_quest_mode_,
            pending_assault_mode_,
            pending_height_indicators_,
            pending_language_};
    }
    return service_guarded(current_controller, current_mod_status);
}

RadarVisibilityHubResult RadarVisibilityHub::service_guarded(
    UObject* current_controller,
    RadarModStatus current_mod_status) noexcept {
#if defined(_MSC_VER)
    __try {
        return service_unsafe(current_controller, current_mod_status);
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
        return service_unsafe(current_controller, current_mod_status);
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
    UObject* current_controller,
    RadarModStatus current_mod_status) {
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
        const AreaQuestDisplayMode current_area_mode =
            source_area_quest_mode_;
        const AssaultDisplayMode current_assault_mode =
            source_assault_mode_;
        const dswros::HeightIndicatorMask current_height_indicators =
            source_height_indicators_;
        const dswros::RadarLanguagePreference current_language =
            source_language_;
        detach_unsafe(current_controller);
        ++close_count_;
        return {
            RadarVisibilityHubAction::Closed,
            current,
            false,
            0,
            current_area_mode,
            current_assault_mode,
            current_height_indicators,
            current_language};
    }

    UObject* language_dropdown = language_dropdown_control_.Get();
    UObject* language_popup_dismiss =
        language_popup_dismiss_control_.Get();
    if (!language_dropdown || !language_popup_dismiss) {
        last_failure_ = 33;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    if (is_checked(language_dropdown, is_checked_)) {
        set_checked(language_dropdown, set_is_checked_, false);
        if (!set_language_popup_visibility_unsafe(
                !language_dropdown_expanded_)) {
            last_failure_ = 33;
            detach_unsafe(current_controller);
            return {
                RadarVisibilityHubAction::Rejected,
                source_masks_,
                false,
                last_failure_};
        }
    }

    if (current_mod_status != displayed_mod_status_
        && !refresh_mod_status_unsafe(current_mod_status)) {
        last_failure_ = 36;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    if (language_dropdown_expanded_) {
        if (is_checked(language_popup_dismiss, is_checked_)) {
            set_checked(language_popup_dismiss, set_is_checked_, false);
            if (!set_language_popup_visibility_unsafe(false)) {
                last_failure_ = 33;
                detach_unsafe(current_controller);
                return {
                    RadarVisibilityHubAction::Rejected,
                    source_masks_,
                    false,
                    last_failure_};
            }
        } else {
            for (std::size_t index = 0;
                 index < kLanguageChoiceCount; ++index) {
                UObject* choice = language_choice_controls_[index].Get();
                if (!choice) {
                    last_failure_ = 33;
                    detach_unsafe(current_controller);
                    return {
                        RadarVisibilityHubAction::Rejected,
                        source_masks_,
                        false,
                        last_failure_};
                }
                if (!is_checked(choice, is_checked_)) {
                    continue;
                }
                set_checked(choice, set_is_checked_, false);
                pending_language_ = dswros::radar_language_choice(index);
                resolved_ui_language_ = dswros::resolve_radar_ui_language(
                    pending_language_, detected_game_language_);
                if (!refresh_localized_text_unsafe()
                    || !set_language_popup_visibility_unsafe(false)) {
                    last_failure_ = 33;
                    detach_unsafe(current_controller);
                    return {
                        RadarVisibilityHubAction::Rejected,
                        source_masks_,
                        false,
                        last_failure_};
                }
                break;
            }
        }
    }

    RadarVisibilityHubCommand command{RadarVisibilityHubCommand::None};
    UObject* mod_action_control = mod_action_control_.Get();
    UObject* bug_report_control = bug_report_control_.Get();
    if (!mod_action_control || !bug_report_control) {
        last_failure_ = 36;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    if (is_checked(mod_action_control, is_checked_)) {
        set_checked(mod_action_control, set_is_checked_, false);
        command = radar_mod_action_for_status(displayed_mod_status_);
    }
    if (is_checked(bug_report_control, is_checked_)) {
        set_checked(bug_report_control, set_is_checked_, false);
        command = RadarVisibilityHubCommand::OpenBugReport;
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

    dswros::HeightIndicatorMask height_indicators =
        pending_height_indicators_;
    for (std::size_t index = 0; index < kHeightIndicatorCount; ++index) {
        UObject* control = height_controls_[index].Get();
        UObject* visual = height_enabled_visuals_[index].Get();
        if (!control || !visual) {
            last_failure_ = 34;
            detach_unsafe(current_controller);
            return {
                RadarVisibilityHubAction::Rejected,
                source_masks_,
                false,
                last_failure_};
        }
        const bool enabled = is_checked(control, is_checked_);
        const auto category = static_cast<dswros::HeightIndicatorCategory>(
            index);
        const dswros::HeightIndicatorMask bit =
            dswros::height_indicator_bit(category);
        const bool was_enabled = (height_indicators & bit) != 0U;
        if (enabled) {
            height_indicators = static_cast<dswros::HeightIndicatorMask>(
                height_indicators | bit);
        } else {
            height_indicators = static_cast<dswros::HeightIndicatorMask>(
                height_indicators & ~bit);
        }
        if (enabled != was_enabled) {
            set_visibility(
                visual, set_visibility_,
                enabled ? kVisible : kCollapsed);
        }
    }
    pending_height_indicators_ =
        sanitize_height_indicators(height_indicators);

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
        && pending_assault_mode_ == source_assault_mode_
        && pending_height_indicators_ == source_height_indicators_
        && pending_language_ == source_language_) {
        return {
            RadarVisibilityHubAction::None,
            pending_masks_,
            false,
            0,
            pending_area_quest_mode_,
            pending_assault_mode_,
            pending_height_indicators_,
            pending_language_,
            command};
    }
    const RadarVisibilityMaskWord applied = pending_masks_;
    const AreaQuestDisplayMode applied_mode = pending_area_quest_mode_;
    const AssaultDisplayMode applied_assault_mode = pending_assault_mode_;
    const dswros::HeightIndicatorMask applied_height_indicators =
        pending_height_indicators_;
    const dswros::RadarLanguagePreference applied_language =
        pending_language_;
    source_masks_ = applied;
    source_area_quest_mode_ = applied_mode;
    source_assault_mode_ = applied_assault_mode;
    source_height_indicators_ = applied_height_indicators;
    source_language_ = applied_language;
    ++apply_count_;
    return {
        RadarVisibilityHubAction::Applied,
        applied,
        true,
        0,
        applied_mode,
        applied_assault_mode,
        applied_height_indicators,
        applied_language,
        command};
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
    main_text_overlay_image_ = FWeakObjectPtr{};
    popup_text_overlay_image_ = FWeakObjectPtr{};
    main_text_overlay_texture_ = FWeakObjectPtr{};
    popup_text_overlay_texture_ = FWeakObjectPtr{};
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
    for (auto& control : height_controls_) {
        control = FWeakObjectPtr{};
    }
    for (auto& visual : height_enabled_visuals_) {
        visual = FWeakObjectPtr{};
    }
    language_dropdown_control_ = FWeakObjectPtr{};
    language_popup_dismiss_control_ = FWeakObjectPtr{};
    for (auto& decoration : language_popup_decorations_) {
        decoration = FWeakObjectPtr{};
    }
    for (auto& control : language_choice_controls_) {
        control = FWeakObjectPtr{};
    }
    for (auto& visual : language_choice_selected_visuals_) {
        visual = FWeakObjectPtr{};
    }
    for (auto& text : language_choice_texts_) {
        text = FWeakObjectPtr{};
    }
    for (auto& text : localized_texts_) {
        text = FWeakObjectPtr{};
    }
    for (auto& text : category_label_texts_) {
        text = FWeakObjectPtr{};
    }
    for (auto& text : height_label_texts_) {
        text = FWeakObjectPtr{};
    }
    for (auto& record : text_layout_records_) {
        record = TextLayoutRecord{};
    }
    text_layout_record_count_ = 0;
    area_mode_available_control_ = FWeakObjectPtr{};
    area_mode_all_control_ = FWeakObjectPtr{};
    area_mode_available_visual_ = FWeakObjectPtr{};
    area_mode_all_visual_ = FWeakObjectPtr{};
    assault_mode_current_control_ = FWeakObjectPtr{};
    assault_mode_all_control_ = FWeakObjectPtr{};
    assault_mode_current_visual_ = FWeakObjectPtr{};
    assault_mode_all_visual_ = FWeakObjectPtr{};
    mod_status_visual_ = FWeakObjectPtr{};
    mod_action_visual_ = FWeakObjectPtr{};
    mod_action_control_ = FWeakObjectPtr{};
    bug_report_control_ = FWeakObjectPtr{};
    close_control_ = FWeakObjectPtr{};
    text_overlay_language_ = dswros::RadarUiLanguage::Count;
    text_overlay_status_ = RadarModStatus::Off;
    text_overlay_active_ = false;
    language_dropdown_expanded_ = false;
    owns_input_mode_ = false;
    previous_cursor_visible_ = false;
}

} // namespace dsnwr
