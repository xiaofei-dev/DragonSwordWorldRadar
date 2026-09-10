#include "compact_umg_renderer.hpp"

#include <dswros/compact_render_model.hpp>
#include <dswros/compact_clock_layout.hpp>
#include <dswros/encounter_height.hpp>
#include <dswros/render_projection.hpp>

#pragma warning(push)
#pragma warning(disable : 4324 4251 5038)
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/UClass.hpp>
#include <Unreal/FProperty.hpp>
#else
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#endif
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include "ue4ss_compat.hpp"
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>

#include <windows.h>

namespace dsnwr {
namespace {

using namespace RC::Unreal;

struct Vector2D {
    double x{};
    double y{};
};

struct LineSegment {
    Vector2D start{};
    Vector2D end{};
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

struct IntReturnParameters {
    std::int32_t return_value{};
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

struct SlateBrushParameters {
    std::array<std::byte, 208> brush{};
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
static_assert(sizeof(IntReturnParameters) == 4);
static_assert(sizeof(VisibilityParameters) == 1);
static_assert(sizeof(LinearColor) == 16);
static_assert(sizeof(BrushColorParameters) == 16);
static_assert(sizeof(SlateBrushParameters) == 208);
static_assert(sizeof(ScalarParameters) == 4);

constexpr std::uint8_t kVisible = 0;
constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
constexpr std::int32_t kViewportZOrder = 2'000'000'000;
constexpr double kReferenceCenterRight = 220.0;
constexpr double kReferenceCenterY = 217.0;
constexpr double kReferenceMarkerExtent = 170.0;
constexpr double kMinimumReferenceMarkerSize = 4.0;
constexpr double kMaximumReferenceMarkerSize = 65.0;
constexpr double kReferenceTreasureHalfWidth = 0.55;
constexpr double kReferenceHeightLength = 26.0;
constexpr double kReferenceHeightClearance = 2.5;
constexpr double kReferenceHeightHeadLength = 10.0;
constexpr double kReferenceHeightHeadHalfWidth = 7.5;
constexpr double kReferenceHeightOutlineWidth = 7.5;
constexpr double kReferenceHeightInnerWidth = 3.25;
constexpr double kReferenceHeightEndCapInset = 2.0;
constexpr double kReferenceHeightTailInset = 3.0;
constexpr double kReferenceMiniGameTriangleHalfWidth = 7.25;
constexpr double kReferenceMiniGameTriangleHalfHeight = 6.0;
constexpr double kReferenceMiniGameTriangleFillBarHeight = 3.25;
constexpr double kReferenceMiniGameTriangleOutlineWidth = 2.4;
constexpr double kReferenceMiniGameTriangleClearance = 1.5;
constexpr double kAreaQuestMarkerTriangleHalfWidth = 0.48;
constexpr double kAreaQuestMarkerTriangleHalfHeight = 0.52;
// Boss, Assault and Area Quest retain distinct icon extents, but share one
// visible stroke in reference units. Only the display/DPI scale changes it.
constexpr double kBandMarkerReferenceStroke = 4.0;
constexpr double kEncounterTriangleReferenceOutline = 1.0;
constexpr double kReferenceHeightGroupHalfSize =
    kReferenceHeightLength + kReferenceHeightHeadHalfWidth
        + kReferenceHeightOutlineWidth * 0.5;
constexpr double kHeightAngleEpsilonDegrees = 0.15;
constexpr double kReferenceHostHalfSize =
    kReferenceMarkerExtent + kMaximumReferenceMarkerSize * 0.5
        + kReferenceHeightLength + kReferenceHeightClearance
        + kReferenceHeightHeadHalfWidth
        + kReferenceHeightOutlineWidth * 0.5;
static_assert(kReferenceHostHalfSize == dswros::kCompactReferenceHostHalfSize);
static_assert(kReferenceCenterRight == dswros::kCompactReferenceCenterRight);
static_assert(kReferenceCenterY == dswros::kCompactReferenceCenterY);
constexpr double kViewportDimensionEpsilon = 0.5;
constexpr double kViewportScaleEpsilon = 1.0e-6;
constexpr double kRadiansToDegrees = 57.2957795130823208768;
constexpr double kDegreesToRadians = 0.01745329251994329577;

constexpr LinearColor kOutline{14.0F / 255.0F, 20.0F / 255.0F,
                               30.0F / 255.0F, 1.0F};
constexpr LinearColor kTreasureOther{1.0F, 1.0F, 1.0F, 1.0F};
constexpr LinearColor kTreasureMiniGame{63.0F / 255.0F, 230.0F / 255.0F,
                                        122.0F / 255.0F, 1.0F};
constexpr LinearColor kTreasureMap{1.0F, 162.0F / 255.0F,
                                   47.0F / 255.0F, 1.0F};
constexpr LinearColor kTreasurePuzzle{67.0F / 255.0F, 159.0F / 255.0F,
                                      1.0F, 1.0F};
constexpr LinearColor kFlyWing{91.0F / 255.0F, 178.0F / 255.0F,
                                244.0F / 255.0F, 1.0F};
constexpr LinearColor kFlyArrow{35.0F / 255.0F, 120.0F / 255.0F,
                                 218.0F / 255.0F, 1.0F};
constexpr LinearColor kFlyOutline{18.0F / 255.0F, 51.0F / 255.0F,
                                   84.0F / 255.0F, 1.0F};
constexpr LinearColor kHammer{210.0F / 255.0F, 145.0F / 255.0F,
                              82.0F / 255.0F, 1.0F};
constexpr LinearColor kMoleHeightOutline{76.0F / 255.0F, 45.0F / 255.0F,
                                         26.0F / 255.0F, 1.0F};
constexpr LinearColor kWave{50.0F / 255.0F, 91.0F / 255.0F,
                            224.0F / 255.0F, 1.0F};
constexpr LinearColor kWaveOutline{17.0F / 255.0F, 31.0F / 255.0F,
                                   92.0F / 255.0F, 1.0F};
constexpr LinearColor kOfficialWhite{247.0F / 255.0F, 1.0F,
                                     253.0F / 255.0F, 1.0F};
constexpr LinearColor kOfficialPale{216.0F / 255.0F, 246.0F / 255.0F,
                                    238.0F / 255.0F, 1.0F};
constexpr LinearColor kOfficialGreenDark{21.0F / 255.0F, 60.0F / 255.0F,
                                         62.0F / 255.0F, 1.0F};
constexpr LinearColor kOfficialCyan{85.0F / 255.0F, 237.0F / 255.0F,
                                      228.0F / 255.0F, 1.0F};
static_assert(dswros::is_encounter_height_marker(CompactUmgMarkerKind::Boss));
static_assert(dswros::is_encounter_height_marker(CompactUmgMarkerKind::Assault));
static_assert(!dswros::is_encounter_height_marker(CompactUmgMarkerKind::AreaQuest));
static_assert(!dswros::is_encounter_height_marker(CompactUmgMarkerKind::TreasureOther));
constexpr LinearColor kAreaQuestBackdrop{8.0F / 255.0F, 12.0F / 255.0F,
                                          18.0F / 255.0F, 0.55F};
constexpr LinearColor kBirdEggShell{1.0F, 244.0F / 255.0F,
                                    205.0F / 255.0F, 1.0F};
constexpr LinearColor kBirdEggHighlight{1.0F, 1.0F, 1.0F, 1.0F};
constexpr LinearColor kBirdEggNest{116.0F / 255.0F, 76.0F / 255.0F,
                                   45.0F / 255.0F, 1.0F};
constexpr LinearColor kClockDigit{1.0F, 1.0F, 1.0F, 1.0F};
constexpr LinearColor kClockDial{29.0F / 255.0F, 38.0F / 255.0F,
                                 49.0F / 255.0F, 0.94F};
constexpr LinearColor kClockMorning{1.0F, 220.0F / 255.0F,
                                    111.0F / 255.0F, 1.0F};
constexpr LinearColor kClockAfternoon{1.0F, 190.0F / 255.0F,
                                      62.0F / 255.0F, 1.0F};
constexpr LinearColor kClockEvening{1.0F, 132.0F / 255.0F,
                                    61.0F / 255.0F, 1.0F};
constexpr LinearColor kClockMoon{210.0F / 255.0F, 228.0F / 255.0F,
                                 1.0F, 1.0F};

struct MarkerPieceStyle {
    double width{};
    double height{};
    double offset_x{};
    double offset_y{};
    double angle_degrees{};
    LinearColor color{};
};

[[nodiscard]] LinearColor treasure_color(CompactUmgMarkerKind kind) noexcept {
    switch (kind) {
    case CompactUmgMarkerKind::TreasureMiniGame:
        return kTreasureMiniGame;
    case CompactUmgMarkerKind::TreasureMap:
        return kTreasureMap;
    case CompactUmgMarkerKind::TreasurePuzzle:
        return kTreasurePuzzle;
    default:
        return kTreasureOther;
    }
}

[[nodiscard]] LinearColor height_pointer_fill_color(
    CompactUmgMarkerKind kind) noexcept {
    switch (kind) {
    case CompactUmgMarkerKind::Fly:
        return kFlyWing;
    case CompactUmgMarkerKind::Mole:
        return kHammer;
    case CompactUmgMarkerKind::Wave:
        return kWave;
    default:
        return treasure_color(kind);
    }
}

[[nodiscard]] LinearColor height_pointer_outline_color(
    CompactUmgMarkerKind kind) noexcept {
    switch (kind) {
    case CompactUmgMarkerKind::Fly:
        return kFlyOutline;
    case CompactUmgMarkerKind::Mole:
        return kMoleHeightOutline;
    case CompactUmgMarkerKind::Wave:
        return kWaveOutline;
    default:
        return kOutline;
    }
}

[[nodiscard]] MarkerPieceStyle marker_piece_style(
    CompactUmgMarkerKind kind, std::size_t piece,
    double reference_size) noexcept {
    if (kind <= CompactUmgMarkerKind::TreasurePuzzle) {
        const LinearColor fill = treasure_color(kind);

        switch (piece) {
        case 0: return {1.10, 0.88, 0.0, 0.06, 0.0, kOutline};
        case 1: return {0.82, 0.46, 0.0, 0.18, 0.0, fill};
        case 2: return {0.86, 0.22, 0.0, -0.21, 0.0, fill};
        default: return {0.18, 0.30, 0.0, 0.13, 0.0, kOutline};
        }
    }
    const double encounter_inner_extent = std::max(
        0.0, 0.98 - 2.0 * kBandMarkerReferenceStroke
            / std::max(1.0, reference_size));
    switch (kind) {
    case CompactUmgMarkerKind::Boss:
        switch (piece) {
        case 0: return {0.98, 0.98, 0.0, 0.0, 45.0, kOfficialWhite};
        case 1: return {encounter_inner_extent, encounter_inner_extent,
                       0.0, 0.0, 45.0, kOfficialGreenDark};
        case 2: return {0.48, 0.14, -0.12, -0.02, 38.0, kOfficialWhite};
        default: return {0.48, 0.14, 0.12, -0.02, -38.0, kOfficialWhite};
        }
    case CompactUmgMarkerKind::Assault:
        switch (piece) {
        case 0: return {0.98, 0.98, 0.0, 0.0, 45.0, kOfficialPale};
        case 1: return {encounter_inner_extent, encounter_inner_extent,
                       0.0, 0.0, 45.0, kOfficialGreenDark};
        case 2: return {0.15, 0.48, 0.0, -0.10, 0.0, kOfficialCyan};
        default: return {0.18, 0.18, 0.0, 0.28, 0.0, kOfficialCyan};
        }
    case CompactUmgMarkerKind::Fly:
        switch (piece) {
        case 0: return {0.52, 0.18, -0.23, -0.07, -25.0, kFlyWing};
        case 1: return {0.52, 0.18, 0.23, -0.07, 25.0, kFlyWing};
        case 2: return {0.17, 0.58, 0.0, 0.09, 0.0, kFlyArrow};
        default: return {0.34, 0.34, 0.0, -0.24, 45.0, kFlyArrow};
        }
    case CompactUmgMarkerKind::Mole:
        switch (piece) {
        case 0: return {0.19, 0.76, -0.03, 0.03, -45.0, kOutline};
        case 1: return {0.10, 0.68, -0.03, 0.03, -45.0, kHammer};
        case 2: return {0.60, 0.31, 0.18, -0.18, -45.0, kOutline};
        default: return {0.49, 0.20, 0.18, -0.18, -45.0, kHammer};
        }
    case CompactUmgMarkerKind::Wave:
        switch (piece) {
        case 0: return {0.76, 0.13, -0.04, -0.27, -18.0, kWave};
        case 1: return {0.60, 0.13, 0.08, -0.09, -18.0, kWave};
        case 2: return {0.72, 0.13, -0.06, 0.10, -18.0, kWave};
        default: return {0.54, 0.13, 0.10, 0.28, -18.0, kWave};
        }
    case CompactUmgMarkerKind::AreaQuest:
        switch (piece) {
        case 0: return {0.92, 0.92, 0.0, 0.0, 45.0, kTreasureOther};
        case 1: return {0.16, 0.16, -0.22, 0.02, 0.0, kTreasureOther};
        case 2: return {0.16, 0.16, 0.0, 0.02, 0.0, kTreasureOther};
        default: return {0.16, 0.16, 0.22, 0.02, 0.0, kTreasureOther};
        }
    case CompactUmgMarkerKind::BirdEgg:
        switch (piece) {
        case 0: return {0.76, 1.00, 0.0, 0.0, 0.0, kOutline};
        case 1: return {0.58, 0.80, 0.0, 0.02, 0.0, kBirdEggShell};
        case 2: return {0.12, 0.17, -0.12, -0.18, 0.0, kBirdEggHighlight};
        default: return {0.58, 0.14, 0.0, 0.35, 0.0, kBirdEggNest};
        }
    default:
        return {1.0, 1.0, 0.0, 0.0, 0.0, kOutline};
    }
}

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

void set_visibility(UObject* widget, UFunction* function, std::uint8_t visibility) {
    VisibilityParameters parameters{visibility};
    widget->ProcessEvent(function, &parameters);
}

void set_slot_vector(UObject* slot, UFunction* function, double x, double y) {
    VectorParameters parameters{{x, y}};
    slot->ProcessEvent(function, &parameters);
}

void set_brush_color(
    UObject* border, UFunction* function, const LinearColor& color) {
    BrushColorParameters parameters{color};
    border->ProcessEvent(function, &parameters);
}

void set_brush(
    UObject* border, UFunction* function,
    const std::array<std::byte, 208>& brush) {
    SlateBrushParameters parameters{brush};
    border->ProcessEvent(function, &parameters);
}

void write_brush_float(
    std::array<std::byte, 208>& brush,
    std::size_t offset, float value) noexcept {
    std::memcpy(brush.data() + offset, &value, sizeof(value));
}

void configure_area_quest_outline_brush(
    std::array<std::byte, 208>& brush, double umg_unit_scale) noexcept {
    // Current pinned game ABI: FSlateBrush is 0xD0 bytes. A RoundedBox brush
    // gives the task marker a true frame without adding any UMG widgets.
    brush[0x11] = std::byte{4}; // ESlateBrushDrawType::RoundedBox
    write_brush_float(brush, 0x30, kAreaQuestBackdrop.red);
    write_brush_float(brush, 0x34, kAreaQuestBackdrop.green);
    write_brush_float(brush, 0x38, kAreaQuestBackdrop.blue);
    write_brush_float(brush, 0x3C, kAreaQuestBackdrop.alpha);
    brush[0x40] = std::byte{0}; // UseColor_Specified
    write_brush_float(brush, 0x70, kOutline.red);
    write_brush_float(brush, 0x74, kOutline.green);
    write_brush_float(brush, 0x78, kOutline.blue);
    write_brush_float(brush, 0x7C, 1.0F);
    brush[0x80] = std::byte{0}; // UseColor_Specified
    write_brush_float(brush, 0x84,
        static_cast<float>(kBandMarkerReferenceStroke * umg_unit_scale));
    brush[0x88] = std::byte{0}; // FixedRadius
    brush[0x89] = std::byte{0};
}

void configure_fly_outline_brush(
    std::array<std::byte, 208>& brush) noexcept {
    // Use the existing four Fly widgets as rounded boxes with a native Slate
    // outline. This improves compact-map contrast without adding widgets or
    // draw layers.
    brush[0x11] = std::byte{4}; // ESlateBrushDrawType::RoundedBox
    write_brush_float(brush, 0x70, kFlyOutline.red);
    write_brush_float(brush, 0x74, kFlyOutline.green);
    write_brush_float(brush, 0x78, kFlyOutline.blue);
    write_brush_float(brush, 0x7C, kFlyOutline.alpha);
    brush[0x80] = std::byte{0}; // UseColor_Specified
    write_brush_float(brush, 0x84, 1.25F);
    brush[0x88] = std::byte{0}; // FixedRadius
    brush[0x89] = std::byte{0};
}

void configure_encounter_triangle_brush(
    std::array<std::byte, 208>& brush, double umg_unit_scale) noexcept {
    // Each of the three retained edge widgets gets a dark-green Slate outline.
    // The category tint supplies the bright inner edge without extra widgets.
    brush[0x11] = std::byte{4}; // ESlateBrushDrawType::RoundedBox
    write_brush_float(brush, 0x70, kOfficialGreenDark.red);
    write_brush_float(brush, 0x74, kOfficialGreenDark.green);
    write_brush_float(brush, 0x78, kOfficialGreenDark.blue);
    write_brush_float(brush, 0x7C, kOfficialGreenDark.alpha);
    brush[0x80] = std::byte{0}; // UseColor_Specified
    write_brush_float(brush, 0x84,
        static_cast<float>(kEncounterTriangleReferenceOutline * umg_unit_scale));
    brush[0x88] = std::byte{0}; // FixedRadius
    brush[0x89] = std::byte{0};
}

void configure_bird_egg_oval_brush(
    std::array<std::byte, 208>& brush) noexcept {
    // HalfHeightRadius turns the deliberately narrow shell pieces into smooth
    // vertical ovals and the nest piece into a capsule. Reuse the existing four
    // widgets: this changes only their Slate geometry and adds no widget,
    // layer, update, or allocation.
    brush[0x11] = std::byte{4}; // ESlateBrushDrawType::RoundedBox
    brush[0x88] = std::byte{1}; // HalfHeightRadius
    brush[0x89] = std::byte{0};
}

void configure_clock_phase_brush(
    std::array<std::byte, 208>& brush) noexcept {
    // RoundedBox plus HalfHeightRadius turns the retained solar center into a
    // disc and gives rays, horizons, crescent strokes, and the star clean
    // rounded endpoints. This is configured once at attachment and adds no
    // widget or steady-state update work.
    brush[0x11] = std::byte{4}; // ESlateBrushDrawType::RoundedBox
    brush[0x88] = std::byte{1}; // HalfHeightRadius
    brush[0x89] = std::byte{0};
}

void set_render_angle(UObject* widget, UFunction* function, double degrees) {
    ScalarParameters parameters{static_cast<float>(degrees)};
    widget->ProcessEvent(function, &parameters);
}

void set_marker_piece_geometry(
    UObject* slot, UFunction* set_position, UFunction* set_size,
    const MarkerPieceStyle& style, double center_x, double center_y,
    double icon_size) {
    set_slot_vector(
        slot, set_position,
        center_x + style.offset_x * icon_size,
        center_y + style.offset_y * icon_size);
    set_slot_vector(
        slot, set_size,
        style.width * icon_size,
        style.height * icon_size);
}

void set_line_geometry(
    UObject* slot, UFunction* set_position, UFunction* set_size,
    double start_x, double start_y, double end_x, double end_y,
    double thickness) {
    const double dx = end_x - start_x;
    const double dy = end_y - start_y;
    set_slot_vector(
        slot, set_position,
        (start_x + end_x) * 0.5,
        (start_y + end_y) * 0.5);
    set_slot_vector(slot, set_size, std::hypot(dx, dy), thickness);
}

[[nodiscard]] LineSegment sharp_head_segment(
    double tip_x, double tip_y, double base_x, double base_y,
    double thickness, bool upper) noexcept {
    const double relative_base_x = base_x - tip_x;
    const double relative_base_y = upper
        ? base_y - tip_y : -(base_y - tip_y);
    const double distance_squared =
        relative_base_x * relative_base_x
        + relative_base_y * relative_base_y;
    const double radius = thickness * 0.5;
    if (!std::isfinite(distance_squared)
        || distance_squared <= radius * radius) {
        return {{tip_x, tip_y}, {base_x, base_y}};
    }

    // A rotated Border is a square-ended rectangle. Place its near endpoint at
    // the tangent point of a radius-thickness circle around the desired tip so
    // the upper and lower rectangles share exactly one outer corner instead of
    // leaving the old fork-shaped gap.
    const double tangent_scale =
        radius * std::sqrt(distance_squared - radius * radius)
        / distance_squared;
    const double radial_scale = radius * radius / distance_squared;
    const double start_x = tip_x
        + radial_scale * relative_base_x
        - tangent_scale * relative_base_y;
    const double upper_start_y = tip_y
        + radial_scale * relative_base_y
        + tangent_scale * relative_base_x;
    return {
        {start_x, upper ? upper_start_y : 2.0 * tip_y - upper_start_y},
        {base_x, base_y}};
}

[[nodiscard]] std::array<LineSegment, 3> height_outline_segments(
    double unit_scale) noexcept {
    const double length = kReferenceHeightLength * unit_scale;
    const double head_length = kReferenceHeightHeadLength * unit_scale;
    const double head_half_width =
        kReferenceHeightHeadHalfWidth * unit_scale;
    const double thickness = kReferenceHeightOutlineWidth * unit_scale;
    const double shaft_head_overlap = 1.0 * unit_scale;
    const LineSegment upper = sharp_head_segment(
        0.0, 0.0, -head_length, head_half_width, thickness, true);
    const LineSegment lower = sharp_head_segment(
        0.0, 0.0, -head_length, -head_half_width, thickness, false);
    return {{
        {{-length, 0.0}, {-head_length + shaft_head_overlap, 0.0}},
        upper,
        lower,
    }};
}

[[nodiscard]] double height_arrow_right_extent(
    double height_angle_degrees, double unit_scale,
    bool include_shaft) noexcept {
    const double radians = height_angle_degrees * kDegreesToRadians;
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const double radius = kReferenceHeightOutlineWidth * unit_scale * 0.5;
    double right_extent{};
    const auto segments = height_outline_segments(unit_scale);
    const std::size_t first_segment = include_shaft ? 0U : 1U;
    for (std::size_t index = first_segment; index < segments.size(); ++index) {
        const LineSegment& segment = segments[index];
        const double dx = segment.end.x - segment.start.x;
        const double dy = segment.end.y - segment.start.y;
        const double segment_length = std::hypot(dx, dy);
        if (!std::isfinite(segment_length) || segment_length <= 0.0) {
            continue;
        }
        const double normal_x = -dy / segment_length * radius;
        const double normal_y = dx / segment_length * radius;
        for (const Vector2D endpoint : {segment.start, segment.end}) {
            for (const double side : {-1.0, 1.0}) {
                const double corner_x = endpoint.x + normal_x * side;
                const double corner_y = endpoint.y + normal_y * side;
                right_extent = std::max(
                    right_extent, corner_x * cosine - corner_y * sine);
            }
        }
    }
    return std::max(0.0, right_extent);
}
constexpr std::size_t kGeometryParameterCapacity = 256;

[[nodiscard]] FProperty* find_struct_field(
    UScriptStruct* structure, const wchar_t* name) {
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

[[nodiscard]] FProperty* find_function_field(
    UFunction* function, const wchar_t* name) {
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

[[nodiscard]] bool function_property_fits(
    UFunction* function, FProperty* property) {
    if (!function || !property || property->GetOffset_Internal() < 0
        || property->GetSize() <= 0) {
        return false;
    }
    const auto parameter_bytes = static_cast<std::size_t>(
        function->GetParmsSize());
    return parameter_bytes > 0
        && parameter_bytes <= kGeometryParameterCapacity
        && static_cast<std::size_t>(
               property->GetOffset_Internal() + property->GetSize())
            <= parameter_bytes;
}

[[nodiscard]] bool vector_struct_is_finite_schema(
    FStructProperty* property) {
    if (!property || property->GetSize() <= 0) {
        return false;
    }
    UScriptStruct* structure = property->GetStruct();
    auto* x = CastField<FNumericProperty>(
        structure ? find_struct_field(structure, L"X") : nullptr);
    auto* y = CastField<FNumericProperty>(
        structure ? find_struct_field(structure, L"Y") : nullptr);
    const auto field_fits = [property](FNumericProperty* field) {
        return field && field->IsFloatingPoint()
            && field->GetOffset_Internal() >= 0
            && field->GetSize() > 0
            && field->GetOffset_Internal() + field->GetSize()
                <= property->GetSize();
    };
    return field_fits(x) && field_fits(y);
}

[[nodiscard]] bool geometry_function_parameters_fit(
    UFunction* function, std::size_t expected_count) {
    if (!function) return false;
    std::size_t count{}, returns{};
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function)) {
        if (!property->HasAnyPropertyFlags(CPF_Parm)) continue;
        if (!function_property_fits(function, property)) return false;
        ++count;
        if (property->HasAnyPropertyFlags(CPF_ReturnParm)) ++returns;
    }
    return count == expected_count && returns == 1;
}

struct GeometryReflectionSchema {
    FStructProperty* cached_geometry_return{};
    FStructProperty* local_size_geometry{};
    FStructProperty* local_size_return{};
    FStructProperty* local_to_absolute_geometry{};
    FStructProperty* local_coordinate{};
    FStructProperty* local_to_absolute_return{};
    FStructProperty* absolute_to_local_geometry{};
    FStructProperty* absolute_coordinate{};
    FStructProperty* absolute_to_local_return{};
};

[[nodiscard]] bool resolve_geometry_reflection_schema(
    UFunction* get_cached_geometry,
    UFunction* get_geometry_local_size,
    UFunction* local_to_absolute,
    UFunction* absolute_to_local,
    GeometryReflectionSchema& schema) {
    schema = {};
    schema.cached_geometry_return = CastField<FStructProperty>(
        find_function_field(get_cached_geometry, L"ReturnValue"));
    schema.local_size_geometry = CastField<FStructProperty>(
        find_function_field(get_geometry_local_size, L"Geometry"));
    schema.local_size_return = CastField<FStructProperty>(
        find_function_field(get_geometry_local_size, L"ReturnValue"));
    schema.local_to_absolute_geometry = CastField<FStructProperty>(
        find_function_field(local_to_absolute, L"Geometry"));
    schema.local_coordinate = CastField<FStructProperty>(
        find_function_field(local_to_absolute, L"LocalCoordinate"));
    schema.local_to_absolute_return = CastField<FStructProperty>(
        find_function_field(local_to_absolute, L"ReturnValue"));
    schema.absolute_to_local_geometry = CastField<FStructProperty>(
        find_function_field(absolute_to_local, L"Geometry"));
    schema.absolute_coordinate = CastField<FStructProperty>(
        find_function_field(absolute_to_local, L"AbsoluteCoordinate"));
    schema.absolute_to_local_return = CastField<FStructProperty>(
        find_function_field(absolute_to_local, L"ReturnValue"));

    const bool properties_fit =
        geometry_function_parameters_fit(get_cached_geometry, 1)
        && geometry_function_parameters_fit(get_geometry_local_size, 2)
        && geometry_function_parameters_fit(local_to_absolute, 3)
        && geometry_function_parameters_fit(absolute_to_local, 3)
        && function_property_fits(
            get_cached_geometry, schema.cached_geometry_return)
        && function_property_fits(
            get_geometry_local_size, schema.local_size_geometry)
        && function_property_fits(
            get_geometry_local_size, schema.local_size_return)
        && function_property_fits(
            local_to_absolute, schema.local_to_absolute_geometry)
        && function_property_fits(
            local_to_absolute, schema.local_coordinate)
        && function_property_fits(
            local_to_absolute, schema.local_to_absolute_return)
        && function_property_fits(
            absolute_to_local, schema.absolute_to_local_geometry)
        && function_property_fits(
            absolute_to_local, schema.absolute_coordinate)
        && function_property_fits(
            absolute_to_local, schema.absolute_to_local_return);
    if (!properties_fit) {
        return false;
    }

    UScriptStruct* geometry = schema.cached_geometry_return->GetStruct();
    UScriptStruct* vector = schema.local_size_return->GetStruct();
    return geometry
        && geometry == schema.local_size_geometry->GetStruct()
        && geometry == schema.local_to_absolute_geometry->GetStruct()
        && geometry == schema.absolute_to_local_geometry->GetStruct()
        && vector
        && vector == schema.local_size_return->GetStruct()
        && vector == schema.local_coordinate->GetStruct()
        && vector == schema.local_to_absolute_return->GetStruct()
        && vector == schema.absolute_coordinate->GetStruct()
        && vector == schema.absolute_to_local_return->GetStruct()
        && vector_struct_is_finite_schema(schema.local_size_return);
}

[[nodiscard]] bool read_numeric_value(
    FProperty* property, void* container, double& value) {
    auto* numeric = CastField<FNumericProperty>(property);
    void* address = numeric && container
        ? numeric->ContainerPtrToValuePtr<void>(container)
        : nullptr;
    if (!numeric || !address) {
        return false;
    }
    if (numeric->IsFloatingPoint()) {
        value = numeric->GetFloatingPointPropertyValue(address);
        return std::isfinite(value);
    }
    if (numeric->IsInteger()) {
        value = static_cast<double>(numeric->GetSignedIntPropertyValue(address));
        return std::isfinite(value);
    }
    return false;
}

struct alignas(std::max_align_t) GeometryParameterBuffer {
    std::array<std::byte, kGeometryParameterCapacity> bytes{};
};

// ProcessEvent callers own the reflected parameter storage. Initialize and
// destroy every parameter property so FGeometry remains safe even if a future
// engine build changes it from plain data to a non-trivial script struct.
class GeometryCallParameters final {
public:
    explicit GeometryCallParameters(UFunction* function) noexcept
        : function_(function) {
        if (!function_ || function_->GetParmsSize() < 0
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

    ~GeometryCallParameters() {
        if (!function_) {
            return;
        }
        for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(function_)) {
            if (property->HasAnyPropertyFlags(CPF_Parm)) {
                property->DestroyValue_InContainer(buffer_.bytes.data());
            }
        }
    }

    GeometryCallParameters(const GeometryCallParameters&) = delete;
    GeometryCallParameters& operator=(const GeometryCallParameters&) = delete;

    [[nodiscard]] bool valid() const noexcept { return function_ != nullptr; }
    [[nodiscard]] void* data() noexcept { return buffer_.bytes.data(); }

private:
    UFunction* function_{};
    GeometryParameterBuffer buffer_{};
};

[[nodiscard]] bool read_vector_property(
    FStructProperty* property,
    void* container,
    double& x,
    double& y) {
    void* vector_value = property && container
        ? property->ContainerPtrToValuePtr<void>(container)
        : nullptr;
    UScriptStruct* structure = property ? property->GetStruct() : nullptr;
    return vector_value && structure
        && read_numeric_value(
            find_struct_field(structure, L"X"), vector_value, x)
        && read_numeric_value(
            find_struct_field(structure, L"Y"), vector_value, y);
}

[[nodiscard]] bool write_vector_property(
    FStructProperty* property,
    void* container,
    double x,
    double y) {
    if (!property || !container || !std::isfinite(x) || !std::isfinite(y)) {
        return false;
    }
    void* vector_value = property->ContainerPtrToValuePtr<void>(container);
    UScriptStruct* structure = property->GetStruct();
    auto* x_property = CastField<FNumericProperty>(
        structure ? find_struct_field(structure, L"X") : nullptr);
    auto* y_property = CastField<FNumericProperty>(
        structure ? find_struct_field(structure, L"Y") : nullptr);
    void* x_value = x_property && vector_value
        ? x_property->ContainerPtrToValuePtr<void>(vector_value)
        : nullptr;
    void* y_value = y_property && vector_value
        ? y_property->ContainerPtrToValuePtr<void>(vector_value)
        : nullptr;
    if (!x_property || !y_property || !x_value || !y_value
        || !x_property->IsFloatingPoint()
        || !y_property->IsFloatingPoint()) {
        return false;
    }
    x_property->SetFloatingPointPropertyValue(x_value, x);
    y_property->SetFloatingPointPropertyValue(y_value, y);
    return true;
}

[[nodiscard]] bool copy_geometry_property(
    FStructProperty* destination_property,
    void* destination_container,
    FStructProperty* source_property,
    void* source_container) {
    if (!destination_property || !destination_container
        || !source_property || !source_container
        || destination_property->GetStruct() != source_property->GetStruct()) {
        return false;
    }
    void* destination = destination_property
        ->ContainerPtrToValuePtr<void>(destination_container);
    const void* source = source_property
        ->ContainerPtrToValuePtr<void>(source_container);
    if (!destination || !source) {
        return false;
    }
    destination_property->CopyCompleteValue(destination, source);
    return true;
}

[[nodiscard]] bool read_geometry_local_size(
    UObject* slate_library,
    UFunction* get_geometry_local_size,
    const GeometryReflectionSchema& schema,
    FStructProperty* source_geometry_property,
    void* source_geometry_container,
    double& width,
    double& height) {
    GeometryCallParameters parameters(get_geometry_local_size);
    if (!parameters.valid()) {
        return false;
    }
    if (!copy_geometry_property(
            schema.local_size_geometry, parameters.data(),
            source_geometry_property, source_geometry_container)) {
        return false;
    }
    slate_library->ProcessEvent(
        get_geometry_local_size, parameters.data());
    return read_vector_property(
            schema.local_size_return, parameters.data(), width, height)
        && width > 0.0 && height > 0.0;
}

[[nodiscard]] bool transform_geometry_point(
    UObject* slate_library,
    UFunction* function,
    FStructProperty* geometry_property,
    FStructProperty* coordinate_property,
    FStructProperty* return_property,
    FStructProperty* source_geometry_property,
    void* source_geometry_container,
    double input_x,
    double input_y,
    double& output_x,
    double& output_y) {
    GeometryCallParameters parameters(function);
    if (!parameters.valid()) {
        return false;
    }
    if (!copy_geometry_property(
            geometry_property, parameters.data(),
            source_geometry_property, source_geometry_container)
        || !write_vector_property(
            coordinate_property, parameters.data(), input_x, input_y)) {
        return false;
    }
    slate_library->ProcessEvent(function, parameters.data());
    return read_vector_property(
        return_property, parameters.data(), output_x, output_y);
}

// Exact named ownership and a bounded paint chain, matching the main HUD
// witness. Never enumerate WidgetTree or global objects for clock placement.
[[nodiscard]] UObject* read_clock_owned_object(UObject* object, const wchar_t* name) {
    auto* property = object ? CastField<FObjectPropertyBase>(
        object->GetPropertyByNameInChain(name)) : nullptr;
    return property && property->GetSize() == sizeof(void*)
        ? property->GetObjectPropertyValue(property->ContainerPtrToValuePtr<void>(object))
        : nullptr;
}

[[nodiscard]] bool clock_widget_paints_to_owner(
    UObject* widget, UObject* main_panel, UClass* widget_class) {
    if (!widget || !main_panel || !widget_class || !main_panel->GetWorld()) return false;
    const auto* world = main_panel->GetWorld();
    std::array<UObject*, 24> visited{};
    UObject* node = widget;
    for (std::size_t depth = 0; node && depth < visited.size(); ++depth) {
        if (!node->IsA(widget_class) || node->GetWorld() != world
            || std::find(visited.begin(), visited.begin() + depth, node)
                != visited.begin() + depth) return false;
        visited[depth] = node;
        FProperty* visibility = node->GetPropertyByNameInChain(L"Visibility");
        auto* numeric = CastField<FNumericProperty>(visibility);
        if (!numeric) {
            auto* enumeration = CastField<FEnumProperty>(visibility);
            numeric = enumeration ? enumeration->GetUnderlyingProperty() : nullptr;
        }
        if (!visibility || visibility->GetSize() != 1
            || !numeric || !numeric->IsInteger() || numeric->GetSize() != 1) return false;
        const auto value = numeric->GetUnsignedIntPropertyValue(
            visibility->ContainerPtrToValuePtr<void>(node));
        auto* opacity = CastField<FNumericProperty>(
            node->GetPropertyByNameInChain(L"RenderOpacity"));
        if (value > 4 || value == 1 || value == 2 || !opacity
            || !opacity->IsFloatingPoint() || opacity->GetSize() != sizeof(float)) return false;
        const double alpha = opacity->GetFloatingPointPropertyValue(
            opacity->ContainerPtrToValuePtr<void>(node));
        if (!std::isfinite(alpha) || alpha <= 0.0 || alpha > 1.0) return false;
        if (node == main_panel) return true;
        UObject* slot = read_clock_owned_object(node, L"Slot");
        UObject* parent = read_clock_owned_object(slot, L"Parent");
        if (parent && read_clock_owned_object(slot, L"Content") == node) {
            node = parent;
            continue;
        }
        UObject* tree = node->GetOuterPrivate();
        UObject* tree_owner = tree ? tree->GetOuterPrivate() : nullptr;
        if (tree && tree_owner && read_clock_owned_object(tree, L"RootWidget") == node
            && read_clock_owned_object(tree_owner, L"WidgetTree") == tree) {
            node = tree_owner;
            continue;
        }
        return false;
    }
    return false;
}

// Three corners prove the native widgets remain axis-aligned. Convert through
// both complete geometries so DPI, safe zone and host render scale occur once.
// Rotated/skewed/reversed or zero-sized inputs keep the established fallback.
[[nodiscard]] bool read_clock_widget_rect(
    UObject* widget, UObject* slate_library,
    UFunction* get_cached_geometry, UFunction* get_local_size,
    UFunction* local_to_absolute, UFunction* absolute_to_local,
    const GeometryReflectionSchema& schema, void* root_geometry,
    dswros::CompactClockRect& rect) {
    GeometryCallParameters geometry(get_cached_geometry);
    if (!geometry.valid()) return false;
    widget->ProcessEvent(get_cached_geometry, geometry.data());
    double width{}, height{};
    if (!read_geometry_local_size(slate_library, get_local_size, schema,
            schema.cached_geometry_return, geometry.data(), width, height)) return false;
    std::array<Vector2D, 3> corners{{{0, 0}, {width, 0}, {0, height}}};
    for (auto& point : corners) {
        double absolute_x{}, absolute_y{};
        if (!transform_geometry_point(slate_library, local_to_absolute,
                schema.local_to_absolute_geometry, schema.local_coordinate,
                schema.local_to_absolute_return, schema.cached_geometry_return,
                geometry.data(), point.x, point.y, absolute_x, absolute_y)
            || !transform_geometry_point(slate_library, absolute_to_local,
                schema.absolute_to_local_geometry, schema.absolute_coordinate,
                schema.absolute_to_local_return, schema.cached_geometry_return,
                root_geometry, absolute_x, absolute_y, point.x, point.y)) return false;
    }
    constexpr double kAxisTolerance = 1.0e-3;
    if (std::abs(corners[0].y - corners[1].y) > kAxisTolerance
        || std::abs(corners[0].x - corners[2].x) > kAxisTolerance) return false;
    rect = {corners[0].x, corners[0].y, corners[1].x, corners[2].y};
    return dswros::valid_compact_clock_rect(rect);
}

} // namespace

void CompactUmgRenderer::initialize() noexcept {
    // Optional geometry ABI: a missing/schema-mismatched function keeps the
    // established clock position and cannot disable the compact renderer.
    clock_geometry_widget_class_ = find<UClass>(L"/Script/UMG.Widget");
    clock_slate_library_ = find<UObject>(L"/Script/UMG.Default__SlateBlueprintLibrary");
    clock_get_cached_geometry_ = find<UFunction>(L"/Script/UMG.Widget:GetCachedGeometry");
    clock_get_local_size_ = find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:GetLocalSize");
    clock_local_to_absolute_ = find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:LocalToAbsolute");
    clock_absolute_to_local_ = find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:AbsoluteToLocal");
    GeometryReflectionSchema clock_schema{};
    clock_geometry_schema_ready_ = clock_geometry_widget_class_
        && clock_slate_library_.Get()
        && resolve_geometry_reflection_schema(
            clock_get_cached_geometry_, clock_get_local_size_,
            clock_local_to_absolute_, clock_absolute_to_local_, clock_schema);
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    canvas_panel_slot_class_ =
        find<UClass>(L"/Script/UMG.CanvasPanelSlot");
    border_class_ = find<UClass>(L"/Script/UMG.Border");
    widget_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    widget_layout_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");

    create_widget_ = find<UFunction>(L"/Script/UMG.WidgetBlueprintLibrary:Create");
    get_owning_player_ = find<UFunction>(L"/Script/UMG.Widget:GetOwningPlayer");
    get_viewport_size_ =
        find<UFunction>(L"/Script/UMG.WidgetLayoutLibrary:GetViewportSize");
    get_viewport_scale_ =
        find<UFunction>(L"/Script/UMG.WidgetLayoutLibrary:GetViewportScale");
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
    set_brush_ = find<UFunction>(L"/Script/UMG.Border:SetBrush");
    set_brush_color_ = find<UFunction>(L"/Script/UMG.Border:SetBrushColor");
    set_render_translation_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderTranslation");
    set_render_angle_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderTransformAngle");
    set_render_pivot_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderTransformPivot");
    set_render_scale_ =
        find<UFunction>(L"/Script/UMG.Widget:SetRenderScale");
    set_position_in_viewport_ =
        find<UFunction>(L"/Script/UMG.UserWidget:SetPositionInViewport");
    set_alignment_in_viewport_ =
        find<UFunction>(L"/Script/UMG.UserWidget:SetAlignmentInViewport");
    set_desired_size_in_viewport_ =
        find<UFunction>(L"/Script/UMG.UserWidget:SetDesiredSizeInViewport");

    force_layout_prepass_ =
        find<UFunction>(L"/Script/UMG.Widget:ForceLayoutPrepass");
    remove_from_parent_ = find<UFunction>(L"/Script/UMG.Widget:RemoveFromParent");
    clear_children_ = find<UFunction>(L"/Script/UMG.PanelWidget:ClearChildren");

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
    require_parameters(1U << 3U, add_to_viewport_, 4);
    require_parameters(1U << 4U, add_child_to_canvas_, 16);
    require_parameters(1U << 6U, set_slot_position_, 16);
    require_parameters(1U << 7U, set_slot_size_, 16);
    require_parameters(1U << 8U, set_slot_alignment_, 16);
    require_parameters(1U << 9U, set_slot_z_order_, 4);
    require_parameters(1U << 10U, set_visibility_, 1);
    // The reflected span excludes local C++ tail padding after the bool.
    require_parameters(1U << 11U, set_position_in_viewport_, 17);
    require_parameters(1U << 12U, set_alignment_in_viewport_, 16);
    require_parameters(1U << 13U, set_desired_size_in_viewport_, 16);
    require_parameters(1U << 14U, force_layout_prepass_, 0);
    require_parameters(1U << 15U, remove_from_parent_, 0);
    require_parameters(1U << 16U, set_brush_color_, 16);
    require_parameters(1U << 17U, set_render_translation_, 16);
    require_parameters(1U << 18U, clear_children_, 0);
    // The reflected span excludes local C++ tail padding after the float.
    require_parameters(1U << 19U, get_viewport_scale_, 12);
    require_parameters(1U << 20U, set_render_angle_, 4);
    require_parameters(1U << 22U, set_render_pivot_, 16);
    require_parameters(1U << 23U, set_brush_, 208);
    require_parameters(1U << 24U, set_render_scale_, 16);

    if (!canvas_panel_class_ || !canvas_panel_slot_class_ || !border_class_
        || !widget_blueprint_library_.Get()
        || !widget_layout_library_.Get()) {
        abi_failure_mask_ |= 1U << 21U;
    }

    state_ = abi_failure_mask_ == 0
        ? CompactUmgRendererState::Ready
        : CompactUmgRendererState::Disabled;
}

void CompactUmgRenderer::begin_activation() noexcept {
    activation_active_ = false;
    const std::uint64_t faults_before_detach = fault_count_;
    detach_guarded();
    // A guarded runtime operation may have failed after a previous world
    // released its widget tree. detach_guarded() has now cleared every weak
    // runtime handle, so a later explicit activation or activity-edge rearm
    // may safely retry when the immutable ABI metadata is still valid. Do not
    // recover a new detach fault from this same call, and never recover an ABI
    // validation failure.
    if (state_ == CompactUmgRendererState::Faulted
        && fault_count_ == faults_before_detach
        && abi_failure_mask_ == 0) {
        state_ = CompactUmgRendererState::Ready;
    }
    if (state_ != CompactUmgRendererState::Ready) {
        return;
    }
    activation_active_ = true;
    attach_attempted_ = false;
    menu_suppressed_ = false;
    last_attach_failure_ = 0;
    state_ = CompactUmgRendererState::Ready;
}

bool CompactUmgRenderer::attach_once(
    UObject* expected_owning_player,
    FWeakObjectPtr minimap_layer_candidate) noexcept {
    if (!activation_active_ || attach_attempted_
        || state_ != CompactUmgRendererState::Ready) {
        return state_ == CompactUmgRendererState::Attached
            || state_ == CompactUmgRendererState::Suppressed;
    }
    attach_attempted_ = true;
    ++attach_attempt_count_;
    if (!expected_owning_player) {
        last_attach_failure_ = 15;
        return false;
    }
    return attach_guarded(expected_owning_player, minimap_layer_candidate);
}

bool CompactUmgRenderer::attach_guarded(
    UObject* expected_owning_player,
    FWeakObjectPtr minimap_layer_candidate) noexcept {
#if defined(_MSC_VER)
    __try {
        return attach_unsafe(
            expected_owning_player, minimap_layer_candidate);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        last_attach_failure_ = 100;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
        return false;
    }
#else
    try {
        return attach_unsafe(
            expected_owning_player, minimap_layer_candidate);
    } catch (...) {
        ++fault_count_;
        last_attach_failure_ = 100;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
        return false;
    }
#endif
}

bool CompactUmgRenderer::attach_unsafe(
    UObject* expected_owning_player,
    FWeakObjectPtr minimap_layer_candidate) {
    UObject* layer = minimap_layer_candidate.Get();
    if (!layer) {
        layer = UObjectGlobals::FindFirstOf(L"DLayerMiniMap");
    }
    if (!layer) {
        last_attach_failure_ = 1;
        return false;
    }
    UObject* layer_map = read_object_property(layer, L"LayerMap");
    if (!layer_map) {
        last_attach_failure_ = 2;
        return false;
    }
    UObject* map_overlay = read_object_property(layer_map, L"MapOverlay");
    if (!map_overlay || !resolve_minimap_scale_schema(map_overlay)) {
        last_attach_failure_ = 17;
        return false;
    }
    UObject* player_icon = read_object_property(layer_map, L"PlayerIconWidget");
    if (!player_icon) {
        last_attach_failure_ = 3;
        return false;
    }

    ObjectReturnParameters owning_player{};
    player_icon->ProcessEvent(get_owning_player_, &owning_player);
    if (!owning_player.return_value) {
        last_attach_failure_ = 5;
        return false;
    }
    if (owning_player.return_value != expected_owning_player) {
        last_attach_failure_ = 16;
        return false;
    }

    ViewportSizeParameters viewport_size{owning_player.return_value};
    UObject* layout_library = widget_layout_library_.Get();
    if (!layout_library) {
        last_attach_failure_ = 6;
        return false;
    }
    layout_library->ProcessEvent(get_viewport_size_, &viewport_size);
    if (!std::isfinite(viewport_size.return_value.x)
        || !std::isfinite(viewport_size.return_value.y)
        || viewport_size.return_value.x < 640.0
        || viewport_size.return_value.y < 360.0) {
        last_attach_failure_ = 7;
        return false;
    }
    ViewportScaleParameters viewport_scale{owning_player.return_value};
    layout_library->ProcessEvent(get_viewport_scale_, &viewport_scale);
    if (!std::isfinite(viewport_scale.return_value)
        || viewport_scale.return_value < 0.1F
        || viewport_scale.return_value > 10.0F) {
        last_attach_failure_ = 18;
        return false;
    }
    viewport_width_ = viewport_size.return_value.x;
    viewport_height_ = viewport_size.return_value.y;
    viewport_dpi_scale_ = viewport_scale.return_value;
    display_scale_ = std::clamp(
        std::min(viewport_width_ / 2560.0, viewport_height_ / 1440.0),
        0.25,
        4.0);
    // Canvas slots and desired widget sizes use DPI-scaled UMG logical units.
    // SetPositionInViewport receives raw pixels because RemoveDPIScale is true.
    // Dividing every local length by the one captured viewport DPI keeps the
    // host center, marker extent, icon size, and root translation in one space.
    umg_unit_scale_ = display_scale_ / viewport_dpi_scale_;
    const auto viewport_layout = dswros::calculate_compact_viewport_layout(
        viewport_width_, viewport_height_, viewport_dpi_scale_,
        umg_unit_scale_);
    if (!viewport_layout) {
        last_attach_failure_ = 18;
        return false;
    }
    host_render_scale_ = viewport_layout->host_render_scale;

    UObject* blueprint_library = widget_blueprint_library_.Get();
    if (!blueprint_library) {
        last_attach_failure_ = 8;
        return false;
    }
    UClass* player_icon_class = player_icon->GetClassPrivate();
    if (!player_icon_class) {
        last_attach_failure_ = 9;
        return false;
    }
    CreateWidgetParameters create{
        owning_player.return_value,
        player_icon_class,
        owning_player.return_value,
        nullptr};
    blueprint_library->ProcessEvent(create_widget_, &create);
    UObject* host = create.return_value;
    if (!host) {
        last_attach_failure_ = 9;
        return false;
    }


    UObject* tree = read_object_property(host, L"WidgetTree");
    if (!tree) {
        last_attach_failure_ = 10;
        return false;
    }

    // The Blueprint-created root already owns a realized Slate subtree. Use
    // that exact Canvas as the compact surface so child coordinates have one
    // documented local space instead of depending on an arbitrary nested
    // ancestor's layout geometry.
    UObject* marker_canvas = read_object_property(tree, L"RootWidget");
    if (!marker_canvas || !marker_canvas->IsA(canvas_panel_class_)) {
        last_attach_failure_ = 11;
        return false;
    }

    // This Blueprint is used only as a runtime-proven realized viewport host.
    // Remove its complete authored visual tree before adding radar markers so
    // no stock player arrow or decorative child can survive or be restored.
    marker_canvas->ProcessEvent(clear_children_, nullptr);

    const double host_size = kReferenceHostHalfSize * 2.0 * umg_unit_scale_;
    const double local_center = kReferenceHostHalfSize * umg_unit_scale_;
    UObject* movement_group =
        UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    if (!movement_group) {
        last_attach_failure_ = 23;
        return false;
    }
    AddChildToCanvasParameters add_movement_group{movement_group};
    marker_canvas->ProcessEvent(add_child_to_canvas_, &add_movement_group);
    UObject* movement_group_slot = add_movement_group.return_value;
    if (!movement_group_slot) {
        last_attach_failure_ = 24;
        return false;
    }
    set_slot_vector(movement_group_slot, set_slot_position_, 0.0, 0.0);
    set_slot_vector(movement_group_slot, set_slot_size_, host_size, host_size);
    set_slot_vector(movement_group_slot, set_slot_alignment_, 0.0, 0.0);

    std::array<std::array<UObject*, kCompactUmgMarkerPieceCount>,
               kCompactUmgMarkerCapacity> pieces{};
    std::array<std::array<UObject*, kCompactUmgMarkerPieceCount>,
               kCompactUmgMarkerCapacity> slots{};
    brush_templates_ready_ = false;
    for (std::size_t index = 0; index < kCompactUmgMarkerCapacity; ++index) {
        for (std::size_t piece = 0; piece < kCompactUmgMarkerPieceCount;
             ++piece) {
            UObject* border =
                UObjectGlobals::NewObject<UObject>(tree, border_class_);
            if (!border) {
                last_attach_failure_ = 13;
                return false;
            }
            if (!brush_templates_ready_) {
                void* background =
                    border->GetValuePtrByPropertyNameInChain(L"Background");
                if (!background) {
                    last_attach_failure_ = 29;
                    return false;
                }
                std::memcpy(
                    solid_brush_template_.data(), background,
                    solid_brush_template_.size());
                fly_outline_brush_template_ = solid_brush_template_;
                configure_fly_outline_brush(
                    fly_outline_brush_template_);
                area_quest_brush_template_ = solid_brush_template_;
                configure_area_quest_outline_brush(
                    area_quest_brush_template_, umg_unit_scale_);
                encounter_triangle_brush_template_ = solid_brush_template_;
                configure_encounter_triangle_brush(
                    encounter_triangle_brush_template_, umg_unit_scale_);
                bird_egg_brush_template_ = solid_brush_template_;
                configure_bird_egg_oval_brush(
                    bird_egg_brush_template_);
                clock_phase_brush_template_ = solid_brush_template_;
                configure_clock_phase_brush(
                    clock_phase_brush_template_);
                brush_templates_ready_ = true;
            }
            set_brush_color(border, set_brush_color_, kOutline);
            AddChildToCanvasParameters add_piece{border};
            movement_group->ProcessEvent(add_child_to_canvas_, &add_piece);
            UObject* slot = add_piece.return_value;
            if (!slot) {
                last_attach_failure_ = 14;
                return false;
            }
            set_slot_vector(slot, set_slot_position_, 0.0, 0.0);
            set_slot_vector(slot, set_slot_size_, 10.0, 10.0);
            set_slot_vector(slot, set_slot_alignment_, 0.5, 0.5);
            ZOrderParameters z_order{static_cast<std::int32_t>(
                index * kCompactUmgMarkerPieceCount + piece + 1U)};
            slot->ProcessEvent(set_slot_z_order_, &z_order);
            set_visibility(border, set_visibility_, kCollapsed);
            pieces[index][piece] = border;
            slots[index][piece] = slot;
        }
    }

    const double height_group_size =
        kReferenceHeightGroupHalfSize * 2.0 * umg_unit_scale_;
    const double height_center =
        kReferenceHeightGroupHalfSize * umg_unit_scale_;
    const double height_length =
        kReferenceHeightLength * umg_unit_scale_;
    const double height_head_length =
        kReferenceHeightHeadLength * umg_unit_scale_;
    const double height_head_half_width =
        kReferenceHeightHeadHalfWidth * umg_unit_scale_;
    const double height_end_cap_inset =
        kReferenceHeightEndCapInset * umg_unit_scale_;
    const auto outline_segments = height_outline_segments(umg_unit_scale_);
    const double inner_tip_x = height_center - height_end_cap_inset;
    const double inner_head_base_distance = std::hypot(
        height_head_length - height_end_cap_inset,
        height_head_half_width);
    const double inner_base_x = height_center - height_head_length
        + (height_head_length - height_end_cap_inset)
            / inner_head_base_distance * height_end_cap_inset;
    const double inner_base_y = height_center + height_head_half_width
        - height_head_half_width / inner_head_base_distance
            * height_end_cap_inset;
    const LineSegment inner_upper = sharp_head_segment(
        inner_tip_x, height_center, inner_base_x, inner_base_y,
        kReferenceHeightInnerWidth * umg_unit_scale_, true);
    const LineSegment inner_lower = sharp_head_segment(
        inner_tip_x, height_center, inner_base_x,
        2.0 * height_center - inner_base_y,
        kReferenceHeightInnerWidth * umg_unit_scale_, false);
    const std::array<std::array<double, 4>, kCompactUmgHeightPieceCount>
        height_segments{{
        {height_center + outline_segments[0].start.x,
         height_center + outline_segments[0].start.y,
         height_center + outline_segments[0].end.x,
         height_center + outline_segments[0].end.y},
        {height_center - height_length
             + kReferenceHeightTailInset * umg_unit_scale_,
         height_center, inner_tip_x, height_center},
        {height_center + outline_segments[1].start.x,
         height_center + outline_segments[1].start.y,
         height_center + outline_segments[1].end.x,
         height_center + outline_segments[1].end.y},
        {height_center + outline_segments[2].start.x,
         height_center + outline_segments[2].start.y,
         height_center + outline_segments[2].end.x,
         height_center + outline_segments[2].end.y},
        {inner_upper.start.x, inner_upper.start.y,
         inner_upper.end.x, inner_upper.end.y},
        {inner_lower.start.x, inner_lower.start.y,
         inner_lower.end.x, inner_lower.end.y},
    }};
    constexpr std::array<bool, kCompactUmgHeightPieceCount>
        height_outline_order{{true, false, true, true, false, false}};
    std::array<UObject*, kCompactUmgHeightChannelCount> height_groups{};
    std::array<UObject*, kCompactUmgHeightChannelCount> height_group_slots{};
    std::array<std::array<UObject*, kCompactUmgHeightPieceCount>,
               kCompactUmgHeightChannelCount> height_pieces{};
    std::array<std::array<UObject*, kCompactUmgHeightPieceCount>,
               kCompactUmgHeightChannelCount> height_slots{};
    for (std::size_t channel = 0;
         channel < kCompactUmgHeightChannelCount; ++channel) {
        const bool mole_channel = channel
            == static_cast<std::size_t>(CompactUmgHeightChannel::Mole);
        const LinearColor initial_outline = mole_channel
            ? kMoleHeightOutline : kOutline;
        const LinearColor initial_fill = mole_channel
            ? kHammer : kTreasureOther;
        UObject* height_group =
            UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
        if (!height_group) {
            last_attach_failure_ = 19;
            return false;
        }
        AddChildToCanvasParameters add_height_group{height_group};
        movement_group->ProcessEvent(
            add_child_to_canvas_, &add_height_group);
        UObject* height_group_slot = add_height_group.return_value;
        if (!height_group_slot) {
            last_attach_failure_ = 20;
            return false;
        }
        set_slot_vector(
            height_group_slot, set_slot_position_, 0.0, 0.0);
        set_slot_vector(
            height_group_slot, set_slot_size_,
            height_group_size, height_group_size);
        set_slot_vector(
            height_group_slot, set_slot_alignment_, 0.5, 0.5);
        ZOrderParameters height_group_z_order{static_cast<std::int32_t>(
            kCompactUmgMarkerCapacity * kCompactUmgMarkerPieceCount
                + channel + 1U)};
        height_group_slot->ProcessEvent(
            set_slot_z_order_, &height_group_z_order);
        VectorParameters height_group_pivot{{0.5, 0.5}};
        height_group->ProcessEvent(
            set_render_pivot_, &height_group_pivot);
        set_visibility(height_group, set_visibility_, kCollapsed);

        for (std::size_t piece = 0; piece < kCompactUmgHeightPieceCount;
             ++piece) {
            UObject* border =
                UObjectGlobals::NewObject<UObject>(tree, border_class_);
            if (!border) {
                last_attach_failure_ = 21;
                return false;
            }
            set_brush_color(
                border, set_brush_color_,
                height_outline_order[piece]
                    ? initial_outline : initial_fill);
            AddChildToCanvasParameters add_piece{border};
            height_group->ProcessEvent(add_child_to_canvas_, &add_piece);
            UObject* slot = add_piece.return_value;
            if (!slot) {
                last_attach_failure_ = 22;
                return false;
            }
            const auto& segment = height_segments[piece];
            set_line_geometry(
                slot, set_slot_position_, set_slot_size_,
                segment[0], segment[1], segment[2], segment[3],
                (height_outline_order[piece]
                    ? kReferenceHeightOutlineWidth
                    : kReferenceHeightInnerWidth) * umg_unit_scale_);
            set_slot_vector(slot, set_slot_alignment_, 0.5, 0.5);
            ZOrderParameters z_order{static_cast<std::int32_t>(piece + 1U)};
            slot->ProcessEvent(set_slot_z_order_, &z_order);
            set_render_angle(
                border, set_render_angle_,
                std::atan2(
                    segment[3] - segment[1],
                    segment[2] - segment[0]) * kRadiansToDegrees);
            // Treasure keeps this complete six-piece shaft and sharp head. The
            // same six fixed Mole-channel pieces are reshaped into a discrete
            // below-marker mini-game triangle before that group is revealed.
            // Area Quest owns no separate pointer: its task marker body is
            // reshaped in place, so this channel stays collapsed.
            const bool area_quest_channel = channel
                == static_cast<std::size_t>(
                    CompactUmgHeightChannel::AreaQuest);
            set_visibility(
                border, set_visibility_,
                area_quest_channel ? kCollapsed : kVisible);
            height_pieces[channel][piece] = border;
            height_slots[channel][piece] = slot;
        }
        height_groups[channel] = height_group;
        height_group_slots[channel] = height_group_slot;
    }

    constexpr double kClockReferenceWidth = dswros::kCompactClockReferenceWidth;
    constexpr double kClockReferenceHeight = dswros::kCompactClockReferenceHeight;
    // First-frame/unsupported-geometry fallback. The existing one-hertz layout
    // service replaces this only after both live HUD rectangles prove a gap.
    constexpr double kClockReferenceTop = kReferenceMarkerExtent + 8.0;
    static_assert(kClockReferenceTop + kClockReferenceHeight
                  <= kReferenceHostHalfSize);
    constexpr std::array<double, kCompactClockDigitCount>
        kClockDigitLeft{{7.0, 21.0, 45.0, 59.0}};
    constexpr std::array<std::array<double, 4>, kCompactClockSegmentCount>
        kClockSegmentGeometry{{
            {5.5, 4.0, 10.0, 2.8},
            {10.0, 9.5, 2.8, 8.2},
            {10.0, 20.5, 2.8, 8.2},
            {5.5, 26.0, 10.0, 2.8},
            {1.0, 20.5, 2.8, 8.2},
            {1.0, 9.5, 2.8, 8.2},
            {5.5, 15.0, 10.0, 2.8},
        }};

    UObject* clock_group =
        UObjectGlobals::NewObject<UObject>(tree, canvas_panel_class_);
    if (!clock_group) {
        last_attach_failure_ = 23;
        return false;
    }
    AddChildToCanvasParameters add_clock_group{clock_group};
    marker_canvas->ProcessEvent(add_child_to_canvas_, &add_clock_group);
    UObject* clock_group_slot = add_clock_group.return_value;
    if (!clock_group_slot) {
        last_attach_failure_ = 24;
        return false;
    }
    set_slot_vector(
        clock_group_slot, set_slot_position_,
        local_center - kClockReferenceWidth * 0.5 * umg_unit_scale_,
        local_center + kClockReferenceTop * umg_unit_scale_);
    clock_position_x_ = local_center - kClockReferenceWidth * 0.5 * umg_unit_scale_;
    clock_position_y_ = local_center + kClockReferenceTop * umg_unit_scale_;
    set_slot_vector(
        clock_group_slot, set_slot_size_,
        kClockReferenceWidth * umg_unit_scale_,
        kClockReferenceHeight * umg_unit_scale_);
    ZOrderParameters clock_group_z_order{static_cast<std::int32_t>(
        kCompactUmgMarkerCapacity * kCompactUmgMarkerPieceCount + 2U)};
    clock_group_slot->ProcessEvent(
        set_slot_z_order_, &clock_group_z_order);
    set_visibility(clock_group, set_visibility_, kCollapsed);

    std::array<std::array<UObject*, kCompactClockSegmentCount>,
               kCompactClockDigitCount> clock_digit_pieces{};
    std::array<std::array<UObject*, kCompactClockSegmentCount>,
               kCompactClockDigitCount> clock_digit_slots{};
    for (std::size_t digit = 0; digit < kCompactClockDigitCount; ++digit) {
        for (std::size_t segment = 0;
             segment < kCompactClockSegmentCount; ++segment) {
            UObject* border =
                UObjectGlobals::NewObject<UObject>(tree, border_class_);
            if (!border) {
                last_attach_failure_ = 25;
                return false;
            }
            set_brush_color(border, set_brush_color_, kClockDigit);
            AddChildToCanvasParameters add_segment{border};
            clock_group->ProcessEvent(add_child_to_canvas_, &add_segment);
            UObject* slot = add_segment.return_value;
            if (!slot) {
                last_attach_failure_ = 26;
                return false;
            }
            const auto& geometry = kClockSegmentGeometry[segment];
            set_slot_vector(
                slot, set_slot_position_,
                (kClockDigitLeft[digit] + geometry[0]
                    - geometry[2] * 0.5) * umg_unit_scale_,
                (geometry[1] - geometry[3] * 0.5) * umg_unit_scale_);
            set_slot_vector(
                slot, set_slot_size_,
                geometry[2] * umg_unit_scale_,
                geometry[3] * umg_unit_scale_);
            ZOrderParameters z_order{1};
            slot->ProcessEvent(set_slot_z_order_, &z_order);
            set_visibility(border, set_visibility_, kCollapsed);
            clock_digit_pieces[digit][segment] = border;
            clock_digit_slots[digit][segment] = slot;
        }
    }

    std::array<UObject*, kCompactClockColonPieceCount> clock_colon_pieces{};
    std::array<UObject*, kCompactClockColonPieceCount> clock_colon_slots{};
    constexpr std::array<double, kCompactClockColonPieceCount>
        kClockColonY{{11.0, 21.0}};
    for (std::size_t piece = 0; piece < kCompactClockColonPieceCount;
         ++piece) {
        UObject* border =
            UObjectGlobals::NewObject<UObject>(tree, border_class_);
        if (!border) {
            last_attach_failure_ = 27;
            return false;
        }
        set_brush_color(border, set_brush_color_, kClockDigit);
        AddChildToCanvasParameters add_colon{border};
        clock_group->ProcessEvent(add_child_to_canvas_, &add_colon);
        UObject* slot = add_colon.return_value;
        if (!slot) {
            last_attach_failure_ = 28;
            return false;
        }
        set_slot_vector(
            slot, set_slot_position_,
            38.5 * umg_unit_scale_,
            (kClockColonY[piece] - 1.7) * umg_unit_scale_);
        set_slot_vector(
            slot, set_slot_size_,
            3.4 * umg_unit_scale_, 3.4 * umg_unit_scale_);
        ZOrderParameters z_order{1};
        slot->ProcessEvent(set_slot_z_order_, &z_order);
        set_visibility(border, set_visibility_, kVisible);
        clock_colon_pieces[piece] = border;
        clock_colon_slots[piece] = slot;
    }

    std::array<UObject*, kCompactClockPhasePieceCount> clock_phase_pieces{};
    std::array<UObject*, kCompactClockPhasePieceCount> clock_phase_slots{};
    for (std::size_t piece = 0; piece < kCompactClockPhasePieceCount;
         ++piece) {
        UObject* border =
            UObjectGlobals::NewObject<UObject>(tree, border_class_);
        if (!border) {
            last_attach_failure_ = 29;
            return false;
        }
        set_brush(border, set_brush_, clock_phase_brush_template_);
        AddChildToCanvasParameters add_phase_piece{border};
        clock_group->ProcessEvent(add_child_to_canvas_, &add_phase_piece);
        UObject* slot = add_phase_piece.return_value;
        if (!slot) {
            last_attach_failure_ = 30;
            return false;
        }
        ZOrderParameters z_order{static_cast<std::int32_t>(piece + 1U)};
        slot->ProcessEvent(set_slot_z_order_, &z_order);
        set_visibility(border, set_visibility_, kCollapsed);
        clock_phase_pieces[piece] = border;
        clock_phase_slots[piece] = slot;
    }

    // Publish the weak transaction handles before the sole viewport mutation.
    // If AddToViewport faults after taking ownership, attach_guarded can still
    // remove this exact host without retaining a raw runtime object.
    host_ = host;
    widget_tree_ = tree;
    host_root_panel_ = marker_canvas;
    root_panel_ = movement_group;
    for (std::size_t channel = 0;
         channel < kCompactUmgHeightChannelCount; ++channel) {
        height_groups_[channel] = height_groups[channel];
        height_group_slots_[channel] = height_group_slots[channel];
    }
    clock_group_ = clock_group;
    clock_group_slot_ = clock_group_slot;
    minimap_layer_ = layer;
    for (std::size_t index = 0; index < kCompactUmgMarkerCapacity; ++index) {
        for (std::size_t piece = 0; piece < kCompactUmgMarkerPieceCount;
             ++piece) {
            marker_pieces_[index][piece] = pieces[index][piece];
            marker_piece_slots_[index][piece] = slots[index][piece];
        }
    }
    for (std::size_t channel = 0;
         channel < kCompactUmgHeightChannelCount; ++channel) {
        for (std::size_t piece = 0; piece < kCompactUmgHeightPieceCount;
             ++piece) {
            height_pieces_[channel][piece] = height_pieces[channel][piece];
            height_piece_slots_[channel][piece] = height_slots[channel][piece];
        }
    }
    for (std::size_t digit = 0; digit < kCompactClockDigitCount; ++digit) {
        for (std::size_t segment = 0;
             segment < kCompactClockSegmentCount; ++segment) {
            clock_digit_pieces_[digit][segment] =
                clock_digit_pieces[digit][segment];
            clock_digit_piece_slots_[digit][segment] =
                clock_digit_slots[digit][segment];
        }
    }
    for (std::size_t piece = 0; piece < kCompactClockColonPieceCount;
         ++piece) {
        clock_colon_pieces_[piece] = clock_colon_pieces[piece];
        clock_colon_piece_slots_[piece] = clock_colon_slots[piece];
    }
    for (std::size_t piece = 0; piece < kCompactClockPhasePieceCount;
         ++piece) {
        clock_phase_pieces_[piece] = clock_phase_pieces[piece];
        clock_phase_piece_slots_[piece] = clock_phase_slots[piece];
    }
    marker_kind_codes_.fill(0xFFU);
    area_quest_marker_shape_codes_.fill(0xFFU);
    area_quest_marker_center_x_.fill(0.0);
    area_quest_marker_center_y_.fill(0.0);
    area_quest_marker_scaled_size_.fill(0.0);
    area_quest_height_profiles_.fill(dswros::AreaQuestHeightProfile{});
    area_quest_height_active_.fill(false);
    height_visible_.fill(false);
    height_transform_valid_.fill(false);
    height_kind_codes_.fill(0xFFU);
    mini_game_height_shape_codes_.fill(0xFFU);
    height_marker_half_widths_.fill(0.0);
    height_angle_degrees_.fill(0.0);
    height_translation_x_.fill(0.0);
    clock_visible_ = false;
    clock_minute_valid_ = false;
    clock_phase_valid_ = false;
    clock_minute_ = 0;
    clock_phase_ = 0xFFU;
    for (auto& digit : clock_segment_visible_) {
        digit.fill(false);
    }

    host_origin_x_ = viewport_layout->host_origin_x;
    host_origin_y_ = viewport_layout->host_origin_y;

    // Keep the Blueprint-created tree and Slate lifecycle intact while the
    // authored visual children remain detached from the root Canvas.
    set_visibility(host, set_visibility_, kHitTestInvisible);
    AddToViewportParameters add_to_viewport{kViewportZOrder};
    host->ProcessEvent(add_to_viewport_, &add_to_viewport);
    VectorParameters viewport_alignment{{0.0, 0.0}};
    host->ProcessEvent(set_alignment_in_viewport_, &viewport_alignment);
    VectorParameters viewport_size_parameter{{host_size, host_size}};
    host->ProcessEvent(
        set_desired_size_in_viewport_, &viewport_size_parameter);
    PositionInViewportParameters viewport_position{{
        host_origin_x_,
        host_origin_y_}, true};
    host->ProcessEvent(set_position_in_viewport_, &viewport_position);
    // AddToViewport runs the cloned PlayerIcon Blueprint's Construct path,
    // which may restore the class-default hidden visibility after the
    // pre-attach write above. Publish the authoritative visible state once
    // more after construction so the compact radar never needs a later menu
    // suppression edge (such as opening/closing the world map) to appear.
    set_visibility(host, set_visibility_, kHitTestInvisible);
    host->ProcessEvent(force_layout_prepass_, nullptr);

    last_attach_failure_ = 0;
    menu_suppressed_ = false;
    active_marker_count_ = 0;
    ++attach_count_;
    state_ = CompactUmgRendererState::Attached;
    bind_clock_owner_guarded(layer);
    return true;
}

bool CompactUmgRenderer::read_minimap_scale(double& scale) noexcept {
    if (!activation_active_
        || (state_ != CompactUmgRendererState::Attached
            && state_ != CompactUmgRendererState::Suppressed)) {
        return false;
    }
    return read_minimap_scale_guarded(scale);
}

bool CompactUmgRenderer::read_minimap_scale_guarded(double& scale) noexcept {
#if defined(_MSC_VER)
    __try {
        return read_minimap_scale_unsafe(scale);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return read_minimap_scale_unsafe(scale);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::read_minimap_scale_unsafe(double& scale) {
    UObject* layer = minimap_layer_.Get();
    if (!layer) {
        return false;
    }
    UObject* layer_map = read_object_property(layer, L"LayerMap");
    UObject* map_overlay = read_object_property(layer_map, L"MapOverlay");
    if (!map_overlay) {
        return false;
    }
    if (UObject* player_icon =
            read_object_property(layer_map, L"PlayerIconWidget")) {
        static_cast<void>(refresh_viewport_layout_unsafe(player_icon));
    }
    refresh_clock_layout_unsafe();
    if ((!render_transform_property_ || !scale_property_ || !scale_x_property_)
        && !resolve_minimap_scale_schema(map_overlay)) {
        return false;
    }
    void* transform_value =
        render_transform_property_->ContainerPtrToValuePtr<void>(map_overlay);
    void* scale_value = transform_value
        ? scale_property_->ContainerPtrToValuePtr<void>(transform_value)
        : nullptr;
    void* x_value = scale_value
        ? scale_x_property_->ContainerPtrToValuePtr<void>(scale_value)
        : nullptr;
    if (!x_value) {
        return false;
    }
    const double value =
        scale_x_property_->GetFloatingPointPropertyValue(x_value);
    if (!std::isfinite(value) || value <= 0.0) {
        return false;
    }
    scale = value;
    return true;
}

void CompactUmgRenderer::bind_clock_owner_guarded(UObject* minimap_layer) noexcept {
#if defined(_MSC_VER)
    __try { bind_clock_owner_unsafe(minimap_layer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { clock_geometry_faulted_ = true; }
#else
    try { bind_clock_owner_unsafe(minimap_layer); }
    catch (...) { clock_geometry_faulted_ = true; }
#endif
}

void CompactUmgRenderer::bind_clock_owner_unsafe(UObject* minimap_layer) {
    clock_main_panel_ = FWeakObjectPtr{};
    if (!clock_geometry_schema_ready_ || !minimap_layer
        || !minimap_layer->GetWorld()) return;
    UObject* owner = minimap_layer;
    for (std::size_t depth = 0; owner && depth < 8; ++depth) {
        if (read_clock_owned_object(owner, L"DLayerMiniMap") == minimap_layer) {
            if (owner->GetWorld() == minimap_layer->GetWorld()
                && owner->IsA(clock_geometry_widget_class_)) clock_main_panel_ = owner;
            return;
        }
        owner = owner->GetOuterPrivate();
    }
}

bool CompactUmgRenderer::read_clock_gap_position_guarded(
    double& left, double& top) noexcept {
#if defined(_MSC_VER)
    __try { return read_clock_gap_position_unsafe(left, top); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        clock_geometry_faulted_ = true;
        return false;
    }
#else
    try { return read_clock_gap_position_unsafe(left, top); }
    catch (...) {
        clock_geometry_faulted_ = true;
        return false;
    }
#endif
}

bool CompactUmgRenderer::read_clock_gap_position_unsafe(double& left, double& top) {
    UObject* layer = minimap_layer_.Get();
    UObject* main_panel = clock_main_panel_.Get();
    UObject* fixed_root = host_root_panel_.Get();
    UObject* slate_library = clock_slate_library_.Get();
    if (!layer || !main_panel || !fixed_root || !slate_library
        || !layer->GetWorld() || layer->GetWorld() != main_panel->GetWorld()
        || fixed_root->GetWorld() != layer->GetWorld()
        || read_clock_owned_object(main_panel, L"DLayerMiniMap") != layer) return false;
    // RetainerBox is the minimap's fixed clipping surface. MapOverlay carries
    // zoom transforms and must never stand in for its visible screen boundary.
    UObject* map = read_clock_owned_object(layer, L"LayerMap");
    UObject* minimap_clip = read_clock_owned_object(map, L"RetainerBox");
    UObject* quest = read_clock_owned_object(main_panel, L"DLayerQuest");
    if (!clock_widget_paints_to_owner(minimap_clip, main_panel, clock_geometry_widget_class_)
        || !clock_widget_paints_to_owner(quest, main_panel, clock_geometry_widget_class_)) return false;
    GeometryReflectionSchema schema{};
    if (!resolve_geometry_reflection_schema(clock_get_cached_geometry_,
            clock_get_local_size_, clock_local_to_absolute_, clock_absolute_to_local_, schema)) return false;
    GeometryCallParameters root_geometry(clock_get_cached_geometry_);
    if (!root_geometry.valid()) return false;
    fixed_root->ProcessEvent(clock_get_cached_geometry_, root_geometry.data());
    double width{}, height{};
    if (!read_geometry_local_size(slate_library, clock_get_local_size_, schema,
            schema.cached_geometry_return, root_geometry.data(), width, height)) return false;
    const double host_extent = kReferenceHostHalfSize * 2.0 * umg_unit_scale_;
    if (std::abs(width - host_extent) > 0.5 * umg_unit_scale_
        || std::abs(height - host_extent) > 0.5 * umg_unit_scale_) return false;
    dswros::CompactClockRect minimap_rect{}, quest_rect{};
    if (!read_clock_widget_rect(minimap_clip, slate_library,
            clock_get_cached_geometry_, clock_get_local_size_, clock_local_to_absolute_,
            clock_absolute_to_local_, schema, root_geometry.data(), minimap_rect)
        || !read_clock_widget_rect(quest, slate_library,
            clock_get_cached_geometry_, clock_get_local_size_, clock_local_to_absolute_,
            clock_absolute_to_local_, schema, root_geometry.data(), quest_rect)) return false;
    const double pixels_per_local = viewport_dpi_scale_ * host_render_scale_;
    if (!std::isfinite(pixels_per_local) || pixels_per_local <= 0.0) return false;
    const dswros::CompactClockRect usable_host{
        std::max(0.0, -host_origin_x_ / pixels_per_local),
        std::max(0.0, -host_origin_y_ / pixels_per_local),
        std::min(width, (viewport_width_ - host_origin_x_) / pixels_per_local),
        std::min(height, (viewport_height_ - host_origin_y_) / pixels_per_local)};
    const auto position = dswros::calculate_compact_clock_gap_position(
        minimap_rect, quest_rect, usable_host, umg_unit_scale_);
    if (!position) return false;
    left = position->left;
    top = position->top;
    return true;
}

void CompactUmgRenderer::refresh_clock_layout_unsafe() {
    // Called only by the existing low-frequency minimap-scale/layout service.
    // Suppressed hosts have stale cached Slate geometry; do not reflect them.
    if (menu_suppressed_ || state_ != CompactUmgRendererState::Attached) return;
    const double centre = kReferenceHostHalfSize * umg_unit_scale_;
    double left = centre - dswros::kCompactClockReferenceWidth * 0.5 * umg_unit_scale_;
    double top = centre + (kReferenceMarkerExtent + 8.0) * umg_unit_scale_;
    clock_gap_layout_active_ = false;
    if (clock_geometry_defer_) {
        clock_geometry_defer_ = false;
    } else if (clock_geometry_schema_ready_ && !clock_geometry_faulted_) {
        clock_gap_layout_active_ = read_clock_gap_position_guarded(left, top);
    }
    if (!dswros::compact_clock_position_changed(
            {clock_position_x_, clock_position_y_}, {left, top}, umg_unit_scale_)) return;
    UObject* slot = clock_group_slot_.Get();
    if (!slot || !slot->IsA(canvas_panel_slot_class_)
        || read_clock_owned_object(slot, L"Content") != clock_group_.Get()
        || read_clock_owned_object(slot, L"Parent") != host_root_panel_.Get()) return;
    set_slot_vector(slot, set_slot_position_, left, top);
    // Retain the last APPLIED position so repeated sub-threshold deltas accrue.
    clock_position_x_ = left;
    clock_position_y_ = top;
}

bool CompactUmgRenderer::refresh_viewport_layout_unsafe(
    UObject* player_icon) {
    UObject* host = host_.Get();
    UObject* layout_library = widget_layout_library_.Get();
    if (!player_icon || !host || !layout_library || !get_owning_player_
        || !get_viewport_size_ || !get_viewport_scale_
        || !set_position_in_viewport_ || !set_render_pivot_
        || !set_render_scale_ || !force_layout_prepass_) {
        return false;
    }

    ObjectReturnParameters owning_player{};
    player_icon->ProcessEvent(get_owning_player_, &owning_player);
    if (!owning_player.return_value) {
        return false;
    }

    ViewportSizeParameters viewport_size{owning_player.return_value};
    layout_library->ProcessEvent(get_viewport_size_, &viewport_size);
    ViewportScaleParameters viewport_scale{owning_player.return_value};
    layout_library->ProcessEvent(get_viewport_scale_, &viewport_scale);
    if (!std::isfinite(viewport_size.return_value.x)
        || !std::isfinite(viewport_size.return_value.y)
        || viewport_size.return_value.x < 640.0
        || viewport_size.return_value.y < 360.0
        || !std::isfinite(viewport_scale.return_value)
        || viewport_scale.return_value < 0.1F
        || viewport_scale.return_value > 10.0F) {
        return false;
    }

    const auto layout = dswros::calculate_compact_viewport_layout(
        viewport_size.return_value.x, viewport_size.return_value.y,
        viewport_scale.return_value, umg_unit_scale_);
    if (!layout) {
        return false;
    }
    const bool viewport_changed =
        std::abs(viewport_width_ - viewport_size.return_value.x)
            > kViewportDimensionEpsilon
        || std::abs(viewport_height_ - viewport_size.return_value.y)
            > kViewportDimensionEpsilon
        || std::abs(viewport_dpi_scale_ - viewport_scale.return_value)
            > kViewportScaleEpsilon;
    if (!viewport_changed) {
        return true;
    }

    if (std::abs(host_render_scale_ - layout->host_render_scale)
        > kViewportScaleEpsilon) {
        VectorParameters top_left_pivot{{0.0, 0.0}};
        host->ProcessEvent(set_render_pivot_, &top_left_pivot);
        VectorParameters render_scale{{
            layout->host_render_scale, layout->host_render_scale}};
        host->ProcessEvent(set_render_scale_, &render_scale);
    }
    PositionInViewportParameters viewport_position{{
        layout->host_origin_x, layout->host_origin_y}, true};
    host->ProcessEvent(set_position_in_viewport_, &viewport_position);
    host->ProcessEvent(force_layout_prepass_, nullptr);

    viewport_width_ = viewport_size.return_value.x;
    viewport_height_ = viewport_size.return_value.y;
    viewport_dpi_scale_ = viewport_scale.return_value;
    display_scale_ = layout->display_scale;
    host_origin_x_ = layout->host_origin_x;
    host_origin_y_ = layout->host_origin_y;
    host_render_scale_ = layout->host_render_scale;
    // GetCachedGeometry still describes the preceding Slate paint this tick.
    clock_geometry_defer_ = true;
    return true;
}

bool CompactUmgRenderer::resolve_minimap_scale_schema(UObject* map_overlay) {
    if (!map_overlay || !map_overlay->GetClassPrivate()) {
        return false;
    }
    FProperty* transform{};
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(
             map_overlay->GetClassPrivate())) {
        if (property->GetName() == STR("RenderTransform")) {
            transform = property;
            break;
        }
    }
    auto* transform_struct = CastField<FStructProperty>(transform);
    if (!transform_struct || !transform_struct->GetStruct()) {
        return false;
    }
    FProperty* scale{};
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(
             transform_struct->GetStruct())) {
        if (property->GetName() == STR("Scale")) {
            scale = property;
            break;
        }
    }
    auto* scale_struct = CastField<FStructProperty>(scale);
    if (!scale_struct || !scale_struct->GetStruct()) {
        return false;
    }
    FNumericProperty* x{};
    for (FProperty* property : DSNWRPR_PROPERTIES_IN_CHAIN(
             scale_struct->GetStruct())) {
        if (property->GetName() == STR("X")) {
            x = CastField<FNumericProperty>(property);
            break;
        }
    }
    if (!x || !x->IsFloatingPoint()) {
        return false;
    }
    render_transform_property_ = transform;
    scale_property_ = scale_struct;
    scale_x_property_ = x;
    return true;
}

void CompactUmgRenderer::set_menu_suppressed(bool suppressed) noexcept {
    if (!activation_active_ || menu_suppressed_ == suppressed
        || (state_ != CompactUmgRendererState::Attached
            && state_ != CompactUmgRendererState::Suppressed)) {
        return;
    }
    if (!set_menu_suppressed_guarded(suppressed)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::set_menu_suppressed_guarded(bool suppressed) noexcept {
#if defined(_MSC_VER)
    __try {
        return set_menu_suppressed_unsafe(suppressed);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return set_menu_suppressed_unsafe(suppressed);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::set_menu_suppressed_unsafe(bool suppressed) {
    UObject* host = host_.Get();
    if (!host) {
        return false;
    }
    set_visibility(
        host,
        set_visibility_,
        suppressed ? kCollapsed : kHitTestInvisible);
    menu_suppressed_ = suppressed;
    ++suppression_change_count_;
    state_ = suppressed
        ? CompactUmgRendererState::Suppressed
        : CompactUmgRendererState::Attached;
    return true;
}

void CompactUmgRenderer::rebind(
    const CompactUmgMarkerArray& markers, std::size_t count) noexcept {
    if (!activation_active_ || menu_suppressed_
        || state_ != CompactUmgRendererState::Attached) {
        return;
    }
    if (count > kCompactUmgMarkerCapacity) {
        ++input_overflow_count_;
        count = kCompactUmgMarkerCapacity;
    }
    if (!rebind_guarded(markers, count)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::rebind_guarded(
    const CompactUmgMarkerArray& markers, std::size_t count) noexcept {
#if defined(_MSC_VER)
    __try {
        return rebind_unsafe(markers, count);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return rebind_unsafe(markers, count);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::rebind_unsafe(
    const CompactUmgMarkerArray& markers, std::size_t count) {
    if (!host_.Get() || !root_panel_.Get()
        || viewport_width_ <= 0.0 || viewport_height_ <= 0.0
        || !std::isfinite(umg_unit_scale_) || umg_unit_scale_ <= 0.0) {
        return false;
    }

    const double local_center = kReferenceHostHalfSize * umg_unit_scale_;
    const double marker_extent = kReferenceMarkerExtent * umg_unit_scale_;
    std::size_t visible_count = 0;
    std::array<bool, kCompactUmgHeightChannelCount> height_requested{};
    std::array<double, kCompactUmgHeightChannelCount> height_anchor_x{};
    std::array<double, kCompactUmgHeightChannelCount> height_anchor_y{};
    std::array<double, kCompactUmgHeightChannelCount> height_marker_size{};
    std::array<double, kCompactUmgHeightChannelCount> height_angle{};
    std::array<CompactUmgMarkerKind, kCompactUmgHeightChannelCount>
        height_kind{{
            CompactUmgMarkerKind::TreasureOther,
            CompactUmgMarkerKind::AreaQuest,
            CompactUmgMarkerKind::Mole,
        }};
    for (std::size_t index = 0; index < kCompactUmgMarkerCapacity; ++index) {
        std::array<UObject*, kCompactUmgMarkerPieceCount> pieces{};
        std::array<UObject*, kCompactUmgMarkerPieceCount> slots{};
        for (std::size_t piece = 0; piece < kCompactUmgMarkerPieceCount;
             ++piece) {
            pieces[piece] = marker_pieces_[index][piece].Get();
            slots[piece] = marker_piece_slots_[index][piece].Get();
            if (!pieces[piece] || !slots[piece]) {
                return false;
            }
        }

        const bool requested = index < count;
        const CompactUmgMarker& marker = markers[index];
        const auto kind_code = static_cast<std::uint8_t>(marker.kind);
        const bool kind_valid = kind_code
            <= static_cast<std::uint8_t>(CompactUmgMarkerKind::BirdEgg);
        const bool area_quest_marker = marker.kind
            == CompactUmgMarkerKind::AreaQuest;
        const bool encounter_marker =
            dswros::is_encounter_height_marker(marker.kind);
        const bool band_height_marker = area_quest_marker || encounter_marker;
        const bool height_input_valid = !marker.show_height
            || encounter_marker // Invalid encounter height preserves its glyph.
            || (area_quest_marker
                ? dswros::area_quest_height_profile_valid(
                    marker.area_quest_height_profile)
                    && std::isfinite(
                        marker.area_quest_comparable_player_z)
                : std::isfinite(marker.height_angle_degrees));
        const bool unavailable_state_valid =
            !marker.height_source_unavailable
            || encounter_marker
            || (area_quest_marker && !marker.show_height);
        const bool valid = requested && kind_valid
            && std::isfinite(marker.normalized_x)
            && std::isfinite(marker.normalized_y)
            && std::isfinite(marker.reference_size)
            && marker.reference_size > 0.0
            && height_input_valid && unavailable_state_valid;
        if (!valid) {
            if (marker_visible_[index]) {
                for (UObject* piece : pieces) {
                    set_visibility(piece, set_visibility_, kCollapsed);
                }
                marker_visible_[index] = false;
            }
            marker_reference_sizes_[index] = 0.0;
            marker_kind_codes_[index] = 0xFFU;
            area_quest_marker_shape_codes_[index] = 0xFFU;
            area_quest_marker_center_x_[index] = 0.0;
            area_quest_marker_center_y_[index] = 0.0;
            area_quest_marker_scaled_size_[index] = 0.0;
            area_quest_height_profiles_[index] = {};
            area_quest_height_active_[index] = false;
            continue;
        }

        const double x = local_center
            + std::clamp(marker.normalized_x, -1.0, 1.0) * marker_extent;
        const double y = local_center
            + std::clamp(marker.normalized_y, -1.0, 1.0) * marker_extent;
        const double reference_size = std::clamp(
            marker.reference_size,
            kMinimumReferenceMarkerSize,
            kMaximumReferenceMarkerSize);
        const double scaled_size = reference_size * umg_unit_scale_;
        const bool style_changed = marker_kind_codes_[index] != kind_code;
        for (std::size_t piece = 0;
             piece < kCompactUmgMarkerPieceCount; ++piece) {
            const MarkerPieceStyle style =
                marker_piece_style(marker.kind, piece, reference_size);
            set_marker_piece_geometry(
                slots[piece], set_slot_position_, set_slot_size_, style,
                x, y, scaled_size);
            if (style_changed) {
                if (!brush_templates_ready_) {
                    return false;
                }
                const auto& brush = marker.kind == CompactUmgMarkerKind::Fly
                    ? fly_outline_brush_template_
                    : marker.kind == CompactUmgMarkerKind::AreaQuest
                            && piece == 0
                        ? area_quest_brush_template_
                        : marker.kind == CompactUmgMarkerKind::BirdEgg
                            ? bird_egg_brush_template_
                        : solid_brush_template_;
                set_brush(pieces[piece], set_brush_, brush);
                set_brush_color(
                    pieces[piece], set_brush_color_, style.color);
                set_render_angle(
                    pieces[piece], set_render_angle_,
                    style.angle_degrees);
            }
        }
        marker_reference_sizes_[index] = reference_size;
        marker_kind_codes_[index] = kind_code;
        if (!marker_visible_[index] || style_changed) {
            for (UObject* piece : pieces) {
                set_visibility(piece, set_visibility_, kVisible);
            }
            marker_visible_[index] = true;
        }
        if (band_height_marker) {
            const auto shape = encounter_marker
                ? (marker.show_height && !marker.height_source_unavailable
                    ? dswros::encounter_height_indicator_shape(
                        marker.area_quest_height_profile,
                        marker.area_quest_comparable_player_z)
                    : dswros::AreaQuestHeightIndicatorShape::Unavailable)
                : marker.show_height
                ? dswros::area_quest_height_indicator_shape(
                    marker.area_quest_height_profile,
                    marker.area_quest_comparable_player_z)
                : marker.height_source_unavailable
                    ? dswros::AreaQuestHeightIndicatorShape::Unavailable
                    : dswros::AreaQuestHeightIndicatorShape::Aligned;
            const auto shape_code = static_cast<std::uint8_t>(shape);
            const bool directional = shape
                    == dswros::AreaQuestHeightIndicatorShape::Above
                || shape == dswros::AreaQuestHeightIndicatorShape::Below;
            if ((style_changed || directional
                    || area_quest_marker_shape_codes_[index] != shape_code)
                && !configure_area_quest_marker_shape_unsafe(
                    index, shape_code, x, y, scaled_size)) {
                return false;
            }
            area_quest_marker_shape_codes_[index] = shape_code;
            area_quest_marker_center_x_[index] = x;
            area_quest_marker_center_y_[index] = y;
            area_quest_marker_scaled_size_[index] = scaled_size;
            area_quest_height_profiles_[index] = marker.show_height
                ? marker.area_quest_height_profile
                : dswros::AreaQuestHeightProfile{};
            area_quest_height_active_[index] = marker.show_height
                && !marker.height_source_unavailable
                && (!encounter_marker || dswros::encounter_height_profile_valid(
                    marker.area_quest_height_profile));
        } else {
            area_quest_marker_shape_codes_[index] = 0xFFU;
            area_quest_marker_center_x_[index] = 0.0;
            area_quest_marker_center_y_[index] = 0.0;
            area_quest_marker_scaled_size_[index] = 0.0;
            area_quest_height_profiles_[index] = {};
            area_quest_height_active_[index] = false;
        }
        const bool treasure_height = marker.kind
            <= CompactUmgMarkerKind::TreasurePuzzle;
        const bool mini_game_height = marker.kind
                == CompactUmgMarkerKind::Fly
            || marker.kind == CompactUmgMarkerKind::Mole
            || marker.kind == CompactUmgMarkerKind::Wave;
        if (marker.show_height
            && (treasure_height || mini_game_height)) {
            const std::size_t channel = mini_game_height
                ? static_cast<std::size_t>(
                    CompactUmgHeightChannel::Mole)
                : static_cast<std::size_t>(
                    CompactUmgHeightChannel::Treasure);
            if (!height_requested[channel]) {
                height_requested[channel] = true;
                height_anchor_x[channel] = x;
                height_anchor_y[channel] = y;
                height_marker_size[channel] = scaled_size;
                height_angle[channel] = marker.height_angle_degrees;
                height_kind[channel] = marker.kind;
            }
        }
        ++visible_count;
    }

    for (std::size_t channel = 0;
         channel < kCompactUmgHeightChannelCount; ++channel) {
        UObject* height_group = height_groups_[channel].Get();
        UObject* height_group_slot = height_group_slots_[channel].Get();
        if (!height_group || !height_group_slot) {
            return false;
        }
        std::array<UObject*, kCompactUmgHeightPieceCount> height_pieces{};
        for (std::size_t piece = 0; piece < kCompactUmgHeightPieceCount;
             ++piece) {
            height_pieces[piece] = height_pieces_[channel][piece].Get();
            if (!height_pieces[piece]) {
                return false;
            }
        }
        if (!height_requested[channel]) {
            if (height_visible_[channel]) {
                set_visibility(
                    height_group, set_visibility_, kCollapsed);
                height_visible_[channel] = false;
            }
            height_transform_valid_[channel] = false;
            height_kind_codes_[channel] = 0xFFU;
            mini_game_height_shape_codes_[channel] = 0xFFU;
            height_marker_half_widths_[channel] = 0.0;
        } else {
            set_slot_vector(
                height_group_slot, set_slot_position_,
                height_anchor_x[channel], height_anchor_y[channel]);
            height_marker_half_widths_[channel] =
                height_marker_size[channel] * kReferenceTreasureHalfWidth;
            const bool mini_game_channel = channel
                == static_cast<std::size_t>(CompactUmgHeightChannel::Mole);
            if (mini_game_channel) {
                if (!configure_mini_game_height_indicator_unsafe(
                        channel, height_kind[channel], height_angle[channel],
                        true)) {
                    return false;
                }
            } else {
                if (!apply_height_pointer_transform_unsafe(
                        channel, height_angle[channel], true)) {
                    return false;
                }
                constexpr std::array<bool, kCompactUmgHeightPieceCount>
                    outline_order{{true, false, true, true, false, false}};
                const auto next_kind_code =
                    static_cast<std::uint8_t>(height_kind[channel]);
                if (height_kind_codes_[channel] != next_kind_code) {
                    const LinearColor fill =
                        height_pointer_fill_color(height_kind[channel]);
                    const LinearColor outline =
                        height_pointer_outline_color(height_kind[channel]);
                    for (std::size_t piece = 0;
                         piece < kCompactUmgHeightPieceCount; ++piece) {
                        set_brush_color(
                            height_pieces[piece], set_brush_color_,
                            outline_order[piece] ? outline : fill);
                    }
                    height_kind_codes_[channel] = next_kind_code;
                }
            }
            if (!height_visible_[channel]) {
                set_visibility(height_group, set_visibility_, kVisible);
                height_visible_[channel] = true;
            }
        }
    }

    // The supplied marker coordinates define a new motion anchor. Clear the
    // previous high-frequency offset as part of this low-frequency transaction.
    VectorParameters anchor_translation{{0.0, 0.0}};
    root_panel_.Get()->ProcessEvent(
        set_render_translation_, &anchor_translation);
    active_marker_count_ = visible_count;
    ++rebind_count_;
    return true;
}
void CompactUmgRenderer::translate(
    double normalized_dx, double normalized_dy) noexcept {
    if (!activation_active_ || menu_suppressed_
        || state_ != CompactUmgRendererState::Attached) {
        return;
    }
    if (!translate_guarded(normalized_dx, normalized_dy)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::translate_guarded(
    double normalized_dx, double normalized_dy) noexcept {
#if defined(_MSC_VER)
    __try {
        return translate_unsafe(normalized_dx, normalized_dy);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return translate_unsafe(normalized_dx, normalized_dy);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::translate_unsafe(
    double normalized_dx, double normalized_dy) {
    UObject* host = host_.Get();
    UObject* root_panel = root_panel_.Get();
    if (!host || !root_panel
        || !std::isfinite(normalized_dx) || !std::isfinite(normalized_dy)
        || !std::isfinite(umg_unit_scale_) || umg_unit_scale_ <= 0.0) {
        return false;
    }
    const double marker_extent = kReferenceMarkerExtent * umg_unit_scale_;
    VectorParameters translation{{
        normalized_dx * marker_extent,
        normalized_dy * marker_extent}};
    root_panel->ProcessEvent(set_render_translation_, &translation);
    ++translation_count_;
    return true;
}

void CompactUmgRenderer::update_height_pointer(
    CompactUmgHeightChannel channel,
    double height_angle_degrees) noexcept {
    const std::size_t channel_index = static_cast<std::size_t>(channel);
    if (!activation_active_ || menu_suppressed_
        || channel_index >= kCompactUmgHeightChannelCount
        || channel == CompactUmgHeightChannel::AreaQuest
        || !height_visible_[channel_index]
        || state_ != CompactUmgRendererState::Attached
        || !std::isfinite(height_angle_degrees)) {
        return;
    }
    if (!update_height_pointer_guarded(channel, height_angle_degrees)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::update_height_pointer_guarded(
    CompactUmgHeightChannel channel,
    double height_angle_degrees) noexcept {
#if defined(_MSC_VER)
    __try {
        return update_height_pointer_unsafe(channel, height_angle_degrees);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return update_height_pointer_unsafe(channel, height_angle_degrees);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::update_height_pointer_unsafe(
    CompactUmgHeightChannel channel,
    double height_angle_degrees) {
    if (channel == CompactUmgHeightChannel::Mole) {
        const std::size_t channel_index = static_cast<std::size_t>(channel);
        const auto kind = static_cast<CompactUmgMarkerKind>(
            height_kind_codes_[channel_index]);
        return configure_mini_game_height_indicator_unsafe(
            channel_index, kind, height_angle_degrees, false);
    }
    return apply_height_pointer_transform_unsafe(
        static_cast<std::size_t>(channel), height_angle_degrees, false);
}

void CompactUmgRenderer::update_area_quest_height_indicators(
    double comparable_player_z) noexcept {
    if (!activation_active_ || menu_suppressed_
        || state_ != CompactUmgRendererState::Attached
        || !std::isfinite(comparable_player_z)) {
        return;
    }
    if (!update_area_quest_height_indicators_guarded(comparable_player_z)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::update_area_quest_height_indicators_guarded(
    double comparable_player_z) noexcept {
#if defined(_MSC_VER)
    __try {
        return update_area_quest_height_indicators_unsafe(
            comparable_player_z);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return update_area_quest_height_indicators_unsafe(
            comparable_player_z);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::update_area_quest_height_indicators_unsafe(
    double comparable_player_z) {
    if (!std::isfinite(comparable_player_z)) {
        return false;
    }
    const std::size_t count = std::min(
        active_marker_count_, kCompactUmgMarkerCapacity);
    for (std::size_t index = 0; index < count; ++index) {
        const auto kind = static_cast<CompactUmgMarkerKind>(
            marker_kind_codes_[index]);
        if (!area_quest_height_active_[index]
            || (kind != CompactUmgMarkerKind::AreaQuest
                && !dswros::is_encounter_height_marker(kind))) {
            continue;
        }
        const dswros::AreaQuestHeightProfile& height_profile =
            area_quest_height_profiles_[index];
        const double center_x = area_quest_marker_center_x_[index];
        const double center_y = area_quest_marker_center_y_[index];
        const double scaled_size = area_quest_marker_scaled_size_[index];
        if (!dswros::area_quest_height_profile_valid(height_profile)
            || !std::isfinite(center_x)
            || !std::isfinite(center_y) || !std::isfinite(scaled_size)
            || scaled_size <= 0.0) {
            return false;
        }
        const auto shape = dswros::area_quest_height_indicator_shape(
            height_profile, comparable_player_z);
        const auto shape_code = static_cast<std::uint8_t>(shape);
        if (shape_code == area_quest_marker_shape_codes_[index]) {
            ++height_transform_skip_count_;
            continue;
        }
        if (!configure_area_quest_marker_shape_unsafe(
                index, shape_code, center_x, center_y, scaled_size)) {
            return false;
        }
        area_quest_marker_shape_codes_[index] = shape_code;
        ++height_transform_count_;
    }
    return true;
}

void CompactUmgRenderer::update_world_clock(
    bool available, std::uint32_t seconds) noexcept {
    if (!activation_active_
        || (state_ != CompactUmgRendererState::Attached
            && state_ != CompactUmgRendererState::Suppressed)) {
        return;
    }
    if (!available && !clock_visible_) {
        return;
    }
    const std::uint32_t minute = (seconds % 86400U) / 60U;
    if (available && clock_visible_ && clock_minute_valid_
        && minute == clock_minute_) {
        return;
    }
    if (!update_world_clock_guarded(available, seconds)) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        detach_guarded();
    }
}

bool CompactUmgRenderer::update_world_clock_guarded(
    bool available, std::uint32_t seconds) noexcept {
#if defined(_MSC_VER)
    __try {
        return update_world_clock_unsafe(available, seconds);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return update_world_clock_unsafe(available, seconds);
    } catch (...) {
        return false;
    }
#endif
}

bool CompactUmgRenderer::update_world_clock_unsafe(
    bool available, std::uint32_t seconds) {
    UObject* clock_group = clock_group_.Get();
    if (!clock_group) {
        return false;
    }
    if (!available) {
        if (clock_visible_) {
            set_visibility(clock_group, set_visibility_, kCollapsed);
        }
        clock_visible_ = false;
        clock_minute_valid_ = false;
        clock_phase_valid_ = false;
        return true;
    }

    const std::uint32_t minute = (seconds % 86400U) / 60U;
    const std::uint32_t hour = minute / 60U;
    const std::uint32_t minute_of_hour = minute % 60U;
    const std::array<std::uint32_t, kCompactClockDigitCount> digits{{
        hour / 10U,
        hour % 10U,
        minute_of_hour / 10U,
        minute_of_hour % 10U,
    }};
    constexpr std::array<std::uint8_t, 10> kDigitMasks{{
        0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U,
        0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU,
    }};
    for (std::size_t digit = 0; digit < kCompactClockDigitCount; ++digit) {
        const std::uint8_t mask = kDigitMasks[digits[digit]];
        for (std::size_t segment = 0;
             segment < kCompactClockSegmentCount; ++segment) {
            UObject* piece = clock_digit_pieces_[digit][segment].Get();
            if (!piece) {
                return false;
            }
            const bool next_visible =
                (mask & (std::uint8_t{1} << segment)) != 0;
            if (!clock_minute_valid_
                || clock_segment_visible_[digit][segment]
                    != next_visible) {
                set_visibility(
                    piece, set_visibility_,
                    next_visible ? kVisible : kCollapsed);
                clock_segment_visible_[digit][segment] = next_visible;
            }
        }
    }

    const std::uint8_t phase = static_cast<std::uint8_t>(
        dswros::compact_time_phase(seconds));
    if (!clock_phase_valid_ || clock_phase_ != phase) {
        const auto configure_phase_piece =
            [this](std::size_t index, double center_x, double center_y,
                   double width, double height, double angle,
                   const LinearColor& color, bool visible) {
                UObject* piece = clock_phase_pieces_[index].Get();
                UObject* slot = clock_phase_piece_slots_[index].Get();
                if (!piece || !slot) {
                    return false;
                }
                set_slot_vector(
                    slot, set_slot_position_,
                    (center_x - width * 0.5) * umg_unit_scale_,
                    (center_y - height * 0.5) * umg_unit_scale_);
                set_slot_vector(
                    slot, set_slot_size_,
                    width * umg_unit_scale_, height * umg_unit_scale_);
                set_render_angle(piece, set_render_angle_, angle);
                set_brush_color(piece, set_brush_color_, color);
                set_visibility(
                    piece, set_visibility_,
                    visible ? kVisible : kCollapsed);
                return true;
            };

        constexpr double dial_center_x = 98.0;
        constexpr double dial_center_y = 15.0;
        if (phase == 3U) {
            // A bounded open crescent and two-stroke star replace the former
            // nested diamonds. All ten attachment-time pieces are reused; the
            // phase path still mutates only at a day-phase edge.
            constexpr std::array<std::array<double, 5>, 8> kMoonPieces{{
                {{96.0, 7.5, 5.2, 2.4, -18.0}},
                {{92.8, 9.8, 5.0, 2.4, 45.0}},
                {{91.3, 14.0, 5.5, 2.4, 90.0}},
                {{92.4, 18.2, 5.0, 2.4, -45.0}},
                {{96.0, 20.5, 5.2, 2.4, 18.0}},
                {{99.2, 9.5, 3.4, 2.2, -25.0}},
                {{99.2, 18.5, 3.4, 2.2, 25.0}},
                {{93.8, 14.0, 3.0, 2.4, 90.0}},
            }};
            for (std::size_t piece = 0; piece < kMoonPieces.size(); ++piece) {
                const auto& geometry = kMoonPieces[piece];
                if (!configure_phase_piece(
                        piece, geometry[0], geometry[1], geometry[2],
                        geometry[3], geometry[4], kClockMoon, true)) {
                    return false;
                }
            }
            if (!configure_phase_piece(
                    8, 107.0, 9.0, 5.2, 1.8, 0.0,
                    kClockMorning, true)
                || !configure_phase_piece(
                    9, 107.0, 9.0, 1.8, 5.2, 0.0,
                    kClockMorning, true)) {
                return false;
            }
        } else {
            const LinearColor sun_color = phase == 0U
                ? kClockMorning
                : (phase == 1U ? kClockAfternoon : kClockEvening);
            if (phase == 1U) {
                // Afternoon uses the complete high sun and eight radial rays.
                if (!configure_phase_piece(
                        0, dial_center_x, dial_center_y,
                        24.0, 24.0, 45.0, kClockDial, true)
                    || !configure_phase_piece(
                        1, dial_center_x, dial_center_y,
                        9.0, 9.0, 45.0, sun_color, true)) {
                    return false;
                }
                constexpr double ray_radius = 9.0;
                for (std::size_t ray_index = 0; ray_index < 8U;
                     ++ray_index) {
                    const double angle =
                        static_cast<double>(ray_index) * 45.0;
                    const double radians = angle * kDegreesToRadians;
                    const double ray_x = dial_center_x
                        + std::sin(radians) * ray_radius;
                    const double ray_y = dial_center_y
                        - std::cos(radians) * ray_radius;
                    if (!configure_phase_piece(
                            ray_index + 2U, ray_x, ray_y,
                            1.8, 4.6, angle, sun_color, true)) {
                        return false;
                    }
                }
            } else {
                // Morning and evening presentation bands use opposite horizon
                // glyphs, making them distinguishable without relying on color
                // alone. Unused retained pieces are explicitly hidden.
                const bool morning = phase == 0U;
                const double sun_y = morning ? 16.5 : 13.5;
                const double horizon_y = morning ? 20.0 : 10.0;
                if (!configure_phase_piece(
                        0, dial_center_x, horizon_y,
                        24.0, 2.4, 0.0, kClockDial, true)
                    || !configure_phase_piece(
                        1, dial_center_x, sun_y,
                        10.0, 10.0, 45.0, sun_color, true)) {
                    return false;
                }
                constexpr std::array<double, 5> ray_angles{{
                    -60.0, -30.0, 0.0, 30.0, 60.0}};
                for (std::size_t ray = 0; ray < ray_angles.size(); ++ray) {
                    const double radians =
                        ray_angles[ray] * kDegreesToRadians;
                    const double ray_x = dial_center_x
                        + std::sin(radians) * 9.0;
                    const double ray_y = sun_y
                        + (morning ? -1.0 : 1.0)
                            * std::cos(radians) * 8.0;
                    if (!configure_phase_piece(
                            ray + 2U, ray_x, ray_y,
                            1.8, 4.4, ray_angles[ray], sun_color, true)) {
                        return false;
                    }
                }
                for (std::size_t piece = 7U;
                     piece < kCompactClockPhasePieceCount; ++piece) {
                    if (!configure_phase_piece(
                            piece, dial_center_x, dial_center_y,
                            1.0, 1.0, 0.0, sun_color, false)) {
                        return false;
                    }
                }
            }
        }
        clock_phase_ = phase;
        clock_phase_valid_ = true;
    }

    clock_minute_ = minute;
    clock_minute_valid_ = true;
    if (!clock_visible_) {
        set_visibility(clock_group, set_visibility_, kHitTestInvisible);
        clock_visible_ = true;
    }
    return true;
}

bool CompactUmgRenderer::apply_height_pointer_transform_unsafe(
    std::size_t channel,
    double height_angle_degrees, bool force) {
    if (channel >= kCompactUmgHeightChannelCount
        || channel == static_cast<std::size_t>(
            CompactUmgHeightChannel::AreaQuest)) {
        return false;
    }
    UObject* height_group = height_groups_[channel].Get();
    if (!height_group || !std::isfinite(height_angle_degrees)
        || !std::isfinite(height_marker_half_widths_[channel])
        || height_marker_half_widths_[channel] <= 0.0
        || !std::isfinite(umg_unit_scale_) || umg_unit_scale_ <= 0.0) {
        return false;
    }
    const double clamped_angle = std::clamp(
        height_angle_degrees, -85.0, 85.0);
    if (!force && height_transform_valid_[channel]
        && std::abs(clamped_angle - height_angle_degrees_[channel])
            < kHeightAngleEpsilonDegrees) {
        ++height_transform_skip_count_;
        return true;
    }

    const double arrow_right_extent = height_arrow_right_extent(
        clamped_angle, umg_unit_scale_, true);
    const double translation_x = -height_marker_half_widths_[channel]
        - kReferenceHeightClearance * umg_unit_scale_
        - arrow_right_extent;
    if (!std::isfinite(translation_x)) {
        return false;
    }
    VectorParameters translation{{translation_x, 0.0}};
    height_group->ProcessEvent(set_render_translation_, &translation);
    set_render_angle(
        height_group, set_render_angle_, clamped_angle);
    height_angle_degrees_[channel] = clamped_angle;
    height_translation_x_[channel] = translation_x;
    height_transform_valid_[channel] = true;
    ++height_transform_count_;
    return true;
}

bool CompactUmgRenderer::configure_mini_game_height_indicator_unsafe(
    std::size_t channel,
    CompactUmgMarkerKind kind,
    double height_angle_degrees,
    bool force) {
    const std::size_t mini_game_channel = static_cast<std::size_t>(
        CompactUmgHeightChannel::Mole);
    const bool kind_valid = kind == CompactUmgMarkerKind::Fly
        || kind == CompactUmgMarkerKind::Mole
        || kind == CompactUmgMarkerKind::Wave;
    if (channel != mini_game_channel || !kind_valid
        || !std::isfinite(height_angle_degrees)
        || !std::isfinite(height_marker_half_widths_[channel])
        || height_marker_half_widths_[channel] <= 0.0
        || !std::isfinite(umg_unit_scale_) || umg_unit_scale_ <= 0.0) {
        return false;
    }

    UObject* height_group = height_groups_[channel].Get();
    if (!height_group) {
        return false;
    }
    std::array<UObject*, kCompactUmgHeightPieceCount> pieces{};
    std::array<UObject*, kCompactUmgHeightPieceCount> slots{};
    for (std::size_t piece = 0; piece < kCompactUmgHeightPieceCount;
         ++piece) {
        pieces[piece] = height_pieces_[channel][piece].Get();
        slots[piece] = height_piece_slots_[channel][piece].Get();
        if (!pieces[piece] || !slots[piece]) {
            return false;
        }
    }

    const auto shape = dswros::mini_game_height_indicator_shape(
        height_angle_degrees);
    const auto shape_code = static_cast<std::uint8_t>(shape);
    const auto kind_code = static_cast<std::uint8_t>(kind);
    if (!force && height_transform_valid_[channel]
        && mini_game_height_shape_codes_[channel] == shape_code
        && height_kind_codes_[channel] == kind_code) {
        ++height_transform_skip_count_;
        return true;
    }

    if (shape == dswros::MiniGameHeightIndicatorShape::Hidden) {
        for (UObject* piece : pieces) {
            set_visibility(piece, set_visibility_, kCollapsed);
        }
        mini_game_height_shape_codes_[channel] = shape_code;
        height_kind_codes_[channel] = kind_code;
        height_angle_degrees_[channel] = 0.0;
        height_transform_valid_[channel] = true;
        ++height_transform_count_;
        return true;
    }

    const double center = kReferenceHeightGroupHalfSize * umg_unit_scale_;
    const double half_width =
        kReferenceMiniGameTriangleHalfWidth * umg_unit_scale_;
    const double half_height =
        kReferenceMiniGameTriangleHalfHeight * umg_unit_scale_;
    const double fill_bar_height =
        kReferenceMiniGameTriangleFillBarHeight * umg_unit_scale_;
    const double outline_width =
        kReferenceMiniGameTriangleOutlineWidth * umg_unit_scale_;
    const LinearColor fill = height_pointer_fill_color(kind);
    const double apex_y = center + (shape
            == dswros::MiniGameHeightIndicatorShape::Above
        ? -half_height : half_height);
    const double base_y = center + (shape
            == dswros::MiniGameHeightIndicatorShape::Above
        ? half_height : -half_height);
    const std::array<LineSegment, 3> outline{{
        {{center - half_width, base_y}, {center, apex_y}},
        {{center, apex_y}, {center + half_width, base_y}},
        {{center + half_width, base_y}, {center - half_width, base_y}},
    }};
    for (std::size_t edge = 0; edge < outline.size(); ++edge) {
        const LineSegment& segment = outline[edge];
        set_line_geometry(
            slots[edge], set_slot_position_, set_slot_size_,
            segment.start.x, segment.start.y,
            segment.end.x, segment.end.y, outline_width);
        set_render_angle(
            pieces[edge], set_render_angle_,
            std::atan2(
                segment.end.y - segment.start.y,
                segment.end.x - segment.start.x) * kRadiansToDegrees);
        set_brush_color(pieces[edge], set_brush_color_, kOutline);
        set_visibility(pieces[edge], set_visibility_, kVisible);
    }
    constexpr std::size_t kFillPieceOffset = 3U;
    constexpr std::size_t kFillPieceCount =
        kCompactUmgHeightPieceCount - kFillPieceOffset;
    for (std::size_t fill_piece = 0; fill_piece < kFillPieceCount;
         ++fill_piece) {
        const std::size_t piece = kFillPieceOffset + fill_piece;
        const double vertical_fraction =
            (static_cast<double>(fill_piece) + 1.0)
            / static_cast<double>(kFillPieceCount + 1U);
        const double bar_width = std::max(
            fill_bar_height * 0.75,
            half_width * 2.0 * vertical_fraction
                - outline_width * 1.15);
        const double bar_y = apex_y
            + (base_y - apex_y) * vertical_fraction;
        set_slot_vector(
            slots[piece], set_slot_position_, center, bar_y);
        set_slot_vector(
            slots[piece], set_slot_size_, bar_width, fill_bar_height);
        set_render_angle(pieces[piece], set_render_angle_, 0.0);
        set_brush_color(pieces[piece], set_brush_color_, fill);
        set_visibility(pieces[piece], set_visibility_, kVisible);
    }

    VectorParameters translation{{
        0.0,
        height_marker_half_widths_[channel]
            + kReferenceMiniGameTriangleClearance * umg_unit_scale_
            + half_height}};
    height_group->ProcessEvent(set_render_translation_, &translation);
    set_render_angle(height_group, set_render_angle_, 0.0);
    mini_game_height_shape_codes_[channel] = shape_code;
    height_kind_codes_[channel] = kind_code;
    height_angle_degrees_[channel] = height_angle_degrees;
    height_translation_x_[channel] = 0.0;
    height_transform_valid_[channel] = true;
    ++height_transform_count_;
    return true;
}

bool CompactUmgRenderer::configure_area_quest_marker_shape_unsafe(
    std::size_t marker_index,
    std::uint8_t shape_code,
    double center_x,
    double center_y,
    double scaled_size) {
    if (shape_code > static_cast<std::uint8_t>(
            dswros::AreaQuestHeightIndicatorShape::Unavailable)
        || marker_index >= kCompactUmgMarkerCapacity
        || !std::isfinite(center_x) || !std::isfinite(center_y)
        || !std::isfinite(scaled_size) || scaled_size <= 0.0) {
        return false;
    }
    const auto shape = static_cast<
        dswros::AreaQuestHeightIndicatorShape>(shape_code);
    const auto kind = static_cast<CompactUmgMarkerKind>(
        marker_kind_codes_[marker_index]);
    const bool encounter_marker = dswros::is_encounter_height_marker(kind);
    if (kind != CompactUmgMarkerKind::AreaQuest && !encounter_marker) {
        return false;
    }
    if (!brush_templates_ready_ || !set_brush_
        || !set_brush_color_ || !set_visibility_
        || !set_slot_position_ || !set_slot_size_ || !set_render_angle_) {
        return false;
    }
    std::array<UObject*, kCompactUmgMarkerPieceCount> pieces{};
    std::array<UObject*, kCompactUmgMarkerPieceCount> slots{};
    for (std::size_t piece = 0;
         piece < kCompactUmgMarkerPieceCount; ++piece) {
        pieces[piece] = marker_pieces_[marker_index][piece].Get();
        slots[piece] = marker_piece_slots_[marker_index][piece].Get();
        if (!pieces[piece] || !slots[piece]) {
            return false;
        }
    }

    if (shape == dswros::AreaQuestHeightIndicatorShape::Aligned
        || shape == dswros::AreaQuestHeightIndicatorShape::Unavailable) {
        const bool show_alignment_dots = shape
            == dswros::AreaQuestHeightIndicatorShape::Aligned;
        for (std::size_t piece = 0;
             piece < kCompactUmgMarkerPieceCount; ++piece) {
            const MarkerPieceStyle style = marker_piece_style(
                kind, piece, marker_reference_sizes_[marker_index]);
            set_marker_piece_geometry(
                slots[piece], set_slot_position_, set_slot_size_, style,
                center_x, center_y, scaled_size);
            set_brush(
                pieces[piece], set_brush_, !encounter_marker && piece == 0U
                    ? area_quest_brush_template_ : solid_brush_template_);
            set_brush_color(
                pieces[piece], set_brush_color_, style.color);
            set_render_angle(
                pieces[piece], set_render_angle_, style.angle_degrees);
            set_visibility(
                pieces[piece], set_visibility_,
                encounter_marker || piece == 0U || show_alignment_dots
                    ? kVisible : kCollapsed);
        }
        return true;
    }

    const bool above = shape
        == dswros::AreaQuestHeightIndicatorShape::Above;
    const double half_width = scaled_size
        * kAreaQuestMarkerTriangleHalfWidth;
    const double half_height = scaled_size
        * kAreaQuestMarkerTriangleHalfHeight;
    // Reuse the marker itself. Directional states replace the normal glyph in
    // place; no separate Treasure-style pointer or additional widget is needed.
    const double apex_y = center_y + (above ? -half_height : half_height);
    const double base_y = center_y + (above ? half_height : -half_height);
    const std::array<LineSegment, 3> triangle{{
        {{center_x - half_width, base_y},
         {center_x, apex_y}},
        {{center_x, apex_y},
         {center_x + half_width, base_y}},
        {{center_x + half_width, base_y},
         {center_x - half_width, base_y}},
    }};
    // Slate's dark outline is inside the edge widget. Reserve its two sides
    // explicitly so the Boss/Assault category-colored core stays as thick as
    // the Area Quest stroke, regardless of the marker's reference size.
    const double stroke = (kBandMarkerReferenceStroke
        + (encounter_marker ? 2.0 * kEncounterTriangleReferenceOutline : 0.0))
        * umg_unit_scale_;
    for (std::size_t edge = 0; edge < triangle.size(); ++edge) {
        const LineSegment& segment = triangle[edge];
        set_brush(pieces[edge], set_brush_, encounter_marker
            ? encounter_triangle_brush_template_ : solid_brush_template_);
        set_brush_color(pieces[edge], set_brush_color_, encounter_marker
            ? (kind == CompactUmgMarkerKind::Boss ? kOfficialWhite : kOfficialCyan)
            : kOutline);
        set_line_geometry(
            slots[edge], set_slot_position_, set_slot_size_,
            segment.start.x, segment.start.y,
            segment.end.x, segment.end.y, stroke);
        set_render_angle(
            pieces[edge], set_render_angle_,
            std::atan2(
                segment.end.y - segment.start.y,
                segment.end.x - segment.start.x)
                * kRadiansToDegrees);
        set_visibility(pieces[edge], set_visibility_, kVisible);
    }
    set_visibility(pieces[3], set_visibility_, kCollapsed);
    return true;
}

void CompactUmgRenderer::detach() noexcept {
    activation_active_ = false;
    detach_guarded();
}

void CompactUmgRenderer::abandon_runtime_handles() noexcept {
    activation_active_ = false;
    reset_runtime_handles();
    if (state_ == CompactUmgRendererState::Attached
        || state_ == CompactUmgRendererState::Suppressed) {
        state_ = CompactUmgRendererState::Ready;
    }
}

void CompactUmgRenderer::detach_guarded() noexcept {
#if defined(_MSC_VER)
    __try {
        detach_unsafe();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        reset_runtime_handles();
    }
#else
    try {
        detach_unsafe();
    } catch (...) {
        ++fault_count_;
        state_ = CompactUmgRendererState::Faulted;
        activation_active_ = false;
        reset_runtime_handles();
    }
#endif
}

void CompactUmgRenderer::detach_unsafe() {
    bool removed{};
    if (UObject* host = host_.Get(); host && remove_from_parent_) {
        host->ProcessEvent(remove_from_parent_, nullptr);
        removed = true;
    }
    if (removed) {
        ++detach_count_;
    }
    reset_runtime_handles();
    if (state_ == CompactUmgRendererState::Attached
        || state_ == CompactUmgRendererState::Suppressed) {
        state_ = CompactUmgRendererState::Ready;
    }
}

void CompactUmgRenderer::reset_runtime_handles() noexcept {
    minimap_layer_ = FWeakObjectPtr{};
    clock_main_panel_ = FWeakObjectPtr{};
    clock_geometry_faulted_ = false;
    clock_geometry_defer_ = false;
    clock_gap_layout_active_ = false;
    clock_position_x_ = 0.0;
    clock_position_y_ = 0.0;
    host_ = FWeakObjectPtr{};
    widget_tree_ = FWeakObjectPtr{};
    root_panel_ = FWeakObjectPtr{};
    host_root_panel_ = FWeakObjectPtr{};
    height_groups_.fill(FWeakObjectPtr{});
    height_group_slots_.fill(FWeakObjectPtr{});
    clock_group_ = FWeakObjectPtr{};
    clock_group_slot_ = FWeakObjectPtr{};
    for (auto& marker : marker_pieces_) {
        for (auto& piece : marker) {
            piece = FWeakObjectPtr{};
        }
    }
    for (auto& marker : marker_piece_slots_) {
        for (auto& slot : marker) {
            slot = FWeakObjectPtr{};
        }
    }
    for (auto& channel : height_pieces_) {
        channel.fill(FWeakObjectPtr{});
    }
    for (auto& channel : height_piece_slots_) {
        channel.fill(FWeakObjectPtr{});
    }
    for (auto& digit : clock_digit_pieces_) {
        for (auto& piece : digit) {
            piece = FWeakObjectPtr{};
        }
    }
    for (auto& digit : clock_digit_piece_slots_) {
        for (auto& slot : digit) {
            slot = FWeakObjectPtr{};
        }
    }
    for (auto& piece : clock_colon_pieces_) {
        piece = FWeakObjectPtr{};
    }
    for (auto& slot : clock_colon_piece_slots_) {
        slot = FWeakObjectPtr{};
    }
    for (auto& piece : clock_phase_pieces_) {
        piece = FWeakObjectPtr{};
    }
    for (auto& slot : clock_phase_piece_slots_) {
        slot = FWeakObjectPtr{};
    }
    marker_visible_.fill(false);
    marker_reference_sizes_.fill(0.0);
    marker_kind_codes_.fill(0xFFU);
    area_quest_marker_shape_codes_.fill(0xFFU);
    area_quest_marker_center_x_.fill(0.0);
    area_quest_marker_center_y_.fill(0.0);
    area_quest_marker_scaled_size_.fill(0.0);
    area_quest_height_profiles_.fill(dswros::AreaQuestHeightProfile{});
    area_quest_height_active_.fill(false);
    height_visible_.fill(false);
    height_transform_valid_.fill(false);
    height_kind_codes_.fill(0xFFU);
    mini_game_height_shape_codes_.fill(0xFFU);
    height_marker_half_widths_.fill(0.0);
    height_angle_degrees_.fill(0.0);
    height_translation_x_.fill(0.0);
    clock_visible_ = false;
    clock_minute_valid_ = false;
    clock_phase_valid_ = false;
    clock_minute_ = 0;
    clock_phase_ = 0xFFU;
    for (auto& digit : clock_segment_visible_) {
        digit.fill(false);
    }
    render_transform_property_ = nullptr;
    scale_property_ = nullptr;
    scale_x_property_ = nullptr;
    viewport_width_ = 0.0;
    viewport_height_ = 0.0;
    host_origin_x_ = 0.0;
    host_origin_y_ = 0.0;
    viewport_dpi_scale_ = 1.0;
    display_scale_ = 1.0;
    umg_unit_scale_ = 1.0;
    host_render_scale_ = 1.0;
    menu_suppressed_ = false;
    active_marker_count_ = 0;
}

UObject* CompactUmgRenderer::read_object_property(
    UObject* object, const wchar_t* property_name) {
    if (!object) {
        return nullptr;
    }
    auto* property = CastField<FObjectPropertyBase>(
        object->GetPropertyByNameInChain(property_name));
    void* value = object->GetValuePtrByPropertyNameInChain(property_name);
    return property && value
        ? property->GetObjectPropertyValue(value)
        : nullptr;
}


} // namespace dsnwr
