#include "world_map_umg_renderer.hpp"

#include <dswros/render_projection.hpp>

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
#include "ue4ss_compat.hpp"
#pragma warning(pop)

#include <algorithm>
#include <bit>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <system_error>
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

struct AddChildParameters {
    UObject* content{};
    UObject* return_value{};
};

struct VectorParameters {
    Vector2D value{};
};

struct ZOrderParameters {
    std::int32_t value{};
};

struct AddToViewportParameters {
    std::int32_t z_order{};
};

struct PositionInViewportParameters {
    Vector2D position{};
    bool remove_dpi_scale{};
    std::array<std::byte, 7> padding{};
};

struct VisibilityParameters {
    std::uint8_t visibility{};
};

struct GetPositionParameters {
    Vector2D return_value{};
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
static_assert(sizeof(AddChildParameters) == 16);
static_assert(sizeof(VectorParameters) == 16);
static_assert(sizeof(ZOrderParameters) == 4);
static_assert(sizeof(AddToViewportParameters) == 4);
static_assert(sizeof(PositionInViewportParameters) == 24);
static_assert(sizeof(VisibilityParameters) == 1);
static_assert(sizeof(GetPositionParameters) == 16);
static_assert(sizeof(SetBrushFromTextureParameters) == 16);
static_assert(sizeof(FString) == 16);
static_assert(sizeof(ImportFileParameters) == 32);

constexpr std::uint8_t kCollapsed = 1;
constexpr std::uint8_t kHitTestInvisible = 3;
constexpr double kTreasureMarkerSize = 12.0;
constexpr double kBossMarkerSize = 44.0;
constexpr double kAssaultMarkerSize = 34.0;
// Match the accepted external renderer's expanded-map mini-game diameter.
// Compact-map sizing remains independently owned by compact_umg_renderer.cpp.
constexpr double kMiniGameMarkerSize = 32.0;
constexpr double kAreaQuestMarkerSize = 26.0;
// Game-native map icons can also occupy the maximum Canvas Z. Both radar
// hosts therefore use that same maximum and are inserted after the native
// children; background-then-foreground insertion preserves internal order.
constexpr std::int32_t kRadarMarkerZ =
    std::numeric_limits<std::int32_t>::max();
constexpr std::int32_t kMaxNativeIconCandidates = 4096;
constexpr std::size_t kGeometryParameterCapacity = 256;
constexpr std::uint32_t kTransparent = 0x00000000U;
constexpr std::uint32_t kOutline = 0xFF162432U;
constexpr std::uint32_t kWhite = 0xFFFFFFFFU;
constexpr std::uint32_t kGreen = 0xFF3FE67AU;
constexpr std::uint32_t kOrange = 0xFFFFA22FU;
constexpr std::uint32_t kBlue = 0xFF439FFFU;
constexpr std::uint32_t kShadow = 0x52050B12U;
constexpr std::uint32_t kOfficialWhite = 0xFFF7FFFDU;
constexpr std::uint32_t kOfficialPale = 0xFFD8F6EEU;
constexpr std::uint32_t kOfficialGreen = 0xFF4D9F8BU;
constexpr std::uint32_t kOfficialGreenDark = 0xFF153C3EU;
constexpr std::uint32_t kOfficialCyan = 0xFF55EDE4U;
constexpr std::uint32_t kFlyWing = 0xFF9ADBFFU;
constexpr std::uint32_t kFlyArrow = 0xFF69C7FFU;
constexpr std::uint32_t kHammer = 0xFFD29152U;
constexpr std::uint32_t kHammerHandle = 0xFF8A5934U;
constexpr std::uint32_t kHammerHighlight = 0xFFF0B56FU;
constexpr std::uint32_t kWave = 0xFF325BE0U;
constexpr std::uint32_t kAreaQuestBubble = 0xFFEFEDE7U;
constexpr std::uint32_t kAreaQuestDark = 0xFF232A2EU;
constexpr std::uint64_t kAtlasStyleRevision = 50U;

enum class AtlasLayer : std::uint8_t {
    Background,
    Foreground,
};

[[nodiscard]] bool marker_belongs_to_layer(
    WorldMapUmgMarkerKind kind, AtlasLayer layer) noexcept {
    const bool encounter = kind == WorldMapUmgMarkerKind::Boss
        || kind == WorldMapUmgMarkerKind::Assault;
    return layer == AtlasLayer::Foreground ? encounter : !encounter;
}

// StaticFindObject accepts canonical object paths, not export-text Class'...' paths.
constexpr wchar_t kWorldMapData100[]{
    L"/Game/Design/CaptureMinimap/Data/WorldMapData_100.WorldMapData_100"};
constexpr wchar_t kWorldMapData200[]{
    L"/Game/Design/CaptureMinimap/Data/WorldMapData_200.WorldMapData_200"};

template <typename T>
T* find(const wchar_t* path) {
    return UObjectGlobals::StaticFindObject<T*>(nullptr, nullptr, path);
}

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

[[nodiscard]] UObject* read_object_property(
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

[[nodiscard]] UObject* read_struct_object_property(
    UObject* object, const wchar_t* struct_name,
    const wchar_t* object_name) {
    auto* structure = CastField<FStructProperty>(
        object ? object->GetPropertyByNameInChain(struct_name) : nullptr);
    void* structure_value = structure
        ? structure->ContainerPtrToValuePtr<void>(object)
        : nullptr;
    auto* object_property = CastField<FObjectPropertyBase>(
        structure ? find_struct_field(structure->GetStruct(), object_name)
                  : nullptr);
    void* object_value = object_property && structure_value
        ? object_property->ContainerPtrToValuePtr<void>(structure_value)
        : nullptr;
    return object_value
        ? object_property->GetObjectPropertyValue(object_value)
        : nullptr;
}

struct NativeIconTemplate {
    UClass* icon_class{};
    UObject* parent_canvas{};
};

enum class NativeIconTemplateLookupResult : std::uint32_t {
    Found,
    NotReady,
    InvalidSchema,
};

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

struct GeometryReflectionSchema {
    FStructProperty* cached_geometry_return{};
    FStructProperty* alignment_return{};
    FStructProperty* local_size_geometry{};
    FStructProperty* local_size_return{};
    FStructProperty* local_to_absolute_geometry{};
    FStructProperty* local_coordinate{};
    FStructProperty* local_to_absolute_return{};
    FStructProperty* absolute_to_local_geometry{};
    FStructProperty* absolute_coordinate{};
    FStructProperty* absolute_to_local_return{};
};

struct ViewportGeometryReflectionSchema {
    FObjectPropertyBase* world_context_object{};
    FStructProperty* return_value{};
};

[[nodiscard]] bool resolve_geometry_reflection_schema(
    UFunction* get_cached_geometry,
    UFunction* get_slot_alignment,
    UFunction* get_geometry_local_size,
    UFunction* local_to_absolute,
    UFunction* absolute_to_local,
    GeometryReflectionSchema& schema) {
    schema = {};
    schema.cached_geometry_return = CastField<FStructProperty>(
        find_function_field(get_cached_geometry, L"ReturnValue"));
    schema.alignment_return = CastField<FStructProperty>(
        find_function_field(get_slot_alignment, L"ReturnValue"));
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
        function_property_fits(
            get_cached_geometry, schema.cached_geometry_return)
        && function_property_fits(
            get_slot_alignment, schema.alignment_return)
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
    UScriptStruct* vector = schema.alignment_return->GetStruct();
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
        && vector_struct_is_finite_schema(schema.alignment_return);
}

[[nodiscard]] bool resolve_viewport_geometry_reflection_schema(
    UFunction* get_viewport_widget_geometry,
    FStructProperty* expected_geometry,
    ViewportGeometryReflectionSchema& schema) {
    schema = {};
    schema.world_context_object = CastField<FObjectPropertyBase>(
        find_function_field(
            get_viewport_widget_geometry, L"WorldContextObject"));
    schema.return_value = CastField<FStructProperty>(
        find_function_field(get_viewport_widget_geometry, L"ReturnValue"));
    return function_property_fits(
               get_viewport_widget_geometry, schema.world_context_object)
        && function_property_fits(
               get_viewport_widget_geometry, schema.return_value)
        && expected_geometry && schema.return_value
        && schema.return_value->GetStruct() == expected_geometry->GetStruct();
}

// Reads only the supplied layer's current icon array once. The selected widget
// must still own its reflected CanvasPanelSlot, belong to the current player,
// and derive from the native map-point widget type. No candidate survives this
// call as a raw pointer.
[[nodiscard]] NativeIconTemplateLookupResult find_native_icon_template(
    UObject* layer,
    UObject* expected_owning_player,
    UClass* map_point_icon_class,
    UClass* canvas_panel_class,
    UClass* canvas_panel_slot_class,
    UFunction* get_owning_player,
    NativeIconTemplate& result) {
    result = {};
    auto* array_property = CastField<FArrayProperty>(
        layer ? layer->GetPropertyByNameInChain(L"ArrayIconInfo") : nullptr);
    void* array_value = array_property
        ? array_property->ContainerPtrToValuePtr<void>(layer)
        : nullptr;
    auto* info_property = CastField<FStructProperty>(
        array_property ? array_property->GetInner() : nullptr);
    auto* icon_property = CastField<FObjectPropertyBase>(
        info_property
            ? find_struct_field(info_property->GetStruct(), L"IconWidget")
            : nullptr);
    if (!array_property || !array_value || !info_property || !icon_property) {
        return NativeIconTemplateLookupResult::InvalidSchema;
    }

    FScriptArrayHelper icons(array_property, array_value);
    const std::int32_t count = icons.Num();
    if (count < 0) {
        return NativeIconTemplateLookupResult::InvalidSchema;
    }
    if (count == 0) {
        return NativeIconTemplateLookupResult::NotReady;
    }
    if (count > kMaxNativeIconCandidates) {
        return NativeIconTemplateLookupResult::InvalidSchema;
    }
    for (std::int32_t index = 0; index < count; ++index) {
        void* info_value = icons.GetRawPtr(index);
        void* icon_value = info_value
            ? icon_property->ContainerPtrToValuePtr<void>(info_value)
            : nullptr;
        UObject* icon = icon_value
            ? icon_property->GetObjectPropertyValue(icon_value)
            : nullptr;
        if (!icon || !icon->IsA(map_point_icon_class)) {
            continue;
        }

        auto* point_panel_property = CastField<FObjectPropertyBase>(
            icon->GetPropertyByNameInChain(L"Panel_Point"));
        auto* slot_property = CastField<FObjectPropertyBase>(
            icon->GetPropertyByNameInChain(L"Slot"));
        if (!point_panel_property || !slot_property) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }
        void* point_panel_value =
            point_panel_property->ContainerPtrToValuePtr<void>(icon);
        void* slot_value = slot_property->ContainerPtrToValuePtr<void>(icon);
        if (!point_panel_value || !slot_value) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }
        UObject* point_panel =
            point_panel_property->GetObjectPropertyValue(point_panel_value);
        UObject* slot = slot_property->GetObjectPropertyValue(slot_value);
        if (!point_panel || !slot) {
            continue;
        }
        if (!point_panel->IsA(canvas_panel_class)
            || !slot->IsA(canvas_panel_slot_class)) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }

        auto* parent_property = CastField<FObjectPropertyBase>(
            slot->GetPropertyByNameInChain(L"Parent"));
        auto* content_property = CastField<FObjectPropertyBase>(
            slot->GetPropertyByNameInChain(L"Content"));
        if (!parent_property || !content_property) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }
        void* parent_value =
            parent_property->ContainerPtrToValuePtr<void>(slot);
        void* content_value =
            content_property->ContainerPtrToValuePtr<void>(slot);
        if (!parent_value || !content_value) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }
        UObject* parent = parent_property->GetObjectPropertyValue(parent_value);
        UObject* content = content_property->GetObjectPropertyValue(content_value);
        if (!parent || !content) {
            continue;
        }
        if (!parent->IsA(canvas_panel_class) || content != icon) {
            return NativeIconTemplateLookupResult::InvalidSchema;
        }

        ObjectReturnParameters owning_player{};
        icon->ProcessEvent(get_owning_player, &owning_player);
        if (owning_player.return_value != expected_owning_player) {
            continue;
        }

        UClass* icon_class = icon->GetClassPrivate();
        if (!icon_class || !icon_class->IsChildOf(map_point_icon_class)) {
            continue;
        }
        result = {icon_class, parent};
        return NativeIconTemplateLookupResult::Found;
    }
    return NativeIconTemplateLookupResult::NotReady;
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

