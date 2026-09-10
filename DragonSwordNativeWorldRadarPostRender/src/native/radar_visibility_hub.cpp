#include "radar_visibility_hub.hpp"
#include "hub_escape_input.hpp"

#include <dswros/radar_localization.hpp>
#include <dswros/radar_confirmation_localization.hpp>
#include <dswros/radar_guide_localization.hpp>
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
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/NameTypes.hpp>
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
#include <fstream>
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
constexpr double kReferencePanelWidth = 760.0;
constexpr double kGuideContentHeight = 1120.0;
constexpr double kGuideAtlasHeight = 1184.0;
// Every section and modal extent derives from one vertical layout.
constexpr double kCardX = 20.0;
constexpr double kCardWidth = kReferencePanelWidth - 2.0 * kCardX;
constexpr double kContentX = 36.0;
constexpr double kSectionTitleScale = 0.50;
constexpr double kMarkerTop = 98.0;
constexpr double kMarkerHeaderY = kMarkerTop + 10.0;
constexpr double kMarkerAllY = kMarkerTop + 46.0;
constexpr double kMarkerRowStep = 24.0;
constexpr double kMarkerRowsY = kMarkerAllY + kMarkerRowStep;
constexpr double kMarkerBottom = kMarkerRowsY + 7.0 * kMarkerRowStep + 8.0;
constexpr double kSceneTop = kMarkerBottom + 10.0;
constexpr double kSceneTitleY = kSceneTop + 10.0;
constexpr double kSceneChipsY = kSceneTop + 48.0;
constexpr double kSceneChipWidth = 222.0;
constexpr double kSceneChipStep = 234.0;
constexpr double kSceneRangeY = kSceneTop + 84.0;
constexpr double kSceneLimitY = kSceneTop + 116.0;
constexpr double kSceneDistanceTitleY = kSceneTop + 150.0;
constexpr double kSceneDistanceY = kSceneTop + 170.0;
constexpr double kSceneBottom = kSceneTop + 208.0;
constexpr double kLanguageValueX = 126.0;
constexpr double kLanguageValueY = 58.0;
constexpr double kLanguageValueWidth = 220.0;
constexpr double kLanguageValueHeight = 26.0;
// The language grid is independent of the sections below the popup. Its
// selected Border must fit the actual cell, not a vertical section coordinate.
constexpr double kLanguageChoiceWidth = 190.0;
constexpr double kLanguageChoiceHeight = 32.0;
constexpr double kLanguageChoiceColumnStep = 202.0;
constexpr double kLanguageChoiceRowStep = 40.0;
constexpr double kLanguageSelectedInset = 2.0;
constexpr double kLanguageSelectedWidth =
    kLanguageChoiceWidth - 2.0 * kLanguageSelectedInset;
constexpr double kLanguageSelectedHeight =
    kLanguageChoiceHeight - 2.0 * kLanguageSelectedInset;
static_assert(kLanguageSelectedWidth > 0.0 && kLanguageSelectedHeight > 0.0);
static_assert(kLanguageSelectedInset + kLanguageSelectedWidth < kLanguageChoiceWidth);
static_assert(kLanguageSelectedInset + kLanguageSelectedHeight < kLanguageChoiceHeight);
static_assert(kLanguageChoiceWidth < kLanguageChoiceColumnStep);
static_assert(kLanguageChoiceHeight < kLanguageChoiceRowStep);
constexpr double kHeightTop = kSceneBottom + 10.0;
constexpr double kHeightHeaderY = kHeightTop + 10.0;
constexpr double kHeightRowsY = kHeightTop + 44.0;
constexpr double kHeightBottom = kHeightTop + 116.0;
constexpr double kFilterTop = kHeightBottom + 10.0;
constexpr double kFilterHeaderY = kFilterTop + 10.0;
constexpr double kFilterRowsY = kFilterTop + 44.0;
constexpr double kFilterOptionsY = kFilterTop + 72.0;
constexpr double kContentBottom = kFilterTop + 110.0;
constexpr double kFooterTop = kContentBottom + 10.0;
constexpr double kFooterHeight = 50.0;
constexpr double kFooterButtonY = kFooterTop + 10.0;
constexpr double kFooterButtonWidth = 222.0;
constexpr double kFooterButtonStep = 234.0;
constexpr double kFooterBottom = kFooterTop + kFooterHeight;
constexpr double kReferencePanelHeight = kFooterBottom + 8.0;
static_assert(kReferencePanelHeight == 876.0);
static_assert(kContentBottom < kFooterTop && kFooterBottom < kReferencePanelHeight);
static_assert(kContentX + 2.0 * kFooterButtonStep + kFooterButtonWidth < kCardX + kCardWidth);
constexpr double kConfirmationX = 190.0;
constexpr double kConfirmationY = 334.0;
constexpr double kConfirmationWidth = 380.0;
constexpr double kConfirmationHeight = 184.0;
constexpr std::size_t kNumericGlyphCount = 14;
constexpr wchar_t kNumericCharacters[] = L"0123456789 m-v";
constexpr std::array<double, kNumericGlyphCount> kNumericAdvances{{
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 5, 15, 9, 9}};
constexpr std::size_t kConfirmationAtlasTiles =
    dswros::kRadarTooltipCount + dswros::kRadarConfirmationTextCount + kNumericGlyphCount;
static_assert(kConfirmationX + kConfirmationWidth < kReferencePanelWidth);
static_assert(kConfirmationY + kConfirmationHeight < kReferencePanelHeight);

// Cold open/language import edge only. An older tooltip atlas can bind to an
// Image successfully while lacking every confirmation tile; its 18-byte TGA
// header must prove the current layout before any confirmation can enable Yes.
[[nodiscard]] bool valid_confirmation_atlas_file(const std::filesystem::path& path) {
    std::array<std::uint8_t, 18> header{};
    std::ifstream input(path, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(header.data()),
            static_cast<std::streamsize>(header.size()))) return false;
    const unsigned width = header[12] | (static_cast<unsigned>(header[13]) << 8U);
    const unsigned height = header[14] | (static_cast<unsigned>(header[15]) << 8U);
    return header[0] == 0 && header[1] == 0 && (header[2] == 2 || header[2] == 10)
        && header[16] == 32 && (header[17] & 0x0FU) == 8
        && width == 640U && height == kConfirmationAtlasTiles * 144U;
}
[[nodiscard]] bool valid_guide_atlas_file(const std::filesystem::path& path) {
    std::array<std::uint8_t, 18> header{};
    std::ifstream input(path, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(header.data()),
            static_cast<std::streamsize>(header.size()))) return false;
    const unsigned width = header[12] | (static_cast<unsigned>(header[13]) << 8U);
    const unsigned height = header[14] | (static_cast<unsigned>(header[15]) << 8U);
    return header[0] == 0 && header[1] == 0 && (header[2] == 2 || header[2] == 10)
        && header[16] == 32 && (header[17] & 0x0FU) == 8
        && width == 1520U && height == 2368U;
}
static_assert(kMarkerBottom < kSceneTop && kSceneBottom < kHeightTop);
static_assert(kHeightBottom < kFilterTop);
constexpr double kReferenceViewportWidth = 1920.0;
constexpr double kReferenceViewportHeight = 1080.0;
constexpr double kMinimumViewportMargin = 16.0;
constexpr double kTextLineHeightSafety = 1.45;
constexpr double kReferenceHubFontSize = 32.0;
constexpr double kMaximumHubFontSize = 512.0;
constexpr double kFontMetricReadbackTolerance = 0.001;
constexpr std::size_t kFontParameterCapacity = 512;
constexpr std::size_t kMaximumHubTextCount = 80;
constexpr double kTooltipReferenceWidth = 320.0;
constexpr double kTooltipReferenceHeight = 72.0;
constexpr double kChipSkinReferenceWidth = 222.0;
constexpr double kChipSkinReferenceHeight = 30.0;
constexpr double kChipCornerRadius = 10.0;
static_assert(dswros::kRadarTooltipCount == 34U);
static_assert(kConfirmationAtlasTiles == 55U);

constexpr std::array<std::uint8_t, 3> kColumnCategories{{
    kRadarVisibilityAllCategories,
    kRadarVisibilityWorldCategories,
    kRadarVisibilitySceneCategories,
}};

[[nodiscard]] constexpr bool uses_packaged_text_overlay(
    dswros::RadarUiLanguage language) noexcept {
    return static_cast<std::size_t>(language) < dswros::kRadarUiLanguageCount;
}

[[nodiscard]] constexpr const wchar_t* packaged_text_overlay_prefix(
    dswros::RadarUiLanguage language) noexcept {
    constexpr std::array<const wchar_t*, 11> codes{{L"en", L"ja", L"ko",
        L"zh-hans", L"zh-hant", L"fr", L"de", L"es-es", L"ru", L"th", L"pt-br"}};
    const auto index = static_cast<std::size_t>(language);
    return index < codes.size() ? codes[index] : nullptr;
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
    86.0F / 255.0F, 105.0F / 255.0F, 125.0F / 255.0F, 0.30F};
constexpr LinearColor kPanelBackground{
    29.0F / 255.0F, 40.0F / 255.0F, 53.0F / 255.0F, 0.79F};
constexpr LinearColor kPanelAccent{
    140.0F / 255.0F, 179.0F / 255.0F, 210.0F / 255.0F, 0.85F};
constexpr LinearColor kHeaderBackground{
    11.0F / 255.0F, 27.0F / 255.0F, 38.0F / 255.0F, 0.99F};
constexpr LinearColor kContentBackground{
    41.0F / 255.0F, 53.0F / 255.0F, 68.0F / 255.0F, 0.20F};
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
    62.0F / 255.0F, 78.0F / 255.0F, 97.0F / 255.0F, 0.85F};
constexpr LinearColor kCompactEnabled{
    169.0F / 255.0F, 215.0F / 255.0F, 232.0F / 255.0F, 0.98F};
constexpr LinearColor kWorldEnabled{
    159.0F / 255.0F, 215.0F / 255.0F, 187.0F / 255.0F, 0.98F};
constexpr LinearColor kCloseButton{
    58.0F / 255.0F, 73.0F / 255.0F, 91.0F / 255.0F, 0.78F};
constexpr LinearColor kLanguageCard{
    18.0F / 255.0F, 43.0F / 255.0F, 56.0F / 255.0F, 0.98F};
constexpr LinearColor kLanguageSelector{
    47.0F / 255.0F, 62.0F / 255.0F, 80.0F / 255.0F, 0.60F};
constexpr LinearColor kPopupDim{
    6.0F / 255.0F, 10.0F / 255.0F, 16.0F / 255.0F, 0.30F};
constexpr LinearColor kPopupBackground{
    30.0F / 255.0F, 42.0F / 255.0F, 57.0F / 255.0F, 0.88F};
constexpr LinearColor kLanguageOption{
    47.0F / 255.0F, 62.0F / 255.0F, 80.0F / 255.0F, 0.65F};
constexpr LinearColor kLanguageSelected{
    40.0F / 255.0F, 78.0F / 255.0F, 95.0F / 255.0F, 0.86F};
constexpr LinearColor kModeOption{
    47.0F / 255.0F, 62.0F / 255.0F, 80.0F / 255.0F, 0.65F};
constexpr LinearColor kModeSelectedAvailable{
    40.0F / 255.0F, 78.0F / 255.0F, 95.0F / 255.0F, 0.86F};
constexpr LinearColor kModeSelectedAll{
    40.0F / 255.0F, 78.0F / 255.0F, 95.0F / 255.0F, 0.86F};
constexpr LinearColor kStatusCard{
    14.0F / 255.0F, 33.0F / 255.0F, 44.0F / 255.0F, 0.90F};
constexpr LinearColor kStatusOff{
    139.0F / 255.0F, 116.0F / 255.0F, 71.0F / 255.0F, 0.94F};
constexpr LinearColor kStatusOn{
    45.0F / 255.0F, 143.0F / 255.0F, 100.0F / 255.0F, 0.94F};
constexpr LinearColor kStatusFault{
    150.0F / 255.0F, 58.0F / 255.0F, 67.0F / 255.0F, 0.96F};
constexpr LinearColor kActionButton{
    34.0F / 255.0F, 77.0F / 255.0F, 96.0F / 255.0F, 0.92F};
constexpr LinearColor kBugReportButton{
    58.0F / 255.0F, 73.0F / 255.0F, 91.0F / 255.0F, 0.78F};

struct RowDefinition {
    RadarVisibilityCategory category{RadarVisibilityCategory::Clock};
};

constexpr std::array<RowDefinition, 7> kRows{{
    {RadarVisibilityCategory::Treasure},
    {RadarVisibilityCategory::Boss},
    {RadarVisibilityCategory::Assault},
    {RadarVisibilityCategory::MiniGames},
    {RadarVisibilityCategory::AreaQuests},
    {RadarVisibilityCategory::BirdEggs},
    {RadarVisibilityCategory::Clock},
}};

[[nodiscard]] constexpr dswros::RadarTooltipId marker_tooltip_for(
    RadarVisibilityCategory category) noexcept {
    switch (category) {
    case RadarVisibilityCategory::Treasure: return dswros::RadarTooltipId::Treasure;
    case RadarVisibilityCategory::Boss: return dswros::RadarTooltipId::Boss;
    case RadarVisibilityCategory::Assault: return dswros::RadarTooltipId::Assault;
    case RadarVisibilityCategory::MiniGames: return dswros::RadarTooltipId::MiniGames;
    case RadarVisibilityCategory::AreaQuests: return dswros::RadarTooltipId::AreaQuests;
    case RadarVisibilityCategory::BirdEggs: return dswros::RadarTooltipId::BirdEggs;
    case RadarVisibilityCategory::Clock: return dswros::RadarTooltipId::Clock;
    default: return dswros::RadarTooltipId::Count;
    }
}


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
    // Donor font metrics establish writability only. Every language and
    // fallback uses the same authored 32-unit type scale as the fixed atlas.
    if (source_size < 0 || source_size > kMaximumHubFontSize) {
        return false;
    }
    const double scaled_size =
        kReferenceHubFontSize * unit_scale * role_scale;
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

[[nodiscard]] LinearColor decode_ui_srgb(const LinearColor& color) noexcept {
    // Authored UI colors are sRGB. Passing byte/255 directly as FLinearColor
    // made the old near-black panel visibly gray after the display transform.
    // Imported sRGB textures bypass this path and must not be decoded twice.
    const auto decode_srgb = [](float value) noexcept {
        return value <= 0.04045F ? value / 12.92F
            : std::pow((value + 0.055F) / 1.055F, 2.4F);
    };
    return {decode_srgb(color.red), decode_srgb(color.green),
            decode_srgb(color.blue), color.alpha};
}

