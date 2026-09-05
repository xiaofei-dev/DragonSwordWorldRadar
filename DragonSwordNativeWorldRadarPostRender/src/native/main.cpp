#include <dswros/area_quest_visibility.hpp>
#include <dswros/compact_menu_state.hpp>
#include <dswros/compact_render_model.hpp>
#include <dswros/object_state.hpp>
#include <dswros/visibility_config.hpp>
#include <dswros/world_map_session_policy.hpp>
#include "compact_umg_renderer.hpp"
#include "native_event_log.hpp"
#include "native_save_reconciler.hpp"
#include "radar_visibility_hub.hpp"
#include "world_map_umg_renderer.hpp"

#pragma warning(push)
#pragma warning(disable : 4324)
#include <Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/TMap.hpp>
#else
#include <Unreal/Core/Containers/Map.hpp>
#endif
#pragma warning(disable : 4251 4324 5038)
#include <Input/KeyDef.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/FScriptArray.hpp>
#else
#include <Unreal/Core/Containers/ScriptArray.hpp>
#endif
#include <Unreal/FWeakObjectPtr.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealInitializer.hpp>
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
#include <Unreal/UClass.hpp>
#include <Unreal/FProperty.hpp>
#else
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#endif
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#include "ue4ss_compat.hpp"
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <windows.h>
#include <shellapi.h>

namespace {

using namespace RC;
using namespace RC::Unreal;
using Clock = std::chrono::steady_clock;
using SystemClock = std::chrono::system_clock;

constexpr auto kVersion = STR("2.2.1");
constexpr std::string_view kRuntimeLabel =
    "DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1";
constexpr auto kLocationFunction = STR("/Script/Engine.Actor:K2_GetActorLocation");
constexpr auto kIsHiddenFunction = STR("/Script/Engine.Actor:IsHidden");
constexpr auto kTreasureInteractFunction =
    STR("/Script/DS.DsAnimationProp:NetMultiExecuteInteractProp");
constexpr auto kTreasureDeathFunction =
    STR("/Script/DS.DsAnimationProp:SetDeathProcess");
constexpr auto kEncounterDeathFunction =
    STR("/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath");
constexpr auto kEncounterDeathProcessFunction =
    STR("/Script/DS.DsFieldCharacter:NetMulticastSetDeathProcess");
constexpr auto kWorldMapLayerClass = STR("/Script/DSClient.DLayerMap");
constexpr auto kWorldMapPanelClass = STR("/Script/DSClient.DPanelWorldMap");
constexpr auto kCompactLayerClass = STR("/Script/DSClient.DLayerMiniMap");
constexpr auto kPositionInterval = std::chrono::milliseconds{16};
constexpr auto kWorldMapImageFunction = STR("/Script/DSClient.DLayerMap:SetWorldMapImage");
constexpr auto kWorldMapZoomFunction =
    STR("/Script/DSClient.DPanelWorldMap:OnSliderValueChanged");
constexpr auto kWidgetIsVisibleFunction = STR("/Script/UMG.Widget:IsVisible");
constexpr auto kIsGamePausedFunction =
    STR("/Script/Engine.GameplayStatics:IsGamePaused");
constexpr auto kGameplayStaticsDefault =
    STR("/Script/Engine.Default__GameplayStatics");
constexpr auto kCurrentLanguageFunction = STR(
    "/Script/Engine.KismetInternationalizationLibrary:GetCurrentLanguage");
constexpr auto kInternationalizationLibraryDefault = STR(
    "/Script/Engine.Default__KismetInternationalizationLibrary");
constexpr auto kEngineClass = STR("/Script/Engine.Engine");
constexpr auto kGameUserSettingsClass =
    STR("/Script/DSClient.DGameUserSettings");
constexpr auto kQuestInfoFunction =
    STR("/Script/DS.DETUtil:GetQuestInfoInStandAlone");
constexpr auto kQuestUtilityDefault =
    STR("/Script/DS.Default__DETUtil");
constexpr auto kQuestBlueprintEndFunction =
    STR("/Script/DSClient.DClientQuestSystem:OnQuestBlueprintEndPlay");
constexpr auto kRenewQuestBlueprintEndFunction =
    STR("/Script/DSClient.DClientQuestSystem:OnRenewQuestBlueprintEndPlay");
constexpr auto kTaskCompleteFunction =
    STR("/Script/DS.DETTaskBaseActor:OnRecvCompleteQuest");
constexpr auto kQuestEventTriggerFunction =
    STR("/Script/DS.DETUtil:ETSendQuestEventTrigger");
constexpr auto kDiscoveryInterval = std::chrono::milliseconds{250};
constexpr auto kActivationStability = std::chrono::milliseconds{750};
constexpr auto kAreaQuestRefreshDebounce = std::chrono::milliseconds{1000};
constexpr auto kAreaQuestRefreshMaxDebounce = std::chrono::milliseconds{2000};
constexpr auto kAreaQuestCompletionWitnessWindow =
    std::chrono::seconds{10};
constexpr auto kAreaQuestCompletionProbeInterval =
    std::chrono::milliseconds{750};
constexpr std::size_t kAreaQuestCompletionProbeBudgetPerTick = 1;
constexpr auto kAreaQuestSaveConfirmationRetryDelay =
    std::chrono::seconds{15};
constexpr std::uint8_t kAreaQuestSaveConfirmationMaximumAttempts = 3U;
constexpr auto kAreaQuestTaskClassMapRetryDelay =
    std::chrono::milliseconds{750};
constexpr auto kTransitionCooldown = std::chrono::milliseconds{1500};
constexpr auto kClockCaptureDelay = std::chrono::milliseconds{2000};
constexpr std::int64_t kGameSecondsPerRealSecond = 60;
constexpr std::int64_t kSecondsPerDay = 86400;
constexpr double kTreasureObservationRadius = 800.0;
constexpr double kEncounterObservationRadius = 10000.0;
constexpr auto kDepartureGrace = std::chrono::seconds{10};
constexpr double kTownCompactRenderRadius = 12500.0;
constexpr double kFieldCompactRenderRadius = 22500.0;
constexpr double kMinimapScaleThreshold = 2.7;
// Initial UMG publication can expose the layer before its map id, owning
// player, or native icon Canvas is ready. Readiness polling and actual host
// attachment are separate bounded phases: early callbacks must never consume
// the three real attachment attempts before the map can be rendered.
constexpr std::uint32_t kWorldMapMaxServiceAttempts = 3U;
constexpr std::uint32_t kWorldMapMaxReadinessAttempts = 40U;

constexpr auto kMinimapScaleSampleInterval = std::chrono::seconds{1};
constexpr auto kCompactAttachRetryDelay = std::chrono::milliseconds{250};
constexpr auto kWorldMapServiceRetryDelay = std::chrono::milliseconds{150};
// SetWorldMapImage is the authoritative opening edge, but the native layer can
// still report IsVisible=false during the same UMG transition. Give that edge
// a short bounded window to reach its first confirmed-visible sample before a
// false sample is allowed to retire the session. Once visibility has ever
// been confirmed, the first later false sample is an authoritative close.
constexpr auto kWorldMapOpenVisibilityGrace =
    std::chrono::milliseconds{1000};
constexpr std::array kWorldMapLayeringSettleDelays{
    std::chrono::milliseconds{100},
    std::chrono::milliseconds{250},
    std::chrono::milliseconds{500},
    std::chrono::milliseconds{1000},
    std::chrono::milliseconds{1250}};
constexpr auto kVisibilityHubServiceInterval =
    std::chrono::milliseconds{50};
constexpr auto kVisibilityHubToggleDebounce =
    std::chrono::milliseconds{250};
constexpr auto kVisibilityHubOpenRetryInterval =
    std::chrono::milliseconds{250};
constexpr auto kVisibilityHubOpenPendingLifetime =
    std::chrono::seconds{15};
constexpr wchar_t kBugReportUrl[] =
    L"https://www.nexusmods.com/dragonswordawakening/mods/254?tab=posts";
constexpr std::uint32_t kCompactMaxAttachAttempts = 3U;
constexpr double kCompactRebindDistance = 1000.0;
constexpr auto kCompactRebindInterval = std::chrono::seconds{5};
constexpr double kCompactNearestSwitchAdvantage = 100.0;
constexpr std::size_t kMaximumTreasureCatalogEntries = 2500;
constexpr std::size_t kExpectedEncounterCount = 49;
constexpr std::size_t kExpectedMiniGameCount = 83;
constexpr std::size_t kExpectedAreaQuestCount = 147;
constexpr std::size_t kExpectedAreaQuestHeightProfileCount = 144;
constexpr std::size_t kExpectedAreaQuestMultiBandCount = 1;
constexpr std::size_t kAreaQuestHeightDiagnosticSlotCapacity = 8;
constexpr std::uint32_t
    kAreaQuestHeightDiagnosticMaximumEventsPerActivation = 128U;
constexpr std::size_t kEncounterCandidateProbeBudgetPerControlTick = 8;
constexpr std::size_t kBirdEggCandidateCapacity = 512;
constexpr std::size_t kBirdEggActiveCapacity = 16;
constexpr std::size_t kBirdEggPositionProbeBudgetPerControlTick = 8;
constexpr std::size_t kAreaQuestCompletionWordCount =
    (kExpectedAreaQuestCount + 63U) / 64U;
constexpr std::size_t kTreasureSaveConfirmationWordCount =
    (kMaximumTreasureCatalogEntries + 63U) / 64U;
constexpr auto kTreasureSaveConfirmationInitialDelay =
    std::chrono::seconds{15};
constexpr auto kTreasureSaveConfirmationRetryDelay =
    std::chrono::seconds{285};
constexpr std::uint8_t kTreasureSaveConfirmationMaximumAttempts = 2U;
constexpr std::uint32_t kAreaQuestTaskClassMapMaxAttempts = 2;
constexpr std::size_t kMaximumDynamicTaskRows = 4096;
constexpr std::size_t kMaximumQuestIdsPerTaskClass = 256;
constexpr std::uint64_t kDynamicQuestTaskUseType = 4;
constexpr auto kEncounterCooldown = std::chrono::hours{2};
constexpr std::size_t kAreaQuestParameterCapacity = 64;
constexpr std::size_t kQuestEventParameterCapacity = 64;
constexpr std::size_t kQuestBlueprintEndParameterCapacity = 32;
constexpr std::size_t kWidgetIsVisibleParameterCapacity = 16;
constexpr std::size_t kIsGamePausedParameterCapacity = 16;
constexpr std::size_t kCurrentLanguageParameterCapacity = 16;
constexpr std::size_t kTreasureInteractParameterCapacity = 16;

static_assert(
    dsnwr::kWorldMapUmgMarkerCapacity
        >= kMaximumTreasureCatalogEntries + kExpectedEncounterCount
            + kExpectedMiniGameCount
            + kExpectedAreaQuestCount);
static_assert(dsnwr::kCompactUmgMarkerCapacity
              <= dswros::CompactRenderModel::kMaximumSelectedTreasures);
static_assert(dsnwr::kMaximumTreasureConfirmationIds <= 64U);

extern "C" IMAGE_DOS_HEADER __ImageBase;

using ProcessShutdownProbe = BOOLEAN(NTAPI*)();

[[nodiscard]] bool pin_own_module_for_process_lifetime() noexcept {
    HMODULE module{};
    return GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&__ImageBase), &module) != FALSE;
}

std::filesystem::path mod_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path()
        / "ue4ss" / "Mods" / "DragonSwordNativeWorldRadarPostRender";
}

[[nodiscard]] ProcessShutdownProbe resolve_process_shutdown_probe() noexcept {
    const HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (!module) {
        return nullptr;
    }
    const auto procedure =
        GetProcAddress(module, "RtlDllShutdownInProgress");
    static_assert(sizeof(procedure) == sizeof(ProcessShutdownProbe));
    return procedure
        ? std::bit_cast<ProcessShutdownProbe>(procedure)
        : nullptr;
}

[[nodiscard]] bool process_shutdown_in_progress() noexcept {
    static const ProcessShutdownProbe probe =
        resolve_process_shutdown_probe();
    return probe && probe() != FALSE;
}

std::filesystem::path game_pak_mod_directory() {
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path().parent_path()
        .parent_path() / "Content" / "Paks" / "~mods";
}

#define append_log(event, detail_expression)                                  \
    do {                                                                      \
        if (dsnwr::native_event_log_enabled()) {                              \
            dsnwr::append_native_event_log((event), (detail_expression));     \
        }                                                                     \
    } while (false)

struct RadarVisibilitySettings {
    dsnwr::RadarVisibilityMaskWord masks{
        dsnwr::kDefaultRadarVisibilityMasks};
    dsnwr::AreaQuestDisplayMode area_quest_mode{
        dsnwr::AreaQuestDisplayMode::Available};
    dsnwr::AssaultDisplayMode assault_mode{
        dsnwr::AssaultDisplayMode::Current};
    dswros::HeightIndicatorMask height_indicators{
        dswros::kDefaultHeightIndicatorMask};
    dswros::RadarLanguagePreference language{
        dswros::RadarLanguagePreference::Auto};
    dswros::VisibilityConfigParseStatus config_status{
        dswros::VisibilityConfigParseStatus::Empty};
    dswros::VisibilityConfigFormat config_format{
        dswros::VisibilityConfigFormat::Sectioned};
};

enum class EngineTickProfileStage : std::size_t {
    Total,
    WorldMapLayering,
    AreaQuest,
    Activity,
    PositionAndSave,
    CompactUmg,
    Encounter,
    BirdEgg,
    ObservedObjects,
    Count,
};

constexpr std::size_t kEngineTickProfileStageCount =
    static_cast<std::size_t>(EngineTickProfileStage::Count);
static_assert(kEngineTickProfileStageCount
              == dsnwr::kNativeEngineTickProfileStageCount);
constexpr auto kEngineTickProfileInterval = std::chrono::seconds{10};
constexpr auto kEngineTickSlowLogInterval = std::chrono::seconds{1};
constexpr std::uint64_t kEngineTickSlowThresholdUs = 2'000;

struct EngineTickProfileSample {
    std::array<std::uint64_t, kEngineTickProfileStageCount> elapsed_us{};
    std::uint32_t executed_mask{};
    std::size_t area_quest_scan_index{};
    std::size_t bird_egg_active_count{};
    bool area_quest_scan_active{};
    bool compact_update_called{};
    bool discovery_called{};
    bool world_map_layering_pending{};
};

struct EngineTickProfileMetric {
    std::uint64_t calls{};
    std::uint64_t total_us{};
    std::uint64_t maximum_us{};
};

[[nodiscard]] RadarVisibilitySettings load_visibility_settings(
    const std::filesystem::path& path) noexcept {
    RadarVisibilitySettings result{};
    try {
        const std::uintmax_t size = std::filesystem::file_size(path);
        if (size > dswros::kMaximumVisibilityConfigBytes) {
            result.config_status =
                dswros::VisibilityConfigParseStatus::TooLarge;
            return result;
        }
        std::array<char, dswros::kMaximumVisibilityConfigBytes> buffer{};
        std::ifstream input{path, std::ios::binary};
        input.read(buffer.data(), static_cast<std::streamsize>(size));
        if (input.gcount() != static_cast<std::streamsize>(size)) {
            result.config_status =
                dswros::VisibilityConfigParseStatus::InvalidValue;
            return result;
        }
        const auto parsed = dswros::parse_visibility_config(
            std::string_view{buffer.data(), static_cast<std::size_t>(size)});
        result.config_status = parsed.status;
        result.config_format = parsed.format;
        if (!parsed) {
            return result;
        }
        result.masks = dsnwr::pack_radar_visibility_masks(
            dswros::compact_visibility_mask(parsed.settings),
            dswros::world_visibility_mask(parsed.settings));
        result.area_quest_mode = parsed.settings.area_quest_mode
                == dswros::VisibilityAreaQuestMode::All
            ? dsnwr::AreaQuestDisplayMode::AllUnfinished
            : dsnwr::AreaQuestDisplayMode::Available;
        result.assault_mode = parsed.settings.assault_mode
                == dswros::VisibilityAssaultMode::All
            ? dsnwr::AssaultDisplayMode::All
            : dsnwr::AssaultDisplayMode::Current;
        result.height_indicators = static_cast<dswros::HeightIndicatorMask>(
            (parsed.settings.height_treasure
                ? dswros::kHeightIndicatorTreasure : 0U)
            | (parsed.settings.height_area_quests
                ? dswros::kHeightIndicatorAreaQuest : 0U)
            | (parsed.settings.height_mole
                ? dswros::kHeightIndicatorMole : 0U));
        result.language = parsed.settings.language;
    } catch (...) {
        result.config_status = dswros::VisibilityConfigParseStatus::Empty;
    }
    return result;
}

[[nodiscard]] bool persist_visibility_settings(
    const std::filesystem::path& path,
    dsnwr::RadarVisibilityMaskWord masks,
    dsnwr::AreaQuestDisplayMode area_quest_mode,
    dsnwr::AssaultDisplayMode assault_mode,
    dswros::HeightIndicatorMask height_indicators,
    dswros::RadarLanguagePreference language) noexcept {
    try {
        const std::uint8_t compact =
            dsnwr::compact_radar_visibility_mask(masks);
        const std::uint8_t world =
            dsnwr::world_radar_visibility_mask(masks);
        dswros::VisibilityConfigSettings settings{};
        settings.radar_clock = (compact & 0x01U) != 0U;
        settings.radar_treasure = (compact & 0x02U) != 0U;
        settings.radar_boss = (compact & 0x04U) != 0U;
        settings.radar_assault = (compact & 0x08U) != 0U;
        settings.radar_mini_games = (compact & 0x10U) != 0U;
        settings.radar_area_quests = (compact & 0x20U) != 0U;
        settings.radar_bird_eggs = (compact & 0x40U) != 0U;
        settings.map_treasure = (world & 0x02U) != 0U;
        settings.map_boss = (world & 0x04U) != 0U;
        settings.map_assault = (world & 0x08U) != 0U;
        settings.map_mini_games = (world & 0x10U) != 0U;
        settings.map_area_quests = (world & 0x20U) != 0U;
        settings.area_quest_mode = area_quest_mode
                == dsnwr::AreaQuestDisplayMode::AllUnfinished
            ? dswros::VisibilityAreaQuestMode::All
            : dswros::VisibilityAreaQuestMode::Available;
        settings.assault_mode = assault_mode == dsnwr::AssaultDisplayMode::All
            ? dswros::VisibilityAssaultMode::All
            : dswros::VisibilityAssaultMode::Available;
        settings.height_treasure = dswros::height_indicator_enabled(
            height_indicators,
            dswros::HeightIndicatorCategory::Treasure);
        settings.height_area_quests = dswros::height_indicator_enabled(
            height_indicators,
            dswros::HeightIndicatorCategory::AreaQuest);
        settings.height_mole = dswros::height_indicator_enabled(
            height_indicators,
            dswros::HeightIndicatorCategory::Mole);
        settings.language = language;
        const std::string contents =
            dswros::format_visibility_config(settings);
        std::filesystem::create_directories(path.parent_path());
        std::filesystem::path temporary = path;
        temporary += L".tmp";
        {
            std::ofstream output{temporary, std::ios::binary | std::ios::trunc};
            output.write(contents.data(),
                         static_cast<std::streamsize>(contents.size()));
            if (!output) {
                return false;
            }
        }
        if (MoveFileExW(
                temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
            == FALSE) {
            std::filesystem::remove(temporary);
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

enum class EncounterKind : std::uint8_t {
    Boss,
    Assault,
};

[[nodiscard]] constexpr const char* encounter_kind_name(
    EncounterKind kind) noexcept {
    return kind == EncounterKind::Boss ? "boss" : "assault";
}

struct EncounterSpec {
    std::int64_t id{};
    std::string class_name{};
    dswros::Position position{};
    std::int32_t map_id{dswros::CompactRenderModel::kCompactMapId};
    EncounterKind kind{EncounterKind::Boss};
    bool has_time_condition{};
    std::int32_t visible_from_hour{};
    std::int32_t hidden_from_hour{};
};

enum class MiniGameKind : std::uint8_t {
    Fly,
    Mole,
    Wave,
};

struct MiniGameSpec {
    std::int64_t id{};
    std::int64_t reward_save_id{};
    std::int32_t map_id{};
    dswros::Position position{};
    double height_z{};
    bool height_trusted{};
    MiniGameKind kind{MiniGameKind::Fly};
};

struct AreaQuestSpec {
    std::int64_t id{};
    dswros::Position position{};
    dswros::AreaQuestHeightProfile height_profile{};
};

struct AreaQuestHeightDiagnosticSelection {
    std::int64_t id{};
    std::size_t catalog_index{};
    dswros::AreaQuestHeightProfile height_profile{};
    bool active{};
    bool height_enabled{};
};

struct AreaQuestHeightDiagnosticState {
    std::int64_t id{};
    std::size_t catalog_index{};
    dswros::AreaQuestHeightProfile height_profile{};
    dswros::AreaQuestHeightIndicatorShape shape{
        dswros::AreaQuestHeightIndicatorShape::Unavailable};
    bool active{};
    bool height_enabled{};
};

[[nodiscard]] bool area_quest_height_profiles_equal(
    const dswros::AreaQuestHeightProfile& left,
    const dswros::AreaQuestHeightProfile& right) noexcept {
    if (left.band_count != right.band_count) {
        return false;
    }
    for (std::size_t index = 0;
         index < dswros::kAreaQuestHeightBandCapacity; ++index) {
        if (left.bands[index].minimum_z != right.bands[index].minimum_z
            || left.bands[index].maximum_z
                != right.bands[index].maximum_z) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] constexpr const char* area_quest_height_shape_name(
    dswros::AreaQuestHeightIndicatorShape shape) noexcept {
    switch (shape) {
    case dswros::AreaQuestHeightIndicatorShape::Aligned:
        return "aligned";
    case dswros::AreaQuestHeightIndicatorShape::Above:
        return "above";
    case dswros::AreaQuestHeightIndicatorShape::Below:
        return "below";
    default:
        return "unavailable";
    }
}

enum class DynamicQuestConditionType : std::uint8_t {
    None = 0,
    QuestClear = 3,
    DynamicQuestComplete = 15,
    MonsterAlive = 19,
};

struct DynamicQuestCondition {
    std::uint8_t type{0xFF};
    std::uint32_t value1{};
    std::uint32_t value2{};
    std::int64_t linked_encounter_id{};
};

struct AreaQuestDefinition {
    std::uint32_t group_id{};
    std::uint32_t group_active_count{};
    std::uint32_t group_member_count{};
    std::uint32_t group_active_weight{};
    std::uint8_t active_timing_type{0xFF};
    DynamicQuestCondition group_condition{};
    DynamicQuestCondition accept_condition{};
    bool found{};
};

struct StaticRenderCandidate {
    std::size_t catalog_index{};
    double planar_distance_squared{};
};

[[nodiscard]] double planar_distance_squared(
    const dswros::Position& left,
    const dswros::Position& right) noexcept {
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    return dx * dx + dy * dy;
}

[[nodiscard]] dsnwr::CompactUmgMarkerKind umg_treasure_kind(
    dswros::CompactTreasureKind kind) noexcept {
    switch (kind) {
    case dswros::CompactTreasureKind::MiniGame:
        return dsnwr::CompactUmgMarkerKind::TreasureMiniGame;
    case dswros::CompactTreasureKind::Map:
        return dsnwr::CompactUmgMarkerKind::TreasureMap;
    case dswros::CompactTreasureKind::Puzzle:
        return dsnwr::CompactUmgMarkerKind::TreasurePuzzle;
    default:
        return dsnwr::CompactUmgMarkerKind::TreasureOther;
    }
}

[[nodiscard]] dsnwr::WorldMapUmgMarkerTone world_map_treasure_tone(
    dswros::CompactTreasureKind kind) noexcept {
    switch (kind) {
    case dswros::CompactTreasureKind::MiniGame:
        return dsnwr::WorldMapUmgMarkerTone::Green;
    case dswros::CompactTreasureKind::Map:
        return dsnwr::WorldMapUmgMarkerTone::Orange;
    case dswros::CompactTreasureKind::Puzzle:
        return dsnwr::WorldMapUmgMarkerTone::Blue;
    default:
        return dsnwr::WorldMapUmgMarkerTone::White;
    }
}

[[nodiscard]] dsnwr::CompactUmgMarkerKind umg_mini_game_kind(
    MiniGameKind kind) noexcept {
    switch (kind) {
    case MiniGameKind::Mole:
        return dsnwr::CompactUmgMarkerKind::Mole;
    case MiniGameKind::Wave:
        return dsnwr::CompactUmgMarkerKind::Wave;
    default:
        return dsnwr::CompactUmgMarkerKind::Fly;
    }
}

[[nodiscard]] dsnwr::WorldMapUmgMarkerKind world_map_mini_game_kind(
    MiniGameKind kind) noexcept {
    switch (kind) {
    case MiniGameKind::Mole:
        return dsnwr::WorldMapUmgMarkerKind::Mole;
    case MiniGameKind::Wave:
        return dsnwr::WorldMapUmgMarkerKind::Wave;
    default:
        return dsnwr::WorldMapUmgMarkerKind::Fly;
    }
}

struct ObservedRuntimeObject {
    FWeakObjectPtr weak{};
    dswros::EventKind kind{};
    std::int64_t id{};
    dswros::Position position{};
    std::uint32_t activation{};
    std::uint32_t epoch{};
    dswros::DisappearanceConfirmation disappearance{};
    dswros::EncounterDisappearanceConfirmation encounter_disappearance{};
    bool logical_end{};
    bool destroyed_end{};
    bool visible_seen{};
    bool recovered_from_end_play{};
    std::optional<Clock::time_point> outside_since{};
};

struct BirdEggRuntimeCandidate {
    FWeakObjectPtr weak{};
    dswros::WeakIdentity identity{};
    dswros::Position position{};
    dswros::LiveMarkerPresenceGate presence{};
    bool position_known{};
    bool retired{};
};

enum class CompactAttachGateStatus : std::uint32_t {
    NotAttempted,
    Attached,
    CurrentPlayerControllerUnavailable,
    RendererRejected,
};

enum class WorldMapLayeringTrigger : std::uint8_t {
    Attach,
    SetWorldMapImage,
    ZoomChanged,
    F7Resume,
};

enum class WorldMapLayeringArmPolicy : std::uint8_t {
    Coalesce,
    Restart,
};

[[nodiscard]] constexpr std::string_view world_map_layering_trigger_name(
    WorldMapLayeringTrigger trigger) noexcept {
    switch (trigger) {
    case WorldMapLayeringTrigger::Attach: return "attach";
    case WorldMapLayeringTrigger::SetWorldMapImage:
        return "set_world_map_image";
    case WorldMapLayeringTrigger::ZoomChanged: return "zoom_changed";
    case WorldMapLayeringTrigger::F7Resume: return "f7_resume";
    default: return "unknown";
    }
}

[[nodiscard]] constexpr std::string_view world_map_layering_result_name(
    dsnwr::WorldMapLayeringRefreshResult result) noexcept {
    switch (result) {
    case dsnwr::WorldMapLayeringRefreshResult::Updated:
        return "updated";
    case dsnwr::WorldMapLayeringRefreshResult::RetryLater:
        return "retry_later";
    case dsnwr::WorldMapLayeringRefreshResult::Retained:
        return "retained";
    case dsnwr::WorldMapLayeringRefreshResult::Faulted:
        return "faulted";
    case dsnwr::WorldMapLayeringRefreshResult::Unchanged:
        return "unchanged";
    default:
        return "unknown";
    }
}

[[nodiscard]] double distance_squared(const dswros::Position& left, const dswros::Position& right) noexcept {
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    const double dz = left.z - right.z;
    return dx * dx + dy * dy + dz * dz;
}

[[nodiscard]] std::vector<std::string> split_tab(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t begin{};
    while (begin <= line.size()) {
        const auto end = line.find('\t', begin);
        fields.push_back(line.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1U;
    }
    return fields;
}

std::vector<dswros::CatalogPoint> load_catalog(const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) throw std::runtime_error{"treasure actor catalog is missing"};
    std::vector<dswros::CatalogPoint> result;
    std::string line;
    std::getline(input, line);
    if (line != "SaveId\tClassName\tX\tY\tZ") {
        throw std::runtime_error{"treasure actor catalog header is invalid"};
    }
    while (std::getline(input, line)) {
        const auto fields = split_tab(line);
        if (fields.size() != 5U) throw std::runtime_error{"treasure actor catalog row is malformed"};
        dswros::CatalogPoint point{};
        point.id = std::stoll(fields[0]);
        point.class_name = fields[1];
        point.position = {std::stod(fields[2]), std::stod(fields[3]), std::stod(fields[4])};
        if (point.id <= 0 || point.class_name.empty()) throw std::runtime_error{"treasure actor catalog identity is invalid"};
        result.push_back(std::move(point));
    }
    if (result.size() < 1600U || result.size() > 2500U) {
        throw std::runtime_error{"treasure actor catalog count is outside the accepted range"};
    }
    return result;
}

[[nodiscard]] std::string generated_text_field(
    const std::string& line, const std::string& name) {
    const std::string prefix = name + " = \"";
    const auto begin = line.find(prefix);
    if (begin == std::string::npos) {
        throw std::runtime_error{"generated treasure field is missing: " + name};
    }
    const auto value_begin = begin + prefix.size();
    const auto end = line.find('"', value_begin);
    if (end == std::string::npos || end == value_begin) {
        throw std::runtime_error{"generated treasure text field is invalid: " + name};
    }
    return line.substr(value_begin, end - value_begin);
}

[[nodiscard]] std::string generated_number_field(
    const std::string& line, const std::string& name) {
    const std::string prefix = name + " = ";
    const auto begin = line.find(prefix);
    if (begin == std::string::npos) {
        throw std::runtime_error{"generated treasure field is missing: " + name};
    }
    const auto value_begin = begin + prefix.size();
    const auto end = line.find_first_of(", }", value_begin);
    if (end == std::string::npos || end == value_begin) {
        throw std::runtime_error{"generated treasure numeric field is invalid: " + name};
    }
    return line.substr(value_begin, end - value_begin);
}

[[nodiscard]] dswros::CompactTreasureKind treasure_kind_from_uid(
    const std::string& uid_name) noexcept {
    if (uid_name.rfind("DT_MiniGame_", 0) == 0) {
        return dswros::CompactTreasureKind::MiniGame;
    }
    if (uid_name.rfind("DT_Map_", 0) == 0) {
        return dswros::CompactTreasureKind::Map;
    }
    if (uid_name.rfind("DT_PressurePuzzle_", 0) == 0
        || uid_name.rfind("DT_StatuePuzzle_", 0) == 0) {
        return dswros::CompactTreasureKind::Puzzle;
    }
    return dswros::CompactTreasureKind::Other;
}
std::vector<dswros::CompactTreasureCatalogEntry> load_render_catalog(
    const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"world treasure render catalog is missing"};
    }
    std::vector<dswros::CompactTreasureCatalogEntry> result;
    std::unordered_set<std::int64_t> unique_ids;
    std::string line;
    while (std::getline(input, line)) {
        if (line.find("{ save_id = ") == std::string::npos) {
            continue;
        }
        const std::string section = generated_text_field(line, "section");
        const std::string uid_name = generated_text_field(line, "uid_name");
        if (section.size() < 3U) {
            throw std::runtime_error{"world treasure section is invalid"};
        }
        const auto map_id = std::stoi(section.substr(section.size() - 3U));
        dswros::CompactTreasureCatalogEntry entry{
            std::stoll(generated_number_field(line, "save_id")),
            map_id,
            {
                std::stod(generated_number_field(line, "x")),
                std::stod(generated_number_field(line, "y")),
                std::stod(generated_number_field(line, "z")),
            },
            true,
            treasure_kind_from_uid(uid_name),
        };
        if (entry.id <= 0 || entry.map_id <= 0) {
            throw std::runtime_error{"world treasure render identity is invalid"};
        }
        if (!unique_ids.insert(entry.id).second) {
            throw std::runtime_error{"world treasure render ID is duplicated"};
        }
        result.push_back(entry);
    }
    if (result.size() != 1693U) {
        throw std::runtime_error{"world treasure render catalog count is invalid"};
    }
    const auto world_count = std::count_if(
        result.begin(), result.end(), [](const auto& entry) {
            return entry.map_id == dswros::CompactRenderModel::kCompactMapId;
        });
    if (world_count != 1506) {
        throw std::runtime_error{"world treasure map-100 count is invalid"};
    }
    return result;
}

std::unordered_set<std::int64_t> load_ignored_treasure_ids(
    const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"treasure override file is unavailable"};
    }
    std::unordered_set<std::int64_t> result;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#') {
            continue;
        }
        std::istringstream fields{line.substr(first)};
        std::string operation;
        std::int64_t id{};
        std::string trailing;
        if (!(fields >> operation >> id) || operation != "ignore" || id <= 0
            || (fields >> trailing)) {
            throw std::runtime_error{"treasure override row is invalid"};
        }
        result.insert(id);
    }
    return result;
}

std::vector<EncounterSpec> load_encounter_catalog(
    const std::filesystem::path& directory) {
    std::vector<EncounterSpec> result;
    const auto load_actor_rows = [&result, &directory](
        const char* name, std::size_t expected, EncounterKind kind) {
        std::ifstream input{directory / name};
        if (!input) {
            throw std::runtime_error{std::string{name} + " is missing"};
        }
        std::string line;
        std::getline(input, line);
        if (line != "Id\tClassName\tX\tY\tZ") {
            throw std::runtime_error{
                std::string{name} + " header is invalid"};
        }
        const auto before = result.size();
        while (std::getline(input, line)) {
            const auto fields = split_tab(line);
            if (fields.size() != 5U) {
                throw std::runtime_error{
                    std::string{name} + " row is malformed"};
            }
            EncounterSpec spec{};
            spec.id = std::stoll(fields[0]);
            spec.class_name = fields[1];
            spec.position = {
                std::stod(fields[2]),
                std::stod(fields[3]),
                std::stod(fields[4])};
            spec.kind = kind;
            if (spec.id <= 0 || spec.class_name.empty()) {
                throw std::runtime_error{
                    std::string{name} + " identity is invalid"};
            }
            result.push_back(std::move(spec));
        }
        if (result.size() - before != expected) {
            throw std::runtime_error{
                std::string{name} + " count is invalid"};
        }
    };

    load_actor_rows("boss-actors.tsv", 9U, EncounterKind::Boss);
    const std::size_t assault_begin = result.size();
    load_actor_rows("assault-actors.tsv", 40U, EncounterKind::Assault);

    {
        std::ifstream input{directory / "bosses.lua"};
        if (!input) {
            throw std::runtime_error{"bosses.lua is missing"};
        }
        std::size_t rows{};
        std::string line;
        while (std::getline(input, line)) {
            if (line.find("{ boss_id = ") == std::string::npos) {
                continue;
            }
            const auto id = std::stoll(
                generated_number_field(line, "boss_id"));
            const auto found = std::find_if(
                result.begin(), result.begin()
                    + static_cast<std::ptrdiff_t>(assault_begin),
                [id](const EncounterSpec& spec) { return spec.id == id; });
            if (found == result.begin()
                    + static_cast<std::ptrdiff_t>(assault_begin)) {
                throw std::runtime_error{"boss render metadata is unmatched"};
            }
            found->map_id = std::stoi(
                generated_number_field(line, "map_id"));
            ++rows;
        }
        if (rows != 9U) {
            throw std::runtime_error{"boss render metadata count is invalid"};
        }
    }

    {
        std::ifstream input{directory / "assaults.lua"};
        if (!input) {
            throw std::runtime_error{"assaults.lua is missing"};
        }
        std::size_t rows{};
        std::size_t conditioned{};
        std::string line;
        while (std::getline(input, line)) {
            if (line.find("{ place_id = ") == std::string::npos) {
                continue;
            }
            if (rows >= 40U) {
                throw std::runtime_error{"assault render metadata overflow"};
            }
            EncounterSpec& spec = result[assault_begin + rows];
            const auto place_id = std::stoll(
                generated_number_field(line, "place_id"));
            if (spec.id != place_id) {
                throw std::runtime_error{
                    "assault actor and render catalogs are misaligned"};
            }
            spec.id = std::stoll(generated_number_field(line, "cid"));
            spec.map_id = std::stoi(
                generated_number_field(line, "map_id"));
            if (line.find("condition_type = ") != std::string::npos) {
                if (generated_text_field(line, "condition_type")
                    != "world_time_window") {
                    throw std::runtime_error{
                        "unsupported assault condition type"};
                }
                spec.has_time_condition = true;
                spec.visible_from_hour = std::stoi(
                    generated_number_field(line, "visible_from_hour"));
                spec.hidden_from_hour = std::stoi(
                    generated_number_field(line, "hidden_from_hour"));
                if (spec.visible_from_hour < 0
                    || spec.visible_from_hour > 23
                    || spec.hidden_from_hour < 0
                    || spec.hidden_from_hour > 23) {
                    throw std::runtime_error{
                        "assault time condition is invalid"};
                }
                ++conditioned;
            }
            ++rows;
        }
        if (rows != 40U || conditioned != 1U) {
            throw std::runtime_error{
                "assault render metadata shape is invalid"};
        }
    }

    std::unordered_set<std::int64_t> unique_ids;
    std::unordered_set<std::string> unique_classes;
    for (const auto& spec : result) {
        if (spec.id <= 0 || spec.map_id <= 0
            || !std::isfinite(spec.position.x)
            || !std::isfinite(spec.position.y)
            || !std::isfinite(spec.position.z)
            || !unique_ids.insert(spec.id).second
            || !unique_classes.insert(spec.class_name).second) {
            throw std::runtime_error{
                "encounter render catalog contains invalid data"};
        }
    }
    if (result.size() != kExpectedEncounterCount) {
        throw std::runtime_error{"encounter render count is invalid"};
    }
    return result;
}

std::vector<MiniGameSpec> load_mini_game_catalog(
    const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"moles.lua is missing"};
    }
    std::vector<MiniGameSpec> result;
    std::unordered_set<std::int64_t> unique_ids;
    std::size_t fly_count{};
    std::size_t mole_count{};
    std::size_t wave_count{};
    std::string line;
    while (std::getline(input, line)) {
        if (line.find("{ mini_game_id = ") == std::string::npos) {
            continue;
        }
        MiniGameSpec spec{};
        spec.id = std::stoll(
            generated_number_field(line, "mini_game_id"));
        spec.reward_save_id = std::stoll(
            generated_number_field(line, "reward_save_id"));
        spec.map_id = std::stoi(
            generated_number_field(line, "map_id"));
        spec.position = {
            std::stod(generated_number_field(line, "x")),
            std::stod(generated_number_field(line, "y")),
            std::stod(generated_number_field(line, "z")),
        };
        if (spec.id >= 11001 && spec.id <= 11034
            && spec.id != 11024) {
            spec.kind = MiniGameKind::Fly;
            ++fly_count;
        } else if (spec.id >= 12001 && spec.id <= 12040) {
            spec.kind = MiniGameKind::Mole;
            ++mole_count;
        } else if (spec.id >= 13001 && spec.id <= 13010) {
            spec.kind = MiniGameKind::Wave;
            ++wave_count;
        } else {
            throw std::runtime_error{"unsupported mini-game ID"};
        }
        const bool declared_trusted =
            line.find("height_trusted = true") != std::string::npos;
        if (declared_trusted
            && line.find("height_z = ") != std::string::npos) {
            try {
                spec.height_z = std::stod(
                    generated_number_field(line, "height_z"));
                spec.height_trusted = std::isfinite(spec.height_z)
                    && std::abs(spec.height_z - spec.position.z) <= 0.001;
            } catch (...) {
                // A malformed optional height field hides only the shared
                // mini-game pointer. The Fly/Mole/Wave marker remains.
                spec.height_z = 0.0;
                spec.height_trusted = false;
            }
        }
        if (spec.reward_save_id <= 0 || spec.map_id <= 0
            || !std::isfinite(spec.position.x)
            || !std::isfinite(spec.position.y)
            || !std::isfinite(spec.position.z)
            || !unique_ids.insert(spec.id).second) {
            throw std::runtime_error{
                "mini-game render catalog contains invalid data"};
        }
        result.push_back(spec);
    }
    if (result.size() != kExpectedMiniGameCount
        || fly_count != 33U || mole_count != 40U
        || wave_count != 10U) {
        throw std::runtime_error{"mini-game render catalog shape is invalid"};
    }
    return result;
}

std::vector<AreaQuestSpec> load_area_quest_catalog(
    const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"area quest catalog is missing"};
    }
    std::vector<AreaQuestSpec> result;
    std::unordered_set<std::int64_t> unique_ids;
    std::string line;
    std::getline(input, line);
    if (line != "Id\tX\tY\tZ\tHeight1MinZ\tHeight1MaxZ\tHeight2MinZ\tHeight2MaxZ\tHeightBandCount") {
        throw std::runtime_error{"area quest catalog header is invalid"};
    }
    std::size_t height_profile_count{};
    std::size_t multi_band_count{};
    while (std::getline(input, line)) {
        const auto fields = split_tab(line);
        if (fields.size() != 9U
            || (fields[8] != "0" && fields[8] != "1"
                && fields[8] != "2")) {
            throw std::runtime_error{"area quest catalog row is malformed"};
        }
        AreaQuestSpec spec{};
        spec.id = std::stoll(fields[0]);
        spec.position = {
            std::stod(fields[1]),
            std::stod(fields[2]),
            std::stod(fields[3]),
        };
        spec.height_profile.bands[0] = {
            std::stod(fields[4]), std::stod(fields[5])};
        spec.height_profile.bands[1] = {
            std::stod(fields[6]), std::stod(fields[7])};
        spec.height_profile.band_count = static_cast<std::uint8_t>(
            std::stoul(fields[8]));
        const bool has_height_profile =
            spec.height_profile.band_count > 0;
        const bool height_profile_valid = !has_height_profile
            || dswros::area_quest_height_profile_valid(
                spec.height_profile);
        const bool unused_bands_zero =
            (spec.height_profile.band_count >= 1
                || (spec.height_profile.bands[0].minimum_z == 0.0
                    && spec.height_profile.bands[0].maximum_z == 0.0))
            && (spec.height_profile.band_count >= 2
                || (spec.height_profile.bands[1].minimum_z == 0.0
                    && spec.height_profile.bands[1].maximum_z == 0.0));
        if (spec.id <= 0
            || !std::isfinite(spec.position.x)
            || !std::isfinite(spec.position.y)
            || !std::isfinite(spec.position.z)
            || !height_profile_valid
            || !unused_bands_zero
            || !unique_ids.insert(spec.id).second) {
            throw std::runtime_error{
                "area quest catalog contains invalid data"};
        }
        height_profile_count += has_height_profile ? 1U : 0U;
        multi_band_count +=
            spec.height_profile.band_count == 2 ? 1U : 0U;
        result.push_back(spec);
    }
    if (result.size() != kExpectedAreaQuestCount
        || height_profile_count
            != kExpectedAreaQuestHeightProfileCount
        || multi_band_count
            != kExpectedAreaQuestMultiBandCount) {
        throw std::runtime_error{"area quest catalog count is invalid"};
    }
    return result;
}

class NativeObjectState final : public CppUserModBase,
                                public FUObjectCreateListener {
public:
    NativeObjectState()
        : instance_generation_(next_instance_generation_.fetch_add(
              1, std::memory_order_relaxed) + 1U) {
        const auto directory = mod_directory();
        const bool diagnostics_enabled =
            dsnwr::load_native_event_log_enabled(
                directory / "config" / "diagnostics.ini");
        dsnwr::begin_native_event_log_session(
            directory, diagnostics_enabled);
        std::error_code font_pak_error;
        const bool incompatible_font_pak_present = std::filesystem::exists(
            game_pak_mod_directory() / "DS_HYFont_P.pak", font_pak_error);
        if (incompatible_font_pak_present) {
            append_log(
                "FONT_PAK_CONFLICT_DETECTED",
                "file=DS_HYFont_P.pak reason=replaces_game_system_fonts_without_all_supported_language_glyphs action=disable_or_replace_external_font_pak");
        }
        ModName = STR("DragonSwordNativeWorldRadarPostRender");
        ModVersion = kVersion;
        ModDescription = STR("Native object-state provider with smooth compact UMG radar and full event-driven world-map atlas");
        ModAuthors = STR("DragonSword mod workspace");
        ModIntendedSDKVersion = STR("3.0.1");
        instance_.store(this, std::memory_order_release);
        const RadarVisibilitySettings visibility = load_visibility_settings(
            mod_directory() / "config" / "visibility.ini");
        visibility_masks_ = visibility.masks;
        publish_world_map_content_visibility_intent();
        area_quest_display_mode_ = visibility.area_quest_mode;
        assault_display_mode_ = visibility.assault_mode;
        height_indicator_mask_ = visibility.height_indicators;
        language_preference_ = visibility.language;
        active_ui_language_ = dswros::resolve_radar_ui_language(
            language_preference_, detected_game_language_);
        try {
            auto treasure_catalog = load_catalog(
                mod_directory() / "data" / "generated" / "treasure-actors.tsv");
            auto encounter_catalog = load_encounter_catalog(
                mod_directory() / "data" / "generated");
            auto render_catalog = load_render_catalog(
                mod_directory() / "data" / "generated" / "treasures.lua");
            auto mini_game_catalog = load_mini_game_catalog(
                mod_directory() / "data" / "generated" / "moles.lua");
            auto area_quest_catalog = load_area_quest_catalog(
                mod_directory() / "data" / "generated"
                    / "area-quests.tsv");
            if (render_catalog.size() > render_catalog_entries_.size()) {
                throw std::runtime_error{
                    "world treasure render catalog exceeds fixed storage"};
            }
            for (std::size_t index = 0; index < render_catalog.size();
                 ++index) {
                render_catalog_entries_[index] = render_catalog[index];
            }
            if (!compact_render_model_.initialize(render_catalog)) {
                throw std::runtime_error{
                    "compact treasure render catalog initialization failed"};
            }
            render_catalog_size_ = render_catalog.size();
            compact_eligibility_.fill(0);
            ignored_treasure_ids_ = load_ignored_treasure_ids(
                mod_directory() / "data" / "defaults"
                    / "treasure_overrides.txt");
            tracker_.set_catalog(std::move(treasure_catalog));
            encounter_catalog_ = std::move(encounter_catalog);
            encounter_class_indices_.reserve(encounter_catalog_.size());
            for (std::size_t index = 0; index < encounter_catalog_.size();
                 ++index) {
                if (!encounter_class_indices_.emplace(
                        encounter_catalog_[index].class_name, index).second) {
                    throw std::runtime_error{
                        "encounter class index contains duplicate data"};
                }
            }
            mini_game_catalog_ = std::move(mini_game_catalog);
            mini_game_eligibility_.fill(0);
            area_quest_catalog_ = std::move(area_quest_catalog);
            area_quest_eligibility_.fill(0);
            area_quest_world_map_eligibility_.fill(0);
            area_quest_save_completion_.fill(0);
            area_quest_save_completion_counts_.fill(
                dswros::kUnknownAreaQuestSaveCompletionCount);
            area_quest_completion_generation_locked_.fill(false);
            area_quest_completion_generation_reactivation_armed_.fill(false);
            area_quest_states_.fill(dswros::AreaQuestState::Unknown);
            area_quest_scan_previous_states_.fill(
                dswros::AreaQuestState::Unknown);
            area_quest_definitions_.fill({});
            area_quest_static_proofs_.fill(
                dswros::AreaQuestEligibilityProof::Unknown);
            catalog_ready_ = true;
        } catch (const std::exception& exception) {
            append_log("CATALOG_DISABLED", exception.what());
        }
        save_reconciler_ready_ = save_reconciler_.initialize(mod_directory());
        if (!save_reconciler_ready_) {
            append_log("SAVE_RECONCILE_DISABLED", "worker_initialization_failed");
        }
        append_log("START", std::format(
            "version=2.2.1 runtime_label={} treasure_catalog={} compact_catalog={} world_map_capacity={} encounter_catalog={} mini_game_catalog={} area_quest_catalog={} area_quest_height_catalog=actor_position_data_144_profiles_1_multiband_3_missing_move_check_trigger_filtered mini_game_height_catalog=actor_position_data_exact_npc_start_83_trusted ignored_treasures={} bird_egg_classes=2 bird_egg_candidate_capacity={} bird_egg_active_capacity={} bird_egg_position_budget={} bird_egg_active_interval_ms=250 bird_egg_active_schedule=shared_discovery_edge bird_egg_missing_debounce_ms=400 bird_egg_availability=owned_interact_component_exact_state bird_egg_world_map=false save_reconciler={} main_menu_owner_boundary=exact_title_map_requires_explicit_open_world_f7 compact_menu_suppression=cursor_or_world_map_visible_or_game_paused shared_probe_ms=250 edge_logging_only runtime_diagnostics=startup_config_once config_debug_logging={} log_schema=2 visibility_config_status={} visibility_config_format={} visibility_config_read=startup_once compact_visibility_mask={} world_visibility_mask={} area_quest_mode={} assault_mode={} height_mask={} language_preference={}",
            kRuntimeLabel, tracker_.catalog_count(), render_catalog_size_,
            dsnwr::kWorldMapUmgMarkerCapacity, encounter_catalog_.size(),
            mini_game_catalog_.size(), area_quest_catalog_.size(),
            ignored_treasure_ids_.size(), kBirdEggCandidateCapacity,
            kBirdEggActiveCapacity,
            kBirdEggPositionProbeBudgetPerControlTick,
            save_reconciler_ready_,
            diagnostics_enabled,
            dswros::visibility_config_status_name(visibility.config_status),
            dswros::visibility_config_format_name(visibility.config_format),
            static_cast<std::uint32_t>(
                dsnwr::compact_radar_visibility_mask(visibility_masks_)),
            static_cast<std::uint32_t>(
                dsnwr::world_radar_visibility_mask(visibility_masks_)),
            area_quest_display_mode_
                    == dsnwr::AreaQuestDisplayMode::AllUnfinished
                ? "all"
                : "available",
            assault_display_mode_ == dsnwr::AssaultDisplayMode::All
                ? "all"
                : "current",
            static_cast<std::uint32_t>(height_indicator_mask_),
            dswros::radar_language_preference_id(language_preference_)));
    }

    ~NativeObjectState() override {
        shutdown_for_process_lifetime();
    }

    void shutdown_for_process_lifetime() noexcept {
        shutting_down_.store(true, std::memory_order_release);
        required_runtime_ready_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(
            expected, nullptr, std::memory_order_acq_rel);

        const DWORD known_game_thread =
            game_thread_id_.load(std::memory_order_acquire);
        const bool process_shutdown = process_shutdown_in_progress();
        const bool live_game_thread_cleanup =
            known_game_thread != 0
            && known_game_thread == GetCurrentThreadId()
            && !uobject_array_shutdown_.load(std::memory_order_acquire)
            && !process_shutdown
            && UnrealInitializer::StaticStorage::bIsInitialized;
        // Process teardown may already own the loader/CRT shutdown path. Do
        // not enqueue, flush, join, close, or otherwise touch log state here.
        // The module and object are pinned, and all external ingress is closed
        // by the atomics above.
        if (process_shutdown) {
            return;
        }
        if (!live_game_thread_cleanup) {
            if (!shutdown_deferred_logged_.exchange(
                    true, std::memory_order_acq_rel)) {
                append_log(
                    "SHUTDOWN_DEFERRED",
                    "reason=late_or_non_game_thread uobject_access=false");
            }
            return;
        }
        if (shutdown_started_.exchange(true, std::memory_order_acq_rel)) {
            return;
        }

        // From here through the detach calls we are on the known GameThread,
        // before UObject-array/process teardown, with the UE registry live.
        // Close the global tick routes first, then drain the create listener,
        // then touch ordinary renderer/gameplay state.
        enabled_ = false;
        engine_tick_engine_ = nullptr;
        unregister_callbacks();
        unregister_object_create_listener(true);
        wait_for_object_create_listener_callbacks();
        clear_world_map_listener_candidate();
        clear_compact_listener_candidate();
        clear_created_encounter_candidates();
        clear_bird_egg_candidates();
        pending_encounter_death_mask_.store(0, std::memory_order_release);
        pending_encounter_death_process_mask_.store(
            0, std::memory_order_release);
        visibility_hub_.detach();
        compact_umg_renderer_.detach();
        world_map_umg_renderer_.detach();
        save_reconciler_.shutdown();
        append_log(
            "SHUTDOWN_COMPLETE",
            "umg=detached hooks=unregistered listener=drained worker=joined");
        dsnwr::flush_native_event_log();
    }

    void on_unreal_init() override {
        compact_umg_renderer_.initialize();
        world_map_umg_renderer_.initialize(mod_directory() / "runtime" / "cache" / "world-map-treasure-atlas.tga");
        visibility_hub_.initialize(
            mod_directory() / "assets" / "ui" / "f6");
        location_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kLocationFunction);
        is_hidden_function_ = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, kIsHiddenFunction);
        treasure_interact_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kTreasureInteractFunction);
        treasure_death_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kTreasureDeathFunction);
        encounter_death_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kEncounterDeathFunction);
        encounter_death_process_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kEncounterDeathProcessFunction);
        encounter_death_process_schema_ready_ =
            initialize_encounter_death_process_schema();
        treasure_interact_schema_ready_ =
            initialize_treasure_interact_schema();
        world_map_layer_class_ = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, kWorldMapLayerClass);
        world_map_panel_class_ = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, kWorldMapPanelClass);
        compact_layer_class_ = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, kCompactLayerClass);
        actor_class_ = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, STR("/Script/Engine.Actor"));
        monster_character_class_ = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, STR("/Script/DS.DsMonsterCharacter"));
        initialize_encounter_class_name_keys();
        initialize_bird_egg_class_name_keys();
        world_map_image_function_ = UObjectGlobals::StaticFindObject<UFunction*>(
            nullptr, nullptr, kWorldMapImageFunction);
        world_map_zoom_function_ = UObjectGlobals::StaticFindObject<UFunction*>(
            nullptr, nullptr, kWorldMapZoomFunction);
        world_map_panel_layer_property_ = CastField<FObjectPropertyBase>(
            world_map_panel_class_
                ? world_map_panel_class_->GetPropertyByNameInChain(L"LayerMap")
                : nullptr);
        world_map_zoom_schema_ready_ = world_map_panel_class_
            && world_map_zoom_function_ && world_map_panel_layer_property_
            && world_map_panel_layer_property_->GetOffset_Internal() >= 0
            && world_map_panel_layer_property_->GetSize()
                == static_cast<int32>(sizeof(UObject*));
        widget_is_visible_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kWidgetIsVisibleFunction);
        widget_is_visible_schema_ready_ =
            initialize_widget_is_visible_schema();
        is_game_paused_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kIsGamePausedFunction);
        gameplay_statics_default_ =
            UObjectGlobals::StaticFindObject<UObject*>(
                nullptr, nullptr, kGameplayStaticsDefault);
        game_pause_schema_ready_ = initialize_game_pause_schema();
        current_language_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kCurrentLanguageFunction);
        internationalization_library_default_ =
            UObjectGlobals::StaticFindObject<UObject*>(
                nullptr, nullptr, kInternationalizationLibraryDefault);
        current_language_schema_ready_ =
            initialize_current_language_provider();
        area_quest_state_provider_ready_ =
            initialize_area_quest_state_provider();
        quest_event_trigger_schema_ready_ =
            initialize_quest_event_trigger_provider();
        quest_blueprint_end_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kQuestBlueprintEndFunction);
        renew_quest_blueprint_end_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kRenewQuestBlueprintEndFunction);
        quest_blueprint_end_schema_ready_ =
            initialize_quest_blueprint_end_schema(
                quest_blueprint_end_function_,
                quest_blueprint_end_actor_property_, "quest_end");
        renew_quest_blueprint_end_schema_ready_ =
            initialize_quest_blueprint_end_schema(
                renew_quest_blueprint_end_function_,
                renew_quest_blueprint_end_actor_property_,
                "renew_quest_end");
        task_complete_function_ = UObjectGlobals::StaticFindObject<UFunction*>(
            nullptr, nullptr, kTaskCompleteFunction);
        const auto generation = instance_generation_;
        if (world_map_layer_class_ || compact_layer_class_ || actor_class_) {
            try {
                UObjectArray::AddUObjectCreateListener(this);
                object_create_listener_registered_.store(
                    true, std::memory_order_release);
            } catch (...) {
                append_log(
                    "OBJECT_CREATE_LISTENER_DISABLED",
                    "registration_failed");
            }
        } else {
            append_log(
                "OBJECT_CREATE_LISTENER_DISABLED",
                "map_layer_and_actor_classes_missing");
        }
        if (treasure_interact_function_
            && treasure_interact_schema_ready_
            && treasure_interact_function_->HasAnyFunctionFlags(FUNC_Native)) {
            try {
                treasure_interact_hook_ids_ = UObjectGlobals::RegisterHook(
                    treasure_interact_function_,
                    [generation](
                        UnrealScriptFunctionCallableContext& context,
                        void*) {
                        if (auto* self = current(generation)) {
                            self->treasure_interact_pre(context);
                        }
                    },
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    nullptr);
                treasure_interact_hook_registered_ = true;
                append_log("TREASURE_EVENT_HOOK_READY", std::format(
                    "function={} evidence=local_net_multi_execute_interact_prop receiver=exact_ds_animation_prop_actor actor_parameter=fresh_local_pawn_or_exact_current_mount_rider",
                    to_string(treasure_interact_function_->GetFullName())));
            } catch (...) {
                append_log("TREASURE_EVENT_HOOK_DISABLED",
                    "evidence=local_net_multi_execute_interact_prop reason=registration_failed");
            }
        } else {
            append_log("TREASURE_EVENT_HOOK_DISABLED", std::format(
                "evidence=local_net_multi_execute_interact_prop reason={}",
                !treasure_interact_function_ ? "function_missing"
                : (!treasure_interact_schema_ready_
                    ? "parameter_schema_unavailable"
                    : "non_native_function")));
        }
        if (treasure_death_function_
            && treasure_death_function_->HasAnyFunctionFlags(FUNC_Native)) {
            try {
                treasure_death_hook_ids_ = UObjectGlobals::RegisterHook(
                    treasure_death_function_,
                    [generation](
                        UnrealScriptFunctionCallableContext& context,
                        void*) {
                        if (auto* self = current(generation)) {
                            self->treasure_death_pre(context);
                        }
                    },
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    nullptr);
                treasure_death_hook_registered_ = true;
                append_log("TREASURE_EVENT_HOOK_READY", std::format(
                    "function={} evidence=set_death_process receiver=exact_ds_animation_prop_actor",
                    to_string(treasure_death_function_->GetFullName())));
            } catch (...) {
                append_log("TREASURE_EVENT_HOOK_DISABLED",
                    "evidence=set_death_process reason=registration_failed");
            }
        } else {
            append_log("TREASURE_EVENT_HOOK_DISABLED", std::format(
                "evidence=set_death_process reason={}",
                treasure_death_function_
                    ? "non_native_function" : "function_missing"));
        }
        if (encounter_death_function_ && monster_character_class_
            && encounter_death_function_->HasAnyFunctionFlags(FUNC_Native)) {
            try {
                encounter_death_hook_ids_ = UObjectGlobals::RegisterHook(
                    encounter_death_function_,
                    [generation](
                        UnrealScriptFunctionCallableContext& context,
                        void*) {
                        if (auto* self = current(generation)) {
                            self->encounter_death_pre(context);
                        }
                    },
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    nullptr);
                encounter_death_hook_registered_ = true;
                append_log("ENCOUNTER_DEATH_HOOK_READY", std::format(
                    "function={} evidence=net_multicast_notify_death receiver=exact_ds_field_character_actor lookup=observed_weak_identity_o1",
                    to_string(encounter_death_function_->GetFullName())));
            } catch (...) {
                append_log("ENCOUNTER_DEATH_HOOK_DISABLED",
                    "evidence=net_multicast_notify_death reason=registration_failed");
            }
        } else {
            append_log("ENCOUNTER_DEATH_HOOK_DISABLED", std::format(
                "evidence=net_multicast_notify_death reason={}",
                !encounter_death_function_ ? "function_missing"
                : (!monster_character_class_
                    ? "monster_character_class_missing"
                    : "non_native_function")));
        }
        if (encounter_death_process_function_ && monster_character_class_
            && encounter_death_process_schema_ready_
            && encounter_death_process_function_->HasAnyFunctionFlags(
                FUNC_Native)) {
            try {
                encounter_death_process_hook_ids_ =
                    UObjectGlobals::RegisterHook(
                        encounter_death_process_function_,
                        [generation](
                            UnrealScriptFunctionCallableContext& context,
                            void*) {
                            if (auto* self = current(generation)) {
                                self->encounter_death_process_pre(context);
                            }
                        },
                        [](UnrealScriptFunctionCallableContext&, void*) {},
                        nullptr);
                encounter_death_process_hook_registered_ = true;
                append_log("ENCOUNTER_DEATH_PROCESS_HOOK_READY", std::format(
                    "function={} accepted_state=end receiver=exact_ds_field_character_actor lookup=observed_weak_identity_o1",
                    to_string(
                        encounter_death_process_function_->GetFullName())));
            } catch (...) {
                append_log("ENCOUNTER_DEATH_PROCESS_HOOK_DISABLED",
                    "reason=registration_failed");
            }
        } else {
            append_log("ENCOUNTER_DEATH_PROCESS_HOOK_DISABLED", std::format(
                "reason={}",
                !encounter_death_process_function_ ? "function_missing"
                : (!monster_character_class_
                    ? "monster_character_class_missing"
                    : (!encounter_death_process_schema_ready_
                        ? "parameter_schema_unavailable"
                        : "non_native_function"))));
        }
        if (world_map_image_function_) {
            try {
                world_map_image_hook_ids_ = UObjectGlobals::RegisterHook(
                    world_map_image_function_,
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    [generation](UnrealScriptFunctionCallableContext& context, void*) {
                        if (auto* self = current(generation)) {
                            self->world_map_image_post(context);
                        }
                    }, nullptr);
                world_map_image_hook_registered_ = true;
            } catch (...) {
                append_log("WORLD_MAP_HOOK_DISABLED", "registration_failed");
            }
        } else {
            append_log("WORLD_MAP_HOOK_DISABLED", "function_missing");
        }
        if (world_map_zoom_schema_ready_) {
            try {
                world_map_zoom_hook_ids_ = UObjectGlobals::RegisterHook(
                    world_map_zoom_function_,
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    [generation](
                        UnrealScriptFunctionCallableContext& context, void*) {
                        if (auto* self = current(generation)) {
                            self->world_map_zoom_post(context);
                        }
                    }, nullptr);
                world_map_zoom_hook_registered_ = true;
                append_log("WORLD_MAP_ZOOM_HOOK_READY", std::format(
                    "function={} receiver_layer=exact_retained_layer schedule=bounded_debounced_layering_settle",
                    to_string(world_map_zoom_function_->GetFullName())));
            } catch (...) {
                append_log(
                    "WORLD_MAP_ZOOM_HOOK_DISABLED",
                    "reason=registration_failed");
            }
        } else {
            append_log("WORLD_MAP_ZOOM_HOOK_DISABLED", std::format(
                "reason={}",
                !world_map_zoom_function_ ? "function_missing"
                : (!world_map_panel_class_ ? "panel_class_missing"
                : "layer_property_schema_invalid")));
        }
        const auto register_quest_refresh_hook =
            [this, generation](
                UFunction* function,
                std::pair<int, int>& ids,
                bool& registered,
                const char* label,
                FObjectPropertyBase* end_actor_property,
                bool schema_ready,
                bool renew) {
                if (!function || !area_quest_state_provider_ready_
                    || !schema_ready || !end_actor_property) {
                    append_log("AREA_QUEST_REFRESH_HOOK_DISABLED",
                        std::format("hook={} reason={}", label,
                            !function ? "function_missing"
                            : (!area_quest_state_provider_ready_
                                ? "state_provider_unavailable"
                                : "parameter_schema_unavailable")));
                    return;
                }
                try {
                    ids = UObjectGlobals::RegisterHook(
                        function,
                        [generation, renew](
                            UnrealScriptFunctionCallableContext& context,
                            void*) {
                            if (auto* self = current(generation)) {
                                self->quest_blueprint_end_pre(
                                    context, renew);
                            }
                        },
                        [generation](
                            UnrealScriptFunctionCallableContext&, void*) {
                            if (auto* self = current(generation)) {
                                self->area_quest_rescan_requests_.fetch_add(
                                    1, std::memory_order_release);
                            }
                        }, nullptr);
                    registered = true;
                } catch (...) {
                    append_log("AREA_QUEST_REFRESH_HOOK_DISABLED",
                        std::format(
                            "hook={} reason=registration_failed", label));
                }
            };
        register_quest_refresh_hook(
            quest_blueprint_end_function_, quest_blueprint_end_hook_ids_,
            quest_blueprint_end_hook_registered_, "quest_end",
            quest_blueprint_end_actor_property_,
            quest_blueprint_end_schema_ready_, false);
        register_quest_refresh_hook(
            renew_quest_blueprint_end_function_,
            renew_quest_blueprint_end_hook_ids_,
            renew_quest_blueprint_end_hook_registered_, "renew_quest_end",
            renew_quest_blueprint_end_actor_property_,
            renew_quest_blueprint_end_schema_ready_, true);
        if (quest_event_trigger_schema_ready_) {
            try {
                quest_event_trigger_hook_ids_ = UObjectGlobals::RegisterHook(
                    quest_event_trigger_function_,
                    [generation](
                        UnrealScriptFunctionCallableContext& context,
                        void*) {
                        if (auto* self = current(generation)) {
                            self->quest_event_trigger_pre(context);
                        }
                    },
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    nullptr);
                quest_event_trigger_hook_registered_ = true;
            } catch (...) {
                append_log(
                    "AREA_QUEST_EVENT_HOOK_DISABLED",
                    "reason=registration_failed");
            }
        }
        if (task_complete_function_ && area_quest_state_provider_ready_
            && task_complete_function_->HasAnyFunctionFlags(FUNC_Native)) {
            try {
                task_complete_hook_ids_ = UObjectGlobals::RegisterHook(
                    task_complete_function_,
                    [](UnrealScriptFunctionCallableContext&, void*) {},
                    [generation](
                        UnrealScriptFunctionCallableContext& context, void*) {
                        if (auto* self = current(generation)) {
                            self->task_complete_post(context);
                        }
                    }, nullptr);
                task_complete_hook_registered_ = true;
            } catch (...) {
                append_log(
                    "AREA_QUEST_COMPLETION_HOOK_DISABLED",
                    "reason=registration_failed");
            }
        } else {
            append_log(
                "AREA_QUEST_COMPLETION_HOOK_DISABLED",
                !task_complete_function_
                    ? "reason=function_missing"
                    : (!area_quest_state_provider_ready_
                        ? "reason=state_provider_unavailable"
                        : "reason=non_native_global_blueprint_hook_avoided"));
        }
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
        Hook::RegisterProcessEventPreCallback(
            [generation](UObject* context, UFunction*, void*) {
                if (auto* self = current(generation)) {
                    self->stable_process_event_pulse(context);
                }
            });
        stable_process_event_registered_ = true;
        Hook::RegisterBeginPlayPostCallback(
            [generation](AActor* actor) {
                if (auto* self = current(generation)) self->actor_begin(actor);
            });
        stable_begin_play_registered_ = true;
        Hook::RegisterInitGameStatePreCallback(
            [generation](AGameModeBase*) {
                if (auto* self = current(generation)) self->transition_begin();
            });
        Hook::RegisterInitGameStatePostCallback(
            [generation](AGameModeBase* game_mode) {
                if (auto* self = current(generation)) self->transition_end(game_mode);
            });
        stable_transition_callbacks_registered_ = true;
#else
        engine_tick_id_ = Hook::RegisterEngineTickPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, UEngine* engine, float, bool) {
                if (auto* self = current(generation)) self->engine_tick(engine);
            }, {false, false, STR("DragonSwordNativeWorldRadarPostRender"), STR("NativeState")});
        begin_play_id_ = Hook::RegisterBeginPlayPostCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor) {
                if (auto* self = current(generation)) self->actor_begin(actor);
            }, {false, false, STR("DragonSwordNativeWorldRadarPostRender"), STR("ActorBegin")});
        end_play_id_ = Hook::RegisterEndPlayPreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AActor* actor, EEndPlayReason reason) {
                if (auto* self = current(generation)) self->actor_end(actor, reason);
            }, {false, false, STR("DragonSwordNativeWorldRadarPostRender"), STR("ActorEnd")});
        transition_pre_id_ = Hook::RegisterInitGameStatePreCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase*) {
                if (auto* self = current(generation)) self->transition_begin();
            }, {false, false, STR("DragonSwordNativeWorldRadarPostRender"), STR("Transition")});
        transition_post_id_ = Hook::RegisterInitGameStatePostCallback(
            [generation](Hook::TCallbackIterationData<void>&, AGameModeBase* game_mode) {
                if (auto* self = current(generation)) self->transition_end(game_mode);
            }, {false, false, STR("DragonSwordNativeWorldRadarPostRender"), STR("Transition")});
#endif
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F7, [generation] {
            if (auto* self = current(generation)) self->f7_requests_.fetch_add(1, std::memory_order_release);
        });
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F8, [generation] {
            if (auto* self = current(generation)) self->f8_requests_.fetch_add(1, std::memory_order_release);
        });
        UE4SSProgram::get_program().register_keydown_event(Input::Key::F6, [generation] {
            if (auto* self = current(generation)) self->f6_requests_.fetch_add(1, std::memory_order_release);
        });
        const bool required_runtime_ready = catalog_ready_
            && location_function_ && actor_class_
            && world_map_layer_class_ && compact_layer_class_
            && object_create_listener_registered_.load(
                std::memory_order_acquire)
            && encounter_class_name_keys_ready_
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
            && stable_process_event_registered_
            && stable_begin_play_registered_
            && stable_transition_callbacks_registered_;
#else
            && engine_tick_id_ != Hook::ERROR_ID
            && begin_play_id_ != Hook::ERROR_ID
            && end_play_id_ != Hook::ERROR_ID
            && transition_pre_id_ != Hook::ERROR_ID
            && transition_post_id_ != Hook::ERROR_ID;
#endif
        required_runtime_ready_.store(
            required_runtime_ready, std::memory_order_release);
        if (!required_runtime_ready) {
            enabled_ = false;
            unregister_object_create_listener();
            append_log("DISABLED", "required native metadata or callback is unavailable");
            return;
        }
        append_log("READY", std::format("hotkeys=F6,F7,F8 coordinate_ms=16 required_runtime_ready=true treasure_actor_hooks={}_{} encounter_death_hooks={}_{} treasure_interact_schema={} treasure_completion=local_interactor_or_exact_current_mount_rider_exact_receiver_or_nearby_nonpawn_exact_id_delayed_positive_save_confirmation_or_set_death_process world_map_hook={} world_map_zoom_hook={} world_map_visibility_provider={} object_create_listener={} compact_layer_class={} area_quest_provider={} area_quest_hooks={}_{} area_quest_end_schemas={}_{} area_quest_completion_hook={} area_quest_event_hook={} area_quest_scan=one_id_per_frame_event_driven area_quest_refresh=one_second_debounced_transactional_preserve_last_complete area_quest_completion=exact_catalog_dynamic_event_or_exact_task_actor_then_ten_second_exact_id_end_probe_then_three_bounded_positive_only_save_attempts area_quest_store=dynamic area_quest_definition_snapshot=f7_game_db_main_group_numeric_only area_quest_monster_alive=unique_bounded_assault_numeric_link area_quest_time_refresh=first_valid_and_world_hour_edge_transactional_runtime_rescan area_quest_compact=nearby_prerequisite_proven_plus_runtime_marker_z_nearest_height_band area_quest_world_map=one_shot_main_group_prerequisite_proof_plus_runtime area_quest_triggerability=fail_closed_main_group_conditions discovery=event_driven_fixed_49_weak_slots_no_enumeration_8_position_queries_per_control_tick bird_egg_discovery=exact_Bird_Egg01_C_or_Bird_Egg02_C_event_driven_fixed_512_weak_slots_no_enumeration_8_interact_component_position_queries_per_250ms bird_egg_active=nearest_16_250ms_shared_discovery_edge_exact_interact_component_or_weak_missing_400ms_debounce_minimap_only encounter_identity=exact_unique_class_player_to_current_actor_within_100m encounter_completion=exact_observed_notify_death_or_death_process_end_plus_strict_nearby_ten_second_missing_fallback encounter_end_recovery=exact_destroyed_class_player_to_current_actor world_map_runtime_delta=current_session_only_if_exact_visible_else_set_world_map_image_deferred world_map_readiness=set_world_map_image_one_shot_serial_matched_budget_rearm encounter_edges=250ms_control_1hz_scalar_49_only_at_hour_or_cooldown_edge sql=native_one_shot_per_activation_plus_event_driven_encounter_dynamic_and_exact_treasure_confirmation visibility_hub=f6_transient_native_umg_auto_apply_change_only_titlebar_bug_report_status_signal_action_keeps_open_x_close_cursor_reassert_open_only markers=treasure_boss_assault_fly_mole_wave_area_quest_bird_egg marker_capacity=80 nearest_treasure_size=22 normal_treasure_size=14 compact_encounter_sizes=30_27 compact_encounter_style=four_piece_official_reference compact_area_quest_style=translucent_charcoal_rounded_brush_thick_dark_frame_three_white_dots nearest_height=sharp_tangent_six_piece_pointer_larger_tighter clock=native_scalar_transparent_thick_seven_segment_lower_crescent_star_minute_edge render_motion=one_single_host_canvas_translation umg_projection=dpi_logical_units minimap_projection=live_scale_1hz compact_layering=single_proven_viewport_host compact_attach=event_candidate_plus_one_bounded_startup_catchup_distinct_replacement_rearm compact_transition_hide=first_invalid_position_sample world_map=map100_full_global_task_minigame_dual_atlas_capacity4096_texture3072 world_map_selection=explicit_session_linear_no_heap_no_radius world_map_encounter_sizes=44_34 world_map_encounter_style=official_reference_simplified_contrast world_map_minigame_size=32 world_map_edge_coverage=4x4_all_formal_glyphs_atlas_revision50 world_map_minigames=33_fly_40_mole_10_wave_save_filtered world_map_layering=independent_viewport_hosts_native_canvas_read_only_live_geometry_transform_sync_no_rebuild world_map_replacement=event_driven_exact_set_image_rearm_nonfatal_layer_mismatch world_map_f8=suspend_collapsed world_map_f7=exact_retained_layer_resume_outside_activity world_map_travel=detach activity_suppression=edge_detach_recreate_both_renderers main_menu=exact_title_map_owner_boundary_hard_stop_explicit_open_world_f7 world_map_metrics=category_counts_first_last_ids_atlas_us_bytes_suspend_resume_counts",
            treasure_interact_hook_registered_, treasure_death_hook_registered_,
            encounter_death_hook_registered_,
            encounter_death_process_hook_registered_,
            treasure_interact_schema_ready_,
            world_map_image_hook_registered_,
            world_map_zoom_hook_registered_,
            widget_is_visible_schema_ready_,
            object_create_listener_registered_.load(
                std::memory_order_acquire),
            compact_layer_class_ != nullptr,
            area_quest_state_provider_ready_,
            quest_blueprint_end_hook_registered_,
            renew_quest_blueprint_end_hook_registered_,
            quest_blueprint_end_schema_ready_,
            renew_quest_blueprint_end_schema_ready_,
            task_complete_hook_registered_,
            quest_event_trigger_hook_registered_));
        append_log(
            "READY_2_2_1",
            "area_quest_height=actor_position_data_144_profiles_1_multiband_3_missing_marker_z_selects_unique_nearest_band_move_check_trigger_filtered "
            "area_quest_pointer=black_outline_white_fill_shaftless_chevron "
            "compact_menu_suppression=set_world_map_image_latch_plus_"
            "is_visible_while_latched_plus_is_game_paused_250ms "
            "controller_mapping_reads=none motion_16ms_delta=boolean_only "
            "world_map_hosts=independent_viewport "
            "native_canvas=read_only "
            "geometry_sync=position_size_visibility_only no_rebuild=true");
    }

    void NotifyUObjectCreated(
        const UObjectBase* object, int32 index) override {
        object_create_listener_in_flight_.fetch_add(
            1U, std::memory_order_acq_rel);
        struct InFlightRelease final {
            std::atomic<std::uint32_t>& counter;
            ~InFlightRelease() {
                counter.fetch_sub(1U, std::memory_order_acq_rel);
            }
        } release{object_create_listener_in_flight_};
        if (!object || shutting_down_.load(std::memory_order_acquire)) {
            return;
        }
        FWeakObjectPtr world_map_weak{};
        if (capture_created_world_map_layer_guarded(
                object, &world_map_weak)) {
            {
                const std::scoped_lock lock{world_map_listener_mutex_};
                world_map_listener_candidate_ = world_map_weak;
                world_map_listener_object_index_ = index;
                world_map_listener_pending_.store(
                    true, std::memory_order_release);
            }
        }
        FWeakObjectPtr compact_weak{};
        if (capture_created_compact_layer_guarded(
                object, &compact_weak)) {
            {
                const std::scoped_lock lock{compact_listener_mutex_};
                compact_listener_candidate_ = compact_weak;
                compact_listener_object_index_ = index;
                compact_listener_pending_.store(
                    true, std::memory_order_release);
            }
        }
        FWeakObjectPtr encounter_weak{};
        std::size_t encounter_index{};
        if (capture_created_encounter_actor_guarded(
                object, &encounter_weak, &encounter_index)) {
            const std::scoped_lock lock{created_encounter_mutex_};
            if (encounter_index < created_encounter_candidates_.size()) {
                created_encounter_candidates_[encounter_index] =
                    encounter_weak;
            }
        }
        FWeakObjectPtr bird_egg_weak{};
        if (capture_created_bird_egg_actor_guarded(
                object, &bird_egg_weak)) {
            static_cast<void>(
                publish_created_bird_egg_candidate(bird_egg_weak));
        }
    }

    void OnUObjectArrayShutdown() override {
        shutting_down_.store(true, std::memory_order_release);
        required_runtime_ready_.store(false, std::memory_order_release);
        auto* expected = this;
        instance_.compare_exchange_strong(
            expected, nullptr, std::memory_order_acq_rel);
        uobject_array_shutdown_.store(true, std::memory_order_release);
        // The UObject array requires listeners to be synchronously removed
        // from this callback. Do this before the once-only late finalizer so a
        // normal-uninstall race cannot make the callback return early.
        unregister_object_create_listener(true);
        wait_for_object_create_listener_callbacks();
        if (!shutdown_started_.exchange(true, std::memory_order_acq_rel)) {
            save_reconciler_.shutdown();
            append_log(
                "SHUTDOWN_COMPLETE",
                "umg=retained hooks=registry_teardown listener=drained worker=joined uobject_access=false");
            dsnwr::flush_native_event_log();
        }
    }

private:
    static NativeObjectState* current(std::uint64_t generation) noexcept {
        auto* self = instance_.load(std::memory_order_acquire);
        return self && self->instance_generation_ == generation
            && !self->shutting_down_.load(std::memory_order_acquire) ? self : nullptr;
    }

    [[nodiscard]] static FProperty* find_function_property(
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

    [[nodiscard]] static FProperty* find_struct_property(
        UStruct* structure, const wchar_t* name) noexcept {
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

    [[nodiscard]] static bool read_unsigned_struct_field(
        UStruct* structure,
        void* container,
        const wchar_t* name,
        std::uint64_t& output) {
        FProperty* property = find_struct_property(structure, name);
        void* value = property
            ? property->ContainerPtrToValuePtr<void>(container)
            : nullptr;
        if (!property || !value) {
            return false;
        }
        if (auto* numeric = CastField<FNumericProperty>(property)) {
            if (!numeric->IsInteger()) {
                return false;
            }
            output = numeric->GetUnsignedIntPropertyValue(value);
            return true;
        }
        auto* enum_property = CastField<FEnumProperty>(property);
        FNumericProperty* underlying = enum_property
            ? enum_property->GetUnderlyingProperty()
            : nullptr;
        if (!underlying || !underlying->IsInteger()) {
            return false;
        }
        output = underlying->GetUnsignedIntPropertyValue(value);
        return true;
    }

    [[nodiscard]] bool initialize_encounter_death_process_schema() noexcept {
        if (!encounter_death_process_function_) {
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            encounter_death_process_function_->GetParmsSize());
        encounter_death_process_property_ = CastField<FEnumProperty>(
            find_function_property(
                encounter_death_process_function_, L"InDeathProcess"));
        encounter_death_process_underlying_property_ =
            encounter_death_process_property_
                ? encounter_death_process_property_->GetUnderlyingProperty()
                : nullptr;
        return parameter_bytes > 0 && parameter_bytes <= 32
            && encounter_death_process_property_
            && encounter_death_process_property_->GetOffset_Internal() >= 0
            && encounter_death_process_property_->GetSize() > 0
            && static_cast<std::size_t>(
                encounter_death_process_property_->GetOffset_Internal()
                    + encounter_death_process_property_->GetSize())
                <= parameter_bytes
            && encounter_death_process_underlying_property_
            && encounter_death_process_underlying_property_->IsInteger();
    }

    [[nodiscard]] static bool read_map_key(
        FMapProperty* property,
        void* pair,
        std::uint32_t& output) {
        FNumericProperty* key_property = property
            ? CastField<FNumericProperty>(property->GetKeyProp())
            : nullptr;
        if (!key_property || !key_property->IsInteger() || !pair) {
            return false;
        }
        const std::uint64_t value =
            key_property->GetUnsignedIntPropertyValue(pair);
        if (value == 0 || value > UINT32_MAX) {
            return false;
        }
        output = static_cast<std::uint32_t>(value);
        return true;
    }

    [[nodiscard]] static bool map_shape_is_bounded(
        const FScriptMap* map) noexcept {
        return map && map->Num() >= 0 && map->Num() <= 8192
            && map->GetMaxIndex() >= map->Num()
            && map->GetMaxIndex() <= 16384;
    }

    [[nodiscard]] bool capture_area_quest_definitions_guarded() noexcept {
#if defined(_MSC_VER)
        __try {
            return capture_area_quest_definitions_unsafe();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        try {
            return capture_area_quest_definitions_unsafe();
        } catch (...) {
            return false;
        }
#endif
    }

    [[nodiscard]] bool capture_area_quest_definitions_unsafe() {
        area_quest_definitions_.fill({});
        area_quest_definition_ready_ = false;
        area_quest_definition_match_count_ = 0;
        area_quest_definition_unknown_condition_count_ = 0;
        area_quest_definition_weighted_selection_count_ = 0;
        area_quest_definition_monster_condition_count_ = 0;
        area_quest_definition_monster_link_count_ = 0;
        area_quest_definition_monster_ambiguous_count_ = 0;

        UObject* singleton = UObjectGlobals::FindFirstOf(STR("DGameSingleton"));
        auto** manager_value = singleton
            ? singleton->GetValuePtrByPropertyNameInChain<UObject*>(
                  STR("GameDBTableManager"))
            : nullptr;
        UObject* manager = manager_value ? *manager_value : nullptr;
        FScriptArray* databases = manager
            ? manager->GetValuePtrByPropertyNameInChain<FScriptArray>(
                  STR("GameDBArray"))
            : nullptr;
        if (!databases || databases->Num() <= 0 || databases->Num() > 2048
            || !databases->GetData()) {
            return false;
        }
        UObject* dynamic_table{};
        auto** database_objects = static_cast<UObject**>(databases->GetData());
        for (int32 index = 0; index < databases->Num(); ++index) {
            UObject* candidate = database_objects[index];
            UClass* candidate_class = candidate
                ? candidate->GetClassPrivate()
                : nullptr;
            if (candidate_class
                && to_string(candidate_class->GetName())
                    == "DDynamicQuestDataTable") {
                dynamic_table = candidate;
                break;
            }
        }
        if (!dynamic_table) {
            return false;
        }

        auto* main_outer_property = CastField<FStructProperty>(
            dynamic_table->GetPropertyByNameInChain(
                STR("DynamicQuestMainMap")));
        void* main_outer = dynamic_table->GetValuePtrByPropertyNameInChain(
            STR("DynamicQuestMainMap"));
        UScriptStruct* main_outer_struct = main_outer_property
            ? main_outer_property->GetStruct()
            : nullptr;
        auto* main_map_property = CastField<FMapProperty>(
            find_struct_property(main_outer_struct, L"Data"));
        FScriptMap* main_map = main_map_property
            ? main_map_property->ContainerPtrToValuePtr<FScriptMap>(main_outer)
            : nullptr;
        auto* main_wrap_property = main_map_property
            ? CastField<FStructProperty>(main_map_property->GetValueProp())
            : nullptr;
        UScriptStruct* main_wrap_struct = main_wrap_property
            ? main_wrap_property->GetStruct()
            : nullptr;
        auto* nested_main_map_property = CastField<FMapProperty>(
            find_struct_property(
                main_wrap_struct, L"DynamicQuestMainDataMap"));
        auto* main_row_property = nested_main_map_property
            ? CastField<FStructProperty>(
                  nested_main_map_property->GetValueProp())
            : nullptr;
        UScriptStruct* main_row_struct = main_row_property
            ? main_row_property->GetStruct()
            : nullptr;
        if (!map_shape_is_bounded(main_map) || !nested_main_map_property
            || !main_row_struct) {
            return false;
        }

        std::unordered_map<std::uint32_t, std::uint32_t> group_member_counts;
        const auto main_layout = DSNWRPR_MAP_LAYOUT(main_map_property);
        for (int32 outer_index = 0;
             outer_index < main_map->GetMaxIndex(); ++outer_index) {
            if (!main_map->IsValidIndex(outer_index)) {
                continue;
            }
            auto* outer_pair = static_cast<std::byte*>(
                main_map->GetData(outer_index, main_layout));
            void* main_wrap = outer_pair + main_layout.ValueOffset;
            FScriptMap* nested_map =
                nested_main_map_property->ContainerPtrToValuePtr<FScriptMap>(
                    main_wrap);
            if (!map_shape_is_bounded(nested_map)) {
                return false;
            }
            const auto nested_layout =
                DSNWRPR_MAP_LAYOUT(nested_main_map_property);
            for (int32 row_index = 0;
                 row_index < nested_map->GetMaxIndex(); ++row_index) {
                if (!nested_map->IsValidIndex(row_index)) {
                    continue;
                }
                auto* row_pair = static_cast<std::byte*>(
                    nested_map->GetData(row_index, nested_layout));
                std::uint32_t quest_id{};
                if (!read_map_key(
                        nested_main_map_property, row_pair, quest_id)) {
                    continue;
                }
                const auto catalog = std::find_if(
                    area_quest_catalog_.begin(), area_quest_catalog_.end(),
                    [quest_id](const AreaQuestSpec& spec) {
                        return spec.id == quest_id;
                    });
                if (catalog == area_quest_catalog_.end()) {
                    continue;
                }
                const std::size_t catalog_index = static_cast<std::size_t>(
                    std::distance(area_quest_catalog_.begin(), catalog));
                void* row = row_pair + nested_layout.ValueOffset;
                std::uint64_t group_id{};
                std::uint64_t group_weight{};
                std::uint64_t active_count{};
                std::uint64_t accept_type{};
                std::uint64_t accept_value1{};
                std::uint64_t accept_value2{};
                if (!read_unsigned_struct_field(
                        main_row_struct, row, L"GroupID", group_id)
                    || !read_unsigned_struct_field(
                        main_row_struct, row, L"GroupActiveWeight",
                        group_weight)
                    || !read_unsigned_struct_field(
                        main_row_struct, row, L"ActiveCnt", active_count)
                    || !read_unsigned_struct_field(
                        main_row_struct, row, L"AcceptConditionType",
                        accept_type)
                    || !read_unsigned_struct_field(
                        main_row_struct, row, L"AcceptConditionValue1",
                        accept_value1)
                    || !read_unsigned_struct_field(
                        main_row_struct, row, L"AcceptConditionValue2",
                        accept_value2)
                    || group_id > UINT32_MAX || group_weight > UINT32_MAX
                    || active_count > UINT32_MAX
                    || accept_type > UINT8_MAX
                    || accept_value1 > UINT32_MAX
                    || accept_value2 > UINT32_MAX) {
                    return false;
                }
                AreaQuestDefinition& definition =
                    area_quest_definitions_[catalog_index];
                definition.group_id = static_cast<std::uint32_t>(group_id);
                definition.group_active_weight =
                    static_cast<std::uint32_t>(group_weight);
                definition.group_active_count =
                    static_cast<std::uint32_t>(active_count);
                definition.accept_condition = DynamicQuestCondition{
                    static_cast<std::uint8_t>(accept_type),
                    static_cast<std::uint32_t>(accept_value1),
                    static_cast<std::uint32_t>(accept_value2)};
                definition.found = true;
                ++group_member_counts[definition.group_id];
                ++area_quest_definition_match_count_;
            }
        }

        auto* group_outer_property = CastField<FStructProperty>(
            dynamic_table->GetPropertyByNameInChain(
                STR("DynamicQuestGroupMap")));
        void* group_outer = dynamic_table->GetValuePtrByPropertyNameInChain(
            STR("DynamicQuestGroupMap"));
        UScriptStruct* group_outer_struct = group_outer_property
            ? group_outer_property->GetStruct()
            : nullptr;
        auto* group_map_property = CastField<FMapProperty>(
            find_struct_property(group_outer_struct, L"Data"));
        FScriptMap* group_map = group_map_property
            ? group_map_property->ContainerPtrToValuePtr<FScriptMap>(group_outer)
            : nullptr;
        auto* group_row_property = group_map_property
            ? CastField<FStructProperty>(group_map_property->GetValueProp())
            : nullptr;
        UScriptStruct* group_row_struct = group_row_property
            ? group_row_property->GetStruct()
            : nullptr;
        if (!map_shape_is_bounded(group_map) || !group_row_struct) {
            return false;
        }
        const auto group_layout = DSNWRPR_MAP_LAYOUT(group_map_property);
        for (int32 group_index = 0;
             group_index < group_map->GetMaxIndex(); ++group_index) {
            if (!group_map->IsValidIndex(group_index)) {
                continue;
            }
            auto* pair = static_cast<std::byte*>(
                group_map->GetData(group_index, group_layout));
            std::uint32_t group_id{};
            if (!read_map_key(group_map_property, pair, group_id)
                || !group_member_counts.contains(group_id)) {
                continue;
            }
            void* row = pair + group_layout.ValueOffset;
            std::uint64_t timing_type{};
            std::uint64_t condition_type{};
            std::uint64_t condition_value1{};
            std::uint64_t condition_value2{};
            if (!read_unsigned_struct_field(
                    group_row_struct, row, L"ActiveTimingType", timing_type)
                || !read_unsigned_struct_field(
                    group_row_struct, row, L"GroupConditionType",
                    condition_type)
                || !read_unsigned_struct_field(
                    group_row_struct, row, L"GroupConditionValue1",
                    condition_value1)
                || !read_unsigned_struct_field(
                    group_row_struct, row, L"GroupConditionValue2",
                    condition_value2)
                || timing_type > UINT8_MAX || condition_type > UINT8_MAX
                || condition_value1 > UINT32_MAX
                || condition_value2 > UINT32_MAX) {
                return false;
            }
            for (AreaQuestDefinition& definition : area_quest_definitions_) {
                if (!definition.found || definition.group_id != group_id) {
                    continue;
                }
                definition.active_timing_type =
                    static_cast<std::uint8_t>(timing_type);
                definition.group_member_count = group_member_counts[group_id];
                definition.group_condition = DynamicQuestCondition{
                    static_cast<std::uint8_t>(condition_type),
                    static_cast<std::uint32_t>(condition_value1),
                    static_cast<std::uint32_t>(condition_value2)};
            }
        }

        constexpr double kMonsterEncounterLinkRadius = 15000.0;
        const double monster_link_radius_squared =
            kMonsterEncounterLinkRadius * kMonsterEncounterLinkRadius;
        const auto link_monster_condition = [this, monster_link_radius_squared](
            std::size_t quest_index,
            DynamicQuestCondition& condition) noexcept {
            if (condition.type != static_cast<std::uint8_t>(
                                      DynamicQuestConditionType::MonsterAlive)
                || condition.value1 != 0
                || quest_index >= area_quest_catalog_.size()) {
                return;
            }
            ++area_quest_definition_monster_condition_count_;
            const EncounterSpec* nearest{};
            double nearest_distance = monster_link_radius_squared;
            std::size_t candidate_count{};
            for (const EncounterSpec& encounter : encounter_catalog_) {
                if (encounter.kind != EncounterKind::Assault) {
                    continue;
                }
                const double distance = planar_distance_squared(
                    area_quest_catalog_[quest_index].position,
                    encounter.position);
                if (!std::isfinite(distance)
                    || distance > monster_link_radius_squared) {
                    continue;
                }
                ++candidate_count;
                if (!nearest || distance < nearest_distance) {
                    nearest = &encounter;
                    nearest_distance = distance;
                }
            }
            // MONSTER_ALIVE rows in the current game data intentionally carry
            // value1=0. Link only when position proves exactly one Assault in
            // the bounded 150-metre neighborhood; otherwise remain fail-closed.
            if (nearest && candidate_count == 1U) {
                condition.linked_encounter_id = nearest->id;
                ++area_quest_definition_monster_link_count_;
            } else if (candidate_count > 1U) {
                ++area_quest_definition_monster_ambiguous_count_;
            }
        };
        for (std::size_t index = 0;
             index < area_quest_definitions_.size(); ++index) {
            AreaQuestDefinition& definition = area_quest_definitions_[index];
            if (!definition.found) {
                continue;
            }
            link_monster_condition(index, definition.group_condition);
            link_monster_condition(index, definition.accept_condition);
        }

        for (const AreaQuestDefinition& definition : area_quest_definitions_) {
            if (!definition.found) {
                continue;
            }
            const auto supported = [](std::uint8_t type) noexcept {
                return type == static_cast<std::uint8_t>(
                                   DynamicQuestConditionType::None)
                    || type == static_cast<std::uint8_t>(
                                   DynamicQuestConditionType::QuestClear)
                    || type == static_cast<std::uint8_t>(
                                   DynamicQuestConditionType::DynamicQuestComplete)
                    || type == static_cast<std::uint8_t>(
                                   DynamicQuestConditionType::MonsterAlive);
            };
            const auto condition_ready = [&supported](
                const DynamicQuestCondition& condition) noexcept {
                return supported(condition.type)
                    && (condition.type != static_cast<std::uint8_t>(
                            DynamicQuestConditionType::MonsterAlive)
                        || condition.linked_encounter_id > 0);
            };
            if (!condition_ready(definition.group_condition)
                || !condition_ready(definition.accept_condition)) {
                ++area_quest_definition_unknown_condition_count_;
            }
            if (definition.group_member_count > 1
                && definition.group_active_weight > 0) {
                ++area_quest_definition_weighted_selection_count_;
            }
        }
        area_quest_definition_ready_ =
            area_quest_definition_match_count_ == area_quest_catalog_.size();
        return area_quest_definition_ready_;
    }

    [[nodiscard]] bool capture_area_quest_task_class_map_guarded(
        UEngine* engine) noexcept {
        if (area_quest_task_class_map_ready_) {
            return true;
        }
#if defined(_MSC_VER)
        __try {
            return capture_area_quest_task_class_map_unsafe(engine);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            area_quest_task_class_indices_.clear();
            area_quest_task_class_map_ready_ = false;
            return false;
        }
#else
        try {
            return capture_area_quest_task_class_map_unsafe(engine);
        } catch (...) {
            area_quest_task_class_indices_.clear();
            area_quest_task_class_map_ready_ = false;
            return false;
        }
#endif
    }

    [[nodiscard]] bool capture_area_quest_task_class_map_unsafe(
        UEngine* engine) {
        area_quest_task_class_indices_.clear();
        area_quest_task_class_map_ready_ = false;
        area_quest_task_class_row_count_ = 0;
        area_quest_dynamic_task_class_row_count_ = 0;
        area_quest_task_class_ambiguity_count_ = 0;

        UObject* game_instance = current_game_instance(engine);
        auto* container_property = game_instance
            ? CastField<FObjectPropertyBase>(
                  game_instance->GetPropertyByNameInChain(
                      STR("TaskActorClassContainer")))
            : nullptr;
        void* container_value = container_property
            ? container_property->ContainerPtrToValuePtr<void>(game_instance)
            : nullptr;
        UObject* container = container_value
            ? container_property->GetObjectPropertyValue(container_value)
            : nullptr;
        auto* task_array_property = container
            ? CastField<FArrayProperty>(
                  container->GetPropertyByNameInChain(
                      STR("DynamicQuestTaskList")))
            : nullptr;
        auto* task_row_property = task_array_property
            ? CastField<FStructProperty>(task_array_property->GetInner())
            : nullptr;
        UScriptStruct* task_row_struct = task_row_property
            ? task_row_property->GetStruct()
            : nullptr;
        auto* quest_ids_property = CastField<FArrayProperty>(
            find_struct_property(task_row_struct, L"UseQuestList"));
        auto* quest_id_property = quest_ids_property
            ? CastField<FNumericProperty>(quest_ids_property->GetInner())
            : nullptr;
        auto* task_class_property = CastField<FClassProperty>(
            find_struct_property(task_row_struct, L"CreateTaskClass"));
        if (!container || !task_array_property || !task_row_struct
            || !find_struct_property(task_row_struct, L"TaskUseType")
            || !quest_ids_property || !quest_id_property
            || !quest_id_property->IsInteger() || !task_class_property) {
            return false;
        }

        FScriptArrayHelper_InContainer rows{task_array_property, container};
        if (rows.Num() <= 0
            || rows.Num() > static_cast<int32>(kMaximumDynamicTaskRows)) {
            return false;
        }
        area_quest_task_class_row_count_ =
            static_cast<std::size_t>(rows.Num());

        std::unordered_map<std::uint64_t, std::size_t>
            catalog_indices_by_id;
        catalog_indices_by_id.reserve(kExpectedAreaQuestCount);
        for (std::size_t catalog_index = 0;
             catalog_index < area_quest_catalog_.size(); ++catalog_index) {
            catalog_indices_by_id.emplace(
                static_cast<std::uint64_t>(
                    area_quest_catalog_[catalog_index].id),
                catalog_index);
        }

        std::unordered_map<std::string, std::uint64_t>
            task_class_quest_ids;
        std::unordered_set<std::string> catalog_classes;
        std::unordered_set<std::string> ambiguous_classes;
        task_class_quest_ids.reserve(kExpectedAreaQuestCount);
        catalog_classes.reserve(kExpectedAreaQuestCount);
        ambiguous_classes.reserve(kExpectedAreaQuestCount);
        for (int32 row_index = 0; row_index < rows.Num(); ++row_index) {
            void* row = rows.GetRawPtr(row_index);
            std::uint64_t task_use_type{};
            if (!row || !read_unsigned_struct_field(
                    task_row_struct, row, L"TaskUseType", task_use_type)) {
                return false;
            }
            if (task_use_type != kDynamicQuestTaskUseType) {
                continue;
            }
            ++area_quest_dynamic_task_class_row_count_;

            void* class_value = task_class_property
                ->ContainerPtrToValuePtr<void>(row);
            UClass* task_class = Cast<UClass>(
                class_value
                    ? task_class_property->GetObjectPropertyValue(class_value)
                    : nullptr);
            if (!task_class) {
                continue;
            }
            const std::string class_name =
                to_string(task_class->GetFullName());
            if (class_name.empty()) {
                continue;
            }

            FScriptArrayHelper_InContainer quest_ids{
                quest_ids_property, row};
            if (quest_ids.Num() < 0
                || quest_ids.Num()
                    > static_cast<int32>(kMaximumQuestIdsPerTaskClass)) {
                return false;
            }
            for (int32 id_index = 0;
                 id_index < quest_ids.Num(); ++id_index) {
                void* id_value = quest_ids.GetRawPtr(id_index);
                if (!id_value) {
                    return false;
                }
                const std::uint64_t quest_id =
                    quest_id_property->GetUnsignedIntPropertyValue(id_value);
                if (quest_id == 0) {
                    continue;
                }
                if (catalog_indices_by_id.contains(quest_id)) {
                    catalog_classes.insert(class_name);
                }
                const auto [existing, inserted] =
                    task_class_quest_ids.emplace(class_name, quest_id);
                if (!inserted && existing->second != quest_id) {
                    ambiguous_classes.insert(class_name);
                }
            }
        }

        std::unordered_map<std::string, std::size_t> candidates;
        candidates.reserve(kExpectedAreaQuestCount);
        std::unordered_set<std::string> catalog_ambiguous_classes;
        catalog_ambiguous_classes.reserve(kExpectedAreaQuestCount);
        for (const std::string& class_name : catalog_classes) {
            if (ambiguous_classes.contains(class_name)) {
                catalog_ambiguous_classes.insert(class_name);
                continue;
            }
            const auto quest_id = task_class_quest_ids.find(class_name);
            const auto catalog = quest_id != task_class_quest_ids.end()
                ? catalog_indices_by_id.find(quest_id->second)
                : catalog_indices_by_id.end();
            if (catalog == catalog_indices_by_id.end()) {
                return false;
            }
            candidates.emplace(class_name, catalog->second);
        }

        std::array<bool, kExpectedAreaQuestCount> covered{};
        bool duplicate_catalog_id{};
        for (const auto& [class_name, catalog_index] : candidates) {
            static_cast<void>(class_name);
            if (catalog_index >= covered.size() || covered[catalog_index]) {
                duplicate_catalog_id = true;
                break;
            }
            covered[catalog_index] = true;
        }
        area_quest_task_class_ambiguity_count_ =
            catalog_ambiguous_classes.size()
            + (duplicate_catalog_id ? 1U : 0U);
        if (duplicate_catalog_id || !catalog_ambiguous_classes.empty()
            || candidates.size() != area_quest_catalog_.size()
            || std::count(covered.begin(), covered.end(), true)
                != static_cast<std::ptrdiff_t>(area_quest_catalog_.size())) {
            return false;
        }

        area_quest_task_class_indices_ = std::move(candidates);
        area_quest_task_class_map_ready_ = true;
        return true;
    }

    [[nodiscard]] bool initialize_area_quest_state_provider() noexcept {
        quest_info_function_ = UObjectGlobals::StaticFindObject<UFunction*>(
            nullptr, nullptr, kQuestInfoFunction);
        UObject* utility_default = UObjectGlobals::StaticFindObject<UObject*>(
            nullptr, nullptr, kQuestUtilityDefault);
        if (!quest_info_function_ || !utility_default) {
            append_log(
                "AREA_QUEST_STATE_DISABLED",
                "reason=function_or_default_object_missing");
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            quest_info_function_->GetParmsSize());
        if (parameter_bytes == 0
            || parameter_bytes > kAreaQuestParameterCapacity) {
            append_log("AREA_QUEST_STATE_DISABLED", std::format(
                "reason=parameter_size_invalid bytes={}",
                parameter_bytes));
            return false;
        }

        quest_world_context_property_ = CastField<FObjectPropertyBase>(
            find_function_property(
                quest_info_function_, L"WorldContextObject"));
        quest_id_property_ = CastField<FNumericProperty>(
            find_function_property(quest_info_function_, L"QuestID"));
        quest_dynamic_property_ = CastField<FBoolProperty>(
            find_function_property(quest_info_function_, L"IsDynamic"));
        quest_step_property_ = CastField<FNumericProperty>(
            find_function_property(quest_info_function_, L"QuestStep"));
        quest_current_count_property_ = CastField<FNumericProperty>(
            find_function_property(
                quest_info_function_, L"QuestCurrentCount"));
        quest_max_count_property_ = CastField<FNumericProperty>(
            find_function_property(
                quest_info_function_, L"QuestMaxCount"));
        quest_state_property_ = CastField<FEnumProperty>(
            find_function_property(quest_info_function_, L"QuestState"));
        quest_return_property_ = CastField<FBoolProperty>(
            find_function_property(quest_info_function_, L"ReturnValue"));
        quest_state_underlying_property_ = quest_state_property_
            ? quest_state_property_->GetUnderlyingProperty()
            : nullptr;

        const auto property_fits = [parameter_bytes](
            FProperty* property) noexcept {
            return property
                && property->GetOffset_Internal() >= 0
                && property->GetSize() > 0
                && static_cast<std::size_t>(
                    property->GetOffset_Internal()
                        + property->GetSize()) <= parameter_bytes;
        };
        const bool valid =
            property_fits(quest_world_context_property_)
            && property_fits(quest_id_property_)
            && property_fits(quest_dynamic_property_)
            && property_fits(quest_step_property_)
            && property_fits(quest_current_count_property_)
            && property_fits(quest_max_count_property_)
            && property_fits(quest_state_property_)
            && property_fits(quest_return_property_)
            && quest_id_property_->IsInteger()
            && quest_step_property_->IsInteger()
            && quest_current_count_property_->IsInteger()
            && quest_max_count_property_->IsInteger()
            && quest_state_underlying_property_
            && quest_state_underlying_property_->IsInteger();
        if (!valid) {
            append_log(
                "AREA_QUEST_STATE_DISABLED",
                "reason=reflected_parameter_schema_invalid");
            return false;
        }
        quest_utility_default_ = utility_default;
        append_log("AREA_QUEST_STATE_READY", std::format(
            "function={} parameter_bytes={} catalog={} schedule=one_id_per_game_frame triggers=f7_travel_quest_end_task_complete_world_hour",
            to_string(quest_info_function_->GetFullName()),
            parameter_bytes, area_quest_catalog_.size()));
        return true;
    }

    [[nodiscard]] bool initialize_quest_event_trigger_provider() noexcept {
        quest_event_trigger_function_ =
            UObjectGlobals::StaticFindObject<UFunction*>(
                nullptr, nullptr, kQuestEventTriggerFunction);
        if (!area_quest_state_provider_ready_
            || !quest_event_trigger_function_) {
            append_log(
                "AREA_QUEST_EVENT_HOOK_DISABLED",
                quest_event_trigger_function_
                    ? "reason=state_provider_unavailable"
                    : "reason=function_missing");
            return false;
        }
        if (!quest_event_trigger_function_->HasAnyFunctionFlags(
                FUNC_Native)) {
            append_log(
                "AREA_QUEST_EVENT_HOOK_DISABLED",
                "reason=non_native_global_blueprint_hook_avoided");
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            quest_event_trigger_function_->GetParmsSize());
        if (parameter_bytes == 0
            || parameter_bytes > kQuestEventParameterCapacity) {
            append_log("AREA_QUEST_EVENT_HOOK_DISABLED", std::format(
                "reason=parameter_size_invalid bytes={}",
                parameter_bytes));
            return false;
        }

        quest_event_world_context_property_ =
            CastField<FObjectPropertyBase>(find_function_property(
                quest_event_trigger_function_, L"WorldContextObject"));
        quest_event_id_property_ = CastField<FNumericProperty>(
            find_function_property(
                quest_event_trigger_function_, L"QuestID"));
        quest_event_step_id_property_ = CastField<FNumericProperty>(
            find_function_property(
                quest_event_trigger_function_, L"StepID"));
        quest_event_step_count_property_ = CastField<FNumericProperty>(
            find_function_property(
                quest_event_trigger_function_, L"StepCnt"));
        quest_event_is_set_property_ = CastField<FBoolProperty>(
            find_function_property(
                quest_event_trigger_function_, L"IsSet"));
        quest_event_dynamic_property_ = CastField<FBoolProperty>(
            find_function_property(
                quest_event_trigger_function_, L"IsDynamic"));
        const auto property_fits = [parameter_bytes](
            FProperty* property) noexcept {
            return property
                && property->GetOffset_Internal() >= 0
                && property->GetSize() > 0
                && static_cast<std::size_t>(
                    property->GetOffset_Internal()
                        + property->GetSize()) <= parameter_bytes;
        };
        const bool valid =
            property_fits(quest_event_world_context_property_)
            && property_fits(quest_event_id_property_)
            && property_fits(quest_event_step_id_property_)
            && property_fits(quest_event_step_count_property_)
            && property_fits(quest_event_is_set_property_)
            && property_fits(quest_event_dynamic_property_)
            && quest_event_id_property_->IsInteger()
            && quest_event_step_id_property_->IsInteger()
            && quest_event_step_count_property_->IsInteger();
        if (!valid) {
            append_log(
                "AREA_QUEST_EVENT_HOOK_DISABLED",
                "reason=reflected_parameter_schema_invalid");
            return false;
        }
        append_log("AREA_QUEST_EVENT_HOOK_READY", std::format(
            "function={} parameter_bytes={} native=true schedule=event_driven_bounded_refresh",
            to_string(quest_event_trigger_function_->GetFullName()),
            parameter_bytes));
        return true;
    }

    [[nodiscard]] bool initialize_quest_blueprint_end_schema(
        UFunction* function,
        FObjectPropertyBase*& end_actor_property,
        const char* label) noexcept {
        end_actor_property = nullptr;
        if (!function) {
            append_log("AREA_QUEST_END_SCHEMA_DISABLED", std::format(
                "hook={} reason=function_missing", label));
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            function->GetParmsSize());
        if (parameter_bytes == 0
            || parameter_bytes > kQuestBlueprintEndParameterCapacity) {
            append_log("AREA_QUEST_END_SCHEMA_DISABLED", std::format(
                "hook={} reason=parameter_size_invalid bytes={}",
                label, parameter_bytes));
            return false;
        }
        end_actor_property = CastField<FObjectPropertyBase>(
            find_function_property(function, L"EndPlayActor"));
        const bool valid = end_actor_property
            && end_actor_property->GetOffset_Internal() >= 0
            && end_actor_property->GetSize() > 0
            && static_cast<std::size_t>(
                   end_actor_property->GetOffset_Internal()
                       + end_actor_property->GetSize())
                <= parameter_bytes;
        if (!valid) {
            end_actor_property = nullptr;
            append_log("AREA_QUEST_END_SCHEMA_DISABLED", std::format(
                "hook={} reason=end_play_actor_schema_invalid bytes={}",
                label, parameter_bytes));
            return false;
        }
        append_log("AREA_QUEST_END_SCHEMA_READY", std::format(
            "hook={} function={} parameter_bytes={} identity=exact_task_actor_class",
            label, to_string(function->GetFullName()), parameter_bytes));
        return true;
    }

    [[nodiscard]] bool initialize_widget_is_visible_schema() noexcept {
        widget_is_visible_return_property_ = nullptr;
        if (!widget_is_visible_function_) {
            append_log("WORLD_MAP_VISIBILITY_PROVIDER_DISABLED",
                "reason=function_missing mode=defer_until_set_world_map_image");
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            widget_is_visible_function_->GetParmsSize());
        widget_is_visible_return_property_ = CastField<FBoolProperty>(
            find_function_property(widget_is_visible_function_, L"ReturnValue"));
        const bool valid = parameter_bytes > 0
            && parameter_bytes <= kWidgetIsVisibleParameterCapacity
            && widget_is_visible_return_property_
            && widget_is_visible_return_property_->GetOffset_Internal() >= 0
            && widget_is_visible_return_property_->GetSize() > 0
            && static_cast<std::size_t>(
                   widget_is_visible_return_property_->GetOffset_Internal()
                       + widget_is_visible_return_property_->GetSize())
                <= parameter_bytes;
        if (!valid) {
            widget_is_visible_return_property_ = nullptr;
            append_log("WORLD_MAP_VISIBILITY_PROVIDER_DISABLED", std::format(
                "reason=reflected_parameter_schema_invalid bytes={} mode=defer_until_set_world_map_image",
                parameter_bytes));
            return false;
        }
        append_log("WORLD_MAP_VISIBILITY_PROVIDER_READY", std::format(
            "function={} parameter_bytes={} schedule=runtime_delta_only",
            to_string(widget_is_visible_function_->GetFullName()),
            parameter_bytes));
        return true;
    }

    [[nodiscard]] bool initialize_game_pause_schema() noexcept {
        game_pause_world_context_property_ = nullptr;
        game_pause_return_property_ = nullptr;
        if (!is_game_paused_function_
            || !gameplay_statics_default_.Get()) {
            append_log("COMPACT_PAUSE_PROVIDER_DISABLED",
                "reason=function_or_default_object_missing mode=cursor_and_world_map_only");
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            is_game_paused_function_->GetParmsSize());
        game_pause_world_context_property_ = CastField<FObjectPropertyBase>(
            find_function_property(
                is_game_paused_function_, L"WorldContextObject"));
        game_pause_return_property_ = CastField<FBoolProperty>(
            find_function_property(
                is_game_paused_function_, L"ReturnValue"));
        const auto property_fits = [parameter_bytes](
            FProperty* property) noexcept {
            return property && property->GetOffset_Internal() >= 0
                && property->GetSize() > 0
                && static_cast<std::size_t>(
                       property->GetOffset_Internal()
                           + property->GetSize())
                    <= parameter_bytes;
        };
        const bool valid = parameter_bytes > 0
            && parameter_bytes <= kIsGamePausedParameterCapacity
            && property_fits(game_pause_world_context_property_)
            && property_fits(game_pause_return_property_);
        if (!valid) {
            game_pause_world_context_property_ = nullptr;
            game_pause_return_property_ = nullptr;
            append_log("COMPACT_PAUSE_PROVIDER_DISABLED", std::format(
                "reason=reflected_parameter_schema_invalid bytes={} mode=cursor_and_world_map_only",
                parameter_bytes));
            return false;
        }
        append_log("COMPACT_PAUSE_PROVIDER_READY", std::format(
            "function={} parameter_bytes={} schedule=shared_250ms_activity_probe edge_logging_only",
            to_string(is_game_paused_function_->GetFullName()),
            parameter_bytes));
        return true;
    }

    [[nodiscard]] bool initialize_current_language_provider() noexcept {
        engine_game_user_settings_property_ = nullptr;
        game_language_text_property_ = nullptr;
        game_language_text_numeric_property_ = nullptr;
        current_language_return_property_ = nullptr;
        game_user_settings_class_ =
            UObjectGlobals::StaticFindObject<UClass*>(
                nullptr, nullptr, kGameUserSettingsClass);
        UClass* engine_class = UObjectGlobals::StaticFindObject<UClass*>(
            nullptr, nullptr, kEngineClass);
        engine_game_user_settings_property_ = CastField<FObjectPropertyBase>(
            engine_class
                ? engine_class->GetPropertyByNameInChain(
                      L"GameUserSettings")
                : nullptr);
        game_language_text_property_ = game_user_settings_class_
            ? game_user_settings_class_->GetPropertyByNameInChain(
                  L"LanguageText")
            : nullptr;
        auto* direct_language_numeric =
            CastField<FNumericProperty>(game_language_text_property_);
        auto* language_enum =
            CastField<FEnumProperty>(game_language_text_property_);
        game_language_text_numeric_property_ = direct_language_numeric
            ? direct_language_numeric
            : language_enum ? language_enum->GetUnderlyingProperty() : nullptr;
        game_user_settings_language_schema_ready_ = engine_class
            && game_user_settings_class_
            && engine_game_user_settings_property_
            && engine_game_user_settings_property_->GetOffset_Internal() >= 0
            && engine_game_user_settings_property_->GetSize()
                == static_cast<int32>(sizeof(UObject*))
            && game_language_text_property_
            && game_language_text_numeric_property_
            && game_language_text_numeric_property_->IsInteger()
            && game_language_text_property_->GetOffset_Internal() >= 0
            && game_language_text_property_->GetSize() > 0
            && game_language_text_property_->GetSize()
                <= static_cast<int32>(sizeof(std::uint64_t));

        std::size_t parameter_bytes{};
        if (current_language_function_
            && internationalization_library_default_.Get()) {
            parameter_bytes = static_cast<std::size_t>(
                current_language_function_->GetParmsSize());
            current_language_return_property_ = CastField<FStrProperty>(
                find_function_property(
                    current_language_function_, L"ReturnValue"));
        }
        internationalization_language_schema_ready_ =
            current_language_function_
            && internationalization_library_default_.Get()
            && parameter_bytes == sizeof(FString)
            && parameter_bytes <= kCurrentLanguageParameterCapacity
            && current_language_return_property_
            && current_language_return_property_->GetOffset_Internal() == 0
            && current_language_return_property_->GetSize()
                == static_cast<int32>(sizeof(FString));
        if (!internationalization_language_schema_ready_) {
            current_language_return_property_ = nullptr;
        }
        const bool ready = game_user_settings_language_schema_ready_
            || internationalization_language_schema_ready_;
        append_log(
            ready
                ? "RADAR_LANGUAGE_PROVIDER_READY"
                : "RADAR_LANGUAGE_PROVIDER_DISABLED",
            std::format(
                "primary=engine_game_user_settings_language_text primary_ready={} primary_enum={} fallback=kismet_current_language fallback_ready={} parameter_bytes={} schedule=f7_and_f6_open fallback_final=en",
                game_user_settings_language_schema_ready_,
                language_enum != nullptr,
                internationalization_language_schema_ready_,
                parameter_bytes));
        return ready;
    }

    [[nodiscard]] dswros::RadarUiLanguage
    detect_current_game_language(UEngine* engine) noexcept {
        current_language_detection_source_ = "fallback_en";
        if (game_user_settings_language_schema_ready_ && engine) {
            try {
                void* settings_value = engine_game_user_settings_property_
                    ->ContainerPtrToValuePtr<void>(engine);
                UObject* settings = settings_value
                    ? engine_game_user_settings_property_
                          ->GetObjectPropertyValue(settings_value)
                    : nullptr;
                void* language_value = settings
                        && settings->IsA(game_user_settings_class_)
                    ? game_language_text_property_
                          ->ContainerPtrToValuePtr<void>(settings)
                    : nullptr;
                if (language_value) {
                    const std::uint64_t raw_language =
                        game_language_text_numeric_property_
                            ->GetUnsignedIntPropertyValue(language_value);
                    dswros::RadarUiLanguage mapped{};
                    if (dswros::radar_ui_language_from_game_setting(
                            raw_language, mapped)) {
                        current_language_detection_source_ =
                            "game_user_settings_language_text";
                        return mapped;
                    }
                }
            } catch (...) {
                // Continue to UE's culture API. A missing transient settings
                // object must not prevent the F6 panel from opening.
            }
        }
        UObject* library = internationalization_library_default_.Get();
        if (!internationalization_language_schema_ready_ || !library
            || !current_language_function_) {
            return dswros::RadarUiLanguage::English;
        }
        struct CurrentLanguageParameters {
            FString return_value{};
        };
        static_assert(sizeof(CurrentLanguageParameters)
                      == kCurrentLanguageParameterCapacity);
        try {
            CurrentLanguageParameters parameters{};
            library->ProcessEvent(current_language_function_, &parameters);
            const int32 length = parameters.return_value.Len();
            if (length <= 0 || length > 63) {
                return dswros::RadarUiLanguage::English;
            }
            current_language_detection_source_ =
                "kismet_current_language";
            return dswros::radar_ui_language_from_culture(
                std::wstring_view{
                    *parameters.return_value,
                    static_cast<std::size_t>(length)});
        } catch (...) {
            return dswros::RadarUiLanguage::English;
        }
    }

    [[nodiscard]] dswros::RadarUiLanguage
    detect_current_game_language_guarded(UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try {
            return detect_current_game_language(engine);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            current_language_detection_source_ =
                "seh_fault_fallback_en";
            return dswros::RadarUiLanguage::English;
        }
#else
        return detect_current_game_language(engine);
#endif
    }

    void migrate_legacy_auto_language_preference(
        std::string_view schedule) noexcept {
        if (language_preference_
            != dswros::RadarLanguagePreference::Auto) {
            return;
        }
        language_preference_ =
            dswros::resolve_explicit_radar_language_preference(
                language_preference_, detected_game_language_);
        active_ui_language_ = dswros::resolve_radar_ui_language(
            language_preference_, detected_game_language_);
        const bool persisted = persist_visibility_settings(
            mod_directory() / "config" / "visibility.ini",
            visibility_masks_, area_quest_display_mode_,
            assault_display_mode_, height_indicator_mask_,
            language_preference_);
        try {
            append_log("RADAR_LANGUAGE_AUTO_MIGRATED", std::format(
                "schedule={} detected={} explicit={} persisted={}",
                schedule,
                dswros::radar_ui_language_id(detected_game_language_),
                dswros::radar_language_preference_id(
                    language_preference_),
                persisted));
        } catch (...) {
        }
    }

    [[nodiscard]] UObject*
    current_player_controller_for_visibility_hub(
        UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try {
            return current_player_controller(engine);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
#else
        try {
            return current_player_controller(engine);
        } catch (...) {
            return nullptr;
        }
#endif
    }

    [[nodiscard]] bool initialize_treasure_interact_schema() noexcept {
        treasure_interact_actor_property_ = nullptr;
        if (!treasure_interact_function_) {
            return false;
        }
        const std::size_t parameter_bytes = static_cast<std::size_t>(
            treasure_interact_function_->GetParmsSize());
        treasure_interact_actor_property_ = CastField<FObjectPropertyBase>(
            find_function_property(treasure_interact_function_, L"InActor"));
        const bool valid = parameter_bytes > 0
            && parameter_bytes <= kTreasureInteractParameterCapacity
            && treasure_interact_actor_property_
            && treasure_interact_actor_property_->GetOffset_Internal() >= 0
            && treasure_interact_actor_property_->GetSize() > 0
            && static_cast<std::size_t>(
                   treasure_interact_actor_property_->GetOffset_Internal()
                       + treasure_interact_actor_property_->GetSize())
                <= parameter_bytes;
        if (!valid) {
            treasure_interact_actor_property_ = nullptr;
            append_log("TREASURE_INTERACT_SCHEMA_DISABLED", std::format(
                "reason=in_actor_schema_invalid parameter_bytes={}",
                parameter_bytes));
            return false;
        }
        append_log("TREASURE_INTERACT_SCHEMA_READY", std::format(
            "function={} parameter_bytes={} identity=fresh_local_pawn",
            to_string(treasure_interact_function_->GetFullName()),
            parameter_bytes));
        return true;
    }

    void request_area_quest_scan(const char* reason) noexcept {
        if (!area_quest_state_provider_ready_
            || area_quest_scan_faulted_) {
            return;
        }
        // Refresh into a private staging snapshot. The last fully published
        // snapshot remains visible until all 147 reflected reads and ordinary
        // prerequisite reads complete. This prevents a generic quest-blueprint
        // teardown from blanking both maps for several frames.
        area_quest_scan_had_published_snapshot_ = area_quest_state_ready_;
        area_quest_scan_eligibility_.fill(0);
        area_quest_scan_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_completion_observed_ =
            area_quest_completion_observed_;
        area_quest_scan_repeatable_reactivation_armed_ =
            area_quest_repeatable_reactivation_armed_;
        area_quest_scan_start_completion_revisions_ =
            area_quest_exact_completion_revisions_;
        area_quest_scan_previous_states_ = area_quest_states_;
        area_quest_state_counts_.fill(0);
        area_quest_unknown_count_ = 0;
        area_quest_scan_total_us_ = 0;
        area_quest_scan_max_us_ = 0;
        area_quest_scan_index_ = 0;
        area_quest_prerequisite_ids_.fill(0);
        area_quest_prerequisite_scan_index_ = 0;
        area_quest_prerequisite_scan_count_ = 0;
        if (area_quest_definition_ready_) {
            const auto add_prerequisite = [this](
                const DynamicQuestCondition& condition) noexcept {
                if (condition.type != static_cast<std::uint8_t>(
                                          DynamicQuestConditionType::QuestClear)
                    || condition.value1 == 0
                    || area_quest_prerequisite_scan_count_
                        >= area_quest_prerequisite_ids_.size()) {
                    return;
                }
                const auto begin = area_quest_prerequisite_ids_.begin();
                const auto end = begin
                    + static_cast<std::ptrdiff_t>(
                        area_quest_prerequisite_scan_count_);
                if (std::find(begin, end, condition.value1) == end) {
                    area_quest_prerequisite_ids_[
                        area_quest_prerequisite_scan_count_++] =
                        condition.value1;
                }
            };
            for (const AreaQuestDefinition& definition :
                 area_quest_definitions_) {
                add_prerequisite(definition.group_condition);
                add_prerequisite(definition.accept_condition);
            }
        }
        area_quest_scan_pending_ = true;
        try {
            append_log("AREA_QUEST_STATE_SCAN_REQUESTED", std::format(
                "activation={} epoch={} reason={} catalog={} prerequisites={} schedule=one_reflected_query_per_game_frame publication=transactional preserve_previous={}",
                activation_, epoch_, reason ? reason : "unspecified",
                area_quest_catalog_.size(),
                area_quest_prerequisite_scan_count_,
                area_quest_scan_had_published_snapshot_));
        } catch (...) {
        }
    }

    [[nodiscard]] bool query_area_quest_state_guarded(
        UObject* world_context,
        std::int64_t quest_id,
        std::int64_t& state,
        bool& query_succeeded) noexcept {
#if defined(_MSC_VER)
        __try {
            query_succeeded = query_area_quest_state_unsafe(
                world_context, quest_id, state);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            query_succeeded = false;
            return false;
        }
#else
        try {
            query_succeeded = query_area_quest_state_unsafe(
                world_context, quest_id, state);
            return true;
        } catch (...) {
            query_succeeded = false;
            return false;
        }
#endif
    }

    [[nodiscard]] bool query_area_quest_state_unsafe(
        UObject* world_context,
        std::int64_t quest_id,
        std::int64_t& state) {
        UObject* utility_default = quest_utility_default_.Get();
        if (!utility_default || !world_context || quest_id <= 0
            || !quest_info_function_) {
            return false;
        }
        alignas(16) std::array<std::byte, kAreaQuestParameterCapacity>
            parameters{};
        void* world_value =
            quest_world_context_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* id_value =
            quest_id_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* dynamic_value =
            quest_dynamic_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        if (!world_value || !id_value || !dynamic_value) {
            return false;
        }
        quest_world_context_property_->SetObjectPropertyValue(
            world_value, world_context);
        quest_id_property_->SetIntPropertyValue(
            id_value, quest_id);
        // MnMRadar's 147-entry area-quest catalog contains dynamic quests.
        // Passing false selects the ordinary quest store and makes every
        // catalog lookup return false, even for currently available quests.
        quest_dynamic_property_->SetPropertyValue(dynamic_value, true);
        utility_default->ProcessEvent(
            quest_info_function_, parameters.data());

        void* state_value =
            quest_state_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* return_value =
            quest_return_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        if (!state_value || !return_value
            || !quest_return_property_->GetPropertyValue(return_value)) {
            return false;
        }
        state = quest_state_underlying_property_
            ->GetSignedIntPropertyValue(state_value);
        return state >= 0 && state <= 4;
    }

    [[nodiscard]] bool query_normal_quest_completion_guarded(
        UObject* world_context,
        std::int64_t quest_id,
        dswros::AreaQuestEligibilityProof& proof) noexcept {
#if defined(_MSC_VER)
        __try {
            return query_normal_quest_completion_unsafe(
                world_context, quest_id, proof);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            proof = dswros::AreaQuestEligibilityProof::Unknown;
            return false;
        }
#else
        try {
            return query_normal_quest_completion_unsafe(
                world_context, quest_id, proof);
        } catch (...) {
            proof = dswros::AreaQuestEligibilityProof::Unknown;
            return false;
        }
#endif
    }

    [[nodiscard]] bool query_normal_quest_completion_unsafe(
        UObject* world_context,
        std::int64_t quest_id,
        dswros::AreaQuestEligibilityProof& proof) {
        UObject* utility_default = quest_utility_default_.Get();
        if (!utility_default || !world_context || quest_id <= 0
            || !quest_info_function_) {
            proof = dswros::AreaQuestEligibilityProof::Unknown;
            return true;
        }
        alignas(16) std::array<std::byte, kAreaQuestParameterCapacity>
            parameters{};
        void* world_value =
            quest_world_context_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* id_value =
            quest_id_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* dynamic_value =
            quest_dynamic_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        if (!world_value || !id_value || !dynamic_value) {
            proof = dswros::AreaQuestEligibilityProof::Unknown;
            return true;
        }
        quest_world_context_property_->SetObjectPropertyValue(
            world_value, world_context);
        quest_id_property_->SetIntPropertyValue(id_value, quest_id);
        quest_dynamic_property_->SetPropertyValue(dynamic_value, false);
        utility_default->ProcessEvent(
            quest_info_function_, parameters.data());

        void* state_value =
            quest_state_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        void* return_value =
            quest_return_property_->ContainerPtrToValuePtr<void>(
                parameters.data());
        if (!state_value || !return_value
            || !quest_return_property_->GetPropertyValue(return_value)) {
            proof = dswros::AreaQuestEligibilityProof::Unknown;
            return true;
        }
        const std::int64_t state = quest_state_underlying_property_
            ->GetSignedIntPropertyValue(state_value);
        proof = state == static_cast<std::int64_t>(
                          dswros::AreaQuestState::End)
            ? dswros::AreaQuestEligibilityProof::Eligible
            : dswros::AreaQuestEligibilityProof::Ineligible;
        return true;
    }

    [[nodiscard]] dswros::AreaQuestEligibilityProof
    evaluate_area_quest_condition(
        const DynamicQuestCondition& condition) const noexcept {
        switch (static_cast<DynamicQuestConditionType>(condition.type)) {
        case DynamicQuestConditionType::None:
            return dswros::AreaQuestEligibilityProof::Eligible;
        case DynamicQuestConditionType::QuestClear: {
            const auto found = normal_quest_completion_proofs_.find(
                condition.value1);
            return found == normal_quest_completion_proofs_.end()
                ? dswros::AreaQuestEligibilityProof::Unknown
                : found->second;
        }
        case DynamicQuestConditionType::DynamicQuestComplete:
            // A completion proven during this activation is authoritative even
            // when the optional F7 save query was unavailable. Otherwise the
            // save snapshot must exist before absence proves ineligibility.
            if (completed_dynamic_quest_ids_.contains(condition.value1)) {
                return dswros::AreaQuestEligibilityProof::Eligible;
            }
            return area_quest_save_completion_query_available_
                ? dswros::AreaQuestEligibilityProof::Ineligible
                : dswros::AreaQuestEligibilityProof::Unknown;
        case DynamicQuestConditionType::MonsterAlive: {
            if (!encounter_state_ready_
                || condition.linked_encounter_id <= 0) {
                return dswros::AreaQuestEligibilityProof::Unknown;
            }
            const EncounterSpec* encounter = encounter_spec_for_id(
                condition.linked_encounter_id);
            if (!encounter || encounter->kind != EncounterKind::Assault
                || (encounter->has_time_condition
                    && !world_time_available_)) {
                return dswros::AreaQuestEligibilityProof::Unknown;
            }
            return encounter_available(*encounter, unix_seconds())
                ? dswros::AreaQuestEligibilityProof::Eligible
                : dswros::AreaQuestEligibilityProof::Ineligible;
        }
        default:
            return dswros::AreaQuestEligibilityProof::Unknown;
        }
    }

    [[nodiscard]] dswros::AreaQuestEligibilityProof
    evaluate_area_quest_definition(
        const AreaQuestDefinition& definition) const noexcept {
        if (!area_quest_definition_ready_ || !definition.found
            || definition.active_timing_type > 3) {
            return dswros::AreaQuestEligibilityProof::Unknown;
        }
        // A weighted group that activates fewer rows than it owns requires a
        // server/runtime selection result. Static table conditions cannot
        // prove which sibling was selected, so keep those NONE states hidden.
        if (definition.group_member_count > 1
            && definition.group_active_weight > 0) {
            return dswros::AreaQuestEligibilityProof::Unknown;
        }
        const auto group = evaluate_area_quest_condition(
            definition.group_condition);
        const auto accept = evaluate_area_quest_condition(
            definition.accept_condition);
        if (group == dswros::AreaQuestEligibilityProof::Ineligible
            || accept == dswros::AreaQuestEligibilityProof::Ineligible) {
            return dswros::AreaQuestEligibilityProof::Ineligible;
        }
        return group == dswros::AreaQuestEligibilityProof::Eligible
                && accept == dswros::AreaQuestEligibilityProof::Eligible
            ? dswros::AreaQuestEligibilityProof::Eligible
            : dswros::AreaQuestEligibilityProof::Unknown;
    }

    void process_one_area_quest(UEngine* engine) noexcept {
        if (!area_quest_scan_pending_ || area_quest_scan_faulted_
            || !position_valid_
            || activity_suppressed_ || !enabled_ || transition_active_) {
            return;
        }
        if (area_quest_scan_index_ >= area_quest_catalog_.size()) {
            if (area_quest_prerequisite_scan_index_
                >= area_quest_prerequisite_scan_count_) {
                finish_area_quest_scan();
                return;
            }
            UObject* world_context = current_player_controller(engine);
            if (!world_context) {
                return;
            }
            const std::uint32_t prerequisite_id =
                area_quest_prerequisite_ids_[
                    area_quest_prerequisite_scan_index_];
            dswros::AreaQuestEligibilityProof proof{
                dswros::AreaQuestEligibilityProof::Unknown};
            const auto started = Clock::now();
            const bool safe = query_normal_quest_completion_guarded(
                world_context, prerequisite_id, proof);
            const auto elapsed_us = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    Clock::now() - started).count());
            area_quest_scan_total_us_ += elapsed_us;
            area_quest_scan_max_us_ = std::max(
                area_quest_scan_max_us_, elapsed_us);
            normal_quest_completion_proofs_.insert_or_assign(
                prerequisite_id, proof);
            ++area_quest_prerequisite_query_count_;
            if (!safe) {
                ++area_quest_prerequisite_fault_count_;
            }
            ++area_quest_prerequisite_scan_index_;
            if (area_quest_prerequisite_scan_index_
                >= area_quest_prerequisite_scan_count_) {
                finish_area_quest_scan();
            }
            return;
        }
        UObject* world_context = current_player_controller(engine);
        if (!world_context) {
            return;
        }
        const std::size_t index = area_quest_scan_index_;
        const AreaQuestSpec& spec = area_quest_catalog_[index];
        std::int64_t state{};
        bool query_succeeded{};
        const auto started = Clock::now();
        const bool safe = query_area_quest_state_guarded(
            world_context, spec.id, state, query_succeeded);
        const auto elapsed_us = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - started).count());
        area_quest_scan_total_us_ += elapsed_us;
        area_quest_scan_max_us_ = std::max(
            area_quest_scan_max_us_, elapsed_us);
        if (!safe) {
            area_quest_scan_pending_ = false;
            area_quest_scan_faulted_ = true;
            // A refresh fault cannot invalidate an already completed numeric
            // snapshot. On first activation there is no prior proof, so area
            // quests still fail closed. Automatic retry remains disabled for
            // the rest of this activation.
            if (!area_quest_scan_had_published_snapshot_) {
                area_quest_state_ready_ = true;
                area_quest_eligibility_.fill(0);
                area_quest_world_map_eligibility_.fill(0);
                area_quest_states_.fill(dswros::AreaQuestState::Unknown);
                compact_rebind_dirty_ = true;
                refresh_world_map_atlas_for_runtime_delta(
                    "area_quest_scan_failed", spec.id);
            }
            append_log("AREA_QUEST_STATE_SCAN_FAILED", std::format(
                "activation={} epoch={} index={} id={} reason=structured_exception published_snapshot_preserved={} retry=next_f8_f7",
                activation_, epoch_, index, spec.id,
                area_quest_scan_had_published_snapshot_));
            return;
        }
        const auto decoded_state = query_succeeded
            ? static_cast<dswros::AreaQuestState>(state)
            : dswros::AreaQuestState::Unknown;
        const bool scan_revision_is_current =
            area_quest_scan_start_completion_revisions_[index]
            == area_quest_exact_completion_revisions_[index];
        const bool decoded_active =
            dswros::area_quest_compact_visible(decoded_state);
        if (dswros::area_quest_completion_generation_arms_reactivation(
                area_quest_completion_generation_locked_[index],
                decoded_state)) {
            area_quest_completion_generation_reactivation_armed_[index] =
                true;
        }
        if (scan_revision_is_current
            && dswros::area_quest_completion_generation_may_unlock(
                area_quest_completion_generation_locked_[index],
                area_quest_completion_generation_reactivation_armed_[index],
                decoded_state)) {
            area_quest_completion_generation_locked_[index] = false;
            area_quest_completion_generation_reactivation_armed_[index] =
                false;
            try {
                append_log("AREA_QUEST_COMPLETION_GENERATION_REARMED",
                    std::format(
                        "activation={} epoch={} id={} catalog_index={} evidence=inactive_none_or_end_then_active",
                        activation_, epoch_, spec.id, index));
            } catch (...) {
            }
        }
        const bool active_sample_may_rearm =
            dswros::area_quest_active_sample_may_rearm(
                area_quest_scan_start_completion_revisions_[index],
                area_quest_exact_completion_revisions_[index],
                area_quest_scan_repeatable_reactivation_armed_[index]);
        const bool preserve_exact_completion =
            !scan_revision_is_current
            || (decoded_active
                && area_quest_scan_completion_observed_[index]
                && !active_sample_may_rearm);
        if (!preserve_exact_completion) {
            area_quest_scan_states_[index] = decoded_state;
            const auto previous_state =
                area_quest_scan_previous_states_[index];
            const bool generic_completion_transition =
                dswros::area_quest_completion_transition(
                    previous_state, decoded_state);
            const bool witnessed_completion_transition =
                Clock::now()
                    <= area_quest_completion_witness_deadlines_[index]
                && dswros::area_quest_witnessed_completion_transition(
                    area_quest_completion_witnesses_[index],
                    previous_state, decoded_state);
            if (generic_completion_transition
                || witnessed_completion_transition) {
                // The generic blueprint-end callback also fires for unrelated
                // nearby treasure and mini-games. Runtime evidence proves that
                // an unrelated teardown can expose PROGRESS -> FAIL. END is
                // therefore accepted from NONE/FAIL/ACCEPTABLE/PROGRESS only
                // while the exact task actor has armed this catalog row. An
                // exact OnRecvCompleteQuest event is handled separately through
                // the exact task-class map.
                area_quest_scan_completion_observed_[index] = true;
                area_quest_scan_repeatable_reactivation_armed_[index] = true;
                area_quest_completion_generation_locked_[index] = true;
                area_quest_completion_generation_reactivation_armed_[index] =
                    true;
                completed_dynamic_quest_ids_.insert(spec.id);
                area_quest_completion_witnesses_[index] = false;
                area_quest_completion_witness_deadlines_[index] = {};
                area_quest_completion_probe_after_[index] = {};
                area_quest_completion_probe_counts_[index] = 0;
                if (witnessed_completion_transition) {
                    try {
                        append_log(
                            "AREA_QUEST_WITNESSED_COMPLETION_OBSERVED",
                            std::format(
                                "activation={} epoch={} id={} catalog_index={} evidence=exact_task_actor_then_end_state",
                                activation_, epoch_, spec.id, index));
                    } catch (...) {
                    }
                }
            } else if (
                decoded_state == dswros::AreaQuestState::Acceptable
                || decoded_state == dswros::AreaQuestState::Progress) {
                // A currently active dynamic quest is authoritative even if
                // the same repeatable quest ID completed earlier in this
                // activation. A revision fence below prevents an older scan
                // from applying this rule after a newer exact completion.
                if (area_quest_scan_completion_observed_[index]
                    && active_sample_may_rearm) {
                    area_quest_scan_completion_observed_[index] = false;
                    area_quest_scan_repeatable_reactivation_armed_[index] =
                        false;
                }
            } else if (
                area_quest_scan_completion_observed_[index]
                && dswros::area_quest_inactive_sample_arms_reactivation(
                    decoded_state)) {
                // Exact completion is already authoritative. A later NONE or
                // END proves that cycle settled, so only a still later active
                // sample may revive a repeatable task. FAIL remains non-proof.
                area_quest_scan_repeatable_reactivation_armed_[index] = true;
            }
            area_quest_scan_eligibility_[index] =
                static_cast<std::uint8_t>(
                    dswros::area_quest_compact_visible(decoded_state)
                        ? 1 : 0);
        } else {
            // An exact completion arrived after this transactional scan began.
            // Preserve the newer result in staging so finish_area_quest_scan()
            // cannot publish an older ACCEPTABLE/PROGRESS sample over it.
            area_quest_scan_states_[index] = dswros::AreaQuestState::End;
            area_quest_scan_completion_observed_[index] = true;
            area_quest_scan_repeatable_reactivation_armed_[index] = false;
            area_quest_scan_eligibility_[index] = 0;
        }
        if (query_succeeded && state >= 0 && state <= 4) {
            ++area_quest_state_counts_[static_cast<std::size_t>(state)];
        } else {
            ++area_quest_unknown_count_;
        }
        ++area_quest_scan_index_;
        if (area_quest_scan_index_ >= area_quest_catalog_.size()) {
            if (area_quest_prerequisite_scan_count_ == 0) {
                finish_area_quest_scan();
            }
        }
    }

    [[nodiscard]] std::size_t
    rebuild_area_quest_world_map_eligibility() noexcept {
        std::size_t visible{};
        area_quest_static_proof_counts_.fill(0);
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const auto proof = evaluate_area_quest_definition(
                area_quest_definitions_[index]);
            area_quest_static_proofs_[index] = proof;
            ++area_quest_static_proof_counts_[
                static_cast<std::size_t>(proof)];
            const bool eligible = dswros::area_quest_world_map_visible(
                area_quest_states_[index],
                area_quest_save_completion_query_available_,
                area_quest_save_completion_[index] != 0,
                area_quest_completion_observed_[index], proof);
            area_quest_world_map_eligibility_[index] =
                static_cast<std::uint8_t>(eligible ? 1 : 0);
            visible += eligible ? 1U : 0U;
        }
        return visible;
    }

    void finish_area_quest_scan() noexcept {
        area_quest_scan_pending_ = false;
        const auto previous_compact = area_quest_eligibility_;
        const auto previous_world_map = area_quest_world_map_eligibility_;
        area_quest_states_ = area_quest_scan_states_;
        area_quest_eligibility_ = area_quest_scan_eligibility_;
        area_quest_completion_observed_ =
            area_quest_scan_completion_observed_;
        area_quest_repeatable_reactivation_armed_ =
            area_quest_scan_repeatable_reactivation_armed_;
        area_quest_state_ready_ = true;
        const std::size_t world_map_visible =
            rebuild_area_quest_world_map_eligibility();
        const bool compact_changed =
            previous_compact != area_quest_eligibility_
            || previous_world_map != area_quest_world_map_eligibility_;
        const bool world_map_changed =
            previous_world_map != area_quest_world_map_eligibility_;
        compact_rebind_dirty_ = compact_rebind_dirty_ || compact_changed;
        const std::size_t visible = static_cast<std::size_t>(
            std::count(area_quest_eligibility_.begin(),
                       area_quest_eligibility_.end(),
                       static_cast<std::uint8_t>(1)));
        if (world_map_changed) {
            refresh_world_map_atlas_for_runtime_delta(
                "area_quest_scan", 0);
        }
        try {
            append_log("AREA_QUEST_STATE_SCAN_COMPLETE", std::format(
                "activation={} epoch={} visible={} world_map_visible={} save_completion_matches={} save_completion_query={} definitions_ready={} definition_matches={} proof_unknown={} proof_eligible={} proof_ineligible={} prerequisite_queries={} prerequisite_faults={} none={} acceptable={} progress={} end={} fail={} unknown={} queries={} total_us={} max_us={} publication=transactional compact_changed={} world_map_changed={}",
                activation_, epoch_, visible, world_map_visible,
                area_quest_save_completion_match_count_,
                area_quest_save_completion_query_available_,
                area_quest_definition_ready_,
                area_quest_definition_match_count_,
                area_quest_static_proof_counts_[0],
                area_quest_static_proof_counts_[1],
                area_quest_static_proof_counts_[2],
                area_quest_prerequisite_query_count_,
                area_quest_prerequisite_fault_count_,
                area_quest_state_counts_[0],
                area_quest_state_counts_[1],
                area_quest_state_counts_[2],
                area_quest_state_counts_[3],
                area_quest_state_counts_[4],
                area_quest_unknown_count_, area_quest_scan_index_,
                area_quest_scan_total_us_, area_quest_scan_max_us_,
                compact_changed, world_map_changed));
        } catch (...) {
        }
    }

    [[nodiscard]] bool game_thread() noexcept {
        const DWORD id = GetCurrentThreadId();
        DWORD expected{};
        static_cast<void>(game_thread_id_.compare_exchange_strong(
            expected, id, std::memory_order_acq_rel));
        return game_thread_id_.load(std::memory_order_acquire) == id;
    }

    [[nodiscard]] static std::int64_t unix_seconds() noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(
            SystemClock::now().time_since_epoch()).count();
    }

    [[nodiscard]] const EncounterSpec* encounter_spec_for_class(
        const std::string& class_name) const noexcept {
        const auto found = encounter_class_indices_.find(class_name);
        if (found == encounter_class_indices_.end()
            || found->second >= encounter_catalog_.size()) {
            return nullptr;
        }
        return &encounter_catalog_[found->second];
    }

    [[nodiscard]] const EncounterSpec* encounter_spec_for_id(
        std::int64_t id) const noexcept {
        const auto found = std::find_if(
            encounter_catalog_.begin(), encounter_catalog_.end(),
            [id](const EncounterSpec& spec) { return spec.id == id; });
        return found == encounter_catalog_.end() ? nullptr : &*found;
    }

    [[nodiscard]] const char* encounter_type_for_id(
        std::int64_t id) const noexcept {
        const EncounterSpec* spec = encounter_spec_for_id(id);
        return spec ? encounter_kind_name(spec->kind) : "unknown";
    }

    [[nodiscard]] bool encounter_time_condition_matches(
        const EncounterSpec& spec) const noexcept {
        if (!spec.has_time_condition) {
            return true;
        }
        if (!world_time_available_) {
            return false;
        }
        const auto hour = static_cast<std::int32_t>(
            current_world_time_seconds() / 3600U);
        return encounter_time_condition_matches_at_hour(spec, hour);
    }

    [[nodiscard]] static bool encounter_time_condition_matches_at_hour(
        const EncounterSpec& spec, std::int32_t hour) noexcept {
        if (!spec.has_time_condition) {
            return true;
        }
        if (hour < 0 || hour >= 24) {
            return false;
        }
        if (spec.visible_from_hour < spec.hidden_from_hour) {
            return hour >= spec.visible_from_hour
                && hour < spec.hidden_from_hour;
        }
        return hour >= spec.visible_from_hour
            || hour < spec.hidden_from_hour;
    }

    [[nodiscard]] bool encounter_available(
        const EncounterSpec& spec,
        std::int64_t now_unix_seconds) const noexcept {
        const auto found = encounter_next_available_unix_seconds_.find(spec.id);
        return dswros::encounter_available_now(
            encounter_state_ready_, encounter_time_condition_matches(spec),
            found != encounter_next_available_unix_seconds_.end(),
            found == encounter_next_available_unix_seconds_.end()
                ? 0 : found->second,
            now_unix_seconds);
    }

    [[nodiscard]] bool encounter_visible_for_selected_mode(
        const EncounterSpec& spec,
        std::int64_t now_unix_seconds) const noexcept {
        if (spec.kind == EncounterKind::Assault
            && assault_display_mode_ == dsnwr::AssaultDisplayMode::All) {
            return true;
        }
        const auto found = encounter_next_available_unix_seconds_.find(spec.id);
        return dswros::encounter_visible_for_display_mode(
            encounter_state_ready_, spec.kind == EncounterKind::Assault,
            assault_display_mode_ == dsnwr::AssaultDisplayMode::All,
            encounter_time_condition_matches(spec),
            found != encounter_next_available_unix_seconds_.end(),
            found == encounter_next_available_unix_seconds_.end()
                ? 0 : found->second,
            now_unix_seconds);
    }

    [[nodiscard]] bool encounter_visible_for_selected_mode_at_hour(
        const EncounterSpec& spec, std::int64_t now_unix_seconds,
        std::int32_t world_hour) const noexcept {
        if (spec.kind == EncounterKind::Assault
            && assault_display_mode_ == dsnwr::AssaultDisplayMode::All) {
            return true;
        }
        const auto found = encounter_next_available_unix_seconds_.find(spec.id);
        return dswros::encounter_visible_for_display_mode(
            encounter_state_ready_, spec.kind == EncounterKind::Assault,
            assault_display_mode_ == dsnwr::AssaultDisplayMode::All,
            encounter_time_condition_matches_at_hour(spec, world_hour),
            found != encounter_next_available_unix_seconds_.end(),
            found == encounter_next_available_unix_seconds_.end()
                ? 0 : found->second,
            now_unix_seconds);
    }

    [[nodiscard]] std::uint64_t encounter_visibility_mask(
        std::int64_t now_unix_seconds,
        std::int32_t world_hour) const noexcept {
        std::uint64_t mask{};
        const std::size_t count = std::min(
            encounter_catalog_.size(), kExpectedEncounterCount);
        for (std::size_t index = 0; index < count; ++index) {
            const EncounterSpec& spec = encounter_catalog_[index];
            const bool available = encounter_visible_for_selected_mode_at_hour(
                spec, now_unix_seconds, world_hour);
            if (available) {
                mask |= std::uint64_t{1} << index;
            }
        }
        return mask;
    }

    void recompute_next_encounter_cooldown_edge(
        std::int64_t now_unix_seconds) noexcept {
        next_encounter_cooldown_edge_unix_seconds_ = 0;
        for (const auto& [id, next_available] :
             encounter_next_available_unix_seconds_) {
            static_cast<void>(id);
            if (next_available <= now_unix_seconds) {
                continue;
            }
            if (next_encounter_cooldown_edge_unix_seconds_ == 0
                || next_available
                    < next_encounter_cooldown_edge_unix_seconds_) {
                next_encounter_cooldown_edge_unix_seconds_ = next_available;
            }
        }
    }

    void establish_encounter_visibility_baseline(
        std::int64_t now_unix_seconds) noexcept {
        const std::int32_t world_hour = world_time_available_
            ? static_cast<std::int32_t>(
                  current_world_time_seconds() / 3600U)
            : -1;
        encounter_visibility_mask_ = encounter_visibility_mask(
            now_unix_seconds, world_hour);
        encounter_visibility_mask_valid_ = encounter_state_ready_;
        recompute_next_encounter_cooldown_edge(now_unix_seconds);
    }

    [[nodiscard]] bool read_world_map_layer_visibility_guarded(
        UObject* current_layer, bool* visible) noexcept {
        if (!visible || !current_layer
            || !widget_is_visible_schema_ready_
            || !widget_is_visible_function_
            || !widget_is_visible_return_property_) {
            return false;
        }
#if defined(_MSC_VER)
        __try {
            alignas(16) std::array<std::byte,
                kWidgetIsVisibleParameterCapacity> parameters{};
            current_layer->ProcessEvent(
                widget_is_visible_function_, parameters.data());
            void* return_value = widget_is_visible_return_property_
                ->ContainerPtrToValuePtr<void>(parameters.data());
            if (!return_value) {
                return false;
            }
            *visible = widget_is_visible_return_property_->GetPropertyValue(
                return_value);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        try {
            alignas(16) std::array<std::byte,
                kWidgetIsVisibleParameterCapacity> parameters{};
            current_layer->ProcessEvent(
                widget_is_visible_function_, parameters.data());
            void* return_value = widget_is_visible_return_property_
                ->ContainerPtrToValuePtr<void>(parameters.data());
            if (!return_value) {
                return false;
            }
            *visible = widget_is_visible_return_property_->GetPropertyValue(
                return_value);
            return true;
        } catch (...) {
            return false;
        }
#endif
    }

    [[nodiscard]] bool world_map_candidate_has_open_evidence() const noexcept {
        return world_map_candidate_available_
            && world_map_candidate_serial_ != 0
            && world_map_open_serial_ == world_map_candidate_serial_;
    }

    [[nodiscard]] bool world_map_candidate_confirmed_visible() const noexcept {
        return world_map_candidate_has_open_evidence()
            && world_map_visible_serial_ == world_map_candidate_serial_;
    }

    [[nodiscard]] bool world_map_layer_open_and_visible_guarded(
        UWorld* current_world, UObject* current_layer) noexcept {
        if (!enabled_ || transition_active_ || activity_suppressed_
            || !world_map_compact_suppressed_
            || !current_world || !current_layer
            || !world_map_candidate_has_open_evidence()
            || world_map_layer_candidate_.Get() != current_layer
            || object_world_guarded(current_layer) != current_world) {
            return false;
        }

        bool native_layer_visible{};
        return read_world_map_layer_visibility_guarded(
                   current_layer, &native_layer_visible)
            && native_layer_visible;
    }

    [[nodiscard]] bool world_map_atlas_visibility_allowed_guarded(
        UWorld* current_world, UObject* current_layer) noexcept {
        return world_map_layer_open_and_visible_guarded(
                   current_world, current_layer)
            && world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Attached
            && world_map_umg_renderer_.attached_layer_matches(current_layer);
    }

    void apply_world_map_atlas_visibility_guarded(
        UWorld* current_world, UObject* current_layer) noexcept {
        // Independent viewport hosts do not inherit the game layer's
        // visibility. Show them only while the exact attached layer remains a
        // visible member of the current world. The renderer adds the final
        // geometry-valid gate before changing either host to Visible.
        world_map_umg_renderer_.publish_runtime_visibility(
            world_map_atlas_visibility_allowed_guarded(
                current_world, current_layer));
    }

    [[nodiscard]] bool read_game_paused_guarded(
        UObject* world_context, bool* paused) noexcept {
        if (!paused || !world_context || !game_pause_schema_ready_
            || !is_game_paused_function_
            || !game_pause_world_context_property_
            || !game_pause_return_property_) {
            return false;
        }
#if defined(_MSC_VER)
        __try {
#else
        try {
#endif
            UObject* gameplay_statics = gameplay_statics_default_.Get();
            if (!gameplay_statics) {
                return false;
            }
            alignas(16) std::array<std::byte,
                kIsGamePausedParameterCapacity> parameters{};
            void* world_value = game_pause_world_context_property_
                ->ContainerPtrToValuePtr<void>(parameters.data());
            void* return_value = game_pause_return_property_
                ->ContainerPtrToValuePtr<void>(parameters.data());
            if (!world_value || !return_value) {
                return false;
            }
            game_pause_world_context_property_->SetObjectPropertyValue(
                world_value, world_context);
            gameplay_statics->ProcessEvent(
                is_game_paused_function_, parameters.data());
            *paused = game_pause_return_property_->GetPropertyValue(
                return_value);
            return true;
#if defined(_MSC_VER)
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        } catch (...) {
            return false;
        }
#endif
    }

    void report_compact_menu_state_edge(
        const char* source, bool pause_known) noexcept {
        try {
            append_log("COMPACT_MENU_STATE", std::format(
                "activation={} epoch={} source={} cursor={} world_map_visible={} game_paused={} pause_known={} activity_suppressed={} compact_suppressed={}",
                activation_, epoch_, source ? source : "unknown",
                mouse_cursor_visible_, world_map_compact_suppressed_,
                game_paused_, pause_known, activity_suppressed_,
                compact_render_suppressed()));
        } catch (...) {
        }
    }

    void latch_world_map_compact_suppression(
        const char* source) noexcept {
        if (!enabled_ || transition_active_ || activity_suppressed_
            || !widget_is_visible_schema_ready_) {
            return;
        }
        // This latch controls only compact-radar suppression. World-map host
        // visibility is handled against the exact layer identity in
        // capture_world_map_candidate_unsafe(); repeated SetWorldMapImage calls
        // for the same live layer must not hide a valid atlas between bounded
        // transform observations.
        if (world_map_compact_suppressed_) {
            return;
        }
        world_map_compact_suppressed_ = true;
        apply_compact_suppression();
        report_compact_menu_state_edge(source, game_pause_sample_known_);
    }

    void clear_world_map_open_evidence() noexcept {
        world_map_compact_suppressed_ = false;
        world_map_open_serial_ = 0;
        world_map_visible_serial_ = 0;
        world_map_open_evidence_at_ = {};
        world_map_session_pending_ = false;
        world_map_service_retry_after_ = {};
        world_map_readiness_attempts_ = 0;
        world_map_service_attempts_ = 0;
        world_map_serviced_serial_ = 0;
        world_map_renderer_session_started_ = false;
        world_map_set_image_rearm_consumed_ = false;
        world_map_layering_refresh_pending_ = false;
        world_map_layering_refresh_started_ = {};
        world_map_layering_refresh_due_ = {};
        world_map_layering_refresh_serial_ = 0;
        world_map_layering_refresh_attempt_ = 0;
        world_map_umg_renderer_.publish_runtime_visibility(false);
    }

    void refresh_compact_menu_state(
        UWorld* current_world, const char* source) noexcept {
        const bool previous_world_map = world_map_compact_suppressed_;
        const bool previous_pause = game_paused_;
        const bool previous_pause_known = game_pause_sample_known_;

        bool paused{};
        const bool pause_known = read_game_paused_guarded(
            reinterpret_cast<UObject*>(current_world), &paused);
        game_pause_sample_known_ = pause_known;
        if (pause_known) {
            game_paused_ = paused;
        }

        // IsVisible is authoritative only after SetWorldMapImage has supplied
        // the exact map-open edge. A constructed DLayerMap can report visible
        // before the player has ever opened it, so activation/travel catch-up
        // must never create this latch from IsVisible alone.
        bool authoritative_world_map_close{};
        if (widget_is_visible_schema_ready_
            && world_map_candidate_has_open_evidence()) {
            UObject* current_layer = world_map_candidate_available_
                ? world_map_layer_candidate_.Get() : nullptr;
            if (world_map_candidate_available_ && !current_layer) {
                // The weak layer was destroyed, so it can no longer prove an
                // open map. An IsVisible call that faults on a still-live
                // layer remains fail-closed without deleting open evidence.
                world_map_compact_suppressed_ = false;
                authoritative_world_map_close = true;
            }
            const bool exact_current_world_layer = current_layer
                && current_world
                && object_world_guarded(current_layer) == current_world;
            bool visible{};
            if (exact_current_world_layer
                && read_world_map_layer_visibility_guarded(
                    current_layer, &visible)) {
                if (visible) {
                    const bool first_visible_confirmation =
                        world_map_visible_serial_
                            != world_map_candidate_serial_;
                    world_map_visible_serial_ = world_map_candidate_serial_;
                    world_map_compact_suppressed_ = true;
                    if (first_visible_confirmation
                        && world_map_content_visibility_intent()
                        && (world_map_umg_renderer_.state()
                                != dsnwr::WorldMapUmgRendererState::Attached
                            || !world_map_umg_renderer_
                                    .attached_layer_matches(current_layer))) {
                        // SetWorldMapImage can precede the widget's first true
                        // IsVisible sample. F7/reset may run in that transition
                        // and intentionally discard the premature request. The
                        // first authoritative visible edge restores exactly one
                        // current-session request without resetting either
                        // finite budget.
                        world_map_session_pending_ = true;
                        world_map_service_retry_after_ = {};
                    }
                } else {
                    const bool confirmed_visible_this_session =
                        world_map_visible_serial_
                            == world_map_candidate_serial_;
                    const bool opening_grace_expired =
                        world_map_open_evidence_at_ != Clock::time_point{}
                        && Clock::now() - world_map_open_evidence_at_
                            >= kWorldMapOpenVisibilityGrace;
                    if (confirmed_visible_this_session
                        || opening_grace_expired) {
                        world_map_compact_suppressed_ = false;
                        authoritative_world_map_close = true;
                    }
                }
            }
        }

        if (authoritative_world_map_close) {
            // A real close edge retires the SetWorldMapImage evidence and all
            // finite work budgets for this open session. The same retained
            // DLayerMap may be reused later, but it needs a fresh authoritative
            // SetWorldMapImage event before any atlas work can run again.
            clear_world_map_open_evidence();
        }

        // Unknown visibility is fail-closed for the independent atlas hosts,
        // while the compact-suppression latch remains conservative until an
        // authoritative IsVisible result or destroyed weak layer closes it.
        UObject* current_layer = world_map_candidate_available_
            ? world_map_layer_candidate_.Get() : nullptr;
        apply_world_map_atlas_visibility_guarded(
            current_world, current_layer);

        if (previous_world_map != world_map_compact_suppressed_
            || previous_pause != game_paused_
            || previous_pause_known != game_pause_sample_known_) {
            apply_compact_suppression();
            report_compact_menu_state_edge(source, pause_known);
        }
    }

    void refresh_world_map_atlas_for_runtime_delta(
        const char* reason, std::int64_t id) noexcept {
        world_map_marker_snapshot_built_ = false;
        const bool content_visible = world_map_content_visibility_intent();
        const bool attached = world_map_umg_renderer_.state()
            == dsnwr::WorldMapUmgRendererState::Attached;
        const bool already_pending = world_map_session_pending_;
        const bool candidate_available = world_map_candidate_available_;
        UObject* current_layer = candidate_available
            ? current_world_map_layer_guarded() : nullptr;
        const bool exact_attachment = current_layer
            && world_map_umg_renderer_.attached_to(
                current_layer, world_map_umg_renderer_.map_id());
        UWorld* candidate_world = current_layer
            ? object_world_guarded(current_layer) : nullptr;
        const bool visibly_open =
            world_map_layer_open_and_visible_guarded(
                candidate_world, current_layer);
        const char* action = "next_map_session";
        if (!content_visible) {
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            world_map_layering_refresh_pending_ = false;
            action = "content_disabled";
        } else if (visibly_open) {
            world_map_umg_renderer_.begin_activation();
            reset_world_map_runtime(true);
            action = world_map_session_pending_
                ? "bounded_visible_session_rebuild"
                : "next_map_session_no_candidate";
        } else if (attached) {
            // Attached is not equivalent to visibly open: the game may retain
            // a collapsed DLayerMap. Detach the stale atlas now, but do not
            // build a 40-100 ms replacement until the next real
            // SetWorldMapImage edge proves that the map is open again.
            world_map_umg_renderer_.begin_activation();
            reset_world_map_runtime(true);
            world_map_session_pending_ = false;
            action = "deferred_until_set_world_map_image";
        } else if (already_pending) {
            // Multiple state changes before the pending bounded attach are
            // coalesced into its not-yet-built marker snapshot.
            action = "coalesced_pending_current_session";
        }
        try {
            append_log("WORLD_MAP_RUNTIME_DELTA", std::format(
                "activation={} epoch={} reason={} id={} content_visible={} attached={} candidate={} exact_attachment={} visibly_open={} already_pending={} action={}",
                activation_, epoch_, reason ? reason : "unknown", id,
                content_visible, attached, candidate_available,
                exact_attachment,
                visibly_open, already_pending, action));
        } catch (...) {
        }
    }

    [[nodiscard]] bool apply_encounter_defeat_state(
        const EncounterSpec& spec,
        bool lifecycle_boundary,
        const char* evidence) {
        const auto now_unix_seconds = unix_seconds();
        const auto existing =
            encounter_next_available_unix_seconds_.find(spec.id);
        if (!dswros::encounter_cooldown_write_allowed(
                existing != encounter_next_available_unix_seconds_.end(),
                existing == encounter_next_available_unix_seconds_.end()
                    ? 0 : existing->second,
                now_unix_seconds)) {
            try {
                append_log("ENCOUNTER_RUNTIME_STATE_SKIPPED", std::format(
                    "activation={} epoch={} id={} encounter_type={} evidence={} reason=active_future_cooldown",
                    activation_, epoch_, spec.id,
                    encounter_kind_name(spec.kind),
                    evidence ? evidence : "unknown"));
            } catch (...) {
            }
            return false;
        }
        const auto cooldown_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(
                kEncounterCooldown).count();
        encounter_next_available_unix_seconds_[spec.id] =
            now_unix_seconds + cooldown_seconds;
        encounter_state_ready_ = true;
        establish_encounter_visibility_baseline(now_unix_seconds);
        if (area_quest_state_ready_) {
            (void)rebuild_area_quest_world_map_eligibility();
        }
        compact_rebind_dirty_ = true;
        bool atlas_was_attached{};
        if (!lifecycle_boundary) {
            atlas_was_attached = world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Attached;
            refresh_world_map_atlas_for_runtime_delta(
                "encounter_defeated", spec.id);
        }
        try {
            append_log("ENCOUNTER_RUNTIME_STATE_APPLIED", std::format(
                "activation={} epoch={} id={} encounter_type={} evidence={} cooldown_minutes=120 compact_refresh=next_update world_map_refresh={} rebuild={}",
                activation_, epoch_, spec.id,
                encounter_kind_name(spec.kind),
                evidence ? evidence : "unknown",
                lifecycle_boundary ? "deferred_by_lifecycle"
                    : (atlas_was_attached ? "invalidated" : "not_attached"),
                lifecycle_boundary ? "next_valid_lifecycle"
                    : "current_session_if_open_else_next_map_session"));
        } catch (...) {
        }
        return true;
    }

    [[nodiscard]] bool mark_encounter_defeated(std::int64_t id) {
        if (id <= 0) {
            return false;
        }
        const EncounterSpec* spec = encounter_spec_for_id(id);
        if (!spec || !encounter_state_ready_
            || !encounter_time_condition_matches(*spec)) {
            try {
                append_log("ENCOUNTER_RUNTIME_STATE_SKIPPED", std::format(
                    "activation={} epoch={} id={} encounter_type={} reason=not_currently_available",
                    activation_, epoch_, id, encounter_type_for_id(id)));
            } catch (...) {
            }
            return false;
        }
        return apply_encounter_defeat_state(
            *spec, false, "bounded_disappearance_or_destroyed_end");
    }

    void reset_area_quest_task_class_capture(
        bool capture_pending) noexcept {
        area_quest_task_class_indices_.clear();
        area_quest_task_class_map_ready_ = false;
        area_quest_task_class_map_capture_pending_ = capture_pending;
        area_quest_task_class_map_attempt_count_ = 0;
        area_quest_task_class_map_retry_after_ = Clock::time_point{};
        area_quest_task_class_row_count_ = 0;
        area_quest_dynamic_task_class_row_count_ = 0;
        area_quest_task_class_ambiguity_count_ = 0;
    }

    void reset_area_quest_task_class_runtime(
        bool capture_pending,
        bool clear_completion_witnesses = true) noexcept {
        reset_area_quest_task_class_capture(capture_pending);
        area_quest_exact_completion_revisions_.fill(0);
        area_quest_scan_start_completion_revisions_.fill(0);
        area_quest_repeatable_reactivation_armed_.fill(false);
        area_quest_scan_repeatable_reactivation_armed_.fill(false);
        for (auto& word : area_quest_exact_completion_bits_) {
            word.store(0, std::memory_order_release);
        }
        area_quest_unmapped_completion_requests_.store(
            0, std::memory_order_release);
        if (clear_completion_witnesses) {
            area_quest_completion_witnesses_.fill(false);
            area_quest_completion_witness_queued_.fill(false);
            area_quest_completion_witness_deadlines_.fill(Clock::time_point{});
            area_quest_completion_probe_after_.fill(Clock::time_point{});
            area_quest_completion_probe_counts_.fill(0);
            area_quest_completion_witness_queue_count_ = 0;
        }
    }

    [[nodiscard]] std::optional<std::size_t>
    area_quest_catalog_index(std::int64_t quest_id) const noexcept {
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            if (area_quest_catalog_[index].id == quest_id) {
                return index;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool arm_area_quest_completion_witness(
        std::size_t exact_index,
        bool exact_identity_proven,
        const char* evidence) noexcept {
        if (!exact_identity_proven || !enabled_ || transition_active_
            || exact_index >= area_quest_catalog_.size()) {
            return false;
        }
        if (!dswros::area_quest_completion_generation_may_arm(
                area_quest_completion_generation_locked_[exact_index])) {
            try {
                append_log(
                    "AREA_QUEST_COMPLETION_WITNESS_SUPPRESSED",
                    std::format(
                        "activation={} epoch={} evidence={} id={} catalog_index={} reason=same_cycle_generation_locked unlock=inactive_none_or_end_then_active",
                        activation_, epoch_, evidence ? evidence : "unknown",
                        area_quest_catalog_[exact_index].id, exact_index));
            } catch (...) {
            }
            return false;
        }
        const auto now = Clock::now();
        if (area_quest_completion_witnesses_[exact_index]
            && now <= area_quest_completion_witness_deadlines_[exact_index]) {
            // Repeated events in the same exact quest transaction do not
            // extend its deadline or enqueue another probe generation.
            return true;
        }
        area_quest_completion_witnesses_[exact_index] = true;
        area_quest_completion_witness_deadlines_[exact_index] =
            now + kAreaQuestCompletionWitnessWindow;
        area_quest_completion_probe_after_[exact_index] =
            now + kAreaQuestCompletionProbeInterval;
        area_quest_completion_probe_counts_[exact_index] = 0;
        if (area_quest_completion_witness_queued_[exact_index]) {
            // An exact completion may have cleared the prior witness before
            // its fixed queue slot was serviced. Reuse that slot instead of
            // appending a duplicate index.
            return true;
        }
        if (area_quest_completion_witness_queue_count_
                >= area_quest_completion_witness_queue_.size()) {
            area_quest_completion_witnesses_[exact_index] = false;
            area_quest_completion_witness_deadlines_[exact_index] = {};
            area_quest_completion_probe_after_[exact_index] = {};
            return false;
        }
        area_quest_completion_witness_queue_[
            area_quest_completion_witness_queue_count_++] = exact_index;
        area_quest_completion_witness_queued_[exact_index] = true;
        area_quest_rescan_requests_.fetch_add(
            1, std::memory_order_release);
        try {
            append_log("AREA_QUEST_COMPLETION_WITNESS_ARMED", std::format(
                "activation={} epoch={} evidence={} id={} catalog_index={} window_seconds=10 probe_ms={} schedule=exact_id_only",
                activation_, epoch_, evidence ? evidence : "unknown",
                area_quest_catalog_[exact_index].id, exact_index,
                kAreaQuestCompletionProbeInterval.count()));
        } catch (...) {
        }
        return true;
    }

    void service_area_quest_completion_witnesses(
        UEngine* engine, Clock::time_point now) noexcept {
        if (area_quest_completion_witness_queue_count_ == 0
            || !enabled_ || transition_active_
            || activity_suppressed_
            || !area_quest_state_provider_ready_) {
            return;
        }
        UObject* world_context = current_game_instance(engine);
        if (!world_context) {
            // Preserve the fixed numeric witnesses until a valid world returns.
            // This avoids both losing a final exact-ID proof during travel and
            // scanning the at-most-147 queue every frame while no query target
            // exists.
            return;
        }
        std::size_t cursor{};
        std::size_t queries_this_tick{};
        while (cursor < area_quest_completion_witness_queue_count_) {
            const std::size_t index =
                area_quest_completion_witness_queue_[cursor];
            const auto remove_current = [this, &cursor, index]() noexcept {
                if (index < area_quest_completion_witness_queued_.size()) {
                    area_quest_completion_witness_queued_[index] = false;
                }
                --area_quest_completion_witness_queue_count_;
                area_quest_completion_witness_queue_[cursor] =
                    area_quest_completion_witness_queue_[
                        area_quest_completion_witness_queue_count_];
            };
            if (index >= area_quest_catalog_.size()
                || !area_quest_completion_witnesses_[index]) {
                remove_current();
                continue;
            }
            const bool expired =
                now > area_quest_completion_witness_deadlines_[index];
            if (expired && world_context
                && now >= area_quest_completion_probe_after_[index]
                && queries_this_tick
                    >= kAreaQuestCompletionProbeBudgetPerTick) {
                // Preserve the final exact-ID query for a later engine tick;
                // the fixed queue cannot grow beyond 147 entries.
                ++cursor;
                continue;
            }
            const bool probe_due = world_context
                && now >= area_quest_completion_probe_after_[index]
                && queries_this_tick
                    < kAreaQuestCompletionProbeBudgetPerTick;
            if (!probe_due && !expired) {
                ++cursor;
                continue;
            }
            bool end_verified{};
            if (probe_due) {
                ++queries_this_tick;
                area_quest_completion_probe_after_[index] =
                    now + kAreaQuestCompletionProbeInterval;
                if (area_quest_completion_probe_counts_[index]
                    < std::numeric_limits<std::uint8_t>::max()) {
                    ++area_quest_completion_probe_counts_[index];
                }
                std::int64_t state{};
                bool query_succeeded{};
                const bool safe = query_area_quest_state_guarded(
                    world_context, area_quest_catalog_[index].id,
                    state, query_succeeded);
                end_verified = safe && query_succeeded
                    && state == static_cast<std::int64_t>(
                        dswros::AreaQuestState::End);
            }
            if (end_verified) {
                const std::size_t word = index / 64U;
                const std::uint64_t bit = 1ULL << (index % 64U);
                area_quest_exact_completion_bits_[word].fetch_or(
                    bit, std::memory_order_release);
                const auto probes = area_quest_completion_probe_counts_[index];
                area_quest_completion_witnesses_[index] = false;
                area_quest_completion_witness_deadlines_[index] = {};
                area_quest_completion_probe_after_[index] = {};
                area_quest_completion_probe_counts_[index] = 0;
                remove_current();
                try {
                    append_log("AREA_QUEST_COMPLETION_VERIFIED", std::format(
                        "activation={} epoch={} id={} catalog_index={} probes={} evidence=exact_task_id_end_state",
                        activation_, epoch_, area_quest_catalog_[index].id,
                        index, probes));
                } catch (...) {
                }
                continue;
            }
            if (expired) {
                const std::int64_t quest_id = area_quest_catalog_[index].id;
                const auto probes = area_quest_completion_probe_counts_[index];
                const bool save_confirmation_queued =
                    queue_area_quest_save_confirmation(
                    index, "exact_task_identity_no_end_state_after_10s");
                area_quest_completion_witnesses_[index] = false;
                area_quest_completion_witness_deadlines_[index] = {};
                area_quest_completion_probe_after_[index] = {};
                area_quest_completion_probe_counts_[index] = 0;
                remove_current();
                try {
                    append_log("AREA_QUEST_COMPLETION_WITNESS_EXPIRED",
                        std::format(
                            "activation={} epoch={} id={} catalog_index={} probes={} result=no_end_state final_probe={} fallback_queued={} fallback=event_driven_positive_only_sql_maximum_three_attempts",
                            activation_, epoch_, quest_id, index, probes,
                            probe_due, save_confirmation_queued));
                } catch (...) {
                }
                continue;
            }
            ++cursor;
        }
    }

    void service_area_quest_task_class_map(
        UEngine* engine, Clock::time_point now) noexcept {
        if (!area_quest_task_class_map_capture_pending_
            || now < area_quest_task_class_map_retry_after_) {
            return;
        }
        ++area_quest_task_class_map_attempt_count_;
        const auto started = Clock::now();
        const bool ready = capture_area_quest_task_class_map_guarded(engine);
        const auto elapsed_us = std::chrono::duration_cast<
            std::chrono::microseconds>(Clock::now() - started).count();
        if (ready
            || area_quest_task_class_map_attempt_count_
                >= kAreaQuestTaskClassMapMaxAttempts) {
            area_quest_task_class_map_capture_pending_ = false;
        } else {
            area_quest_task_class_map_capture_pending_ = true;
            area_quest_task_class_map_retry_after_ =
                now + kAreaQuestTaskClassMapRetryDelay;
        }
        const char* retry = ready
            ? "none"
            : (area_quest_task_class_map_capture_pending_
                ? "bounded_delayed_second_attempt"
                : "next_explicit_f7_or_travel");
        try {
            append_log("AREA_QUEST_TASK_CLASS_MAP", std::format(
                "activation={} epoch={} ready={} rows={} dynamic_rows={} bindings={} ambiguities={} elapsed_us={} attempt={}/{} retry={}",
                activation_, epoch_, ready,
                area_quest_task_class_row_count_,
                area_quest_dynamic_task_class_row_count_,
                area_quest_task_class_indices_.size(),
                area_quest_task_class_ambiguity_count_, elapsed_us,
                area_quest_task_class_map_attempt_count_,
                kAreaQuestTaskClassMapMaxAttempts, retry));
        } catch (...) {
        }
    }

    void latch_exact_area_quest_completion_numeric(
        std::size_t index) noexcept {
        const std::int64_t quest_id = area_quest_catalog_[index].id;
        ++area_quest_exact_completion_revisions_[index];
        area_quest_completion_observed_[index] = true;
        area_quest_scan_completion_observed_[index] = true;
        area_quest_repeatable_reactivation_armed_[index] = false;
        area_quest_scan_repeatable_reactivation_armed_[index] = false;
        area_quest_completion_generation_locked_[index] = true;
        area_quest_completion_generation_reactivation_armed_[index] = false;
        area_quest_states_[index] = dswros::AreaQuestState::End;
        area_quest_scan_states_[index] = dswros::AreaQuestState::End;
        area_quest_eligibility_[index] = 0;
        area_quest_scan_eligibility_[index] = 0;
        const std::size_t confirmation_word = index / 64U;
        const std::uint64_t confirmation_bit =
            1ULL << (index % 64U);
        const bool confirmation_was_pending =
            (area_quest_save_confirmation_pending_[confirmation_word]
                & confirmation_bit) != 0;
        area_quest_save_confirmation_pending_[confirmation_word] &=
            ~confirmation_bit;
        if ((area_quest_save_confirmation_inflight_[confirmation_word]
                & confirmation_bit) == 0) {
            area_quest_save_confirmation_due_[index] = {};
            area_quest_save_confirmation_attempts_[index] = 0;
        }
        if (confirmation_was_pending) {
            recompute_area_quest_save_confirmation_due();
        }
        area_quest_completion_witnesses_[index] = false;
        area_quest_completion_witness_deadlines_[index] = {};
        area_quest_completion_probe_after_[index] = {};
        area_quest_completion_probe_counts_[index] = 0;
        completed_dynamic_quest_ids_.insert(quest_id);
        compact_rebind_dirty_ = true;
    }

    void apply_exact_area_quest_completion(
        std::size_t index) noexcept {
        if (index >= area_quest_catalog_.size()) {
            return;
        }
        const auto previous_world_map = area_quest_world_map_eligibility_;
        const std::int64_t quest_id = area_quest_catalog_[index].id;
        latch_exact_area_quest_completion_numeric(index);
        bool world_map_changed{};
        if (area_quest_state_ready_) {
            (void)rebuild_area_quest_world_map_eligibility();
            world_map_changed = previous_world_map
                != area_quest_world_map_eligibility_;
        }
        if (world_map_changed) {
            refresh_world_map_atlas_for_runtime_delta(
                "area_quest_exact_completion", quest_id);
        }
        try {
            append_log("AREA_QUEST_EXACT_COMPLETION_OBSERVED", std::format(
                "activation={} epoch={} id={} catalog_index={} evidence=exact_task_id_end_state action=immediate_numeric_completion world_map_changed={}",
                activation_, epoch_, quest_id, index,
                world_map_changed));
        } catch (...) {
        }
    }

    void consume_exact_area_quest_completions(
        bool preserve_for_transition = false) noexcept {
        for (std::size_t word_index = 0;
             word_index < area_quest_exact_completion_bits_.size();
             ++word_index) {
            std::uint64_t pending =
                area_quest_exact_completion_bits_[word_index].exchange(
                    0, std::memory_order_acq_rel);
            while (pending != 0) {
                const auto bit_index = static_cast<std::size_t>(
                    std::countr_zero(pending));
                const std::size_t index = word_index * 64U + bit_index;
                if (preserve_for_transition
                    && index < area_quest_catalog_.size()) {
                    const std::int64_t quest_id =
                        area_quest_catalog_[index].id;
                    // InitGameState pre runs before the travel teardown. Keep
                    // only numeric evidence here; the ordinary renderer path
                    // must not touch a soon-to-be-destroyed widget tree.
                    latch_exact_area_quest_completion_numeric(index);
                    try {
                        append_log(
                            "AREA_QUEST_EXACT_COMPLETION_OBSERVED",
                            std::format(
                                "activation={} epoch={} id={} catalog_index={} evidence=task_class_map action=travel_boundary_numeric_preserve world_map_changed=false",
                                activation_, epoch_, quest_id, index));
                    } catch (...) {
                    }
                } else {
                    apply_exact_area_quest_completion(index);
                }
                pending &= pending - 1U;
            }
        }
    }

    void schedule_area_quest_rescan(Clock::time_point now) noexcept {
        const auto requested_after = now + kAreaQuestRefreshDebounce;
        if (!area_quest_rescan_scheduled_) {
            area_quest_rescan_scheduled_ = true;
            area_quest_rescan_deadline_ =
                now + kAreaQuestRefreshMaxDebounce;
            area_quest_rescan_after_ = requested_after;
            return;
        }
        // Preserve one second of quiet time when possible, but cap a sustained
        // callback burst at two seconds from its first request. This remains a
        // single coalesced scan and cannot accumulate an unbounded queue.
        area_quest_rescan_after_ = std::min(
            requested_after, area_quest_rescan_deadline_);
    }

    void service_runtime_visibility_edges(
        Clock::time_point now) noexcept {
        if (now < next_runtime_visibility_edge_probe_) {
            return;
        }
        next_runtime_visibility_edge_probe_ =
            now + kMinimapScaleSampleInterval;

        const std::int64_t now_unix_seconds = unix_seconds();
        const std::int32_t world_hour = world_time_available_
            ? static_cast<std::int32_t>(
                  current_world_time_seconds() / 3600U)
            : -1;
        const std::int32_t previous_world_hour =
            last_area_quest_world_hour_;
        const bool world_hour_changed =
            world_hour != previous_world_hour;
        if (world_hour_changed) {
            last_area_quest_world_hour_ = world_hour;
            if (world_hour >= 0 && area_quest_state_provider_ready_) {
                // Dynamic quest state remains authoritative for time-activated
                // tasks. Coalesce one transactional refresh on the first valid
                // clock sample after F7 as well as each displayed hour edge;
                // this edge service itself runs at only 1 Hz.
                area_quest_time_rescan_scheduled_ = true;
                try {
                    append_log("AREA_QUEST_TIME_STATE_REFRESH", std::format(
                        "activation={} epoch={} previous_hour={} hour={} schedule=one_transactional_scan coalesced={}",
                        activation_, epoch_, previous_world_hour, world_hour,
                        area_quest_scan_pending_));
                } catch (...) {
                }
            }
        }

        const bool cooldown_edge_reached = encounter_state_ready_
            && next_encounter_cooldown_edge_unix_seconds_ > 0
            && now_unix_seconds
                >= next_encounter_cooldown_edge_unix_seconds_;
        if (!encounter_state_ready_) {
            encounter_visibility_mask_valid_ = false;
            next_encounter_cooldown_edge_unix_seconds_ = 0;
            return;
        }
        if (!world_hour_changed && !cooldown_edge_reached
            && encounter_visibility_mask_valid_) {
            return;
        }

        const std::uint64_t previous_encounter_visibility =
            encounter_visibility_mask_;
        const bool had_visibility_baseline =
            encounter_visibility_mask_valid_;
        encounter_visibility_mask_ = encounter_visibility_mask(
            now_unix_seconds, world_hour);
        encounter_visibility_mask_valid_ = true;
        recompute_next_encounter_cooldown_edge(now_unix_seconds);
        const bool encounter_visibility_changed =
            had_visibility_baseline
            && previous_encounter_visibility
                != encounter_visibility_mask_;

        bool area_quest_visibility_changed{};
        std::size_t area_quest_world_map_visible{};
        if ((world_hour_changed || cooldown_edge_reached)
            && area_quest_state_ready_
            && area_quest_definition_monster_link_count_ > 0) {
            const auto previous = area_quest_world_map_eligibility_;
            area_quest_world_map_visible =
                rebuild_area_quest_world_map_eligibility();
            area_quest_visibility_changed =
                previous != area_quest_world_map_eligibility_;
        }
        if (!encounter_visibility_changed
            && !area_quest_visibility_changed) {
            return;
        }

        compact_rebind_dirty_ = true;
        const bool atlas_was_attached = world_map_umg_renderer_.state()
            == dsnwr::WorldMapUmgRendererState::Attached;
        refresh_world_map_atlas_for_runtime_delta(
            cooldown_edge_reached ? "encounter_cooldown_edge"
                                  : "world_hour_edge",
            0);
        try {
            append_log("RUNTIME_VISIBILITY_EDGE", std::format(
                "activation={} epoch={} reason={} previous_hour={} hour={} encounter_changed={} area_quest_changed={} area_quest_world_map_visible={} next_cooldown_edge_unix={} world_map_refresh={} rebuild=current_session_if_open_else_next_map_session",
                activation_, epoch_,
                cooldown_edge_reached
                    ? (world_hour_changed
                        ? "cooldown_and_world_hour"
                        : "cooldown")
                    : "world_hour",
                previous_world_hour, world_hour,
                encounter_visibility_changed,
                area_quest_visibility_changed,
                area_quest_world_map_visible,
                next_encounter_cooldown_edge_unix_seconds_,
                atlas_was_attached ? "invalidated" : "not_attached"));
        } catch (...) {
        }
    }

    [[nodiscard]] bool compact_visibility_enabled(
        dsnwr::RadarVisibilityCategory category) const noexcept {
        return (dsnwr::compact_radar_visibility_mask(visibility_masks_)
                & dsnwr::radar_visibility_bit(category)) != 0;
    }

    [[nodiscard]] bool world_visibility_enabled(
        dsnwr::RadarVisibilityCategory category) const noexcept {
        return (dsnwr::world_radar_visibility_mask(visibility_masks_)
                & dsnwr::radar_visibility_bit(category)) != 0;
    }

    [[nodiscard]] bool world_map_content_visibility_intent() const noexcept {
        return dsnwr::world_radar_visibility_mask(visibility_masks_) != 0U;
    }

    void publish_world_map_content_visibility_intent() noexcept {
        const bool content_visible = world_map_content_visibility_intent();
        world_map_umg_renderer_.set_content_visibility_intent(
            content_visible);
        if (!content_visible) {
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            world_map_layering_refresh_pending_ = false;
        }
    }

    [[nodiscard]] bool area_quest_visible_for_selected_mode(
        std::size_t index) const noexcept {
        if (index >= area_quest_catalog_.size()) {
            return false;
        }
        return dswros::area_quest_visible_for_display_mode(
            area_quest_display_mode_
                == dsnwr::AreaQuestDisplayMode::AllUnfinished,
            area_quest_world_map_eligibility_[index] != 0,
            area_quest_states_[index],
            area_quest_save_completion_query_available_,
            area_quest_save_completion_[index] != 0,
            area_quest_completion_observed_[index]);
    }

    void apply_visibility_hub_result(
        const dsnwr::RadarVisibilityHubResult& result) noexcept {
        if (result.action != dsnwr::RadarVisibilityHubAction::Applied
            || !result.changed) {
            return;
        }
        const auto previous = visibility_masks_;
        const auto previous_area_quest_mode = area_quest_display_mode_;
        const auto previous_assault_mode = assault_display_mode_;
        const auto previous_height_indicators = height_indicator_mask_;
        const auto previous_language = language_preference_;
        visibility_masks_ = result.packed_masks;
        const bool world_mask_changed =
            dsnwr::world_radar_visibility_mask(previous)
                != dsnwr::world_radar_visibility_mask(visibility_masks_);
        if (world_mask_changed) {
            publish_world_map_content_visibility_intent();
        }
        area_quest_display_mode_ = result.area_quest_mode;
        assault_display_mode_ = result.assault_mode;
        height_indicator_mask_ = static_cast<dswros::HeightIndicatorMask>(
            result.height_indicators & dswros::kHeightIndicatorAll);
        language_preference_ = result.language;
        active_ui_language_ = dswros::resolve_radar_ui_language(
            language_preference_, detected_game_language_);
        const bool area_quest_mode_changed =
            previous_area_quest_mode != area_quest_display_mode_;
        const bool assault_mode_changed =
            previous_assault_mode != assault_display_mode_;
        const bool height_indicators_changed =
            previous_height_indicators != height_indicator_mask_;
        const bool language_changed =
            previous_language != language_preference_;
        const bool compact_changed =
            dsnwr::compact_radar_visibility_mask(previous)
                != dsnwr::compact_radar_visibility_mask(visibility_masks_)
            || area_quest_mode_changed
            || assault_mode_changed
            || height_indicators_changed;
        const bool world_changed =
            world_mask_changed
            || area_quest_mode_changed
            || assault_mode_changed;
        const bool bird_egg_visibility_changed =
            ((dsnwr::compact_radar_visibility_mask(previous)
                ^ dsnwr::compact_radar_visibility_mask(
                    visibility_masks_))
                & dsnwr::radar_visibility_bit(
                    dsnwr::RadarVisibilityCategory::BirdEggs)) != 0;
        if (compact_changed) {
            compact_rebind_dirty_ = true;
        }
        if (assault_mode_changed) {
            // The mode affects display selection only. Re-baseline the
            // existing 49-bit edge detector so the next 1 Hz clock sample
            // does not interpret this explicit UI choice as a runtime edge.
            establish_encounter_visibility_baseline(unix_seconds());
        }
        if (bird_egg_visibility_changed) {
            reset_bird_egg_active_visibility();
        }
        if (world_changed) {
            if (visibility_hub_.is_open()) {
                // Hub controls still persist and update compact selection on
                // every real edge. Coalesce the expensive expanded-map atlas
                // rebuild until the Hub closes so rapid selection changes
                // cannot cause one 80-100 ms rebuild per click.
                visibility_hub_world_map_refresh_pending_ =
                    !visibility_hub_world_map_baseline_valid_
                    || visibility_hub_world_map_baseline_mask_
                        != dsnwr::world_radar_visibility_mask(
                            visibility_masks_)
                    || visibility_hub_world_map_baseline_area_quest_mode_
                        != area_quest_display_mode_
                    || visibility_hub_world_map_baseline_assault_mode_
                        != assault_display_mode_;
            } else {
                refresh_world_map_after_visibility_hub_change();
            }
        }
        const bool persisted = persist_visibility_settings(
            mod_directory() / "config" / "visibility.ini",
            visibility_masks_, area_quest_display_mode_,
            assault_display_mode_, height_indicator_mask_,
            language_preference_);
        try {
            append_log("VISIBILITY_HUB_APPLIED", std::format(
                "activation={} epoch={} compact_mask={} world_mask={} area_quest_mode={} assault_mode={} height_mask={} language_preference={} active_language={} area_quest_mode_changed={} assault_mode_changed={} height_changed={} language_changed={} compact_changed={} world_changed={} persisted={} world_map_refresh={}",
                activation_, epoch_,
                static_cast<std::uint32_t>(
                    dsnwr::compact_radar_visibility_mask(
                        visibility_masks_)),
                static_cast<std::uint32_t>(
                    dsnwr::world_radar_visibility_mask(
                        visibility_masks_)),
                area_quest_display_mode_
                        == dsnwr::AreaQuestDisplayMode::AllUnfinished
                    ? "all"
                    : "available",
                assault_display_mode_ == dsnwr::AssaultDisplayMode::All
                    ? "all"
                    : "current",
                static_cast<std::uint32_t>(height_indicator_mask_),
                dswros::radar_language_preference_id(language_preference_),
                dswros::radar_ui_language_id(active_ui_language_),
                area_quest_mode_changed, assault_mode_changed,
                height_indicators_changed, language_changed,
                compact_changed, world_changed,
                persisted,
                world_changed
                    && visibility_hub_world_map_refresh_pending_
                    ? "deferred_until_close"
                    : world_changed
                        && world_map_umg_renderer_.state()
                            == dsnwr::WorldMapUmgRendererState::Ready
                    ? "bounded_current_session"
                    : "next_map_session"));
        } catch (...) {
        }
    }

    void refresh_world_map_after_visibility_hub_change() noexcept {
        visibility_hub_world_map_refresh_pending_ = false;
        world_map_marker_snapshot_built_ = false;
        if (!world_map_content_visibility_intent()) {
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            world_map_layering_refresh_pending_ = false;
            if (world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Attached) {
                world_map_umg_renderer_.begin_activation();
                reset_world_map_runtime(true);
            }
            return;
        }
        if (!enabled_ || transition_active_) {
            // F6 remains available while Radar is off. Persist those choices,
            // but never wake the expanded-map renderer until a real F7/F6
            // activation owns a playable World again.
            return;
        }
        UObject* current_layer = world_map_candidate_available_
            ? current_world_map_layer_guarded() : nullptr;
        UWorld* candidate_world = current_layer
            ? object_world_guarded(current_layer) : nullptr;
        const bool visibly_open =
            world_map_layer_open_and_visible_guarded(
                candidate_world, current_layer);
        const bool attached = world_map_umg_renderer_.state()
            == dsnwr::WorldMapUmgRendererState::Attached;
        if (visibly_open || attached) {
            world_map_umg_renderer_.begin_activation();
            reset_world_map_runtime(true);
            if (!visibly_open) {
                // A retained but hidden layer is not an open-map signal.
                world_map_session_pending_ = false;
            }
        }
    }

    void flush_visibility_hub_world_map_refresh(
        const char* reason) noexcept {
        const bool refresh = visibility_hub_world_map_refresh_pending_;
        visibility_hub_world_map_baseline_valid_ = false;
        if (!refresh) {
            return;
        }
        const bool can_refresh = enabled_ && !transition_active_;
        refresh_world_map_after_visibility_hub_change();
        try {
            append_log("VISIBILITY_HUB_WORLD_MAP_REFRESH", std::format(
                "activation={} epoch={} reason={} action={}",
                activation_, epoch_, reason ? reason : "closed",
                can_refresh
                    ? "single_coalesced_rebuild"
                    : "deferred_until_activation"));
        } catch (...) {
        }
    }

    [[nodiscard]] dsnwr::RadarModStatus radar_mod_status() const noexcept {
        if (!required_runtime_ready_.load(std::memory_order_acquire)
            || engine_tick_fault_pending_
            || engine_tick_fault_terminal_
            || engine_tick_fault_cleanup_failed_
            || compact_umg_renderer_.state()
                == dsnwr::CompactUmgRendererState::Faulted
            || world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Faulted
            || area_quest_scan_faulted_
            || (save_reconcile_completed_
                && !treasure_eligibility_ready_)) {
            return dsnwr::RadarModStatus::Fault;
        }
        return enabled_ && !transition_active_
                && !main_menu_activation_latched_
            ? dsnwr::RadarModStatus::On
            : dsnwr::RadarModStatus::Off;
    }

    [[nodiscard]] bool request_radar_activation(
        UEngine* engine, const char* request_source,
        bool preserve_visibility_hub = false) {
        const char* source = request_source ? request_source : "unknown";
        if (!engine
            || !required_runtime_ready_.load(std::memory_order_acquire)
            || shutting_down_.load(std::memory_order_acquire)) {
            enabled_ = false;
            append_log("ACTIVATION_REJECTED", std::format(
                "reason=required_runtime_unavailable action=remain_disabled request_source={}",
                source));
            return false;
        }
        if (main_menu_activation_latched_ || !enabled_) {
            std::string current_world{};
            UWorld* current_world_object{};
            static_cast<void>(capture_current_world_identity_guarded(
                engine, &current_world, &current_world_object));
            if (!is_open_world_identity(current_world)) {
                append_log("F7_REJECTED", std::format(
                    "reason={} action=remain_disabled world={} request_source={}",
                    main_menu_activation_latched_
                        ? "main_menu_owner_boundary_requires_loaded_open_world"
                        : "activation_requires_loaded_open_world",
                    current_world.empty() ? "unavailable" : current_world,
                    source));
                return false;
            }
            prune_ui_layer_candidates_for_world(current_world_object);
            consume_world_map_listener_candidate(current_world_object);
            consume_compact_listener_candidate(current_world_object);
            prune_bird_egg_candidates_for_world(current_world_object);
            main_menu_activation_latched_ = false;
            baseline_world_key_.clear();
            baseline_context_key_.clear();
            current_world_key_ = current_world;
            current_context_key_.clear();
            activate(engine, preserve_visibility_hub);
            return enabled_ && !transition_active_;
        }

        const bool retryable_attach_failure =
            engine_tick_fault_terminal_
            || compact_attach_gate_status_
                == CompactAttachGateStatus::CurrentPlayerControllerUnavailable
            || compact_attach_gate_status_
                == CompactAttachGateStatus::RendererRejected
            || compact_umg_renderer_.state()
                == dsnwr::CompactUmgRendererState::Faulted
            || world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Faulted
            || area_quest_scan_faulted_
            || (save_reconcile_completed_
                && !treasure_eligibility_ready_);
        if (enabled_ && !transition_active_ && !retryable_attach_failure) {
            if (!area_quest_task_class_map_ready_
                && area_quest_state_provider_ready_
                && area_quest_catalog_.size() == kExpectedAreaQuestCount) {
                reset_area_quest_task_class_capture(true);
                append_log("AREA_QUEST_TASK_CLASS_MAP_REARMED", std::format(
                    "activation={} epoch={} reason=explicit_f7 attempts_max={} request_source={}",
                    activation_, epoch_, kAreaQuestTaskClassMapMaxAttempts,
                    source));
            }
            if (!rearm_world_map_from_f7()) {
                append_log("F7_COALESCED", std::format(
                    "activation={} epoch={} compact_state={} compact_attach_gate={} world_map_state={} world_map_failure={} world_map_attempts={} world_map_pending={} world_map_candidate={} candidate_serial={} serviced_serial={} reason=already_active request_source={}",
                    activation_, epoch_,
                    static_cast<std::uint32_t>(compact_umg_renderer_.state()),
                    static_cast<std::uint32_t>(compact_attach_gate_status_),
                    static_cast<std::uint32_t>(world_map_umg_renderer_.state()),
                    world_map_umg_renderer_.last_attach_failure(),
                    world_map_service_attempts_, world_map_session_pending_,
                    world_map_candidate_available_, world_map_candidate_serial_,
                    world_map_serviced_serial_, source));
            }
        } else {
            if (engine_tick_fault_terminal_
                && !engine_tick_fault_cleanup_failed_) {
                // A fresh explicit F6/F7 request owns a new bounded recovery
                // budget. Automatic recovery never reaches this path, so it
                // cannot turn a repeated engine fault into an unbounded loop.
                engine_tick_fault_recovery_attempts_ = 0;
            }
            activate(engine, preserve_visibility_hub);
        }
        return enabled_ && !transition_active_;
    }

    void establish_visibility_hub_baseline() noexcept {
        visibility_hub_world_map_refresh_pending_ = false;
        visibility_hub_world_map_baseline_valid_ = true;
        visibility_hub_world_map_baseline_mask_ =
            dsnwr::world_radar_visibility_mask(visibility_masks_);
        visibility_hub_world_map_baseline_area_quest_mode_ =
            area_quest_display_mode_;
        visibility_hub_world_map_baseline_assault_mode_ =
            assault_display_mode_;
    }

    void handle_visibility_hub_result(
        UEngine* engine,
        UObject* current_controller,
        const dsnwr::RadarVisibilityHubResult& result,
        const char* close_reason) {
        if (result.action == dsnwr::RadarVisibilityHubAction::Applied) {
            apply_visibility_hub_result(result);
        } else if (result.action
                   == dsnwr::RadarVisibilityHubAction::Closed) {
            flush_visibility_hub_world_map_refresh(close_reason);
            append_log("VISIBILITY_HUB_CLOSED", std::format(
                "activation={} epoch={} reason={}", activation_, epoch_,
                close_reason ? close_reason : "closed"));
        } else if (result.action
                   == dsnwr::RadarVisibilityHubAction::Rejected) {
            append_log("VISIBILITY_HUB_REJECTED", std::format(
                "activation={} epoch={} failure={} state={} abi_failures={} font_abi_details={} font_source={} font_fallback_reason={} text_runtime_failure={} text_overlay_active={} text_overlay_failure={}",
                activation_, epoch_, result.failure,
                static_cast<std::uint32_t>(visibility_hub_.state()),
                visibility_hub_.abi_failure_mask(),
                visibility_hub_.font_abi_detail_mask(),
                static_cast<std::uint32_t>(visibility_hub_.font_source()),
                visibility_hub_.font_fallback_reason(),
                visibility_hub_.text_runtime_failure(),
                visibility_hub_.text_overlay_active(),
                visibility_hub_.text_overlay_failure()));
        }

        if (result.command
            == dsnwr::RadarVisibilityHubCommand::OpenBugReport) {
            // Restore the game's cursor/input policy and release the transient
            // UMG tree before invoking the fixed external destination.
            visibility_hub_.detach(current_controller);
            flush_visibility_hub_world_map_refresh("bug_report");
            visibility_hub_service_after_ = {};
            visibility_hub_world_map_baseline_valid_ = false;
            const auto shell_result = reinterpret_cast<INT_PTR>(ShellExecuteW(
                nullptr, L"open", kBugReportUrl, nullptr, nullptr,
                SW_SHOWNORMAL));
            append_log("BUG_REPORT_OPEN", std::format(
                "activation={} epoch={} launched={} shell_result={}",
                activation_, epoch_, shell_result > 32, shell_result));
        } else if (result.command
                   == dsnwr::RadarVisibilityHubCommand::DisableMod) {
            // The status action is an in-panel control. Keep the transient Hub
            // and its input owner alive so the same page can immediately show
            // OFF and offer Enable without forcing another F6 round trip.
            disable(true);
        } else if (result.command
                   == dsnwr::RadarVisibilityHubCommand::EnableMod) {
            // A successful activation keeps the Hub open and refreshes its
            // status on the next existing 50 ms service edge. A rejected
            // title/loading request likewise leaves the page available.
            static_cast<void>(request_radar_activation(
                engine, "f6_status_action", true));
        }
    }

    void open_visibility_hub_when_ready(
        UEngine* engine, Clock::time_point now) {
        if (!visibility_hub_open_pending_) {
            return;
        }
        const dsnwr::RadarModStatus current_mod_status = radar_mod_status();
        if (transition_active_
            && current_mod_status != dsnwr::RadarModStatus::Fault) {
            // F6 is valid during loading, but a controller from the departing
            // World must not own the new Hub. Keep the explicit request alive
            // until transition_end publishes a stable controller; a second
            // F6 remains the bounded user cancellation path.
            visibility_hub_open_pending_until_ =
                now + kVisibilityHubOpenPendingLifetime;
            visibility_hub_open_retry_after_ = now;
            return;
        }
        if (now >= visibility_hub_open_pending_until_) {
            visibility_hub_open_pending_ = false;
            append_log("VISIBILITY_HUB_REJECTED", std::format(
                "activation={} epoch={} reason=controller_wait_expired",
                activation_, epoch_));
            return;
        }
        if (now < visibility_hub_open_retry_after_) {
            return;
        }
        visibility_hub_open_retry_after_ =
            now + kVisibilityHubOpenRetryInterval;
        UObject* controller =
            current_player_controller_for_visibility_hub(engine);
        if (!controller) {
            return;
        }

        detected_game_language_ =
            detect_current_game_language_guarded(engine);
        migrate_legacy_auto_language_preference("f6_real_open");
        active_ui_language_ = dswros::resolve_radar_ui_language(
            language_preference_, detected_game_language_);
        append_log("RADAR_LANGUAGE_SELECTED", std::format(
            "preference={} detected={} active={} source={} schedule=f6_real_open",
            dswros::radar_language_preference_id(language_preference_),
            dswros::radar_ui_language_id(detected_game_language_),
            dswros::radar_ui_language_id(active_ui_language_),
            current_language_detection_source_));
        const auto result = visibility_hub_.toggle(
            controller, visibility_masks_, area_quest_display_mode_,
            assault_display_mode_, height_indicator_mask_,
            language_preference_, detected_game_language_,
            current_mod_status);
        visibility_hub_open_pending_ = false;
        visibility_hub_service_after_ = now;
        if (result.action == dsnwr::RadarVisibilityHubAction::Opened) {
            establish_visibility_hub_baseline();
            append_log("VISIBILITY_HUB_OPENED", std::format(
                "activation={} epoch={} compact_mask={} world_mask={} area_quest_mode={} assault_mode={} height_mask={} language_preference={} detected_language={} active_language={} mod_status={} font_source={} font_fallback_reason={} text_runtime_failure={} font_abi_details={} text_overlay_active={} text_overlay_failure={} service_ms=50",
                activation_, epoch_,
                static_cast<std::uint32_t>(
                    dsnwr::compact_radar_visibility_mask(visibility_masks_)),
                static_cast<std::uint32_t>(
                    dsnwr::world_radar_visibility_mask(visibility_masks_)),
                area_quest_display_mode_
                        == dsnwr::AreaQuestDisplayMode::AllUnfinished
                    ? "all" : "available",
                assault_display_mode_ == dsnwr::AssaultDisplayMode::All
                    ? "all" : "current",
                static_cast<std::uint32_t>(height_indicator_mask_),
                dswros::radar_language_preference_id(language_preference_),
                dswros::radar_ui_language_id(detected_game_language_),
                dswros::radar_ui_language_id(active_ui_language_),
                static_cast<std::uint32_t>(radar_mod_status()),
                static_cast<std::uint32_t>(visibility_hub_.font_source()),
                visibility_hub_.font_fallback_reason(),
                visibility_hub_.text_runtime_failure(),
                visibility_hub_.font_abi_detail_mask(),
                visibility_hub_.text_overlay_active(),
                visibility_hub_.text_overlay_failure()));
        } else {
            handle_visibility_hub_result(
                engine, controller, result, "f6_open");
        }
    }

    void service_visibility_hub_toggle_request(
        UEngine* engine, Clock::time_point now) {
        const std::uint32_t requests = f6_requests_.exchange(
            0, std::memory_order_acq_rel);
        if (requests != 0 && now >= visibility_hub_toggle_after_) {
            visibility_hub_toggle_after_ =
                now + kVisibilityHubToggleDebounce;
            if (visibility_hub_.is_open()) {
                UObject* controller =
                    current_player_controller_for_visibility_hub(engine);
                if (!controller) {
                    // The transient current-controller probe may disappear
                    // before the host's own controller. Detach through the
                    // retained host owner so GameOnly input and the previous
                    // cursor state are restored instead of merely dropping
                    // the weak handles.
                    visibility_hub_.detach();
                    flush_visibility_hub_world_map_refresh(
                        "f6_close_controller_missing");
                    append_log("VISIBILITY_HUB_CLOSED", std::format(
                        "activation={} epoch={} reason=f6_controller_missing",
                        activation_, epoch_));
                } else {
                    const auto result = visibility_hub_.toggle(
                        controller, visibility_masks_,
                        area_quest_display_mode_, assault_display_mode_,
                        height_indicator_mask_, language_preference_,
                        detected_game_language_, radar_mod_status());
                    handle_visibility_hub_result(
                        engine, controller, result, "f6_toggle");
                }
                visibility_hub_open_pending_ = false;
                return;
            }
            if (visibility_hub_open_pending_) {
                visibility_hub_open_pending_ = false;
                visibility_hub_open_retry_after_ = {};
                visibility_hub_open_pending_until_ = {};
                append_log("VISIBILITY_HUB_OPEN_CANCELLED", std::format(
                    "activation={} epoch={} reason=second_f6",
                    activation_, epoch_));
                return;
            }
            visibility_hub_open_pending_ = true;
            visibility_hub_open_retry_after_ = now;
            visibility_hub_open_pending_until_ =
                now + kVisibilityHubOpenPendingLifetime;
            append_log("VISIBILITY_HUB_OPEN_PENDING", std::format(
                "activation={} epoch={} timeout_ms={} retry_ms={}",
                activation_, epoch_,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    kVisibilityHubOpenPendingLifetime).count(),
                kVisibilityHubOpenRetryInterval.count()));
        }
        open_visibility_hub_when_ready(engine, now);
    }

    void service_visibility_hub(
        UEngine* engine, Clock::time_point now) noexcept {
        if (!visibility_hub_.is_open()
            || now < visibility_hub_service_after_) {
            return;
        }
        visibility_hub_service_after_ =
            now + kVisibilityHubServiceInterval;
        UObject* controller =
            current_player_controller_for_visibility_hub(engine);
        const auto result = visibility_hub_.service_open_panel(
            controller, radar_mod_status());
        handle_visibility_hub_result(
            engine, controller, result, "x_close");
    }

    [[nodiscard]] static std::uint64_t profile_elapsed_us(
        Clock::time_point started) noexcept {
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count();
        return elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 0;
    }

    void finish_engine_tick_profile(
        Clock::time_point now,
        Clock::time_point tick_started,
        EngineTickProfileSample& sample) noexcept {
        const auto total_index = static_cast<std::size_t>(
            EngineTickProfileStage::Total);
        sample.elapsed_us[total_index] = profile_elapsed_us(tick_started);
        sample.executed_mask |= 1U << total_index;
        for (std::size_t index = 0;
             index < engine_tick_profile_metrics_.size(); ++index) {
            if ((sample.executed_mask & (1U << index)) == 0) {
                continue;
            }
            auto& metric = engine_tick_profile_metrics_[index];
            ++metric.calls;
            metric.total_us += sample.elapsed_us[index];
            metric.maximum_us = std::max(
                metric.maximum_us, sample.elapsed_us[index]);
        }

        const auto total_us = sample.elapsed_us[total_index];
        if (total_us >= kEngineTickSlowThresholdUs) {
            ++engine_tick_slow_count_;
            if (now >= engine_tick_slow_log_after_) {
                engine_tick_slow_log_after_ =
                    now + kEngineTickSlowLogInterval;
                dsnwr::NativeEngineTickProfileSample record{};
                record.activation = activation_;
                record.epoch = epoch_;
                record.elapsed_us = sample.elapsed_us;
                record.area_quest_scan_active = sample.area_quest_scan_active;
                record.area_quest_scan_index = sample.area_quest_scan_index;
                record.area_quest_catalog_size = area_quest_catalog_.size();
                record.compact_update_called = sample.compact_update_called;
                record.discovery_called = sample.discovery_called;
                record.world_map_layering_pending =
                    sample.world_map_layering_pending;
                record.bird_egg_active_count = sample.bird_egg_active_count;
                dsnwr::append_native_engine_tick_slow(
                    record, kEngineTickSlowThresholdUs,
                    engine_tick_slow_count_);
            }
        }

        if (engine_tick_profile_report_after_ == Clock::time_point{}) {
            engine_tick_profile_report_after_ =
                now + kEngineTickProfileInterval;
            return;
        }
        if (now < engine_tick_profile_report_after_) {
            return;
        }
        engine_tick_profile_report_after_ = now + kEngineTickProfileInterval;
        dsnwr::NativeEngineTickProfileReport report{};
        report.activation = activation_;
        report.epoch = epoch_;
        report.interval_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                kEngineTickProfileInterval).count());
        for (std::size_t index = 0;
             index < engine_tick_profile_metrics_.size(); ++index) {
            const auto& source = engine_tick_profile_metrics_[index];
            report.metrics[index] = {
                source.calls, source.total_us, source.maximum_us};
        }
        report.slow_ticks = engine_tick_slow_count_;
        report.slow_threshold_us = kEngineTickSlowThresholdUs;
        dsnwr::append_native_engine_tick_profile(report);
        engine_tick_profile_metrics_.fill({});
        engine_tick_slow_count_ = 0;
    }

    void engine_tick(UEngine* engine) noexcept {
        if (!game_thread()) return;
#if defined(_MSC_VER)
        __try { engine_tick_unsafe(engine); }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            engine_tick_fault_code_ = GetExceptionCode();
            if (engine_tick_fault_cleanup_in_progress_) {
                engine_tick_fault_cleanup_in_progress_ = false;
                engine_tick_fault_cleanup_failed_ = true;
            }
            engine_tick_fault_was_enabled_ = engine_tick_fault_was_enabled_
                || enabled_ || engine_tick_fault_recovery_in_progress_;
            engine_tick_fault_pending_ = true;
            enabled_ = false;
            transition_active_ = true;
        }
#else
        engine_tick_unsafe(engine);
#endif
    }

#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
    void stable_process_event_pulse(UObject*) noexcept {
        if (shutting_down_.load(std::memory_order_acquire)) return;
        const std::uint64_t now = GetTickCount64();
        std::uint64_t due = stable_next_pulse_ms_.load(
            std::memory_order_relaxed);
        if (now < due) return;
        if (!stable_next_pulse_ms_.compare_exchange_strong(
                due, now + 8U, std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return;
        }
        if (!engine_tick_engine_) {
            engine_tick_engine_ = static_cast<UEngine*>(
                UObjectGlobals::FindFirstOf(L"GameEngine"));
            if (!engine_tick_engine_) return;
        }
        engine_tick(engine_tick_engine_);
    }
#endif

    void engine_tick_unsafe(UEngine* engine) {
        // UEngine is process-lifetime stable. Hooks resolve the current
        // Controller/Pawn fresh through it and never retain either gameplay
        // UObject across frames or worlds.
        engine_tick_engine_ = engine;
        if (engine_tick_fault_pending_) {
            service_engine_tick_fault(engine);
            return;
        }
        const auto now = Clock::now();
        // F6 is a control-plane request, not Radar object work. Service it
        // before the runtime-ready gate so OFF/FAULT status and settings remain
        // reachable at the title screen and during bounded loading waits.
        service_visibility_hub_toggle_request(engine, now);
        service_visibility_hub(engine, now);
        if (!required_runtime_ready_.load(std::memory_order_acquire)) {
            f8_requests_.store(0, std::memory_order_release);
            if (f7_requests_.exchange(0, std::memory_order_acq_rel) != 0) {
                append_log(
                    "F7_REJECTED",
                    "reason=required_runtime_unavailable action=remain_disabled");
            }
            enabled_ = false;
            return;
        }
        if (!transition_active_ && !main_menu_activation_latched_) {
            consume_world_map_listener_candidate();
            consume_compact_listener_candidate();
        }
        if (f8_requests_.exchange(0, std::memory_order_acq_rel) != 0) disable();
        if (f7_requests_.exchange(0, std::memory_order_acq_rel) != 0) {
            static_cast<void>(request_radar_activation(engine, "f7"));
        }
        const bool profile_enabled = dsnwr::native_event_log_enabled();
        const auto profile_tick_started = now;
        EngineTickProfileSample profile_sample{};
        const auto profile_stage = [profile_enabled, &profile_sample](
            EngineTickProfileStage stage, auto&& function) {
            if (!profile_enabled) {
                function();
                return;
            }
            const auto started = Clock::now();
            function();
            const auto index = static_cast<std::size_t>(stage);
            profile_sample.elapsed_us[index] +=
                profile_elapsed_us(started);
            profile_sample.executed_mask |= 1U << index;
        };
        const auto finish_profile = [this, profile_enabled, now,
                                     profile_tick_started,
                                     &profile_sample]() {
            if (profile_enabled) {
                finish_engine_tick_profile(
                    now, profile_tick_started, profile_sample);
            }
        };
        const std::uint32_t quest_refresh_requests =
            area_quest_rescan_requests_.exchange(
                0, std::memory_order_acq_rel);
        const std::uint32_t unmapped_completion_requests =
            area_quest_unmapped_completion_requests_.exchange(
                0, std::memory_order_acq_rel);
        if (quest_refresh_requests != 0
            && enabled_ && !transition_active_) {
            // The callback is emitted in bursts while the quest and nearby
            // mini-game blueprints are being torn down. Coalesce the burst and
            // wait for the game query to settle before taking the next complete
            // numeric snapshot.
            schedule_area_quest_rescan(now);
        }
        if (unmapped_completion_requests != 0
            && enabled_ && !transition_active_) {
            try {
                append_log("AREA_QUEST_COMPLETION_UNMAPPED", std::format(
                    "activation={} epoch={} requests={} map_ready={} action=generic_transactional_refresh_only",
                    activation_, epoch_, unmapped_completion_requests,
                    area_quest_task_class_map_ready_));
            } catch (...) {
            }
        }
        if (area_quest_rescan_scheduled_
            && now >= area_quest_rescan_after_
            && !area_quest_scan_pending_
            && enabled_ && !transition_active_) {
            area_quest_rescan_scheduled_ = false;
            request_area_quest_scan("quest_state_event_debounced");
        }
        if (area_quest_time_rescan_scheduled_
            && !area_quest_scan_pending_
            && enabled_ && !transition_active_) {
            area_quest_time_rescan_scheduled_ = false;
            request_area_quest_scan("world_hour_edge");
        }
        report_compact_pool_state_change();
        report_world_map_state_change();
        profile_sample.world_map_layering_pending =
            world_map_layering_refresh_pending_;
        profile_stage(EngineTickProfileStage::WorldMapLayering, [this, now] {
            service_world_map_layering_refresh(now);
        });
        if (!enabled_ || transition_active_) {
            finish_profile();
            return;
        }
        if (now < stable_after_) return;
        // Mapping begins only after the existing activation/travel stability
        // gate. A failed first attempt gets exactly one delayed retry; there is
        // no steady-state polling. Suppressed activities preserve the pending
        // request and pay no reflection cost until open-world rendering resumes.
        profile_sample.area_quest_scan_active = area_quest_scan_pending_;
        profile_sample.area_quest_scan_index = area_quest_scan_index_;
        profile_stage(EngineTickProfileStage::AreaQuest, [this, engine, now] {
            if (!activity_suppressed_) {
                service_area_quest_task_class_map(engine, now);
            }
            service_area_quest_completion_witnesses(engine, now);
            consume_exact_area_quest_completions();
            process_one_area_quest(engine);
        });
        if (!clock_capture_attempted_ && now >= clock_capture_after_) capture_world_clock();
        const bool activity_profile_due = now >= next_activity_probe_;
        const auto activity_profile_started = profile_enabled
            ? Clock::now() : Clock::time_point{};
        if (now >= next_activity_probe_) {
            next_activity_probe_ = now + kDiscoveryInterval;
            probe_activity_context_guarded(engine);
            if (!enabled_) {
                return;
            }
            apply_compact_suppression();
            service_runtime_visibility_edges(now);
        }
        if (profile_enabled && activity_profile_due) {
            const auto index = static_cast<std::size_t>(
                EngineTickProfileStage::Activity);
            profile_sample.elapsed_us[index] +=
                profile_elapsed_us(activity_profile_started);
            profile_sample.executed_mask |= 1U << index;
        }
        if (now >= next_position_) {
            next_position_ = now + kPositionInterval;
            dswros::Position position{};
            bool player_position_read{};
            profile_stage(
                EngineTickProfileStage::PositionAndSave,
                [this, engine, &position, &player_position_read] {
                    player_position_read = read_player_position(engine, &position);
                    if (player_position_read) {
                        player_ = position;
                        position_valid_ = true;
                        request_or_apply_save_reconcile();
                        service_world_map_atlas(engine);
                        apply_compact_suppression();
                    } else {
                        position_valid_ = false;
                        // The viewport host is independent from the game minimap.
                        // Collapse it on the first failed current-Pawn sample instead
                        // of waiting for a later InitGameState or world-identity edge.
                        apply_compact_suppression();
                    }
                });
            if (player_position_read && !compact_render_suppressed()) {
                profile_sample.compact_update_called = true;
                profile_stage(
                    EngineTickProfileStage::CompactUmg,
                    [this, engine, now] { update_compact_pool(engine, now); });
            }
        }
        if (position_valid_ && now >= next_discovery_) {
            next_discovery_ = now + kDiscoveryInterval;
            profile_sample.discovery_called = true;
            profile_stage(EngineTickProfileStage::Encounter, [this] {
                consume_pending_encounter_deaths();
                consume_created_encounter_candidates();
            });
            profile_stage(EngineTickProfileStage::BirdEgg, [this, now] {
                discover_bird_egg_candidates();
                service_active_bird_eggs(now);
            });
            profile_sample.bird_egg_active_count = bird_egg_active_count_;
            profile_stage(
                EngineTickProfileStage::ObservedObjects,
                [this] { probe_observed_objects(); });
        }
        finish_profile();
    }

    void service_engine_tick_fault(UEngine* engine) {
        const DWORD fault_code = engine_tick_fault_code_;
        const bool was_enabled = engine_tick_fault_was_enabled_;
        if (engine_tick_fault_cleanup_failed_) {
            engine_tick_fault_pending_ = false;
            engine_tick_fault_was_enabled_ = false;
            engine_tick_fault_recovery_in_progress_ = false;
            engine_tick_fault_terminal_ = true;
            enabled_ = false;
            transition_active_ = true;
            try {
                append_log("ENGINE_TICK_RECOVERY_STOPPED", std::format(
                    "activation={} epoch={} reason=cleanup_fault code=0x{:08X} recovery_attempts={}",
                    activation_, epoch_, static_cast<std::uint32_t>(fault_code),
                    engine_tick_fault_recovery_attempts_));
            } catch (...) {
            }
            return;
        }
        const bool recover = was_enabled && engine
            && required_runtime_ready_.load(std::memory_order_acquire)
            && !main_menu_activation_latched_
            && engine_tick_fault_recovery_attempts_ == 0;
        engine_tick_fault_pending_ = false;
        engine_tick_fault_was_enabled_ = false;
        engine_tick_fault_recovery_in_progress_ = false;
        if (recover) {
            // Spend the only automatic budget before cleanup. If cleanup or
            // activation faults, the outer engine-tick SEH records a second
            // pending failure and the next tick can only fail closed.
            ++engine_tick_fault_recovery_attempts_;
        }
        try {
            append_log("ENGINE_TICK_FAULT", std::format(
                "activation={} epoch={} code=0x{:08X} was_enabled={} auto_recovery={} recovery_attempts={} action=guarded_cleanup",
                activation_, epoch_, static_cast<std::uint32_t>(fault_code),
                was_enabled, recover, engine_tick_fault_recovery_attempts_));
        } catch (...) {
        }

        // These owners each guard their own UObject cleanup. disable() clears
        // activation-local numeric work; the explicit world-map detach then
        // removes a suspended host instead of preserving a frozen atlas.
        engine_tick_fault_cleanup_in_progress_ = true;
        disable();
        world_map_umg_renderer_.detach();
        reset_world_map_runtime(false);
        transition_active_ = true;
        engine_tick_fault_cleanup_in_progress_ = false;

        if (!recover) {
            engine_tick_fault_terminal_ = true;
            try {
                append_log("ENGINE_TICK_RECOVERY_STOPPED", std::format(
                    "activation={} epoch={} reason={} recovery_attempts={}",
                    activation_, epoch_,
                    was_enabled ? "budget_exhausted" : "radar_was_inactive",
                    engine_tick_fault_recovery_attempts_));
            } catch (...) {
            }
            return;
        }

        engine_tick_fault_recovery_in_progress_ = true;
        activate(engine);
        engine_tick_fault_recovery_in_progress_ = false;
        try {
            append_log("ENGINE_TICK_RECOVERED", std::format(
                "activation={} epoch={} recovery_attempts={} mode=single_bounded_activation",
                activation_, epoch_, engine_tick_fault_recovery_attempts_));
        } catch (...) {
        }
    }

    void activate(
        UEngine* engine, bool preserve_visibility_hub = false) {
        if (!engine
            || !required_runtime_ready_.load(std::memory_order_acquire)
            || shutting_down_.load(std::memory_order_acquire)) {
            enabled_ = false;
            append_log(
                "ACTIVATION_REJECTED",
                "reason=required_runtime_unavailable action=remain_disabled");
            return;
        }
        detected_game_language_ =
            detect_current_game_language_guarded(engine);
        migrate_legacy_auto_language_preference("once_per_f7");
        active_ui_language_ = dswros::resolve_radar_ui_language(
            language_preference_, detected_game_language_);
        append_log("RADAR_LANGUAGE_SELECTED", std::format(
            "preference={} detected={} active={} source={} schedule=once_per_f7",
            dswros::radar_language_preference_id(language_preference_),
            dswros::radar_ui_language_id(detected_game_language_),
            dswros::radar_ui_language_id(active_ui_language_),
            current_language_detection_source_));
        if (enabled_) {
            // A repeated F7 or bounded fault recovery must not discard a
            // death notification that already passed exact actor, catalog,
            // availability, visibility, and distance evidence.
            consume_pending_encounter_deaths(true);
        }
        if (!preserve_visibility_hub) {
            visibility_hub_.detach();
            visibility_hub_world_map_refresh_pending_ = false;
            visibility_hub_world_map_baseline_valid_ = false;
            visibility_hub_service_after_ = {};
            visibility_hub_toggle_after_ = {};
            visibility_hub_open_pending_ = false;
            visibility_hub_open_retry_after_ = {};
            visibility_hub_open_pending_until_ = {};
        }
        compact_umg_renderer_.begin_activation();
        reset_compact_pool_runtime();
        world_map_umg_renderer_.begin_activation();
        if (world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Suspended
            && (!world_map_content_visibility_intent()
                || area_quest_state_provider_ready_)) {
            // An explicit F8/F7 is also a requested task-state sync. Drop the
            // suspended atlas so stale quest eligibility cannot flash before
            // the new bounded scan completes. The live layer candidate remains
            // weakly held by this owner and the file cache avoids duplicate I/O
            // when the marker fingerprint is unchanged.
            world_map_umg_renderer_.detach();
            world_map_umg_renderer_.begin_activation();
        }
        reset_world_map_runtime(true);
        enabled_ = true;
        engine_tick_fault_terminal_ = false;
        transition_active_ = false;
        position_valid_ = false;
        ++activation_;
        ++epoch_;
        engine_tick_profile_metrics_.fill({});
        engine_tick_profile_report_after_ = {};
        engine_tick_slow_log_after_ = {};
        engine_tick_slow_count_ = 0;
        // Exact accepted death evidence is catalog-index only and may survive
        // this world reset until a valid encounter-state baseline can apply it.
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        reset_bird_egg_active_visibility();
        stable_after_ = Clock::now() + kActivationStability;
        clock_capture_after_ = Clock::now() + kClockCaptureDelay;
        clock_capture_attempted_ = false;
        world_time_available_ = false;
        last_area_quest_world_hour_ = -1;
        activity_suppressed_ = false;
        world_map_compact_suppressed_ = false;
        game_paused_ = false;
        game_pause_sample_known_ = false;
        save_reconcile_requested_ = false;
        save_reconcile_completed_ = false;
        save_reconcile_inflight_request_id_ = 0;
        save_reconcile_request_after_ = {};
        area_quest_save_confirmation_pending_.fill(0);
        area_quest_save_confirmation_inflight_.fill(0);
        area_quest_save_confirmation_due_.fill({});
        area_quest_save_confirmation_attempts_.fill(0);
        area_quest_save_confirmation_next_due_ = {};
        area_quest_save_confirmation_pending_armed_ = false;
        area_quest_save_completion_counts_.fill(
            dswros::kUnknownAreaQuestSaveCompletionCount);
        area_quest_completion_generation_locked_.fill(false);
        area_quest_completion_generation_reactivation_armed_.fill(false);
        treasure_save_confirmation_pending_.fill(0);
        treasure_save_confirmation_inflight_.fill(0);
        treasure_save_confirmation_due_.fill({});
        treasure_save_confirmation_attempts_.fill(0);
        treasure_save_confirmation_next_due_ = {};
        treasure_save_confirmation_pending_armed_ = false;
        treasure_eligibility_ready_ = false;
        compact_eligibility_.fill(0);
        mini_game_eligibility_.fill(0);
        encounter_state_ready_ = false;
        encounter_visibility_mask_valid_ = false;
        next_encounter_cooldown_edge_unix_seconds_ = 0;
        area_quest_scan_faulted_ = false;
        area_quest_state_ready_ = !area_quest_state_provider_ready_;
        area_quest_scan_pending_ = false;
        area_quest_rescan_scheduled_ = false;
        area_quest_time_rescan_scheduled_ = false;
        area_quest_rescan_requests_.store(0, std::memory_order_release);
        reset_area_quest_task_class_runtime(
            area_quest_state_provider_ready_
            && area_quest_catalog_.size() == kExpectedAreaQuestCount);
        area_quest_eligibility_.fill(0);
        area_quest_world_map_eligibility_.fill(0);
        area_quest_save_completion_.fill(0);
        area_quest_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_previous_states_.fill(
            dswros::AreaQuestState::Unknown);
        area_quest_completion_observed_.fill(false);
        area_quest_static_proofs_.fill(
            dswros::AreaQuestEligibilityProof::Unknown);
        normal_quest_completion_proofs_.clear();
        completed_dynamic_quest_ids_.clear();
        area_quest_save_completion_query_available_ = false;
        area_quest_save_completion_match_count_ = 0;
        area_quest_prerequisite_query_count_ = 0;
        area_quest_prerequisite_fault_count_ = 0;
        const auto definition_started = Clock::now();
        const bool definitions_captured =
            capture_area_quest_definitions_guarded();
        const auto definition_elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - definition_started).count();
        append_log("AREA_QUEST_DEFINITION_SNAPSHOT", std::format(
            "activation={} ready={} matches={} catalog={} unknown_conditions={} monster_conditions={} monster_links={} monster_ambiguous={} weighted_selection={} elapsed_us={} retry=next_f8_f7",
            activation_, definitions_captured,
            area_quest_definition_match_count_, area_quest_catalog_.size(),
            area_quest_definition_unknown_condition_count_,
            area_quest_definition_monster_condition_count_,
            area_quest_definition_monster_link_count_,
            area_quest_definition_monster_ambiguous_count_,
            area_quest_definition_weighted_selection_count_,
            definition_elapsed_us));
        request_area_quest_scan("f7");
        if (!capture_activation_context_guarded(engine)) {
            current_context_key_.clear();
            current_world_key_.clear();
        }
        next_position_ = stable_after_;
        next_discovery_ = stable_after_;
        next_activity_probe_ = stable_after_;
        next_runtime_visibility_edge_probe_ = stable_after_;
        bool world_map_resumed{};
        if (!activity_suppressed_) {
            std::string activation_world_key{};
            UWorld* activation_world{};
            static_cast<void>(capture_current_world_identity_guarded(
                engine, &activation_world_key, &activation_world));
            world_map_activation_catch_up(activation_world);
            world_map_resumed =
                resume_suspended_world_map_after_f7();
        }
        append_log("F7_ACTIVATED", std::format(
            "activation={} epoch={} save_sync=native_one_shot_pending context_baseline={} world_baseline={} compact_paint_suppressed={} compact_state={} compact_attach_gate={} compact_catalog={} compact_capacity={} world_map_resumed={} world_map_state={} world_map_markers={} world_map_suspends={} world_map_resumes={}",
            activation_, epoch_, baseline_context_key_.empty() ? "unavailable" : baseline_context_key_,
            baseline_world_key_.empty() ? "unavailable" : baseline_world_key_, activity_suppressed_,
            static_cast<std::uint32_t>(compact_umg_renderer_.state()),
            static_cast<std::uint32_t>(compact_attach_gate_status_),
            render_catalog_size_, dsnwr::kCompactUmgMarkerCapacity,
            world_map_resumed,
            static_cast<std::uint32_t>(world_map_umg_renderer_.state()),
            world_map_umg_renderer_.active_marker_count(),
            world_map_umg_renderer_.suspend_count(),
            world_map_umg_renderer_.resume_count()));
    }

    void disable(bool preserve_visibility_hub = false) {
        // F8 stops future object work, but an already accepted fixed-bit death
        // notification is completed before activation-local state is reset.
        consume_pending_encounter_deaths(true);
        const auto compact_attach_gate_status =
            compact_attach_gate_status_;
        if (!preserve_visibility_hub) {
            visibility_hub_.detach();
            visibility_hub_world_map_refresh_pending_ = false;
            visibility_hub_world_map_baseline_valid_ = false;
            visibility_hub_service_after_ = {};
            visibility_hub_toggle_after_ = {};
            visibility_hub_open_pending_ = false;
            visibility_hub_open_retry_after_ = {};
            visibility_hub_open_pending_until_ = {};
        }
        compact_umg_renderer_.detach();
        reset_compact_pool_runtime();
        if (world_map_content_visibility_intent()) {
            world_map_umg_renderer_.suspend();
        } else {
            world_map_umg_renderer_.detach();
        }
        const bool preserve_world_map_candidate =
            world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Suspended
            && world_map_candidate_available_;
        reset_world_map_runtime(preserve_world_map_candidate);
        enabled_ = false;
        position_valid_ = false;
        world_map_compact_suppressed_ = false;
        game_paused_ = false;
        game_pause_sample_known_ = false;
        world_time_available_ = false;
        last_area_quest_world_hour_ = -1;
        activity_suppressed_ = false;
        area_quest_scan_pending_ = false;
        area_quest_rescan_scheduled_ = false;
        area_quest_time_rescan_scheduled_ = false;
        area_quest_rescan_requests_.store(0, std::memory_order_release);
        reset_area_quest_task_class_runtime(false);
        area_quest_state_ready_ = false;
        area_quest_eligibility_.fill(0);
        area_quest_world_map_eligibility_.fill(0);
        area_quest_save_completion_.fill(0);
        area_quest_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_previous_states_.fill(
            dswros::AreaQuestState::Unknown);
        area_quest_completion_observed_.fill(false);
        area_quest_static_proofs_.fill(
            dswros::AreaQuestEligibilityProof::Unknown);
        normal_quest_completion_proofs_.clear();
        completed_dynamic_quest_ids_.clear();
        area_quest_save_completion_query_available_ = false;
        area_quest_save_completion_match_count_ = 0;
        save_reconcile_inflight_request_id_ = 0;
        save_reconcile_request_after_ = {};
        area_quest_save_confirmation_pending_.fill(0);
        area_quest_save_confirmation_inflight_.fill(0);
        area_quest_save_confirmation_due_.fill({});
        area_quest_save_confirmation_attempts_.fill(0);
        area_quest_save_confirmation_next_due_ = {};
        area_quest_save_confirmation_pending_armed_ = false;
        area_quest_save_completion_counts_.fill(
            dswros::kUnknownAreaQuestSaveCompletionCount);
        area_quest_completion_generation_locked_.fill(false);
        area_quest_completion_generation_reactivation_armed_.fill(false);
        treasure_save_confirmation_pending_.fill(0);
        treasure_save_confirmation_inflight_.fill(0);
        treasure_save_confirmation_due_.fill({});
        treasure_save_confirmation_attempts_.fill(0);
        treasure_save_confirmation_next_due_ = {};
        treasure_save_confirmation_pending_armed_ = false;
        ++epoch_;
        // Do not clear an exact accepted death bit that could not be applied
        // during the boundary drain. It carries no UObject or world pointer.
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        reset_bird_egg_active_visibility();
        append_log("F8_DISABLED", std::format(
            "activation={} epoch={} game_tick_object_work=stopped create_listener=event_only compact_state={} compact_attach_gate={} compact_attempts={} compact_attaches={} compact_detaches={} compact_rebinds={} compact_translations={} compact_height_transforms={} compact_height_skips={} compact_nearest_holds={} compact_nearest_switches={} compact_faults={} compact_last_attach_failure={} world_map_state={} world_map_markers={} world_map_suspends={} world_map_resumes={} world_map_detaches={} world_map_atlas_build_us={} world_map_atlas_file_bytes={} bird_egg_slots={} bird_egg_captures={} bird_egg_drops={} bird_egg_active={} bird_egg_position_queries={} bird_egg_discovery_total_us={} bird_egg_discovery_max_us={} bird_egg_active_changes={} bird_egg_availability_queries={} bird_egg_availability_unknown={} bird_egg_available_samples={} bird_egg_unavailable_samples={} bird_egg_end_events={}",
            activation_, epoch_,
            static_cast<std::uint32_t>(compact_umg_renderer_.state()),
            static_cast<std::uint32_t>(compact_attach_gate_status),
            compact_umg_renderer_.attach_attempt_count(),
            compact_umg_renderer_.attach_count(), compact_umg_renderer_.detach_count(),
            compact_umg_renderer_.rebind_count(),
            compact_umg_renderer_.translation_count(),
            compact_umg_renderer_.height_transform_count(),
            compact_umg_renderer_.height_transform_skip_count(),
            compact_nearest_hold_count_,
            compact_nearest_switch_count_,
            compact_umg_renderer_.fault_count(),
            compact_umg_renderer_.last_attach_failure(),
            static_cast<std::uint32_t>(world_map_umg_renderer_.state()),
            world_map_umg_renderer_.active_marker_count(),
            world_map_umg_renderer_.suspend_count(),
            world_map_umg_renderer_.resume_count(),
            world_map_umg_renderer_.detach_count(),
            world_map_umg_renderer_.atlas_build_elapsed_us(),
            world_map_umg_renderer_.atlas_file_bytes(),
            created_bird_egg_candidates_.size(),
            bird_egg_candidate_capture_count_.load(
                std::memory_order_relaxed),
            bird_egg_candidate_drop_count_.load(
                std::memory_order_relaxed),
            bird_egg_active_count_, bird_egg_position_query_count_,
            bird_egg_discovery_total_us_, bird_egg_discovery_max_us_,
            bird_egg_active_change_count_,
            bird_egg_availability_query_count_,
            bird_egg_availability_unknown_count_,
            bird_egg_available_sample_count_,
            bird_egg_unavailable_sample_count_,
            bird_egg_end_event_count_));
    }

    void disable_for_main_menu_owner_boundary(const char* reason) noexcept {
        if (main_menu_activation_latched_) {
            enabled_ = false;
            transition_active_ = false;
            return;
        }
        const bool was_enabled = enabled_;
        const std::size_t opened_treasure_count =
            runtime_opened_treasure_ids_.size();
        const std::size_t encounter_cooldown_count =
            encounter_next_available_unix_seconds_.size();
        main_menu_activation_latched_ = true;

        // Ordinary F8 deliberately preserves same-save session deltas. The
        // title map is instead a save-owner boundary: stop the normal runtime
        // first, then clear every mutable value that could belong to the
        // previous save. Immutable catalogs, user visibility masks, ignored
        // treasure IDs, and the UObject creation listener remain intact.
        disable();
        visibility_hub_.detach();
        visibility_hub_world_map_refresh_pending_ = false;
        visibility_hub_world_map_baseline_valid_ = false;
        visibility_hub_service_after_ = {};
        visibility_hub_toggle_after_ = {};
        compact_umg_renderer_.detach();
        reset_compact_pool_runtime();
        world_map_umg_renderer_.detach();
        reset_world_map_runtime(false);
        clear_world_map_listener_candidate();
        clear_compact_listener_candidate();
        clear_created_encounter_candidates();
        clear_bird_egg_candidates();

        runtime_opened_treasure_ids_.clear();
        encounter_next_available_unix_seconds_.clear();
        next_encounter_cooldown_edge_unix_seconds_ = 0;
        encounter_visibility_mask_ = 0;
        encounter_visibility_mask_valid_ = false;
        encounter_state_ready_ = false;
        pending_encounter_death_mask_.store(0, std::memory_order_release);
        pending_encounter_death_process_mask_.store(
            0, std::memory_order_release);

        save_reconcile_requested_ = false;
        save_reconcile_completed_ = false;
        save_reconcile_inflight_request_id_ = 0;
        save_reconcile_request_after_ = {};
        area_quest_save_confirmation_pending_.fill(0);
        area_quest_save_confirmation_inflight_.fill(0);
        area_quest_save_confirmation_due_.fill({});
        area_quest_save_confirmation_attempts_.fill(0);
        area_quest_save_confirmation_next_due_ = {};
        area_quest_save_confirmation_pending_armed_ = false;
        area_quest_save_completion_counts_.fill(
            dswros::kUnknownAreaQuestSaveCompletionCount);
        area_quest_completion_generation_locked_.fill(false);
        area_quest_completion_generation_reactivation_armed_.fill(false);
        treasure_save_confirmation_pending_.fill(0);
        treasure_save_confirmation_inflight_.fill(0);
        treasure_save_confirmation_due_.fill({});
        treasure_save_confirmation_attempts_.fill(0);
        treasure_save_confirmation_next_due_ = {};
        treasure_save_confirmation_pending_armed_ = false;

        compact_eligibility_.fill(0);
        mini_game_eligibility_.fill(0);
        treasure_eligibility_ready_ = false;
        reset_area_quest_task_class_runtime(false);
        area_quest_scan_pending_ = false;
        area_quest_rescan_scheduled_ = false;
        area_quest_time_rescan_scheduled_ = false;
        area_quest_rescan_requests_.store(0, std::memory_order_release);
        area_quest_state_ready_ = false;
        area_quest_definition_ready_ = false;
        area_quest_eligibility_.fill(0);
        area_quest_scan_eligibility_.fill(0);
        area_quest_world_map_eligibility_.fill(0);
        area_quest_save_completion_.fill(0);
        area_quest_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_previous_states_.fill(
            dswros::AreaQuestState::Unknown);
        area_quest_completion_observed_.fill(false);
        area_quest_scan_completion_observed_.fill(false);
        area_quest_static_proofs_.fill(
            dswros::AreaQuestEligibilityProof::Unknown);
        normal_quest_completion_proofs_.clear();
        completed_dynamic_quest_ids_.clear();
        area_quest_save_completion_query_available_ = false;
        area_quest_save_completion_match_count_ = 0;

        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        player_ = {};
        compact_render_anchor_ = {};
        position_valid_ = false;
        mouse_cursor_visible_ = false;
        world_time_available_ = false;
        world_time_baseline_seconds_ = 0;
        world_time_baseline_at_ = {};
        clock_capture_attempted_ = false;
        last_area_quest_world_hour_ = -1;
        activity_suppressed_ = false;
        world_map_compact_suppressed_ = false;
        game_paused_ = false;
        game_pause_sample_known_ = false;
        transition_active_ = false;
        baseline_world_key_.clear();
        baseline_context_key_.clear();
        current_world_key_.clear();
        current_context_key_.clear();
        engine_tick_fault_pending_ = false;
        engine_tick_fault_was_enabled_ = false;
        engine_tick_fault_recovery_in_progress_ = false;
        if (!engine_tick_fault_cleanup_failed_) {
            engine_tick_fault_terminal_ = false;
        }
        f6_requests_.store(0, std::memory_order_release);

        try {
            append_log("MAIN_MENU_DISABLED", std::format(
                "activation={} epoch={} reason={} was_enabled={} cleared_runtime_treasures={} cleared_encounter_cooldowns={} save_owner_state=cleared renderers=detached listener=retained action=require_explicit_f7_after_open_world_load",
                activation_, epoch_, reason ? reason : "exact_title_map",
                was_enabled, opened_treasure_count,
                encounter_cooldown_count));
        } catch (...) {
        }
    }

    void report_compact_pool_state_change() {
        const auto state =
            static_cast<std::uint32_t>(compact_umg_renderer_.state());
        const auto last_attach_failure =
            compact_umg_renderer_.last_attach_failure();
        if (state == reported_compact_state_
            && compact_refresh_status_code_
                == reported_compact_refresh_status_code_
            && last_attach_failure
                == reported_compact_last_attach_failure_) {
            return;
        }
        reported_compact_state_ = state;
        reported_compact_refresh_status_code_ = compact_refresh_status_code_;
        reported_compact_last_attach_failure_ = last_attach_failure;
        append_log("COMPACT_POOL_STATE", std::format(
            "state={} attach_gate={} refresh_status={} active_markers={} encounter_event_slots={} bird_egg_active={} attempts={} attaches={} detaches={} rebinds={} translations={} height_transforms={} height_skips={} nearest_holds={} nearest_switches={} suppressions={} faults={} last_attach_failure={} abi_failures={}",
            state, static_cast<std::uint32_t>(compact_attach_gate_status_),
            compact_refresh_status_code_, compact_marker_count_,
            created_encounter_candidates_.size(),
            bird_egg_active_count_,
            compact_umg_renderer_.attach_attempt_count(),
            compact_umg_renderer_.attach_count(),
            compact_umg_renderer_.detach_count(),
            compact_umg_renderer_.rebind_count(),
            compact_umg_renderer_.translation_count(),
            compact_umg_renderer_.height_transform_count(),
            compact_umg_renderer_.height_transform_skip_count(),
            compact_nearest_hold_count_,
            compact_nearest_switch_count_,
            compact_umg_renderer_.suppression_change_count(),
            compact_umg_renderer_.fault_count(),
            compact_umg_renderer_.last_attach_failure(),
            compact_umg_renderer_.abi_failure_mask()));
    }
    void report_world_map_state_change() {
        const auto state = static_cast<std::uint32_t>(
            world_map_umg_renderer_.state());
        const auto last_attach_failure =
            world_map_umg_renderer_.last_attach_failure();
        if (state == reported_world_map_state_
            && world_map_refresh_status_code_
                == reported_world_map_refresh_status_code_
            && last_attach_failure
                == reported_world_map_last_attach_failure_) {
            return;
        }
        reported_world_map_state_ = state;
        reported_world_map_refresh_status_code_ =
            world_map_refresh_status_code_;
        reported_world_map_last_attach_failure_ = last_attach_failure;
        append_log("WORLD_MAP_STATE", std::format(
            "state={} refresh_status={} active_markers={} first_id={} last_id={} attempts={} attaches={} detaches={} suspends={} resumes={} faults={} map_data_lookups={} map_data_cache_hits={} map_data_source={} last_attach_failure={} abi_failures={} map_id={} paint_owner={} atlas_build_us={} atlas_file_bytes={}",
            state, world_map_refresh_status_code_,
            world_map_umg_renderer_.active_marker_count(),
            world_map_first_marker_id_, world_map_last_marker_id_,
            world_map_umg_renderer_.attach_attempt_count(),
            world_map_umg_renderer_.attach_count(),
            world_map_umg_renderer_.detach_count(),
            world_map_umg_renderer_.suspend_count(),
            world_map_umg_renderer_.resume_count(),
            world_map_umg_renderer_.fault_count(),
            world_map_umg_renderer_.map_data_lookup_count(),
            world_map_umg_renderer_.map_data_cache_hit_count(),
            world_map_umg_renderer_.last_map_data_source(),
            last_attach_failure,
            world_map_umg_renderer_.abi_failure_mask(),
            world_map_umg_renderer_.map_id(),
            static_cast<std::uint32_t>(
                world_map_umg_renderer_.paint_owner_status()),
            world_map_umg_renderer_.atlas_build_elapsed_us(),
            world_map_umg_renderer_.atlas_file_bytes()));
    }

    void world_map_image_post(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!game_thread() || !context.Context
            || !required_runtime_ready_.load(std::memory_order_acquire)
            || shutting_down_.load(std::memory_order_acquire)) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            capture_world_map_candidate_unsafe(context.Context, true);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#else
        try {
            capture_world_map_candidate_unsafe(context.Context, true);
        } catch (...) {
        }
#endif
    }

    void world_map_zoom_post(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!game_thread() || !context.Context || !enabled_
            || transition_active_ || activity_suppressed_
            || !world_map_zoom_schema_ready_
            || !required_runtime_ready_.load(std::memory_order_acquire)
            || shutting_down_.load(std::memory_order_acquire)) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            world_map_zoom_post_unsafe(context.Context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#else
        try {
            world_map_zoom_post_unsafe(context.Context);
        } catch (...) {
        }
#endif
    }

    void world_map_zoom_post_unsafe(UObject* panel) {
        if (!panel || !panel->IsA(world_map_panel_class_)
            || !world_map_candidate_has_open_evidence()
            || !world_map_compact_suppressed_
            || world_map_umg_renderer_.state()
                != dsnwr::WorldMapUmgRendererState::Attached) {
            return;
        }
        void* layer_value = world_map_panel_layer_property_
            ->ContainerPtrToValuePtr<void>(panel);
        UObject* current_layer = layer_value
            ? world_map_panel_layer_property_
                ->GetObjectPropertyValue(layer_value)
            : nullptr;
        UObject* retained_layer = world_map_layer_candidate_.Get();
        if (!current_layer || current_layer != retained_layer
            || !world_map_umg_renderer_.attached_layer_matches(
                current_layer)) {
            return;
        }
        arm_world_map_layering_refresh(
            Clock::now(), WorldMapLayeringTrigger::ZoomChanged,
            WorldMapLayeringArmPolicy::Coalesce);
    }

    void capture_world_map_candidate_unsafe(
        UObject* current_layer, bool set_world_map_image_event) {
        if (!current_layer) {
            return;
        }
        UObject* retained = world_map_layer_candidate_.Get();
        const bool same_layer = world_map_candidate_available_
            && retained == current_layer;
        const auto candidate_transition =
            dswros::world_map_candidate_transition({
                true,
                same_layer,
                set_world_map_image_event,
                world_map_candidate_has_open_evidence(),
            });
        const bool exact_attached_layer = same_layer
            && world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Attached
            && world_map_umg_renderer_.attached_layer_matches(current_layer);
        const bool content_visible = world_map_content_visibility_intent();
        if (set_world_map_image_event) {
            // This exact DLayerMap event is the authoritative map-open edge.
            // Controller navigation does not necessarily expose a mouse
            // cursor, so suppress compact paint immediately. Collapse the
            // independent world-map hosts only for a new or mismatched layer;
            // the exact already-attached layer remains visible while its
            // finite transform-only settle tail runs.
            if (!exact_attached_layer) {
                world_map_umg_renderer_.publish_runtime_visibility(false);
            }
            latch_world_map_compact_suppression("set_world_map_image");
            if (same_layer) {
                const bool new_open_edge =
                    !world_map_candidate_has_open_evidence();
                world_map_open_serial_ = world_map_candidate_serial_;
                if (new_open_edge) {
                    world_map_visible_serial_ = 0;
                    world_map_open_evidence_at_ = Clock::now();
                }
                if (content_visible && !exact_attached_layer) {
                    world_map_session_pending_ = true;
                    world_map_service_retry_after_ = {};
                }
            }
        }
        if (dswros::world_map_candidate_replaced(
                candidate_transition)) {
            // A different DLayerMap is a different UMG lifetime even when an
            // allocator later reuses the same address (A -> B -> A). Retire
            // the independent Mod hosts and every finite session budget
            // before binding the replacement. The native Canvas remains
            // read-only throughout this transition.
            world_map_umg_renderer_.detach();
            if (enabled_ && !transition_active_
                && !activity_suppressed_) {
                world_map_umg_renderer_.begin_activation();
            }
            reset_world_map_runtime(false);
            world_map_layer_candidate_ = current_layer;
            world_map_candidate_available_ = true;
            world_map_set_image_rearm_consumed_ = false;
            world_map_open_serial_ = set_world_map_image_event
                ? world_map_candidate_serial_ : 0;
            world_map_visible_serial_ = 0;
            world_map_open_evidence_at_ = set_world_map_image_event
                ? Clock::now() : Clock::time_point{};
            world_map_session_pending_ = content_visible
                && set_world_map_image_event;
            if (!set_world_map_image_event) {
                world_map_compact_suppressed_ = false;
                apply_compact_suppression();
            }
            return;
        }
        if (same_layer && !content_visible) {
            return;
        }
        if (same_layer && !set_world_map_image_event) {
            // Listener/catch-up discovery records identity only. It must not
            // create a new candidate epoch or manufacture map-open evidence.
            return;
        }
        const bool retry_budget_consumed =
            (world_map_readiness_attempts_ > 0
                || world_map_service_attempts_ > 0)
            && world_map_serviced_serial_ == world_map_candidate_serial_;
        const bool completed_service =
            world_map_serviced_serial_ == world_map_candidate_serial_
            && (world_map_marker_snapshot_built_
                || (world_map_renderer_session_started_
                    && world_map_service_attempts_ == 0));
        const bool renderer_ready_for_candidate_rearm =
            world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Ready
            || (world_map_umg_renderer_.state()
                    == dsnwr::WorldMapUmgRendererState::Attached
                && !world_map_umg_renderer_.attached_layer_matches(
                    current_layer));
        const bool set_image_rearm_allowed = same_layer
            && set_world_map_image_event
            && enabled_
            && !transition_active_
            && !activity_suppressed_
            && content_visible
            && !world_map_set_image_rearm_consumed_
            && world_map_serviced_serial_ == world_map_candidate_serial_
            && (world_map_readiness_attempts_ > 0
                || world_map_service_attempts_ > 0)
            && renderer_ready_for_candidate_rearm
            && (world_map_session_pending_
                || world_map_umg_renderer_.retryable_not_ready()
                || world_map_readiness_attempts_
                    >= kWorldMapMaxReadinessAttempts
                || world_map_service_attempts_
                    >= kWorldMapMaxServiceAttempts);
        if (set_image_rearm_allowed) {
            const auto previous_readiness_attempts =
                world_map_readiness_attempts_;
            const auto previous_attempts = world_map_service_attempts_;
            const auto previous_failure =
                world_map_umg_renderer_.last_attach_failure();
            world_map_umg_renderer_.begin_activation();
            reset_world_map_runtime(true);
            world_map_set_image_rearm_consumed_ = true;
            append_log("WORLD_MAP_ATLAS_SET_IMAGE_REARM", std::format(
                "activation={} epoch={} candidate_serial={} previous_readiness_attempts={} previous_attempts={} previous_failure={} readiness_budget={} attach_budget={} reason=set_world_map_image_readiness_event_once_per_layer_ready_or_distinct_attached_layer",
                activation_, epoch_, world_map_candidate_serial_,
                previous_readiness_attempts, previous_attempts,
                previous_failure, kWorldMapMaxReadinessAttempts,
                kWorldMapMaxServiceAttempts));
            return;
        }
        const bool attached_layer_transform_sync = same_layer
            && set_world_map_image_event
            && enabled_
            && !transition_active_
            && !activity_suppressed_
            && content_visible
            && world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Attached
            && world_map_umg_renderer_.attached_layer_matches(
                current_layer);
        if (attached_layer_transform_sync) {
            // SetWorldMapImage may return before the map viewport settles.
            // Debounce repeated events and run one finite, transform-only
            // observation tail against the read-only native map geometry.
            arm_world_map_layering_refresh(
                Clock::now(), WorldMapLayeringTrigger::SetWorldMapImage,
                WorldMapLayeringArmPolicy::Coalesce);
            return;
        }
        if (same_layer
            && (world_map_session_pending_
                || world_map_umg_renderer_.state()
                    == dsnwr::WorldMapUmgRendererState::Attached
                || retry_budget_consumed
                || completed_service)) {
            // Repeated SetWorldMapImage/create-listener delivery for the same
            // live layer must not reset a completed no-marker/non-atlas
            // session or an exhausted bounded service budget. A deliberately
            // retired atlas clears both completion signals and is rearmed only
            // through its explicit current-session refresh.
            return;
        }
        // All replacement and first-bind cases return above. Reaching this
        // point means a duplicate same-layer event whose active or exhausted
        // session state must be preserved.
    }

    void arm_world_map_layering_refresh(
        Clock::time_point now, WorldMapLayeringTrigger trigger,
        WorldMapLayeringArmPolicy policy) noexcept {
        if (!world_map_content_visibility_intent()
            || !world_map_candidate_has_open_evidence()
            || !world_map_compact_suppressed_) {
            return;
        }
        const bool same_serial_tail_pending =
            world_map_layering_refresh_pending_
            && world_map_layering_refresh_serial_
                == world_map_candidate_serial_;
        if (policy == WorldMapLayeringArmPolicy::Coalesce
            && same_serial_tail_pending) {
            // Repeated notifications for the same live layer are evidence for
            // the already-running finite observation tail, not a new epoch.
            // Preserve its absolute deadlines and progress so event bursts
            // cannot starve the first sample or turn settling into a poll.
            return;
        }
        world_map_layering_refresh_pending_ = true;
        world_map_layering_refresh_started_ = now;
        world_map_layering_refresh_attempt_ = 0;
        world_map_layering_refresh_due_ = now
            + kWorldMapLayeringSettleDelays.front();
        world_map_layering_refresh_serial_ = world_map_candidate_serial_;
        world_map_layering_refresh_trigger_ = trigger;
    }

    void service_world_map_layering_refresh(Clock::time_point now) {
        if (!world_map_layering_refresh_pending_
            || now < world_map_layering_refresh_due_) {
            return;
        }
        if (!enabled_ || transition_active_ || activity_suppressed_
            || !world_map_content_visibility_intent()
            || !world_map_candidate_has_open_evidence()
            || !world_map_compact_suppressed_
            || world_map_layering_refresh_serial_
                != world_map_candidate_serial_
            || world_map_umg_renderer_.state()
                != dsnwr::WorldMapUmgRendererState::Attached) {
            world_map_layering_refresh_pending_ = false;
            return;
        }
        std::size_t attempt_index =
            world_map_layering_refresh_attempt_;
        // Service at most one read-only geometry observation per game-thread
        // pass. The full finite tail is retained even after an unchanged
        // sample because map zoom and viewport layout can settle later.
        if (attempt_index >= kWorldMapLayeringSettleDelays.size()) {
            world_map_layering_refresh_pending_ = false;
            return;
        }
        UObject* current_layer = world_map_layer_candidate_.Get();
        const bool final_attempt = attempt_index + 1U
            >= kWorldMapLayeringSettleDelays.size();
        dsnwr::WorldMapLayeringRefreshResult refresh_result{
            dsnwr::WorldMapLayeringRefreshResult::RetryLater};
        if (current_layer) {
            refresh_result =
                world_map_umg_renderer_.sync_viewport_transform(current_layer);
        } else {
            // The exact weak layer no longer exists, so runtime visibility is
            // definitively false. This is distinct from a renderer-returned
            // transform RetryLater result.
            world_map_umg_renderer_.publish_runtime_visibility(false);
        }
        if (refresh_result == dsnwr::WorldMapLayeringRefreshResult::Updated
            || refresh_result
                == dsnwr::WorldMapLayeringRefreshResult::Unchanged
            || refresh_result
                == dsnwr::WorldMapLayeringRefreshResult::Retained) {
            apply_world_map_atlas_visibility_guarded(
                object_world_guarded(current_layer), current_layer);
        }
        const char* layering_event =
            refresh_result == dsnwr::WorldMapLayeringRefreshResult::Updated
            ? "WORLD_MAP_VIEWPORT_TRANSFORM_UPDATED"
            : (refresh_result
                    == dsnwr::WorldMapLayeringRefreshResult::Unchanged
                ? "WORLD_MAP_VIEWPORT_TRANSFORM_UNCHANGED"
                : (refresh_result
                        == dsnwr::WorldMapLayeringRefreshResult::Retained
                    ? "WORLD_MAP_VIEWPORT_TRANSFORM_RETAINED"
                    : (refresh_result
                            == dsnwr::WorldMapLayeringRefreshResult::RetryLater
                        ? "WORLD_MAP_VIEWPORT_TRANSFORM_DEFERRED"
                        : "WORLD_MAP_VIEWPORT_TRANSFORM_FAILED")));
        append_log(
            layering_event,
            std::format(
                "activation={} epoch={} candidate_serial={} result={} trigger={} reason=bounded_read_only_native_geometry_sync ownership=independent_viewport_hosts native_canvas=read_only attempt={}/{} delay_ms={} final_attempt={} state={} failure={} transform_stage={}",
                activation_, epoch_, world_map_candidate_serial_,
                world_map_layering_result_name(refresh_result),
                world_map_layering_trigger_name(
                    world_map_layering_refresh_trigger_),
                attempt_index + 1U,
                kWorldMapLayeringSettleDelays.size(),
                kWorldMapLayeringSettleDelays[attempt_index].count(),
                final_attempt,
                static_cast<std::uint32_t>(
                    world_map_umg_renderer_.state()),
                world_map_umg_renderer_.last_attach_failure(),
                static_cast<std::uint32_t>(
                    world_map_umg_renderer_.last_transform_sync_stage())));
        if (refresh_result == dsnwr::WorldMapLayeringRefreshResult::Faulted) {
            world_map_layering_refresh_pending_ = false;
            return;
        }
        if (final_attempt) {
            world_map_layering_refresh_pending_ = false;
            return;
        }
        ++world_map_layering_refresh_attempt_;
        world_map_layering_refresh_due_ = world_map_layering_refresh_started_
            + kWorldMapLayeringSettleDelays[
                world_map_layering_refresh_attempt_];
    }

    [[nodiscard]] static bool capture_created_world_map_layer_guarded(
        const UObjectBase* object, FWeakObjectPtr* weak) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            auto* self = instance_.load(std::memory_order_acquire);
            auto* unreal_object = std::bit_cast<UObject*>(object);
            auto* object_class = unreal_object->GetClassPrivate();
            if (self && weak && self->world_map_layer_class_
                && object_class
                && unreal_object->IsA(self->world_map_layer_class_)) {
                *weak = unreal_object;
                captured = true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        auto* self = instance_.load(std::memory_order_acquire);
        auto* unreal_object = std::bit_cast<UObject*>(object);
        auto* object_class = unreal_object->GetClassPrivate();
        if (self && weak && self->world_map_layer_class_
            && object_class
            && unreal_object->IsA(self->world_map_layer_class_)) {
            *weak = unreal_object;
            captured = true;
        }
#endif
        return captured;
    }

    [[nodiscard]] static bool capture_created_compact_layer_guarded(
        const UObjectBase* object, FWeakObjectPtr* weak) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            auto* self = instance_.load(std::memory_order_acquire);
            auto* unreal_object = std::bit_cast<UObject*>(object);
            auto* object_class = unreal_object->GetClassPrivate();
            if (self && weak && self->compact_layer_class_
                && object_class
                && unreal_object->IsA(self->compact_layer_class_)) {
                *weak = unreal_object;
                captured = weak->ObjectIndex >= 0
                    && weak->ObjectSerialNumber > 0;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        try {
            auto* self = instance_.load(std::memory_order_acquire);
            auto* unreal_object = std::bit_cast<UObject*>(object);
            auto* object_class = unreal_object
                ? unreal_object->GetClassPrivate() : nullptr;
            if (self && weak && self->compact_layer_class_
                && object_class
                && unreal_object->IsA(self->compact_layer_class_)) {
                *weak = unreal_object;
                captured = weak->ObjectIndex >= 0
                    && weak->ObjectSerialNumber > 0;
            }
        } catch (...) {
            captured = false;
        }
#endif
        return captured;
    }

    void clear_world_map_listener_candidate() noexcept {
        const std::scoped_lock lock{world_map_listener_mutex_};
        world_map_listener_candidate_ = FWeakObjectPtr{};
        world_map_listener_object_index_ = -1;
        world_map_listener_pending_.store(
            false, std::memory_order_release);
    }

    void clear_compact_listener_candidate() noexcept {
        {
            const std::scoped_lock lock{compact_listener_mutex_};
            compact_listener_candidate_ = FWeakObjectPtr{};
            compact_listener_object_index_ = -1;
            compact_listener_pending_.store(
                false, std::memory_order_release);
        }
        compact_layer_candidate_ = FWeakObjectPtr{};
        compact_candidate_available_ = false;
    }

    void consume_compact_listener_candidate(
        UWorld* expected_world = nullptr) noexcept {
        if (!compact_listener_pending_.load(std::memory_order_acquire)) {
            return;
        }
        FWeakObjectPtr weak{};
        std::int32_t object_index{-1};
        {
            const std::scoped_lock lock{compact_listener_mutex_};
            if (!compact_listener_pending_.exchange(
                    false, std::memory_order_acq_rel)) {
                return;
            }
            weak = compact_listener_candidate_;
            object_index = compact_listener_object_index_;
            compact_listener_candidate_ = FWeakObjectPtr{};
            compact_listener_object_index_ = -1;
        }
        bool rearmed{};
        const bool captured = consume_compact_listener_candidate_guarded(
            weak, expected_world, &rearmed);
        if (captured) {
            try {
                append_log("COMPACT_LAYER_CAPTURE", std::format(
                    "source=create_listener activation={} epoch={} enabled={} transition={} suppressed={} serial={} object_index={} attach_rearmed={}",
                    activation_, epoch_, enabled_, transition_active_,
                    activity_suppressed_, compact_candidate_serial_,
                    object_index, rearmed));
            } catch (...) {
            }
        }
    }

    [[nodiscard]] bool consume_compact_listener_candidate_guarded(
        FWeakObjectPtr weak, UWorld* expected_world,
        bool* rearmed) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            captured = consume_compact_listener_candidate_unsafe(
                weak, expected_world, rearmed);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        try {
            captured = consume_compact_listener_candidate_unsafe(
                weak, expected_world, rearmed);
        } catch (...) {
            captured = false;
        }
#endif
        return captured;
    }

    [[nodiscard]] bool consume_compact_listener_candidate_unsafe(
        FWeakObjectPtr weak, UWorld* expected_world, bool* rearmed) {
        UObject* layer = weak.Get();
        if (!layer || !compact_layer_class_
            || !layer->IsA(compact_layer_class_)
            || (expected_world && layer->GetWorld() != expected_world)) {
            return false;
        }
        UObject* retained = compact_layer_candidate_.Get();
        const bool same_layer = compact_candidate_available_
            && retained == layer;
        if (!same_layer) {
            compact_layer_candidate_ = weak;
            compact_candidate_available_ = true;
            ++compact_candidate_serial_;
            if (enabled_ && !transition_active_
                && !activity_suppressed_) {
                // A distinct DLayerMiniMap identity is an exact UI
                // replacement event. Rearm once even when the old renderer
                // still reports Attached or Suppressed; its weak layer can
                // already be stale. Repeated delivery of the same identity is
                // filtered above.
                compact_umg_renderer_.begin_activation();
                reset_compact_pool_runtime();
                if (rearmed) {
                    *rearmed = true;
                }
            }
        }
        return true;
    }

    void initialize_encounter_class_name_keys() noexcept {
        encounter_class_name_keys_.fill(0);
        encounter_class_name_keys_ready_ =
            encounter_catalog_.size() == kExpectedEncounterCount;
        if (!encounter_class_name_keys_ready_) {
            return;
        }
        try {
            for (std::size_t index = 0; index < encounter_catalog_.size();
                 ++index) {
                const std::wstring class_name =
                    to_wstring(encounter_catalog_[index].class_name);
                const FName name{class_name.c_str(), FNAME_Add};
                const std::uint64_t key = DSNWRPR_NAME_KEY(name);
                if (key == 0
                    || std::find(
                           encounter_class_name_keys_.begin(),
                           encounter_class_name_keys_.begin() + index,
                           key)
                        != encounter_class_name_keys_.begin() + index) {
                    encounter_class_name_keys_ready_ = false;
                    encounter_class_name_keys_.fill(0);
                    return;
                }
                encounter_class_name_keys_[index] = key;
            }
        } catch (...) {
            encounter_class_name_keys_ready_ = false;
            encounter_class_name_keys_.fill(0);
        }
    }

    void initialize_bird_egg_class_name_keys() noexcept {
        bird_egg_class_name_keys_.fill(0);
        bird_egg_class_name_keys_ready_ = false;
        try {
            constexpr std::array<const wchar_t*, 2> class_names{{
                L"Bird_Egg01_C", L"Bird_Egg02_C"}};
            for (std::size_t index = 0; index < class_names.size(); ++index) {
                const FName name{class_names[index], FNAME_Add};
                const std::uint64_t key = DSNWRPR_NAME_KEY(name);
                if (key == 0
                    || (index != 0
                        && key == bird_egg_class_name_keys_[0])) {
                    bird_egg_class_name_keys_.fill(0);
                    return;
                }
                bird_egg_class_name_keys_[index] = key;
            }
            bird_egg_class_name_keys_ready_ = true;
        } catch (...) {
            bird_egg_class_name_keys_.fill(0);
        }
    }

    [[nodiscard]] bool initialize_bird_egg_availability_schema(
        UObject* actor) noexcept {
        if (bird_egg_availability_schema_ready_) {
            return true;
        }
        if (!actor) {
            return false;
        }
        auto* component_property = CastField<FObjectPropertyBase>(
            actor->GetPropertyByNameInChain(L"InteractComponent"));
        void* component_value = component_property
            ? component_property->ContainerPtrToValuePtr<void>(actor)
            : nullptr;
        UObject* component = component_value
            ? component_property->GetObjectPropertyValue(component_value)
            : nullptr;
        auto* interactable_property = component
            ? CastField<FEnumProperty>(
                  component->GetPropertyByNameInChain(L"InteractableValue"))
            : nullptr;
        auto* interact_type_property = component
            ? CastField<FEnumProperty>(
                  component->GetPropertyByNameInChain(L"InteractTypeValue"))
            : nullptr;
        auto* interactable_underlying_property = interactable_property
            ? interactable_property->GetUnderlyingProperty()
            : nullptr;
        auto* interact_type_underlying_property = interact_type_property
            ? interact_type_property->GetUnderlyingProperty()
            : nullptr;
        if (!component_property || !component || component->GetOuterPrivate() != actor
            || !interactable_property || !interactable_underlying_property
            || !interactable_underlying_property->IsInteger()
            || interactable_property->GetSize() != sizeof(std::uint8_t)
            || !interact_type_property || !interact_type_underlying_property
            || !interact_type_underlying_property->IsInteger()
            || interact_type_property->GetSize() != sizeof(std::uint8_t)) {
            return false;
        }
        bird_egg_interact_component_property_ = component_property;
        bird_egg_interactable_value_property_ = interactable_property;
        bird_egg_interactable_value_underlying_property_ =
            interactable_underlying_property;
        bird_egg_interact_type_property_ = interact_type_property;
        bird_egg_interact_type_underlying_property_ =
            interact_type_underlying_property;
        bird_egg_availability_schema_ready_ = true;
        return true;
    }

    [[nodiscard]] bool read_bird_egg_available(
        UObject* actor, bool* available) noexcept {
        if (!actor || !available
            || !initialize_bird_egg_availability_schema(actor)) {
            return false;
        }
        void* component_value = bird_egg_interact_component_property_
            ->ContainerPtrToValuePtr<void>(actor);
        UObject* component = component_value
            ? bird_egg_interact_component_property_
                  ->GetObjectPropertyValue(component_value)
            : nullptr;
        if (!component || component->GetOuterPrivate() != actor) {
            return false;
        }
        void* interactable_value = bird_egg_interactable_value_property_
            ->ContainerPtrToValuePtr<void>(component);
        void* interact_type_value = bird_egg_interact_type_property_
            ->ContainerPtrToValuePtr<void>(component);
        if (!interactable_value || !interact_type_value) {
            return false;
        }
        const auto interactable = static_cast<std::uint8_t>(
            bird_egg_interactable_value_underlying_property_
                ->GetUnsignedIntPropertyValue(interactable_value));
        const auto interact_type = static_cast<std::uint8_t>(
            bird_egg_interact_type_underlying_property_
                ->GetUnsignedIntPropertyValue(interact_type_value));
        *available = dswros::bird_egg_interaction_available(
            interactable, interact_type);
        return true;
    }

    [[nodiscard]] static bool capture_created_bird_egg_actor_unsafe(
        const UObjectBase* object, FWeakObjectPtr* weak) {
        auto* self = instance_.load(std::memory_order_acquire);
        auto* unreal_object = std::bit_cast<UObject*>(object);
        auto* object_class = unreal_object
            ? unreal_object->GetClassPrivate() : nullptr;
        if (!self || !weak || !self->actor_class_
            || !self->bird_egg_class_name_keys_ready_ || !object_class) {
            return false;
        }
        const std::uint64_t class_name_key =
            DSNWRPR_CLASS_NAME_KEY(object_class);
        if (class_name_key == 0
            || std::find(
                   self->bird_egg_class_name_keys_.begin(),
                   self->bird_egg_class_name_keys_.end(), class_name_key)
                == self->bird_egg_class_name_keys_.end()) {
            return false;
        }
        if (!unreal_object->IsA(self->actor_class_)) {
            return false;
        }
        *weak = unreal_object;
        return weak->ObjectIndex >= 0 && weak->ObjectSerialNumber > 0;
    }

    [[nodiscard]] static bool capture_created_bird_egg_actor_guarded(
        const UObjectBase* object, FWeakObjectPtr* weak) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            captured = capture_created_bird_egg_actor_unsafe(object, weak);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        try {
            captured = capture_created_bird_egg_actor_unsafe(object, weak);
        } catch (...) {
            captured = false;
        }
#endif
        return captured;
    }

    [[nodiscard]] std::size_t publish_created_bird_egg_candidate(
        FWeakObjectPtr weak) noexcept {
        const dswros::WeakIdentity identity{
            weak.ObjectIndex, weak.ObjectSerialNumber};
        if (!identity.valid()) {
            return created_bird_egg_candidates_.size();
        }
        const std::scoped_lock lock{bird_egg_candidate_mutex_};
        std::size_t empty = created_bird_egg_candidates_.size();
        for (std::size_t index = 0;
             index < created_bird_egg_candidates_.size(); ++index) {
            const auto& existing = created_bird_egg_candidates_[index];
            const dswros::WeakIdentity existing_identity{
                existing.ObjectIndex, existing.ObjectSerialNumber};
            if (existing_identity.valid()
                && existing_identity.packed() == identity.packed()) {
                return index;
            }
            if (!existing_identity.valid()
                && empty == created_bird_egg_candidates_.size()) {
                empty = index;
            }
        }
        if (empty == created_bird_egg_candidates_.size()) {
            bird_egg_candidate_drop_count_.fetch_add(
                1, std::memory_order_relaxed);
            return created_bird_egg_candidates_.size();
        }
        created_bird_egg_candidates_[empty] = weak;
        bird_egg_candidates_dirty_.store(true, std::memory_order_release);
        bird_egg_candidate_capture_count_.fetch_add(
            1, std::memory_order_relaxed);
        return empty;
    }

    void reset_bird_egg_active_visibility() noexcept {
        if (bird_egg_active_count_ == 0) {
            return;
        }
        bool changed{};
        for (auto& candidate : bird_egg_runtime_candidates_) {
            changed = changed || candidate.presence.visible();
            candidate.presence.reset();
        }
        if (changed || bird_egg_active_count_ != 0) {
            compact_rebind_dirty_ = true;
        }
        bird_egg_active_count_ = 0;
    }

    void clear_bird_egg_candidates() noexcept {
        {
            const std::scoped_lock lock{bird_egg_candidate_mutex_};
            created_bird_egg_candidates_.fill(FWeakObjectPtr{});
            bird_egg_candidates_dirty_.store(
                false, std::memory_order_release);
        }
        bird_egg_runtime_candidates_.fill(BirdEggRuntimeCandidate{});
        bird_egg_candidate_probe_cursor_ = 0;
        bird_egg_unresolved_count_ = 0;
        bird_egg_active_count_ = 0;
        compact_rebind_dirty_ = true;
    }

    void synchronize_bird_egg_candidates() noexcept {
        if (!bird_egg_candidates_dirty_.exchange(
                false, std::memory_order_acq_rel)) {
            return;
        }
        std::array<FWeakObjectPtr, kBirdEggCandidateCapacity> snapshot{};
        {
            const std::scoped_lock lock{bird_egg_candidate_mutex_};
            snapshot = created_bird_egg_candidates_;
        }
        std::size_t unresolved_count{};
        for (std::size_t index = 0; index < snapshot.size(); ++index) {
            const dswros::WeakIdentity next_identity{
                snapshot[index].ObjectIndex,
                snapshot[index].ObjectSerialNumber};
            auto& runtime = bird_egg_runtime_candidates_[index];
            if (!next_identity.valid()) {
                if (runtime.identity.valid()) {
                    if (runtime.presence.visible()) {
                        compact_rebind_dirty_ = true;
                    }
                    runtime = {};
                }
                continue;
            }
            if (!runtime.identity.valid()
                || runtime.identity.packed() != next_identity.packed()) {
                if (runtime.presence.visible()) {
                    compact_rebind_dirty_ = true;
                }
                runtime = {};
                runtime.weak = snapshot[index];
                runtime.identity = next_identity;
            }
            if (runtime.identity.valid() && !runtime.retired
                && !runtime.position_known) {
                ++unresolved_count;
            }
        }
        bird_egg_unresolved_count_ = unresolved_count;
    }

    void retire_bird_egg_candidate(std::size_t index) noexcept {
        if (index >= bird_egg_runtime_candidates_.size()) {
            return;
        }
        const auto identity = bird_egg_runtime_candidates_[index].identity;
        if (!identity.valid()) {
            bird_egg_runtime_candidates_[index] = {};
            return;
        }
        if (bird_egg_runtime_candidates_[index].presence.visible()) {
            compact_rebind_dirty_ = true;
        }
        if (!bird_egg_runtime_candidates_[index].position_known
            && bird_egg_unresolved_count_ != 0) {
            --bird_egg_unresolved_count_;
        }
        bird_egg_runtime_candidates_[index] = {};
        const std::scoped_lock lock{bird_egg_candidate_mutex_};
        const auto& current = created_bird_egg_candidates_[index];
        const dswros::WeakIdentity current_identity{
            current.ObjectIndex, current.ObjectSerialNumber};
        if (current_identity.valid()
            && current_identity.packed() == identity.packed()) {
            created_bird_egg_candidates_[index] = FWeakObjectPtr{};
        }
    }

    void mark_bird_egg_end(
        dswros::WeakIdentity identity) noexcept {
        if (!identity.valid()) {
            return;
        }
        synchronize_bird_egg_candidates();
        for (std::size_t index = 0;
             index < bird_egg_runtime_candidates_.size(); ++index) {
            const auto& candidate = bird_egg_runtime_candidates_[index];
            if (candidate.identity.valid()
                && candidate.identity.packed() == identity.packed()) {
                retire_bird_egg_candidate(index);
                ++bird_egg_end_event_count_;
                return;
            }
        }
    }

    void prune_bird_egg_candidates_for_world(
        UWorld* current_world) noexcept {
        if (!current_world) {
            clear_bird_egg_candidates();
            return;
        }
        std::array<FWeakObjectPtr, kBirdEggCandidateCapacity> retained{};
        std::size_t retained_count{};
        {
            const std::scoped_lock lock{bird_egg_candidate_mutex_};
            for (const auto& weak : created_bird_egg_candidates_) {
                UObject* object = weak.Get();
                if (!object
                    || object_world_guarded(object) != current_world
                    || retained_count >= retained.size()) {
                    continue;
                }
                retained[retained_count++] = weak;
            }
            created_bird_egg_candidates_ = retained;
            bird_egg_candidates_dirty_.store(
                true, std::memory_order_release);
        }
        bird_egg_runtime_candidates_.fill(BirdEggRuntimeCandidate{});
        bird_egg_candidate_probe_cursor_ = 0;
        bird_egg_unresolved_count_ = 0;
        bird_egg_active_count_ = 0;
        compact_rebind_dirty_ = true;
    }

    void discover_bird_egg_candidates() noexcept {
        if (!compact_visibility_enabled(
                dsnwr::RadarVisibilityCategory::BirdEggs)
            || activity_suppressed_ || !position_valid_
            || !engine_tick_engine_) {
            return;
        }
        synchronize_bird_egg_candidates();
        if (bird_egg_unresolved_count_ == 0) {
            return;
        }
        UObject* pawn = current_player_pawn(engine_tick_engine_);
        UWorld* current_world = object_world_guarded(pawn);
        if (!current_world) {
            return;
        }
        const auto started = Clock::now();
        std::size_t visited{};
        std::size_t position_queries{};
        while (visited < bird_egg_runtime_candidates_.size()
            && position_queries
                < kBirdEggPositionProbeBudgetPerControlTick) {
            const std::size_t index = bird_egg_candidate_probe_cursor_;
            bird_egg_candidate_probe_cursor_ =
                (bird_egg_candidate_probe_cursor_ + 1U)
                % bird_egg_runtime_candidates_.size();
            ++visited;
            auto& candidate = bird_egg_runtime_candidates_[index];
            if (!candidate.identity.valid() || candidate.retired
                || candidate.position_known) {
                continue;
            }
            UObject* actor = candidate.weak.Get();
            if (!actor || object_world_guarded(actor) != current_world) {
                retire_bird_egg_candidate(index);
                continue;
            }
            ++position_queries;
            dswros::Position position{};
            bool available{};
            const bool availability_known =
                read_bird_egg_available(actor, &available);
            ++bird_egg_availability_query_count_;
            if (!availability_known) {
                ++bird_egg_availability_unknown_count_;
                continue;
            }
            if (!available) {
                ++bird_egg_unavailable_sample_count_;
                continue;
            }
            if (!read_actor_position(actor, &position)) {
                continue;
            }
            ++bird_egg_available_sample_count_;
            candidate.position = position;
            candidate.position_known = true;
            if (bird_egg_unresolved_count_ != 0) {
                --bird_egg_unresolved_count_;
            }
        }
        bird_egg_position_query_count_ += position_queries;
        const auto elapsed = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now() - started).count());
        bird_egg_discovery_total_us_ += elapsed;
        bird_egg_discovery_max_us_ =
            std::max(bird_egg_discovery_max_us_, elapsed);
    }

    void service_active_bird_eggs(Clock::time_point now) noexcept {
        if (!compact_visibility_enabled(
                dsnwr::RadarVisibilityCategory::BirdEggs)
            || compact_render_suppressed() || !engine_tick_engine_
            || !compact_radius_valid_) {
            return;
        }
        UObject* pawn = current_player_pawn(engine_tick_engine_);
        UWorld* current_world = object_world_guarded(pawn);
        if (!current_world) {
            reset_bird_egg_active_visibility();
            return;
        }
        std::array<StaticRenderCandidate, kBirdEggActiveCapacity> nearest{};
        std::size_t nearest_count{};
        const double radius_squared =
            compact_render_radius_ * compact_render_radius_;
        for (std::size_t index = 0;
             index < bird_egg_runtime_candidates_.size(); ++index) {
            const auto& candidate = bird_egg_runtime_candidates_[index];
            if (!candidate.identity.valid() || candidate.retired
                || !candidate.position_known) {
                continue;
            }
            const double distance =
                planar_distance_squared(player_, candidate.position);
            if (!std::isfinite(distance) || distance > radius_squared) {
                continue;
            }
            if (nearest_count < nearest.size()) {
                nearest[nearest_count++] = {index, distance};
                continue;
            }
            auto worst = std::max_element(
                nearest.begin(), nearest.end(),
                [](const StaticRenderCandidate& left,
                   const StaticRenderCandidate& right) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                });
            if (distance < worst->planar_distance_squared
                || (distance == worst->planar_distance_squared
                    && index < worst->catalog_index)) {
                *worst = {index, distance};
            }
        }
        std::sort(
            nearest.begin(),
            nearest.begin() + static_cast<std::ptrdiff_t>(nearest_count),
            [](const StaticRenderCandidate& left,
               const StaticRenderCandidate& right) {
                if (left.planar_distance_squared
                    != right.planar_distance_squared) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                }
                return left.catalog_index < right.catalog_index;
            });
        std::array<bool, kBirdEggCandidateCapacity> selected{};
        const auto now_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
        for (std::size_t item = 0; item < nearest_count; ++item) {
            const std::size_t index = nearest[item].catalog_index;
            selected[index] = true;
            auto& candidate = bird_egg_runtime_candidates_[index];
            UObject* actor = candidate.weak.Get();
            const bool weak_identity_valid = actor != nullptr;
            const bool same_world = weak_identity_valid
                && object_world_guarded(actor) == current_world;
            bool available{};
            const bool availability_known = same_world
                && read_bird_egg_available(actor, &available);
            ++bird_egg_availability_query_count_;
            if (!availability_known) {
                ++bird_egg_availability_unknown_count_;
            } else if (available) {
                ++bird_egg_available_sample_count_;
            } else {
                ++bird_egg_unavailable_sample_count_;
            }
            const bool present = dswros::bird_egg_runtime_present(
                weak_identity_valid, same_world,
                availability_known, available);
            const bool was_visible = candidate.presence.visible();
            const bool visible = candidate.presence.sample(
                present, now_milliseconds);
            if (visible != was_visible) {
                compact_rebind_dirty_ = true;
            }
            if (!weak_identity_valid || !same_world
                || (!visible && availability_known && !available)) {
                retire_bird_egg_candidate(index);
            }
        }
        for (std::size_t index = 0;
             index < bird_egg_runtime_candidates_.size(); ++index) {
            auto& candidate = bird_egg_runtime_candidates_[index];
            if (!selected[index] && candidate.presence.visible()) {
                candidate.presence.reset();
                compact_rebind_dirty_ = true;
            }
        }
        const std::size_t previous_active = bird_egg_active_count_;
        bird_egg_active_count_ = static_cast<std::size_t>(std::count_if(
            bird_egg_runtime_candidates_.begin(),
            bird_egg_runtime_candidates_.end(),
            [](const BirdEggRuntimeCandidate& candidate) {
                return candidate.presence.visible();
            }));
        if (bird_egg_active_count_ != previous_active) {
            ++bird_egg_active_change_count_;
        }
    }

    [[nodiscard]] static bool capture_created_encounter_actor_unsafe(
        const UObjectBase* object, FWeakObjectPtr* weak,
        std::size_t* encounter_index) {
        auto* self = instance_.load(std::memory_order_acquire);
        auto* unreal_object = std::bit_cast<UObject*>(object);
        auto* object_class = unreal_object
            ? unreal_object->GetClassPrivate() : nullptr;
        if (!self || !weak || !encounter_index || !self->actor_class_
            || !self->encounter_class_name_keys_ready_
            || !object_class
            || !unreal_object->IsA(self->actor_class_)) {
            return false;
        }
        const std::uint64_t class_name_key =
            DSNWRPR_CLASS_NAME_KEY(object_class);
        const auto found = std::find(
            self->encounter_class_name_keys_.begin(),
            self->encounter_class_name_keys_.end(), class_name_key);
        if (class_name_key == 0
            || found == self->encounter_class_name_keys_.end()) {
            return false;
        }
        *weak = unreal_object;
        *encounter_index = static_cast<std::size_t>(
            found - self->encounter_class_name_keys_.begin());
        return weak->ObjectIndex >= 0 && weak->ObjectSerialNumber > 0;
    }

    [[nodiscard]] static bool capture_created_encounter_actor_guarded(
        const UObjectBase* object, FWeakObjectPtr* weak,
        std::size_t* encounter_index) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            captured = capture_created_encounter_actor_unsafe(
                object, weak, encounter_index);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        try {
            captured = capture_created_encounter_actor_unsafe(
                object, weak, encounter_index);
        } catch (...) {
            captured = false;
        }
#endif
        return captured;
    }

    void clear_created_encounter_candidates() noexcept {
        const std::scoped_lock lock{created_encounter_mutex_};
        created_encounter_candidates_.fill({});
        created_encounter_processed_identity_.fill(0);
        created_encounter_processed_activation_.fill(0);
        encounter_candidate_probe_cursor_ = 0;
    }

    void clear_processed_encounter_identity(std::int64_t id) noexcept {
        for (std::size_t index = 0; index < encounter_catalog_.size();
             ++index) {
            if (encounter_catalog_[index].id != id) {
                continue;
            }
            created_encounter_processed_identity_[index] = 0;
            created_encounter_processed_activation_[index] = 0;
            return;
        }
    }

    void consume_created_encounter_candidates() {
        if (activity_suppressed_ || !position_valid_) {
            return;
        }
        std::array<FWeakObjectPtr, kExpectedEncounterCount> candidates{};
        {
            const std::scoped_lock lock{created_encounter_mutex_};
            candidates = created_encounter_candidates_;
        }
        const double radius_squared =
            kEncounterObservationRadius * kEncounterObservationRadius;
        const auto now_unix_seconds = unix_seconds();
        std::size_t visited{};
        std::size_t position_queries{};
        while (visited < candidates.size()
            && position_queries
                < kEncounterCandidateProbeBudgetPerControlTick) {
            const std::size_t index = encounter_candidate_probe_cursor_;
            encounter_candidate_probe_cursor_ =
                (encounter_candidate_probe_cursor_ + 1U)
                % candidates.size();
            ++visited;
            if (index >= encounter_catalog_.size()
                || !encounter_state_ready_
                || !encounter_available(
                    encounter_catalog_[index], now_unix_seconds)) {
                continue;
            }
            auto* actor = static_cast<AActor*>(candidates[index].Get());
            if (!actor) {
                continue;
            }
            const dswros::WeakIdentity identity{
                candidates[index].ObjectIndex,
                candidates[index].ObjectSerialNumber};
            if (!identity.valid()
                || (created_encounter_processed_activation_[index]
                        == activation_
                    && created_encounter_processed_identity_[index]
                        == identity.packed())) {
                continue;
            }
            dswros::Position actor_position{};
            ++position_queries;
            if (!read_actor_position(actor, &actor_position)
                || distance_squared(player_, actor_position)
                    > radius_squared) {
                continue;
            }
            observe_encounter(
                candidates[index], identity,
                encounter_catalog_[index].class_name,
                actor_position);
            const auto observed = observed_objects_.find(identity.packed());
            if (observed != observed_objects_.end()
                && observed->second.kind
                    == dswros::EventKind::EncounterDefeated
                && observed->second.id == encounter_catalog_[index].id) {
                created_encounter_processed_identity_[index] =
                    identity.packed();
                created_encounter_processed_activation_[index] =
                    activation_;
            }
        }
    }

    void consume_world_map_listener_candidate(
        UWorld* expected_world = nullptr) noexcept {
        if (!world_map_listener_pending_.load(std::memory_order_acquire)) {
            return;
        }
        FWeakObjectPtr weak{};
        std::int32_t object_index{-1};
        {
            const std::scoped_lock lock{world_map_listener_mutex_};
            if (!world_map_listener_pending_.exchange(
                    false, std::memory_order_acq_rel)) {
                return;
            }
            weak = world_map_listener_candidate_;
            object_index = world_map_listener_object_index_;
            world_map_listener_candidate_ = FWeakObjectPtr{};
            world_map_listener_object_index_ = -1;
        }
        if (consume_world_map_listener_candidate_guarded(
                weak, expected_world)) {
            append_log("WORLD_MAP_LAYER_CAPTURE", std::format(
                "source=create_listener activation={} epoch={} enabled={} serial={} object_index={}",
                activation_, epoch_, enabled_, world_map_candidate_serial_,
                object_index));
        }
    }

    [[nodiscard]] bool consume_world_map_listener_candidate_guarded(
        FWeakObjectPtr weak, UWorld* expected_world) noexcept {
        bool captured{};
#if defined(_MSC_VER)
        __try {
            captured = consume_world_map_listener_candidate_unsafe(
                weak, expected_world);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            captured = false;
        }
#else
        captured = consume_world_map_listener_candidate_unsafe(
            weak, expected_world);
#endif
        return captured;
    }

    [[nodiscard]] bool consume_world_map_listener_candidate_unsafe(
        FWeakObjectPtr weak, UWorld* expected_world) {
        UObject* current_layer = weak.Get();
        if (!current_layer || !world_map_layer_class_
            || !current_layer->IsA(world_map_layer_class_)
            || (expected_world
                && current_layer->GetWorld() != expected_world)) {
            return false;
        }
        capture_world_map_candidate_unsafe(current_layer, false);
        return true;
    }

    void world_map_activation_catch_up(
        UWorld* expected_world = nullptr) noexcept {
        if (!enabled_ || transition_active_) {
            return;
        }
        if (world_map_layer_catch_up_attempted_) {
            return;
        }
        world_map_layer_catch_up_attempted_ = true;
#if defined(_MSC_VER)
        __try {
            world_map_activation_catch_up_unsafe(expected_world);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            log_world_map_activation_catch_up_fault();
        }
#else
        world_map_activation_catch_up_unsafe(expected_world);
#endif
    }

    void world_map_activation_catch_up_unsafe(
        UWorld* expected_world) {
        if (world_map_candidate_available_) {
            UObject* retained = world_map_layer_candidate_.Get();
            if (retained && world_map_layer_class_
                && retained->IsA(world_map_layer_class_)
                && (!expected_world
                    || object_world_guarded(retained) == expected_world)) {
                if (world_map_candidate_has_open_evidence()) {
                    bool native_layer_visible{};
                    if (read_world_map_layer_visibility_guarded(
                            retained, &native_layer_visible)) {
                        if (native_layer_visible) {
                            world_map_visible_serial_ =
                                world_map_candidate_serial_;
                            latch_world_map_compact_suppression(
                                "world_map_activation_catch_up_open_evidence");
                        } else if (world_map_visible_serial_
                                == world_map_candidate_serial_
                            || (world_map_open_evidence_at_
                                    != Clock::time_point{}
                                && Clock::now()
                                        - world_map_open_evidence_at_
                                    >= kWorldMapOpenVisibilityGrace)) {
                            world_map_compact_suppressed_ = false;
                            clear_world_map_open_evidence();
                        }
                    }
                }
                if (expected_world) {
                    refresh_compact_menu_state(
                        expected_world,
                        "world_map_activation_catch_up_retained");
                }
                world_map_session_pending_ =
                    world_map_content_visibility_intent()
                    && world_map_candidate_has_open_evidence()
                    && world_map_compact_suppressed_;
                append_log("WORLD_MAP_LAYER_CAPTURE", std::format(
                    "source=retained activation={} epoch={} result=ready serial={} open_evidence={} session_pending={}",
                    activation_, epoch_, world_map_candidate_serial_,
                    world_map_candidate_has_open_evidence(),
                    world_map_session_pending_));
                return;
            }
            world_map_layer_candidate_ = FWeakObjectPtr{};
            world_map_candidate_available_ = false;
            clear_world_map_open_evidence();
        }

        const auto started = Clock::now();
        UObject* current_layer = UObjectGlobals::FindFirstOf(L"DLayerMap");
        const auto elapsed_us = std::chrono::duration_cast<
            std::chrono::microseconds>(Clock::now() - started).count();
        if (!current_layer || !world_map_layer_class_
            || !current_layer->IsA(world_map_layer_class_)
            || (expected_world
                && object_world_guarded(current_layer)
                    != expected_world)) {
            append_log("WORLD_MAP_LAYER_CAPTURE", std::format(
                "source=activation_catchup activation={} epoch={} result=not_found elapsed_us={}",
                activation_, epoch_, elapsed_us));
            return;
        }
        capture_world_map_candidate_unsafe(current_layer, false);
        if (expected_world) {
            refresh_compact_menu_state(
                expected_world,
                "world_map_activation_catch_up_captured");
        }
        append_log("WORLD_MAP_LAYER_CAPTURE", std::format(
            "source=activation_catchup activation={} epoch={} result=captured serial={} elapsed_us={}",
            activation_, epoch_, world_map_candidate_serial_, elapsed_us));
    }

    void log_world_map_activation_catch_up_fault() {
        append_log("WORLD_MAP_LAYER_CAPTURE", std::format(
            "source=activation_catchup activation={} epoch={} result=fault",
            activation_, epoch_));
    }

    [[nodiscard]] UObject* current_world_map_layer_guarded() noexcept {
#if defined(_MSC_VER)
        __try {
            return world_map_layer_candidate_.Get();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
#else
        return world_map_layer_candidate_.Get();
#endif
    }

    [[nodiscard]] UObject* current_compact_layer_guarded() noexcept {
#if defined(_MSC_VER)
        __try {
            return compact_layer_candidate_.Get();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
#else
        try {
            return compact_layer_candidate_.Get();
        } catch (...) {
            return nullptr;
        }
#endif
    }

    [[nodiscard]] bool resume_suspended_world_map_after_f7() noexcept {
        if (activity_suppressed_
            || !world_map_content_visibility_intent()
            || !world_map_candidate_has_open_evidence()
            || !world_map_compact_suppressed_
            || world_map_umg_renderer_.state()
                != dsnwr::WorldMapUmgRendererState::Suspended
            || !world_map_candidate_available_
            || world_map_umg_renderer_.active_marker_count() == 0) {
            return false;
        }

        UObject* current_layer = current_world_map_layer_guarded();
        const bool resumed = current_layer
            && world_map_umg_renderer_.resume_suspended(current_layer);
        if (resumed) {
            world_map_serviced_serial_ = world_map_candidate_serial_;
            world_map_readiness_attempts_ = 0;
            world_map_service_attempts_ = 0;
            world_map_renderer_session_started_ = true;
            world_map_session_pending_ = false;
            world_map_marker_count_ =
                world_map_umg_renderer_.active_marker_count();
            world_map_refresh_status_code_ = 0;
            apply_world_map_atlas_visibility_guarded(
                object_world_guarded(current_layer), current_layer);
            arm_world_map_layering_refresh(
                Clock::now(), WorldMapLayeringTrigger::F7Resume,
                WorldMapLayeringArmPolicy::Restart);
        }
        append_log("WORLD_MAP_ATLAS_RESUME", std::format(
            "activation={} epoch={} candidate_serial={} result={} markers={} state={} failure={} suspends={} resumes={} detaches={} atlas_build_us={} atlas_file_bytes={} attach_total_us={}",
            activation_, epoch_, world_map_candidate_serial_,
            resumed ? "success" : "rejected",
            world_map_umg_renderer_.active_marker_count(),
            static_cast<std::uint32_t>(world_map_umg_renderer_.state()),
            world_map_umg_renderer_.last_attach_failure(),
            world_map_umg_renderer_.suspend_count(),
            world_map_umg_renderer_.resume_count(),
            world_map_umg_renderer_.detach_count(),
            world_map_umg_renderer_.atlas_build_elapsed_us(),
            world_map_umg_renderer_.atlas_file_bytes(),
            world_map_umg_renderer_.attach_elapsed_us()));
        return resumed;
    }
    void reset_world_map_runtime(bool preserve_candidate) noexcept {
        world_map_umg_renderer_.publish_runtime_visibility(false);
        world_map_layering_refresh_pending_ = false;
        world_map_layering_refresh_started_ = {};
        world_map_layering_refresh_due_ = {};
        world_map_layering_refresh_serial_ = 0;
        world_map_layering_refresh_attempt_ = 0;
        world_map_layering_refresh_trigger_ =
            WorldMapLayeringTrigger::Attach;
        world_map_umg_markers_.fill({});
        world_map_marker_count_ = 0;
        world_map_marker_snapshot_built_ = false;
        world_map_first_marker_id_ = 0;
        world_map_last_marker_id_ = 0;
        world_map_treasure_marker_count_ = 0;
        world_map_boss_marker_count_ = 0;
        world_map_assault_marker_count_ = 0;
        world_map_fly_marker_count_ = 0;
        world_map_mole_marker_count_ = 0;
        world_map_wave_marker_count_ = 0;
        world_map_area_quest_marker_count_ = 0;
        world_map_refresh_status_code_ = 0xFFFFFFFFU;
        if (!preserve_candidate) {
            world_map_layer_candidate_ = FWeakObjectPtr{};
            world_map_candidate_available_ = false;
            world_map_set_image_rearm_consumed_ = false;
            world_map_open_serial_ = 0;
            world_map_visible_serial_ = 0;
            world_map_open_evidence_at_ = {};
            world_map_candidate_serial_ =
                dswros::next_world_map_candidate_serial(
                    world_map_candidate_serial_);
        }
        world_map_serviced_serial_ = 0;
        world_map_readiness_attempts_ = 0;
        world_map_service_attempts_ = 0;
        world_map_service_retry_after_ = {};
        world_map_renderer_session_started_ = false;
        world_map_layer_catch_up_attempted_ = false;
        world_map_session_pending_ =
            preserve_candidate && world_map_candidate_available_
            && world_map_candidate_has_open_evidence()
            && world_map_compact_suppressed_
            && world_map_content_visibility_intent();
    }

    [[nodiscard]] bool rearm_world_map_from_f7() noexcept {
        const bool readiness_budget_exhausted =
            world_map_readiness_attempts_
                >= kWorldMapMaxReadinessAttempts;
        const bool attach_budget_exhausted =
            world_map_service_attempts_ >= kWorldMapMaxServiceAttempts
            && world_map_umg_renderer_.state()
                == dsnwr::WorldMapUmgRendererState::Ready
            && world_map_umg_renderer_.retryable_not_ready();
        const bool bounded_retry_ready =
            enabled_ && !transition_active_
            && world_map_content_visibility_intent()
            && world_map_candidate_available_
            && world_map_candidate_has_open_evidence()
            && world_map_compact_suppressed_
            && !world_map_session_pending_
            && world_map_candidate_serial_ != 0
            && world_map_serviced_serial_ == world_map_candidate_serial_
            && (readiness_budget_exhausted || attach_budget_exhausted);
        if (!bounded_retry_ready) {
            return false;
        }

        const auto previous_readiness_attempts =
            world_map_readiness_attempts_;
        const auto previous_attempts = world_map_service_attempts_;
        const auto previous_failure =
            world_map_umg_renderer_.last_attach_failure();
        const auto previous_map_data_source =
            world_map_umg_renderer_.last_map_data_source();
        const auto previous_map_data_lookups =
            world_map_umg_renderer_.map_data_lookup_count();
        const auto previous_cache_hits =
            world_map_umg_renderer_.map_data_cache_hit_count();
        const auto candidate_serial = world_map_candidate_serial_;
        world_map_umg_renderer_.begin_activation();
        reset_world_map_runtime(true);
        append_log("WORLD_MAP_ATLAS_F7_REARM", std::format(
            "activation={} epoch={} candidate_serial={} previous_readiness_attempts={} previous_attempts={} previous_failure={} previous_map_data_source={} map_data_lookups={} map_data_cache_hits={} session_pending={} save_sync=unchanged reason=explicit_f7_bounded_retry",
            activation_, epoch_, candidate_serial,
            previous_readiness_attempts, previous_attempts,
            previous_failure, previous_map_data_source,
            previous_map_data_lookups, previous_cache_hits,
            world_map_session_pending_));
        return true;
    }

    [[nodiscard]] bool collect_world_map_marker_snapshot() noexcept {
        world_map_umg_markers_.fill({});
        world_map_marker_count_ = 0;
        world_map_first_marker_id_ = 0;
        world_map_last_marker_id_ = 0;
        world_map_treasure_marker_count_ = 0;
        world_map_boss_marker_count_ = 0;
        world_map_assault_marker_count_ = 0;
        world_map_fly_marker_count_ = 0;
        world_map_mole_marker_count_ = 0;
        world_map_wave_marker_count_ = 0;
        world_map_area_quest_marker_count_ = 0;

        const auto append_marker =
            [this](const dsnwr::WorldMapUmgMarker& marker) noexcept {
                if (world_map_marker_count_
                    >= world_map_umg_markers_.size()) {
                    return false;
                }
                world_map_umg_markers_[world_map_marker_count_] = marker;
                if (world_map_marker_count_ == 0) {
                    world_map_first_marker_id_ = marker.id;
                }
                world_map_last_marker_id_ = marker.id;
                ++world_map_marker_count_;
                return true;
            };

        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            const auto& entry = render_catalog_entries_[index];
            if (!world_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::Treasure)
                || compact_eligibility_[index] == 0
                || entry.map_id
                    != dswros::CompactRenderModel::kCompactMapId) {
                continue;
            }
            if (!append_marker({
                    entry.id,
                    entry.position.x,
                    entry.position.y,
                    world_map_treasure_tone(entry.kind),
                    dsnwr::WorldMapUmgMarkerKind::Treasure,
                    true,
                })) {
                world_map_refresh_status_code_ = static_cast<std::uint32_t>(
                    dswros::CompactRefreshStatus::InvalidOutputCapacity);
                world_map_marker_snapshot_built_ = true;
                return false;
            }
            ++world_map_treasure_marker_count_;
        }

        const std::int64_t now_unix_seconds = unix_seconds();
        for (const EncounterSpec& spec : encounter_catalog_) {
            if (spec.map_id != dswros::CompactRenderModel::kCompactMapId
                || !encounter_visible_for_selected_mode(
                    spec, now_unix_seconds)
                || !world_visibility_enabled(
                    spec.kind == EncounterKind::Boss
                        ? dsnwr::RadarVisibilityCategory::Boss
                        : dsnwr::RadarVisibilityCategory::Assault)) {
                continue;
            }
            const auto kind = spec.kind == EncounterKind::Boss
                ? dsnwr::WorldMapUmgMarkerKind::Boss
                : dsnwr::WorldMapUmgMarkerKind::Assault;
            if (!append_marker({
                    spec.id,
                    spec.position.x,
                    spec.position.y,
                    dsnwr::WorldMapUmgMarkerTone::White,
                    kind,
                    true,
                })) {
                world_map_refresh_status_code_ = static_cast<std::uint32_t>(
                    dswros::CompactRefreshStatus::InvalidOutputCapacity);
                world_map_marker_snapshot_built_ = true;
                return false;
            }
            if (spec.kind == EncounterKind::Boss) {
                ++world_map_boss_marker_count_;
            } else {
                ++world_map_assault_marker_count_;
            }
        }

        for (std::size_t index = 0;
             index < mini_game_catalog_.size(); ++index) {
            const MiniGameSpec& spec = mini_game_catalog_[index];
            if (!world_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::MiniGames)
                || mini_game_eligibility_[index] == 0
                || spec.map_id
                    != dswros::CompactRenderModel::kCompactMapId) {
                continue;
            }
            if (!append_marker({
                    spec.id,
                    spec.position.x,
                    spec.position.y,
                    dsnwr::WorldMapUmgMarkerTone::White,
                    world_map_mini_game_kind(spec.kind),
                    true,
                })) {
                world_map_refresh_status_code_ = static_cast<std::uint32_t>(
                    dswros::CompactRefreshStatus::InvalidOutputCapacity);
                world_map_marker_snapshot_built_ = true;
                return false;
            }
            switch (spec.kind) {
            case MiniGameKind::Fly: ++world_map_fly_marker_count_; break;
            case MiniGameKind::Mole: ++world_map_mole_marker_count_; break;
            case MiniGameKind::Wave: ++world_map_wave_marker_count_; break;
            }
        }

        const bool player_is_interior = player_.x < -100000.0;
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const AreaQuestSpec& spec = area_quest_catalog_[index];
            if (!world_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::AreaQuests)
                || !area_quest_visible_for_selected_mode(index)
                || (spec.position.x < -100000.0)
                    != player_is_interior) {
                continue;
            }
            if (!append_marker({
                    spec.id,
                    spec.position.x,
                    spec.position.y,
                    dsnwr::WorldMapUmgMarkerTone::Orange,
                    dsnwr::WorldMapUmgMarkerKind::AreaQuest,
                    true,
                })) {
                world_map_refresh_status_code_ = static_cast<std::uint32_t>(
                    dswros::CompactRefreshStatus::InvalidOutputCapacity);
                world_map_marker_snapshot_built_ = true;
                return false;
            }
            ++world_map_area_quest_marker_count_;
        }

        world_map_refresh_status_code_ = static_cast<std::uint32_t>(
            dswros::CompactRefreshStatus::Ok);
        world_map_marker_snapshot_built_ = true;
        return true;
    }

    void service_world_map_atlas(UEngine* engine) {
        if (!enabled_ || transition_active_ || activity_suppressed_
            || !world_map_session_pending_) {
            return;
        }
        if (!world_map_content_visibility_intent()) {
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            return;
        }
        if (!world_map_candidate_has_open_evidence()) {
            // Activation catch-up can retain a DLayerMap while gameplay is
            // active. A stale request without candidate-serial-bound
            // SetWorldMapImage evidence belongs to no open session and must be
            // cancelled without consuming readiness or attach budget.
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            return;
        }
        if (!world_map_compact_suppressed_) {
            // SetWorldMapImage may precede the first native visible sample.
            // Preserve the exact session request during that bounded opening
            // transition; visibility observation will either latch it open or
            // retire it after the grace window.
            return;
        }
        if (!world_map_candidate_confirmed_visible()) {
            // SetWorldMapImage is an opening edge, not proof that the native
            // map widget has finished becoming visible. Do not inspect map ID,
            // collect markers, start a renderer session, or consume either
            // finite budget until the first authoritative true sample arrives.
            return;
        }
        const auto now = Clock::now();
        if (now < world_map_service_retry_after_) {
            return;
        }
        if (save_reconcile_completed_ && !treasure_eligibility_ready_) {
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_ATTACH_FAILED", std::format(
                "activation={} epoch={} reason=save_eligibility_unavailable retry=next_f8_f7",
                activation_, epoch_));
            return;
        }

        if (!catalog_ready_ || !treasure_eligibility_ready_
            || !encounter_state_ready_
            || !area_quest_state_ready_
            || !position_valid_) {
            return;
        }
        if (world_map_serviced_serial_ != world_map_candidate_serial_) {
            world_map_serviced_serial_ = world_map_candidate_serial_;
            world_map_readiness_attempts_ = 0;
            world_map_service_attempts_ = 0;
            world_map_service_retry_after_ = {};
            world_map_renderer_session_started_ = false;
            world_map_marker_snapshot_built_ = false;
            world_map_marker_count_ = 0;
        }

        UObject* current_layer = world_map_layer_candidate_.Get();
        if (!current_layer) {
            world_map_candidate_available_ = false;
            clear_world_map_open_evidence();
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_ATTACH_FAILED", std::format(
                "activation={} epoch={} reason=layer_expired retry=next_map_session",
                activation_, epoch_));
            return;
        }

        if (world_map_readiness_attempts_
            >= kWorldMapMaxReadinessAttempts) {
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_READINESS_FAILED", std::format(
                "activation={} epoch={} reason=bounded_readiness_limit readiness_attempts={} attach_attempts={} retry=next_map_session_or_f7",
                activation_, epoch_, world_map_readiness_attempts_,
                world_map_service_attempts_));
            return;
        }

        if (world_map_service_attempts_
            >= kWorldMapMaxServiceAttempts) {
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_ATTACH_FAILED", std::format(
                "activation={} epoch={} reason=bounded_attempt_limit attempts={} retry=next_map_session",
                activation_, epoch_, world_map_service_attempts_));
            return;
        }

        std::int32_t map_id{};
        if (!world_map_umg_renderer_.detect_current_map_id(
                current_layer, map_id)) {
            ++world_map_readiness_attempts_;
            const bool retry = world_map_readiness_attempts_
                < kWorldMapMaxReadinessAttempts;
            world_map_session_pending_ = retry;
            world_map_service_retry_after_ = retry
                ? now + kWorldMapServiceRetryDelay : Clock::time_point{};
            append_log(
                retry ? "WORLD_MAP_ATLAS_ATTACH_DEFERRED"
                      : "WORLD_MAP_ATLAS_READINESS_FAILED",
                std::format(
                    "activation={} epoch={} reason=map_id_unavailable readiness_attempt={}/{} attach_attempt={}/{} retry={}",
                    activation_, epoch_, world_map_readiness_attempts_,
                    kWorldMapMaxReadinessAttempts,
                    world_map_service_attempts_,
                    kWorldMapMaxServiceAttempts,
                    retry ? "bounded_current_session"
                          : "next_map_session_or_f7"));
            return;
        }

        if (world_map_umg_renderer_.attached_to(current_layer, map_id)) {
            const auto transform_result =
                world_map_umg_renderer_.sync_viewport_transform(
                    current_layer);
            world_map_session_pending_ = false;

            if (transform_result
                == dsnwr::WorldMapLayeringRefreshResult::Faulted) {
                append_log("WORLD_MAP_VIEWPORT_TRANSFORM_FAILED",
                    std::format(
                        "activation={} epoch={} map_id={} result={} reason=same_live_layer_transform_sync retry=none transform_stage={}",
                        activation_, epoch_, map_id,
                        world_map_layering_result_name(transform_result),
                        static_cast<std::uint32_t>(
                            world_map_umg_renderer_.last_transform_sync_stage())));
                return;
            }
            if (transform_result
                == dsnwr::WorldMapLayeringRefreshResult::RetryLater) {
                append_log("WORLD_MAP_VIEWPORT_TRANSFORM_DEFERRED",
                    std::format(
                        "activation={} epoch={} map_id={} result={} reason=same_live_layer_transform_sync retry=bounded_settle_tail transform_stage={}",
                        activation_, epoch_, map_id,
                        world_map_layering_result_name(transform_result),
                        static_cast<std::uint32_t>(
                            world_map_umg_renderer_.last_transform_sync_stage())));
                arm_world_map_layering_refresh(
                    now, WorldMapLayeringTrigger::SetWorldMapImage,
                    WorldMapLayeringArmPolicy::Coalesce);
                return;
            }

            apply_world_map_atlas_visibility_guarded(
                object_world_guarded(current_layer), current_layer);

            world_map_marker_count_ =
                world_map_umg_renderer_.active_marker_count();
            append_log("WORLD_MAP_ATLAS_REUSED", std::format(
                "activation={} epoch={} map_id={} markers={} first_id={} last_id={} attempt={} reason={} transform={} ownership=independent_viewport_hosts native_canvas=read_only suspends={} resumes={} atlas_build_us={} atlas_file_bytes={} attach_total_us={}",
                activation_, epoch_, map_id, world_map_marker_count_,
                world_map_first_marker_id_, world_map_last_marker_id_,
                world_map_service_attempts_,
                transform_result
                        == dsnwr::WorldMapLayeringRefreshResult::Updated
                    ? "viewport_transform_updated"
                    : (transform_result
                            == dsnwr::WorldMapLayeringRefreshResult::Retained
                        ? "retained_last_verified_transform"
                        : "retained_transform_unchanged"),
                world_map_layering_result_name(transform_result),
                world_map_umg_renderer_.suspend_count(),
                world_map_umg_renderer_.resume_count(),
                world_map_umg_renderer_.atlas_build_elapsed_us(),
                world_map_umg_renderer_.atlas_file_bytes(),
                world_map_umg_renderer_.attach_elapsed_us()));
            // The native map can settle over several frames after opening or
            // zooming. Keep the finite transform-only tail for both changed
            // and unchanged samples; it never mutates the native Canvas tree.
            arm_world_map_layering_refresh(
                now, WorldMapLayeringTrigger::SetWorldMapImage,
                WorldMapLayeringArmPolicy::Coalesce);
            return;
        }

        if (map_id != dswros::CompactRenderModel::kCompactMapId) {
            if (!world_map_renderer_session_started_) {
                world_map_umg_renderer_.begin_map_session();
                world_map_renderer_session_started_ = true;
            }
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_SKIPPED", std::format(
                "activation={} epoch={} map_id={} reason=atlas_catalog_is_map100",
                activation_, epoch_, map_id));
            return;
        }

        if (!world_map_marker_snapshot_built_
            && !collect_world_map_marker_snapshot()) {
            world_map_session_pending_ = false;
            append_log("WORLD_MAP_ATLAS_ATTACH_FAILED", std::format(
                "activation={} epoch={} map_id={} reason=selection_capacity_exceeded refresh_status={} capacity={} retry=next_map_session",
                activation_, epoch_, map_id,
                world_map_refresh_status_code_,
                world_map_umg_markers_.size()));
            return;
        }
        if (world_map_marker_count_ == 0) {
            world_map_session_pending_ = false;
            world_map_service_retry_after_ = {};
            append_log("WORLD_MAP_ATLAS_SKIPPED", std::format(
                "activation={} epoch={} map_id={} reason=no_selected_or_eligible_world_markers refresh_status={} retry=content_or_runtime_delta",
                activation_, epoch_, map_id,
                world_map_refresh_status_code_));
            return;
        }

        UObject* expected_owning_player = current_player_controller(engine);
        if (!expected_owning_player) {
            ++world_map_readiness_attempts_;
            const bool retry = world_map_readiness_attempts_
                < kWorldMapMaxReadinessAttempts;
            world_map_session_pending_ = retry;
            world_map_service_retry_after_ = retry
                ? now + kWorldMapServiceRetryDelay : Clock::time_point{};
            append_log(
                retry ? "WORLD_MAP_ATLAS_ATTACH_DEFERRED"
                      : "WORLD_MAP_ATLAS_READINESS_FAILED",
                std::format(
                    "activation={} epoch={} map_id={} reason=current_player_controller_unavailable readiness_attempt={}/{} attach_attempt={}/{} retry={}",
                    activation_, epoch_, map_id,
                    world_map_readiness_attempts_,
                    kWorldMapMaxReadinessAttempts,
                    world_map_service_attempts_,
                    kWorldMapMaxServiceAttempts,
                    retry ? "bounded_current_session"
                          : "next_map_session_or_f7"));
            return;
        }

        if (!world_map_renderer_session_started_) {
            world_map_umg_renderer_.begin_map_session();
            world_map_renderer_session_started_ = true;
        }
        ++world_map_service_attempts_;
        const bool attached = world_map_umg_renderer_.attach_once(
            current_layer, expected_owning_player, world_map_umg_markers_,
            world_map_marker_count_, player_.x, player_.y);
        const bool retry = !attached
            && world_map_umg_renderer_.retryable_not_ready()
            && world_map_service_attempts_ < kWorldMapMaxServiceAttempts;
        world_map_session_pending_ = retry;
        world_map_service_retry_after_ = retry
            ? now + kWorldMapServiceRetryDelay : Clock::time_point{};
        if (attached) {
            // The atlas hosts are independent viewport widgets. The finite
            // settle tail only reads the native Canvas geometry and mirrors
            // its viewport transform; no child insertion or rebuild occurs.
            apply_world_map_atlas_visibility_guarded(
                object_world_guarded(current_layer), current_layer);
            arm_world_map_layering_refresh(
                now, WorldMapLayeringTrigger::Attach,
                WorldMapLayeringArmPolicy::Restart);
        }
        append_log(
            attached ? "WORLD_MAP_ATLAS_ATTACHED"
                     : (retry ? "WORLD_MAP_ATLAS_ATTACH_DEFERRED"
                              : "WORLD_MAP_ATLAS_ATTACH_FAILED"),
            std::format(
                "activation={} epoch={} map_id={} markers={} treasures={} bosses={} assaults={} fly={} mole={} wave={} area_quests={} first_id={} last_id={} attempt={}/{} retry={} state={} failure={} abi_failures={} map_data_cache_hits={} map_data_source={} dimensions={:.3f} ui_size={:.3f} overlay_left={:.3f} overlay_top={:.3f} zoom={:.6f} player_anchor_source={} player_anchor_x={:.3f} player_anchor_y={:.3f} native_parent_width={:.3f} native_parent_height={:.3f} geometry_stability={} geometry_sample_max_delta={:.3f} paint_owner={} atlas_build_us={} atlas_file_bytes={} attach_total_us={} suspends={} resumes={} detaches={}",
                activation_, epoch_, map_id, world_map_marker_count_,
                world_map_treasure_marker_count_,
                world_map_boss_marker_count_,
                world_map_assault_marker_count_,
                world_map_fly_marker_count_,
                world_map_mole_marker_count_,
                world_map_wave_marker_count_,
                world_map_area_quest_marker_count_,
                world_map_first_marker_id_, world_map_last_marker_id_,
                world_map_service_attempts_, kWorldMapMaxServiceAttempts,
                retry ? "bounded_current_session" : "none",
                static_cast<std::uint32_t>(
                    world_map_umg_renderer_.state()),
                world_map_umg_renderer_.last_attach_failure(),
                world_map_umg_renderer_.abi_failure_mask(),
                world_map_umg_renderer_.map_data_cache_hit_count(),
                world_map_umg_renderer_.last_map_data_source(),
                world_map_umg_renderer_.map_dimensions(),
                world_map_umg_renderer_.map_ui_size(),
                world_map_umg_renderer_.map_overlay_left(),
                world_map_umg_renderer_.map_overlay_top(),
                world_map_umg_renderer_.map_overlay_zoom(),
                world_map_umg_renderer_.player_anchor_source() == 1U
                    ? "slate_geometry"
                    : "none",
                world_map_umg_renderer_.player_canvas_anchor_x(),
                world_map_umg_renderer_.player_canvas_anchor_y(),
                world_map_umg_renderer_.native_parent_width(),
                world_map_umg_renderer_.native_parent_height(),
                dswros::world_map_geometry_stability_name(
                    world_map_umg_renderer_.geometry_stability_result()),
                world_map_umg_renderer_.geometry_sample_max_delta(),
                static_cast<std::uint32_t>(
                    world_map_umg_renderer_.paint_owner_status()),
                world_map_umg_renderer_.atlas_build_elapsed_us(),
                world_map_umg_renderer_.atlas_file_bytes(),
                world_map_umg_renderer_.attach_elapsed_us(),
                world_map_umg_renderer_.suspend_count(),
                world_map_umg_renderer_.resume_count(),
                world_map_umg_renderer_.detach_count()));
    }
    [[nodiscard]] bool compact_render_suppressed() const noexcept {
        return dswros::compact_render_suppressed({
            dsnwr::compact_radar_visibility_mask(visibility_masks_) != 0,
            position_valid_,
            mouse_cursor_visible_,
            world_map_compact_suppressed_,
            game_paused_,
            activity_suppressed_,
        });
    }

    void apply_compact_suppression() noexcept {
        if (!enabled_ || transition_active_) return;
        const bool suppressed = compact_render_suppressed();
        compact_umg_renderer_.set_menu_suppressed(suppressed);
    }

    void reset_compact_pool_runtime() noexcept {
        compact_attach_requested_ = false;
        compact_attach_attempt_count_ = 0;
        compact_attach_retry_after_ = {};
        compact_attach_gate_status_ =
            CompactAttachGateStatus::NotAttempted;
        compact_anchor_valid_ = false;
        compact_rebind_dirty_ = true;
        compact_marker_count_ = 0;
        compact_refresh_status_code_ = 0xFFFFFFFFU;
        reported_compact_nearest_id_ = -1;
        reported_compact_selection_count_ = static_cast<std::size_t>(-1);
        compact_rebind_after_ = {};
        compact_radius_sample_after_ = {};
        compact_render_radius_ = 0.0;
        compact_stable_nearest_id_ = 0;
        compact_height_target_z_ = 0.0;
        compact_height_target_valid_ = false;
        compact_mole_height_target_z_ = 0.0;
        compact_mole_height_target_valid_ = false;
        compact_radius_valid_ = false;
        compact_selected_markers_.fill({});
        compact_umg_markers_.fill({});
    }

    void report_area_quest_height_diagnostics(
        double comparable_player_z) noexcept {
        if (!dsnwr::native_event_log_enabled()) {
            return;
        }
        if (area_quest_height_diagnostic_activation_ != activation_) {
            area_quest_height_diagnostic_activation_ = activation_;
            area_quest_height_diagnostic_event_count_ = 0;
            area_quest_height_diagnostic_limit_reported_ = false;
            area_quest_height_diagnostic_states_.fill({});
        }

        for (std::size_t slot = 0;
             slot < area_quest_height_diagnostic_selections_.size(); ++slot) {
            const AreaQuestHeightDiagnosticSelection& selection =
                area_quest_height_diagnostic_selections_[slot];
            AreaQuestHeightDiagnosticState& previous =
                area_quest_height_diagnostic_states_[slot];
            if (!selection.active) {
                previous = {};
                continue;
            }

            const auto shape = selection.height_enabled
                ? dswros::area_quest_height_indicator_shape(
                    selection.height_profile, comparable_player_z)
                : dswros::AreaQuestHeightIndicatorShape::Unavailable;
            const bool selection_changed = !previous.active
                || previous.id != selection.id
                || previous.catalog_index != selection.catalog_index;
            const bool profile_changed = previous.active
                && (!area_quest_height_profiles_equal(
                        previous.height_profile,
                        selection.height_profile)
                    || previous.height_enabled
                        != selection.height_enabled);
            const bool shape_changed = previous.active
                && previous.shape != shape;
            if (!selection_changed && !profile_changed && !shape_changed) {
                continue;
            }

            previous = {
                selection.id,
                selection.catalog_index,
                selection.height_profile,
                shape,
                true,
                selection.height_enabled,
            };

            if (area_quest_height_diagnostic_event_count_
                    >= kAreaQuestHeightDiagnosticMaximumEventsPerActivation) {
                continue;
            }
            if (area_quest_height_diagnostic_event_count_
                    == kAreaQuestHeightDiagnosticMaximumEventsPerActivation
                        - 1U) {
                if (!area_quest_height_diagnostic_limit_reported_) {
                    area_quest_height_diagnostic_limit_reported_ = true;
                    ++area_quest_height_diagnostic_event_count_;
                    try {
                        append_log("AREA_QUEST_HEIGHT_DIAGNOSTIC_LIMIT",
                            std::format(
                                "activation={} epoch={} maximum_events={} tracked_slots={} action=suppress_until_next_activation",
                                activation_, epoch_,
                                kAreaQuestHeightDiagnosticMaximumEventsPerActivation,
                                kAreaQuestHeightDiagnosticSlotCapacity));
                    } catch (...) {
                    }
                }
                continue;
            }

            ++area_quest_height_diagnostic_event_count_;
            const char* reason = selection_changed
                ? "selection_changed"
                : (profile_changed ? "profile_changed" : "shape_changed");
            const auto& first = selection.height_profile.bands[0];
            const auto& second = selection.height_profile.bands[1];
            try {
                append_log("AREA_QUEST_HEIGHT_PROFILE", std::format(
                    "activation={} epoch={} slot={} id={} catalog_index={} band_count={} band1_min_z={:.3f} band1_max_z={:.3f} band2_min_z={:.3f} band2_max_z={:.3f} comparable_player_z={:.3f} height_enabled={} profile_valid={} shape={} reason={}",
                    activation_, epoch_, slot, selection.id,
                    selection.catalog_index,
                    static_cast<std::uint32_t>(
                        selection.height_profile.band_count),
                    first.minimum_z, first.maximum_z,
                    second.minimum_z, second.maximum_z,
                    comparable_player_z, selection.height_enabled,
                    dswros::area_quest_height_profile_valid(
                        selection.height_profile),
                    area_quest_height_shape_name(shape), reason));
            } catch (...) {
            }
        }
    }

    void rebuild_compact_snapshot() {
        const auto refresh = compact_render_model_.refresh(
            player_, true, compact_render_radius_,
            std::span<const std::uint8_t>{
                compact_eligibility_.data(), render_catalog_size_},
            std::span<dswros::CompactTreasureMarker>{
                compact_selected_markers_.data(),
                compact_selected_markers_.size()});
        compact_refresh_status_code_ =
            static_cast<std::uint32_t>(refresh.status);
        std::size_t treasure_count =
            refresh.ok()
                && compact_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::Treasure)
            ? refresh.count : 0;
        if (treasure_count > 0) {
            const auto choice = dswros::choose_compact_nearest(
                std::span<const dswros::CompactTreasureMarker>{
                    compact_selected_markers_.data(), treasure_count},
                compact_stable_nearest_id_,
                kCompactNearestSwitchAdvantage);
            if (choice.index != 0) {
                const bool promoted = dswros::promote_compact_nearest(
                    std::span<dswros::CompactTreasureMarker>{
                        compact_selected_markers_.data(), treasure_count},
                    choice.index);
                if (promoted && choice.retained_non_best) {
                    ++compact_nearest_hold_count_;
                }
            }
            const std::int64_t next_nearest_id =
                compact_selected_markers_[0].id;
            if (compact_stable_nearest_id_ > 0
                && compact_stable_nearest_id_ != next_nearest_id) {
                ++compact_nearest_switch_count_;
            }
            compact_stable_nearest_id_ = next_nearest_id;
        } else {
            compact_stable_nearest_id_ = 0;
        }
        const double radius_squared =
            compact_render_radius_ * compact_render_radius_;
        const std::int64_t now_unix_seconds = unix_seconds();

        std::array<StaticRenderCandidate, kExpectedEncounterCount>
            encounter_candidates{};
        std::size_t encounter_candidate_count{};
        std::size_t boss_candidate_count{};
        std::size_t assault_candidate_count{};
        for (std::size_t index = 0; index < encounter_catalog_.size(); ++index) {
            const EncounterSpec& spec = encounter_catalog_[index];
            if (spec.map_id != dswros::CompactRenderModel::kCompactMapId
                || !encounter_visible_for_selected_mode(
                    spec, now_unix_seconds)
                || !compact_visibility_enabled(
                    spec.kind == EncounterKind::Boss
                        ? dsnwr::RadarVisibilityCategory::Boss
                        : dsnwr::RadarVisibilityCategory::Assault)) {
                continue;
            }
            const double distance =
                planar_distance_squared(player_, spec.position);
            if (!std::isfinite(distance) || distance > radius_squared) {
                continue;
            }
            encounter_candidates[encounter_candidate_count++] = {
                index, distance};
            if (spec.kind == EncounterKind::Boss) {
                ++boss_candidate_count;
            } else {
                ++assault_candidate_count;
            }
        }
        std::sort(
            encounter_candidates.begin(),
            encounter_candidates.begin()
                + static_cast<std::ptrdiff_t>(encounter_candidate_count),
            [](const StaticRenderCandidate& left,
               const StaticRenderCandidate& right) {
                if (left.planar_distance_squared
                    != right.planar_distance_squared) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                }
                return left.catalog_index < right.catalog_index;
            });

        std::array<StaticRenderCandidate, kExpectedMiniGameCount>
            mini_game_candidates{};
        std::size_t mini_game_candidate_count{};
        std::size_t fly_candidate_count{};
        std::size_t mole_candidate_count{};
        std::size_t wave_candidate_count{};
        for (std::size_t index = 0; index < mini_game_catalog_.size(); ++index) {
            const MiniGameSpec& spec = mini_game_catalog_[index];
            if (!compact_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::MiniGames)
                || mini_game_eligibility_[index] == 0
                || spec.map_id != dswros::CompactRenderModel::kCompactMapId) {
                continue;
            }
            const double distance =
                planar_distance_squared(player_, spec.position);
            if (!std::isfinite(distance) || distance > radius_squared) {
                continue;
            }
            mini_game_candidates[mini_game_candidate_count++] = {
                index, distance};
            switch (spec.kind) {
            case MiniGameKind::Fly: ++fly_candidate_count; break;
            case MiniGameKind::Mole: ++mole_candidate_count; break;
            case MiniGameKind::Wave: ++wave_candidate_count; break;
            }
        }
        std::sort(
            mini_game_candidates.begin(),
            mini_game_candidates.begin()
                + static_cast<std::ptrdiff_t>(mini_game_candidate_count),
            [](const StaticRenderCandidate& left,
               const StaticRenderCandidate& right) {
                if (left.planar_distance_squared
                    != right.planar_distance_squared) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                }
                return left.catalog_index < right.catalog_index;
            });

        std::array<StaticRenderCandidate, kExpectedAreaQuestCount>
            area_quest_candidates{};
        std::size_t area_quest_candidate_count{};
        const bool player_is_interior = player_.x < -100000.0;
        if (area_quest_state_ready_
            && compact_visibility_enabled(
                dsnwr::RadarVisibilityCategory::AreaQuests)) {
            for (std::size_t index = 0;
                 index < area_quest_catalog_.size(); ++index) {
                const AreaQuestSpec& spec = area_quest_catalog_[index];
                if (!area_quest_visible_for_selected_mode(index)
                    || (spec.position.x < -100000.0)
                        != player_is_interior) {
                    continue;
                }
                const double distance =
                    planar_distance_squared(player_, spec.position);
                if (!std::isfinite(distance) || distance > radius_squared) {
                    continue;
                }
                area_quest_candidates[area_quest_candidate_count++] = {
                    index, distance};
            }
        }
        std::sort(
            area_quest_candidates.begin(),
            area_quest_candidates.begin()
                + static_cast<std::ptrdiff_t>(
                    area_quest_candidate_count),
            [](const StaticRenderCandidate& left,
               const StaticRenderCandidate& right) {
                if (left.planar_distance_squared
                    != right.planar_distance_squared) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                }
                return left.catalog_index < right.catalog_index;
            });

        std::array<StaticRenderCandidate, kBirdEggActiveCapacity>
            bird_egg_candidates{};
        std::size_t bird_egg_candidate_count{};
        if (compact_visibility_enabled(
                dsnwr::RadarVisibilityCategory::BirdEggs)) {
            for (std::size_t index = 0;
                 index < bird_egg_runtime_candidates_.size(); ++index) {
                const auto& candidate = bird_egg_runtime_candidates_[index];
                if (!candidate.presence.visible()
                    || !candidate.position_known) {
                    continue;
                }
                const double distance =
                    planar_distance_squared(player_, candidate.position);
                if (!std::isfinite(distance) || distance > radius_squared
                    || bird_egg_candidate_count
                        >= bird_egg_candidates.size()) {
                    continue;
                }
                bird_egg_candidates[bird_egg_candidate_count++] = {
                    index, distance};
            }
        }
        std::sort(
            bird_egg_candidates.begin(),
            bird_egg_candidates.begin()
                + static_cast<std::ptrdiff_t>(bird_egg_candidate_count),
            [](const StaticRenderCandidate& left,
               const StaticRenderCandidate& right) {
                if (left.planar_distance_squared
                    != right.planar_distance_squared) {
                    return left.planar_distance_squared
                        < right.planar_distance_squared;
                }
                return left.catalog_index < right.catalog_index;
            });

        compact_umg_markers_.fill({});
        constexpr std::size_t capacity = dsnwr::kCompactUmgMarkerCapacity;
        const std::size_t nearest_reserve = treasure_count > 0 ? 1U : 0U;
        const std::size_t encounter_keep = std::min(
            encounter_candidate_count, capacity - nearest_reserve);
        const std::size_t after_encounters = capacity - encounter_keep;
        const std::size_t area_quest_keep = std::min(
            area_quest_candidate_count,
            after_encounters - nearest_reserve);
        const std::size_t after_area_quests =
            after_encounters - area_quest_keep;
        const std::size_t bird_egg_keep = std::min(
            bird_egg_candidate_count,
            after_area_quests - nearest_reserve);
        const std::size_t after_bird_eggs =
            after_area_quests - bird_egg_keep;
        const std::size_t mini_game_keep = std::min(
            mini_game_candidate_count,
            after_bird_eggs - nearest_reserve);
        const std::size_t treasure_keep = std::min(
            treasure_count,
            capacity - encounter_keep - area_quest_keep
                - bird_egg_keep - mini_game_keep);
        std::size_t output_count{};

        const auto append_static = [this, &output_count](
            const dswros::Position& position,
            double reference_size,
            dsnwr::CompactUmgMarkerKind kind) {
            compact_umg_markers_[output_count++] = {
                std::clamp(
                    (position.x - player_.x) / compact_render_radius_,
                    -1.0, 1.0),
                std::clamp(
                    (position.y - player_.y) / compact_render_radius_,
                    -1.0, 1.0),
                reference_size,
                kind,
                false,
                0.0,
            };
        };

        for (std::size_t index = 0; index < encounter_keep; ++index) {
            const EncounterSpec& spec = encounter_catalog_[
                encounter_candidates[index].catalog_index];
            append_static(
                spec.position,
                spec.kind == EncounterKind::Boss ? 30.0 : 27.0,
                spec.kind == EncounterKind::Boss
                    ? dsnwr::CompactUmgMarkerKind::Boss
                    : dsnwr::CompactUmgMarkerKind::Assault);
        }
        const bool area_quest_height_enabled =
            dswros::height_indicator_enabled(
                height_indicator_mask_,
                dswros::HeightIndicatorCategory::AreaQuest);
        const bool capture_area_quest_height_diagnostics =
            dsnwr::native_event_log_enabled();
        if (capture_area_quest_height_diagnostics) {
            area_quest_height_diagnostic_selections_.fill({});
        }
        for (std::size_t index = 0;
             index < area_quest_keep; ++index) {
            const std::size_t catalog_index =
                area_quest_candidates[index].catalog_index;
            const AreaQuestSpec& spec = area_quest_catalog_[catalog_index];
            const dswros::AreaQuestHeightProfile height_profile =
                dswros::area_quest_height_profile_for_marker(
                    spec.height_profile, spec.position.z);
            const bool show_height = area_quest_height_enabled
                && dswros::area_quest_height_profile_valid(
                    height_profile)
                && std::isfinite(player_.z);
            const double comparable_player_z =
                dswros::CompactRenderModel::comparable_player_z(player_.z);
            if (capture_area_quest_height_diagnostics
                && index < kAreaQuestHeightDiagnosticSlotCapacity) {
                area_quest_height_diagnostic_selections_[index] = {
                    spec.id,
                    catalog_index,
                    height_profile,
                    true,
                    area_quest_height_enabled,
                };
            }
            compact_umg_markers_[output_count++] = {
                std::clamp(
                    (spec.position.x - player_.x) / compact_render_radius_,
                    -1.0, 1.0),
                std::clamp(
                    (spec.position.y - player_.y) / compact_render_radius_,
                    -1.0, 1.0),
                28.0,
                dsnwr::CompactUmgMarkerKind::AreaQuest,
                show_height,
                0.0,
                area_quest_height_enabled && !show_height,
                show_height
                    ? height_profile
                    : dswros::AreaQuestHeightProfile{},
                show_height ? comparable_player_z : 0.0,
            };
        }
        for (std::size_t index = 1; index < treasure_keep; ++index) {
            const auto& marker = compact_selected_markers_[index];
            compact_umg_markers_[output_count++] = {
                marker.normalized_x,
                marker.normalized_y,
                14.0,
                umg_treasure_kind(marker.kind),
                false,
                0.0,
            };
        }
        bool mini_game_height_selected{};
        compact_mole_height_target_valid_ = false;
        compact_mole_height_target_z_ = 0.0;
        for (std::size_t index = 0; index < mini_game_keep; ++index) {
            const MiniGameSpec& spec = mini_game_catalog_[
                mini_game_candidates[index].catalog_index];
            const bool nearest_mini_game = !mini_game_height_selected;
            const bool show_height = nearest_mini_game
                && dswros::height_indicator_enabled(
                    height_indicator_mask_,
                    dswros::HeightIndicatorCategory::Mole)
                && spec.height_trusted
                && std::isfinite(spec.height_z)
                && std::isfinite(player_.z);
            compact_umg_markers_[output_count++] = {
                std::clamp(
                    (spec.position.x - player_.x) / compact_render_radius_,
                    -1.0, 1.0),
                std::clamp(
                    (spec.position.y - player_.y) / compact_render_radius_,
                    -1.0, 1.0),
                28.0,
                umg_mini_game_kind(spec.kind),
                show_height,
                show_height
                    ? dswros::CompactRenderModel::
                        mini_game_height_angle_from_delta(
                        spec.height_z
                            - dswros::CompactRenderModel::comparable_player_z(
                                player_.z))
                    : 0.0,
            };
            if (nearest_mini_game) {
                mini_game_height_selected = true;
            }
            if (show_height) {
                compact_mole_height_target_valid_ = true;
                compact_mole_height_target_z_ = spec.height_z;
            }
        }
        for (std::size_t index = 0; index < bird_egg_keep; ++index) {
            const auto& candidate = bird_egg_runtime_candidates_[
                bird_egg_candidates[index].catalog_index];
            append_static(
                candidate.position, 18.0,
                dsnwr::CompactUmgMarkerKind::BirdEgg);
        }
        if (treasure_keep > 0) {
            const auto& nearest = compact_selected_markers_[0];
            const bool show_height = nearest.height_available
                && dswros::height_indicator_enabled(
                    height_indicator_mask_,
                    dswros::HeightIndicatorCategory::Treasure);
            compact_umg_markers_[output_count++] = {
                nearest.normalized_x,
                nearest.normalized_y,
                22.0,
                umg_treasure_kind(nearest.kind),
                show_height,
                show_height ? nearest.height_angle_degrees : 0.0,
            };
            compact_height_target_valid_ = show_height;
            compact_height_target_z_ = show_height
                ? nearest.height_target_z : 0.0;
        } else {
            compact_height_target_valid_ = false;
            compact_height_target_z_ = 0.0;
        }
        compact_marker_count_ = output_count;
        compact_umg_renderer_.rebind(
            compact_umg_markers_, compact_marker_count_);

        const std::int64_t nearest_id = treasure_count > 0
            ? compact_selected_markers_[0].id : 0;
        if (nearest_id != reported_compact_nearest_id_
            || compact_marker_count_ != reported_compact_selection_count_) {
            reported_compact_nearest_id_ = nearest_id;
            reported_compact_selection_count_ = compact_marker_count_;
            append_log("COMPACT_SELECTION", std::format(
                "activation={} epoch={} count={} treasure={} boss={} assault={} area_quest={} area_quest_height_source=actor_position_data_profile bird_egg={} fly={} mole={} wave={} nearest_id={} player_x={:.3f} player_y={:.3f} radius_cm={:.0f}",
                activation_, epoch_, compact_marker_count_, treasure_keep,
                boss_candidate_count, assault_candidate_count,
                area_quest_keep,
                bird_egg_keep,
                fly_candidate_count, mole_candidate_count,
                wave_candidate_count, nearest_id,
                player_.x, player_.y, compact_render_radius_));
        }
    }

    void update_compact_pool(
        UEngine* engine, Clock::time_point now) noexcept {
        if (!catalog_ready_ || !compact_render_model_.initialized()
            || !position_valid_ || compact_render_suppressed()) {
            return;
        }

        bool first_attach = false;
        if (!compact_attach_requested_
            && compact_attach_attempt_count_ < kCompactMaxAttachAttempts
            && now >= compact_attach_retry_after_) {
            compact_attach_requested_ = true;
            ++compact_attach_attempt_count_;
            UObject* expected_owning_player =
                current_player_controller(engine);
            if (!expected_owning_player) {
                compact_attach_gate_status_ =
                    CompactAttachGateStatus::CurrentPlayerControllerUnavailable;
                const bool retry = compact_attach_attempt_count_
                    < kCompactMaxAttachAttempts;
                compact_attach_requested_ = !retry;
                compact_attach_retry_after_ = retry
                    ? now + kCompactAttachRetryDelay : Clock::time_point{};
                try {
                    append_log("COMPACT_POOL_ATTACH_FAILED", std::format(
                        "activation={} epoch={} status={} reason=current_player_controller_unavailable attempt={}/{} retry={}",
                        activation_, epoch_,
                        static_cast<std::uint32_t>(
                            compact_attach_gate_status_),
                        compact_attach_attempt_count_,
                        kCompactMaxAttachAttempts,
                        retry ? "bounded_current_activation"
                              : "next_f7_or_travel"));
                } catch (...) {
                }
                return;
            }
            if (!compact_umg_renderer_.attach_once(
                    expected_owning_player,
                    compact_candidate_available_
                        ? compact_layer_candidate_
                        : FWeakObjectPtr{})) {
                compact_attach_gate_status_ =
                    CompactAttachGateStatus::RendererRejected;
                const std::uint32_t failure =
                    compact_umg_renderer_.last_attach_failure();
                const bool soft_not_ready = failure == 1U || failure == 2U
                    || failure == 3U || failure == 5U || failure == 7U
                    || failure == 16U || failure == 17U
                    || failure == 18U;
                const bool retry = soft_not_ready
                    && compact_attach_attempt_count_
                        < kCompactMaxAttachAttempts;
                if (retry) {
                    compact_umg_renderer_.begin_activation();
                    compact_attach_requested_ = false;
                    compact_attach_retry_after_ =
                        now + kCompactAttachRetryDelay;
                }
                try {
                    append_log("COMPACT_POOL_ATTACH_FAILED", std::format(
                        "activation={} epoch={} status={} reason=renderer_rejected last_attach_failure={} abi_failures={} attempt={}/{} retry={}",
                        activation_, epoch_,
                        static_cast<std::uint32_t>(
                            compact_attach_gate_status_),
                        failure,
                        compact_umg_renderer_.abi_failure_mask(),
                        compact_attach_attempt_count_,
                        kCompactMaxAttachAttempts,
                        retry ? "bounded_current_activation"
                              : "next_f7_or_travel"));
                } catch (...) {
                }
                return;
            }
            compact_attach_gate_status_ =
                CompactAttachGateStatus::Attached;
            first_attach = true;
            try {
                append_log("COMPACT_GEOMETRY", std::format(
                    "activation={} epoch={} viewport_width={:.3f} viewport_height={:.3f} viewport_dpi_scale={:.6f} display_scale={:.6f} umg_unit_scale={:.6f} reference_center_right=220 reference_center_y=217 reference_extent=170",
                    activation_, epoch_,
                    compact_umg_renderer_.viewport_width(),
                    compact_umg_renderer_.viewport_height(),
                    compact_umg_renderer_.viewport_dpi_scale(),
                    compact_umg_renderer_.display_scale(),
                    compact_umg_renderer_.umg_unit_scale()));
            } catch (...) {
            }
        }
        if (compact_umg_renderer_.state()
            != dsnwr::CompactUmgRendererState::Attached) {
            return;
        }
        const std::uint32_t world_time_seconds =
            current_world_time_seconds();
        compact_umg_renderer_.update_world_clock(
            world_time_available_
                && compact_visibility_enabled(
                    dsnwr::RadarVisibilityCategory::Clock),
            world_time_seconds);

        if (now >= compact_radius_sample_after_) {
            compact_radius_sample_after_ = now + kMinimapScaleSampleInterval;
            double minimap_scale{};
            if (!compact_umg_renderer_.read_minimap_scale(minimap_scale)) {
                if (compact_radius_valid_ || first_attach) {
                    compact_radius_valid_ = false;
                    compact_render_radius_ = 0.0;
                    compact_anchor_valid_ = false;
                    compact_marker_count_ = 0;
                    compact_stable_nearest_id_ = 0;
                    compact_height_target_valid_ = false;
                    compact_height_target_z_ = 0.0;
                    compact_mole_height_target_valid_ = false;
                    compact_mole_height_target_z_ = 0.0;
                    compact_selected_markers_.fill({});
                    compact_umg_markers_.fill({});
                    compact_umg_renderer_.rebind(
                        compact_umg_markers_, compact_marker_count_);
                    try {
                        append_log("MINIMAP_RADIUS_UNAVAILABLE", std::format(
                            "activation={} epoch={} retry_ms=1000 action=markers_collapsed",
                            activation_, epoch_));
                    } catch (...) {
                    }
                }
            } else {
                const double selected_radius = minimap_scale >= kMinimapScaleThreshold
                    ? kTownCompactRenderRadius
                    : kFieldCompactRenderRadius;
                if (!compact_radius_valid_
                    || selected_radius != compact_render_radius_) {
                    compact_radius_valid_ = true;
                    compact_render_radius_ = selected_radius;
                    compact_anchor_valid_ = false;
                    compact_rebind_dirty_ = true;
                    try {
                        append_log("MINIMAP_RADIUS", std::format(
                            "activation={} epoch={} scale={:.4f} mode={} radius_cm={:.0f} radius_m={:.0f}",
                            activation_, epoch_, minimap_scale,
                            selected_radius == kTownCompactRenderRadius
                                ? "town" : "field",
                            selected_radius, selected_radius / 100.0));
                    } catch (...) {
                    }
                }
            }
        }
        if (!compact_radius_valid_) {
            return;
        }

        const double anchor_delta_x = player_.x - compact_render_anchor_.x;
        const double anchor_delta_y = player_.y - compact_render_anchor_.y;
        const bool moved_for_rebind = compact_anchor_valid_
            && anchor_delta_x * anchor_delta_x
                + anchor_delta_y * anchor_delta_y
                >= kCompactRebindDistance * kCompactRebindDistance;
        if (first_attach || !compact_anchor_valid_ || compact_rebind_dirty_
            || moved_for_rebind
            || now >= compact_rebind_after_) {
            rebuild_compact_snapshot();
            compact_render_anchor_ = player_;
            compact_anchor_valid_ = true;
            compact_rebind_dirty_ = false;
            compact_rebind_after_ = now + kCompactRebindInterval;
        }

        if (compact_umg_renderer_.state()
            == dsnwr::CompactUmgRendererState::Attached) {
            compact_umg_renderer_.translate(
                -(player_.x - compact_render_anchor_.x)
                    / compact_render_radius_,
                -(player_.y - compact_render_anchor_.y)
                    / compact_render_radius_);
            if (compact_height_target_valid_) {
                const double height_delta = compact_height_target_z_
                    - dswros::CompactRenderModel::comparable_player_z(
                        player_.z);
                compact_umg_renderer_.update_height_pointer(
                    dsnwr::CompactUmgHeightChannel::Treasure,
                    dswros::CompactRenderModel::height_angle_from_delta(
                        height_delta));
            }
            if (dswros::height_indicator_enabled(
                    height_indicator_mask_,
                    dswros::HeightIndicatorCategory::AreaQuest)) {
                compact_umg_renderer_.update_area_quest_height_indicators(
                    dswros::CompactRenderModel::comparable_player_z(
                        player_.z));
            }
            report_area_quest_height_diagnostics(
                dswros::CompactRenderModel::comparable_player_z(player_.z));
            if (compact_mole_height_target_valid_) {
                const double height_delta = compact_mole_height_target_z_
                    - dswros::CompactRenderModel::comparable_player_z(
                        player_.z);
                compact_umg_renderer_.update_height_pointer(
                    dsnwr::CompactUmgHeightChannel::Mole,
                    dswros::CompactRenderModel::
                        mini_game_height_angle_from_delta(height_delta));
            }
        }
    }

    [[nodiscard]] static bool save_result_contains(
        const dsnwr::SaveReconcileResult& result,
        std::int64_t save_id) noexcept {
        if (save_id <= 0) {
            return false;
        }
        const std::int64_t category = save_id / 64;
        const std::uint32_t bit = static_cast<std::uint32_t>(save_id % 64);
        const auto found = std::lower_bound(
            result.opened_fields.begin(), result.opened_fields.end(), category,
            [](const dsnwr::OpenedTreasureField& field,
               std::int64_t requested_category) {
                return field.category < requested_category;
            });
        return found != result.opened_fields.end()
            && found->category == category
            && (found->bits & (std::uint64_t{1} << bit)) != 0;
    }

    [[nodiscard]] std::optional<std::size_t>
    treasure_render_catalog_index(std::int64_t save_id) const noexcept {
        if (save_id <= 0) {
            return std::nullopt;
        }
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            if (render_catalog_entries_[index].id == save_id) {
                return index;
            }
        }
        return std::nullopt;
    }

    void recompute_treasure_save_confirmation_due() noexcept {
        treasure_save_confirmation_pending_armed_ = false;
        treasure_save_confirmation_next_due_ = {};
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((treasure_save_confirmation_pending_[word] & bit) == 0) {
                continue;
            }
            if (!treasure_save_confirmation_pending_armed_
                || treasure_save_confirmation_due_[index]
                    < treasure_save_confirmation_next_due_) {
                treasure_save_confirmation_pending_armed_ = true;
                treasure_save_confirmation_next_due_ =
                    treasure_save_confirmation_due_[index];
            }
        }
    }

    [[nodiscard]] bool queue_treasure_save_confirmation(
        std::size_t index, const char* evidence) noexcept {
        if (index >= render_catalog_size_
            || index >= kMaximumTreasureCatalogEntries) {
            return false;
        }
        const std::int64_t save_id = render_catalog_entries_[index].id;
        if (save_id <= 0 || ignored_treasure_ids_.contains(save_id)
            || runtime_opened_treasure_ids_.contains(save_id)) {
            return false;
        }
        const std::size_t word = index / 64U;
        const std::uint64_t bit = 1ULL << (index % 64U);
        if ((treasure_save_confirmation_pending_[word] & bit) != 0
            || (treasure_save_confirmation_inflight_[word] & bit) != 0) {
            return false;
        }
        treasure_save_confirmation_pending_[word] |= bit;
        treasure_save_confirmation_attempts_[index] = 0;
        treasure_save_confirmation_due_[index] =
            Clock::now() + kTreasureSaveConfirmationInitialDelay;
        if (!treasure_save_confirmation_pending_armed_
            || treasure_save_confirmation_due_[index]
                < treasure_save_confirmation_next_due_) {
            treasure_save_confirmation_pending_armed_ = true;
            treasure_save_confirmation_next_due_ =
                treasure_save_confirmation_due_[index];
        }
        try {
            append_log("TREASURE_SAVE_CONFIRMATION_QUEUED", std::format(
                "activation={} epoch={} id={} catalog_index={} evidence={} first_delay_seconds=15 final_retry_after_seconds=285 maximum_attempts=2 query=exact_category_positive_only",
                activation_, epoch_, save_id, index,
                evidence ? evidence : "unknown"));
        } catch (...) {
        }
        return true;
    }

    [[nodiscard]] std::size_t collect_due_treasure_save_confirmations(
        Clock::time_point now,
        std::array<std::int64_t,
                   dsnwr::kMaximumTreasureConfirmationIds>& ids) noexcept {
        if (!treasure_save_confirmation_pending_armed_
            || now < treasure_save_confirmation_next_due_) {
            return 0;
        }
        treasure_save_confirmation_inflight_.fill(0);
        std::size_t count{};
        for (std::size_t index = 0;
             index < render_catalog_size_ && count < ids.size(); ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((treasure_save_confirmation_pending_[word] & bit) == 0
                || now < treasure_save_confirmation_due_[index]) {
                continue;
            }
            const std::int64_t save_id = render_catalog_entries_[index].id;
            treasure_save_confirmation_pending_[word] &= ~bit;
            if (save_id <= 0
                || runtime_opened_treasure_ids_.contains(save_id)) {
                treasure_save_confirmation_due_[index] = {};
                treasure_save_confirmation_attempts_[index] = 0;
                continue;
            }
            treasure_save_confirmation_inflight_[word] |= bit;
            ids[count++] = save_id;
            if (treasure_save_confirmation_attempts_[index]
                < std::numeric_limits<std::uint8_t>::max()) {
                ++treasure_save_confirmation_attempts_[index];
            }
        }
        recompute_treasure_save_confirmation_due();
        return count;
    }

    void restore_unsubmitted_treasure_save_confirmations(
        Clock::time_point now) noexcept {
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((treasure_save_confirmation_inflight_[word] & bit) == 0) {
                continue;
            }
            treasure_save_confirmation_pending_[word] |= bit;
            treasure_save_confirmation_inflight_[word] &= ~bit;
            treasure_save_confirmation_due_[index] = now + kDiscoveryInterval;
            if (treasure_save_confirmation_attempts_[index] > 0) {
                --treasure_save_confirmation_attempts_[index];
            }
        }
        recompute_treasure_save_confirmation_due();
    }

    void apply_treasure_save_confirmation_result(
        const dsnwr::SaveReconcileResult& result,
        bool query_succeeded) noexcept {
        const auto now = Clock::now();
        std::size_t requested{};
        std::size_t confirmed{};
        std::size_t already_resolved{};
        std::size_t retry_scheduled{};
        std::size_t exhausted{};
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((treasure_save_confirmation_inflight_[word] & bit) == 0) {
                continue;
            }
            ++requested;
            treasure_save_confirmation_inflight_[word] &= ~bit;
            const std::int64_t save_id = render_catalog_entries_[index].id;
            if (runtime_opened_treasure_ids_.contains(save_id)) {
                ++already_resolved;
                treasure_save_confirmation_due_[index] = {};
                treasure_save_confirmation_attempts_[index] = 0;
                continue;
            }
            const bool accepted =
                dswros::accept_treasure_save_confirmation(
                    true, query_succeeded,
                    save_result_contains(result, save_id));
            if (accepted) {
                static_cast<void>(mark_treasure_opened(save_id));
                ++confirmed;
                treasure_save_confirmation_due_[index] = {};
                treasure_save_confirmation_attempts_[index] = 0;
                continue;
            }
            if (dswros::retry_treasure_save_confirmation(
                    true, accepted,
                    treasure_save_confirmation_attempts_[index],
                    kTreasureSaveConfirmationMaximumAttempts)) {
                treasure_save_confirmation_pending_[word] |= bit;
                treasure_save_confirmation_due_[index] =
                    now + kTreasureSaveConfirmationRetryDelay;
                ++retry_scheduled;
                continue;
            }
            treasure_save_confirmation_due_[index] = {};
            treasure_save_confirmation_attempts_[index] = 0;
            ++exhausted;
        }
        recompute_treasure_save_confirmation_due();
        try {
            append_log("TREASURE_SAVE_CONFIRMATION_APPLIED", std::format(
                "activation={} request_id={} databases={} requested={} result_requested={} confirmed={} already_resolved={} retry_scheduled={} exhausted={} query_succeeded={} copy_us={} query_us={} total_us={} policy=positive_only_two_attempts_no_poll",
                activation_, result.request_id, result.database_count,
                requested, result.requested_treasure_count, confirmed,
                already_resolved, retry_scheduled, exhausted,
                query_succeeded, result.copy_elapsed_us,
                result.query_elapsed_us, result.total_elapsed_us));
        } catch (...) {
        }
    }

    void recompute_area_quest_save_confirmation_due() noexcept {
        area_quest_save_confirmation_pending_armed_ = false;
        area_quest_save_confirmation_next_due_ = {};
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((area_quest_save_confirmation_pending_[word] & bit) == 0) {
                continue;
            }
            if (!area_quest_save_confirmation_pending_armed_
                || area_quest_save_confirmation_due_[index]
                    < area_quest_save_confirmation_next_due_) {
                area_quest_save_confirmation_pending_armed_ = true;
                area_quest_save_confirmation_next_due_ =
                    area_quest_save_confirmation_due_[index];
            }
        }
    }

    [[nodiscard]] bool queue_area_quest_save_confirmation(
        std::size_t index, const char* evidence) noexcept {
        if (index >= area_quest_catalog_.size()) {
            return false;
        }
        if (area_quest_completion_generation_locked_[index]) {
            return false;
        }
        const std::int64_t baseline_count =
            area_quest_save_completion_counts_[index];
        if (baseline_count
            == dswros::kUnknownAreaQuestSaveCompletionCount) {
            area_quest_completion_generation_locked_[index] = true;
            area_quest_completion_generation_reactivation_armed_[index] =
                false;
            try {
                append_log("AREA_QUEST_SAVE_CONFIRMATION_SKIPPED",
                    std::format(
                        "activation={} epoch={} id={} catalog_index={} evidence={} reason=verified_complete_count_baseline_unknown action=same_cycle_generation_locked sql_requested=false",
                        activation_, epoch_, area_quest_catalog_[index].id,
                        index, evidence ? evidence : "unknown"));
            } catch (...) {
            }
            return false;
        }
        const std::size_t word = index / 64U;
        const std::uint64_t bit = 1ULL << (index % 64U);
        if ((area_quest_save_confirmation_pending_[word] & bit) != 0
            || (area_quest_save_confirmation_inflight_[word] & bit) != 0) {
            return false;
        }
        area_quest_save_confirmation_pending_[word] |= bit;
        area_quest_save_confirmation_attempts_[index] = 0;
        area_quest_save_confirmation_due_[index] = Clock::now();
        if (!area_quest_save_confirmation_pending_armed_
            || area_quest_save_confirmation_due_[index]
                < area_quest_save_confirmation_next_due_) {
            area_quest_save_confirmation_pending_armed_ = true;
            area_quest_save_confirmation_next_due_ =
                area_quest_save_confirmation_due_[index];
        }
        try {
            append_log("AREA_QUEST_SAVE_CONFIRMATION_QUEUED", std::format(
                "activation={} epoch={} id={} catalog_index={} evidence={} verified_baseline_count={} first_delay_seconds=0 retry_delay_seconds=15 maximum_attempts=3 schedule=event_driven_coalesced_positive_only_sql acceptance=strict_complete_count_growth",
                activation_, epoch_, area_quest_catalog_[index].id, index,
                evidence ? evidence : "unknown", baseline_count));
        } catch (...) {
        }
        return true;
    }

    [[nodiscard]] std::size_t collect_due_area_quest_save_confirmations(
        Clock::time_point now) noexcept {
        if (!area_quest_save_confirmation_pending_armed_
            || now < area_quest_save_confirmation_next_due_) {
            return 0;
        }
        area_quest_save_confirmation_inflight_.fill(0);
        std::size_t count{};
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((area_quest_save_confirmation_pending_[word] & bit) == 0
                || now < area_quest_save_confirmation_due_[index]) {
                continue;
            }
            area_quest_save_confirmation_pending_[word] &= ~bit;
            if (area_quest_completion_observed_[index]) {
                area_quest_save_confirmation_due_[index] = {};
                area_quest_save_confirmation_attempts_[index] = 0;
                continue;
            }
            if (area_quest_save_completion_counts_[index]
                == dswros::kUnknownAreaQuestSaveCompletionCount) {
                area_quest_save_confirmation_due_[index] = {};
                area_quest_save_confirmation_attempts_[index] = 0;
                area_quest_completion_generation_locked_[index] = true;
                area_quest_completion_generation_reactivation_armed_[index] =
                    false;
                continue;
            }
            area_quest_save_confirmation_inflight_[word] |= bit;
            if (area_quest_save_confirmation_attempts_[index]
                < std::numeric_limits<std::uint8_t>::max()) {
                ++area_quest_save_confirmation_attempts_[index];
            }
            ++count;
        }
        recompute_area_quest_save_confirmation_due();
        return count;
    }

    void restore_unsubmitted_area_quest_save_confirmations(
        Clock::time_point now) noexcept {
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((area_quest_save_confirmation_inflight_[word] & bit) == 0) {
                continue;
            }
            area_quest_save_confirmation_pending_[word] |= bit;
            area_quest_save_confirmation_inflight_[word] &= ~bit;
            area_quest_save_confirmation_due_[index] =
                now + kDiscoveryInterval;
            if (area_quest_save_confirmation_attempts_[index] > 0) {
                --area_quest_save_confirmation_attempts_[index];
            }
        }
        recompute_area_quest_save_confirmation_due();
    }

    void apply_completion_confirmation_result(
        const dsnwr::SaveReconcileResult& result,
        bool query_succeeded) noexcept {
        const auto now = Clock::now();
        std::size_t area_requested{};
        std::size_t area_confirmed{};
        std::size_t area_already_resolved{};
        std::size_t area_retry_scheduled{};
        std::size_t area_exhausted{};
        std::size_t area_generation_locked{};
        for (std::size_t index = 0;
             index < area_quest_catalog_.size(); ++index) {
            const std::size_t word = index / 64U;
            const std::uint64_t bit = 1ULL << (index % 64U);
            if ((area_quest_save_confirmation_inflight_[word] & bit) == 0) {
                continue;
            }
            ++area_requested;
            area_quest_save_confirmation_inflight_[word] &= ~bit;
            if (area_quest_completion_observed_[index]) {
                area_quest_save_confirmation_due_[index] = {};
                area_quest_save_confirmation_attempts_[index] = 0;
                ++area_already_resolved;
                continue;
            }
            const std::int64_t quest_id = area_quest_catalog_[index].id;
            const auto completion = std::lower_bound(
                result.dynamic_quest_completions.begin(),
                result.dynamic_quest_completions.end(), quest_id,
                [](const dsnwr::DynamicQuestCompletionField& field,
                   std::int64_t requested_id) {
                    return field.quest_id < requested_id;
                });
            const bool exact_id_matches =
                completion != result.dynamic_quest_completions.end()
                && completion->quest_id == quest_id;
            const std::int64_t complete_count = exact_id_matches
                ? completion->complete_count : 0;
            const std::int64_t verified_baseline_count =
                area_quest_save_completion_counts_[index];
            const bool accepted =
                dswros::accept_area_quest_save_confirmation(
                    true,
                    query_succeeded
                        && result.dynamic_quest_completion_query_available,
                    result.dynamic_quest_completion_identity_ambiguous,
                    exact_id_matches, verified_baseline_count,
                    complete_count);
            if (accepted) {
                area_quest_save_completion_counts_[index] = complete_count;
                area_quest_save_completion_[index] = 1;
                area_quest_save_confirmation_due_[index] = {};
                area_quest_save_confirmation_attempts_[index] = 0;
                apply_exact_area_quest_completion(index);
                ++area_confirmed;
                continue;
            }
            if (dswros::retry_area_quest_save_confirmation(
                    true, accepted,
                    area_quest_save_confirmation_attempts_[index],
                    kAreaQuestSaveConfirmationMaximumAttempts)) {
                area_quest_save_confirmation_pending_[word] |= bit;
                area_quest_save_confirmation_due_[index] =
                    now + kAreaQuestSaveConfirmationRetryDelay;
                ++area_retry_scheduled;
                continue;
            }
            area_quest_save_confirmation_due_[index] = {};
            area_quest_save_confirmation_attempts_[index] = 0;
            area_quest_completion_generation_locked_[index] = true;
            area_quest_completion_generation_reactivation_armed_[index] =
                false;
            ++area_exhausted;
            ++area_generation_locked;
        }
        recompute_area_quest_save_confirmation_due();

        const auto cooldown_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(
                kEncounterCooldown).count();
        std::size_t encounter_confirmed{};
        for (const auto& field : result.encounter_respawns) {
            if (!query_succeeded) {
                break;
            }
            const auto target = std::find_if(
                encounter_catalog_.begin(), encounter_catalog_.end(),
                [&field](const EncounterSpec& spec) {
                    return spec.id == field.id;
                });
            if (target == encounter_catalog_.end()) {
                continue;
            }
            const std::int64_t next_available =
                field.destroy_time_unix_seconds + cooldown_seconds;
            const auto existing =
                encounter_next_available_unix_seconds_.find(field.id);
            const std::int64_t current_next_available =
                existing == encounter_next_available_unix_seconds_.end()
                    ? 0 : existing->second;
            const std::int64_t merged = dswros::merge_encounter_cooldown(
                current_next_available, next_available);
            if (merged <= current_next_available) {
                continue;
            }
            encounter_next_available_unix_seconds_[field.id] = merged;
            ++encounter_confirmed;
        }
        if (encounter_confirmed > 0) {
            encounter_state_ready_ = true;
            establish_encounter_visibility_baseline(unix_seconds());
            compact_rebind_dirty_ = true;
            refresh_world_map_atlas_for_runtime_delta(
                "completion_confirmation", 0);
        }
        try {
            append_log("SAVE_COMPLETION_CONFIRMATION_APPLIED", std::format(
                "activation={} request_id={} databases={} area_requested={} area_confirmed={} area_already_resolved={} area_retry_scheduled={} area_exhausted={} area_generation_locked={} encounter_confirmed={} query_succeeded={} dynamic_completion_query={} dynamic_completion_identity_ambiguous={} owner_pointer_route={} owner_pattern_attempted={} owner_pattern_status={} save_key_field_route={} save_key_field_offset=0x{:X} save_key_candidates={} save_key_validations={} owner_resolution_us={} copy_us={} query_us={} total_us={} policy=event_driven_positive_only_strict_complete_count_growth_three_attempts_no_poll unlock=inactive_none_or_end_then_active_or_f7",
                activation_, result.request_id, result.database_count,
                area_requested, area_confirmed, area_already_resolved,
                area_retry_scheduled, area_exhausted,
                area_generation_locked, encounter_confirmed,
                query_succeeded,
                result.dynamic_quest_completion_query_available,
                result.dynamic_quest_completion_identity_ambiguous,
                static_cast<std::uint32_t>(result.owner_pointer_route),
                result.owner_pointer_pattern_attempted,
                static_cast<std::uint32_t>(
                    result.owner_pointer_pattern_status),
                static_cast<std::uint32_t>(result.save_key_field_route),
                result.save_key_field_offset,
                result.save_key_candidate_count,
                result.save_key_validation_count,
                result.owner_pointer_resolution_us,
                result.copy_elapsed_us, result.query_elapsed_us,
                result.total_elapsed_us));
        } catch (...) {
        }
        area_quest_save_confirmation_inflight_.fill(0);
    }

    void request_or_apply_save_reconcile() {
        if (auto result = save_reconciler_.try_take()) {
            const bool expected = save_reconcile_inflight_request_id_ != 0
                && result->request_id == save_reconcile_inflight_request_id_;
            save_reconcile_inflight_request_id_ = 0;
            if (!enabled_ || result->activation != activation_
                || !expected) {
                area_quest_save_confirmation_inflight_.fill(0);
                treasure_save_confirmation_inflight_.fill(0);
                append_log("SAVE_RECONCILE_DISCARDED", std::format(
                    "result_activation={} current_activation={} result_request_id={} enabled={} expected={} reason=stale_token",
                    result->activation, activation_, result->request_id,
                    enabled_, expected));
            } else if (!result->success()) {
                if (result->scope
                    == dsnwr::SaveReconcileScope::FullActivation) {
                    save_reconcile_completed_ = true;
                } else if (result->scope
                           == dsnwr::SaveReconcileScope::TreasureConfirmation) {
                    apply_treasure_save_confirmation_result(*result, false);
                } else {
                    apply_completion_confirmation_result(*result, false);
                }
                append_log("SAVE_RECONCILE_FAILED", std::format(
                    "activation={} request_id={} scope={} error={} owner_pointer_route={} owner_pattern_attempted={} owner_pattern_status={} save_key_field_route={} save_key_field_offset=0x{:X} save_key_candidates={} save_key_validations={} owner_resolution_us={} elapsed_us={} retry={}",
                    activation_, result->request_id,
                    static_cast<std::uint32_t>(result->scope),
                    static_cast<std::uint32_t>(result->error),
                    static_cast<std::uint32_t>(
                        result->owner_pointer_route),
                    result->owner_pointer_pattern_attempted,
                    static_cast<std::uint32_t>(
                        result->owner_pointer_pattern_status),
                    static_cast<std::uint32_t>(
                        result->save_key_field_route),
                    result->save_key_field_offset,
                    result->save_key_candidate_count,
                    result->save_key_validation_count,
                    result->owner_pointer_resolution_us,
                    result->total_elapsed_us,
                    result->scope
                            == dsnwr::SaveReconcileScope::FullActivation
                        ? "next_explicit_f6_retry_or_f7"
                        : (result->scope
                                   == dsnwr::SaveReconcileScope::TreasureConfirmation
                               ? "one_bounded_delayed_retry"
                               : "bounded_event_driven_retries")));
            } else if (result->scope
                       == dsnwr::SaveReconcileScope::FullActivation) {
                save_reconcile_completed_ = true;
                apply_full_save_reconcile_result(*result);
            } else if (result->scope
                       == dsnwr::SaveReconcileScope::TreasureConfirmation) {
                std::size_t inflight_count{};
                for (const std::uint64_t word
                     : treasure_save_confirmation_inflight_) {
                    inflight_count += std::popcount(word);
                }
                apply_treasure_save_confirmation_result(
                    *result,
                    inflight_count == result->requested_treasure_count);
            } else {
                apply_completion_confirmation_result(*result, true);
            }
        }

        if (!enabled_ || !save_reconciler_ready_
            || save_reconcile_inflight_request_id_ != 0) {
            return;
        }
        const auto now = Clock::now();
        if (now < save_reconcile_request_after_) {
            return;
        }
        if (!save_reconcile_requested_) {
            const std::uint32_t request_id =
                ++save_reconcile_next_request_id_;
            if (!save_reconciler_.request(
                    activation_, request_id,
                    dsnwr::SaveReconcileScope::FullActivation)) {
                save_reconcile_request_after_ =
                    now + kDiscoveryInterval;
                return;
            }
            save_reconcile_requested_ = true;
            save_reconcile_inflight_request_id_ = request_id;
            append_log("SAVE_RECONCILE_REQUESTED", std::format(
                "activation={} request_id={} scope=full_activation thread_priority=below_normal retry=none",
                activation_, request_id));
            return;
        }
        if (!save_reconcile_completed_) {
            return;
        }
        std::array<std::int64_t,
                   dsnwr::kMaximumTreasureConfirmationIds>
            treasure_confirmation_ids{};
        const std::size_t treasure_confirmation_count =
            collect_due_treasure_save_confirmations(
                now, treasure_confirmation_ids);
        if (treasure_confirmation_count > 0) {
            const std::uint32_t request_id =
                ++save_reconcile_next_request_id_;
            if (!save_reconciler_.request(
                    activation_, request_id,
                    dsnwr::SaveReconcileScope::TreasureConfirmation,
                    std::span<const std::int64_t>{
                        treasure_confirmation_ids.data(),
                        treasure_confirmation_count})) {
                restore_unsubmitted_treasure_save_confirmations(now);
                save_reconcile_request_after_ = now + kDiscoveryInterval;
                return;
            }
            save_reconcile_inflight_request_id_ = request_id;
            append_log("TREASURE_SAVE_CONFIRMATION_REQUESTED", std::format(
                "activation={} request_id={} ids={} scope=exact_treasure_positive_only thread_priority=below_normal retry=bounded_once full_treasure_query=false encounter_query=false dynamic_quest_query=false",
                activation_, request_id, treasure_confirmation_count));
            return;
        }
        const std::size_t area_confirmation_count =
            collect_due_area_quest_save_confirmations(now);
        if (area_confirmation_count == 0) {
            return;
        }
        const std::uint32_t request_id = ++save_reconcile_next_request_id_;
        if (!save_reconciler_.request(
                activation_, request_id,
                dsnwr::SaveReconcileScope::CompletionConfirmation)) {
            restore_unsubmitted_area_quest_save_confirmations(now);
            save_reconcile_request_after_ = now + kDiscoveryInterval;
            return;
        }
        save_reconcile_inflight_request_id_ = request_id;
        append_log("SAVE_COMPLETION_CONFIRMATION_REQUESTED", std::format(
            "activation={} request_id={} exact_area_ids={} scope=encounter_and_dynamic_quest_positive_only treasure_query=false trigger=expired_exact_area_quest_witness coalesced=true thread_priority=below_normal retry=bounded maximum_attempts=3 retry_delay_seconds=15 periodic_poll=false",
            activation_, request_id, area_confirmation_count));
    }

    void apply_full_save_reconcile_result(
        const dsnwr::SaveReconcileResult& reconciled) {
        const auto* result = &reconciled;

        std::size_t eligible_count{};
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            const std::int64_t id = render_catalog_entries_[index].id;
            const bool eligible = !ignored_treasure_ids_.contains(id)
                && !runtime_opened_treasure_ids_.contains(id)
                && !save_result_contains(*result, id);
            compact_eligibility_[index] =
                static_cast<std::uint8_t>(eligible ? 1 : 0);
            eligible_count += eligible ? 1U : 0U;
        }
        std::size_t mini_game_eligible_count{};
        for (std::size_t index = 0;
             index < mini_game_catalog_.size(); ++index) {
            const MiniGameSpec& spec = mini_game_catalog_[index];
            const bool eligible =
                !runtime_opened_treasure_ids_.contains(spec.reward_save_id)
                && !save_result_contains(*result, spec.reward_save_id);
            mini_game_eligibility_[index] =
                static_cast<std::uint8_t>(eligible ? 1 : 0);
            mini_game_eligible_count += eligible ? 1U : 0U;
        }

        area_quest_save_completion_.fill(0);
        area_quest_save_completion_counts_.fill(
            dswros::kUnknownAreaQuestSaveCompletionCount);
        area_quest_save_completion_query_available_ =
            result->dynamic_quest_completion_query_available
            && !result->dynamic_quest_completion_identity_ambiguous;
        area_quest_save_completion_match_count_ = 0;
        // The one-shot F7 save query runs asynchronously. A native task
        // completion may arrive after that snapshot was taken but before it is
        // applied here, so snapshot application must only merge positive
        // completion evidence. The activation lifecycle owns clearing;
        // an older save result must never roll back a newer runtime delta.
        if (area_quest_save_completion_query_available_) {
            // A valid, single-owner table snapshot proves zero for catalog IDs
            // absent from the completion table. Later event-driven confirmation
            // accepts only a strict increase over this per-ID baseline.
            area_quest_save_completion_counts_.fill(0);
            for (const auto& completion :
                 result->dynamic_quest_completions) {
                if (completion.quest_id <= 0) {
                    continue;
                }
                if (completion.complete_count > 0) {
                    completed_dynamic_quest_ids_.insert(
                        completion.quest_id);
                }
                for (std::size_t index = 0;
                     index < area_quest_catalog_.size(); ++index) {
                    if (area_quest_catalog_[index].id
                        != completion.quest_id) {
                        continue;
                    }
                    area_quest_save_completion_[index] =
                        static_cast<std::uint8_t>(
                            completion.complete_count > 0 ? 1 : 0);
                    area_quest_save_completion_counts_[index] =
                        completion.complete_count;
                    ++area_quest_save_completion_match_count_;
                    break;
                }
            }
        }
        const auto cooldown_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(
                kEncounterCooldown).count();
        std::size_t encounter_record_count{};
        for (const auto& field : result->encounter_respawns) {
            const auto target = std::find_if(
                encounter_catalog_.begin(), encounter_catalog_.end(),
                [&field](const EncounterSpec& spec) {
                    return spec.id == field.id;
                });
            if (target == encounter_catalog_.end()) {
                continue;
            }
            const std::int64_t next_available =
                field.destroy_time_unix_seconds + cooldown_seconds;
            const auto existing =
                encounter_next_available_unix_seconds_.find(field.id);
            const std::int64_t current_next_available =
                existing == encounter_next_available_unix_seconds_.end()
                    ? 0 : existing->second;
            encounter_next_available_unix_seconds_[field.id] =
                dswros::merge_encounter_cooldown(
                    current_next_available, next_available);
            ++encounter_record_count;
        }
        encounter_state_ready_ = true;
        establish_encounter_visibility_baseline(unix_seconds());
        const std::size_t area_quest_world_map_visible =
            area_quest_state_ready_
                ? rebuild_area_quest_world_map_eligibility()
                : 0U;
        const bool atlas_was_attached = world_map_umg_renderer_.state()
            == dsnwr::WorldMapUmgRendererState::Attached;
        refresh_world_map_atlas_for_runtime_delta(
            "save_reconcile", 0);
        treasure_eligibility_ready_ = true;
        compact_rebind_dirty_ = true;
        append_log("SAVE_RECONCILE_APPLIED", std::format(
            "activation={} databases={} opened_bits={} runtime_opened={} ignored={} eligible={} mini_games_eligible={} encounter_rows={} encounter_targets={} dynamic_completion_query={} dynamic_completion_user_dbid={} dynamic_completion_identity_ambiguous={} dynamic_completion_rows={} dynamic_completion_matches={} area_quest_world_map_visible={} cooldown_minutes=120 world_map_refresh={} rebuild=current_session_if_open_else_next_map_session owner_pointer_route={} owner_pattern_attempted={} owner_pattern_status={} save_key_field_route={} save_key_field_offset=0x{:X} save_key_candidates={} save_key_validations={} owner_resolution_us={} copy_us={} query_us={} total_us={}",
            activation_, result->database_count, result->opened_bit_count,
            runtime_opened_treasure_ids_.size(), ignored_treasure_ids_.size(),
            eligible_count, mini_game_eligible_count,
            result->encounter_respawns.size(), encounter_record_count,
            result->dynamic_quest_completion_query_available,
            result->dynamic_quest_completion_user_dbid,
            result->dynamic_quest_completion_identity_ambiguous,
            result->dynamic_quest_completions.size(),
            area_quest_save_completion_match_count_,
            area_quest_world_map_visible,
            atlas_was_attached ? "invalidated" : "not_attached",
            static_cast<std::uint32_t>(result->owner_pointer_route),
            result->owner_pointer_pattern_attempted,
            static_cast<std::uint32_t>(
                result->owner_pointer_pattern_status),
            static_cast<std::uint32_t>(result->save_key_field_route),
            result->save_key_field_offset,
            result->save_key_candidate_count,
            result->save_key_validation_count,
            result->owner_pointer_resolution_us,
            result->copy_elapsed_us, result->query_elapsed_us,
            result->total_elapsed_us));
    }

    [[nodiscard]] bool mark_treasure_opened(std::int64_t id) {
        if (id <= 0) {
            return false;
        }
        const bool newly_recorded =
            runtime_opened_treasure_ids_.insert(id).second;
        if (!newly_recorded) {
            return false;
        }
        bool compact_visibility_changed{};
        bool world_map_visibility_changed{};
        for (std::size_t index = 0; index < render_catalog_size_; ++index) {
            if (render_catalog_entries_[index].id == id) {
                const std::size_t word = index / 64U;
                const std::uint64_t bit = 1ULL << (index % 64U);
                const bool confirmation_was_pending =
                    (treasure_save_confirmation_pending_[word] & bit) != 0;
                treasure_save_confirmation_pending_[word] &= ~bit;
                if (confirmation_was_pending) {
                    recompute_treasure_save_confirmation_due();
                }
                if ((treasure_save_confirmation_inflight_[word] & bit)
                    == 0) {
                    treasure_save_confirmation_due_[index] = {};
                    treasure_save_confirmation_attempts_[index] = 0;
                }
                if (compact_eligibility_[index] != 0) {
                    compact_eligibility_[index] = 0;
                    compact_visibility_changed = true;
                    world_map_visibility_changed =
                        render_catalog_entries_[index].map_id
                        == dswros::CompactRenderModel::kCompactMapId;
                }
                break;
            }
        }
        for (std::size_t index = 0;
             index < mini_game_catalog_.size(); ++index) {
            if (mini_game_catalog_[index].reward_save_id == id) {
                if (mini_game_eligibility_[index] != 0) {
                    mini_game_eligibility_[index] = 0;
                    compact_visibility_changed = true;
                    world_map_visibility_changed =
                        world_map_visibility_changed
                        || mini_game_catalog_[index].map_id
                            == dswros::CompactRenderModel::kCompactMapId;
                }
            }
        }
        compact_rebind_dirty_ = compact_rebind_dirty_
            || compact_visibility_changed;
        if (!world_map_visibility_changed) {
            return compact_visibility_changed;
        }
        refresh_world_map_atlas_for_runtime_delta(
            "treasure_opened", id);
        return compact_visibility_changed || world_map_visibility_changed;
    }

    [[nodiscard]] static UWorld* object_world_guarded(
        UObject* object) noexcept {
#if defined(_MSC_VER)
        __try {
            return object ? object->GetWorld() : nullptr;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
#else
        try {
            return object ? object->GetWorld() : nullptr;
        } catch (...) {
            return nullptr;
        }
#endif
    }

    [[nodiscard]] static bool weak_candidate_belongs_to_world_guarded(
        FWeakObjectPtr weak, UWorld* expected_world) noexcept {
        if (!expected_world) {
            return false;
        }
#if defined(_MSC_VER)
        __try {
            UObject* object = weak.Get();
            return object && object->GetWorld() == expected_world;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        try {
            UObject* object = weak.Get();
            return object && object->GetWorld() == expected_world;
        } catch (...) {
            return false;
        }
#endif
    }

    void prune_ui_layer_candidates_for_world(
        UWorld* current_world) noexcept {
        const bool world_map_candidate_matches =
            world_map_candidate_available_
            && weak_candidate_belongs_to_world_guarded(
                world_map_layer_candidate_, current_world);
        if (!world_map_candidate_matches) {
            world_map_umg_renderer_.detach();
            reset_world_map_runtime(false);
        }

        const bool compact_candidate_matches =
            compact_candidate_available_
            && weak_candidate_belongs_to_world_guarded(
                compact_layer_candidate_, current_world);
        if (!compact_candidate_matches) {
            compact_umg_renderer_.detach();
            compact_layer_candidate_ = FWeakObjectPtr{};
            compact_candidate_available_ = false;
            reset_compact_pool_runtime();
        }
    }

    void transition_begin() {
        if (!game_thread()) {
            return;
        }
        // The transient Hub is independent from Radar activation. Tear it
        // down before either disabled/runtime-unavailable branch so no old
        // world widget or input-mode ownership survives travel.
        visibility_hub_.detach();
        visibility_hub_world_map_refresh_pending_ = false;
        visibility_hub_world_map_baseline_valid_ = false;
        visibility_hub_service_after_ = {};
        visibility_hub_toggle_after_ = {};
        if (!required_runtime_ready_.load(std::memory_order_acquire)) {
            transition_active_ = true;
            return;
        }
        // InitGameState pre-transition still owns a valid old-world UMG tree.
        // Detach every retained viewport host before the disabled early return
        // as well: F8 suspends hosts rather than destroying them, so a travel
        // while disabled must not carry old-world widget handles forward.
        compact_umg_renderer_.detach();
        reset_compact_pool_runtime();
        world_map_umg_renderer_.detach();
        reset_world_map_runtime(false);
        if (!enabled_) {
            transition_active_ = true;
            return;
        }
        // A task completion and travel can occur in the same game-thread
        // frame. Drain the fixed atomic bits before lifecycle reset so the
        // activation-local completion evidence survives the transition.
        consume_exact_area_quest_completions(true);
        consume_pending_encounter_deaths(true);
        transition_active_ = true;
        world_map_compact_suppressed_ = false;
        game_paused_ = false;
        game_pause_sample_known_ = false;
        area_quest_scan_pending_ = false;
        area_quest_rescan_scheduled_ = false;
        area_quest_time_rescan_scheduled_ = false;
        area_quest_rescan_requests_.store(0, std::memory_order_release);
        // Task-class/UObject identity is world-local, but a completion witness
        // contains only a catalog index, deadline, and bounded exact-ID probe
        // schedule.
        // Preserve that numeric evidence across travel so a completion event
        // immediately followed by travel can be consumed by the travel-end
        // scan before its original ten-second deadline.
        reset_area_quest_task_class_runtime(false, false);
        area_quest_state_ready_ = false;
        area_quest_eligibility_.fill(0);
        area_quest_world_map_eligibility_.fill(0);
        area_quest_states_.fill(dswros::AreaQuestState::Unknown);
        area_quest_scan_previous_states_.fill(
            dswros::AreaQuestState::Unknown);
        position_valid_ = false;
        ++epoch_;
        // Exact accepted death evidence is catalog-index only and may survive
        // this world reset until a valid encounter-state baseline can apply it.
        tracker_.reset(activation_, epoch_);
        observed_objects_.clear();
        reset_bird_egg_active_visibility();
        // Keep the unpublished weak mailbox until transition_end can compare
        // its layer world with the new GameMode world. DLayerMiniMap may be
        // created just before InitGameStatePre; clearing the mailbox here
        // would lose that only event. The old active candidate is never kept.
        compact_layer_candidate_ = FWeakObjectPtr{};
        compact_candidate_available_ = false;
        clear_created_encounter_candidates();
    }

    void transition_end(AGameModeBase* game_mode) {
        if (!game_thread()
            || !required_runtime_ready_.load(std::memory_order_acquire)) {
            return;
        }
        UWorld* current_world = object_world_guarded(
            reinterpret_cast<UObject*>(game_mode));
        std::string transition_world_key{};
        static_cast<void>(world_identity_key_guarded(
            reinterpret_cast<UObject*>(current_world),
            &transition_world_key));
        current_world_key_ = transition_world_key;
        if (is_main_menu_world_identity(current_world_key_)) {
            disable_for_main_menu_owner_boundary("transition-end");
            return;
        }
        if (!enabled_ || main_menu_activation_latched_) {
            prune_ui_layer_candidates_for_world(current_world);
            if (current_world) {
                consume_world_map_listener_candidate(current_world);
                consume_compact_listener_candidate(current_world);
                prune_bird_egg_candidates_for_world(current_world);
            } else {
                clear_world_map_listener_candidate();
                clear_compact_listener_candidate();
                clear_bird_egg_candidates();
            }
            transition_active_ = false;
            return;
        }
        compact_umg_renderer_.begin_activation();
        reset_compact_pool_runtime();
        world_map_umg_renderer_.begin_activation();
        if (current_world) {
            consume_world_map_listener_candidate(current_world);
            consume_compact_listener_candidate(current_world);
            prune_bird_egg_candidates_for_world(current_world);
        } else {
            // Without an exact new-world identity, discard rather than risk
            // promoting a valid-but-old UMG layer across travel. Both
            // renderers retain their existing one-shot bounded catch-up.
            clear_world_map_listener_candidate();
            clear_compact_listener_candidate();
            clear_bird_egg_candidates();
        }
        reset_world_map_runtime(true);
        if (!update_activity_context_guarded(game_mode)) {
            current_context_key_.clear();
            (void)recompute_activity_suppression("game-state-probe-failed");
            append_log("ACTIVITY_CONTEXT", std::format(
                "activation={} epoch={} baseline={} current=probe-failed compact_paint_suppressed=false",
                activation_, epoch_, baseline_context_key_.empty() ? "unavailable" : baseline_context_key_));
        }
        transition_active_ = false;
        area_quest_scan_faulted_ = false;
        area_quest_state_ready_ = !area_quest_state_provider_ready_;
        reset_area_quest_task_class_runtime(
            area_quest_state_provider_ready_
            && area_quest_catalog_.size() == kExpectedAreaQuestCount,
            false);
        request_area_quest_scan("travel_end");
        stable_after_ = Clock::now() + kTransitionCooldown;
        next_position_ = stable_after_;
        next_discovery_ = stable_after_;
        next_activity_probe_ = stable_after_;
        next_runtime_visibility_edge_probe_ = stable_after_;
        if (!activity_suppressed_) {
            world_map_activation_catch_up(current_world);
            refresh_compact_menu_state(
                current_world, "travel_visibility_catch_up");
        }
    }

    [[nodiscard]] bool capture_activation_context_guarded(UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try {
            capture_activation_context_unsafe(engine);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        capture_activation_context_unsafe(engine);
        return true;
#endif
    }

    void capture_activation_context_unsafe(UEngine* engine) {
        if (!engine) return;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        UObject* viewport = viewport_value ? *viewport_value : nullptr;
        auto** world_value = viewport
            ? viewport->GetValuePtrByPropertyNameInChain<UObject*>(STR("World")) : nullptr;
        UObject* world = world_value ? *world_value : nullptr;
        auto** game_mode_value = world
            ? world->GetValuePtrByPropertyNameInChain<UObject*>(STR("AuthorityGameMode")) : nullptr;
        UObject* game_mode = game_mode_value ? *game_mode_value : nullptr;
        current_context_key_ = activity_context_key(game_mode);
        current_world_key_ = world_identity_key(world);
        if (baseline_world_key_.empty() && is_open_world_identity(current_world_key_)) {
            baseline_world_key_ = current_world_key_;
            baseline_context_key_ = current_context_key_;
        }
        (void)recompute_activity_suppression("f7-context-capture");
        refresh_compact_menu_state(
            reinterpret_cast<UWorld*>(world),
            "f7_visibility_catch_up");
    }

    [[nodiscard]] bool update_activity_context_guarded(AGameModeBase* game_mode) noexcept {
#if defined(_MSC_VER)
        __try {
            update_activity_context_unsafe(game_mode);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        update_activity_context_unsafe(game_mode);
        return true;
#endif
    }

    void update_activity_context_unsafe(AGameModeBase* game_mode) {
        current_context_key_ = activity_context_key(reinterpret_cast<UObject*>(game_mode));
        (void)recompute_activity_suppression("game-state-callback");
        append_log("ACTIVITY_CONTEXT", std::format(
            "activation={} epoch={} baseline={} current={} world_baseline={} world_current={} compact_paint_suppressed={}",
            activation_, epoch_, baseline_context_key_.empty() ? "unavailable" : baseline_context_key_,
            current_context_key_.empty() ? "unavailable" : current_context_key_,
            baseline_world_key_.empty() ? "unavailable" : baseline_world_key_,
            current_world_key_.empty() ? "unavailable" : current_world_key_, activity_suppressed_));
    }

    void probe_activity_context_guarded(UEngine* engine) noexcept {
#if defined(_MSC_VER)
        __try { probe_activity_context_unsafe(engine); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return; }
#else
        probe_activity_context_unsafe(engine);
#endif
    }

    void probe_activity_context_unsafe(UEngine* engine) {
        if (!engine) return;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        UObject* viewport = viewport_value ? *viewport_value : nullptr;
        auto** world_value = viewport
            ? viewport->GetValuePtrByPropertyNameInChain<UObject*>(STR("World")) : nullptr;
        UObject* world = world_value ? *world_value : nullptr;
        const std::string world_key = world_identity_key(world);
        if (world_key.empty()) return;
        auto** game_mode_value = world->GetValuePtrByPropertyNameInChain<UObject*>(STR("AuthorityGameMode"));
        UObject* game_mode = game_mode_value ? *game_mode_value : nullptr;
        const std::string context_key = activity_context_key(game_mode);
        const bool identity_changed = world_key != current_world_key_ || context_key != current_context_key_;
        current_world_key_ = world_key;
        current_context_key_ = context_key;
        if (is_main_menu_world_identity(current_world_key_)) {
            disable_for_main_menu_owner_boundary("world-identity-probe");
            return;
        }
        if (baseline_world_key_.empty() && is_open_world_identity(current_world_key_)) {
            baseline_world_key_ = current_world_key_;
            baseline_context_key_ = current_context_key_;
        }
        const bool suppression_changed = recompute_activity_suppression("world-identity-probe");
        refresh_compact_menu_state(
            reinterpret_cast<UWorld*>(world),
            "shared_250ms_activity_probe");
        if (identity_changed || suppression_changed) {
            append_log("ACTIVITY_WORLD", std::format(
                "activation={} epoch={} world_baseline={} world_current={} context_baseline={} context_current={} compact_paint_suppressed={}",
                activation_, epoch_, baseline_world_key_.empty() ? "unavailable" : baseline_world_key_,
                current_world_key_, baseline_context_key_.empty() ? "unavailable" : baseline_context_key_,
                current_context_key_.empty() ? "unavailable" : current_context_key_, activity_suppressed_));
        }
    }

    [[nodiscard]] std::size_t
    clear_encounter_observations_for_activity_suppression() noexcept {
        std::size_t cleared{};
        for (auto it = observed_objects_.begin();
             it != observed_objects_.end();) {
            if (it->second.kind
                == dswros::EventKind::EncounterDefeated) {
                it = observed_objects_.erase(it);
                ++cleared;
            } else {
                ++it;
            }
        }
        {
            const std::scoped_lock lock{created_encounter_mutex_};
            created_encounter_processed_identity_.fill(0);
            created_encounter_processed_activation_.fill(0);
        }
        return cleared;
    }

    void apply_activity_suppression_edge(
        const char* reason, bool changed) noexcept {
        if (!changed) {
            return;
        }
        std::size_t encounter_observations_cleared{};
        if (dswros::encounter_activity_edge_requires_reset(
                changed, activity_suppressed_)) {
            // The callback already accepted exact same-world, in-range death
            // evidence. Apply its fixed catalog bit before the activity edge
            // clears world-local observers; no UObject is retained or read.
            consume_pending_encounter_deaths(true);
            // This boundary is authoritative even when Pawn sampling has
            // already failed and the 250 ms observation service cannot run.
            // Keep the weak candidate slots so a still-live same-world actor
            // can bind again after the open-world return edge, but discard all
            // armed disappearance evidence and processed identities now.
            encounter_observations_cleared =
                clear_encounter_observations_for_activity_suppression();
        }
        if (activity_suppressed_) {
            // Activity maps own a different world-local widget tree. Never
            // retain or uncollapse the open-world compact host across that
            // boundary: the old weak handles can become stale before the
            // open-world identity returns.
            compact_umg_renderer_.detach();
            reset_compact_pool_runtime();
            reset_bird_egg_active_visibility();
            world_map_umg_renderer_.detach();
            reset_world_map_runtime(false);
        } else if (enabled_ && !transition_active_) {
            // Recreate both renderers from current-world objects on the next
            // bounded service pass. This is edge-only lifecycle work and adds
            // no recurring discovery or per-frame cost.
            compact_umg_renderer_.begin_activation();
            reset_compact_pool_runtime();
            world_map_umg_renderer_.begin_activation();
            reset_world_map_runtime(false);
            world_map_activation_catch_up();
        }
        try {
            append_log("RADAR_ACTIVITY_SUPPRESSION", std::format(
                "activation={} epoch={} suppressed={} reason={} compact_work={} world_map_work={} encounter_observations_cleared={} encounter_processed_identities=cleared_on_entry",
                activation_, epoch_, activity_suppressed_,
                reason ? reason : "unspecified",
                activity_suppressed_ ? "detached" : "rearmed",
                activity_suppressed_ ? "detached" : "event_rearmed",
                encounter_observations_cleared));
        } catch (...) {
        }
    }

    [[nodiscard]] bool recompute_activity_suppression(
        const char* reason) noexcept {
        const bool known_non_open_world = baseline_world_key_.empty()
            && !current_world_key_.empty()
            && !is_open_world_identity(current_world_key_);
        const bool world_mismatch = !baseline_world_key_.empty() && !current_world_key_.empty()
            && current_world_key_ != baseline_world_key_;
        const bool context_mismatch = !baseline_context_key_.empty() && !current_context_key_.empty()
            && current_context_key_ != baseline_context_key_;
        const bool next = enabled_ && (known_non_open_world || world_mismatch || context_mismatch);
        const bool changed = next != activity_suppressed_;
        activity_suppressed_ = next;
        apply_activity_suppression_edge(reason, changed);
        return changed;
    }

    [[nodiscard]] static std::string world_identity_key(UObject* world) {
        if (!world) return {};
        return to_string(world->GetFullName());
    }

    static void world_identity_key_unsafe(
        UObject* world, std::string* output) {
        if (!output) {
            return;
        }
        *output = world_identity_key(world);
    }

    [[nodiscard]] static bool world_identity_key_guarded(
        UObject* world, std::string* output) noexcept {
        if (!output) {
            return false;
        }
#if defined(_MSC_VER)
        __try {
            world_identity_key_unsafe(world, output);
            return !output->empty();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            output->clear();
            return false;
        }
#else
        try {
            world_identity_key_unsafe(world, output);
            return !output->empty();
        } catch (...) {
            output->clear();
            return false;
        }
#endif
    }

    [[nodiscard]] static bool is_open_world_identity(const std::string& key) noexcept {
        return key.find("/Maps/World/") != std::string::npos;
    }

    [[nodiscard]] static bool is_main_menu_world_identity(
        const std::string& key) noexcept {
        return dswros::is_main_menu_world_identity(key);
    }

    static void capture_current_world_identity_unsafe(
        UEngine* engine, std::string* output,
        UWorld** world_output) {
        if (!engine || !output) {
            return;
        }
        auto** viewport_value =
            engine->GetValuePtrByPropertyNameInChain<UObject*>(
                STR("GameViewport"));
        UObject* viewport = viewport_value ? *viewport_value : nullptr;
        auto** world_value = viewport
            ? viewport->GetValuePtrByPropertyNameInChain<UObject*>(
                  STR("World"))
            : nullptr;
        UWorld* world = reinterpret_cast<UWorld*>(
            world_value ? *world_value : nullptr);
        *output = world_identity_key(reinterpret_cast<UObject*>(world));
        if (world_output) {
            *world_output = world;
        }
    }

    [[nodiscard]] static bool capture_current_world_identity_guarded(
        UEngine* engine, std::string* output,
        UWorld** world_output = nullptr) noexcept {
        if (!output) {
            return false;
        }
        if (world_output) {
            *world_output = nullptr;
        }
#if defined(_MSC_VER)
        __try {
            capture_current_world_identity_unsafe(
                engine, output, world_output);
            return !output->empty();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            output->clear();
            if (world_output) {
                *world_output = nullptr;
            }
            return false;
        }
#else
        try {
            capture_current_world_identity_unsafe(
                engine, output, world_output);
            return !output->empty();
        } catch (...) {
            output->clear();
            if (world_output) {
                *world_output = nullptr;
            }
            return false;
        }
#endif
    }

    [[nodiscard]] static std::string activity_context_key(UObject* game_mode) {
        if (!game_mode) return {};
        auto* game_mode_class = game_mode->GetClassPrivate();
        if (!game_mode_class) return {};
        std::string result = to_string(game_mode_class->GetName());
        auto** game_state_value = game_mode->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameState"));
        UObject* game_state = game_state_value ? *game_state_value : nullptr;
        auto* game_state_class = game_state ? game_state->GetClassPrivate() : nullptr;
        result += '|';
        result += game_state_class ? to_string(game_state_class->GetName()) : "none";
        return result;
    }

    [[nodiscard]] UObject* current_game_instance(UEngine* engine) {
        if (!engine) return nullptr;
        auto** viewport_value = engine->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameViewport"));
        auto* viewport = viewport_value ? *viewport_value : nullptr;
        auto** game_instance_value = viewport
            ? viewport->GetValuePtrByPropertyNameInChain<UObject*>(STR("GameInstance")) : nullptr;
        return game_instance_value ? *game_instance_value : nullptr;
    }

    [[nodiscard]] UObject* current_player_controller(UEngine* engine) {
        auto* game_instance = current_game_instance(engine);
        auto* players = game_instance
            ? game_instance->GetValuePtrByPropertyNameInChain<FScriptArray>(STR("LocalPlayers")) : nullptr;
        if (!players || !players->IsValidIndex(0) || !players->GetData()) {
            return nullptr;
        }
        auto* local_player = static_cast<UObject* const*>(players->GetData())[0];
        auto** controller_value = local_player
            ? local_player->GetValuePtrByPropertyNameInChain<UObject*>(STR("PlayerController")) : nullptr;
        return controller_value ? *controller_value : nullptr;
    }

    [[nodiscard]] UObject* current_player_pawn(UEngine* engine) {
        UObject* controller = current_player_controller(engine);
        auto** pawn_value = controller
            ? controller->GetValuePtrByPropertyNameInChain<UObject*>(
                STR("Pawn"))
            : nullptr;
        return pawn_value ? *pawn_value : nullptr;
    }

    [[nodiscard]] bool read_player_position(UEngine* engine, dswros::Position* output) {
        if (!output || !location_function_) return false;
        auto* controller = current_player_controller(engine);
        auto* cursor_property = controller
            ? CastField<FBoolProperty>(
                controller->GetPropertyByNameInChain(
                    STR("bShowMouseCursor")))
            : nullptr;
        mouse_cursor_visible_ = cursor_property
            && cursor_property->GetPropertyValueInContainer(controller);
        auto** pawn_value = controller
            ? controller->GetValuePtrByPropertyNameInChain<UObject*>(
                STR("Pawn"))
            : nullptr;
        auto* pawn = pawn_value ? *pawn_value : nullptr;
        if (!pawn) return false;
        return read_actor_position(pawn, output);
    }

    [[nodiscard]] bool read_actor_position(UObject* actor, dswros::Position* output) {
        if (!actor || !output || !location_function_) return false;
        struct Parameters { FVector return_value{}; } parameters{};
        actor->ProcessEvent(location_function_, &parameters);
        *output = {parameters.return_value.X(), parameters.return_value.Y(), parameters.return_value.Z()};
        return std::isfinite(output->x) && std::isfinite(output->y) && std::isfinite(output->z);
    }

    void actor_begin(AActor* actor) noexcept {
        if (transition_active_ || !game_thread() || !actor) return;
#if defined(_MSC_VER)
        __try { actor_begin_unsafe(actor); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        actor_begin_unsafe(actor);
#endif
    }

    void actor_begin_unsafe(AActor* actor) {
        auto* object_class = actor->GetClassPrivate();
        if (!object_class) return;
        const std::string class_name = to_string(object_class->GetName());
        if (is_bird_egg_class(class_name)) {
            FWeakObjectPtr weak{};
            weak = actor;
            // Availability is resolved from the owned InteractComponent on the
            // bounded discovery service. BeginPlay may run before that
            // component reaches its live state, so never seed or retire here.
            static_cast<void>(publish_created_bird_egg_candidate(weak));
            return;
        }
        if (!enabled_) return;
        if (!is_target_class(class_name)) return;
        dswros::Position position{};
        if (!read_actor_position(actor, &position)) return;
        FWeakObjectPtr weak{};
        weak = actor;
        const dswros::WeakIdentity identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        if (is_treasure_class(class_name)) {
            if (const auto id = tracker_.observe(identity, class_name, position)) {
                bind_observed(weak, identity, dswros::EventKind::TreasureOpened, *id, position);
            }
        }
        observe_encounter(weak, identity, class_name, position);
    }

    void actor_end(AActor* actor, EEndPlayReason reason) noexcept {
        if (!enabled_ || !game_thread() || !actor) return;
#if defined(_MSC_VER)
        __try { actor_end_unsafe(actor, reason); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        actor_end_unsafe(actor, reason);
#endif
    }

    void actor_end_unsafe(AActor* actor, EEndPlayReason reason) {
        FWeakObjectPtr weak{};
        weak = actor;
        const dswros::WeakIdentity identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        auto* object_class = actor->GetClassPrivate();
        if (object_class
            && is_bird_egg_class(to_string(object_class->GetName()))) {
            mark_bird_egg_end(identity);
            return;
        }
        static_cast<void>(tracker_.end(identity, false, transition_active_, player_, activation_, epoch_));
        const auto found = observed_objects_.find(identity.packed());
        if (found == observed_objects_.end()) {
            static_cast<void>(recover_unobserved_encounter_end(
                weak, identity, actor, reason));
            return;
        }
        const bool destroyed = reason == EEndPlayReason::Destroyed;
        const bool treasure_removed = found->second.kind
                == dswros::EventKind::TreasureOpened
            && reason == EEndPlayReason::RemovedFromWorld;
        const bool encounter_removed =
            dswros::preserve_observed_encounter_removal(
                found->second.kind
                    == dswros::EventKind::EncounterDefeated,
                reason == EEndPlayReason::RemovedFromWorld,
                !transition_active_
                    && found->second.activation == activation_
                    && found->second.epoch == epoch_);
        const bool eligible = destroyed || treasure_removed
            || encounter_removed;
        if (!eligible || transition_active_ || found->second.activation != activation_ || found->second.epoch != epoch_) {
            observed_objects_.erase(found);
            return;
        }
        if (destroyed
            && found->second.kind
                == dswros::EventKind::EncounterDefeated) {
            dswros::Position end_position{};
            if (read_actor_position(actor, &end_position)
                && distance_squared(player_, end_position)
                    <= kEncounterObservationRadius
                        * kEncounterObservationRadius) {
                // Preserve the last position that still belonged to the
                // player's local encounter context. A pooled actor can be
                // moved far away before EndPlay and must not poison fallback
                // evidence with that relocation.
                found->second.position = end_position;
            }
        }
        if (found->second.kind == dswros::EventKind::TreasureOpened) {
            found->second.disappearance.mark_eligible_end();
        } else if (encounter_removed && found->second.visible_seen) {
            const auto now_milliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    Clock::now().time_since_epoch()).count();
            found->second.encounter_disappearance
                .arm_observed_end_fallback(now_milliseconds);
        }
        found->second.logical_end = true;
        found->second.destroyed_end = destroyed;
        found->second.weak = FWeakObjectPtr{};
        append_log("OBJECT_END_NATIVE", std::format("activation={} epoch={} kind={} id={} encounter_type={} reason={}",
            activation_, epoch_, static_cast<std::uint32_t>(found->second.kind), found->second.id,
            found->second.kind == dswros::EventKind::EncounterDefeated
                ? encounter_type_for_id(found->second.id) : "none",
            static_cast<std::uint32_t>(reason)));
    }

    [[nodiscard]] bool recover_unobserved_encounter_end(
        FWeakObjectPtr weak, dswros::WeakIdentity identity, AActor* actor,
        EEndPlayReason reason) {
        if (!actor || reason != EEndPlayReason::Destroyed
            || transition_active_ || activity_suppressed_ || !position_valid_
            || !encounter_state_ready_ || !identity.valid()) {
            return false;
        }
        auto* object_class = actor->GetClassPrivate();
        if (!object_class) {
            return false;
        }
        const std::string class_name = to_string(object_class->GetName());
        const EncounterSpec* spec = encounter_spec_for_class(class_name);
        if (!spec) {
            return false;
        }
        dswros::Position actor_position{};
        const bool actor_position_valid =
            read_actor_position(actor, &actor_position);
        const double radius_squared =
            kEncounterObservationRadius * kEncounterObservationRadius;
        const bool already_observed = std::any_of(
            observed_objects_.begin(), observed_objects_.end(),
            [spec](const auto& entry) {
                return entry.second.kind
                        == dswros::EventKind::EncounterDefeated
                    && entry.second.id == spec->id;
            });
        const auto now_unix_seconds = unix_seconds();
        const dswros::UnobservedEncounterEndEvidence evidence{
            reason == EEndPlayReason::Destroyed,
            transition_active_,
            activity_suppressed_,
            position_valid_,
            actor_position_valid,
            encounter_state_ready_,
            encounter_available(*spec, now_unix_seconds),
            true,
            actor_position_valid
                && distance_squared(player_, actor_position)
                    <= radius_squared,
            identity.valid(),
            already_observed};
        if (!dswros::accept_unobserved_encounter_end(evidence)) {
            append_log("OBJECT_END_RECOVERY_REJECTED_NATIVE", std::format(
                "activation={} epoch={} id={} encounter_type={} reason={} destroyed={} transition={} suppressed={} player_position={} actor_position={} state_ready={} available={} exact_catalog_class={} player_near_actor={} weak_identity={} duplicate={}",
                activation_, epoch_, spec->id,
                encounter_kind_name(spec->kind),
                static_cast<std::uint32_t>(reason),
                evidence.destroyed, evidence.transition_active,
                evidence.activity_suppressed, evidence.player_position_valid,
                evidence.actor_position_valid,
                evidence.encounter_state_ready, evidence.encounter_available,
                evidence.exact_catalog_class, evidence.player_near_actor,
                evidence.weak_identity_valid,
                evidence.duplicate_observation));
            return false;
        }
        ObservedRuntimeObject recovered{
            weak, dswros::EventKind::EncounterDefeated, spec->id,
            actor_position, activation_, epoch_, {}, {}, true, true, true,
            false, std::nullopt};
        recovered.recovered_from_end_play = true;
        recovered.disappearance.mark_eligible_end();
        observed_objects_.emplace(identity.packed(), std::move(recovered));
        append_log("OBJECT_END_RECOVERED_NATIVE", std::format(
            "activation={} epoch={} kind={} id={} encounter_type={} reason={} evidence=destroyed_exact_unique_class_player_to_actor_proximity_current_available",
            activation_, epoch_,
            static_cast<std::uint32_t>(
                dswros::EventKind::EncounterDefeated),
            spec->id, encounter_kind_name(spec->kind),
            static_cast<std::uint32_t>(reason)));
        return true;
    }

    void observe_encounter(FWeakObjectPtr weak, dswros::WeakIdentity identity, const std::string& class_name,
                           const dswros::Position& position) {
        if (activity_suppressed_) {
            return;
        }
        // All 49 generated encounter classes are load-time unique. Identity
        // comes from that exact class; the 100-metre safety bound is always
        // player-to-current-actor, never player/actor-to-static spawn point.
        const EncounterSpec* spec = encounter_spec_for_class(class_name);
        if (spec
            && encounter_state_ready_
            && encounter_available(*spec, unix_seconds())) {
            bind_observed(weak, identity,
                dswros::EventKind::EncounterDefeated, spec->id,
                position);
        }
    }

    void bind_observed(FWeakObjectPtr weak, dswros::WeakIdentity identity, dswros::EventKind kind,
                       std::int64_t id, const dswros::Position& position) {
        if (!position_valid_) return;
        const double radius = observation_radius(kind);
        if (distance_squared(player_, position) > radius * radius) return;
        for (auto it = observed_objects_.begin(); it != observed_objects_.end();) {
            if (it->second.kind == kind && it->second.id == id && it->first != identity.packed()) {
                it = observed_objects_.erase(it);
            } else ++it;
        }
        bool hidden = false;
        const bool visible_seen = !read_actor_hidden(weak.Get(), &hidden) || !hidden;
        observed_objects_[identity.packed()] = {
            weak, kind, id, position, activation_, epoch_, {}, {}, false,
            false, visible_seen, false, std::nullopt};
        append_log("OBJECT_OBSERVED_NATIVE", std::format("activation={} epoch={} kind={} id={} class={} encounter_type={}",
            activation_, epoch_, static_cast<std::uint32_t>(kind), id,
            kind == dswros::EventKind::TreasureOpened ? "treasure" : "encounter",
            kind == dswros::EventKind::EncounterDefeated
                ? encounter_type_for_id(id) : "none"));
    }

    void probe_observed_objects() {
        const auto now = Clock::now();
        const auto now_milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
        const auto now_unix_seconds = unix_seconds();
        for (auto it = observed_objects_.begin(); it != observed_objects_.end();) {
            auto& observed = it->second;
            const bool lifecycle_valid = !transition_active_ && position_valid_
                && !activity_suppressed_
                && observed.activation == activation_ && observed.epoch == epoch_;
            const EncounterSpec* current_encounter = observed.kind
                    == dswros::EventKind::EncounterDefeated
                ? encounter_spec_for_id(observed.id) : nullptr;
            const bool encounter_currently_available = observed.kind
                    != dswros::EventKind::EncounterDefeated
                || (current_encounter
                    && encounter_available(
                        *current_encounter, now_unix_seconds));
            if (!lifecycle_valid || !encounter_currently_available) {
                if (observed.kind
                    == dswros::EventKind::EncounterDefeated) {
                    if (lifecycle_valid && !encounter_currently_available) {
                        append_log("OBJECT_EVICTED_NATIVE", std::format(
                            "activation={} epoch={} kind={} id={} reason=encounter_not_currently_available",
                            activation_, epoch_,
                            static_cast<std::uint32_t>(observed.kind),
                            observed.id));
                    }
                    clear_processed_encounter_identity(observed.id);
                }
                it = observed_objects_.erase(it);
                continue;
            }
            // EndPlay is a logical lifetime boundary. Never resolve or call
            // ProcessEvent on an object after that boundary.
            bool present = false;
            std::optional<dswros::Position> current_live_position{};
            if (!observed.logical_end) {
                UObject* live_object = observed.weak.Get();
                if (live_object
                    && observed.kind == dswros::EventKind::EncounterDefeated) {
                    dswros::Position live_position{};
                    if (read_actor_position(live_object, &live_position)) {
                        current_live_position = live_position;
                    }
                }
                bool hidden = false;
                const bool hidden_known =
                    live_object && read_actor_hidden(live_object, &hidden);
                if (hidden_known && !hidden) {
                    observed.visible_seen = true;
                }
                const bool visually_present =
                    !hidden_known || !observed.visible_seen || !hidden;
                present = live_object != nullptr && visually_present;
            }
            const double radius = observation_radius(observed.kind);
            const bool live_position_inside = current_live_position
                && distance_squared(player_, *current_live_position)
                    <= radius * radius;
            const bool encounter_left_live_context = observed.kind
                    == dswros::EventKind::EncounterDefeated
                && observed.outside_since.has_value();
            const bool inside = current_live_position
                ? live_position_inside
                : (!encounter_left_live_context
                    && distance_squared(player_, observed.position)
                        <= radius * radius);
            if (live_position_inside) {
                // Keep only an in-range actor position as fallback evidence.
                // A far pool relocation still drives the current inside/outside
                // decision but cannot replace the last trusted coordinate.
                observed.position = *current_live_position;
            }
            if (inside) {
                observed.outside_since.reset();
            } else if (!observed.outside_since) {
                observed.outside_since = now;
            }
            const bool grace_active = observed.outside_since
                && now - *observed.outside_since <= kDepartureGrace;
            const bool context_valid = observed.kind
                    == dswros::EventKind::TreasureOpened
                ? inside || grace_active
                : dswros::accept_encounter_disappearance({
                    lifecycle_valid
                        && dswros::encounter_cursor_context_allowed(
                            mouse_cursor_visible_, observed.logical_end),
                    observed.destroyed_end, observed.visible_seen, inside});
            const bool disappearance_confirmed = observed.kind
                    == dswros::EventKind::TreasureOpened
                ? observed.disappearance.sample(present, context_valid)
                : observed.encounter_disappearance.sample(
                    present, context_valid, observed.destroyed_end,
                    now_milliseconds);
            if (disappearance_confirmed) {
                bool state_applied = false;
                if (observed.kind == dswros::EventKind::TreasureOpened) {
                    state_applied = mark_treasure_opened(observed.id);
                } else if (observed.kind
                    == dswros::EventKind::EncounterDefeated) {
                    state_applied = mark_encounter_defeated(observed.id);
                }
                if (!state_applied) {
                    if (observed.kind
                        == dswros::EventKind::EncounterDefeated) {
                        clear_processed_encounter_identity(observed.id);
                        append_log("OBJECT_COMPLETION_REJECTED_NATIVE",
                            std::format(
                                "activation={} epoch={} kind={} id={} reason=completion_state_rejected",
                                activation_, epoch_,
                                static_cast<std::uint32_t>(observed.kind),
                                observed.id));
                    }
                    it = observed_objects_.erase(it);
                    continue;
                }
                append_log(observed.kind == dswros::EventKind::TreasureOpened
                    ? "TREASURE_OPENED_NATIVE" : "ENCOUNTER_DEFEATED_NATIVE",
                    std::format(
                        "activation={} epoch={} id={} encounter_type={} evidence={}",
                        activation_, epoch_, observed.id,
                        observed.kind == dswros::EventKind::EncounterDefeated
                            ? encounter_type_for_id(observed.id) : "none",
                        observed.recovered_from_end_play
                            ? "destroyed_exact_unique_class_player_to_actor_proximity_current_available"
                            : observed.kind == dswros::EventKind::EncounterDefeated
                                ? "stable_visible_encounter_near_last_actor_then_forty_missing_samples"
                                : "positive_observation_then_two_missing_samples"));
                it = observed_objects_.erase(it);
            } else if (!inside && !grace_active) {
                append_log("OBJECT_EVICTED_NATIVE", std::format(
                    "activation={} epoch={} kind={} id={} reason=departure_grace_expired present={}",
                    activation_, epoch_, static_cast<std::uint32_t>(observed.kind), observed.id, present));
                if (observed.kind
                    == dswros::EventKind::EncounterDefeated) {
                    clear_processed_encounter_identity(observed.id);
                }
                it = observed_objects_.erase(it);
            } else ++it;
        }
    }

    [[nodiscard]] bool read_actor_hidden(UObject* actor, bool* output) {
        if (!actor || !output || !is_hidden_function_) return false;
        struct Parameters { bool return_value{}; } parameters{};
        actor->ProcessEvent(is_hidden_function_, &parameters);
        *output = parameters.return_value;
        return true;
    }

    void capture_world_clock() noexcept {
        clock_capture_attempted_ = true;
        const auto started = Clock::now();
        if (!capture_world_clock_guarded()) {
            world_time_available_ = false;
            append_log("WORLD_CLOCK_UNAVAILABLE", "reason=structured_exception retry=next_f7");
        }
        const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - started).count();
        append_log("WORLD_CLOCK_CAPTURE_PERF", std::format(
            "available={} elapsed_us={} retry=next_f7", world_time_available_, elapsed_us));
    }

    [[nodiscard]] bool capture_world_clock_guarded() noexcept {
#if defined(_MSC_VER)
        __try {
            capture_world_clock_unsafe();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        try {
            capture_world_clock_unsafe();
            return true;
        }
        catch (...) {
            return false;
        }
#endif
    }

    void capture_world_clock_unsafe() {
        UObject* singleton = UObjectGlobals::FindFirstOf(STR("DGameSingleton"));
        if (!singleton) return;
        auto* property = CastField<FNumericProperty>(
            singleton->GetPropertyByNameInChain(STR("TimeOfDay")));
        void* value = singleton->GetValuePtrByPropertyNameInChain(STR("TimeOfDay"));
        if (!property || !value) return;
        const double seconds = property->IsFloatingPoint()
            ? property->GetFloatingPointPropertyValue(value)
            : static_cast<double>(property->GetSignedIntPropertyValue(value));
        if (!std::isfinite(seconds)) return;
        auto normalized = static_cast<std::int64_t>(std::floor(seconds)) % kSecondsPerDay;
        if (normalized < 0) normalized += kSecondsPerDay;
        world_time_baseline_seconds_ = normalized;
        world_time_baseline_at_ = Clock::now();
        world_time_available_ = true;
    }

    [[nodiscard]] std::uint32_t current_world_time_seconds() const noexcept {
        if (!world_time_available_) return 0U;
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            Clock::now() - world_time_baseline_at_).count();
        return static_cast<std::uint32_t>((world_time_baseline_seconds_
            + elapsed * kGameSecondsPerRealSecond) % kSecondsPerDay);
    }

    void quest_blueprint_end_pre(
        UnrealScriptFunctionCallableContext& context,
        bool renew) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !area_quest_task_class_map_ready_) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            quest_blueprint_end_pre_unsafe(context, renew);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
        }
#else
        try {
            quest_blueprint_end_pre_unsafe(context, renew);
        } catch (...) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
        }
#endif
    }

    void quest_blueprint_end_pre_unsafe(
        UnrealScriptFunctionCallableContext& context,
        bool renew) {
        auto* property = renew
            ? renew_quest_blueprint_end_actor_property_
            : quest_blueprint_end_actor_property_;
        auto* locals = context.TheStack.Locals();
        void* actor_value = property && locals
            ? property->ContainerPtrToValuePtr<void>(locals) : nullptr;
        UObject* task_actor = actor_value
            ? property->GetObjectPropertyValue(actor_value) : nullptr;
        UClass* task_class = task_actor
            ? task_actor->GetClassPrivate() : nullptr;
        const std::string class_name = task_class
            ? to_string(task_class->GetFullName()) : std::string{};
        const auto found = area_quest_task_class_indices_.find(class_name);
        if (found == area_quest_task_class_indices_.end()
            || found->second >= area_quest_catalog_.size()) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
            return;
        }
        const bool armed = arm_area_quest_completion_witness(
            found->second, true,
            renew ? "renew_quest_blueprint_end_actor"
                  : "quest_blueprint_end_actor");
        try {
            append_log("AREA_QUEST_END_ACTOR", std::format(
                "activation={} epoch={} hook={} id={} catalog_index={} witness_armed={} action=exact_id_verification_window",
                activation_, epoch_, renew ? "renew_quest_end" : "quest_end",
                area_quest_catalog_[found->second].id, found->second,
                armed));
        } catch (...) {
        }
    }

    void quest_event_trigger_pre(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !quest_event_trigger_schema_ready_) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            quest_event_trigger_pre_unsafe(context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#else
        try {
            quest_event_trigger_pre_unsafe(context);
        } catch (...) {
        }
#endif
    }

    void quest_event_trigger_pre_unsafe(
        UnrealScriptFunctionCallableContext& context) {
        auto* locals = context.TheStack.Locals();
        if (!locals) {
            return;
        }
        void* id_value = quest_event_id_property_
            ->ContainerPtrToValuePtr<void>(locals);
        void* step_id_value = quest_event_step_id_property_
            ->ContainerPtrToValuePtr<void>(locals);
        void* step_count_value = quest_event_step_count_property_
            ->ContainerPtrToValuePtr<void>(locals);
        void* is_set_value = quest_event_is_set_property_
            ->ContainerPtrToValuePtr<void>(locals);
        void* dynamic_value = quest_event_dynamic_property_
            ->ContainerPtrToValuePtr<void>(locals);
        void* world_context_value = quest_event_world_context_property_
            ->ContainerPtrToValuePtr<void>(locals);
        if (!id_value || !step_id_value || !step_count_value
            || !is_set_value || !dynamic_value
            || !world_context_value
            || !quest_event_dynamic_property_->GetPropertyValue(
                dynamic_value)) {
            return;
        }
        const std::int64_t quest_id =
            quest_event_id_property_->GetSignedIntPropertyValue(id_value);
        const auto index = area_quest_catalog_index(quest_id);
        if (!index) {
            return;
        }
        // The dynamic flag and exact 147-entry catalog ID already prove which
        // task emitted this event. Delivery-style quests can advance to END
        // before either the live or published state can still expose
        // PROGRESS, so requiring that pre-event sample drops the only exact
        // completion witness. Arming is not completion: the bounded witness
        // still accepts only END for this exact numeric ID, then falls back to
        // a bounded positive-only save confirmation.
        const bool armed = arm_area_quest_completion_witness(
            *index, true, "native_dynamic_event_exact_catalog_id");
        if (!armed) {
            area_quest_rescan_requests_.fetch_add(
                1, std::memory_order_release);
        }
        try {
            append_log("AREA_QUEST_EVENT_TRIGGER", std::format(
                "activation={} epoch={} id={} catalog_index={} step={} count={} is_set={} exact_catalog_identity=true progress_precondition=not_required witness_armed={} completion_policy=exact_id_end_only action=coalesced_transactional_refresh",
                activation_, epoch_, quest_id, *index,
                quest_event_step_id_property_->GetSignedIntPropertyValue(
                    step_id_value),
                quest_event_step_count_property_->GetSignedIntPropertyValue(
                    step_count_value),
                quest_event_is_set_property_->GetPropertyValue(
                    is_set_value),
                armed));
        } catch (...) {
        }
    }

    void task_complete_post(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !context.Context) {
            return;
        }
        // The exact class map provides immediate identity. A coalesced numeric
        // refresh still follows every real completion so repeatable quests can
        // become active again without retaining the task actor.
        area_quest_rescan_requests_.fetch_add(
            1, std::memory_order_release);
#if defined(_MSC_VER)
        __try {
            task_complete_post_unsafe(context.Context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
        }
#else
        try {
            task_complete_post_unsafe(context.Context);
        } catch (...) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
        }
#endif
    }

    void task_complete_post_unsafe(UObject* task_actor) {
        UClass* task_class = task_actor
            ? task_actor->GetClassPrivate() : nullptr;
        const std::string class_name = task_class
            ? to_string(task_class->GetFullName()) : std::string{};
        const auto found = area_quest_task_class_map_ready_
            ? area_quest_task_class_indices_.find(class_name)
            : area_quest_task_class_indices_.end();
        if (found == area_quest_task_class_indices_.end()
            || found->second >= kExpectedAreaQuestCount) {
            area_quest_unmapped_completion_requests_.fetch_add(
                1, std::memory_order_release);
            return;
        }
        const std::size_t word = found->second / 64U;
        const std::uint64_t bit = 1ULL << (found->second % 64U);
        area_quest_exact_completion_bits_[word].fetch_or(
            bit, std::memory_order_release);
    }

    void encounter_death_process_pre(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !context.Context || activity_suppressed_ || !position_valid_
            || !encounter_state_ready_ || !monster_character_class_
            || !encounter_death_process_schema_ready_) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            encounter_death_process_pre_unsafe(context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        try {
            encounter_death_process_pre_unsafe(context);
        } catch (...) {}
#endif
    }

    void encounter_death_process_pre_unsafe(
        UnrealScriptFunctionCallableContext& context) {
        void* locals = context.TheStack.Locals();
        void* state_value = locals && encounter_death_process_property_
            ? encounter_death_process_property_
                ->ContainerPtrToValuePtr<void>(locals)
            : nullptr;
        if (!state_value
            || !dswros::encounter_death_process_is_terminal(
                encounter_death_process_underlying_property_
                    ->GetSignedIntPropertyValue(state_value))) {
            return;
        }
        encounter_death_pre_unsafe(context.Context, true);
    }

    void encounter_death_pre(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !context.Context || activity_suppressed_ || !position_valid_
            || !encounter_state_ready_ || !monster_character_class_) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            encounter_death_pre_unsafe(context.Context, false);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        try {
            encounter_death_pre_unsafe(context.Context, false);
        } catch (...) {}
#endif
    }

    void encounter_death_pre_unsafe(
        UObject* actor, bool death_process_end) {
        FWeakObjectPtr weak{};
        weak = actor;
        const dswros::WeakIdentity identity{
            weak.ObjectIndex, weak.ObjectSerialNumber};
        if (!identity.valid()) {
            return;
        }
        const auto observed_entry =
            observed_objects_.find(identity.packed());
        if (observed_entry == observed_objects_.end()) {
            return;
        }
        auto& observed = observed_entry->second;
        const bool exact_observed_actor = observed.kind
                == dswros::EventKind::EncounterDefeated
            && observed.activation == activation_
            && observed.epoch == epoch_
            && !observed.logical_end
            && observed.weak.Get() == actor;
        if (!exact_observed_actor) {
            return;
        }

        UClass* object_class = actor->GetClassPrivate();
        const bool monster_character = object_class
            && actor->IsA(monster_character_class_);
        const std::uint64_t class_name_key = object_class
            ? DSNWRPR_CLASS_NAME_KEY(object_class) : 0;
        const auto class_entry = std::find(
            encounter_class_name_keys_.begin(),
            encounter_class_name_keys_.end(), class_name_key);
        const std::size_t encounter_index = static_cast<std::size_t>(
            class_entry - encounter_class_name_keys_.begin());
        const bool exact_catalog_class = class_name_key != 0
            && class_entry != encounter_class_name_keys_.end()
            && encounter_index < encounter_catalog_.size()
            && encounter_catalog_[encounter_index].id == observed.id;

        dswros::Position actor_position{};
        const bool actor_position_valid =
            read_actor_position(actor, &actor_position);
        const bool player_near_actor = actor_position_valid
            && distance_squared(player_, actor_position)
                <= kEncounterObservationRadius
                    * kEncounterObservationRadius;
        const EncounterSpec* spec = exact_catalog_class
            ? &encounter_catalog_[encounter_index] : nullptr;
        const dswros::EncounterDeathNotificationEvidence evidence{
            !transition_active_ && !activity_suppressed_
                && position_valid_
                && observed.activation == activation_
                && observed.epoch == epoch_,
            spec && encounter_available(*spec, unix_seconds()),
            exact_observed_actor,
            exact_catalog_class,
            monster_character,
            observed.visible_seen,
            player_near_actor};
        if (!dswros::accept_encounter_death_notification(evidence)) {
            return;
        }

        // The native death callback only publishes a fixed numeric bit. The
        // existing 250 ms control service owns cooldown mutation, marker
        // invalidation, atlas work, and logging outside the game callback.
        observed.position = actor_position;
        auto& pending_mask = death_process_end
            ? pending_encounter_death_process_mask_
            : pending_encounter_death_mask_;
        pending_mask.fetch_or(
            1ULL << encounter_index, std::memory_order_release);
    }

    void consume_pending_encounter_deaths(
        bool lifecycle_boundary = false) noexcept {
        // Do not exchange the fixed mask until its existing 250 ms owner can
        // apply it. A transient menu/Pawn/lifecycle edge therefore cannot
        // erase accepted evidence. Authoritative reset edges call with true
        // before clearing activation-local state.
        const dswros::PendingEncounterDeathConsumptionContext context{
            lifecycle_boundary,
            enabled_,
            encounter_state_ready_,
            transition_active_,
            activity_suppressed_,
            position_valid_};
        if (!dswros::can_consume_pending_encounter_death(context)) {
            return;
        }
        const std::uint64_t notify_pending =
            pending_encounter_death_mask_.load(std::memory_order_acquire);
        const std::uint64_t process_pending =
            pending_encounter_death_process_mask_.load(
                std::memory_order_acquire);
        const std::uint64_t pending = notify_pending | process_pending;
        if (pending == 0) {
            return;
        }
        for (std::size_t index = 0;
             index < encounter_catalog_.size() && index < 64U; ++index) {
            if ((pending & (1ULL << index)) == 0) {
                continue;
            }
            const auto& spec = encounter_catalog_[index];
            const std::uint64_t bit = 1ULL << index;
            const bool death_process_end = (process_pending & bit) != 0;
            const char* evidence = death_process_end
                ? "exact_observed_net_multicast_death_process_end"
                : "exact_observed_net_multicast_notify_death";
            try {
                if (!apply_encounter_defeat_state(
                        spec,
                        lifecycle_boundary,
                        evidence)) {
                    if (!lifecycle_boundary) {
                        clear_processed_encounter_identity(spec.id);
                    }
                    try {
                        append_log("OBJECT_COMPLETION_REJECTED_NATIVE",
                            std::format(
                                "activation={} epoch={} kind={} id={} reason=death_event_cooldown_already_applied",
                                activation_, epoch_,
                                static_cast<std::uint32_t>(
                                    dswros::EventKind::EncounterDefeated),
                                spec.id));
                    } catch (...) {
                    }
                } else {
                    if (!lifecycle_boundary) {
                        for (auto observed = observed_objects_.begin();
                             observed != observed_objects_.end();) {
                            if (observed->second.kind
                                    == dswros::EventKind::EncounterDefeated
                                && observed->second.id == spec.id) {
                                observed = observed_objects_.erase(observed);
                            } else {
                                ++observed;
                            }
                        }
                        clear_processed_encounter_identity(spec.id);
                    }
                    try {
                        append_log("ENCOUNTER_DEFEATED_NATIVE", std::format(
                            "activation={} epoch={} id={} encounter_type={} evidence={} lifecycle_boundary={}",
                            activation_, epoch_, spec.id,
                            encounter_kind_name(spec.kind),
                            evidence,
                            lifecycle_boundary));
                    } catch (...) {
                    }
                }
                // Clear only this successfully applied or definitively
                // deduplicated catalog bit. A later bit remains pending if an
                // earlier apply faults, and a newly published different bit is
                // never erased by a whole-mask exchange.
                pending_encounter_death_mask_.fetch_and(
                    ~bit, std::memory_order_acq_rel);
                pending_encounter_death_process_mask_.fetch_and(
                    ~bit, std::memory_order_acq_rel);
            } catch (...) {
                // The bit intentionally remains set. A normal 250 ms service
                // or the next authoritative boundary can retry it without
                // retaining an Actor, Pawn, Canvas, or other UObject.
                try {
                    append_log("ENCOUNTER_DEATH_HANDOFF_RETAINED", std::format(
                        "activation={} epoch={} id={} reason=state_apply_exception",
                        activation_, epoch_, spec.id));
                } catch (...) {
                }
                return;
            }
        }
    }

    void treasure_interact_pre(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !context.Context || !treasure_interact_schema_ready_
            || !engine_tick_engine_) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            treasure_interact_pre_unsafe(context);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        try {
            treasure_interact_pre_unsafe(context);
        } catch (...) {}
#endif
    }

    void treasure_interact_pre_unsafe(
        UnrealScriptFunctionCallableContext& context) {
        auto* locals = context.TheStack.Locals();
        void* actor_value = locals && treasure_interact_actor_property_
            ? treasure_interact_actor_property_
                ->ContainerPtrToValuePtr<void>(locals)
            : nullptr;
        UObject* interacting_actor = actor_value
            ? treasure_interact_actor_property_
                ->GetObjectPropertyValue(actor_value)
            : nullptr;
        UObject* local_pawn = current_player_pawn(engine_tick_engine_);
        if (!interacting_actor || !local_pawn) {
            queue_rejected_treasure_interaction_confirmation(
                context.Context, interacting_actor, local_pawn);
            return;
        }
        if (interacting_actor != local_pawn) {
            UObject* current_rider{};
            if (auto** rider_value =
                    local_pawn->GetValuePtrByPropertyNameInChain<UObject*>(
                        STR("Rider"))) {
                current_rider = *rider_value;
            }
            auto* treasure_class = context.Context->GetClassPrivate();
            const std::string treasure_class_name = treasure_class
                ? to_string(treasure_class->GetName()) : std::string{};
            auto* pawn_world = local_pawn->GetWorld();
            const bool rider_world_matches = current_rider
                && pawn_world
                && current_rider->GetWorld() == pawn_world;
            dswros::Position treasure_position{};
            const bool exact_receiver_near_local_player = position_valid_
                && read_actor_position(
                    context.Context, &treasure_position)
                && distance_squared(player_, treasure_position)
                    <= kTreasureObservationRadius
                        * kTreasureObservationRadius;
            if (!dswros::accept_mounted_treasure_interactor(
                    is_mount_treasure_class(treasure_class_name),
                    interacting_actor == current_rider,
                    rider_world_matches,
                    exact_receiver_near_local_player)) {
                queue_rejected_treasure_interaction_confirmation(
                    context.Context, interacting_actor, local_pawn);
                return;
            }
            treasure_completion_event_unsafe(
                context.Context,
                "local_mounted_rider_net_multi_execute_interact_prop");
            return;
        }
        treasure_completion_event_unsafe(
            context.Context, "local_net_multi_execute_interact_prop");
    }

    void treasure_death_pre(
        UnrealScriptFunctionCallableContext& context) noexcept {
        if (!enabled_ || transition_active_ || !game_thread()
            || !context.Context) {
            return;
        }
#if defined(_MSC_VER)
        __try {
            treasure_completion_event_unsafe(
                context.Context, "set_death_process");
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
#else
        try {
            treasure_completion_event_unsafe(
                context.Context, "set_death_process");
        } catch (...) {}
#endif
    }

    [[nodiscard]] std::optional<std::int64_t>
    resolve_treasure_actor_id(
        UObject* actor,
        const std::string& class_name,
        const dswros::Position& position,
        dswros::WeakIdentity identity,
        const char** identity_source) {
        if (!actor || !identity.valid()) return std::nullopt;
        auto* object_id = actor
            ->GetValuePtrByPropertyNameInChain<std::int32_t>(
                STR("ObjectID"));
        if (object_id && *object_id > 0) {
            auto exact = tracker_.observe_reported_id(
                identity, class_name, position,
                static_cast<std::int64_t>(*object_id));
            if (exact && identity_source) {
                *identity_source = "object_id_class_3d";
            }
            return exact;
        }
        const auto observed = observed_objects_.find(identity.packed());
        if (observed != observed_objects_.end()
            && observed->second.kind == dswros::EventKind::TreasureOpened
            && observed->second.activation == activation_
            && observed->second.epoch == epoch_) {
            if (identity_source) *identity_source = "observed_weak_identity";
            return observed->second.id;
        }
        auto spatial = tracker_.observe(identity, class_name, position);
        if (spatial && identity_source) {
            *identity_source = "unique_class_3d_fallback";
        }
        return spatial;
    }

    void queue_rejected_treasure_interaction_confirmation(
        UObject* treasure_actor,
        UObject* interacting_actor,
        UObject* local_pawn) {
        std::string interactor_class{"null"};
        std::string interactor_name{"null"};
        std::string pawn_class{"null"};
        std::string pawn_name{"null"};
        if (interacting_actor) {
            if (auto* object_class = interacting_actor->GetClassPrivate()) {
                interactor_class = to_string(object_class->GetName());
            }
            interactor_name = to_string(interacting_actor->GetName());
        }
        if (local_pawn) {
            if (auto* object_class = local_pawn->GetClassPrivate()) {
                pawn_class = to_string(object_class->GetName());
            }
            pawn_name = to_string(local_pawn->GetName());
        }

        std::int64_t save_id{};
        const char* identity_source = "none";
        bool exact_receiver_near_local_player{};
        bool queued{};
        if (interacting_actor && treasure_actor && position_valid_) {
            auto* object_class = treasure_actor->GetClassPrivate();
            const std::string class_name = object_class
                ? to_string(object_class->GetName()) : std::string{};
            dswros::Position position{};
            if (is_treasure_class(class_name)
                && read_actor_position(treasure_actor, &position)
                && distance_squared(player_, position)
                    <= kTreasureObservationRadius
                        * kTreasureObservationRadius) {
                exact_receiver_near_local_player = true;
                FWeakObjectPtr weak{};
                weak = treasure_actor;
                const dswros::WeakIdentity identity{
                    weak.ObjectIndex, weak.ObjectSerialNumber};
                if (const auto resolved = resolve_treasure_actor_id(
                        treasure_actor, class_name, position, identity,
                        &identity_source)) {
                    save_id = *resolved;
                    if (const auto index =
                            treasure_render_catalog_index(save_id)) {
                        queued = queue_treasure_save_confirmation(
                            *index,
                            "non_pawn_interactor_exact_nearby_receiver");
                    }
                }
            }
        }
        append_log("TREASURE_EVENT_REJECTED", std::format(
            "activation={} epoch={} evidence=net_multi_execute_interact_prop reason=interactor_is_not_fresh_local_pawn interactor_class={} interactor_name={} pawn_class={} pawn_name={} exact_receiver_near_local_player={} id={} identity_source={} delayed_positive_only_confirmation_queued={}",
            activation_, epoch_, interactor_class, interactor_name,
            pawn_class, pawn_name, exact_receiver_near_local_player,
            save_id, identity_source, queued));
    }

    void treasure_completion_event_unsafe(
        UObject* actor, const char* evidence) {
        auto* object_class = actor->GetClassPrivate();
        if (!object_class) return;
        const std::string class_name = to_string(object_class->GetName());
        dswros::Position position{};
        if (!is_treasure_class(class_name)
            || !read_actor_position(actor, &position)) {
            return;
        }
        FWeakObjectPtr weak{};
        weak = actor;
        const dswros::WeakIdentity identity{weak.ObjectIndex, weak.ObjectSerialNumber};
        const char* identity_source = "none";
        const auto id = resolve_treasure_actor_id(
            actor, class_name, position, identity, &identity_source);
        if (!id) return;
        const bool visibility_changed = mark_treasure_opened(*id);
        observed_objects_.erase(identity.packed());
        static_cast<void>(tracker_.end(identity, false, transition_active_, player_, activation_, epoch_));
        if (visibility_changed) {
            append_log("TREASURE_OPENED_NATIVE", std::format(
                "activation={} epoch={} id={} evidence={} identity=exact_treasure_actor_receiver source={} visibility_changed=true",
                activation_, epoch_, *id,
                evidence ? evidence : "unknown_treasure_actor_event",
                identity_source));
        } else {
            append_log("TREASURE_EVENT_NO_VISIBLE_TRANSITION", std::format(
                "activation={} epoch={} id={} evidence={} identity_source={} reason=already_hidden_or_not_currently_eligible",
                activation_, epoch_, *id,
                evidence ? evidence : "unknown_treasure_actor_event",
                identity_source));
        }
    }

    [[nodiscard]] static double observation_radius(dswros::EventKind kind) noexcept {
        return kind == dswros::EventKind::TreasureOpened
            ? kTreasureObservationRadius : kEncounterObservationRadius;
    }

    [[nodiscard]] static bool is_treasure_class(const std::string& name) {
        static const std::unordered_set<std::string> names{
            "TreasureBox01_C", "TreasureBox02_C", "TreasureBox02_Mount_C", "TreasureBox03_C",
            "TreasureBox03_Mount_C", "TreasureBox03_OnlyFront_C", "TreasureBox04Key_C",
            "TreasureBox04_C", "TreasureBox05_C", "TreasureBox05_Mount_C", "TreasureBox06_C"};
        return names.contains(name);
    }

    [[nodiscard]] static bool is_bird_egg_class(
        const std::string& name) noexcept {
        return name == "Bird_Egg01_C" || name == "Bird_Egg02_C";
    }

    [[nodiscard]] static bool is_mount_treasure_class(
        const std::string& name) {
        static const std::unordered_set<std::string> names{
            "TreasureBox02_Mount_C",
            "TreasureBox03_Mount_C",
            "TreasureBox05_Mount_C"};
        return names.contains(name);
    }

    [[nodiscard]] bool is_target_class(const std::string& name) const {
        if (is_treasure_class(name)) return true;
        return encounter_class_indices_.contains(name);
    }

    void unregister_callbacks() noexcept {
#if !defined(DSNWRPR_UE4SS_STABLE_ROOT)
        // Stop the high-frequency/global ingress before unregistering the
        // individual UFunction hooks or detaching any UMG tree.
        for (auto* id : {&engine_tick_id_, &begin_play_id_, &end_play_id_,
                         &transition_pre_id_, &transition_post_id_}) {
            if (*id != Hook::ERROR_ID) {
                try {
                    Hook::UnregisterCallback(*id);
                } catch (...) {
                }
                *id = Hook::ERROR_ID;
            }
        }
#endif
        if (quest_event_trigger_hook_registered_
            && quest_event_trigger_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    quest_event_trigger_function_,
                    quest_event_trigger_hook_ids_);
            } catch (...) {}
            quest_event_trigger_hook_registered_ = false;
        }
        if (task_complete_hook_registered_ && task_complete_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    task_complete_function_, task_complete_hook_ids_);
            } catch (...) {}
            task_complete_hook_registered_ = false;
        }
        if (renew_quest_blueprint_end_hook_registered_
            && renew_quest_blueprint_end_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    renew_quest_blueprint_end_function_,
                    renew_quest_blueprint_end_hook_ids_);
            } catch (...) {}
            renew_quest_blueprint_end_hook_registered_ = false;
        }
        if (quest_blueprint_end_hook_registered_
            && quest_blueprint_end_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    quest_blueprint_end_function_,
                    quest_blueprint_end_hook_ids_);
            } catch (...) {}
            quest_blueprint_end_hook_registered_ = false;
        }
        if (world_map_zoom_hook_registered_ && world_map_zoom_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    world_map_zoom_function_, world_map_zoom_hook_ids_);
            } catch (...) {}
            world_map_zoom_hook_registered_ = false;
        }
        if (world_map_image_hook_registered_ && world_map_image_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    world_map_image_function_, world_map_image_hook_ids_);
            } catch (...) {}
            world_map_image_hook_registered_ = false;
        }
        if (encounter_death_hook_registered_
            && encounter_death_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    encounter_death_function_, encounter_death_hook_ids_);
            } catch (...) {}
            encounter_death_hook_registered_ = false;
        }
        if (encounter_death_process_hook_registered_
            && encounter_death_process_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    encounter_death_process_function_,
                    encounter_death_process_hook_ids_);
            } catch (...) {}
            encounter_death_process_hook_registered_ = false;
        }
        if (treasure_death_hook_registered_ && treasure_death_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    treasure_death_function_, treasure_death_hook_ids_);
            } catch (...) {}
            treasure_death_hook_registered_ = false;
        }
        if (treasure_interact_hook_registered_
            && treasure_interact_function_) {
            try {
                UObjectGlobals::UnregisterHook(
                    treasure_interact_function_,
                    treasure_interact_hook_ids_);
            } catch (...) {}
            treasure_interact_hook_registered_ = false;
        }
    }

    void unregister_object_create_listener(
        bool wait_for_completion = false) noexcept {
        bool expected = false;
        if (!object_create_listener_removal_claimed_.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
            if (wait_for_completion) {
                while (!object_create_listener_removal_complete_.load(
                    std::memory_order_acquire)) {
                    SwitchToThread();
                }
            }
            return;
        }
        if (object_create_listener_registered_.load(
                std::memory_order_acquire)) {
            UObjectArray::RemoveUObjectCreateListener(this);
            object_create_listener_registered_.store(
                false, std::memory_order_release);
        }
        object_create_listener_removal_complete_.store(
            true, std::memory_order_release);
    }

    void wait_for_object_create_listener_callbacks() noexcept {
        while (object_create_listener_in_flight_.load(
            std::memory_order_acquire) != 0U) {
            SwitchToThread();
        }
    }

    inline static std::atomic<NativeObjectState*> instance_{};
    inline static std::atomic<std::uint64_t> next_instance_generation_{};
    const std::uint64_t instance_generation_{};
    dswros::ObjectStateTracker tracker_{};
    dswros::CompactRenderModel compact_render_model_{};
    std::array<std::uint8_t, kMaximumTreasureCatalogEntries>
        compact_eligibility_{};
    // Populated once from the validated install-time catalog, then read-only.
    std::array<dswros::CompactTreasureCatalogEntry,
               kMaximumTreasureCatalogEntries>
        render_catalog_entries_{};
    std::array<dswros::CompactTreasureMarker,
               dsnwr::kCompactUmgMarkerCapacity>
        compact_selected_markers_{};
    dsnwr::CompactUmgMarkerArray compact_umg_markers_{};
    dsnwr::WorldMapUmgMarkerArray world_map_umg_markers_{};
    FWeakObjectPtr world_map_layer_candidate_{};
    FWeakObjectPtr world_map_listener_candidate_{};
    FWeakObjectPtr compact_layer_candidate_{};
    FWeakObjectPtr compact_listener_candidate_{};
    std::unordered_set<std::int64_t> ignored_treasure_ids_{};
    std::unordered_set<std::int64_t> runtime_opened_treasure_ids_{};
    std::vector<EncounterSpec> encounter_catalog_{};
    std::unordered_map<std::string, std::size_t>
        encounter_class_indices_{};
    std::array<std::uint64_t, kExpectedEncounterCount>
        encounter_class_name_keys_{};
    std::array<std::uint64_t, 2> bird_egg_class_name_keys_{};
    std::vector<MiniGameSpec> mini_game_catalog_{};
    std::vector<AreaQuestSpec> area_quest_catalog_{};
    std::array<AreaQuestHeightDiagnosticSelection,
               kAreaQuestHeightDiagnosticSlotCapacity>
        area_quest_height_diagnostic_selections_{};
    std::array<AreaQuestHeightDiagnosticState,
               kAreaQuestHeightDiagnosticSlotCapacity>
        area_quest_height_diagnostic_states_{};
    std::array<std::uint8_t, kExpectedMiniGameCount>
        mini_game_eligibility_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_eligibility_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_scan_eligibility_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_world_map_eligibility_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_save_completion_{};
    std::array<std::int64_t, kExpectedAreaQuestCount>
        area_quest_save_completion_counts_{};
    std::array<std::uint64_t, kAreaQuestCompletionWordCount>
        area_quest_save_confirmation_pending_{};
    std::array<std::uint64_t, kAreaQuestCompletionWordCount>
        area_quest_save_confirmation_inflight_{};
    std::array<Clock::time_point, kExpectedAreaQuestCount>
        area_quest_save_confirmation_due_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_save_confirmation_attempts_{};
    std::array<std::uint64_t, kTreasureSaveConfirmationWordCount>
        treasure_save_confirmation_pending_{};
    std::array<std::uint64_t, kTreasureSaveConfirmationWordCount>
        treasure_save_confirmation_inflight_{};
    std::array<Clock::time_point, kMaximumTreasureCatalogEntries>
        treasure_save_confirmation_due_{};
    std::array<std::uint8_t, kMaximumTreasureCatalogEntries>
        treasure_save_confirmation_attempts_{};
    std::array<dswros::AreaQuestState, kExpectedAreaQuestCount>
        area_quest_states_{};
    std::array<dswros::AreaQuestState, kExpectedAreaQuestCount>
        area_quest_scan_states_{};
    std::array<dswros::AreaQuestState, kExpectedAreaQuestCount>
        area_quest_scan_previous_states_{};
    std::array<AreaQuestDefinition, kExpectedAreaQuestCount>
        area_quest_definitions_{};
    std::array<dswros::AreaQuestEligibilityProof,
               kExpectedAreaQuestCount>
        area_quest_static_proofs_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_completion_observed_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_scan_completion_observed_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_repeatable_reactivation_armed_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_scan_repeatable_reactivation_armed_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_completion_generation_locked_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_completion_generation_reactivation_armed_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_completion_witnesses_{};
    std::array<bool, kExpectedAreaQuestCount>
        area_quest_completion_witness_queued_{};
    std::array<Clock::time_point, kExpectedAreaQuestCount>
        area_quest_completion_witness_deadlines_{};
    std::array<Clock::time_point, kExpectedAreaQuestCount>
        area_quest_completion_probe_after_{};
    std::array<std::uint8_t, kExpectedAreaQuestCount>
        area_quest_completion_probe_counts_{};
    std::array<std::size_t, kExpectedAreaQuestCount>
        area_quest_completion_witness_queue_{};
    // Engine-thread revision fence. A 147-item scan snapshots the current
    // revisions; an exact completion increments one item before publication,
    // preventing an older in-flight scan from reviving that task.
    std::array<std::uint64_t, kExpectedAreaQuestCount>
        area_quest_exact_completion_revisions_{};
    std::array<std::uint64_t, kExpectedAreaQuestCount>
        area_quest_scan_start_completion_revisions_{};
    std::array<std::size_t, 5> area_quest_state_counts_{};
    std::array<std::size_t, 3> area_quest_static_proof_counts_{};
    std::array<std::uint32_t, kExpectedAreaQuestCount * 2U>
        area_quest_prerequisite_ids_{};
    std::unordered_map<std::uint32_t,
                       dswros::AreaQuestEligibilityProof>
        normal_quest_completion_proofs_{};
    std::unordered_set<std::int64_t> completed_dynamic_quest_ids_{};
    // Rebuilt once per F7 activation or travel from reflected numeric task
    // metadata. Only class names and catalog indices survive the capture; no
    // task actor, game instance, data library, or other UObject is retained.
    std::unordered_map<std::string, std::size_t>
        area_quest_task_class_indices_{};
    std::unordered_map<std::int64_t, std::int64_t>
        encounter_next_available_unix_seconds_{};
    std::int64_t next_encounter_cooldown_edge_unix_seconds_{};
    std::uint64_t encounter_visibility_mask_{};
    dsnwr::RadarVisibilityMaskWord visibility_masks_{
        dsnwr::kDefaultRadarVisibilityMasks};
    dsnwr::AreaQuestDisplayMode area_quest_display_mode_{
        dsnwr::AreaQuestDisplayMode::Available};
    dsnwr::AssaultDisplayMode assault_display_mode_{
        dsnwr::AssaultDisplayMode::Current};
    dswros::HeightIndicatorMask height_indicator_mask_{
        dswros::kDefaultHeightIndicatorMask};
    dswros::RadarLanguagePreference language_preference_{
        dswros::RadarLanguagePreference::Auto};
    dswros::RadarUiLanguage detected_game_language_{
        dswros::RadarUiLanguage::English};
    dswros::RadarUiLanguage active_ui_language_{
        dswros::RadarUiLanguage::English};
    std::unordered_map<std::uint64_t, ObservedRuntimeObject> observed_objects_{};
    std::array<FWeakObjectPtr, kExpectedEncounterCount>
        created_encounter_candidates_{};
    std::array<std::uint64_t, kExpectedEncounterCount>
        created_encounter_processed_identity_{};
    std::array<std::uint32_t, kExpectedEncounterCount>
        created_encounter_processed_activation_{};
    std::array<FWeakObjectPtr, kBirdEggCandidateCapacity>
        created_bird_egg_candidates_{};
    std::array<BirdEggRuntimeCandidate, kBirdEggCandidateCapacity>
        bird_egg_runtime_candidates_{};
    std::size_t encounter_candidate_probe_cursor_{};
    std::size_t bird_egg_candidate_probe_cursor_{};
    std::size_t bird_egg_unresolved_count_{};
    UFunction* location_function_{};
    UFunction* is_hidden_function_{};
    FObjectPropertyBase* bird_egg_interact_component_property_{};
    FEnumProperty* bird_egg_interactable_value_property_{};
    FNumericProperty* bird_egg_interactable_value_underlying_property_{};
    FEnumProperty* bird_egg_interact_type_property_{};
    FNumericProperty* bird_egg_interact_type_underlying_property_{};
    UFunction* treasure_interact_function_{};
    UFunction* treasure_death_function_{};
    UFunction* encounter_death_function_{};
    UFunction* encounter_death_process_function_{};
    UFunction* world_map_image_function_{};
    UFunction* world_map_zoom_function_{};
    UFunction* widget_is_visible_function_{};
    UFunction* is_game_paused_function_{};
    UFunction* current_language_function_{};
    UFunction* quest_info_function_{};
    UFunction* quest_blueprint_end_function_{};
    UFunction* renew_quest_blueprint_end_function_{};
    UFunction* task_complete_function_{};
    UFunction* quest_event_trigger_function_{};
    UClass* world_map_layer_class_{};
    UClass* world_map_panel_class_{};
    UClass* compact_layer_class_{};
    UClass* game_user_settings_class_{};
    UClass* actor_class_{};
    UClass* monster_character_class_{};
    FWeakObjectPtr quest_utility_default_{};
    FWeakObjectPtr gameplay_statics_default_{};
    FWeakObjectPtr internationalization_library_default_{};
    FObjectPropertyBase* quest_world_context_property_{};
    FNumericProperty* quest_id_property_{};
    FBoolProperty* quest_dynamic_property_{};
    FNumericProperty* quest_step_property_{};
    FNumericProperty* quest_current_count_property_{};
    FNumericProperty* quest_max_count_property_{};
    FEnumProperty* quest_state_property_{};
    FNumericProperty* quest_state_underlying_property_{};
    FEnumProperty* encounter_death_process_property_{};
    FNumericProperty* encounter_death_process_underlying_property_{};
    FBoolProperty* quest_return_property_{};
    FObjectPropertyBase* quest_event_world_context_property_{};
    FNumericProperty* quest_event_id_property_{};
    FNumericProperty* quest_event_step_id_property_{};
    FNumericProperty* quest_event_step_count_property_{};
    FBoolProperty* quest_event_is_set_property_{};
    FBoolProperty* quest_event_dynamic_property_{};
    FObjectPropertyBase* treasure_interact_actor_property_{};
    FBoolProperty* widget_is_visible_return_property_{};
    FObjectPropertyBase* game_pause_world_context_property_{};
    FBoolProperty* game_pause_return_property_{};
    FObjectPropertyBase* engine_game_user_settings_property_{};
    FProperty* game_language_text_property_{};
    FNumericProperty* game_language_text_numeric_property_{};
    FStrProperty* current_language_return_property_{};
    FObjectPropertyBase* quest_blueprint_end_actor_property_{};
    FObjectPropertyBase* renew_quest_blueprint_end_actor_property_{};
    FObjectPropertyBase* world_map_panel_layer_property_{};
    std::pair<int, int> treasure_interact_hook_ids_{};
    std::pair<int, int> treasure_death_hook_ids_{};
    std::pair<int, int> encounter_death_hook_ids_{};
    std::pair<int, int> encounter_death_process_hook_ids_{};
    std::pair<int, int> world_map_image_hook_ids_{};
    std::pair<int, int> world_map_zoom_hook_ids_{};
    std::pair<int, int> quest_blueprint_end_hook_ids_{};
    std::pair<int, int> renew_quest_blueprint_end_hook_ids_{};
    std::pair<int, int> task_complete_hook_ids_{};
    std::pair<int, int> quest_event_trigger_hook_ids_{};
    dswros::Position player_{};
    UEngine* engine_tick_engine_{};
    dswros::Position compact_render_anchor_{};
    Clock::time_point stable_after_{};
    Clock::time_point next_position_{};
    Clock::time_point next_discovery_{};
    Clock::time_point next_activity_probe_{};
    Clock::time_point next_runtime_visibility_edge_probe_{};
    Clock::time_point compact_rebind_after_{};
    Clock::time_point compact_attach_retry_after_{};
    Clock::time_point area_quest_rescan_after_{};
    Clock::time_point area_quest_rescan_deadline_{};
    Clock::time_point area_quest_task_class_map_retry_after_{};
    Clock::time_point visibility_hub_service_after_{};
    Clock::time_point visibility_hub_toggle_after_{};
    Clock::time_point visibility_hub_open_retry_after_{};
    Clock::time_point visibility_hub_open_pending_until_{};
    Clock::time_point compact_radius_sample_after_{};
    Clock::time_point clock_capture_after_{};
    Clock::time_point world_time_baseline_at_{};
    Clock::time_point world_map_service_retry_after_{};
    Clock::time_point world_map_layering_refresh_started_{};
    Clock::time_point world_map_layering_refresh_due_{};
    Clock::time_point save_reconcile_request_after_{};
    Clock::time_point treasure_save_confirmation_next_due_{};
    Clock::time_point area_quest_save_confirmation_next_due_{};
    Clock::time_point engine_tick_profile_report_after_{};
    Clock::time_point engine_tick_slow_log_after_{};
    std::mutex world_map_listener_mutex_{};
    std::mutex compact_listener_mutex_{};
    std::mutex created_encounter_mutex_{};
    std::mutex bird_egg_candidate_mutex_{};
    std::atomic<bool> shutting_down_{};
    std::atomic<bool> shutdown_started_{};
    std::atomic<bool> shutdown_deferred_logged_{};
    std::atomic<bool> uobject_array_shutdown_{};
    std::atomic<bool> required_runtime_ready_{};
    std::atomic<bool> object_create_listener_registered_{};
    std::atomic<bool> object_create_listener_removal_claimed_{};
    std::atomic<bool> object_create_listener_removal_complete_{};
    std::atomic<std::uint32_t> object_create_listener_in_flight_{};
    std::atomic<bool> world_map_listener_pending_{};
    std::atomic<bool> compact_listener_pending_{};
    std::atomic<std::uint32_t> f7_requests_{};
    std::atomic<std::uint32_t> f8_requests_{};
    std::atomic<std::uint32_t> f6_requests_{};
    std::atomic<std::uint32_t> area_quest_rescan_requests_{};
    std::atomic<std::uint64_t> pending_encounter_death_mask_{};
    std::atomic<std::uint64_t> pending_encounter_death_process_mask_{};
    std::atomic<std::uint64_t> bird_egg_candidate_capture_count_{};
    std::atomic<std::uint64_t> bird_egg_candidate_drop_count_{};
    std::atomic<bool> bird_egg_candidates_dirty_{};
    std::array<std::atomic<std::uint64_t>,
               kAreaQuestCompletionWordCount>
        area_quest_exact_completion_bits_{};
    std::atomic<std::uint32_t>
        area_quest_unmapped_completion_requests_{};
    std::uint64_t compact_nearest_hold_count_{};
    std::uint64_t compact_nearest_switch_count_{};
    std::uint64_t area_quest_scan_total_us_{};
    std::uint64_t area_quest_scan_max_us_{};
    std::uint64_t bird_egg_position_query_count_{};
    std::uint64_t bird_egg_discovery_total_us_{};
    std::uint64_t bird_egg_discovery_max_us_{};
    std::uint64_t bird_egg_active_change_count_{};
    std::uint64_t bird_egg_availability_query_count_{};
    std::uint64_t bird_egg_availability_unknown_count_{};
    std::uint64_t bird_egg_available_sample_count_{};
    std::uint64_t bird_egg_unavailable_sample_count_{};
    std::uint64_t bird_egg_end_event_count_{};
    std::array<EngineTickProfileMetric, kEngineTickProfileStageCount>
        engine_tick_profile_metrics_{};
    std::uint64_t engine_tick_slow_count_{};
    std::int64_t reported_compact_nearest_id_{-1};
    std::int64_t compact_stable_nearest_id_{};
    std::int64_t world_map_first_marker_id_{};
    std::int64_t world_map_last_marker_id_{};
    std::size_t render_catalog_size_{};
    std::size_t compact_marker_count_{};
    std::size_t world_map_marker_count_{};
    std::size_t world_map_treasure_marker_count_{};
    std::size_t world_map_boss_marker_count_{};
    std::size_t world_map_assault_marker_count_{};
    std::size_t world_map_fly_marker_count_{};
    std::size_t world_map_mole_marker_count_{};
    std::size_t world_map_wave_marker_count_{};
    std::size_t world_map_area_quest_marker_count_{};
    std::size_t bird_egg_active_count_{};
    std::size_t area_quest_scan_index_{};
    std::size_t area_quest_unknown_count_{};
    std::size_t area_quest_save_completion_match_count_{};
    std::size_t area_quest_definition_match_count_{};
    std::size_t area_quest_definition_unknown_condition_count_{};
    std::size_t area_quest_definition_weighted_selection_count_{};
    std::size_t area_quest_definition_monster_condition_count_{};
    std::size_t area_quest_definition_monster_link_count_{};
    std::size_t area_quest_definition_monster_ambiguous_count_{};
    std::size_t area_quest_prerequisite_query_count_{};
    std::size_t area_quest_prerequisite_fault_count_{};
    std::size_t area_quest_prerequisite_scan_index_{};
    std::size_t area_quest_prerequisite_scan_count_{};
    std::size_t area_quest_task_class_row_count_{};
    std::size_t area_quest_dynamic_task_class_row_count_{};
    std::size_t area_quest_task_class_ambiguity_count_{};
    std::size_t area_quest_completion_witness_queue_count_{};
    std::size_t reported_compact_selection_count_{static_cast<std::size_t>(-1)};
    std::uint64_t world_map_candidate_serial_{};
    std::uint64_t world_map_open_serial_{};
    std::uint64_t world_map_visible_serial_{};
    std::uint64_t world_map_serviced_serial_{};
    std::uint64_t world_map_layering_refresh_serial_{};
    std::uint64_t compact_candidate_serial_{};
    std::int32_t world_map_listener_object_index_{-1};
    std::int32_t compact_listener_object_index_{-1};
    std::uint32_t activation_{};
    std::uint32_t world_map_readiness_attempts_{};
    std::uint32_t world_map_service_attempts_{};
    std::uint32_t world_map_layering_refresh_attempt_{};
    std::uint32_t compact_attach_attempt_count_{};
    std::uint32_t area_quest_task_class_map_attempt_count_{};
    std::uint32_t area_quest_height_diagnostic_activation_{};
    std::uint32_t area_quest_height_diagnostic_event_count_{};
    std::uint32_t engine_tick_fault_recovery_attempts_{};
    std::uint32_t save_reconcile_next_request_id_{};
    std::uint32_t save_reconcile_inflight_request_id_{};
    std::uint32_t epoch_{};
    Clock::time_point world_map_open_evidence_at_{};
    double compact_render_radius_{};
    double compact_height_target_z_{};
    double compact_mole_height_target_z_{};
    std::atomic<DWORD> game_thread_id_{};
    bool catalog_ready_{};
    bool encounter_class_name_keys_ready_{};
    bool bird_egg_class_name_keys_ready_{};
    bool bird_egg_availability_schema_ready_{};
    bool enabled_{};
    bool main_menu_activation_latched_{};
    bool transition_active_{};
    bool position_valid_{};
    bool compact_attach_requested_{};
    CompactAttachGateStatus compact_attach_gate_status_{
        CompactAttachGateStatus::NotAttempted};
    bool compact_anchor_valid_{};
    bool compact_radius_valid_{};
    bool compact_rebind_dirty_{true};
    bool compact_height_target_valid_{};
    bool compact_mole_height_target_valid_{};
    bool area_quest_height_diagnostic_limit_reported_{};
    bool mouse_cursor_visible_{};
    bool world_map_compact_suppressed_{};
    bool game_paused_{};
    bool game_pause_sample_known_{};
    bool activity_suppressed_{};
    bool clock_capture_attempted_{};
    bool world_time_available_{};
    std::int64_t world_time_baseline_seconds_{};
    std::int32_t last_area_quest_world_hour_{-1};
    bool treasure_interact_hook_registered_{};
    bool treasure_death_hook_registered_{};
    bool encounter_death_hook_registered_{};
    bool encounter_death_process_hook_registered_{};
    bool encounter_death_process_schema_ready_{};
    bool treasure_interact_schema_ready_{};
    bool widget_is_visible_schema_ready_{};
    bool game_pause_schema_ready_{};
    bool current_language_schema_ready_{};
    bool game_user_settings_language_schema_ready_{};
    bool internationalization_language_schema_ready_{};
    const char* current_language_detection_source_{"fallback_en"};
    bool world_map_image_hook_registered_{};
    bool world_map_zoom_hook_registered_{};
    bool world_map_zoom_schema_ready_{};
    bool quest_blueprint_end_hook_registered_{};
    bool renew_quest_blueprint_end_hook_registered_{};
    bool task_complete_hook_registered_{};
    bool quest_event_trigger_hook_registered_{};
    bool quest_event_trigger_schema_ready_{};
    bool quest_blueprint_end_schema_ready_{};
    bool renew_quest_blueprint_end_schema_ready_{};
    bool area_quest_state_provider_ready_{};
    bool area_quest_scan_pending_{};
    bool area_quest_scan_had_published_snapshot_{};
    bool area_quest_rescan_scheduled_{};
    bool area_quest_time_rescan_scheduled_{};
    bool area_quest_scan_faulted_{};
    bool area_quest_state_ready_{};
    bool area_quest_definition_ready_{};
    bool area_quest_task_class_map_ready_{};
    bool area_quest_task_class_map_capture_pending_{};
    bool area_quest_save_completion_query_available_{};
    bool world_map_candidate_available_{};
    bool compact_candidate_available_{};
    bool world_map_session_pending_{};
    bool world_map_set_image_rearm_consumed_{};
    bool save_reconciler_ready_{};
    bool world_map_renderer_session_started_{};
    bool world_map_marker_snapshot_built_{};
    bool world_map_layer_catch_up_attempted_{};
    bool world_map_layering_refresh_pending_{};
    bool visibility_hub_world_map_refresh_pending_{};
    bool visibility_hub_world_map_baseline_valid_{};
    bool visibility_hub_open_pending_{};
    dsnwr::RadarVisibilityMaskWord visibility_hub_world_map_baseline_mask_{};
    dsnwr::AreaQuestDisplayMode
        visibility_hub_world_map_baseline_area_quest_mode_{
            dsnwr::AreaQuestDisplayMode::Available};
    dsnwr::AssaultDisplayMode
        visibility_hub_world_map_baseline_assault_mode_{
            dsnwr::AssaultDisplayMode::Current};
    WorldMapLayeringTrigger world_map_layering_refresh_trigger_{
        WorldMapLayeringTrigger::Attach};
    bool save_reconcile_requested_{};
    bool save_reconcile_completed_{};
    bool encounter_state_ready_{};
    bool encounter_visibility_mask_valid_{};
    bool treasure_eligibility_ready_{};
    bool treasure_save_confirmation_pending_armed_{};
    bool area_quest_save_confirmation_pending_armed_{};
    bool engine_tick_fault_pending_{};
    bool engine_tick_fault_was_enabled_{};
    bool engine_tick_fault_recovery_in_progress_{};
    bool engine_tick_fault_terminal_{};
    bool engine_tick_fault_cleanup_in_progress_{};
    bool engine_tick_fault_cleanup_failed_{};
    DWORD engine_tick_fault_code_{};
    dsnwr::CompactUmgRenderer compact_umg_renderer_{};
    dsnwr::WorldMapUmgRenderer world_map_umg_renderer_{};
    dsnwr::RadarVisibilityHub visibility_hub_{};
    dsnwr::NativeSaveReconciler save_reconciler_{};
    std::uint32_t compact_refresh_status_code_{0xFFFFFFFFU};
    std::uint32_t reported_compact_state_{0xFFFFFFFFU};
    std::uint32_t reported_compact_refresh_status_code_{0xFFFFFFFFU};
    std::uint32_t reported_compact_last_attach_failure_{0xFFFFFFFFU};
    std::uint32_t world_map_refresh_status_code_{0xFFFFFFFFU};
    std::uint32_t reported_world_map_state_{0xFFFFFFFFU};
    std::uint32_t reported_world_map_refresh_status_code_{0xFFFFFFFFU};
    std::uint32_t reported_world_map_last_attach_failure_{0xFFFFFFFFU};
    std::string baseline_context_key_{};
    std::string current_context_key_{};
    std::string baseline_world_key_{};
    std::string current_world_key_{};
#if defined(DSNWRPR_UE4SS_STABLE_ROOT)
    std::atomic<std::uint64_t> stable_next_pulse_ms_{};
    bool stable_process_event_registered_{};
    bool stable_begin_play_registered_{};
    bool stable_transition_callbacks_registered_{};
#else
    Hook::GlobalCallbackId engine_tick_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId begin_play_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId end_play_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId transition_pre_id_{Hook::ERROR_ID};
    Hook::GlobalCallbackId transition_post_id_{Hook::ERROR_ID};
#endif
};

} // namespace

#define DSNWRPR_API __declspec(dllexport)
extern "C" {
DSNWRPR_API RC::CppUserModBase* start_mod() {
    if (!pin_own_module_for_process_lifetime()) return nullptr;
    return new NativeObjectState();
}
DSNWRPR_API void uninstall_mod(RC::CppUserModBase* mod) {
    // The module is deliberately pinned for the process lifetime. A thread may
    // have fetched the detour address immediately before slot restoration, so
    // the owning object also remains resident until process exit.
    if (mod) static_cast<NativeObjectState*>(mod)->shutdown_for_process_lifetime();
}
}