[[nodiscard]] bool read_nested_vector(
    UObject* object, const wchar_t* outer_struct_name,
    const wchar_t* vector_name, double& x, double& y) {
    auto* outer = CastField<FStructProperty>(
        object ? object->GetPropertyByNameInChain(outer_struct_name) : nullptr);
    void* outer_value = outer
        ? outer->ContainerPtrToValuePtr<void>(object)
        : nullptr;
    auto* vector = CastField<FStructProperty>(
        outer ? find_struct_field(outer->GetStruct(), vector_name) : nullptr);
    void* vector_value = vector && outer_value
        ? vector->ContainerPtrToValuePtr<void>(outer_value)
        : nullptr;
    if (!vector || !vector_value) {
        return false;
    }
    return read_numeric_value(
               find_struct_field(vector->GetStruct(), L"X"), vector_value, x)
        && read_numeric_value(
               find_struct_field(vector->GetStruct(), L"Y"), vector_value, y);
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

[[nodiscard]] bool write_object_property(
    FObjectPropertyBase* property,
    void* container,
    UObject* value) {
    void* address = property && container
        ? property->ContainerPtrToValuePtr<void>(container)
        : nullptr;
    if (!property || !address || !value) {
        return false;
    }
    property->SetObjectPropertyValue(address, value);
    return property->GetObjectPropertyValue(address) == value;
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

[[nodiscard]] bool read_slate_geometry_snapshot(
    UObject* slate_library,
    UFunction* get_geometry_local_size,
    UFunction* local_to_absolute,
    const GeometryReflectionSchema& schema,
    FStructProperty* source_geometry_property,
    void* source_geometry_container,
    dswros::WorldMapSlateGeometry& snapshot) {
    snapshot = {};
    double width{};
    double height{};
    double absolute_left{};
    double absolute_top{};
    double absolute_right{};
    double absolute_bottom{};
    if (!read_geometry_local_size(
            slate_library, get_geometry_local_size, schema,
            source_geometry_property, source_geometry_container,
            width, height)
        || !transform_geometry_point(
            slate_library, local_to_absolute,
            schema.local_to_absolute_geometry, schema.local_coordinate,
            schema.local_to_absolute_return,
            source_geometry_property, source_geometry_container,
            0.0, 0.0, absolute_left, absolute_top)
        || !transform_geometry_point(
            slate_library, local_to_absolute,
            schema.local_to_absolute_geometry, schema.local_coordinate,
            schema.local_to_absolute_return,
            source_geometry_property, source_geometry_container,
            width, height, absolute_right, absolute_bottom)) {
        return false;
    }
    const double scale_x = (absolute_right - absolute_left) / width;
    const double scale_y = (absolute_bottom - absolute_top) / height;
    if (!std::isfinite(absolute_left) || !std::isfinite(absolute_top)
        || !std::isfinite(scale_x) || !std::isfinite(scale_y)
        || width <= 0.0 || height <= 0.0
        || scale_x <= 0.0 || scale_y <= 0.0) {
        return false;
    }
    snapshot = {
        absolute_left, absolute_top, width, height, scale_x, scale_y};
    return true;
}

// Converts the player icon's exact Canvas alignment pivot into the selected
// native icon Canvas local space through the widgets' completed Slate
// geometries. Because both transforms traverse the live hierarchy, anchors,
// alignment, Safe Zone, window DPI, letterboxing, pan, zoom, and render
// translation are applied exactly once. Cached geometry is read only during an
// attach/layout event and is rejected while it is absent or stale.
[[nodiscard]] bool read_live_player_canvas_anchor(
    UObject* player_icon,
    UObject* expected_native_parent,
    UClass* canvas_panel_class,
    UClass* canvas_panel_slot_class,
    UObject* slate_library,
    UFunction* get_slot_alignment,
    UFunction* get_cached_geometry,
    UFunction* get_geometry_local_size,
    UFunction* local_to_absolute,
    UFunction* absolute_to_local,
    double map_ui_size,
    double& anchor_x,
    double& anchor_y,
    double& parent_width,
    double& parent_height) {
    UObject* slot = read_object_property(player_icon, L"Slot");
    UObject* player_parent = read_object_property(slot, L"Parent");
    if (!slot || !slot->IsA(canvas_panel_slot_class)
        || !player_parent || !player_parent->IsA(canvas_panel_class)
        || read_object_property(slot, L"Content") != player_icon) {
        return false;
    }

    GeometryReflectionSchema schema{};
    if (!slate_library || !resolve_geometry_reflection_schema(
            get_cached_geometry, get_slot_alignment,
            get_geometry_local_size, local_to_absolute,
            absolute_to_local, schema)) {
        return false;
    }

    GeometryCallParameters alignment_parameters(get_slot_alignment);
    if (!alignment_parameters.valid()) {
        return false;
    }
    slot->ProcessEvent(
        get_slot_alignment, alignment_parameters.data());
    double alignment_x{};
    double alignment_y{};
    if (!read_vector_property(
            schema.alignment_return, alignment_parameters.data(),
            alignment_x, alignment_y)
        || alignment_x < -1.0 || alignment_x > 2.0
        || alignment_y < -1.0 || alignment_y > 2.0) {
        return false;
    }

    GeometryCallParameters player_geometry(get_cached_geometry);
    GeometryCallParameters parent_geometry(get_cached_geometry);
    if (!player_geometry.valid() || !parent_geometry.valid()) {
        return false;
    }
    player_icon->ProcessEvent(
        get_cached_geometry, player_geometry.data());
    expected_native_parent->ProcessEvent(
        get_cached_geometry, parent_geometry.data());

    double player_width{};
    double player_height{};
    if (!read_geometry_local_size(
            slate_library, get_geometry_local_size, schema,
            schema.cached_geometry_return, player_geometry.data(),
            player_width, player_height)
        || !read_geometry_local_size(
            slate_library, get_geometry_local_size, schema,
            schema.cached_geometry_return, parent_geometry.data(),
            parent_width, parent_height)) {
        return false;
    }

    double absolute_x{};
    double absolute_y{};
    if (!transform_geometry_point(
            slate_library, local_to_absolute,
            schema.local_to_absolute_geometry, schema.local_coordinate,
            schema.local_to_absolute_return,
            schema.cached_geometry_return, player_geometry.data(),
            player_width * alignment_x, player_height * alignment_y,
            absolute_x, absolute_y)
        || !transform_geometry_point(
            slate_library, absolute_to_local,
            schema.absolute_to_local_geometry, schema.absolute_coordinate,
            schema.absolute_to_local_return,
            schema.cached_geometry_return, parent_geometry.data(),
            absolute_x, absolute_y, anchor_x, anchor_y)) {
        return false;
    }

    const auto validated = dswros::validate_world_map_canvas_anchor(
        anchor_x, anchor_y, parent_width, parent_height, map_ui_size);
    if (!validated) {
        return false;
    }
    anchor_x = validated->x;
    anchor_y = validated->y;
    return true;
}

[[nodiscard]] bool read_map_data(
    UObject* object, std::int32_t& map_id,
    double& dimensions, double& ui_size) {
    auto* info = CastField<FStructProperty>(
        object ? object->GetPropertyByNameInChain(L"WorldMapDataInfo") : nullptr);
    void* info_value = info
        ? info->ContainerPtrToValuePtr<void>(object)
        : nullptr;
    if (!info || !info_value) {
        return false;
    }
    double id_value{};
    if (!read_numeric_value(
            find_struct_field(info->GetStruct(), L"MapID"), info_value, id_value)
        || !read_numeric_value(
            find_struct_field(info->GetStruct(), L"MapDimensions"),
            info_value, dimensions)
        || !read_numeric_value(
            find_struct_field(info->GetStruct(), L"WorldMapUISize"),
            info_value, ui_size)
        || std::trunc(id_value) != id_value
        || id_value < static_cast<double>(
            std::numeric_limits<std::int32_t>::min())
        || id_value > static_cast<double>(
            std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    map_id = static_cast<std::int32_t>(id_value);
    return dimensions > 0.0 && dimensions < 10'000'000.0
        && ui_size > 0.0 && ui_size < 1'000'000.0;
}

[[nodiscard]] bool valid_cached_map_data(
    std::int32_t cached_map_id,
    double cached_dimensions,
    double cached_ui_size,
    std::int32_t detected_map_id) {
    return cached_map_id == detected_map_id
        && (detected_map_id == 100 || detected_map_id == 200)
        && std::isfinite(cached_dimensions)
        && cached_dimensions > 0.0
        && cached_dimensions < 10'000'000.0
        && std::isfinite(cached_ui_size)
        && cached_ui_size > 0.0
        && cached_ui_size < 1'000'000.0;
}

[[nodiscard]] std::int32_t detect_map_id(UObject* layer) {
    UObject* map_image = read_object_property(layer, L"Map_0_0");
    UObject* resource = read_struct_object_property(
        map_image, L"Brush", L"ResourceObject");
    if (!resource) {
        return 0;
    }
    const auto full_name = resource->GetFullName();
    if (full_name.find(STR("_W2_")) != full_name.npos
        || full_name.find(STR("world_02")) != full_name.npos) {
        return 200;
    }
    if (full_name.find(STR("_W1_")) != full_name.npos
        || full_name.find(STR("world_01")) != full_name.npos) {
        return 100;
    }
    return 0;
}

struct AtlasBounds {
    double left{};
    double top{};
    double width{};
    double height{};
};

struct AtlasBuildResult {
    bool success{};
    std::size_t drawn_marker_count{};
    std::uint64_t elapsed_us{};
    std::uint64_t file_bytes{};
};

struct AtlasFileCache {
    std::uint64_t path_hash{};
    std::uint64_t input_fingerprint{};
    std::uint64_t file_bytes{};
    std::size_t visible_marker_count{};
    bool valid{};
};

[[nodiscard]] std::array<AtlasFileCache, kWorldMapAtlasLayerCount>&
atlas_file_caches() noexcept {
    static std::array<AtlasFileCache, kWorldMapAtlasLayerCount> caches{};
    return caches;
}

void hash_u64(std::uint64_t& hash, std::uint64_t value) noexcept {
    constexpr std::uint64_t prime = 1099511628211ULL;
    for (std::uint32_t shift = 0; shift < 64U; shift += 8U) {
        hash ^= (value >> shift) & 0xFFU;
        hash *= prime;
    }
}

[[nodiscard]] std::uint64_t atlas_input_fingerprint(
    const WorldMapUmgMarkerArray& markers,
    const std::array<Vector2D, kWorldMapUmgMarkerCapacity>& local_positions,
    std::size_t marker_count,
    const AtlasBounds& bounds,
    AtlasLayer layer,
    std::size_t& visible_marker_count) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    hash_u64(hash, kAtlasStyleRevision);
    hash_u64(hash, marker_count);
    hash_u64(hash, kWorldMapAtlasTextureSize);
    hash_u64(hash, static_cast<std::uint8_t>(layer));
    hash_u64(hash, std::bit_cast<std::uint64_t>(bounds.left));
    hash_u64(hash, std::bit_cast<std::uint64_t>(bounds.top));
    hash_u64(hash, std::bit_cast<std::uint64_t>(bounds.width));
    hash_u64(hash, std::bit_cast<std::uint64_t>(bounds.height));
    visible_marker_count = 0;
    for (std::size_t index = 0; index < marker_count; ++index) {
        const WorldMapUmgMarker& marker = markers[index];
        if (!marker.visible
            || !marker_belongs_to_layer(marker.kind, layer)) {
            continue;
        }
        hash_u64(hash, static_cast<std::uint64_t>(marker.id));
        hash_u64(hash, std::bit_cast<std::uint64_t>(local_positions[index].x));
        hash_u64(hash, std::bit_cast<std::uint64_t>(local_positions[index].y));
        hash_u64(hash, static_cast<std::uint8_t>(marker.tone));
        hash_u64(hash, static_cast<std::uint8_t>(marker.kind));
        ++visible_marker_count;
    }
    return hash;
}

[[nodiscard]] double marker_reference_size(
    WorldMapUmgMarkerKind kind) noexcept {
    switch (kind) {
    case WorldMapUmgMarkerKind::Boss: return kBossMarkerSize + 2.0;
    case WorldMapUmgMarkerKind::Assault: return kAssaultMarkerSize + 2.0;
    case WorldMapUmgMarkerKind::Fly:
    case WorldMapUmgMarkerKind::Mole:
    case WorldMapUmgMarkerKind::Wave:
        // The accepted wave silhouette intentionally extends slightly beyond
        // its nominal diameter. Preserve enough atlas padding at map edges.
        return kMiniGameMarkerSize + 8.0;
    case WorldMapUmgMarkerKind::AreaQuest:
        return kAreaQuestMarkerSize + 2.0;
    default: return kTreasureMarkerSize;
    }
}

[[nodiscard]] std::uint32_t marker_fill(
    WorldMapUmgMarkerTone tone) noexcept {
    switch (tone) {
    case WorldMapUmgMarkerTone::Green: return kGreen;
    case WorldMapUmgMarkerTone::Orange: return kOrange;
    case WorldMapUmgMarkerTone::Blue: return kBlue;
    default: return kWhite;
    }
}

[[nodiscard]] bool calculate_atlas_bounds(
    const WorldMapUmgMarkerArray& markers,
    const std::array<Vector2D, kWorldMapUmgMarkerCapacity>& local_positions,
    std::size_t marker_count,
    AtlasBounds& bounds,
    std::size_t& visible_count) noexcept {
    visible_count = 0;
    double minimum_x = std::numeric_limits<double>::infinity();
    double maximum_x = -std::numeric_limits<double>::infinity();
    double minimum_y = std::numeric_limits<double>::infinity();
    double maximum_y = -std::numeric_limits<double>::infinity();
    for (std::size_t index = 0; index < marker_count; ++index) {
        if (!markers[index].visible) {
            continue;
        }
        const Vector2D position = local_positions[index];
        if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
            return false;
        }
        const double half_marker =
            marker_reference_size(markers[index].kind) * 0.5;
        minimum_x = std::min(minimum_x, position.x - half_marker);
        maximum_x = std::max(maximum_x, position.x + half_marker);
        minimum_y = std::min(minimum_y, position.y - half_marker);
        maximum_y = std::max(maximum_y, position.y + half_marker);
        ++visible_count;
    }
    if (visible_count == 0) {
        bounds = {
            0.0, 0.0, kTreasureMarkerSize, kTreasureMarkerSize};
        return true;
    }

    bounds.left = minimum_x;
    bounds.top = minimum_y;
    bounds.width = maximum_x - minimum_x;
    bounds.height = maximum_y - minimum_y;
    return std::isfinite(bounds.left) && std::isfinite(bounds.top)
        && std::isfinite(bounds.width) && std::isfinite(bounds.height)
        && bounds.width >= kTreasureMarkerSize
        && bounds.height >= kTreasureMarkerSize;
}

template <std::size_t PointCount>
void draw_atlas_polygon_aa(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const std::array<Vector2D, PointCount>& points,
    std::uint32_t color);

void draw_atlas_rectangle(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    double center_x,
    double center_y,
    double width,
    double height,
    std::uint32_t color) {
    const double half_width = width * 0.5;
    const double half_height = height * 0.5;
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        Vector2D{center_x - half_width, center_y - half_height},
        Vector2D{center_x + half_width, center_y - half_height},
        Vector2D{center_x + half_width, center_y + half_height},
        Vector2D{center_x - half_width, center_y + half_height},
    }, color);
}