void set_brush_color(
    UObject* border, UFunction* function, const LinearColor& color) {
    BrushColorParameters parameters{decode_ui_srgb(color)};
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
        world_radar_visibility_mask(masks),
        scene_radar_visibility_mask(masks));
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
    slider_class_ = find<UClass>(L"/Script/UMG.Slider");
    set_slider_value_ = find<UFunction>(L"/Script/UMG.Slider:SetValue");
    get_slider_value_ = find<UFunction>(L"/Script/UMG.Slider:GetValue");
    set_slider_min_ = find<UFunction>(L"/Script/UMG.Slider:SetMinValue");
    set_slider_max_ = find<UFunction>(L"/Script/UMG.Slider:SetMaxValue");
    set_slider_step_ = find<UFunction>(L"/Script/UMG.Slider:SetStepSize");
    set_slider_bar_color_ = find<UFunction>(L"/Script/UMG.Slider:SetSliderBarColor");
    set_slider_handle_color_ = find<UFunction>(L"/Script/UMG.Slider:SetSliderHandleColor");
    for (auto** optional_color : {&set_slider_bar_color_, &set_slider_handle_color_}) {
        if (*optional_color && (*optional_color)->GetParmsSize() != 16) {
            *optional_color = nullptr;
        }
    }
    image_class_ = find<UClass>(L"/Script/UMG.Image");
    size_box_class_ = find<UClass>(L"/Script/UMG.SizeBox");
    scroll_box_class_ = find<UClass>(L"/Script/UMG.ScrollBox");
    add_child_to_panel_ = find<UFunction>(L"/Script/UMG.PanelWidget:AddChild");
    set_scroll_offset_ = find<UFunction>(L"/Script/UMG.ScrollBox:SetScrollOffset");
    get_scroll_offset_ = find<UFunction>(L"/Script/UMG.ScrollBox:GetScrollOffset");
    set_scroll_orientation_ = find<UFunction>(L"/Script/UMG.ScrollBox:SetOrientation");
    set_scrollbar_visibility_ = find<UFunction>(L"/Script/UMG.ScrollBox:SetScrollBarVisibility");
    set_tool_tip_ = find<UFunction>(L"/Script/UMG.Widget:SetToolTip");
    set_tool_tip_text_ = find<UFunction>(L"/Script/UMG.Widget:SetToolTipText");
    tool_tip_text_property_ = CastField<FTextProperty>(
        find_function_property(set_tool_tip_text_, L"InToolTipText"));
    if (!set_tool_tip_text_ || set_tool_tip_text_->GetParmsSize() != 24
        || !tool_tip_text_property_ || tool_tip_text_property_->GetOffset_Internal() != 0
        || tool_tip_text_property_->GetSize() != 24) {
        set_tool_tip_text_ = nullptr;
        tool_tip_text_property_ = nullptr;
    }
    set_content_ = find<UFunction>(L"/Script/UMG.ContentWidget:SetContent");
    set_width_override_ = find<UFunction>(L"/Script/UMG.SizeBox:SetWidthOverride");
    set_height_override_ = find<UFunction>(L"/Script/UMG.SizeBox:SetHeightOverride");
    set_clipping_ = find<UFunction>(L"/Script/UMG.Widget:SetClipping");
    const auto exact_parameter = [](UFunction* function, const wchar_t* name,
        std::int32_t offset, std::int32_t size, std::int32_t total) {
        FProperty* property = find_function_property(function, name);
        return function && function->GetParmsSize() == total && property
            && property->GetOffset_Internal() == offset && property->GetSize() == size;
    };
    tooltip_widget_abi_available_ = size_box_class_
        && exact_parameter(set_tool_tip_, L"Widget", 0, 8, 8)
        && CastField<FObjectPropertyBase>(find_function_property(set_tool_tip_, L"Widget"))
        && exact_parameter(set_content_, L"Content", 0, 8, 16)
        && CastField<FObjectPropertyBase>(find_function_property(set_content_, L"Content"))
        && exact_parameter(set_content_, L"ReturnValue", 8, 8, 16)
        && CastField<FObjectPropertyBase>(find_function_property(set_content_, L"ReturnValue"))
        && exact_parameter(set_width_override_, L"InWidthOverride", 0, 4, 4)
        && CastField<FFloatProperty>(find_function_property(set_width_override_, L"InWidthOverride"))
        && exact_parameter(set_height_override_, L"InHeightOverride", 0, 4, 4)
        && CastField<FFloatProperty>(find_function_property(set_height_override_, L"InHeightOverride"))
        && exact_parameter(set_clipping_, L"InClipping", 0, 1, 1);
    set_image_brush_ = find<UFunction>(L"/Script/UMG.Image:SetBrush");
    image_brush_property_ = CastField<FStructProperty>(find_reflected_property(image_class_, L"Brush"));
    set_image_brush_value_property_ = CastField<FStructProperty>(find_function_property(set_image_brush_, L"InBrush"));
    UScriptStruct* brush_struct = image_brush_property_ ? image_brush_property_->GetStruct().Get() : nullptr;
    brush_margin_property_ = CastField<FStructProperty>(find_reflected_property(brush_struct, L"Margin"));
    brush_image_size_property_ = CastField<FStructProperty>(find_reflected_property(brush_struct, L"ImageSize"));
    brush_draw_as_property_ = find_reflected_property(brush_struct, L"DrawAs");
    chip_nine_slice_abi_available_ = set_image_brush_ && image_brush_property_
        && set_image_brush_value_property_ && brush_struct
        && set_image_brush_value_property_->GetStruct().Get() == brush_struct
        && set_image_brush_value_property_->GetOffset_Internal() == 0
        && set_image_brush_value_property_->GetSize() == set_image_brush_->GetParmsSize()
        && set_image_brush_->GetParmsSize() > 0
        && set_image_brush_->GetParmsSize() <= static_cast<std::int32_t>(kFontParameterCapacity)
        && brush_margin_property_ && brush_margin_property_->IsInContainer(brush_struct)
        && brush_image_size_property_ && brush_image_size_property_->IsInContainer(brush_struct)
        && CastField<FNumericProperty>(brush_draw_as_property_)
        && brush_draw_as_property_->IsInContainer(brush_struct)
        && brush_draw_as_property_->GetSize() == 1;
    constexpr std::array<const wchar_t*, 6> box_names{{L"Left", L"Top", L"Right", L"Bottom", L"X", L"Y"}};
    for (std::size_t index = 0; index < box_names.size(); ++index) {
        UScriptStruct* owner = index < 4U
            ? (brush_margin_property_ ? brush_margin_property_->GetStruct().Get() : nullptr)
            : (brush_image_size_property_ ? brush_image_size_property_->GetStruct().Get() : nullptr);
        auto* metric = CastField<FNumericProperty>(find_reflected_property(owner, box_names[index]));
        brush_box_metrics_[index] = metric;
        chip_nine_slice_abi_available_ = chip_nine_slice_abi_available_ && metric
            && metric->IsFloatingPoint() && metric->IsInContainer(owner)
            && (metric->GetSize() == 4 || metric->GetSize() == 8);
    }
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
    set_is_enabled_ = find<UFunction>(L"/Script/UMG.Widget:SetIsEnabled");
    set_is_enabled_value_property_ = nullptr;
    if (set_is_enabled_) {
        std::size_t parameters{};
        for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(set_is_enabled_)) {
            if (!property->HasAnyPropertyFlags(CPF_Parm)) continue;
            ++parameters;
            if (!property->HasAnyPropertyFlags(CPF_ReturnParm)
                && CastField<FBoolProperty>(property)
                && property->GetOffset_Internal() == 0 && property->GetSize() == 1)
                set_is_enabled_value_property_ = property;
        }
        if (parameters != 1) set_is_enabled_value_property_ = nullptr;
    }
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
    // Background keyboard/mouse input must be disabled before exposing a modal.
    require_parameters(1U << 29U, set_is_enabled_, 1);
    if (!set_is_enabled_value_property_) abi_failure_mask_ |= 1U << 29U;
    // A real vertical ScrollBox preserves readable controls on short screens.
    // Validate every new reflected parameter before constructing the tree.
    bool scroll_abi = scroll_box_class_ && tooltip_widget_abi_available_
        && set_render_scale_ && set_render_pivot_
        && exact_parameter(add_child_to_panel_, L"Content", 0, 8, 16)
        && CastField<FObjectPropertyBase>(find_function_property(add_child_to_panel_, L"Content"))
        && exact_parameter(add_child_to_panel_, L"ReturnValue", 8, 8, 16)
        && CastField<FObjectPropertyBase>(find_function_property(add_child_to_panel_, L"ReturnValue"))
        && exact_parameter(set_scroll_offset_, L"NewScrollOffset", 0, 4, 4)
        && CastField<FFloatProperty>(find_function_property(set_scroll_offset_, L"NewScrollOffset"))
        && exact_parameter(get_scroll_offset_, L"ReturnValue", 0, 4, 4)
        && CastField<FFloatProperty>(find_function_property(get_scroll_offset_, L"ReturnValue"));
    // These are UMG UFunction parameter names, not the similarly named Slate setters.
    for (const auto& entry : {std::pair{set_scroll_orientation_, L"NewOrientation"}}) {
        FProperty* value = find_function_property(entry.first, entry.second);
        scroll_abi = scroll_abi && exact_parameter(entry.first, entry.second, 0, 1, 1)
            && (CastField<FByteProperty>(value) || CastField<FEnumProperty>(value));
    }
    auto* viewport_vector = CastField<FStructProperty>(find_function_property(get_viewport_size_, L"ReturnValue"));
    for (const auto& entry : {std::pair{set_render_scale_, L"Scale"},
                              std::pair{set_render_pivot_, L"Pivot"}}) {
        auto* vector = CastField<FStructProperty>(find_function_property(entry.first, entry.second));
        scroll_abi = scroll_abi && exact_parameter(entry.first, entry.second, 0, 16, 16)
            && vector && viewport_vector && vector->GetStruct()
            && vector->GetStruct() == viewport_vector->GetStruct();
    }
    if (!scroll_abi) abi_failure_mask_ |= 1U << 30U;
    // The shipped UE 5.3 game does not reflect SetScrollBarVisibility.
    // This is optional appearance: keep ScrollBox's default when unavailable.
    FProperty* scrollbar_visibility = find_function_property(
        set_scrollbar_visibility_, L"NewScrollBarVisibility");
    if (!exact_parameter(set_scrollbar_visibility_, L"NewScrollBarVisibility", 0, 1, 1)
        || !(CastField<FByteProperty>(scrollbar_visibility)
            || CastField<FEnumProperty>(scrollbar_visibility))) {
        set_scrollbar_visibility_ = nullptr;
    }
    // Sliders use reflected float parameters, never guessed object offsets.
    for (auto* function : {set_slider_value_, set_slider_min_,
                           set_slider_max_, set_slider_step_, get_slider_value_}) {
        require_parameters(1U << 28U, function, 4);
        auto* value = CastField<FFloatProperty>(find_function_property(
            function, function == get_slider_value_ ? L"ReturnValue" : L"InValue"));
        if (!value || value->GetOffset_Internal() != 0 || value->GetSize() != 4) {
            abi_failure_mask_ |= 1U << 28U;
        }
    }
    if (!slider_class_) abi_failure_mask_ |= 1U << 28U;
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

    // Native fallback uses the standard TextBlock style, not a donor's bold,
    // outline, material or tracking. The verified language font supplies glyph
    // coverage; packaged text uses pinned regular fonts at fixed reference size.
    set_font_value_property_->CopyCompleteValue(parameter_font, live_font);
    UObject* base_default = text_block_class_ ? text_block_class_->GetClassDefaultObject().Get() : nullptr;
    FStructProperty* base_font = compatible_font_property(base_default, text_block_class_, expected_font_property);
    if (base_font && base_font->GetStruct() == live_font_property->GetStruct())
        set_font_value_property_->CopyCompleteValue(parameter_font,
            base_font->ContainerPtrToValuePtr<void>(base_default));
    UScriptStruct* font_struct = live_font_property->GetStruct().Get();
    auto* face = CastField<FNameProperty>(find_reflected_property(font_struct, L"TypefaceFontName"));
    if (face && face->IsInContainer(font_struct) && face->GetSize() == sizeof(FName)) {
        const FName regular(L"Regular", FNAME_Add);
        face->CopyCompleteValue(face->ContainerPtrToValuePtr<void>(parameter_font), &regular);
    }
    if (auto* spacing = compatible_font_metric(live_font_property, L"LetterSpacing"))
        (void)write_font_metric(spacing, spacing->ContainerPtrToValuePtr<void>(parameter_font), 0);
    // Preserve the already fixed layout size across a locale change.
    if (auto* size = compatible_font_metric(live_font_property, L"Size"))
        size->CopyCompleteValue(size->ContainerPtrToValuePtr<void>(parameter_font),
            size->ContainerPtrToValuePtr<void>(live_font));
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
    RadarModStatus current_mod_status,
    dswros::SceneDisplaySettings current_scene_settings) noexcept {
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
        return close(current_controller, current_mod_status);
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
        detected_game_language, current_mod_status, current_scene_settings);
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
    RadarModStatus current_mod_status,
    dswros::SceneDisplaySettings current_scene_settings) noexcept {
#if defined(_MSC_VER)
    __try {
        RadarVisibilityHubResult result =
            open_unsafe(
                current_controller, current_masks,
                current_area_quest_mode, current_assault_mode,
                current_height_indicators, current_language,
                detected_game_language, current_mod_status, current_scene_settings);
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
                detected_game_language, current_mod_status, current_scene_settings);
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
    RadarModStatus current_mod_status,
    dswros::SceneDisplaySettings current_scene_settings) {
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
    source_scene_settings_ = dswros::normalize_scene_display_settings(current_scene_settings);
    pending_scene_settings_ = source_scene_settings_;
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
    const auto viewport_layout = dswros::compute_hub_viewport_layout(
        viewport_size.return_value.x, viewport_size.return_value.y,
        viewport_scale.return_value);
    if (!viewport_layout.valid) {
        last_failure_ = 4;
        return {
            RadarVisibilityHubAction::Rejected,
            sanitized,
            false,
            last_failure_};
    }
    const double unit_scale = viewport_layout.unit_scale;
    const double display_scale = viewport_layout.display_scale;

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

    const auto add_to_canvas = [this, unit_scale](UObject* parent,
        UObject* widget, double x, double y, double width, double height,
        std::int32_t z_order) -> UObject* {
        if (!widget || !parent) {
            return nullptr;
        }
        AddChildToCanvasParameters add{widget};
        parent->ProcessEvent(add_child_to_canvas_, &add);
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

    UObject* page = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* header = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* body = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* body_stack = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* footer = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* modal = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* scroll = UObjectGlobals::NewObject<UObject>(tree, scroll_box_class_);
    UObject* body_size = UObjectGlobals::NewObject<UObject>(tree, size_box_class_);
    if (!page || !header || !body || !body_stack || !footer || !modal || !scroll || !body_size) {
        last_failure_ = 43;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    page_panel_ = page;
    page_slot_ = add_to_canvas(root, page, 0, 0, kReferencePanelWidth,
        viewport_layout.panel_reference_height, 0);
    UObject* header_slot = add_to_canvas(page, header, 0, 0, kReferencePanelWidth, kMarkerTop, 0);
    body_scroll_ = scroll;
    body_scroll_slot_ = add_to_canvas(page, scroll, 0, kMarkerTop, kReferencePanelWidth,
        viewport_layout.body_viewport_reference_height, 1);
    footer_slot_ = add_to_canvas(page, footer, 0, viewport_layout.footer_reference_y,
        kReferencePanelWidth, kReferencePanelHeight - kFooterTop, 2);
    modal_slot_ = add_to_canvas(page, modal, 0, viewport_layout.modal_reference_y,
        kReferencePanelWidth, kConfirmationHeight, 81);
    if (!page_slot_.Get() || !header_slot || !body_scroll_slot_.Get()
        || !footer_slot_.Get() || !modal_slot_.Get()) {
        last_failure_ = 43;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    ByteParameters clip{1}; // ClipToBounds: fixed header/footer and independent body viewport.
    for (UObject* clipped : {page, header, footer, scroll}) clipped->ProcessEvent(set_clipping_, &clip);
    // Containers receive input only through their visible children.
    for (UObject* panel : {page, header, body, body_stack, footer, modal})
        set_visibility(panel, set_visibility_, 4); // SelfHitTestInvisible.
    ByteParameters vertical{1}, scrollbar_visible{0};
    scroll->ProcessEvent(set_scroll_orientation_, &vertical);
    if (set_scrollbar_visibility_)
        scroll->ProcessEvent(set_scrollbar_visibility_, &scrollbar_visible);
    ScalarParameters body_width{static_cast<float>(kReferencePanelWidth * unit_scale)};
    // The last ten reference units preserve the authored gap above the footer.
    ScalarParameters body_height{static_cast<float>((kFooterTop - kMarkerTop) * unit_scale)};
    body_size->ProcessEvent(set_width_override_, &body_width);
    body_size->ProcessEvent(set_height_override_, &body_height);
    settings_body_ = body;
    body_size_ = body_size;
    if (!add_to_canvas(body_stack, body, 0, 0, kReferencePanelWidth,
        kFooterTop - kMarkerTop, 0)) {
        last_failure_ = 43;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    AddChildToCanvasParameters body_content{body_stack}, scroll_content{body_size};
    body_size->ProcessEvent(set_content_, &body_content);
    scroll->ProcessEvent(add_child_to_panel_, &scroll_content);
    if (!body_content.return_value || !scroll_content.return_value) {
        last_failure_ = 43;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    authored_unit_scale_ = unit_scale;
    viewport_layout_ = viewport_layout;
    viewport_check_after_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    const auto local_y = [](double y, std::int32_t z) {
        if (z >= 81) return y - kConfirmationY;
        if (z < 29 && y >= kFooterTop) return y - kFooterTop;
        if (z < 29 && y >= kMarkerTop && y < kContentBottom) return y - kMarkerTop;
        return y;
    };
    const auto add_widget = [this, page, header, body, footer, modal,
        &add_to_canvas, &local_y, &viewport_layout](UObject* widget,
        double x, double y, double width, double height, std::int32_t z) -> UObject* {
        UObject* parent = z >= 81 ? modal : z >= 29 ? page
            : y >= kFooterTop ? footer : y >= kMarkerTop && y < kContentBottom ? body : header;
        if (z == 79 || z == 80) height = viewport_layout.panel_reference_height;
        if (z == 29) height = viewport_layout.panel_reference_height - y - 8.0;
        if (z == 30 && y == 90.0) height = viewport_layout.panel_reference_height - y - 8.0;
        UObject* slot = add_to_canvas(parent, widget, x, local_y(y, z), width, height, z);
        if (z == 79) confirmation_dim_slot_ = slot;
        if (z == 80) confirmation_blocker_slot_ = slot;
        if (z == 29) popup_dim_slot_ = slot;
        if (z == 30 && y == 90.0) popup_dismiss_slot_ = slot;
        return slot;
    };
    // Three consumers share one full-page texture. Only the body slice scrolls.
    const auto add_page_images = [this, tree, header, body, footer, &add_to_canvas](
        UObject* texture, std::int32_t z) {
        std::array<UObject*, 3> images{};
        const std::array<UObject*, 3> parents{{header, body, footer}};
        const std::array<double, 3> offsets{{0, -kMarkerTop, -kFooterTop}};
        for (std::size_t index = 0; index < images.size(); ++index) {
            images[index] = UObjectGlobals::NewObject<UObject>(tree, image_class_);
            if (!images[index] || !add_to_canvas(parents[index], images[index], 0, offsets[index],
                kReferencePanelWidth, kReferencePanelHeight, z)
                || (texture && !apply_text_overlay_unsafe(images[index], texture))) return std::array<UObject*, 3>{};
            set_visibility(images[index], set_visibility_, texture ? kHitTestInvisible : kCollapsed);
        }
        return images;
    };

    // Six shared, pre-rasterized skins provide real rounded geometry and
    // highlight/shadow layers. Each file is attempted at most once per open;
    // the Image Brushes own textures, not a global UObject cache.
    constexpr std::array<const wchar_t*, 6> skin_files{{L"main-glass.tga",
        L"popup-glass.tga", L"chip-idle.tga", L"chip-active.tga",
        L"check-idle.tga", L"check-active.tga"}};
    std::array<UObject*, skin_files.size()> skin_textures{};
    std::array<bool, skin_files.size()> skin_attempted{};
    UObject* main_glass{};
    UObject* popup_glass{};
    const auto add_skin = [this, tree, current_controller, unit_scale, &add_widget, &add_page_images,
        &skin_files, &skin_textures, &skin_attempted](std::size_t index,
        double x, double y, double width, double height, std::int32_t z_order) -> UObject* {
        if (!image_class_ || index >= skin_files.size()) return nullptr;
        if (!skin_attempted[index]) {
            skin_attempted[index] = true;
            skin_textures[index] = import_text_overlay_unsafe(
                current_controller, text_overlay_root_ / skin_files[index]);
        }
        if (!skin_textures[index]) return nullptr;
        if (index == 0U) return add_page_images(skin_textures[index], z_order)[0];
        UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (!image || !apply_text_overlay_unsafe(image, skin_textures[index])) return nullptr;
        if ((index == 2U || index == 3U) && !configure_chip_nine_slice_unsafe(image, unit_scale)) return nullptr;
        set_visibility(image, set_visibility_, kHitTestInvisible);
        return add_widget(image, x, y, width, height, z_order) ? image : nullptr;
    };
    const auto add_border = [this, tree, header, body, footer, &add_to_canvas, &add_widget, &add_skin,
                            &main_glass, &popup_glass](
        double x, double y, double width, double height,
        std::int32_t z_order, const LinearColor& color) -> UObject* {
        if (&color == &kPanelFrame && x == 0.0) {
            main_glass = add_skin(0, 0.0, 0.0, kReferencePanelWidth,
                kReferencePanelHeight, 0);
            if (main_glass) return main_glass;
        }
        if (main_glass && (&color == &kPanelBackground || &color == &kPanelAccent
            || &color == &kContentBackground || &color == &kBugReportButton
            || &color == &kCloseButton || &color == &kLanguageSelector)) return main_glass;
        if (&color == &kPopupDim) {
            popup_glass = add_skin(1, 0.0, 0.0, kReferencePanelWidth,
                kReferencePanelHeight, 30);
            if (popup_glass) return popup_glass;
        }
        if (popup_glass && (&color == &kPanelFrame || &color == &kPopupBackground
            || &color == &kLanguageOption)) return popup_glass;
        std::size_t skin_index = 6;
        if (&color == &kModeOption) skin_index = 2;
        else if (&color == &kModeSelectedAvailable || &color == &kModeSelectedAll
            || &color == &kLanguageSelected || &color == &kActionButton) skin_index = 3;
        else if (&color == &kToggleFrame) skin_index = 4;
        else if (&color == &kCompactEnabled || &color == &kWorldEnabled) skin_index = 5;
        if (skin_index < 6) {
            if (UObject* image = add_skin(skin_index, x, y, width, height, z_order)) return image;
        }
        if (z_order < 2 && height > kContentBottom) {
            const std::array<UObject*, 3> parents{{header, body, footer}};
            const std::array<double, 3> offsets{{0, -kMarkerTop, -kFooterTop}};
            UObject* first{};
            for (std::size_t index = 0; index < parents.size(); ++index) {
                UObject* slice = UObjectGlobals::NewObject<UObject>(tree, border_class_);
                if (!slice || !add_to_canvas(parents[index], slice, x, y + offsets[index],
                    width, height, z_order)) return nullptr;
                set_brush_color(slice, set_brush_color_, color);
                set_visibility(slice, set_visibility_, kHitTestInvisible);
                if (!first) first = slice;
            }
            return first;
        }
        // Missing optional skin files retain functional native controls and
        // the correctly decoded dark-color fallback, never a blank settings UI.
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
    const auto add_text = [this, tree, &add_widget, &local_y, unit_scale,
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
            local_y(y, z_order),
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
    const auto add_numeric_glyph = [this, tree, &add_widget, &local_y, unit_scale](
        std::size_t index, double x, double y, std::int32_t z) {
        if (index >= numeric_texts_.size() || !image_class_ || !tooltip_widget_abi_available_) return;
        auto& record = numeric_texts_[index];
        UObject* canvas = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
        UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
        UObject* slot = canvas ? add_widget(canvas, x, y, 16.0, 26.0, z) : nullptr;
        if (!slot || !image) return;
        ByteParameters clipping{1};
        canvas->ProcessEvent(set_clipping_, &clipping);
        AddChildToCanvasParameters child{image};
        canvas->ProcessEvent(add_child_to_canvas_, &child);
        if (!child.return_value) return;
        set_slot_vector(child.return_value, set_slot_alignment_, 0, 0);
        set_slot_vector(child.return_value, set_slot_size_, kTooltipReferenceWidth * unit_scale,
            kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * unit_scale);
        set_visibility(canvas, set_visibility_, kCollapsed);
        set_visibility(image, set_visibility_, kHitTestInvisible);
        record = {canvas, slot, image, child.return_value, x, local_y(y, z)};
    };
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

    std::array<std::array<UObject*, kCategoryCount>, kColumnCount> controls{};
    std::array<std::array<UObject*, kCategoryCount>, kColumnCount> enabled_visuals{};
    std::array<UObject*, 2> column_all_controls{};
    std::array<UObject*, 2> column_all_visuals{};
    std::array<UObject*, kHeightIndicatorCount> height_controls{};
    std::array<UObject*, kHeightIndicatorCount> height_visuals{};
    std::array<UObject*, 2> area_mode_controls{};
    std::array<UObject*, 2> area_mode_visuals{};
    std::array<UObject*, 2> assault_mode_controls{};
    std::array<UObject*, 2> assault_mode_visuals{};
    const auto add_control = [&add_widget, tree, this](
        double x, double y, double width, double height,
        bool checked, std::int32_t z_order = 12) -> UObject* {
        UObject* control = UObjectGlobals::NewObject<UObject>(tree, check_box_class_);
        if (!control) return nullptr;
        set_checked(control, set_is_checked_, checked);
        set_render_opacity(control, set_render_opacity_, 0.01F);
        return add_widget(control, x, y, width, height, z_order) ? control : nullptr;
    };
    // One background per module, generous gaps and high-contrast headings.
    // There is no enclosing table frame or per-row grid of decorative borders.
    if (!add_border(0.0, 0.0, kReferencePanelWidth, kReferencePanelHeight, 0, kPanelFrame)
        || !add_border(1.0, 1.0, kReferencePanelWidth - 2.0,
                       kReferencePanelHeight - 2.0, 1, kPanelBackground)
        || !add_border(24.0, 46.0, 44.0, 2.0, 2, kPanelAccent)
        || !add_localized_text(LocalizedTextSlot::Title, localized.title,
            24.0, 12.0, 610.0, 36.0, 5, 0.6875)
        || !add_border(kCardX, kFooterTop, kCardWidth, kFooterHeight, 3, kContentBackground)
        || !add_border(kContentX, kFooterButtonY, kFooterButtonWidth, 30.0, 19, kBugReportButton)
        || !add_localized_text(LocalizedTextSlot::GlobalReset, localized.restore_defaults,
            44.0, 831.0, 206.0, 24.0, 21, 0.40625, kTextCenter)
        || !add_border(kContentX + kFooterButtonStep, kFooterButtonY,
            kFooterButtonWidth, 30.0, 19, kBugReportButton)
        || !add_border(kContentX + 2.0 * kFooterButtonStep, kFooterButtonY,
            kFooterButtonWidth, 30.0, 19, kBugReportButton)
        || !add_localized_text(LocalizedTextSlot::BugReport, localized.bug_report,
            512.0, 831.0, 206.0, 24.0, 21, 0.40625, kTextCenter)
        || !add_border(652.0, 14.0, 88.0, 30.0, 19, kCloseButton)
        || !add_localized_text(LocalizedTextSlot::Close, localized.close,
            658.0, 17.0, 76.0, 24.0, 21, 0.40625, kTextCenter)) {
        last_failure_ = 8;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    global_reset_control_ = add_control(kContentX, kFooterButtonY,
        kFooterButtonWidth, 30.0, false, 22);
    endorsement_control_ = add_control(kContentX + kFooterButtonStep,
        kFooterButtonY, kFooterButtonWidth, 30.0, false, 22);
    UObject* bug_report_control = add_control(kContentX + 2.0 * kFooterButtonStep,
        kFooterButtonY, kFooterButtonWidth, 30.0, false, 22);
    UObject* close_control = add_control(652.0, 12.0, 88.0, 34.0, false, 22);
    // The guide is a second body page in the existing ScrollBox. Its optional
    // artwork must never make the settings menu fail to open.
    (void)add_skin(2U, 550.0, 14.0, 92.0, 30.0, 19);
    guide_control_ = add_control(550.0, 12.0, 92.0, 34.0, false, 22);
    guide_fallback_text_ = add_text(L"Guide", 550.0, 17.0, 92.0, 24.0,
        21, 0.40625, kTextCenter);
    if (image_class_) {
        UObject* guide = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
        if (guide && add_to_canvas(body_stack, guide, 0, 0, kReferencePanelWidth,
                kGuideContentHeight, 1)) {
            guide_body_ = guide;
            guide->ProcessEvent(set_clipping_, &clip);
            set_visibility(guide, set_visibility_, kCollapsed);
            for (std::size_t index = 0; index < guide_images_.size(); ++index) {
                UObject* parent = guide;
                if (index < 2U) {
                    parent = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
                    if (!parent || !add_to_canvas(header, parent, 550, 17, 92, 24, 23)) continue;
                    parent->ProcessEvent(set_clipping_, &clip);
                    set_visibility(parent, set_visibility_, kCollapsed);
                    guide_label_canvases_[index] = parent;
                }
                UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
                const double atlas_y = index < 2U ? -24.0 * static_cast<double>(index) : -64.0;
                if (image && add_to_canvas(parent, image, 0, atlas_y,
                        kReferencePanelWidth, kGuideAtlasHeight, 0)) {
                    set_visibility(image, set_visibility_, kHitTestInvisible);
                    guide_images_[index] = image;
                }
            }
        }
    }
    if (!global_reset_control_.Get() || !endorsement_control_.Get()
        || !bug_report_control || !close_control) {
        last_failure_ = 13;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }

    // Language and status share a compact utility row below the title.
    placeholder_texts_[2] = add_text(L"v", 346.0, 59.0, 14.0, 24.0, 7, 0.40, kTextCenter);
    add_numeric_glyph(14, 345.0, 58.0, 8);
    if (!add_border(24.0, 54.0, 344.0, 34.0, 3, kLanguageSelector)
        || !add_localized_text(LocalizedTextSlot::Language, localized.language,
            36.0, 59.0, 84.0, 24.0, 6, 0.375)
        || !add_localized_text(LocalizedTextSlot::LanguageValue, language_display.data(),
            kLanguageValueX, kLanguageValueY, kLanguageValueWidth,
            kLanguageValueHeight, 6, 0.4375, kTextCenter)
        || !placeholder_texts_[2].Get()) {
        last_failure_ = 31;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    UObject* language_dropdown_control = add_control(24.0, 54.0, 344.0, 34.0, false);
    UObject* mod_status_visual = add_border(483.0, 63.0, 4.0, 16.0, 6, status_color);
    UObject* mod_action_visual = add_border(606.0, 56.0, 130.0, 30.0, 6, kActionButton);
    UObject* mod_action_control = add_control(604.0, 54.0, 134.0, 34.0, false);
    if (!language_dropdown_control || !mod_status_visual || !mod_action_visual
        || !mod_action_control
        || !add_localized_text(LocalizedTextSlot::StatusLabel, localized.status,
            388.0, 59.0, 90.0, 24.0, 7, 0.375)
        || !add_localized_text(LocalizedTextSlot::StatusValue, status_value,
            493.0, 59.0, 102.0, 24.0, 7, 0.40625, kTextCenter)
        || !add_localized_text(LocalizedTextSlot::StatusAction, status_action,
            612.0, 59.0, 118.0, 24.0, 7, 0.40625, kTextCenter)) {
        last_failure_ = 36;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }

    std::array<UObject*, kLanguagePopupDecorationCount> language_popup_decorations{};
    std::array<UObject*, kLanguageChoiceCount> language_choice_controls{};
    std::array<UObject*, kLanguageChoiceCount> language_choice_selected_visuals{};
    std::array<UObject*, kLanguageChoiceCount> language_choice_texts{};
    std::size_t popup_decoration_index{};
    const auto add_popup_decoration =
        [&language_popup_decorations, &popup_decoration_index](UObject* widget) -> bool {
        if (!widget || popup_decoration_index >= language_popup_decorations.size())
            return false;
        language_popup_decorations[popup_decoration_index++] = widget;
        return true;
    };
    if (!add_popup_decoration(add_border(kCardX, 92.0, kCardWidth,
            kFooterBottom - 92.0, 29, kPopupDim))
        || !add_popup_decoration(add_border(66.0, 90.0, 628.0, 178.0, 31, kPanelFrame))
        || !add_popup_decoration(add_border(68.0, 92.0, 624.0, 174.0, 32, kPopupBackground))) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    UObject* language_popup_dismiss_control = add_control(
        kCardX, 90.0, kCardWidth, kFooterBottom - 90.0, false, 30);
    if (!language_popup_dismiss_control) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    for (std::size_t index = 0; index < kLanguageChoiceCount; ++index) {
        const std::size_t column = index % 3U;
        const std::size_t row = index / 3U;
        const double x = 78.0 + static_cast<double>(column) * kLanguageChoiceColumnStep;
        const double y = 101.0 + static_cast<double>(row) * kLanguageChoiceRowStep;
        const auto choice = dswros::radar_language_choice(index);
        const bool follow_game = choice == dswros::RadarLanguagePreference::Auto;
        const auto choice_language = dswros::explicit_radar_ui_language(choice);
        const wchar_t* choice_label = follow_game ? L"Use game language"
            : dswros::radar_localized_text(choice_language).language_name;
        if (!add_popup_decoration(add_border(
                x, y, kLanguageChoiceWidth, kLanguageChoiceHeight, 33, kLanguageOption))) {
            last_failure_ = 35;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        language_choice_selected_visuals[index] = add_border(
            x + kLanguageSelectedInset, y + kLanguageSelectedInset,
            kLanguageSelectedWidth, kLanguageSelectedHeight, 34, kLanguageSelected);
        language_choice_texts[index] = add_text(
            choice_label, x + 5.0, y + 4.0, 180.0, 24.0, 35, 0.40625,
            kTextCenter, choice_language);
        language_choice_controls[index] = add_control(
            x, y, kLanguageChoiceWidth, kLanguageChoiceHeight, false, 40);
        if (!language_choice_selected_visuals[index] || !language_choice_texts[index]
            || !language_choice_controls[index]) {
            last_failure_ = 35;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
    }
    if (popup_decoration_index != language_popup_decorations.size()) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }

    if (!add_border(kCardX, kMarkerTop, kCardWidth, kMarkerBottom - kMarkerTop,
                    3, kContentBackground)
        || !add_border(kCardX, kMarkerHeaderY + 4.0, 3.0, 20.0, 4, kPanelAccent)
        || !add_localized_text(LocalizedTextSlot::MarkerVisibility,
            localized.marker_visibility, kContentX, kMarkerHeaderY,
            380.0, 28.0, 5, kSectionTitleScale)
        || !add_localized_text(LocalizedTextSlot::Radar, localized.radar,
            488.0, kMarkerHeaderY + 2.0, 100.0, 26.0, 5, 0.40625, kTextCenter)
        || !add_localized_text(LocalizedTextSlot::Map, localized.map,
            614.0, kMarkerHeaderY + 2.0, 100.0, 26.0, 5, 0.40625, kTextCenter)) {
        last_failure_ = 8;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    const std::array<double, 2> control_x{{516.0, 642.0}};
    if (!add_localized_text(LocalizedTextSlot::MarkerAll, localized.all_markers,
            kContentX, kMarkerAllY, 420.0, 24.0, 5, 0.4375)) {
        last_failure_ = 9;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    for (std::size_t column = 0; column < control_x.size(); ++column) {
        const auto mask = column == 0 ? compact_radar_visibility_mask(sanitized)
                                      : world_radar_visibility_mask(sanitized);
        const bool enabled = (mask & kColumnCategories[column]) == kColumnCategories[column];
        column_all_visuals[column] = add_border(control_x[column] + 11.0,
            kMarkerAllY + 1.0, 22.0, 22.0, 11, column == 0 ? kCompactEnabled : kWorldEnabled);
        column_all_controls[column] = add_control(control_x[column] + 10.0,
            kMarkerAllY, 24.0, 24.0, enabled);
        if (!column_all_visuals[column] || !column_all_controls[column]
            || !add_border(control_x[column] + 11.0, kMarkerAllY + 1.0,
                22.0, 22.0, 10, kToggleFrame)) {
            last_failure_ = 11;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        set_visibility(column_all_visuals[column], set_visibility_, enabled ? kVisible : kCollapsed);
        column_all_selected_[column] = enabled;
    }
    std::size_t placeholder_count{};
    for (std::size_t row = 0; row < kRows.size(); ++row) {
        const RowDefinition& definition = kRows[row];
        const std::size_t category_index = static_cast<std::size_t>(definition.category);
        const double y = kMarkerRowsY + static_cast<double>(row) * kMarkerRowStep;
        category_label_texts[category_index] = add_text(
            localized.marker_categories[category_index], kContentX, y,
            420.0, 24.0, 5, 0.4375);
        if (!category_label_texts[category_index]) {
            last_failure_ = 9;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        // Only Radar and Map belong to this grid. Logical column 2 is built
        // below as Scene's three independent category chips.
        for (std::size_t column = 0; column < control_x.size(); ++column) {
            if ((kColumnCategories[column] & radar_visibility_bit(definition.category)) == 0U) {
                UObject* placeholder = add_text(L"-", control_x[column], y, 44.0, 24.0, 5, 0.46, kTextCenter);
                if (!placeholder || placeholder_count >= 2U) {
                    last_failure_ = 10;
                    return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
                }
                placeholder_texts_[placeholder_count] = placeholder;
                add_numeric_glyph(12U + placeholder_count, control_x[column] + 14.0, y - 1.0, 8);
                ++placeholder_count;
                continue;
            }
            const std::uint8_t mask = column == 0
                ? compact_radar_visibility_mask(sanitized) : world_radar_visibility_mask(sanitized);
            const bool enabled = (mask & radar_visibility_bit(definition.category)) != 0;
            UObject* inner = add_border(control_x[column] + 11.0, y + 1.0,
                22.0, 22.0, 11, column == 0 ? kCompactEnabled : kWorldEnabled);
            UObject* control = add_control(control_x[column] + 10.0, y, 24.0, 24.0, enabled);
            if (!inner || !control
                || !add_border(control_x[column] + 11.0, y + 1.0, 22.0, 22.0, 10, kToggleFrame)) {
                last_failure_ = 11;
                return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
            }
            set_visibility(inner, set_visibility_, enabled ? kVisible : kCollapsed);
            controls[column][category_index] = control;
            enabled_visuals[column][category_index] = inner;
        }
    }

    if (!add_border(kCardX, kSceneTop, kCardWidth, kSceneBottom - kSceneTop,
                    3, kContentBackground)
        || !add_border(kCardX, kSceneTitleY + 4.0, 3.0, 20.0, 4, kPanelAccent)
        || !add_localized_text(LocalizedTextSlot::SceneSettings,
            localized.scene_settings, kContentX, kSceneTitleY, 490.0, 28.0, 5, kSectionTitleScale)
        || !add_localized_text(LocalizedTextSlot::SceneRange, localized.scene_range,
            kContentX, kSceneRangeY, 192.0, 26.0, 5, 0.4375)
        || !add_localized_text(LocalizedTextSlot::SceneLimit, localized.scene_limit,
            kContentX, kSceneLimitY, 192.0, 26.0, 5, 0.4375)
        || !add_localized_text(LocalizedTextSlot::SceneDistance, localized.scene_distance,
            kContentX, kSceneDistanceTitleY, 686.0, 18.0, 5, 0.375)) {
        last_failure_ = 39;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    const std::array<RadarVisibilityCategory, 3> scene_categories{{
        RadarVisibilityCategory::Treasure, RadarVisibilityCategory::AreaQuests,
        RadarVisibilityCategory::MiniGames}};
    const std::array<LocalizedTextSlot, 3> scene_category_slots{{
        LocalizedTextSlot::SceneTreasure, LocalizedTextSlot::SceneAreaQuest,
        LocalizedTextSlot::SceneMiniGame}};
    for (std::size_t index = 0; index < scene_categories.size(); ++index) {
        const std::size_t category = static_cast<std::size_t>(scene_categories[index]);
        const double x = kContentX + static_cast<double>(index) * kSceneChipStep;
        const bool enabled = (scene_radar_visibility_mask(sanitized)
            & radar_visibility_bit(scene_categories[index])) != 0;
        UObject* inner = add_border(x + 1.0, kSceneChipsY + 1.0,
            kSceneChipWidth - 2.0, 28.0, 7, kModeSelectedAvailable);
        UObject* control = add_control(x, kSceneChipsY, kSceneChipWidth, 30.0, enabled);
        if (!inner || !control
            || !add_border(x, kSceneChipsY, kSceneChipWidth, 30.0, 6, kModeOption)
            || !add_localized_text(scene_category_slots[index],
                localized.marker_categories[category], x + 10.0, kSceneChipsY + 3.0,
                kSceneChipWidth - 20.0, 24.0, 8, 0.40625, kTextCenter)) {
            last_failure_ = 39;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        set_visibility(inner, set_visibility_, enabled ? kVisible : kCollapsed);
        controls[2][category] = control;
        enabled_visuals[2][category] = inner;
    }

    const std::array<double, 2> slider_y{{kSceneRangeY, kSceneLimitY}};
    const std::array<unsigned, 2> slider_maximum{{1000U, 50U}};
    const std::array<unsigned, 2> slider_value{{
        pending_scene_settings_.range_meters, pending_scene_settings_.marker_limit}};
    for (std::size_t index = 0; index < slider_y.size(); ++index) {
        UObject* slider = UObjectGlobals::NewObject<UObject>(tree, slider_class_);
        std::array<wchar_t, 24> display{};
        std::swprintf(display.data(), display.size(), index == 0 ? L"%u m" : L"%u", slider_value[index]);
        UObject* value_text = add_text(display.data(), 634.0, slider_y[index],
            88.0, 26.0, 7, 0.4375, kTextCenter);
        if (!slider || !value_text
            || !add_border(632.0, slider_y[index], 92.0, 26.0, 5, kModeOption)
            || !add_widget(slider, 244.0, slider_y[index], 372.0, 26.0, 12)) {
            last_failure_ = 39;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        ScalarParameters minimum{0.0F}, maximum{1.0F};
        ScalarParameters step{1.0F / static_cast<float>(slider_maximum[index])};
        ScalarParameters value{static_cast<float>(slider_value[index])
            / static_cast<float>(slider_maximum[index])};
        slider->ProcessEvent(set_slider_min_, &minimum);
        slider->ProcessEvent(set_slider_max_, &maximum);
        slider->ProcessEvent(set_slider_step_, &step);
        slider->ProcessEvent(set_slider_value_, &value);
        if (set_slider_bar_color_) {
            BrushColorParameters color{decode_ui_srgb(kToggleFrame)};
            slider->ProcessEvent(set_slider_bar_color_, &color);
        }
        if (set_slider_handle_color_) {
            BrushColorParameters color{decode_ui_srgb(kCompactEnabled)};
            slider->ProcessEvent(set_slider_handle_color_, &color);
        }
        scene_sliders_[index] = slider;
        scene_value_texts_[index] = value_text;
        for (std::size_t digit = 0; digit < 6U; ++digit)
            add_numeric_glyph(index * 6U + digit, 634.0, slider_y[index], 8);
    }
    const std::array<LocalizedTextSlot, 4> distance_slots{{
        LocalizedTextSlot::SceneDistanceOff, LocalizedTextSlot::SceneDistanceCentral,
        LocalizedTextSlot::SceneDistanceNearest, LocalizedTextSlot::SceneDistanceAll}};
    for (std::size_t index = 0; index < distance_slots.size(); ++index) {
        const double x = kContentX + static_cast<double>(index) * 174.0;
        const bool selected = static_cast<std::size_t>(pending_scene_settings_.distance_mode) == index;
        UObject* visual = add_border(x + 1.0, kSceneDistanceY + 1.0,
            164.0, 28.0, 7, kModeSelectedAvailable);
        UObject* control = add_control(x, kSceneDistanceY, 166.0, 30.0, selected);
        if (!visual || !control
            || !add_border(x, kSceneDistanceY, 166.0, 30.0, 6, kModeOption)
            || !add_localized_text(distance_slots[index], localized.scene_distance_modes[index],
                x + 4.0, kSceneDistanceY + 3.0, 158.0, 24.0, 8, 0.40625, kTextCenter)) {
            last_failure_ = 39;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        set_visibility(visual, set_visibility_, selected ? kVisible : kCollapsed);
        scene_distance_controls_[index] = control;
        scene_distance_visuals_[index] = visual;
    }

    if (!add_border(kCardX, kHeightTop, kCardWidth, kHeightBottom - kHeightTop,
                    3, kContentBackground)
        || !add_border(kCardX, kHeightHeaderY + 4.0, 3.0, 20.0, 4, kPanelAccent)
        || !add_localized_text(LocalizedTextSlot::HeightIndicators, localized.height_indicators,
            kContentX, kHeightHeaderY, 500.0, 28.0, 5, kSectionTitleScale)
        || !add_localized_text(LocalizedTextSlot::RadarOnly, localized.radar_only,
            580.0, kHeightHeaderY + 3.0, 142.0, 22.0, 5, 0.375, kTextCenter)) {
        last_failure_ = 32;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    for (std::size_t index = 0; index < kHeightIndicatorCount; ++index) {
        const double x = kContentX + static_cast<double>(index % 3U) * kSceneChipStep;
        const double y = kHeightRowsY + static_cast<double>(index / 3U) * 34.0;
        const auto category = static_cast<dswros::HeightIndicatorCategory>(index);
        const bool enabled = dswros::height_indicator_enabled(pending_height_indicators_, category);
        UObject* inner = add_border(x + 1.0, y + 1.0, 220.0, 26.0, 7, kModeSelectedAvailable);
        UObject* control = add_control(x, y, 222.0, 28.0, enabled);
        height_label_texts[index] = add_text(localized.height_categories[index],
            x + 10.0, y + 2.0, 202.0, 24.0, 8, 0.40625, kTextCenter);
        if (!height_label_texts[index] || !inner || !control
            || !add_border(x, y, 222.0, 28.0, 6, kModeOption)) {
            last_failure_ = 32;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        set_visibility(inner, set_visibility_, enabled ? kVisible : kCollapsed);
        height_controls[index] = control;
        height_visuals[index] = inner;
    }

    if (!add_border(kCardX, kFilterTop, kCardWidth, kContentBottom - kFilterTop,
                    3, kContentBackground)
        || !add_border(kCardX, kFilterHeaderY + 4.0, 3.0, 20.0, 4, kPanelAccent)
        || !add_localized_text(LocalizedTextSlot::FilterModes, localized.filter_modes,
            kContentX, kFilterHeaderY, 620.0, 28.0, 5, kSectionTitleScale)) {
        last_failure_ = 16;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    const std::array<RadarVisibilityCategory, 2> mode_categories{{
        RadarVisibilityCategory::AreaQuests, RadarVisibilityCategory::Assault}};
    const std::array<LocalizedTextSlot, 2> mode_label_slots{{
        LocalizedTextSlot::AreaQuestMode, LocalizedTextSlot::AssaultMode}};
    const std::array<LocalizedTextSlot, 2> available_slots{{
        LocalizedTextSlot::AreaAvailable, LocalizedTextSlot::AssaultAvailable}};
    const std::array<LocalizedTextSlot, 2> all_slots{{
        LocalizedTextSlot::AreaAll, LocalizedTextSlot::AssaultAll}};
    for (std::size_t group = 0; group < 2U; ++group) {
        const double left = kContentX + static_cast<double>(group) * 354.0;
        if (!add_localized_text(mode_label_slots[group],
                localized.marker_categories[static_cast<std::size_t>(mode_categories[group])],
                left, kFilterRowsY, 334.0, 22.0, 5, 0.4375)) {
            last_failure_ = 16;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        for (std::size_t option = 0; option < 2U; ++option) {
            const double x = left + static_cast<double>(option) * 172.0;
            const bool selected = group == 0U
                ? (current_area_quest_mode == AreaQuestDisplayMode::Available) == (option == 0U)
                : (current_assault_mode == AssaultDisplayMode::Current) == (option == 0U);
            UObject* inner = add_border(x + 1.0, kFilterOptionsY + 1.0,
                158.0, 28.0, 7, option == 0U ? kModeSelectedAvailable : kModeSelectedAll);
            UObject* control = add_control(x, kFilterOptionsY, 160.0, 30.0, selected);
            if (!inner || !control
                || !add_border(x, kFilterOptionsY, 160.0, 30.0, 6, kModeOption)
                || !add_localized_text(option == 0U ? available_slots[group] : all_slots[group],
                    option == 0U ? localized.available : localized.all,
                    x + 4.0, kFilterOptionsY + 3.0, 152.0, 24.0, 8, 0.40625, kTextCenter)) {
                last_failure_ = 17;
                return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
            }
            set_visibility(inner, set_visibility_, selected ? kVisible : kCollapsed);
            if (group == 0U) {
                area_mode_controls[option] = control;
                area_mode_visuals[option] = inner;
            } else {
                assault_mode_controls[option] = control;
                assault_mode_visuals[option] = inner;
            }
        }
    }

    // The optional packaged text path owns pixels only. These images never
    // participate in hit testing, and every existing CheckBox remains above
    // the corresponding visual layer. The full-panel textures and the small
    // language-name texture share the existing viewport / DPI layout authority.
    UObject* main_text_overlay_image{};
    UObject* popup_text_overlay_image{};
    UObject* language_value_overlay_image{};
    if (image_class_) {
        const auto images = add_page_images(nullptr, 21);
        for (std::size_t index = 0; index < images.size(); ++index)
            main_text_overlay_images_[index] = images[index];
        main_text_overlay_image = images[0];
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
        language_value_overlay_image =
            UObjectGlobals::NewObject<UObject>(tree, image_class_);
        if (language_value_overlay_image) {
            set_visibility(
                language_value_overlay_image, set_visibility_, kCollapsed);
            if (!add_widget(
                    language_value_overlay_image, kLanguageValueX,
                    kLanguageValueY, kLanguageValueWidth,
                    kLanguageValueHeight, 7)) {
                language_value_overlay_image = nullptr;
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
    for (std::size_t column = 0; column < column_all_controls.size(); ++column) {
        column_all_controls_[column] = column_all_controls[column];
        column_all_visuals_[column] = column_all_visuals[column];
    }
    popup_text_overlay_image_ = popup_text_overlay_image;
    language_value_overlay_image_ = language_value_overlay_image;
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
    const auto bind_tip = [this, tree, unit_scale](UObject* widget, dswros::RadarTooltipId id) {
        bind_tooltip_unsafe(widget, static_cast<std::uint8_t>(id), tree, unit_scale);
    };
    const LinearColor transparent_hover_color{};
    bind_tip(column_all_controls[0], dswros::RadarTooltipId::AllRadar);
    bind_tip(column_all_controls[1], dswros::RadarTooltipId::AllMap);
    for (std::size_t row = 0; row < kRows.size(); ++row) {
        const auto category = kRows[row].category;
        const auto category_index = static_cast<std::size_t>(category);
        const auto topic = marker_tooltip_for(category);
        for (std::size_t column = 0; column < 2U; ++column)
            if (UObject* control = controls[column][category_index]) bind_tip(control, topic);
        // The row-name target remains visible when packaged text hides the
        // native label. It ends before both settings columns and adds no input focus.
        UObject* label_tooltip_target = add_border(kContentX,
            kMarkerRowsY + static_cast<double>(row) * kMarkerRowStep,
            420.0, kMarkerRowStep, 11, transparent_hover_color);
        set_visibility(label_tooltip_target, set_visibility_, kVisible);
        bind_tip(label_tooltip_target, topic);
    }
    constexpr std::array<dswros::RadarTooltipId, 3> scene_tips{{
        dswros::RadarTooltipId::SceneTreasure, dswros::RadarTooltipId::SceneAreaQuests,
        dswros::RadarTooltipId::SceneMiniGames}};
    for (std::size_t index = 0; index < scene_categories.size(); ++index)
        bind_tip(controls[2][static_cast<std::size_t>(scene_categories[index])], scene_tips[index]);
    constexpr std::array<dswros::RadarTooltipId, kHeightIndicatorCount> height_tips{{
        dswros::RadarTooltipId::HeightTreasure, dswros::RadarTooltipId::HeightAreaQuests,
        dswros::RadarTooltipId::HeightMole, dswros::RadarTooltipId::HeightBoss,
        dswros::RadarTooltipId::HeightAssault}};
    for (std::size_t index = 0; index < height_controls.size(); ++index)
        bind_tip(height_controls[index], height_tips[index]);
    bind_tip(area_mode_controls[0], dswros::RadarTooltipId::AreaQuestAvailable);
    bind_tip(area_mode_controls[1], dswros::RadarTooltipId::AreaQuestAll);
    bind_tip(assault_mode_controls[0], dswros::RadarTooltipId::AssaultAvailable);
    bind_tip(assault_mode_controls[1], dswros::RadarTooltipId::AssaultAll);
    bind_tip(scene_sliders_[0].Get(), dswros::RadarTooltipId::SceneRange);
    bind_tip(scene_sliders_[1].Get(), dswros::RadarTooltipId::SceneLimit);
    constexpr std::array<dswros::RadarTooltipId, 4> distance_tips{{
        dswros::RadarTooltipId::DistanceOff, dswros::RadarTooltipId::DistanceAim,
        dswros::RadarTooltipId::DistanceAuto, dswros::RadarTooltipId::DistanceAll}};
    for (std::size_t index = 0; index < scene_distance_controls_.size(); ++index)
        bind_tip(scene_distance_controls_[index].Get(), distance_tips[index]);
    bind_tip(language_dropdown_control, dswros::RadarTooltipId::Language);
    for (UObject* control : language_choice_controls)
        bind_tip(control, dswros::RadarTooltipId::Language);
    UObject* status_tooltip_target = add_border(388.0, 54.0, 207.0, 34.0, 11,
                                               transparent_hover_color);
    set_visibility(status_tooltip_target, set_visibility_, kVisible);
    bind_tip(status_tooltip_target, dswros::RadarTooltipId::ModStatus);
    bind_tip(mod_action_control, dswros::RadarTooltipId::EnableDisable);
    bind_tip(global_reset_control_.Get(), dswros::RadarTooltipId::RestoreDefaults);
    bind_tip(bug_report_control, dswros::RadarTooltipId::BugReport);
    bind_tip(endorsement_control_.Get(), dswros::RadarTooltipId::Endorse);
    bind_tip(close_control, dswros::RadarTooltipId::Close);

    // Confirmation controls are preallocated in the same game-owned tree.
    // The input shield and disabled background prevent a modal click or key
    // from changing a setting underneath. No OS dialog blocks the game thread.
    const auto add_modal_border = [&add_widget, &add_skin, tree, this](
        double x, double y, double width, double height,
        std::int32_t z, const LinearColor& color) -> UObject* {
        // Reuse validated nine-slice skins: compact rounded dialog and buttons
        // need neither a new texture nor an unverified engine styling API.
        if (z >= 81 && height >= 30.0) {
            const std::size_t skin = &color == &kActionButton ? 3U : 2U;
            if (UObject* image = add_skin(skin, x, y, width, height, z)) {
                set_visibility(image, set_visibility_, kCollapsed);
                return image;
            }
        }
        UObject* border = UObjectGlobals::NewObject<UObject>(tree, border_class_);
        if (!border) return nullptr;
        set_brush_color(border, set_brush_color_, color);
        set_visibility(border, set_visibility_, kCollapsed);
        return add_widget(border, x, y, width, height, z) ? border : nullptr;
    };
    constexpr LinearColor confirmation_surface{
        30.0F / 255.0F, 42.0F / 255.0F, 57.0F / 255.0F, 0.96F};
    constexpr LinearColor confirmation_divider{0.75F, 0.82F, 0.88F, 0.06F};
    confirmation_decorations_[0] = add_modal_border(0, 0,
        kReferencePanelWidth, kReferencePanelHeight, 79, kPopupDim);
    confirmation_decorations_[1] = add_modal_border(kConfirmationX, kConfirmationY,
        kConfirmationWidth, kConfirmationHeight, 81, confirmation_surface);
    confirmation_decorations_[2] = add_modal_border(220, 455, 320, 1, 82, confirmation_divider);
    confirmation_decorations_[3] = add_modal_border(396, 462, 144, 32, 83, kActionButton);
    confirmation_decorations_[4] = add_modal_border(220, 462, 144, 32, 83, kModeOption);
    confirmation_blocker_ = add_control(0, 0, kReferencePanelWidth,
        kReferencePanelHeight, false, 80);
    confirmation_yes_control_ = add_control(396, 462, 144, 32, false, 86);
    confirmation_no_control_ = add_control(220, 462, 144, 32, false, 86);
    for (const auto& decoration : confirmation_decorations_) {
        if (!decoration.Get()) {
            last_failure_ = 42;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
    }
    // Atlas crops occupy the same rectangles in every language. Native ASCII
    // text remains a readable cancel-only fallback if the optional atlas fails.
    constexpr std::array<std::array<double, 6>, 5> confirmation_text_geometry{{
        {{278, 831, 206, 24, 0.40625, 1.0}},
        {{220, 350, 320, 32, 0.50, 1.0}},
        {{220, 382, 320, 72, 0.40625, 1.0}},
        {{396, 466, 144, 24, 0.40625, 1.0}},
        {{220, 466, 144, 24, 0.40625, 1.0}},
    }};
    constexpr std::array<const wchar_t*, 5> confirmation_fallback{{
        L"Vote for this mod", L"Confirm", L"Text unavailable. Reopen Settings to try again.",
        L"Yes", L"No"}};
    for (std::size_t index = 0; index < confirmation_texts_.size(); ++index) {
        auto& record = confirmation_texts_[index];
        const auto& geometry = confirmation_text_geometry[index];
        record.native_text = add_text(confirmation_fallback[index],
            geometry[0], geometry[1], geometry[2], geometry[3], index == 0 ? 21 : 84,
            geometry[4], kTextCenter, dswros::RadarUiLanguage::English);
        record.scale = unit_scale * geometry[5];
        if (!record.native_text.Get()) {
            last_failure_ = 42;
            return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
        }
        if (tooltip_widget_abi_available_ && image_class_) {
            UObject* canvas = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
            UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
            if (!canvas || !image || !add_widget(canvas, geometry[0], geometry[1],
                    geometry[2], geometry[3], index == 0 ? 23 : 85)) continue;
            ByteParameters clipping{1}; // ClipToBounds; the shared atlas never paints adjacent tiles.
            canvas->ProcessEvent(set_clipping_, &clipping);
            AddChildToCanvasParameters child{image};
            canvas->ProcessEvent(add_child_to_canvas_, &child);
            if (!child.return_value) continue;
            set_slot_vector(child.return_value, set_slot_alignment_, 0, 0);
            set_slot_vector(child.return_value, set_slot_size_, kTooltipReferenceWidth * record.scale,
                kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * record.scale);
            set_visibility(canvas, set_visibility_, kCollapsed);
            set_visibility(image, set_visibility_, kHitTestInvisible);
            record.canvas = canvas;
            record.image = image;
            record.image_slot = child.return_value;
        }
    }
    refresh_tooltips_unsafe();
    (void)refresh_numeric_text_unsafe();
    if (!refresh_confirmation_text_unsafe()
        || !set_confirmation_visibility_unsafe(false)) {
        last_failure_ = 42;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
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
    (void)refresh_language_value_overlay_unsafe(current_controller);
    refresh_guide_unsafe();
    if (!set_language_popup_visibility_unsafe(false)) {
        last_failure_ = 35;
        return {RadarVisibilityHubAction::Rejected, sanitized, false,
                last_failure_};
    }

    set_visibility(host, set_visibility_, kVisible);
    // Install consumption before exposing any viewport UI, including opening
    // prepasses that may re-enter the Windows message pump.
    if (!hub_escape_input::open()) {
        last_failure_ = 40;
        return {RadarVisibilityHubAction::Rejected, sanitized, false, last_failure_};
    }
    AddToViewportParameters add_to_viewport{kViewportZOrder};
    host->ProcessEvent(add_to_viewport_, &add_to_viewport);
    VectorParameters viewport_alignment{{0.0, 0.0}};
    host->ProcessEvent(set_alignment_in_viewport_, &viewport_alignment);
    VectorParameters desired_size{{
        kReferencePanelWidth * unit_scale,
        viewport_layout.panel_reference_height * unit_scale}};
    host->ProcessEvent(set_desired_size_in_viewport_, &desired_size);
    const double physical_width =
        kReferencePanelWidth * display_scale;
    const double physical_height =
        viewport_layout.panel_reference_height * display_scale;
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
        source_language_, RadarVisibilityHubCommand::None, source_scene_settings_};
}

bool RadarVisibilityHub::configure_chip_nine_slice_unsafe(UObject* image, double unit_scale) {
    if (!chip_nine_slice_abi_available_ || !image || !image->IsA(image_class_)
        || !std::isfinite(unit_scale) || !(unit_scale > 0.0)) return false;
    void* current = image_brush_property_->ContainerPtrToValuePtr<void>(image);
    FontCallParameters parameters(set_image_brush_);
    if (!current || !parameters.valid()) return false;
    void* target = set_image_brush_value_property_->ContainerPtrToValuePtr<void>(parameters.data());
    if (!target) return false;
    set_image_brush_value_property_->CopyCompleteValue(target, current);
    void* margin = brush_margin_property_->ContainerPtrToValuePtr<void>(target);
    void* image_size = brush_image_size_property_->ContainerPtrToValuePtr<void>(target);
    auto* draw_as = CastField<FNumericProperty>(brush_draw_as_property_);
    void* draw_value = draw_as->ContainerPtrToValuePtr<void>(target);
    // Standard Slate Box drawing preserves ten-reference-unit corner caps
    // across all fixed chip widths; no UV offsets or native struct layout is guessed.
    constexpr std::array<double, 4> margins{{kChipCornerRadius / kChipSkinReferenceWidth,
        kChipCornerRadius / kChipSkinReferenceHeight, kChipCornerRadius / kChipSkinReferenceWidth,
        kChipCornerRadius / kChipSkinReferenceHeight}};
    const std::array<double, 6> expected{{margins[0], margins[1], margins[2], margins[3],
        kChipSkinReferenceWidth * unit_scale, kChipSkinReferenceHeight * unit_scale}};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        auto* metric = CastField<FNumericProperty>(brush_box_metrics_[index]);
        void* address = metric->ContainerPtrToValuePtr<void>(index < 4U ? margin : image_size);
        if (!write_font_metric(metric, address, expected[index])) return false;
    }
    if (!write_font_metric(draw_as, draw_value, 1.0)) return false; // ESlateBrushDrawType::Box.
    UObject* texture = read_struct_object_property(image, L"Brush", L"ResourceObject");
    image->ProcessEvent(set_image_brush_, parameters.data());
    current = image_brush_property_->ContainerPtrToValuePtr<void>(image);
    margin = brush_margin_property_->ContainerPtrToValuePtr<void>(current);
    image_size = brush_image_size_property_->ContainerPtrToValuePtr<void>(current);
    for (std::size_t index = 0; index < expected.size(); ++index) {
        auto* metric = CastField<FNumericProperty>(brush_box_metrics_[index]);
        const void* address = metric->ContainerPtrToValuePtr<void>(index < 4U ? margin : image_size);
        double actual{};
        if (!read_font_metric(metric, address, actual) || !font_metric_matches(actual, expected[index])) return false;
    }
    double actual_draw_as{};
    return read_font_metric(draw_as, draw_as->ContainerPtrToValuePtr<void>(current), actual_draw_as)
        && actual_draw_as == 1.0 && texture
        && read_struct_object_property(image, L"Brush", L"ResourceObject") == texture;
}

void RadarVisibilityHub::bind_tooltip_unsafe(
    UObject* control, std::uint8_t tooltip_id, UObject* tree, double unit_scale) {
    if (!control || !tree || tooltip_count_ >= tooltips_.size()
        || tooltip_id >= static_cast<std::uint8_t>(dswros::RadarTooltipId::Count)) return;
    TooltipRecord& record = tooltips_[tooltip_count_++];
    record.control = control;
    record.id = tooltip_id;
    record.unit_scale = unit_scale;
    create_tooltip_content_unsafe(tooltip_count_ - 1U, tree);
}

void RadarVisibilityHub::create_tooltip_content_unsafe(std::size_t index, UObject* tree) {
    if (index >= tooltip_count_ || !tree) return;
    TooltipRecord& record = tooltips_[index];
    UObject* control = record.control.Get();
    if (!control) return;
    const double unit_scale = record.unit_scale;
    const std::uint8_t tooltip_id = record.id;
    // Every owner has a distinct tooltip widget; only the atlas texture is
    // shared. Slate owns hover timing/position and the widget's reflected
    // ToolTipWidget reference owns its content. No mouse polling is added.
    if (!tooltip_widget_abi_available_ || !image_class_) return;
    UObject* size_box = UObjectGlobals::NewObject<UObject>(tree, size_box_class_);
    UObject* canvas = UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    UObject* image = UObjectGlobals::NewObject<UObject>(tree, image_class_);
    if (!size_box || !canvas || !image) return;
    ScalarParameters width{static_cast<float>(kTooltipReferenceWidth * unit_scale)};
    ScalarParameters height{static_cast<float>(kTooltipReferenceHeight * unit_scale)};
    size_box->ProcessEvent(set_width_override_, &width);
    size_box->ProcessEvent(set_height_override_, &height);
    AddChildToCanvasParameters content{canvas};
    size_box->ProcessEvent(set_content_, &content);
    if (!content.return_value) return;
    ByteParameters clipping{1}; // EWidgetClipping::ClipToBounds.
    canvas->ProcessEvent(set_clipping_, &clipping);
    AddChildToCanvasParameters child{image};
    canvas->ProcessEvent(add_child_to_canvas_, &child);
    if (!child.return_value) return;
    set_slot_vector(child.return_value, set_slot_position_, 0.0,
        -kTooltipReferenceHeight * static_cast<double>(tooltip_id) * unit_scale);
    set_slot_vector(child.return_value, set_slot_size_, kTooltipReferenceWidth * unit_scale,
        kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * unit_scale);
    set_slot_vector(child.return_value, set_slot_alignment_, 0.0, 0.0);
    set_visibility(size_box, set_visibility_, kHitTestInvisible);
    set_visibility(canvas, set_visibility_, kHitTestInvisible);
    set_visibility(image, set_visibility_, kHitTestInvisible);
    // Bind now so reflected ownership retains content through any GC that
    // occurs while remaining controls and localized resources are prepared.
    ObjectReturnParameters tooltip{size_box};
    control->ProcessEvent(set_tool_tip_, &tooltip);
    record.content = size_box;
    record.image = image;
    record.image_slot = child.return_value;
}

void RadarVisibilityHub::refresh_tooltips_unsafe() {
    if (tooltip_count_ == 0) return;
    constexpr std::array<const wchar_t*, 11> atlas_codes{{L"en", L"ja", L"ko",
        L"zh-hans", L"zh-hant", L"fr", L"de", L"es-es", L"ru", L"th", L"pt-br"}};
    const std::size_t language_index = static_cast<std::size_t>(resolved_ui_language_);
    if (language_index >= atlas_codes.size()) return;
    if (tooltip_atlas_language_ != resolved_ui_language_) {
        tooltip_atlas_ = FWeakObjectPtr{};
        tooltip_atlas_attempted_ = false;
        tooltip_atlas_language_ = resolved_ui_language_;
    }
    if (tooltip_widget_abi_available_ && !tooltip_atlas_attempted_) {
        tooltip_atlas_attempted_ = true;
        const std::wstring filename = std::wstring(L"tooltip-") + atlas_codes[language_index] + L".tga";
        const auto atlas_path = text_overlay_root_ / filename;
        tooltip_atlas_ = valid_confirmation_atlas_file(atlas_path)
            ? import_text_overlay_unsafe(host_.Get(), atlas_path) : nullptr;
    }
    UObject* texture = tooltip_atlas_.Get();
    const auto& localized = dswros::radar_localized_text(resolved_ui_language_);
    for (std::size_t index = 0; index < tooltip_count_; ++index) {
        TooltipRecord& record = tooltips_[index];
        UObject* control = record.control.Get();
        if (!control || record.id >= localized.tooltips.size()) continue;
        UObject* content = record.content.Get();
        UObject* image = record.image.Get();
        if (texture && (!content || !image)) {
            // A native fallback releases the custom content's owning property.
            // Recreate it only on a later successful language/open edge, never
            // retain a dead widget or append another binding to the fixed pool.
            create_tooltip_content_unsafe(index, widget_tree_.Get());
            content = record.content.Get();
            image = record.image.Get();
        }
        if (texture && content && image && apply_text_overlay_unsafe(image, texture)) {
            ObjectReturnParameters tooltip{content};
            control->ProcessEvent(set_tool_tip_, &tooltip);
        } else {
            // Never keep the previous language's atlas after a failed switch.
            // The standard native text tooltip remains the asset-failure path.
            if (tooltip_widget_abi_available_) {
                ObjectReturnParameters clear{};
                control->ProcessEvent(set_tool_tip_, &clear);
            }
            if (set_tool_tip_text_ && tool_tip_text_property_)
                set_text(control, set_tool_tip_text_, tool_tip_text_property_, localized.tooltips[record.id]);
        }
    }
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
    const auto fallback = [this](std::uint32_t failure) {
        text_overlay_active_ = false;
        text_overlay_failure_ = failure;
        for (const auto& handle : main_text_overlay_images_) {
            if (UObject* image = handle.Get()) {
                set_visibility(image, set_visibility_, kCollapsed);
                if (set_brush_from_texture_) {
                    SetBrushFromTextureParameters clear{};
                    image->ProcessEvent(set_brush_from_texture_, &clear);
                }
            }
        }
        main_text_overlay_texture_ = FWeakObjectPtr{};
        return set_native_text_visibility_unsafe(true);
    };
    if (!uses_packaged_text_overlay(resolved_ui_language_)) return fallback(0);
    if (!image_class_ || !kismet_rendering_library_.Get()
        || !set_brush_from_texture_ || !import_file_as_texture_) return fallback(1);
    for (const auto& handle : main_text_overlay_images_)
        if (!handle.Get()) return fallback(1);
    UObject* texture = main_text_overlay_texture_.Get();
    if (!texture || text_overlay_language_ != resolved_ui_language_
        || text_overlay_status_ != displayed_mod_status_) {
        const wchar_t* prefix = packaged_text_overlay_prefix(resolved_ui_language_);
        const wchar_t* status = packaged_text_overlay_status(displayed_mod_status_);
        const std::wstring filename = std::wstring(prefix ? prefix : L"") + L"-" + status + L".tga";
        texture = import_text_overlay_unsafe(world_context, text_overlay_root_ / filename);
        if (!texture) return fallback(2);
        for (const auto& handle : main_text_overlay_images_)
            if (!apply_text_overlay_unsafe(handle.Get(), texture)) return fallback(3);
        main_text_overlay_texture_ = texture;
        text_overlay_language_ = resolved_ui_language_;
        text_overlay_status_ = displayed_mod_status_;
    }
    if (!set_native_text_visibility_unsafe(false)) return false;
    for (const auto& handle : main_text_overlay_images_)
        set_visibility(handle.Get(), set_visibility_, kHitTestInvisible);
    text_overlay_active_ = true;
    text_overlay_failure_ = 0;
    return true;
}

bool RadarVisibilityHub::refresh_language_value_overlay_unsafe(
    UObject* world_context) {
    UObject* native_value = localized_texts_[static_cast<std::size_t>(
        LocalizedTextSlot::LanguageValue)].Get();
    if (!native_value) {
        return false;
    }
    // Every main language atlas already contains this value. The two legacy
    // name-only images are retained strictly as a missing-main-asset fallback.
    set_visibility(native_value, set_visibility_,
                   text_overlay_active_ ? kCollapsed : kHitTestInvisible);
    UObject* image = language_value_overlay_image_.Get();
    if (image) {
        set_visibility(image, set_visibility_, kCollapsed);
    }
    if (text_overlay_active_) return true;

    std::size_t index{};
    const wchar_t* filename{};
    if (resolved_ui_language_ == dswros::RadarUiLanguage::French) {
        index = 0;
        filename = L"fr-language-value.tga";
    } else if (resolved_ui_language_ == dswros::RadarUiLanguage::SpanishSpain) {
        index = 1;
        filename = L"es-language-value.tga";
    } else {
        return true;
    }

    // Runs only on open, language and status edges. A failed resource is not
    // retried until the next open; a collected inactive weak texture may be
    // imported again when its language is selected. There are only two slots.
    auto& failure = language_value_overlay_failures_[index];
    if (failure != 0) {
        text_overlay_failure_ = failure;
        return true;
    }
    if (!image || !image_class_ || !kismet_rendering_library_.Get()
        || !set_brush_from_texture_ || !import_file_as_texture_) {
        failure = text_overlay_failure_ = 7;
        return true;
    }
    UObject* texture = language_value_overlay_textures_[index].Get();
    if (!texture) {
        texture = import_text_overlay_unsafe(
            world_context, text_overlay_root_ / filename);
        if (!texture) {
            failure = text_overlay_failure_ = 8;
            return true;
        }
    }
    if (!apply_text_overlay_unsafe(image, texture)) {
        failure = text_overlay_failure_ = 9;
        return true;
    }
    language_value_overlay_textures_[index] = texture;
    // apply_text_overlay_unsafe verifies Brush.ResourceObject before native
    // text is hidden, so missing or rejected assets retain a usable fallback.
    set_visibility(native_value, set_visibility_, kCollapsed);
    set_visibility(image, set_visibility_, kHitTestInvisible);
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

bool RadarVisibilityHub::refresh_confirmation_text_unsafe() {
    using Text = dswros::RadarConfirmationTextId;
    using Action = dswros::RadarConfirmationAction;
    const Text body = confirmation_.pending() == Action::RestoreDefaults ? Text::RestoreBody
        : confirmation_.pending() == Action::BugReport ? Text::FeedbackBody : Text::EndorseBody;
    const std::array<Text, 5> ids{{Text::Endorse, Text::Title, body, Text::Yes, Text::No}};
    UObject* texture = tooltip_atlas_language_ == resolved_ui_language_
        ? tooltip_atlas_.Get() : nullptr;
    std::array<bool, 5> ready{};
    for (std::size_t index = 0; index < confirmation_texts_.size(); ++index) {
        auto& record = confirmation_texts_[index];
        UObject* text = record.native_text.Get();
        UObject* canvas = record.canvas.Get();
        UObject* image = record.image.Get();
        UObject* slot = record.image_slot.Get();
        if (!text) return false;
        if (texture && canvas && image && slot) {
            const auto tile = dswros::kRadarTooltipCount + static_cast<std::size_t>(ids[index]);
            set_slot_vector(slot, set_slot_position_, 0,
                -kTooltipReferenceHeight * static_cast<double>(tile) * record.scale);
            ready[index] = apply_text_overlay_unsafe(image, texture);
        }
        if (!ready[index] && image && set_brush_from_texture_) {
            // Drop the old language's resource ownership as well as hiding it.
            SetBrushFromTextureParameters clear{};
            image->ProcessEvent(set_brush_from_texture_, &clear);
        }
    }
    confirmation_text_ready_ = ready[1] && ready[2] && ready[3] && ready[4];
    for (std::size_t index = 0; index < confirmation_texts_.size(); ++index) {
        const auto& record = confirmation_texts_[index];
        const bool visible = index == 0 || confirmation_.active();
        const bool packaged = index == 0 ? ready[0] : confirmation_text_ready_;
        if (UObject* canvas = record.canvas.Get())
            set_visibility(canvas, set_visibility_, visible && packaged ? kHitTestInvisible : kCollapsed);
        set_visibility(record.native_text.Get(), set_visibility_,
            visible && !packaged ? kHitTestInvisible : kCollapsed);
    }
    return true;
}

void RadarVisibilityHub::set_guide_visibility_unsafe(bool visible) {
    UObject* settings = settings_body_.Get();
    UObject* guide = guide_body_.Get();
    UObject* size = body_size_.Get();
    UObject* scroll = body_scroll_.Get();
    visible = visible && guide_ready_;
    if (!settings || !guide || !size || !scroll) return;
    if (guide_open_ != visible) {
        ScalarParameters offset{};
        scroll->ProcessEvent(get_scroll_offset_, &offset);
        const float old = std::isfinite(offset.value) ? std::max(0.0F, offset.value) : 0.0F;
        if (guide_open_) guide_scroll_offset_ = old;
        else settings_scroll_offset_ = old;
        guide_open_ = visible;
        const double content_height = visible ? kGuideContentHeight : kFooterTop - kMarkerTop;
        ScalarParameters height{static_cast<float>(content_height * authored_unit_scale_)};
        size->ProcessEvent(set_height_override_, &height);
        set_visibility(settings, set_visibility_, visible ? kCollapsed : 4);
        set_visibility(guide, set_visibility_, visible ? kHitTestInvisible : kCollapsed);
        if (UObject* host = host_.Get()) host->ProcessEvent(force_layout_prepass_, nullptr);
        const double maximum = std::max(0.0,
            content_height - viewport_layout_.body_viewport_reference_height) * authored_unit_scale_;
        offset.value = static_cast<float>(std::clamp(static_cast<double>(
            visible ? guide_scroll_offset_ : settings_scroll_offset_), 0.0, maximum));
        scroll->ProcessEvent(set_scroll_offset_, &offset);
    }
    for (std::size_t index = 0; index < guide_label_canvases_.size(); ++index)
        if (UObject* canvas = guide_label_canvases_[index].Get())
            set_visibility(canvas, set_visibility_, guide_ready_ && ((index == 1U) == guide_open_)
                ? kHitTestInvisible : kCollapsed);
}

void RadarVisibilityHub::refresh_guide_unsafe() {
    if (guide_language_ != resolved_ui_language_) {
        guide_language_ = resolved_ui_language_;
        guide_ready_ = guide_body_.Get() && guide_control_.Get()
            && guide_label_canvases_[0].Get() && guide_label_canvases_[1].Get();
        for (const auto& image : guide_images_) guide_ready_ = guide_ready_ && image.Get();
        const wchar_t* prefix = packaged_text_overlay_prefix(resolved_ui_language_);
        const auto path = text_overlay_root_.parent_path() / L"guide"
            / (std::wstring(prefix ? prefix : L"en") + L".tga");
        UObject* texture = guide_ready_ && valid_guide_atlas_file(path)
            ? import_text_overlay_unsafe(host_.Get(), path) : nullptr;
        guide_ready_ = guide_ready_ && texture;
        if (guide_ready_) {
            for (const auto& image : guide_images_)
                guide_ready_ = apply_text_overlay_unsafe(image.Get(), texture) && guide_ready_;
        }
        guide_texture_ = guide_ready_ ? texture : nullptr;
        if (!guide_ready_) {
            for (const auto& handle : guide_images_)
                if (UObject* image = handle.Get(); image && set_brush_from_texture_) {
                    SetBrushFromTextureParameters clear{};
                    image->ProcessEvent(set_brush_from_texture_, &clear);
                }
        }
    }
    if (UObject* text = guide_fallback_text_.Get()) {
        (void)apply_language_font_unsafe(text, resolved_ui_language_);
        set_text(text, set_text_, set_text_value_property_, dswros::radar_guide_text(resolved_ui_language_).guide_button);
        set_visibility(text, set_visibility_, guide_ready_ ? kCollapsed : kHitTestInvisible);
        set_render_opacity(text, set_render_opacity_, 0.4F);
    }
    if (UObject* control = guide_control_.Get()) {
        if (auto* property = CastField<FBoolProperty>(set_is_enabled_value_property_)) {
            std::uint8_t value{};
            property->SetPropertyValue(property->ContainerPtrToValuePtr<void>(&value),
                guide_ready_ && !confirmation_.active() && !confirmation_dismiss_guard_);
            control->ProcessEvent(set_is_enabled_, &value);
        }
    }
    set_guide_visibility_unsafe(guide_open_);
}

bool RadarVisibilityHub::set_confirmation_visibility_unsafe(bool visible, bool guard_background) {
    auto* enabled_property = CastField<FBoolProperty>(set_is_enabled_value_property_);
    UObject* blocker = confirmation_blocker_.Get();
    UObject* yes = confirmation_yes_control_.Get();
    UObject* no = confirmation_no_control_.Get();
    if (!enabled_property || !set_is_enabled_ || !blocker || !yes || !no) return false;
    const auto enable = [this, enabled_property](UObject* widget, bool value) {
        std::uint8_t parameters{};
        enabled_property->SetPropertyValue(
            enabled_property->ContainerPtrToValuePtr<void>(&parameters), value);
        widget->ProcessEvent(set_is_enabled_, &parameters);
    };
    const bool block_background = visible || guard_background;
    if (UObject* guide = guide_control_.Get()) {
        set_checked(guide, set_is_checked_, false);
        enable(guide, !block_background && guide_ready_);
    }
    for (std::size_t index = 0; index < tooltip_count_; ++index) {
        UObject* control = tooltips_[index].control.Get();
        if (!control) return false;
        enable(control, !block_background);
    }
    UObject* popup_dismiss = language_popup_dismiss_control_.Get();
    if (!popup_dismiss) return false;
    enable(popup_dismiss, !block_background);
    set_checked(blocker, set_is_checked_, false);
    set_visibility(blocker, set_visibility_, block_background ? kVisible : kCollapsed);
    for (const auto& handle : confirmation_decorations_) {
        UObject* decoration = handle.Get();
        if (!decoration) return false;
        set_visibility(decoration, set_visibility_, visible ? kHitTestInvisible : kCollapsed);
    }
    for (UObject* control : {yes, no}) {
        set_checked(control, set_is_checked_, false);
        set_visibility(control, set_visibility_, visible ? kVisible : kCollapsed);
    }
    enable(yes, visible && confirmation_text_ready_);
    enable(no, visible);
    set_render_opacity(confirmation_decorations_[3].Get(), set_render_opacity_,
        confirmation_text_ready_ ? 1.0F : 0.35F);
    for (std::size_t index = 1; index < confirmation_texts_.size(); ++index) {
        const auto& record = confirmation_texts_[index];
        if (UObject* canvas = record.canvas.Get())
            set_visibility(canvas, set_visibility_, visible && confirmation_text_ready_ ? kHitTestInvisible : kCollapsed);
        UObject* text = record.native_text.Get();
        if (!text) return false;
        set_visibility(text, set_visibility_, visible && !confirmation_text_ready_ ? kHitTestInvisible : kCollapsed);
        if (index == 3) set_render_opacity(text, set_render_opacity_, confirmation_text_ready_ ? 1.0F : 0.35F);
    }
    confirmation_dismiss_guard_ = guard_background;
    return true;
}

bool RadarVisibilityHub::refresh_numeric_text_unsafe() {
    UObject* texture = tooltip_atlas_.Get();
    bool ready = texture && tooltip_atlas_language_ == resolved_ui_language_;
    for (const auto& record : numeric_texts_)
        ready = ready && record.canvas.Get() && record.slot.Get()
            && record.image.Get() && record.image_slot.Get();
    const auto restore_native = [this] {
        for (const auto& record : numeric_texts_) {
            if (UObject* canvas = record.canvas.Get()) set_visibility(canvas, set_visibility_, kCollapsed);
            if (UObject* image = record.image.Get(); image && set_brush_from_texture_) {
                SetBrushFromTextureParameters clear{};
                image->ProcessEvent(set_brush_from_texture_, &clear);
            }
        }
        for (const auto& handle : scene_value_texts_)
            if (UObject* text = handle.Get()) set_visibility(text, set_visibility_, kHitTestInvisible);
        for (const auto& handle : placeholder_texts_)
            if (UObject* text = handle.Get()) set_visibility(text, set_visibility_, kHitTestInvisible);
    };
    if (!ready) { restore_native(); return false; }
    const double unit = authored_unit_scale_;
    const auto draw = [this, texture, unit](std::size_t index, std::size_t glyph,
        double x, double y) {
        auto& record = numeric_texts_[index];
        if (!apply_text_overlay_unsafe(record.image.Get(), texture)) return false;
        set_slot_vector(record.slot.Get(), set_slot_position_, x * unit, y * unit);
        const auto tile = dswros::kRadarTooltipCount + dswros::kRadarConfirmationTextCount + glyph;
        set_slot_vector(record.image_slot.Get(), set_slot_position_, 0,
            -static_cast<double>(tile) * kTooltipReferenceHeight * unit);
        set_visibility(record.canvas.Get(), set_visibility_, kHitTestInvisible);
        return true;
    };
    const std::array<unsigned, 2> numbers{{pending_scene_settings_.range_meters,
        pending_scene_settings_.marker_limit}};
    for (std::size_t field = 0; field < numbers.size(); ++field) {
        std::array<wchar_t, 16> display{};
        std::swprintf(display.data(), display.size(), field == 0 ? L"%u m" : L"%u", numbers[field]);
        const std::size_t length = std::wcslen(display.data());
        if (length > 6U) { restore_native(); return false; }
        std::array<std::size_t, 6> glyphs{};
        double width{};
        for (std::size_t digit = 0; digit < length; ++digit) {
            const wchar_t* found = std::wcschr(kNumericCharacters, display[digit]);
            if (!found) { restore_native(); return false; }
            glyphs[digit] = static_cast<std::size_t>(found - kNumericCharacters);
            width += kNumericAdvances[glyphs[digit]];
        }
        const auto& origin = numeric_texts_[field * 6U];
        double cursor = origin.authored_x + (88.0 - width) * 0.5;
        for (std::size_t digit = 0; digit < 6U; ++digit) {
            const std::size_t index = field * 6U + digit;
            if (digit >= length) {
                set_visibility(numeric_texts_[index].canvas.Get(), set_visibility_, kCollapsed);
                continue;
            }
            const double advance = kNumericAdvances[glyphs[digit]];
            if (!draw(index, glyphs[digit], cursor + advance * 0.5 - 8.0, origin.authored_y)) {
                restore_native(); return false;
            }
            cursor += advance;
        }
    }
    for (std::size_t index = 12; index < numeric_texts_.size(); ++index) {
        const auto& record = numeric_texts_[index];
        if (!draw(index, index == 14 ? 13U : 12U, record.authored_x, record.authored_y)) {
            restore_native(); return false;
        }
    }
    for (const auto& handle : scene_value_texts_)
        if (UObject* text = handle.Get()) set_visibility(text, set_visibility_, kCollapsed);
    for (const auto& handle : placeholder_texts_)
        if (UObject* text = handle.Get()) set_visibility(text, set_visibility_, kCollapsed);
    return true;
}

bool RadarVisibilityHub::apply_viewport_layout_unsafe(const dswros::HubViewportLayout& layout) {
    UObject* host = host_.Get();
    UObject* page = page_panel_.Get();
    UObject* scroll = body_scroll_.Get();
    if (!layout.valid || !host || !page || !scroll || !page_slot_.Get()
        || !body_scroll_slot_.Get() || !footer_slot_.Get() || !modal_slot_.Get()
        || !confirmation_dim_slot_.Get() || !confirmation_blocker_slot_.Get()
        || !popup_dismiss_slot_.Get() || !std::isfinite(authored_unit_scale_)
        || authored_unit_scale_ <= 0) return false;
    const double authored = authored_unit_scale_;
    ScalarParameters offset{};
    scroll->ProcessEvent(get_scroll_offset_, &offset);
    if (!std::isfinite(offset.value)) return false;
    const double content_height = guide_open_ ? kGuideContentHeight : kFooterTop - kMarkerTop;
    const double maximum = std::max(0.0,
        content_height - layout.body_viewport_reference_height) * authored;
    offset.value = static_cast<float>(std::clamp(static_cast<double>(offset.value), 0.0, maximum));
    VectorParameters pivot{{0.0, 0.0}};
    page->ProcessEvent(set_render_pivot_, &pivot);
    const double scale = layout.unit_scale / authored;
    VectorParameters rendered_scale{{scale, scale}};
    page->ProcessEvent(set_render_scale_, &rendered_scale);
    set_slot_vector(page_slot_.Get(), set_slot_size_, kReferencePanelWidth * authored,
        layout.panel_reference_height * authored);
    set_slot_vector(body_scroll_slot_.Get(), set_slot_size_, kReferencePanelWidth * authored,
        layout.body_viewport_reference_height * authored);
    set_slot_vector(footer_slot_.Get(), set_slot_position_, 0, layout.footer_reference_y * authored);
    set_slot_vector(modal_slot_.Get(), set_slot_position_, 0, layout.modal_reference_y * authored);
    for (const auto& handle : {confirmation_dim_slot_, confirmation_blocker_slot_})
        set_slot_vector(handle.Get(), set_slot_size_, kReferencePanelWidth * authored,
            layout.panel_reference_height * authored);
    if (UObject* dim_slot = popup_dim_slot_.Get())
        set_slot_vector(dim_slot, set_slot_size_, kCardWidth * authored,
            (layout.panel_reference_height - 100.0) * authored);
    set_slot_vector(popup_dismiss_slot_.Get(), set_slot_size_, kCardWidth * authored,
        (layout.panel_reference_height - 98.0) * authored);
    scroll->ProcessEvent(set_scroll_offset_, &offset);
    // Native hover widgets live outside the scrolled page. Resize their retained
    // geometry too, so a DPI change cannot leave tiny or oversized tooltips.
    for (std::size_t index = 0; index < tooltip_count_; ++index) {
        auto& record = tooltips_[index];
        record.unit_scale = layout.unit_scale;
        UObject* content = record.content.Get();
        UObject* image_slot = record.image_slot.Get();
        if (!content || !image_slot) continue;
        ScalarParameters width{static_cast<float>(kTooltipReferenceWidth * layout.unit_scale)};
        ScalarParameters height{static_cast<float>(kTooltipReferenceHeight * layout.unit_scale)};
        content->ProcessEvent(set_width_override_, &width);
        content->ProcessEvent(set_height_override_, &height);
        set_slot_vector(image_slot, set_slot_position_, 0,
            -kTooltipReferenceHeight * static_cast<double>(record.id) * layout.unit_scale);
        set_slot_vector(image_slot, set_slot_size_, kTooltipReferenceWidth * layout.unit_scale,
            kTooltipReferenceHeight * static_cast<double>(kConfirmationAtlasTiles) * layout.unit_scale);
    }
    VectorParameters desired{{kReferencePanelWidth * layout.unit_scale,
        layout.panel_reference_height * layout.unit_scale}};
    host->ProcessEvent(set_desired_size_in_viewport_, &desired);
    PositionInViewportParameters position{{layout.physical_left, layout.physical_top}, true};
    host->ProcessEvent(set_position_in_viewport_, &position);
    host->ProcessEvent(force_layout_prepass_, nullptr);
    viewport_layout_ = layout;
    return true;
}

bool RadarVisibilityHub::refresh_viewport_layout_unsafe(UObject* controller, bool& changed) {
    changed = false;
    const auto now = std::chrono::steady_clock::now();
    if (now < viewport_check_after_) return true;
    viewport_check_after_ = now + std::chrono::milliseconds(250);
    UObject* library = widget_layout_library_.Get();
    if (!controller || !library) return false;
    ViewportSizeParameters size{controller};
    ViewportScaleParameters dpi{controller};
    library->ProcessEvent(get_viewport_size_, &size);
    library->ProcessEvent(get_viewport_scale_, &dpi);
    const auto layout = dswros::compute_hub_viewport_layout(
        size.return_value.x, size.return_value.y, dpi.return_value);
    if (!layout.valid) return false;
    changed = std::abs(layout.unit_scale - viewport_layout_.unit_scale) > 0.000001
        || std::abs(layout.panel_reference_height - viewport_layout_.panel_reference_height) > 0.001
        || std::abs(layout.physical_left - viewport_layout_.physical_left) > 0.01
        || std::abs(layout.physical_top - viewport_layout_.physical_top) > 0.01;
    if (!changed) return true;
    // Cancel before geometry mutation; never interpret an old-position Yes.
    confirmation_.clear();
    if (!set_language_popup_visibility_unsafe(false)
        || !set_confirmation_visibility_unsafe(false, true)) return false;
    return apply_viewport_layout_unsafe(layout);
}

bool RadarVisibilityHub::refresh_localized_text_unsafe() {
    refresh_tooltips_unsafe();
    (void)refresh_numeric_text_unsafe();
    if (!refresh_confirmation_text_unsafe()) return false;
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
        localized.scene_settings,
        localized.scene_range,
        localized.scene_limit,
        localized.scene_distance,
        localized.scene_distance_modes[0],
        localized.scene_distance_modes[1],
        localized.scene_distance_modes[2],
        localized.scene_distance_modes[3],
        localized.marker_categories[static_cast<std::size_t>(RadarVisibilityCategory::Treasure)],
        localized.marker_categories[static_cast<std::size_t>(RadarVisibilityCategory::AreaQuests)],
        localized.marker_categories[static_cast<std::size_t>(RadarVisibilityCategory::MiniGames)],
        localized.restore_defaults,
        localized.all_markers,
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
    if (!refresh_language_value_overlay_unsafe(host_.Get())) {
        return false;
    }
    refresh_guide_unsafe();
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
    set_text(
        status_text, set_text_, set_text_value_property_, status_value);
    set_text(
        action_text, set_text_, set_text_value_property_, status_action);
    if (!refresh_packaged_text_overlay_unsafe(host_.Get())) {
        return false;
    }
    if (!refresh_language_value_overlay_unsafe(host_.Get())) {
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
        // All twelve choices, including AUTO, use the same regular raster path.
        const bool packaged_choice = true;
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
            source_language_, RadarVisibilityHubCommand::None, source_scene_settings_};
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
            pending_language_, RadarVisibilityHubCommand::None, pending_scene_settings_};
    }
    return service_guarded(current_controller, current_mod_status);
}

RadarVisibilityHubResult RadarVisibilityHub::close(
    UObject* current_controller, RadarModStatus current_mod_status) noexcept {
    if (!is_open()) return {};
    return service_guarded(current_controller, current_mod_status, true);
}

RadarVisibilityHubResult RadarVisibilityHub::escape(
    UObject* current_controller, RadarModStatus current_mod_status) noexcept {
    if (!is_open()) return {};
    return confirmation_.active()
        ? service_guarded(current_controller, current_mod_status, false, true)
        : close(current_controller, current_mod_status);
}

RadarVisibilityHubResult RadarVisibilityHub::service_guarded(
    UObject* current_controller,
    RadarModStatus current_mod_status, bool force_close, bool cancel_confirmation) noexcept {
#if defined(_MSC_VER)
    __try {
        return service_unsafe(current_controller, current_mod_status, force_close, cancel_confirmation);
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
        return service_unsafe(current_controller, current_mod_status, force_close, cancel_confirmation);
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
    RadarModStatus current_mod_status, bool force_close, bool cancel_confirmation) {
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
    if ((force_close || cancel_confirmation) && !current_controller) {
        current_controller = owning_player.return_value;
    }
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

    using ConfirmationAction = dswros::RadarConfirmationAction;
    const auto unchanged = [this](RadarVisibilityHubCommand command = RadarVisibilityHubCommand::None) {
        return RadarVisibilityHubResult{RadarVisibilityHubAction::None, source_masks_, false, 0,
            source_area_quest_mode_, source_assault_mode_, source_height_indicators_,
            source_language_, command, source_scene_settings_};
    };
    bool confirmed_reset{};
    if (!force_close && !cancel_confirmation) {
        bool resized{};
        if (!refresh_viewport_layout_unsafe(current_controller, resized)) {
            // Minimized/unsupported viewports close through the normal final
            // sample so the latest slider value is not lost between services.
            force_close = true;
        }
        if (resized && !force_close) return unchanged();
    }
    if (force_close && (confirmation_.active() || confirmation_dismiss_guard_)) {
        confirmation_.clear();
        if (!set_confirmation_visibility_unsafe(false)) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
    } else if (cancel_confirmation && confirmation_.active()) {
        confirmation_.clear();
        if (!set_confirmation_visibility_unsafe(false, true)) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        return unchanged(); // Esc cancels intent; never samples/reset settings.
    } else if (confirmation_.active()) {
        UObject* yes = confirmation_yes_control_.Get();
        UObject* no = confirmation_no_control_.Get();
        if (!yes || !no) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        const bool yes_checked = is_checked(yes, is_checked_);
        const bool no_checked = is_checked(no, is_checked_);
        set_checked(yes, set_is_checked_, false);
        set_checked(no, set_is_checked_, false);
        const auto response = confirmation_.sample(
            confirmation_text_ready_ && yes_checked, no_checked);
        if (!response.dismissed) return unchanged();
        // Intent is consumed before any URL command or preset mutation. Keep
        // the shield for the next service turn to drain the closing UI gesture.
        if (!set_confirmation_visibility_unsafe(false, true)) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        if (response.confirmed == ConfirmationAction::Endorse)
            return unchanged(RadarVisibilityHubCommand::OpenEndorsement);
        if (response.confirmed == ConfirmationAction::BugReport)
            return unchanged(RadarVisibilityHubCommand::OpenBugReport);
        confirmed_reset = response.confirmed == ConfirmationAction::RestoreDefaults;
        if (!confirmed_reset) return unchanged(); // No changes only the modal.
    } else if (confirmation_dismiss_guard_ && !force_close) {
        if (!set_confirmation_visibility_unsafe(false)) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        return unchanged(); // Do not sample a queued click from behind the modal.
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
    const bool close_requested = force_close || is_checked(close_control, is_checked_);
    bool guide_toggle_requested{};
    if (UObject* guide = guide_control_.Get()) {
        guide_toggle_requested = !close_requested && guide_ready_ && is_checked(guide, is_checked_);
        set_checked(guide, set_is_checked_, false);
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
    ConfirmationAction requested_confirmation{ConfirmationAction::None};
    UObject* mod_action_control = mod_action_control_.Get();
    UObject* bug_report_control = bug_report_control_.Get();
    UObject* endorsement_control = endorsement_control_.Get();
    if (!mod_action_control || !bug_report_control || !endorsement_control) {
        last_failure_ = 36;
        detach_unsafe(current_controller);
        return {
            RadarVisibilityHubAction::Rejected,
            source_masks_,
            false,
            last_failure_};
    }
    if (!close_requested && is_checked(mod_action_control, is_checked_)) {
        set_checked(mod_action_control, set_is_checked_, false);
        command = radar_mod_action_for_status(displayed_mod_status_);
    }
    if (!close_requested && is_checked(bug_report_control, is_checked_)) {
        set_checked(bug_report_control, set_is_checked_, false);
        requested_confirmation = ConfirmationAction::BugReport;
    }
    if (!close_requested && is_checked(endorsement_control, is_checked_)) {
        set_checked(endorsement_control, set_is_checked_, false);
        if (requested_confirmation == ConfirmationAction::None)
            requested_confirmation = ConfirmationAction::Endorse;
    }

    UObject* global_reset_control = global_reset_control_.Get();
    if (!global_reset_control) {
        last_failure_ = 41;
        detach_unsafe(current_controller);
        return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
    }
    if (!close_requested && is_checked(global_reset_control, is_checked_)) {
        set_checked(global_reset_control, set_is_checked_, false);
        if (requested_confirmation == ConfirmationAction::None)
            requested_confirmation = ConfirmationAction::RestoreDefaults;
    }
    const bool global_reset_requested = !close_requested && confirmed_reset;
    if (global_reset_requested) {
        const dswros::SceneDisplaySettings defaults{};
        // One transaction restores every displayed preference. Running state
        // and startup hotkeys are deliberately outside this preset.
        command = RadarVisibilityHubCommand::None;
        for (std::size_t column = 0; column < kColumnCount; ++column) {
            for (std::size_t category = 0; category < kCategoryCount; ++category) {
                if ((kColumnCategories[column] & (1U << category)) == 0U) continue;
                UObject* control = controls_[column][category].Get();
                if (!control) {
                    last_failure_ = 41;
                    detach_unsafe(current_controller);
                    return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
                }
                set_checked(control, set_is_checked_, true);
            }
        }
        for (std::size_t index = 0; index < height_controls_.size(); ++index) {
            UObject* control = height_controls_[index].Get();
            if (!control) {
                last_failure_ = 41;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            set_checked(control, set_is_checked_, dswros::height_indicator_enabled(
                dswros::kDefaultHeightIndicatorMask,
                static_cast<dswros::HeightIndicatorCategory>(index)));
        }
        for (const auto& handle : column_all_controls_) {
            UObject* control = handle.Get();
            if (!control) {
                last_failure_ = 41;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            set_checked(control, set_is_checked_, true);
        }
        for (auto* control : {area_mode_available_control_.Get(), area_mode_all_control_.Get(),
                              assault_mode_current_control_.Get(), assault_mode_all_control_.Get()}) {
            if (!control) {
                last_failure_ = 41;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            set_checked(control, set_is_checked_, control == area_mode_available_control_.Get()
                || control == assault_mode_current_control_.Get());
        }
        pending_language_ = dswros::RadarLanguagePreference::Auto;
        resolved_ui_language_ = dswros::resolve_radar_ui_language(
            pending_language_, detected_game_language_);
        if (!refresh_localized_text_unsafe() || !set_language_popup_visibility_unsafe(false)) {
            last_failure_ = 41;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        const std::array<float, 2> default_slider_values{{
            static_cast<float>(defaults.range_meters) / 1000.0F,
            static_cast<float>(defaults.marker_limit) / 50.0F}};
        for (std::size_t index = 0; index < scene_sliders_.size(); ++index) {
            UObject* slider = scene_sliders_[index].Get();
            if (!slider) {
                last_failure_ = 41;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            ScalarParameters value{default_slider_values[index]};
            slider->ProcessEvent(set_slider_value_, &value);
        }
        for (std::size_t index = 0; index < scene_distance_controls_.size(); ++index) {
            UObject* control = scene_distance_controls_[index].Get();
            if (!control) {
                last_failure_ = 41;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            set_checked(control, set_is_checked_,
                index == static_cast<std::size_t>(defaults.distance_mode));
        }
    }

    const std::array<unsigned, 2> slider_maximum{{1000U, 50U}};
    for (std::size_t index = 0; index < scene_sliders_.size(); ++index) {
        UObject* slider = scene_sliders_[index].Get();
        UObject* text = scene_value_texts_[index].Get();
        if (!slider || !text) {
            last_failure_ = 39;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        ScalarParameters sample{};
        slider->ProcessEvent(get_slider_value_, &sample);
        if (!std::isfinite(sample.value)) {
            last_failure_ = 39;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        const auto value = static_cast<unsigned>(std::lround(
            std::clamp(sample.value, 0.0F, 1.0F)
                * static_cast<float>(slider_maximum[index])));
        const unsigned previous = index == 0 ? pending_scene_settings_.range_meters
                                             : pending_scene_settings_.marker_limit;
        if (value != previous) {
            if (index == 0) pending_scene_settings_.range_meters = static_cast<std::uint16_t>(value);
            else pending_scene_settings_.marker_limit = static_cast<std::uint8_t>(value);
            std::array<wchar_t, 24> display{};
            std::swprintf(display.data(), display.size(), index == 0 ? L"%u m" : L"%u", value);
            set_text(text, set_text_, set_text_value_property_, display.data());
            (void)refresh_numeric_text_unsafe();
        }
    }
    std::size_t selected_distance = static_cast<std::size_t>(pending_scene_settings_.distance_mode);
    for (std::size_t index = 0; index < scene_distance_controls_.size(); ++index) {
        UObject* control = scene_distance_controls_[index].Get();
        if (!control || !scene_distance_visuals_[index].Get()) {
            last_failure_ = 39;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        if (index != static_cast<std::size_t>(pending_scene_settings_.distance_mode)
            && is_checked(control, is_checked_)) selected_distance = index;
    }
    pending_scene_settings_.distance_mode = static_cast<dswros::SceneDistanceMode>(selected_distance);
    for (std::size_t index = 0; index < scene_distance_controls_.size(); ++index) {
        const bool selected = index == selected_distance;
        set_checked(scene_distance_controls_[index].Get(), set_is_checked_, selected);
        set_visibility(scene_distance_visuals_[index].Get(), set_visibility_, selected ? kVisible : kCollapsed);
    }

    std::uint8_t compact = compact_radar_visibility_mask(pending_masks_);
    std::uint8_t world = world_radar_visibility_mask(pending_masks_);
    std::uint8_t scene = scene_radar_visibility_mask(pending_masks_);
    // Compare with the last displayed aggregate, before reading member states.
    // A changed member must only update All's display, never rewrite its peers.
    for (std::size_t column = 0; column < column_all_controls_.size(); ++column) {
        UObject* control = column_all_controls_[column].Get();
        if (!control || !column_all_visuals_[column].Get()) {
            last_failure_ = 22;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
        const bool selected = is_checked(control, is_checked_);
        if (selected == column_all_selected_[column]) continue;
        for (std::size_t category = 0; category < kCategoryCount; ++category) {
            if ((kColumnCategories[column] & (1U << category)) == 0U) continue;
            UObject* member = controls_[column][category].Get();
            if (!member) {
                last_failure_ = 22;
                detach_unsafe(current_controller);
                return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
            }
            set_checked(member, set_is_checked_, selected);
        }
    }
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        for (std::size_t category = 0; category < kCategoryCount; ++category) {
            if ((kColumnCategories[column] & (1U << category)) == 0U) {
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
            std::uint8_t& mask = column == 0 ? compact
                : column == 1 ? world : scene;
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
    pending_masks_ = pack_radar_visibility_masks(compact, world, scene);
    for (std::size_t column = 0; column < column_all_controls_.size(); ++column) {
        const auto mask = column == 0 ? compact : world;
        const bool selected = (mask & kColumnCategories[column]) == kColumnCategories[column];
        if (selected != column_all_selected_[column]) {
            set_checked(column_all_controls_[column].Get(), set_is_checked_, selected);
            set_visibility(column_all_visuals_[column].Get(), set_visibility_,
                selected ? kVisible : kCollapsed);
        }
        column_all_selected_[column] = selected;
    }

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

    if (!close_requested && requested_confirmation != ConfirmationAction::None) {
        command = RadarVisibilityHubCommand::None;
        if (!confirmation_.begin(requested_confirmation)
            || !set_language_popup_visibility_unsafe(false)
            || !refresh_confirmation_text_unsafe()
            || !set_confirmation_visibility_unsafe(true)) {
            last_failure_ = 42;
            detach_unsafe(current_controller);
            return {RadarVisibilityHubAction::Rejected, source_masks_, false, last_failure_};
        }
    }
    // Sample pending settings before changing pages, including a slider's last
    // drag value. Returning from the read-only guide restores its prior scroll.
    if (guide_toggle_requested && !confirmation_.active()) {
        (void)set_language_popup_visibility_unsafe(false);
        set_guide_visibility_unsafe(!guide_open_);
    }
    const bool changed = !(pending_masks_ == source_masks_
        && pending_area_quest_mode_ == source_area_quest_mode_
        && pending_assault_mode_ == source_assault_mode_
        && pending_height_indicators_ == source_height_indicators_
        && pending_language_ == source_language_
        && dswros::scene_display_settings_equal(pending_scene_settings_, source_scene_settings_));
    if (!changed && !close_requested && !global_reset_requested) {
        return {
            RadarVisibilityHubAction::None,
            pending_masks_,
            false,
            0,
            pending_area_quest_mode_,
            pending_assault_mode_,
            pending_height_indicators_,
            pending_language_,
            command, pending_scene_settings_};
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
    source_scene_settings_ = pending_scene_settings_;
    if (changed) ++apply_count_;
    if (close_requested) {
        const auto applied_scene_settings = source_scene_settings_;
        detach_unsafe(current_controller);
        ++close_count_;
        return {RadarVisibilityHubAction::Closed, applied, changed, 0,
                applied_mode, applied_assault_mode, applied_height_indicators,
                applied_language, RadarVisibilityHubCommand::None, applied_scene_settings};
    }
    return {
        RadarVisibilityHubAction::Applied,
        applied,
        changed,
        0,
        applied_mode,
        applied_assault_mode,
        applied_height_indicators,
        applied_language,
        command, source_scene_settings_, global_reset_requested};
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
    // A consumed press retains its repeat/release guard after UMG is gone.
    hub_escape_input::panel_closed();
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
    hub_escape_input::reset();
    const bool was_open = state_ == RadarVisibilityHubState::Open;
    reset_runtime_handles();
    if (was_open) {
        ++detach_count_;
        state_ = RadarVisibilityHubState::Ready;
    }
}

void RadarVisibilityHub::reset_runtime_handles() noexcept {
    confirmation_.clear();
    confirmation_text_ready_ = false;
    confirmation_dismiss_guard_ = false;
    for (auto& record : confirmation_texts_) record = ConfirmationTextRecord{};
    confirmation_decorations_.fill(FWeakObjectPtr{});
    confirmation_blocker_ = FWeakObjectPtr{};
    confirmation_yes_control_ = FWeakObjectPtr{};
    confirmation_no_control_ = FWeakObjectPtr{};
    endorsement_control_ = FWeakObjectPtr{};
    host_ = FWeakObjectPtr{};
    widget_tree_ = FWeakObjectPtr{};
    root_panel_ = FWeakObjectPtr{};
    page_panel_ = FWeakObjectPtr{};
    page_slot_ = FWeakObjectPtr{};
    body_scroll_ = FWeakObjectPtr{};
    settings_body_ = FWeakObjectPtr{};
    body_size_ = FWeakObjectPtr{};
    guide_body_ = FWeakObjectPtr{};
    guide_control_ = FWeakObjectPtr{};
    guide_fallback_text_ = FWeakObjectPtr{};
    guide_texture_ = FWeakObjectPtr{};
    guide_images_.fill(FWeakObjectPtr{});
    guide_label_canvases_.fill(FWeakObjectPtr{});
    guide_language_ = dswros::RadarUiLanguage::Count;
    guide_open_ = false;
    guide_ready_ = false;
    settings_scroll_offset_ = 0.0F;
    guide_scroll_offset_ = 0.0F;
    body_scroll_slot_ = FWeakObjectPtr{};
    footer_slot_ = FWeakObjectPtr{};
    modal_slot_ = FWeakObjectPtr{};
    confirmation_dim_slot_ = FWeakObjectPtr{};
    confirmation_blocker_slot_ = FWeakObjectPtr{};
    popup_dim_slot_ = FWeakObjectPtr{};
    popup_dismiss_slot_ = FWeakObjectPtr{};
    authored_unit_scale_ = 1.0;
    viewport_layout_ = {};
    viewport_check_after_ = {};
    main_text_overlay_images_.fill(FWeakObjectPtr{});
    column_all_controls_.fill(FWeakObjectPtr{});
    column_all_visuals_.fill(FWeakObjectPtr{});
    column_all_selected_.fill(false);
    for (auto& record : numeric_texts_) record = NumericTextRecord{};
    placeholder_texts_.fill(FWeakObjectPtr{});
    for (auto& tooltip : tooltips_) tooltip = TooltipRecord{};
    tooltip_count_ = 0;
    tooltip_atlas_ = FWeakObjectPtr{};
    tooltip_atlas_language_ = dswros::RadarUiLanguage::Count;
    tooltip_atlas_attempted_ = false;
    main_text_overlay_image_ = FWeakObjectPtr{};
    popup_text_overlay_image_ = FWeakObjectPtr{};
    language_value_overlay_image_ = FWeakObjectPtr{};
    main_text_overlay_texture_ = FWeakObjectPtr{};
    popup_text_overlay_texture_ = FWeakObjectPtr{};
    for (auto& texture : language_value_overlay_textures_) {
        texture = FWeakObjectPtr{};
    }
    language_value_overlay_failures_ = {};
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
    global_reset_control_ = FWeakObjectPtr{};
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
    for (auto& value : scene_sliders_) value = FWeakObjectPtr{};
    for (auto& value : scene_value_texts_) value = FWeakObjectPtr{};
    for (auto& value : scene_distance_controls_) value = FWeakObjectPtr{};
    for (auto& value : scene_distance_visuals_) value = FWeakObjectPtr{};
    text_overlay_language_ = dswros::RadarUiLanguage::Count;
    text_overlay_status_ = RadarModStatus::Off;
    text_overlay_active_ = false;
    language_dropdown_expanded_ = false;
    owns_input_mode_ = false;
    previous_cursor_visible_ = false;
}

} // namespace dsnwr
