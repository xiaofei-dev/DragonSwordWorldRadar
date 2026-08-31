#pragma once

#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/UFunction.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FNumericProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/ScriptArrayHelper.hpp>
#include <Unreal/StableScriptMap.hpp>

namespace RC::Unreal {
enum class EEndPlayReason : std::uint8_t {
    Destroyed = 0,
    LevelTransition = 1,
    EndPlayInEditor = 2,
    RemovedFromWorld = 3,
    Quit = 4,
};
}

namespace dsnwrpr::ue4ss_compat {
[[nodiscard]] inline auto unstable_name_key(
    const RC::Unreal::FName& name) noexcept -> std::uint64_t {
    return (static_cast<std::uint64_t>(name.GetNumber()) << 32)
        | static_cast<std::uint64_t>(name.GetComparisonIndex());
}
}

#define DSNWRPR_PROPERTIES_IN_CHAIN(structure) \
    ((structure)->ForEachPropertyInChain())
#define DSNWRPR_MAP_LAYOUT(property) \
    RC::Unreal::StableCompat::make_map_layout((property))
#define DSNWRPR_NAME_KEY(name) \
    dsnwrpr::ue4ss_compat::unstable_name_key((name))
#define DSNWRPR_CLASS_NAME_KEY(object_class) \
    DSNWRPR_NAME_KEY((object_class)->GetNamePrivate())
#else
#define DSNWRPR_PROPERTIES_IN_CHAIN(structure) \
    RC::Unreal::TFieldRange<RC::Unreal::FProperty>( \
        (structure), \
        RC::Unreal::EFieldIterationFlags::IncludeSuper \
            | RC::Unreal::EFieldIterationFlags::IncludeDeprecated)
#define DSNWRPR_MAP_LAYOUT(property) ((property)->GetMapLayout())
#define DSNWRPR_NAME_KEY(name) ((name).ToUnstableInt())
#define DSNWRPR_CLASS_NAME_KEY(object_class) \
    DSNWRPR_NAME_KEY((object_class)->GetFName())
#endif