void draw_atlas_diamond(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center,
    double width,
    double height,
    std::uint32_t color) {
    const double half_width = width * 0.5;
    const double half_height = height * 0.5;
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        Vector2D{center.x, center.y - half_height},
        Vector2D{center.x + half_width, center.y},
        Vector2D{center.x, center.y + half_height},
        Vector2D{center.x - half_width, center.y},
    }, color);
}

template <std::size_t PointCount>
void draw_atlas_polygon(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const std::array<Vector2D, PointCount>& points,
    std::uint32_t color) {
    static_assert(PointCount >= 3);
    constexpr std::int32_t texture_size = kWorldMapAtlasTextureSize;
    double minimum_x = points[0].x;
    double maximum_x = points[0].x;
    double minimum_y = points[0].y;
    double maximum_y = points[0].y;
    for (const Vector2D& point : points) {
        minimum_x = std::min(minimum_x, point.x);
        maximum_x = std::max(maximum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_y = std::max(maximum_y, point.y);
    }
    const auto x_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (minimum_x - bounds.left) / bounds.width
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto x_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (maximum_x - bounds.left) / bounds.width
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto y_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (minimum_y - bounds.top) / bounds.height
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto y_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (maximum_y - bounds.top) / bounds.height
            * static_cast<double>(texture_size))),
        0, texture_size);
    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const double local_y = bounds.top
            + (static_cast<double>(y) + 0.5) / texture_size * bounds.height;
        auto* row = pixels.data()
            + static_cast<std::size_t>(y) * texture_size;
        for (std::int32_t x = x_begin; x < x_end; ++x) {
            const double local_x = bounds.left
                + (static_cast<double>(x) + 0.5) / texture_size
                    * bounds.width;
            bool inside{};
            for (std::size_t current = 0, previous = PointCount - 1;
                 current < PointCount; previous = current++) {
                const Vector2D& left = points[current];
                const Vector2D& right = points[previous];
                const bool crosses = (left.y > local_y)
                    != (right.y > local_y);
                if (crosses
                    && local_x
                        < (right.x - left.x) * (local_y - left.y)
                            / (right.y - left.y)
                            + left.x) {
                    inside = !inside;
                }
            }
            if (inside) {
                row[x] = color;
            }
        }
    }
}

[[nodiscard]] std::uint32_t alpha_blend_coverage(
    std::uint32_t destination,
    std::uint32_t source,
    std::uint32_t covered_samples,
    std::uint32_t sample_count) noexcept {
    if (covered_samples == 0U || sample_count == 0U) {
        return destination;
    }
    if (covered_samples >= sample_count
        && ((source >> 24U) & 0xFFU) == 0xFFU) {
        return source;
    }

    const double coverage = static_cast<double>(covered_samples)
        / static_cast<double>(sample_count);
    const double source_alpha = static_cast<double>((source >> 24U) & 0xFFU)
        / 255.0 * coverage;
    const double destination_alpha =
        static_cast<double>((destination >> 24U) & 0xFFU) / 255.0;
    const double output_alpha = source_alpha
        + destination_alpha * (1.0 - source_alpha);
    if (output_alpha <= 0.0) {
        return kTransparent;
    }
    const auto blend_channel = [&](std::uint32_t shift) noexcept {
        const double source_channel =
            static_cast<double>((source >> shift) & 0xFFU);
        const double destination_channel =
            static_cast<double>((destination >> shift) & 0xFFU);
        const double output = (source_channel * source_alpha
            + destination_channel * destination_alpha
                * (1.0 - source_alpha)) / output_alpha;
        return static_cast<std::uint32_t>(
            std::clamp(std::lround(output), 0L, 255L));
    };
    const auto output_a = static_cast<std::uint32_t>(
        std::clamp(std::lround(output_alpha * 255.0), 0L, 255L));
    return (output_a << 24U)
        | (blend_channel(16U) << 16U)
        | (blend_channel(8U) << 8U)
        | blend_channel(0U);
}

template <std::size_t PointCount>
[[nodiscard]] bool point_inside_polygon(
    const std::array<Vector2D, PointCount>& points,
    double x,
    double y) noexcept {
    static_assert(PointCount >= 3);
    bool inside{};
    for (std::size_t current = 0, previous = PointCount - 1;
         current < PointCount; previous = current++) {
        const Vector2D& left = points[current];
        const Vector2D& right = points[previous];
        const bool crosses = (left.y > y) != (right.y > y);
        if (crosses
            && x < (right.x - left.x) * (y - left.y)
                    / (right.y - left.y) + left.x) {
            inside = !inside;
        }
    }
    return inside;
}

template <std::size_t PointCount>
void draw_atlas_polygon_aa(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const std::array<Vector2D, PointCount>& points,
    std::uint32_t color) {
    static_assert(PointCount >= 3);
    constexpr std::int32_t texture_size = kWorldMapAtlasTextureSize;
    constexpr std::int32_t sample_axis = 4;
    constexpr std::uint32_t sample_count = sample_axis * sample_axis;
    double minimum_x = points[0].x;
    double maximum_x = points[0].x;
    double minimum_y = points[0].y;
    double maximum_y = points[0].y;
    for (const Vector2D& point : points) {
        minimum_x = std::min(minimum_x, point.x);
        maximum_x = std::max(maximum_x, point.x);
        minimum_y = std::min(minimum_y, point.y);
        maximum_y = std::max(maximum_y, point.y);
    }
    const auto x_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (minimum_x - bounds.left) / bounds.width * texture_size)) - 1,
        0, texture_size);
    const auto x_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (maximum_x - bounds.left) / bounds.width * texture_size)) + 1,
        0, texture_size);
    const auto y_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (minimum_y - bounds.top) / bounds.height * texture_size)) - 1,
        0, texture_size);
    const auto y_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (maximum_y - bounds.top) / bounds.height * texture_size)) + 1,
        0, texture_size);
    for (std::int32_t y = y_begin; y < y_end; ++y) {
        auto* row = pixels.data()
            + static_cast<std::size_t>(y) * texture_size;
        for (std::int32_t x = x_begin; x < x_end; ++x) {
            std::uint32_t covered{};
            for (std::int32_t sample_y = 0; sample_y < sample_axis; ++sample_y) {
                const double local_y = bounds.top
                    + (static_cast<double>(y)
                        + (static_cast<double>(sample_y) + 0.5) / sample_axis)
                        / texture_size * bounds.height;
                for (std::int32_t sample_x = 0;
                     sample_x < sample_axis; ++sample_x) {
                    const double local_x = bounds.left
                        + (static_cast<double>(x)
                            + (static_cast<double>(sample_x) + 0.5)
                                / sample_axis)
                            / texture_size * bounds.width;
                    if (point_inside_polygon(points, local_x, local_y)) {
                        ++covered;
                    }
                }
            }
            if (covered != 0U) {
                row[x] = alpha_blend_coverage(
                    row[x], color, covered, sample_count);
            }
        }
    }
}

template <std::size_t PointCount>
[[nodiscard]] std::array<Vector2D, PointCount> transform_polygon(
    const std::array<Vector2D, PointCount>& points,
    const Vector2D& origin,
    double scale,
    double offset_x = 0.0,
    double offset_y = 0.0) noexcept {
    std::array<Vector2D, PointCount> transformed{};
    for (std::size_t index = 0; index < PointCount; ++index) {
        transformed[index] = {
            origin.x + (points[index].x - origin.x) * scale + offset_x,
            origin.y + (points[index].y - origin.y) * scale + offset_y,
        };
    }
    return transformed;
}

[[nodiscard]] Vector2D cubic_bezier(
    const Vector2D& p0,
    const Vector2D& p1,
    const Vector2D& p2,
    const Vector2D& p3,
    double t) noexcept {
    const double one_minus_t = 1.0 - t;
    const double a = one_minus_t * one_minus_t * one_minus_t;
    const double b = 3.0 * one_minus_t * one_minus_t * t;
    const double c = 3.0 * one_minus_t * t * t;
    const double d = t * t * t;
    return {
        a * p0.x + b * p1.x + c * p2.x + d * p3.x,
        a * p0.y + b * p1.y + c * p2.y + d * p3.y,
    };
}

[[nodiscard]] Vector2D boss_silhouette_point(
    const Vector2D& center,
    double source_x,
    double source_y) noexcept {
    return {
        center.x + (source_x - 0.5) * 0.5 * kBossMarkerSize,
        center.y + (-0.28 + source_y * 0.56) * kBossMarkerSize,
    };
}

void draw_boss_silhouette(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center,
    std::uint32_t color) {
    const auto point = [&center](double x, double y) noexcept {
        return boss_silhouette_point(center, x, y);
    };
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        point(0.45, 0.44), point(0.11, 0.05), point(0.17, 0.34),
        point(0.02, 0.27), point(0.20, 0.56), point(0.38, 0.59),
    }, color);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        point(0.55, 0.44), point(0.89, 0.05), point(0.83, 0.34),
        point(0.98, 0.27), point(0.80, 0.56), point(0.62, 0.59),
    }, color);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        point(0.40, 0.55), point(0.10, 0.53), point(0.00, 0.66),
        point(0.20, 0.69), point(0.12, 0.92), point(0.34, 0.76),
    }, color);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        point(0.60, 0.55), point(0.90, 0.53), point(1.00, 0.66),
        point(0.80, 0.69), point(0.88, 0.92), point(0.66, 0.76),
    }, color);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        point(0.34, 0.31), point(0.40, 0.18), point(0.46, 0.31),
        point(0.50, 0.12), point(0.54, 0.31), point(0.60, 0.18),
        point(0.66, 0.31), point(0.63, 0.51), point(0.58, 0.61),
        point(0.60, 0.81), point(0.72, 0.96), point(0.55, 0.91),
        point(0.50, 1.00), point(0.45, 0.91), point(0.28, 0.96),
        point(0.40, 0.81), point(0.42, 0.61), point(0.37, 0.51),
    }, color);
}

void draw_atlas_diagonal_cross(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center,
    double size,
    double thickness,
    std::uint32_t color) {
    constexpr std::int32_t texture_size = kWorldMapAtlasTextureSize;
    const double half = size * 0.5;
    const auto x_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (center.x - half - bounds.left) / bounds.width
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto x_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (center.x + half - bounds.left) / bounds.width
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto y_begin = std::clamp(
        static_cast<std::int32_t>(std::floor(
            (center.y - half - bounds.top) / bounds.height
            * static_cast<double>(texture_size))),
        0, texture_size);
    const auto y_end = std::clamp(
        static_cast<std::int32_t>(std::ceil(
            (center.y + half - bounds.top) / bounds.height
            * static_cast<double>(texture_size))),
        0, texture_size);
    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const double dy = bounds.top
            + (static_cast<double>(y) + 0.5) / texture_size * bounds.height
            - center.y;
        auto* row = pixels.data()
            + static_cast<std::size_t>(y) * texture_size;
        for (std::int32_t x = x_begin; x < x_end; ++x) {
            const double dx = bounds.left
                + (static_cast<double>(x) + 0.5) / texture_size
                    * bounds.width
                - center.x;
            if (std::abs(dx) <= half && std::abs(dy) <= half
                && (std::abs(dy - dx) <= thickness
                    || std::abs(dy + dx) <= thickness)) {
                row[x] = color;
            }
        }
    }
}

void draw_chest_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center,
    WorldMapUmgMarkerTone tone) {
    const std::uint32_t fill = marker_fill(tone);
    // Keep the authored 12-pixel footprint, but use a symmetric lid/body
    // silhouette and one strong lock instead of several sub-pixel bars. This
    // remains legible when the event atlas is sampled below one texel per UI
    // unit on the fully zoomed-out world map.
    draw_atlas_rectangle(
        pixels, bounds, center.x + 0.6, center.y + 0.8,
        10.8, 8.8, kShadow);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        Vector2D{center.x - 5.6, center.y - 0.8},
        Vector2D{center.x - 4.2, center.y - 4.4},
        Vector2D{center.x + 4.2, center.y - 4.4},
        Vector2D{center.x + 5.6, center.y - 0.8},
    }, kOutline);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        Vector2D{center.x - 4.0, center.y - 1.2},
        Vector2D{center.x - 3.2, center.y - 3.0},
        Vector2D{center.x + 3.2, center.y - 3.0},
        Vector2D{center.x + 4.0, center.y - 1.2},
    }, fill);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 2.2,
        11.2, 5.8, kOutline);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 2.0,
        8.8, 3.6, fill);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 1.4,
        2.8, 4.4, kOutline);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 1.1,
        1.2, 1.8, kOfficialPale);
}

void draw_boss_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    constexpr double shadow_offset = 0.9;
    const Vector2D shadow_center{
        center.x + shadow_offset, center.y + shadow_offset};
    draw_atlas_diamond(
        pixels, bounds, shadow_center,
        kBossMarkerSize, kBossMarkerSize, kShadow);
    draw_atlas_diamond(
        pixels, bounds, center,
        kBossMarkerSize, kBossMarkerSize, kOfficialWhite);
    draw_atlas_diamond(
        pixels, bounds, center,
        kBossMarkerSize - 5.0, kBossMarkerSize - 5.0,
        kOfficialGreenDark);
    draw_boss_silhouette(pixels, bounds, center, kOfficialWhite);
    draw_atlas_rectangle(
        pixels, bounds, center.x - 2.2, center.y - 1.0,
        2.2, 2.2, kOfficialGreenDark);
    draw_atlas_rectangle(
        pixels, bounds, center.x + 2.2, center.y - 1.0,
        2.2, 2.2, kOfficialGreenDark);
}

void draw_assault_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    constexpr double shadow_offset = 0.9;
    const Vector2D shadow_center{
        center.x + shadow_offset, center.y + shadow_offset};
    draw_atlas_diamond(
        pixels, bounds, shadow_center, kAssaultMarkerSize,
        kAssaultMarkerSize, kShadow);
    draw_atlas_diamond(
        pixels, bounds, center, kAssaultMarkerSize,
        kAssaultMarkerSize, kOfficialWhite);
    draw_atlas_diamond(
        pixels, bounds, center, kAssaultMarkerSize - 4.0,
        kAssaultMarkerSize - 4.0, kOfficialPale);
    draw_atlas_diamond(
        pixels, bounds, center, kAssaultMarkerSize - 8.0,
        kAssaultMarkerSize - 8.0, kOfficialGreenDark);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y - 3.2,
        5.2, 13.0, kOfficialWhite);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y - 3.2,
        3.0, 10.4, kOfficialCyan);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 7.2,
        5.4, 5.4, kOfficialWhite);
    draw_atlas_rectangle(
        pixels, bounds, center.x, center.y + 7.2,
        3.2, 3.2, kOfficialCyan);
}

void draw_fly_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    constexpr double d = kMiniGameMarkerSize;
    constexpr double outline_scale = 1.11;
    constexpr double shadow_offset = 0.9;
    const auto wing = [&](double side) {
        return std::array{
            Vector2D{center.x + side * d * .10, center.y + d * .05},
            Vector2D{center.x + side * d * .31, center.y - d * .17},
            Vector2D{center.x + side * d * .48, center.y - d * .18},
            Vector2D{center.x + side * d * .40, center.y - d * .04},
            Vector2D{center.x + side * d * .50, center.y + d * .01},
            Vector2D{center.x + side * d * .35, center.y + d * .09},
            Vector2D{center.x + side * d * .40, center.y + d * .18},
            Vector2D{center.x + side * d * .13, center.y + d * .13},
        };
    };
    const auto arrow = std::array{
        Vector2D{center.x - d * .10, center.y + d * .40},
        Vector2D{center.x + d * .10, center.y + d * .40},
        Vector2D{center.x + d * .10, center.y - d * .13},
        Vector2D{center.x + d * .27, center.y - d * .13},
        Vector2D{center.x, center.y - d * .43},
        Vector2D{center.x - d * .27, center.y - d * .13},
        Vector2D{center.x - d * .10, center.y - d * .13},
    };
    const auto left_wing = wing(-1.0);
    const auto right_wing = wing(1.0);
    // Paint each visual layer as one pass. A later wing must not place its
    // translucent shadow over an already filled sibling.
    for (const auto& shape : {left_wing, right_wing}) {
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(shape, center, 1.0, shadow_offset, shadow_offset),
            kShadow);
    }
    draw_atlas_polygon_aa(pixels, bounds,
        transform_polygon(arrow, center, 1.0, shadow_offset, shadow_offset),
        kShadow);
    for (const auto& shape : {left_wing, right_wing}) {
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(shape, center, outline_scale), kOutline);
    }
    draw_atlas_polygon_aa(pixels, bounds,
        transform_polygon(arrow, center, outline_scale), kOutline);
    for (const auto& shape : {left_wing, right_wing}) {
        draw_atlas_polygon_aa(pixels, bounds, shape, kFlyWing);
    }
    draw_atlas_polygon_aa(pixels, bounds, arrow, kFlyArrow);
}

void draw_mole_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    constexpr double d = kMiniGameMarkerSize;
    constexpr double outline_scale = 1.12;
    constexpr double shadow_offset = 0.9;
    // Exact accepted overlay proportions: a broad four-corner head and a
    // separate tapered handle, instead of the former two-block approximation.
    const auto head = std::array{
        Vector2D{center.x - d * .27, center.y - d * .34},
        Vector2D{center.x + d * .30, center.y - d * .08},
        Vector2D{center.x + d * .18, center.y + d * .14},
        Vector2D{center.x - d * .39, center.y - d * .12},
    };
    const auto handle = std::array{
        Vector2D{center.x - d * .06, center.y - d * .01},
        Vector2D{center.x + d * .07, center.y + d * .05},
        Vector2D{center.x - d * .27, center.y + d * .40},
        Vector2D{center.x - d * .40, center.y + d * .34},
    };
    for (const auto& shape : {handle, head}) {
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(shape, center, 1.0, shadow_offset, shadow_offset),
            kShadow);
    }
    for (const auto& shape : {handle, head}) {
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(shape, center, outline_scale), kOutline);
    }
    draw_atlas_polygon_aa(pixels, bounds, handle, kHammerHandle);
    draw_atlas_polygon_aa(pixels, bounds, head, kHammer);
    draw_atlas_polygon_aa(pixels, bounds,
        transform_polygon(head, center, 0.72, -d * .035, -d * .035),
        kHammerHighlight);
}

void draw_wave_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    constexpr double d = kMiniGameMarkerSize;
    constexpr double outline_scale = 1.10;
    constexpr double shadow_offset = 0.9;
    constexpr std::size_t curve_steps = 5;
    std::array<std::array<Vector2D, 17>, 4> waves{};
    for (std::size_t strand = 0; strand < 4; ++strand) {
        const double offset =
            (static_cast<double>(strand) - 1.5) * d * .11;
        const Vector2D p0{center.x - d * .42 + offset,
                          center.y + d * .28};
        const Vector2D p1{center.x - d * .23 + offset,
                          center.y - d * .18};
        const Vector2D p2{center.x - d * .02 + offset,
                          center.y - d * .43};
        const Vector2D p3{center.x + d * .18 + offset,
                          center.y - d * .22};
        const Vector2D p4{center.x + d * .04 + offset,
                          center.y - d * .06};
        const Vector2D p5{center.x + d * .01 + offset,
                          center.y + d * .15};
        const Vector2D p6{center.x + d * .12 + offset,
                          center.y + d * .34};
        const Vector2D p7{center.x - d * .01 + offset,
                          center.y + d * .35};
        const Vector2D p8{center.x - d * .17 + offset,
                          center.y + d * .08};
        const Vector2D p9{center.x - d * .18 + offset,
                          center.y - d * .04};
        auto& wave = waves[strand];
        wave[0] = p0;
        for (std::size_t step = 1; step <= curve_steps; ++step) {
            wave[step] = cubic_bezier(
                p0, p1, p2, p3,
                static_cast<double>(step) / curve_steps);
            wave[curve_steps + step] = cubic_bezier(
                p3, p4, p5, p6,
                static_cast<double>(step) / curve_steps);
            wave[2 * curve_steps + 1 + step] = cubic_bezier(
                p7, p8, p9, p0,
                static_cast<double>(step) / curve_steps);
        }
        wave[2 * curve_steps + 1] = p7;
    }
    for (std::size_t strand = 0; strand < waves.size(); ++strand) {
        const double offset =
            (static_cast<double>(strand) - 1.5) * d * .11;
        const Vector2D strand_center{center.x + offset, center.y};
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(waves[strand], strand_center, 1.0,
                              shadow_offset, shadow_offset), kShadow);
    }
    for (std::size_t strand = 0; strand < waves.size(); ++strand) {
        const double offset =
            (static_cast<double>(strand) - 1.5) * d * .11;
        const Vector2D strand_center{center.x + offset, center.y};
        draw_atlas_polygon_aa(pixels, bounds,
            transform_polygon(waves[strand], strand_center, outline_scale),
            kOutline);
    }
    for (const auto& wave : waves) {
        draw_atlas_polygon_aa(pixels, bounds, wave, kWave);
    }
}

void draw_area_quest_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center) {
    const Vector2D shadow_center{center.x + 1.0, center.y + 1.0};
    draw_atlas_diamond(
        pixels, bounds, shadow_center, kAreaQuestMarkerSize,
        kAreaQuestMarkerSize, kShadow);
    draw_atlas_diamond(
        pixels, bounds, center, kAreaQuestMarkerSize,
        kAreaQuestMarkerSize, kAreaQuestBubble);
    draw_atlas_diamond(
        pixels, bounds, center, kAreaQuestMarkerSize - 4.0,
        kAreaQuestMarkerSize - 4.0, kAreaQuestDark);
    draw_atlas_polygon_aa(pixels, bounds, std::array{
        Vector2D{center.x - 7.0, center.y - 5.0},
        Vector2D{center.x + 7.0, center.y - 5.0},
        Vector2D{center.x + 7.0, center.y + 3.0},
        Vector2D{center.x - 1.0, center.y + 3.0},
        Vector2D{center.x - 4.0, center.y + 6.0},
        Vector2D{center.x - 3.0, center.y + 3.0},
        Vector2D{center.x - 7.0, center.y + 3.0},
    }, kOfficialWhite);
    for (const double offset_x : {-3.8, 0.0, 3.8}) {
        draw_atlas_rectangle(
            pixels, bounds, center.x + offset_x, center.y - 0.8,
            2.2, 2.2, kAreaQuestDark);
    }
}

void draw_marker_glyph(
    std::vector<std::uint32_t>& pixels,
    const AtlasBounds& bounds,
    const Vector2D& center,
    const WorldMapUmgMarker& marker) {
    switch (marker.kind) {
    case WorldMapUmgMarkerKind::Boss:
        draw_boss_glyph(pixels, bounds, center);
        break;
    case WorldMapUmgMarkerKind::Assault:
        draw_assault_glyph(pixels, bounds, center);
        break;
    case WorldMapUmgMarkerKind::Fly:
        draw_fly_glyph(pixels, bounds, center);
        break;
    case WorldMapUmgMarkerKind::Mole:
        draw_mole_glyph(pixels, bounds, center);
        break;
    case WorldMapUmgMarkerKind::Wave:
        draw_wave_glyph(pixels, bounds, center);
        break;
    case WorldMapUmgMarkerKind::AreaQuest:
        draw_area_quest_glyph(pixels, bounds, center);
        break;
    default:
        draw_chest_glyph(pixels, bounds, center, marker.tone);
        break;
    }
}

void write_tga_pixel(std::ofstream& output, std::uint32_t pixel) {
    const std::array<char, 4> bytes{
        static_cast<char>(pixel & 0xFFU),
        static_cast<char>((pixel >> 8U) & 0xFFU),
        static_cast<char>((pixel >> 16U) & 0xFFU),
        static_cast<char>((pixel >> 24U) & 0xFFU)};
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

[[nodiscard]] bool write_rle_tga(
    const std::filesystem::path& path,
    const std::vector<std::uint32_t>& pixels) {
    constexpr std::size_t texture_size = kWorldMapAtlasTextureSize;
    if (pixels.size() != texture_size * texture_size) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            return false;
        }
    }
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    if (!output) {
        return false;
    }

    std::array<std::uint8_t, 18> header{};
    header[2] = 10U;
    header[12] = static_cast<std::uint8_t>(texture_size & 0xFFU);
    header[13] = static_cast<std::uint8_t>((texture_size >> 8U) & 0xFFU);
    header[14] = static_cast<std::uint8_t>(texture_size & 0xFFU);
    header[15] = static_cast<std::uint8_t>((texture_size >> 8U) & 0xFFU);
    header[16] = 32U;
    header[17] = 0x28U;
    output.write(
        reinterpret_cast<const char*>(header.data()),
        static_cast<std::streamsize>(header.size()));

    for (std::size_t y = 0; y < texture_size; ++y) {
        const std::uint32_t* row = pixels.data() + y * texture_size;
        std::size_t x = 0;
        while (x < texture_size) {
            std::size_t run = 1;
            while (run < 128U && x + run < texture_size
                   && row[x + run] == row[x]) {
                ++run;
            }
            if (run >= 2U) {
                output.put(static_cast<char>(0x80U | (run - 1U)));
                write_tga_pixel(output, row[x]);
                x += run;
                continue;
            }

            const std::size_t raw_start = x++;
            std::size_t raw_count = 1;
            while (raw_count < 128U && x < texture_size) {
                std::size_t following_run = 1;
                while (following_run < 128U
                       && x + following_run < texture_size
                       && row[x + following_run] == row[x]) {
                    ++following_run;
                }
                if (following_run >= 2U) {
                    break;
                }
                ++x;
                ++raw_count;
            }
            output.put(static_cast<char>(raw_count - 1U));
            for (std::size_t index = 0; index < raw_count; ++index) {
                write_tga_pixel(output, row[raw_start + index]);
            }
        }
    }
    output.flush();
    return output.good();
}

[[nodiscard]] AtlasBuildResult build_rle_tga_atlas(
    const std::filesystem::path& path,
    const WorldMapUmgMarkerArray& markers,
    const std::array<Vector2D, kWorldMapUmgMarkerCapacity>& local_positions,
    std::size_t marker_count,
    const AtlasBounds& bounds,
    AtlasLayer layer) noexcept {
    const auto started = std::chrono::steady_clock::now();
    AtlasBuildResult result{};
    try {
        std::size_t fingerprint_visible_count{};
        const std::uint64_t fingerprint = atlas_input_fingerprint(
            markers, local_positions, marker_count, bounds,
            layer, fingerprint_visible_count);
        const std::uint64_t path_hash = static_cast<std::uint64_t>(
            std::filesystem::hash_value(path));
        AtlasFileCache& cache = atlas_file_caches()[
            static_cast<std::size_t>(layer)];
        if (cache.valid && cache.path_hash == path_hash
            && cache.input_fingerprint == fingerprint
            && cache.visible_marker_count == fingerprint_visible_count) {
            std::error_code error;
            const std::uint64_t current_bytes =
                std::filesystem::file_size(path, error);
            if (!error && current_bytes == cache.file_bytes
                && current_bytes >= 18U) {
                result.success = true;
                result.drawn_marker_count = fingerprint_visible_count;
                result.file_bytes = current_bytes;
            }
        }

        if (!result.success) {
            const std::size_t pixel_count =
                static_cast<std::size_t>(kWorldMapAtlasTextureSize)
                * kWorldMapAtlasTextureSize;
            std::vector<std::uint32_t> pixels(pixel_count, kTransparent);

            // Within the lower radar host, treasure chests remain above tasks
            // and mini-games. Both radar hosts are above game-native map icons.
            for (std::size_t index = 0; index < marker_count; ++index) {
                if (!markers[index].visible
                    || !marker_belongs_to_layer(markers[index].kind, layer)
                    || markers[index].kind
                        == WorldMapUmgMarkerKind::Treasure) {
                    continue;
                }
                draw_marker_glyph(
                    pixels, bounds, local_positions[index], markers[index]);
                ++result.drawn_marker_count;
            }
            for (std::size_t index = 0; index < marker_count; ++index) {
                if (!markers[index].visible
                    || !marker_belongs_to_layer(markers[index].kind, layer)
                    || markers[index].kind
                        != WorldMapUmgMarkerKind::Treasure) {
                    continue;
                }
                draw_marker_glyph(
                    pixels, bounds, local_positions[index], markers[index]);
                ++result.drawn_marker_count;
            }
            result.success = write_rle_tga(path, pixels);
            if (result.success) {
                std::error_code error;
                result.file_bytes = std::filesystem::file_size(path, error);
                result.success = !error && result.file_bytes >= 18U;
            }
            if (result.success) {
                cache.path_hash = path_hash;
                cache.input_fingerprint = fingerprint;
                cache.file_bytes = result.file_bytes;
                cache.visible_marker_count = result.drawn_marker_count;
                cache.valid = true;
            }
        }
    } catch (...) {
        result.success = false;
    }
    result.elapsed_us = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started).count());
    return result;
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
[[nodiscard]] WorldMapUmgPaintOwnerStatus resolve_paint_owner_status(
    UClass* host_class) {
    if (!host_class) {
        return WorldMapUmgPaintOwnerStatus::Unresolved;
    }
    constexpr wchar_t paint_name[]{L'O', L'n', L'P', L'a', L'i', L'n', L't', L'\0'};
    UFunction* function = host_class->GetFunctionByName(paint_name);
    if (!function) {
        return WorldMapUmgPaintOwnerStatus::Missing;
    }
    UObject* owner = function->GetOuterPrivate();
    if (owner == host_class) {
        return WorldMapUmgPaintOwnerStatus::HostClass;
    }
    for (UClass* base = host_class->GetSuperClass(); base;
         base = base->GetSuperClass()) {
        if (owner == base) {
            return WorldMapUmgPaintOwnerStatus::BaseClass;
        }
    }
    return WorldMapUmgPaintOwnerStatus::UnexpectedOwner;
}

} // namespace

void WorldMapUmgRenderer::initialize(
    std::filesystem::path atlas_cache_path) noexcept {
    reset_geometry_stability_sample();
    reset_reparent_geometry_stability_sample();
    cached_map_id_ = 0;
    cached_map_dimensions_ = 0.0;
    cached_map_ui_size_ = 0.0;
    map_data_cache_hit_count_ = 0;
    last_map_data_source_ = 0;
    atlas_build_elapsed_us_ = 0;
    atlas_file_bytes_ = 0;
    attach_elapsed_us_ = 0;
    reparent_count_ = 0;
    reproject_count_ = 0;
    atlas_cache_paths_[0] = std::move(atlas_cache_path);
    atlas_cache_paths_[1] = atlas_cache_paths_[0].parent_path()
        / "world-map-encounter-atlas.tga";

    world_map_layer_class_ = find<UClass>(L"/Script/DSClient.DLayerMap");
    world_map_data_class_ = find<UClass>(L"/Script/DSClient.DWorldMapData");
    overlay_class_ = find<UClass>(L"/Script/UMG.Overlay");
    retainer_box_class_ = find<UClass>(L"/Script/UMG.RetainerBox");
    canvas_panel_class_ = find<UClass>(L"/Script/UMG.CanvasPanel");
    canvas_panel_slot_class_ = find<UClass>(L"/Script/UMG.CanvasPanelSlot");
    map_point_icon_class_ =
        find<UClass>(L"/Script/DSClient.DMapPointIconUserWidget");
    image_class_ = find<UClass>(L"/Script/UMG.Image");
    widget_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetBlueprintLibrary");
    kismet_rendering_library_ =
        find<UObject>(L"/Script/Engine.Default__KismetRenderingLibrary");
    slate_blueprint_library_ =
        find<UObject>(L"/Script/UMG.Default__SlateBlueprintLibrary");
    widget_layout_library_ =
        find<UObject>(L"/Script/UMG.Default__WidgetLayoutLibrary");

    create_widget_ = find<UFunction>(L"/Script/UMG.WidgetBlueprintLibrary:Create");
    get_owning_player_ = find<UFunction>(L"/Script/UMG.Widget:GetOwningPlayer");
    get_slot_position_ = find<UFunction>(L"/Script/UMG.CanvasPanelSlot:GetPosition");
    get_slot_alignment_ =
        find<UFunction>(L"/Script/UMG.CanvasPanelSlot:GetAlignment");
    get_cached_geometry_ =
        find<UFunction>(L"/Script/UMG.Widget:GetCachedGeometry");
    get_geometry_local_size_ =
        find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:GetLocalSize");
    local_to_absolute_ =
        find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:LocalToAbsolute");
    absolute_to_local_ =
        find<UFunction>(L"/Script/UMG.SlateBlueprintLibrary:AbsoluteToLocal");
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
    set_brush_from_texture_ =
        find<UFunction>(L"/Script/UMG.Image:SetBrushFromTexture");
    import_file_as_texture_ = find<UFunction>(
        L"/Script/Engine.KismetRenderingLibrary:ImportFileAsTexture2D");
    clear_children_ = find<UFunction>(L"/Script/UMG.PanelWidget:ClearChildren");
    remove_from_parent_ = find<UFunction>(L"/Script/UMG.Widget:RemoveFromParent");
    add_to_viewport_ = find<UFunction>(L"/Script/UMG.UserWidget:AddToViewport");
    get_viewport_widget_geometry_ = find<UFunction>(
        L"/Script/UMG.WidgetLayoutLibrary:GetViewportWidgetGeometry");
    set_alignment_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetAlignmentInViewport");
    set_desired_size_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetDesiredSizeInViewport");
    set_position_in_viewport_ = find<UFunction>(
        L"/Script/UMG.UserWidget:SetPositionInViewport");

    abi_failure_mask_ = 0;
    const auto require_parameters =
        [this](std::uint32_t bit, UFunction* function, std::int32_t size) {
            if (!function || function->GetParmsSize() != size) {
                abi_failure_mask_ |= bit;
            }
        };
    require_parameters(1U << 0U, create_widget_, 32);
    require_parameters(1U << 1U, get_owning_player_, 8);
    require_parameters(1U << 2U, get_slot_position_, 16);
    require_parameters(1U << 3U, add_child_to_canvas_, 16);
    require_parameters(1U << 4U, set_slot_position_, 16);
    require_parameters(1U << 5U, set_slot_size_, 16);
    require_parameters(1U << 6U, set_slot_alignment_, 16);
    require_parameters(1U << 7U, set_slot_z_order_, 4);
    require_parameters(1U << 8U, set_visibility_, 1);
    // Unreal reports the reflected parameter span without the local C++ tail
    // padding after bMatchSize: UTexture2D* at offset 0 plus bool at offset 8.
    // Keep the naturally aligned 16-byte call buffer, but validate the real
    // nine-byte UFunction span just like SetPositionInViewport in the compact
    // renderer validates 17 bytes for its 24-byte local buffer.
    require_parameters(1U << 9U, set_brush_from_texture_, 9);
    require_parameters(1U << 10U, import_file_as_texture_, 32);
    require_parameters(1U << 12U, clear_children_, 0);
    require_parameters(1U << 13U, remove_from_parent_, 0);
    require_parameters(1U << 18U, add_to_viewport_, 4);
    require_parameters(1U << 19U, set_alignment_in_viewport_, 16);
    require_parameters(1U << 20U, set_desired_size_in_viewport_, 16);
    require_parameters(1U << 21U, set_position_in_viewport_, 17);

    GeometryReflectionSchema geometry_schema{};
    if (!resolve_geometry_reflection_schema(
            get_cached_geometry_, get_slot_alignment_,
            get_geometry_local_size_, local_to_absolute_,
            absolute_to_local_, geometry_schema)) {
        abi_failure_mask_ |= 1U << 17U;
    }
    ViewportGeometryReflectionSchema viewport_geometry_schema{};
    if (!resolve_viewport_geometry_reflection_schema(
            get_viewport_widget_geometry_,
            geometry_schema.cached_geometry_return,
            viewport_geometry_schema)) {
        abi_failure_mask_ |= 1U << 22U;
    }

    if (!world_map_layer_class_ || !world_map_data_class_
        || !overlay_class_ || !retainer_box_class_
        || !canvas_panel_class_ || !canvas_panel_slot_class_
        || !map_point_icon_class_ || !image_class_
        || !widget_blueprint_library_.Get()
        || !kismet_rendering_library_.Get()
        || !slate_blueprint_library_.Get()
        || !widget_layout_library_.Get()) {
        abi_failure_mask_ |= 1U << 15U;
    }
    if (atlas_cache_paths_[0].empty() || atlas_cache_paths_[1].empty()) {
        abi_failure_mask_ |= 1U << 16U;
    }

    state_ = abi_failure_mask_ == 0
        ? WorldMapUmgRendererState::Ready
        : WorldMapUmgRendererState::Disabled;
}

void WorldMapUmgRenderer::begin_activation() noexcept {
    activation_active_ = false;
    runtime_visibility_allowed_ = false;
    transform_ready_ = false;
    last_transform_sync_stage_ = dswros::WorldMapTransformSyncStage::None;
    reset_geometry_stability_sample();
    reset_reparent_geometry_stability_sample();
    if (state_ == WorldMapUmgRendererState::Suspended) {
        activation_active_ = true;
        attach_attempted_ = false;
        map_data_lookup_attempted_ = false;
        last_attach_failure_ = 0;
        return;
    }
    const std::uint64_t faults_before_detach = fault_count_;
    detach_guarded();
    // A previous guarded runtime operation may have faulted after its world
    // released the expanded-map widget tree. detach_guarded() has now cleared
    // every weak runtime handle, so one later explicit activation may retry
    // when the immutable reflected ABI remains valid. Never recover an ABI
    // failure or a detach fault raised by this same activation attempt.
    if (state_ == WorldMapUmgRendererState::Faulted
        && fault_count_ == faults_before_detach
        && abi_failure_mask_ == 0) {
        state_ = WorldMapUmgRendererState::Ready;
    }
    if (state_ != WorldMapUmgRendererState::Ready) {
        return;
    }
    activation_active_ = true;
    attach_attempted_ = false;
    map_data_lookup_attempted_ = false;
    last_attach_failure_ = 0;
    paint_owner_status_ = WorldMapUmgPaintOwnerStatus::Unresolved;
}

void WorldMapUmgRenderer::begin_map_session() noexcept {
    if (!activation_active_) {
        return;
    }
    runtime_visibility_allowed_ = false;
    transform_ready_ = false;
    reset_geometry_stability_sample();
    reset_reparent_geometry_stability_sample();
    last_transform_sync_stage_ = dswros::WorldMapTransformSyncStage::None;
    detach_guarded();
    if (state_ != WorldMapUmgRendererState::Ready) {
        activation_active_ = false;
        return;
    }
    attach_attempted_ = false;
    map_data_lookup_attempted_ = false;
    last_attach_failure_ = 0;
    paint_owner_status_ = WorldMapUmgPaintOwnerStatus::Unresolved;
}

bool WorldMapUmgRenderer::detect_current_map_id(
    UObject* current_layer, std::int32_t& map_id) const noexcept {
    return detect_current_map_id_guarded(current_layer, map_id);
}

bool WorldMapUmgRenderer::detect_current_map_id_guarded(
    UObject* current_layer, std::int32_t& map_id) const noexcept {
    map_id = 0;
#if defined(_MSC_VER)
    __try {
        if (!current_layer || !world_map_layer_class_
            || !current_layer->IsA(world_map_layer_class_)) {
            return false;
        }
        map_id = detect_map_id(current_layer);
        return map_id == 100 || map_id == 200;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        map_id = 0;
        return false;
    }
#else
    try {
        if (!current_layer || !world_map_layer_class_
            || !current_layer->IsA(world_map_layer_class_)) {
            return false;
        }
        map_id = detect_map_id(current_layer);
        return map_id == 100 || map_id == 200;
    } catch (...) {
        map_id = 0;
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::validate_host_unsafe(
    UObject* current_layer) const {
    if (!current_layer || current_layer != layer_.Get()
        || !current_layer->IsA(world_map_layer_class_)) {
        return false;
    }
    UObject* owning_player{};
    for (std::size_t layer_index = 0;
         layer_index < kWorldMapAtlasLayerCount; ++layer_index) {
        UObject* host = hosts_[layer_index].Get();
        UObject* tree = widget_trees_[layer_index].Get();
        UObject* root_panel = root_panels_[layer_index].Get();
        UObject* image = atlas_images_[layer_index].Get();
        UObject* image_slot = atlas_image_slots_[layer_index].Get();
        UObject* texture = atlas_textures_[layer_index].Get();
        if (!host || !host->IsA(map_point_icon_class_)
            || !tree || read_object_property(host, L"WidgetTree") != tree
            || !root_panel || !root_panel->IsA(canvas_panel_class_)
            || read_object_property(host, L"Panel_Point") != root_panel
            || !image || !image->IsA(image_class_)
            || !image_slot || !image_slot->IsA(canvas_panel_slot_class_)
            || !texture
            || read_object_property(image, L"Slot") != image_slot
            || read_object_property(image_slot, L"Parent") != root_panel
            || read_object_property(image_slot, L"Content") != image
            || read_struct_object_property(
                   image, L"Brush", L"ResourceObject") != texture) {
            return false;
        }
        ObjectReturnParameters host_owner{};
        host->ProcessEvent(get_owning_player_, &host_owner);
        if (!host_owner.return_value
            || (owning_player && owning_player != host_owner.return_value)) {
            return false;
        }
        owning_player = host_owner.return_value;
    }
    return owning_player != nullptr;
}

bool WorldMapUmgRenderer::validate_host_payload_unsafe(
    UObject* current_layer, UObject*& owning_player) const {
    owning_player = nullptr;
    if (!validate_host_unsafe(current_layer)) {
        return false;
    }
    for (const auto& host_handle : hosts_) {
        UObject* host = host_handle.Get();
        ObjectReturnParameters owner{};
        host->ProcessEvent(get_owning_player_, &owner);
        if (!owner.return_value
            || (owning_player && owning_player != owner.return_value)) {
            return false;
        }
        owning_player = owner.return_value;
    }
    return owning_player != nullptr;
}

bool WorldMapUmgRenderer::validate_host_payload_guarded(
    UObject* current_layer) const noexcept {
    UObject* owning_player{};
#if defined(_MSC_VER)
    __try {
        return validate_host_payload_unsafe(
            current_layer, owning_player);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return validate_host_payload_unsafe(
            current_layer, owning_player);
    } catch (...) {
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::attached_to(
    UObject* current_layer, std::int32_t map_id) const noexcept {
    if (!current_layer || map_id <= 0
        || state_ != WorldMapUmgRendererState::Attached
        || map_id_ != map_id) {
        return false;
    }
#if defined(_MSC_VER)
    __try {
        return layer_.Get() == current_layer;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return layer_.Get() == current_layer;
    } catch (...) {
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::attached_layer_matches(
    UObject* current_layer) const noexcept {
    if (!current_layer || state_ != WorldMapUmgRendererState::Attached) {
        return false;
    }
#if defined(_MSC_VER)
    __try {
        return layer_.Get() == current_layer;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    try {
        return layer_.Get() == current_layer;
    } catch (...) {
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::apply_host_visibility_unsafe(
    bool visible,
    volatile dswros::WorldMapTransformSyncStage& stage) {
    if (!dswros::world_map_host_visibility_write_required(
            applied_host_visibility_, visible)) {
        return true;
    }

    stage = dswros::WorldMapTransformSyncStage::OwnedHostValidation;
    std::array<UObject*, kWorldMapAtlasLayerCount> hosts{};
    for (std::size_t index = 0;
         index < kWorldMapAtlasLayerCount; ++index) {
        hosts[index] = hosts_[index].Get();
        if (!hosts[index]) {
            applied_host_visibility_.reset();
            return false;
        }
    }

    // A two-host update is published only after both reflected writes
    // succeed. If either write faults, the enclosing guard observes Unknown
    // rather than incorrectly deduplicating a split host state later.
    applied_host_visibility_.reset();
    stage = dswros::WorldMapTransformSyncStage::OwnedHostApplication;
    for (UObject* host : hosts) {
        set_visibility(
            host, set_visibility_,
            visible ? kHitTestInvisible : kCollapsed);
    }
    applied_host_visibility_ = visible;
    return true;
}

bool WorldMapUmgRenderer::reconcile_host_visibility_unsafe(
    bool force_collapsed,
    volatile dswros::WorldMapTransformSyncStage& stage) {
    if (state_ != WorldMapUmgRendererState::Attached
        && state_ != WorldMapUmgRendererState::Suspended) {
        return true;
    }
    const bool show = dswros::world_map_host_visibility_target({
        content_visibility_intent_,
        runtime_visibility_allowed_,
        state_ == WorldMapUmgRendererState::Attached,
        transform_ready_,
    });
    return apply_host_visibility_unsafe(
        !force_collapsed && show, stage);
}

bool WorldMapUmgRenderer::reconcile_host_visibility_guarded(
    bool force_collapsed,
    volatile dswros::WorldMapTransformSyncStage& stage) noexcept {
    bool completed = false;
#if defined(_MSC_VER)
    __try {
        completed = reconcile_host_visibility_unsafe(
            force_collapsed, stage);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        completed = false;
    }
#else
    try {
        completed = reconcile_host_visibility_unsafe(
            force_collapsed, stage);
    } catch (...) {
        completed = false;
    }
#endif
    return completed;
}

void WorldMapUmgRenderer::fault_and_detach(
    std::uint32_t failure) noexcept {
    ++fault_count_;
    last_attach_failure_ = failure;
    activation_active_ = false;
    runtime_visibility_allowed_ = false;
    transform_ready_ = false;
    applied_host_visibility_.reset();
    state_ = WorldMapUmgRendererState::Faulted;
    detach_guarded();
}

bool WorldMapUmgRenderer::sync_viewport_transform_unsafe(
    UObject* current_layer,
    WorldMapLayeringRefreshResult& result,
    volatile dswros::WorldMapTransformSyncStage& stage) {
    result = WorldMapLayeringRefreshResult::Faulted;
    stage = dswros::WorldMapTransformSyncStage::NativeCanvasObservation;
    UObject* retained_layer = layer_.Get();
    const auto handle_transient_observation =
        [this, current_layer, retained_layer, &stage](
            dswros::WorldMapTransformObservationFailure failure,
            WorldMapLayeringRefreshResult& transient_result) {
        const auto action =
            dswros::classify_world_map_transform_observation_failure(
                failure, retained_layer == current_layer,
                viewport_transform_valid_);
        if (action
            == dswros::WorldMapTransformObservationFailureAction::Fault) {
            return false;
        }
        const auto visibility_policy =
            dswros::world_map_transform_visibility_policy(action);
        const auto observation_stage = stage;
        transform_ready_ = visibility_policy.transform_ready;
        if (!reconcile_host_visibility_unsafe(
                visibility_policy.force_collapsed, stage)) {
            return false;
        }
        // Keep the observation reason for successful RetryLater/Retained
        // diagnostics. A visibility-write exception leaves the stage at
        // OwnedHostApplication and is classified as a hard runtime fault.
        stage = observation_stage;
        transient_result = visibility_policy.transform_ready
            ? WorldMapLayeringRefreshResult::Retained
            : WorldMapLayeringRefreshResult::RetryLater;
        return true;
    };

    // A newly created or temporarily absent game layer is not evidence that
    // the existing Mod-owned viewport payload is corrupt. Let the bounded
    // candidate lifecycle attach the new layer without mutating either tree.
    // This check must precede validation, whose exact-layer identity guard is
    // intentionally a hard ownership invariant once the layer matches.
    if (!current_layer || current_layer != retained_layer) {
        return handle_transient_observation(
            dswros::WorldMapTransformObservationFailure::NativeCanvasUnavailable,
            result);
    }

    stage = dswros::WorldMapTransformSyncStage::OwnedHostValidation;
    UObject* owning_player{};
    if (!validate_host_payload_unsafe(current_layer, owning_player)) {
        return false;
    }

    stage = dswros::WorldMapTransformSyncStage::AbiValidation;
    GeometryReflectionSchema geometry_schema{};
    ViewportGeometryReflectionSchema viewport_schema{};
    UObject* slate_library = slate_blueprint_library_.Get();
    UObject* layout_library = widget_layout_library_.Get();
    if (!slate_library || !layout_library
        || !resolve_geometry_reflection_schema(
            get_cached_geometry_, get_slot_alignment_,
            get_geometry_local_size_, local_to_absolute_,
            absolute_to_local_, geometry_schema)
        || !resolve_viewport_geometry_reflection_schema(
            get_viewport_widget_geometry_,
            geometry_schema.cached_geometry_return,
            viewport_schema)) {
        return false;
    }
    GeometryCallParameters native_parameters(get_cached_geometry_);
    GeometryCallParameters viewport_parameters(get_viewport_widget_geometry_);
    if (!native_parameters.valid() || !viewport_parameters.valid()
        || !write_object_property(
            viewport_schema.world_context_object,
            viewport_parameters.data(), current_layer)) {
        return false;
    }

    // The native Canvas selected during attachment is the sole coordinate
    // witness for this attached layer. Re-scanning ArrayIconInfo while the map
    // animates can momentarily select an empty or replacement Canvas, causing
    // A-B-A placement oscillation. Never switch witnesses during a live
    // attachment; a real layer replacement is handled by the attach lifecycle.
    stage = dswros::WorldMapTransformSyncStage::NativeCanvasObservation;
    UObject* observed_native_parent = native_parent_.Get();
    if (!observed_native_parent
        || !observed_native_parent->IsA(canvas_panel_class_)) {
        return handle_transient_observation(
            dswros::WorldMapTransformObservationFailure::NativeCanvasUnavailable,
            result);
    }

    stage = dswros::WorldMapTransformSyncStage::GeometryObservation;
    observed_native_parent->ProcessEvent(
        get_cached_geometry_, native_parameters.data());
    layout_library->ProcessEvent(
        get_viewport_widget_geometry_, viewport_parameters.data());
    dswros::WorldMapSlateGeometry native_geometry{};
    dswros::WorldMapSlateGeometry viewport_geometry{};
    const bool geometry_ready =
        read_slate_geometry_snapshot(
            slate_library, get_geometry_local_size_, local_to_absolute_,
            geometry_schema, geometry_schema.cached_geometry_return,
            native_parameters.data(), native_geometry)
        && read_slate_geometry_snapshot(
            slate_library, get_geometry_local_size_, local_to_absolute_,
            geometry_schema, viewport_schema.return_value,
            viewport_parameters.data(), viewport_geometry);
    const auto placement = geometry_ready
        ? dswros::calculate_world_map_viewport_placement(
            {atlas_left_, atlas_top_, atlas_width_, atlas_height_},
            native_geometry, viewport_geometry)
        : std::nullopt;
    if (!placement) {
        return handle_transient_observation(
            dswros::WorldMapTransformObservationFailure::GeometryUnavailable,
            result);
    }
    bool transform_changed = !viewport_transform_valid_;
    if (viewport_transform_valid_) {
        const auto changed =
            dswros::world_map_viewport_transform_changed(
                viewport_placement_, *placement);
        if (!changed) {
            return handle_transient_observation(
                dswros::WorldMapTransformObservationFailure::GeometryUnavailable,
                result);
        }
        transform_changed = *changed;
    }
    if (transform_changed) {
        stage = dswros::WorldMapTransformSyncStage::OwnedHostApplication;
        for (auto& host_handle : hosts_) {
            UObject* host = host_handle.Get();
            VectorParameters size_parameters{{
                placement->width, placement->height}};
            host->ProcessEvent(
                set_desired_size_in_viewport_, &size_parameters);
            PositionInViewportParameters position_parameters{};
            position_parameters.position = {
                placement->left, placement->top};
            position_parameters.remove_dpi_scale = false;
            host->ProcessEvent(
                set_position_in_viewport_, &position_parameters);
        }
    }
    viewport_placement_ = *placement;
    viewport_transform_valid_ = true;
    viewport_geometry_sample_valid_ = true;
    viewport_geometry_sample_ = {
        native_geometry.absolute_left, native_geometry.absolute_top,
        native_geometry.local_width, native_geometry.local_height};
    transform_ready_ = true;
    if (!reconcile_host_visibility_unsafe(false, stage)) {
        return false;
    }
    result = transform_changed
        ? WorldMapLayeringRefreshResult::Updated
        : WorldMapLayeringRefreshResult::Unchanged;
    stage = dswros::WorldMapTransformSyncStage::None;
    return true;
}

WorldMapLayeringRefreshResult WorldMapUmgRenderer::sync_viewport_transform(
    UObject* current_layer) noexcept {
    if (state_ != WorldMapUmgRendererState::Attached) {
        return WorldMapLayeringRefreshResult::RetryLater;
    }
    const bool same_layer = attached_layer_matches(current_layer);
    const bool had_valid_transform = viewport_transform_valid_;
    WorldMapLayeringRefreshResult result{};
    volatile dswros::WorldMapTransformSyncStage stage =
        dswros::WorldMapTransformSyncStage::None;
    bool completed = false;
#if defined(_MSC_VER)
    __try {
        completed = sync_viewport_transform_unsafe(
            current_layer, result, stage);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        completed = false;
    }
#else
    try {
        completed = sync_viewport_transform_unsafe(
            current_layer, result, stage);
    } catch (...) {
        completed = false;
    }
#endif
    last_transform_sync_stage_ = stage;
    if (completed) {
        return result;
    }

    const auto failure = dswros::world_map_transform_failure_for_stage(stage);
    const bool application_hosts_still_valid =
        stage == dswros::WorldMapTransformSyncStage::OwnedHostApplication
        && validate_host_payload_guarded(current_layer);
    const auto action = stage
            == dswros::WorldMapTransformSyncStage::OwnedHostApplication
        ? dswros::classify_world_map_owned_host_application_failure(
            same_layer, had_valid_transform,
            application_hosts_still_valid)
        : dswros::classify_world_map_transform_observation_failure(
            failure, same_layer, had_valid_transform);
    const auto visibility_policy =
        dswros::world_map_transform_visibility_policy(action);
    if (visibility_policy.retain_host) {
        transform_ready_ = visibility_policy.transform_ready;
        if (stage
                == dswros::WorldMapTransformSyncStage::OwnedHostApplication
            && action
                == dswros::WorldMapTransformObservationFailureAction::RetainLastValid) {
            // Do not issue another reflected write from the exception path.
            // The prior placement remains authoritative and the caller's
            // finite settle tail will converge both hosts on a later pass.
            return WorldMapLayeringRefreshResult::Retained;
        }
        const auto observation_stage = stage;
        if (!reconcile_host_visibility_guarded(
                visibility_policy.force_collapsed, stage)) {
            last_transform_sync_stage_ = stage;
            fault_and_detach(103);
            return WorldMapLayeringRefreshResult::Faulted;
        }
        last_transform_sync_stage_ = observation_stage;
        return visibility_policy.transform_ready
            ? WorldMapLayeringRefreshResult::Retained
            : WorldMapLayeringRefreshResult::RetryLater;
    }

    fault_and_detach(103);
    return WorldMapLayeringRefreshResult::Faulted;
}

WorldMapLayeringRefreshResult WorldMapUmgRenderer::refresh_layering(
    UObject* current_layer,
    bool allow_tree_mutation,
    double player_world_x,
    double player_world_y) noexcept {
    (void)allow_tree_mutation;
    (void)player_world_x;
    (void)player_world_y;
    return sync_viewport_transform(current_layer);
}

void WorldMapUmgRenderer::set_content_visibility_intent(
    bool enabled) noexcept {
    content_visibility_intent_ = enabled;
    if (state_ != WorldMapUmgRendererState::Attached
        && state_ != WorldMapUmgRendererState::Suspended) {
        return;
    }
    volatile dswros::WorldMapTransformSyncStage stage =
        dswros::WorldMapTransformSyncStage::None;
    if (!reconcile_host_visibility_guarded(false, stage)) {
        last_transform_sync_stage_ = stage;
        fault_and_detach(104);
    }
}

void WorldMapUmgRenderer::publish_runtime_visibility(
    bool visible) noexcept {
    runtime_visibility_allowed_ = visible;
    if (state_ != WorldMapUmgRendererState::Attached
        && state_ != WorldMapUmgRendererState::Suspended) {
        return;
    }
    volatile dswros::WorldMapTransformSyncStage stage =
        dswros::WorldMapTransformSyncStage::None;
    if (!reconcile_host_visibility_guarded(false, stage)) {
        last_transform_sync_stage_ = stage;
        fault_and_detach(104);
    }
}

bool WorldMapUmgRenderer::attach_once(
    UObject* current_layer,
    UObject* expected_owning_player,
    const WorldMapUmgMarkerArray& markers,
    std::size_t marker_count,
    double player_world_x,
    double player_world_y,
    UObject* current_map_data) noexcept {
    if (!activation_active_ || attach_attempted_
        || state_ != WorldMapUmgRendererState::Ready) {
        return state_ == WorldMapUmgRendererState::Attached;
    }
    const auto attach_started = std::chrono::steady_clock::now();
    ++attach_attempt_count_;
    if (!current_layer || !expected_owning_player
        || marker_count > kWorldMapUmgMarkerCapacity
        || !std::isfinite(player_world_x)
        || !std::isfinite(player_world_y)) {
        attach_attempted_ = true;
        last_attach_failure_ = 1;
        attach_elapsed_us_ = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - attach_started).count());
        return false;
    }
    const bool attached = attach_guarded(
        current_layer, expected_owning_player, markers, marker_count,
        player_world_x, player_world_y, current_map_data);
    attach_elapsed_us_ = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - attach_started).count());
    if (dswros::world_map_attach_attempt_is_terminal(
            attached, last_attach_failure_)) {
        attach_attempted_ = true;
    }
    return attached;
}

bool WorldMapUmgRenderer::attach_guarded(
    UObject* current_layer,
    UObject* expected_owning_player,
    const WorldMapUmgMarkerArray& markers,
    std::size_t marker_count,
    double player_world_x,
    double player_world_y,
    UObject* current_map_data) noexcept {
#if defined(_MSC_VER)
    __try {
        return attach_unsafe(
            current_layer, expected_owning_player, markers, marker_count,
            player_world_x, player_world_y, current_map_data);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fault_and_detach(100);
        return false;
    }
#else
    try {
        return attach_unsafe(
            current_layer, expected_owning_player, markers, marker_count,
            player_world_x, player_world_y, current_map_data);
    } catch (...) {
        fault_and_detach(100);
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::attach_unsafe(
    UObject* current_layer,
    UObject* expected_owning_player,
    const WorldMapUmgMarkerArray& markers,
    std::size_t marker_count,
    double player_world_x,
    double player_world_y,
    UObject* current_map_data) {
    last_map_data_source_ = 0;
    player_anchor_source_ = 0;
    geometry_stability_result_ = geometry_sample_valid_
        ? dswros::WorldMapGeometryStabilityResult::Seeded
        : dswros::WorldMapGeometryStabilityResult::None;
    geometry_sample_max_delta_ = 0.0;
    runtime_visibility_allowed_ = false;
    transform_ready_ = false;
    applied_host_visibility_.reset();
    player_canvas_anchor_x_ = 0.0;
    player_canvas_anchor_y_ = 0.0;
    native_parent_width_ = 0.0;
    native_parent_height_ = 0.0;
    if (!current_layer->IsA(world_map_layer_class_)) {
        last_attach_failure_ = 2;
        return false;
    }

    UObject* retainer_box = read_object_property(current_layer, L"RetainerBox");
    UObject* map_overlay = read_object_property(current_layer, L"MapOverlay");
    UObject* map_overlay_slot =
        read_object_property(current_layer, L"MapOverlaySlot");
    UObject* player_icon =
        read_object_property(current_layer, L"PlayerIconWidget");
    if (!retainer_box || !retainer_box->IsA(retainer_box_class_)
        || !map_overlay || !map_overlay->IsA(overlay_class_)
        || !map_overlay_slot || !map_overlay_slot->IsA(canvas_panel_slot_class_)
        || !player_icon) {
        last_attach_failure_ = 3;
        return false;
    }

    ObjectReturnParameters owning_player{};
    player_icon->ProcessEvent(get_owning_player_, &owning_player);
    if (!owning_player.return_value
        || owning_player.return_value != expected_owning_player) {
        last_attach_failure_ = 4;
        return false;
    }

    GetPositionParameters overlay_position{};
    map_overlay_slot->ProcessEvent(get_slot_position_, &overlay_position);
    double overlay_scale_x{};
    double overlay_scale_y{};
    if (!std::isfinite(overlay_position.return_value.x)
        || !std::isfinite(overlay_position.return_value.y)
        || !read_nested_vector(
            map_overlay, L"RenderTransform", L"Scale",
            overlay_scale_x, overlay_scale_y)
        || overlay_scale_x < 0.05 || overlay_scale_x > 20.0
        || overlay_scale_y < 0.05 || overlay_scale_y > 20.0) {
        last_attach_failure_ = 5;
        return false;
    }

    NativeIconTemplate native_icon_template{};
    if (find_native_icon_template(
            current_layer, expected_owning_player,
            map_point_icon_class_, canvas_panel_class_,
            canvas_panel_slot_class_, get_owning_player_,
            native_icon_template) != NativeIconTemplateLookupResult::Found) {
        last_attach_failure_ = 7;
        return false;
    }

    const std::int32_t detected_map_id = detect_map_id(current_layer);
    if (detected_map_id != 100 && detected_map_id != 200) {
        last_attach_failure_ = 6;
        return false;
    }

    UObject* map_data = current_map_data;
    bool used_static_lookup = false;
    if (!map_data) {
        map_data_lookup_attempted_ = true;
        ++map_data_lookup_count_;
        const wchar_t* data_path = detected_map_id == 100
            ? kWorldMapData100 : kWorldMapData200;
        map_data = UObjectGlobals::StaticFindObject<UObject*>(
            nullptr, nullptr, data_path);
        used_static_lookup = map_data != nullptr;
    }
    if (map_data && !map_data->IsA(world_map_data_class_)) {
        last_attach_failure_ = 8;
        return false;
    }

    std::int32_t data_map_id{};
    double dimensions{};
    double ui_size{};
    if (map_data) {
        if (!read_map_data(map_data, data_map_id, dimensions, ui_size)
            || data_map_id != detected_map_id) {
            last_attach_failure_ = 9;
            return false;
        }
        cached_map_id_ = data_map_id;
        cached_map_dimensions_ = dimensions;
        cached_map_ui_size_ = ui_size;
        last_map_data_source_ = used_static_lookup ? 2U : 1U;
    } else {
        if (!valid_cached_map_data(
                cached_map_id_, cached_map_dimensions_, cached_map_ui_size_,
                detected_map_id)) {
            last_attach_failure_ = 8;
            return false;
        }
        data_map_id = cached_map_id_;
        dimensions = cached_map_dimensions_;
        ui_size = cached_map_ui_size_;
        ++map_data_cache_hit_count_;
        last_map_data_source_ = 3U;
    }

    double player_map_x{};
    double player_map_y{};
    double parent_width{};
    double parent_height{};
    if (!read_live_player_canvas_anchor(
            player_icon, native_icon_template.parent_canvas,
            canvas_panel_class_, canvas_panel_slot_class_,
            slate_blueprint_library_.Get(), get_slot_alignment_,
            get_cached_geometry_, get_geometry_local_size_,
            local_to_absolute_, absolute_to_local_, ui_size,
            player_map_x, player_map_y, parent_width, parent_height)) {
        // Geometry can legitimately be absent during the first map-layout
        // event. Failure code 24 is distinct from an invalid overlay transform
        // and remains retryable for this bounded session;
        // never publish a centered fallback that shifts 21:9 or 16:10 maps.
        reset_geometry_stability_sample();
        last_attach_failure_ = 24;
        return false;
    }
    player_anchor_source_ = 1;
    player_canvas_anchor_x_ = player_map_x;
    player_canvas_anchor_y_ = player_map_y;
    native_parent_width_ = parent_width;
    native_parent_height_ = parent_height;
    geometry_stability_result_ = dswros::observe_world_map_geometry_sample(
        geometry_sample_valid_, geometry_sample_,
        {player_map_x, player_map_y, parent_width, parent_height},
        geometry_sample_max_delta_);
    if (geometry_stability_result_
        != dswros::WorldMapGeometryStabilityResult::Stable) {
        // The first valid sample only seeds four numeric values. A changed
        // second sample replaces them, allowing the third and final existing
        // service attempt to accept only the settled coordinate space.
        last_attach_failure_ = 24;
        return false;
    }
    const dswros::Position player_world{
        player_world_x, player_world_y, 0.0};
    std::array<Vector2D, kWorldMapUmgMarkerCapacity> local_positions{};
    for (std::size_t index = 0; index < marker_count; ++index) {
        const WorldMapUmgMarker& marker = markers[index];
        if (!std::isfinite(marker.world_x) || !std::isfinite(marker.world_y)) {
            last_attach_failure_ = 10;
            return false;
        }
        const auto projected = dswros::project_world_map_point(
            player_map_x, player_map_y, player_world,
            {marker.world_x, marker.world_y, 0.0}, dimensions,
            parent_width, parent_height);
        if (!projected) {
            last_attach_failure_ = 10;
            return false;
        }
        local_positions[index] = {projected->x, projected->y};
        if (!std::isfinite(local_positions[index].x)
            || !std::isfinite(local_positions[index].y)
            || std::abs(local_positions[index].x)
                > std::max(parent_width, parent_height) * 4.0
            || std::abs(local_positions[index].y)
                > std::max(parent_width, parent_height) * 4.0) {
            last_attach_failure_ = 11;
            return false;
        }
    }

    AtlasBounds atlas_bounds{};
    std::size_t visible_marker_count{};
    if (!calculate_atlas_bounds(
            markers, local_positions, marker_count,
            atlas_bounds, visible_marker_count)
        || atlas_bounds.width > std::max(parent_width, parent_height) * 8.0
        || atlas_bounds.height > std::max(parent_width, parent_height) * 8.0) {
        last_attach_failure_ = 12;
        return false;
    }
    UClass* native_icon_class = native_icon_template.icon_class;
    UObject* native_parent = native_icon_template.parent_canvas;
    UObject* blueprint_library = widget_blueprint_library_.Get();
    UObject* rendering_library = kismet_rendering_library_.Get();
    if (!native_icon_class || !native_parent
        || !blueprint_library || !rendering_library) {
        last_attach_failure_ = 13;
        return false;
    }
    paint_owner_status_ = resolve_paint_owner_status(native_icon_class);

    std::array<AtlasBuildResult, kWorldMapAtlasLayerCount> atlas_results{};
    std::array<UObject*, kWorldMapAtlasLayerCount> atlas_textures{};
    std::array<UObject*, kWorldMapAtlasLayerCount> hosts{};
    std::array<UObject*, kWorldMapAtlasLayerCount> trees{};
    std::array<UObject*, kWorldMapAtlasLayerCount> root_panels{};
    std::array<UObject*, kWorldMapAtlasLayerCount> atlas_images{};
    std::array<UObject*, kWorldMapAtlasLayerCount> atlas_image_slots{};
    std::size_t layered_marker_count{};
    atlas_build_elapsed_us_ = 0;
    atlas_file_bytes_ = 0;
    for (std::size_t layer_index = 0;
         layer_index < kWorldMapAtlasLayerCount; ++layer_index) {
        const AtlasLayer atlas_layer = layer_index == 0
            ? AtlasLayer::Background : AtlasLayer::Foreground;
        atlas_results[layer_index] = build_rle_tga_atlas(
            atlas_cache_paths_[layer_index], markers, local_positions,
            marker_count, atlas_bounds, atlas_layer);
        const AtlasBuildResult& atlas_result = atlas_results[layer_index];
        atlas_build_elapsed_us_ += atlas_result.elapsed_us;
        atlas_file_bytes_ += atlas_result.file_bytes;
        if (!atlas_result.success) {
            last_attach_failure_ = 14;
            return false;
        }
        layered_marker_count += atlas_result.drawn_marker_count;

        ImportFileParameters import_texture{
            current_layer, atlas_cache_paths_[layer_index].c_str()};
        rendering_library->ProcessEvent(
            import_file_as_texture_, &import_texture);
        atlas_textures[layer_index] = import_texture.return_value;
        if (!atlas_textures[layer_index]) {
            last_attach_failure_ = 15;
            return false;
        }

        CreateWidgetParameters create{
            expected_owning_player, native_icon_class,
            expected_owning_player, nullptr};
        blueprint_library->ProcessEvent(create_widget_, &create);
        hosts[layer_index] = create.return_value;
        if (!hosts[layer_index]
            || !hosts[layer_index]->IsA(map_point_icon_class_)) {
            last_attach_failure_ = 16;
            return false;
        }
        ObjectReturnParameters host_owner{};
        hosts[layer_index]->ProcessEvent(get_owning_player_, &host_owner);
        if (host_owner.return_value != expected_owning_player) {
            last_attach_failure_ = 16;
            return false;
        }
        trees[layer_index] = read_object_property(
            hosts[layer_index], L"WidgetTree");
        root_panels[layer_index] = read_object_property(
            hosts[layer_index], L"Panel_Point");
        if (!trees[layer_index] || !root_panels[layer_index]
            || !root_panels[layer_index]->IsA(canvas_panel_class_)) {
            last_attach_failure_ = 17;
            return false;
        }
        root_panels[layer_index]->ProcessEvent(clear_children_, nullptr);
        set_visibility(
            root_panels[layer_index], set_visibility_, kHitTestInvisible);

        atlas_images[layer_index] = UObjectGlobals::NewObject<UObject>(
            trees[layer_index], image_class_);
        if (!atlas_images[layer_index]
            || !atlas_images[layer_index]->IsA(image_class_)) {
            last_attach_failure_ = 18;
            return false;
        }
        SetBrushFromTextureParameters brush{
            atlas_textures[layer_index], false};
        atlas_images[layer_index]->ProcessEvent(
            set_brush_from_texture_, &brush);
        if (read_struct_object_property(
                atlas_images[layer_index], L"Brush", L"ResourceObject")
            != atlas_textures[layer_index]) {
            last_attach_failure_ = 19;
            return false;
        }
        set_visibility(
            atlas_images[layer_index], set_visibility_, kHitTestInvisible);
        AddChildParameters add_image{atlas_images[layer_index]};
        root_panels[layer_index]->ProcessEvent(
            add_child_to_canvas_, &add_image);
        atlas_image_slots[layer_index] = add_image.return_value;
        if (!atlas_image_slots[layer_index]
            || !atlas_image_slots[layer_index]->IsA(
                canvas_panel_slot_class_)
            || read_object_property(
                   atlas_images[layer_index], L"Slot")
                != atlas_image_slots[layer_index]
            || read_object_property(
                   atlas_image_slots[layer_index], L"Parent")
                != root_panels[layer_index]
            || read_object_property(
                   atlas_image_slots[layer_index], L"Content")
                != atlas_images[layer_index]) {
            last_attach_failure_ = 20;
            return false;
        }
        set_slot_vector(
            atlas_image_slots[layer_index], set_slot_position_,
            0.0, 0.0);
        set_slot_vector(
            atlas_image_slots[layer_index], set_slot_size_,
            atlas_bounds.width, atlas_bounds.height);
        set_slot_vector(
            atlas_image_slots[layer_index], set_slot_alignment_, 0.0, 0.0);
        ZOrderParameters image_z{1};
        atlas_image_slots[layer_index]->ProcessEvent(
            set_slot_z_order_, &image_z);
        set_visibility(
            hosts[layer_index], set_visibility_, kCollapsed);
    }
    if (layered_marker_count != visible_marker_count) {
        last_attach_failure_ = 14;
        return false;
    }

    // Publish handles before the independent viewport hosts become live.
    // Rollback removes only Mod-owned hosts; the native map tree is read-only.
    layer_ = current_layer;
    retainer_box_ = retainer_box;
    native_parent_ = native_parent;
    atlas_left_ = atlas_bounds.left;
    atlas_top_ = atlas_bounds.top;
    atlas_width_ = atlas_bounds.width;
    atlas_height_ = atlas_bounds.height;
    for (std::size_t layer_index = 0;
         layer_index < kWorldMapAtlasLayerCount; ++layer_index) {
        hosts_[layer_index] = hosts[layer_index];
        widget_trees_[layer_index] = trees[layer_index];
        root_panels_[layer_index] = root_panels[layer_index];
        atlas_images_[layer_index] = atlas_images[layer_index];
        atlas_image_slots_[layer_index] = atlas_image_slots[layer_index];
        atlas_textures_[layer_index] = atlas_textures[layer_index];
    }
    applied_host_visibility_.reset();
    for (std::size_t layer_index = 0;
         layer_index < kWorldMapAtlasLayerCount; ++layer_index) {
        AddToViewportParameters viewport_add{kRadarMarkerZ};
        hosts[layer_index]->ProcessEvent(add_to_viewport_, &viewport_add);
        VectorParameters alignment_parameters{{0.0, 0.0}};
        hosts[layer_index]->ProcessEvent(
            set_alignment_in_viewport_, &alignment_parameters);
        set_visibility(
            hosts[layer_index], set_visibility_, kCollapsed);
    }
    applied_host_visibility_ = false;
    map_id_ = data_map_id;
    map_dimensions_ = dimensions;
    map_ui_size_ = ui_size;
    map_overlay_left_ = overlay_position.return_value.x;
    map_overlay_top_ = overlay_position.return_value.y;
    map_overlay_zoom_ = overlay_scale_x;
    active_marker_input_count_ = marker_count;
    for (std::size_t index = 0; index < marker_count; ++index) {
        active_markers_[index] = markers[index];
    }
    active_marker_count_ = layered_marker_count;
    state_ = WorldMapUmgRendererState::Attached;
    WorldMapLayeringRefreshResult sync_result{};
    volatile dswros::WorldMapTransformSyncStage sync_stage =
        dswros::WorldMapTransformSyncStage::None;
    const bool transform_synced = sync_viewport_transform_unsafe(
        current_layer, sync_result, sync_stage);
    last_transform_sync_stage_ = sync_stage;
    if (!transform_synced
        || sync_result == WorldMapLayeringRefreshResult::Faulted) {
        fault_and_detach(100);
        return false;
    }
    last_attach_failure_ = 0;
    ++attach_count_;
    return true;
}

void WorldMapUmgRenderer::suspend() noexcept {
    activation_active_ = false;
    if (state_ == WorldMapUmgRendererState::Attached) {
        (void)suspend_guarded();
    }
}

bool WorldMapUmgRenderer::suspend_guarded() noexcept {
    volatile dswros::WorldMapTransformSyncStage visibility_stage =
        dswros::WorldMapTransformSyncStage::None;
#if defined(_MSC_VER)
    __try {
        if (state_ != WorldMapUmgRendererState::Attached) {
            return state_ == WorldMapUmgRendererState::Suspended;
        }
        runtime_visibility_allowed_ = false;
        transform_ready_ = false;
        if (!validate_host_unsafe(layer_.Get())) {
            last_attach_failure_ = 22;
            detach_unsafe();
            return false;
        }
        state_ = WorldMapUmgRendererState::Suspended;
        if (!reconcile_host_visibility_unsafe(false, visibility_stage)) {
            last_transform_sync_stage_ = visibility_stage;
            fault_and_detach(101);
            return false;
        }
        ++suspend_count_;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        last_transform_sync_stage_ = visibility_stage;
        fault_and_detach(101);
        return false;
    }
#else
    try {
        if (state_ != WorldMapUmgRendererState::Attached) {
            return state_ == WorldMapUmgRendererState::Suspended;
        }
        runtime_visibility_allowed_ = false;
        transform_ready_ = false;
        if (!validate_host_unsafe(layer_.Get())) {
            last_attach_failure_ = 22;
            detach_unsafe();
            return false;
        }
        state_ = WorldMapUmgRendererState::Suspended;
        if (!reconcile_host_visibility_unsafe(false, visibility_stage)) {
            last_transform_sync_stage_ = visibility_stage;
            fault_and_detach(101);
            return false;
        }
        ++suspend_count_;
        return true;
    } catch (...) {
        last_transform_sync_stage_ = visibility_stage;
        fault_and_detach(101);
        return false;
    }
#endif
}

bool WorldMapUmgRenderer::resume_suspended(
    UObject* current_layer) noexcept {
    if (!activation_active_
        || state_ != WorldMapUmgRendererState::Suspended) {
        return state_ == WorldMapUmgRendererState::Attached;
    }
    return resume_suspended_guarded(current_layer);
}

bool WorldMapUmgRenderer::resume_suspended_guarded(
    UObject* current_layer) noexcept {
    volatile dswros::WorldMapTransformSyncStage sync_stage =
        dswros::WorldMapTransformSyncStage::None;
#if defined(_MSC_VER)
    __try {
        if (!validate_host_unsafe(current_layer)) {
            last_attach_failure_ = 23;
            detach_unsafe();
            return false;
        }
        runtime_visibility_allowed_ = false;
        transform_ready_ = false;
        state_ = WorldMapUmgRendererState::Attached;
        WorldMapLayeringRefreshResult result{};
        const bool transform_synced = sync_viewport_transform_unsafe(
            current_layer, result, sync_stage);
        last_transform_sync_stage_ = sync_stage;
        if (!transform_synced
            || result == WorldMapLayeringRefreshResult::Faulted) {
            fault_and_detach(102);
            return false;
        }
        attach_attempted_ = true;
        last_attach_failure_ = 0;
        ++resume_count_;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        last_transform_sync_stage_ = sync_stage;
        fault_and_detach(102);
        return false;
    }
#else
    try {
        if (!validate_host_unsafe(current_layer)) {
            last_attach_failure_ = 23;
            detach_unsafe();
            return false;
        }
        runtime_visibility_allowed_ = false;
        transform_ready_ = false;
        state_ = WorldMapUmgRendererState::Attached;
        WorldMapLayeringRefreshResult result{};
        const bool transform_synced = sync_viewport_transform_unsafe(
            current_layer, result, sync_stage);
        last_transform_sync_stage_ = sync_stage;
        if (!transform_synced
            || result == WorldMapLayeringRefreshResult::Faulted) {
            fault_and_detach(102);
            return false;
        }
        attach_attempted_ = true;
        last_attach_failure_ = 0;
        ++resume_count_;
        return true;
    } catch (...) {
        last_transform_sync_stage_ = sync_stage;
        fault_and_detach(102);
        return false;
    }
#endif
}

void WorldMapUmgRenderer::detach() noexcept {
    activation_active_ = false;
    detach_guarded();
}

void WorldMapUmgRenderer::abandon_runtime_handles() noexcept {
    const WorldMapUmgRendererState previous_state = state_;
    activation_active_ = false;
    reset_runtime_handles();
    if (previous_state == WorldMapUmgRendererState::Attached
        || previous_state == WorldMapUmgRendererState::Suspended
        || previous_state == WorldMapUmgRendererState::Ready) {
        state_ = WorldMapUmgRendererState::Ready;
    }
}

void WorldMapUmgRenderer::detach_guarded() noexcept {
#if defined(_MSC_VER)
    __try {
        detach_unsafe();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ++fault_count_;
        activation_active_ = false;
        reset_runtime_handles();
        state_ = WorldMapUmgRendererState::Faulted;
    }
#else
    try {
        detach_unsafe();
    } catch (...) {
        ++fault_count_;
        activation_active_ = false;
        reset_runtime_handles();
        state_ = WorldMapUmgRendererState::Faulted;
    }
#endif
}

void WorldMapUmgRenderer::detach_unsafe() {
    const WorldMapUmgRendererState previous_state = state_;
    bool removed = false;
    if (remove_from_parent_) {
        for (auto& host_handle : hosts_) {
            if (UObject* host = host_handle.Get()) {
                host->ProcessEvent(remove_from_parent_, nullptr);
                removed = true;
            }
        }
    }
    if (removed) {
        ++detach_count_;
    }
    reset_runtime_handles();
    if (previous_state == WorldMapUmgRendererState::Attached
        || previous_state == WorldMapUmgRendererState::Suspended
        || previous_state == WorldMapUmgRendererState::Ready) {
        state_ = WorldMapUmgRendererState::Ready;
    }
}

void WorldMapUmgRenderer::reset_runtime_handles() noexcept {
    runtime_visibility_allowed_ = false;
    transform_ready_ = false;
    applied_host_visibility_.reset();
    layer_ = FWeakObjectPtr{};
    retainer_box_ = FWeakObjectPtr{};
    native_parent_ = FWeakObjectPtr{};
    for (std::size_t layer_index = 0;
         layer_index < kWorldMapAtlasLayerCount; ++layer_index) {
        hosts_[layer_index] = FWeakObjectPtr{};
        widget_trees_[layer_index] = FWeakObjectPtr{};
        root_panels_[layer_index] = FWeakObjectPtr{};
        atlas_images_[layer_index] = FWeakObjectPtr{};
        atlas_image_slots_[layer_index] = FWeakObjectPtr{};
        atlas_textures_[layer_index] = FWeakObjectPtr{};
    }
    active_marker_input_count_ = 0;
    active_marker_count_ = 0;
    map_id_ = 0;
    map_dimensions_ = 0.0;
    map_ui_size_ = 0.0;
    map_overlay_left_ = 0.0;
    map_overlay_top_ = 0.0;
    map_overlay_zoom_ = 1.0;
    player_canvas_anchor_x_ = 0.0;
    player_canvas_anchor_y_ = 0.0;
    native_parent_width_ = 0.0;
    native_parent_height_ = 0.0;
    player_anchor_source_ = 0;
    reset_geometry_stability_sample();
    reset_reparent_geometry_stability_sample();
    atlas_left_ = 0.0;
    atlas_top_ = 0.0;
    atlas_width_ = 0.0;
    atlas_height_ = 0.0;
    viewport_transform_valid_ = false;
    viewport_placement_ = {};
    viewport_geometry_sample_valid_ = false;
    viewport_geometry_sample_ = {};
    last_layering_parent_changed_ = false;
    last_layering_geometry_changed_ = false;
    last_layering_previous_parent_index_ = -1;
    last_layering_current_parent_index_ = -1;
    last_layering_previous_parent_serial_ = 0;
    last_layering_current_parent_serial_ = 0;
    last_layering_previous_parent_width_ = 0.0;
    last_layering_previous_parent_height_ = 0.0;
    last_layering_current_parent_width_ = 0.0;
    last_layering_current_parent_height_ = 0.0;
}

void WorldMapUmgRenderer::reset_geometry_stability_sample() noexcept {
    geometry_sample_valid_ = false;
    geometry_sample_ = {};
    geometry_stability_result_ =
        dswros::WorldMapGeometryStabilityResult::None;
    geometry_sample_max_delta_ = 0.0;
}

void WorldMapUmgRenderer::reset_reparent_geometry_stability_sample() noexcept {
    reparent_geometry_sample_valid_ = false;
    reparent_geometry_sample_ = {};
    reparent_geometry_stability_result_ =
        dswros::WorldMapGeometryStabilityResult::None;
    reparent_geometry_sample_max_delta_ = 0.0;
    reparent_geometry_parent_index_ = -1;
    reparent_geometry_parent_serial_ = 0;
}

} // namespace dsnwr
