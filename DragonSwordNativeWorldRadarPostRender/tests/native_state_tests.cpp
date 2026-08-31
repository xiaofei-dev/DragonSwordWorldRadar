#include <dswros/area_quest_visibility.hpp>
#include <dswros/compact_render_model.hpp>
#include <dswros/diagnostics_config.hpp>
#include <dswros/diagnostic_log_format.hpp>
#include <dswros/object_state.hpp>
#include <dswros/owner_pointer_pattern.hpp>
#include <dswros/render_projection.hpp>
#include <dswros/visibility_config.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <vector>

namespace {

std::size_t allocation_count{};
std::size_t assertion_count{};

} // namespace

void* operator new(std::size_t size) {
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        ++allocation_count;
        return memory;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size) {
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        ++allocation_count;
        return memory;
    }
    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

namespace {

static_assert(
    dswros::LiveMarkerPresenceGate::kMissingDebounceMilliseconds == 400,
    "live runtime markers must retain the 400 ms disappearance debounce");

void require(bool condition, const char* message) {
    ++assertion_count;
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

bool near(double left, double right, double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

const dswros::CompactTreasureMarker* marker_by_id(
    std::span<const dswros::CompactTreasureMarker> markers,
    std::int64_t id) {
    for (const dswros::CompactTreasureMarker& marker : markers) {
        if (marker.id == id) {
            return &marker;
        }
    }
    return nullptr;
}

dswros::ObjectStateTracker tracker() {
    dswros::ObjectStateTracker value;
    value.set_catalog({
        {1001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
        {1002, "TreasureBox02_C", {8000.0, 9000.0, 1000.0}},
    });
    value.reset(7, 11);
    return value;
}

template <typename Value>
void write_pe_value(
    std::vector<std::uint8_t>& image,
    std::size_t offset,
    Value value) {
    require(offset <= image.size() && image.size() - offset >= sizeof(Value),
            "synthetic PE write must stay in bounds");
    std::memcpy(image.data() + offset, &value, sizeof(Value));
}

void write_owner_pointer_pattern(
    std::vector<std::uint8_t>& image,
    std::size_t raw_offset,
    std::uint32_t instruction_rva,
    std::uint32_t target_rva) {
    constexpr std::array<std::uint8_t, 24> pattern{
        0x48, 0x8B, 0x0D, 0, 0, 0, 0,
        0xE8, 0, 0, 0, 0,
        0x8B, 0xC7, 0x48, 0x8B, 0x5C, 0x24,
        0x40, 0x48, 0x8B, 0x6C, 0x24, 0x50};
    require(raw_offset <= image.size()
                && image.size() - raw_offset >= pattern.size(),
            "synthetic owner-pointer pattern must stay in bounds");
    std::memcpy(image.data() + raw_offset, pattern.data(), pattern.size());
    const auto displacement = static_cast<std::int32_t>(
        static_cast<std::int64_t>(target_rva)
        - static_cast<std::int64_t>(instruction_rva + 7U));
    write_pe_value(image, raw_offset + 3U, displacement);
    write_pe_value(image, raw_offset + 8U, std::int32_t{0x10});
}

std::vector<std::uint8_t> synthetic_owner_pointer_pe() {
    std::vector<std::uint8_t> image(0x800U, 0U);
    write_pe_value(image, 0U, std::uint16_t{0x5A4D});
    write_pe_value(image, 0x3CU, std::uint32_t{0x80});
    write_pe_value(image, 0x80U, std::uint32_t{0x00004550});
    write_pe_value(image, 0x84U, std::uint16_t{0x8664});
    write_pe_value(image, 0x86U, std::uint16_t{1});
    write_pe_value(image, 0x94U, std::uint16_t{0xF0});
    write_pe_value(image, 0x96U, std::uint16_t{0x0002});
    write_pe_value(image, 0x98U, std::uint16_t{0x020B});
    write_pe_value(image, 0xD0U, std::uint32_t{0x4000});
    constexpr std::size_t section = 0x188U;
    write_pe_value(image, section + 8U, std::uint32_t{0x400});
    write_pe_value(image, section + 12U, std::uint32_t{0x1000});
    write_pe_value(image, section + 16U, std::uint32_t{0x400});
    write_pe_value(image, section + 20U, std::uint32_t{0x200});
    write_pe_value(image, section + 36U, std::uint32_t{0x60000020});
    write_owner_pointer_pattern(image, 0x240U, 0x1040U, 0x3000U);
    return image;
}

} // namespace

int main() {
    {
        constexpr std::string_view current_config{
            "# Public defaults\n"
            "[radar]\n"
            "clock=true\n"
            "treasure=true\n"
            "boss=true\n"
            "assault=true\n"
            "mini_games=true\n"
            "area_quests=true\n"
            "bird_eggs=true\n"
            "[map]\n"
            "treasure=true\n"
            "boss=true\n"
            "assault=true\n"
            "mini_games=true\n"
            "area_quests=true\n"
            "[modes]\n"
            "area_quests=available\n"
            "assault=available\n"};
        const std::size_t allocations_before = allocation_count;
        const auto current = dswros::parse_visibility_config(current_config);
        require(current
                    && current.format
                        == dswros::VisibilityConfigFormat::Sectioned
                    && dswros::compact_visibility_mask(current.settings)
                        == 0x7FU
                    && dswros::world_visibility_mask(current.settings)
                        == 0x3EU,
                "the sectioned visibility defaults must parse exactly");
        require(allocation_count == allocations_before,
                "visibility parsing must not allocate");

        dswros::VisibilityConfigSettings customized{};
        customized.radar_clock = false;
        customized.radar_assault = false;
        customized.map_boss = false;
        customized.area_quest_mode =
            dswros::VisibilityAreaQuestMode::All;
        customized.assault_mode = dswros::VisibilityAssaultMode::All;
        const std::string serialized =
            dswros::format_visibility_config(customized);
        const auto round_trip =
            dswros::parse_visibility_config(serialized);
        require(round_trip
                    && !round_trip.settings.radar_clock
                    && !round_trip.settings.radar_assault
                    && !round_trip.settings.map_boss
                    && round_trip.settings.area_quest_mode
                        == dswros::VisibilityAreaQuestMode::All
                    && round_trip.settings.assault_mode
                        == dswros::VisibilityAssaultMode::All,
                "F6 visibility output must round-trip without masks");

        const auto legacy1 = dswros::parse_visibility_config(
            "; schema 1\ncompact_mask=63\nworld_mask=62\n");
        require(legacy1
                    && legacy1.format
                        == dswros::VisibilityConfigFormat::LegacySchema1
                    && dswros::compact_visibility_mask(legacy1.settings)
                        == 0x7FU,
                "schema 1 visibility must migrate the bird-egg default");
        const auto legacy2 = dswros::parse_visibility_config(
            "schema_version=2\ncompact_mask=5\nworld_mask=6\n");
        require(legacy2
                    && legacy2.format
                        == dswros::VisibilityConfigFormat::LegacySchema2
                    && dswros::compact_visibility_mask(legacy2.settings) == 5U
                    && dswros::world_visibility_mask(legacy2.settings) == 6U,
                "schema 2 visibility masks must remain readable");
        const auto legacy3 = dswros::parse_visibility_config(
            "schema_version=3\ncompact_mask=7\nworld_mask=10\n"
            "area_quest_mode=all\n");
        require(legacy3
                    && legacy3.settings.area_quest_mode
                        == dswros::VisibilityAreaQuestMode::All,
                "schema 3 area-quest mode must remain readable");
        const auto legacy4 = dswros::parse_visibility_config(
            "schema_version=4\ncompact_mask=127\nworld_mask=62\n"
            "area_quest_mode=available\nassault_mode=current\n");
        require(legacy4
                    && legacy4.format
                        == dswros::VisibilityConfigFormat::LegacySchema4
                    && legacy4.settings.assault_mode
                        == dswros::VisibilityAssaultMode::Available,
                "schema 4 current assault mode must remain readable");

        require(!dswros::parse_visibility_config(
                    "[radar]\nclock=true\n"),
                "incomplete sectioned visibility must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config} + "clock=true\n"),
                "a key in the wrong section must fail closed");
        require(!dswros::parse_visibility_config(
                    std::string{current_config} + "[unknown]\n"),
                "unknown visibility sections must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\nclock=TRUE\n"),
                "non-canonical visibility booleans must fail closed");
        require(!dswros::parse_visibility_config(
                    "compact_mask=127\nworld_mask=62\n[radar]\n"),
                "mixed legacy and sectioned visibility must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=5\ncompact_mask=127\nworld_mask=62\n"),
                "unsupported legacy visibility schemas must fail closed");
        require(!dswros::parse_visibility_config(
                    "SCHEMA_VERSION=4\ncompact_mask=127\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "uppercase legacy visibility keys must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=4\nCOMPACT_MASK=127\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "mixed-case legacy visibility keys must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=4\ncompact_mask=128\nworld_mask=62\n"
                    "area_quest_mode=available\nassault_mode=available\n"),
                "out-of-range legacy masks must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\rclock=true\n"),
                "bare carriage returns must fail closed");
        require(!dswros::parse_visibility_config(
                    "schema_version=1\ncompact_mask=63\nworld_mask=62\r"),
                "a trailing bare carriage return must fail closed");
        require(!dswros::parse_visibility_config(
                    "[radar]\r\nclock=true\n"),
                "mixed visibility line endings must fail closed");
        const auto bom_legacy = dswros::parse_visibility_config(
            "\xEF\xBB\xBFschema_version=4\ncompact_mask=127\n"
            "world_mask=62\narea_quest_mode=available\n"
            "assault_mode=available\n");
        require(bom_legacy
                    && bom_legacy.format
                        == dswros::VisibilityConfigFormat::LegacySchema4,
                "legacy UTF-8 BOM visibility must remain readable");
        require(!dswros::parse_visibility_config(
                    std::string(dswros::kMaximumVisibilityConfigBytes + 1U,
                                'x')),
                "oversized visibility input must fail closed");
    }
    {
        auto image = synthetic_owner_pointer_pe();
        const std::size_t allocations_before = allocation_count;
        const auto unique = dswros::resolve_owner_pointer_rva(image);
        require(unique.status == dswros::OwnerPointerPatternStatus::Unique
                    && unique.rva == 0x3000U
                    && unique.match_count == 1U,
                "one executable-section owner-pointer signature must resolve");
        require(allocation_count == allocations_before,
                "owner-pointer resolution must not allocate");

        auto missing = image;
        missing[0x240U] = 0U;
        require(dswros::resolve_owner_pointer_rva(missing).status
                    == dswros::OwnerPointerPatternStatus::NotFound,
                "a missing owner-pointer signature must fail closed");

        auto ambiguous = image;
        write_owner_pointer_pattern(
            ambiguous, 0x280U, 0x1080U, 0x3010U);
        const auto ambiguous_result =
            dswros::resolve_owner_pointer_rva(ambiguous);
        require(ambiguous_result.status
                    == dswros::OwnerPointerPatternStatus::Ambiguous
                    && ambiguous_result.match_count == 2U,
                "multiple owner-pointer signatures must fail closed");

        auto invalid_target = synthetic_owner_pointer_pe();
        write_owner_pointer_pattern(
            invalid_target, 0x240U, 0x1040U, 0x5000U);
        require(dswros::resolve_owner_pointer_rva(invalid_target).status
                    == dswros::OwnerPointerPatternStatus::TargetOutOfRange,
                "an out-of-image owner-pointer target must fail closed");

        image[0] = 0U;
        require(dswros::resolve_owner_pointer_rva(image).status
                    == dswros::OwnerPointerPatternStatus::InvalidImage,
                "a malformed game image must fail closed");
    }
    require(dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "# Read once at native startup.\n"
                "debug_logging = true\n")
                == std::optional<bool>{true},
            "the documented diagnostics true value must be accepted");
    require(dswros::parse_event_log_enabled(
                "; Public release default\r\n"
                "\r\n"
                "[diagnostics]\r\n"
                "debug_logging=false\r\n")
                == std::optional<bool>{false},
            "comments, blank lines, and CRLF must be accepted");
    require(dswros::parse_event_log_enabled("event_log_enabled=true\n")
                == std::optional<bool>{true},
            "the exact legacy diagnostics true value must remain accepted");
    require(dswros::parse_event_log_enabled("event_log_enabled=false\r\n")
                == std::optional<bool>{false},
            "the exact legacy diagnostics false value must remain accepted");
    require(!dswros::parse_event_log_enabled(""),
            "missing diagnostics input must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\ndebug_logging=TRUE\n"),
            "non-canonical diagnostics values must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "debug_logging=true\n"
                "debug_logging=false\n"),
            "duplicate diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\nunknown=true\n"),
            "unknown diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled("debug_logging=true\n"),
            "a sectionless current diagnostics key must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\n"
                "debug_logging=true\n"
                "event_log_enabled=true\n"),
            "mixed current and legacy diagnostics keys must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[other]\ndebug_logging=true\n"),
            "unknown diagnostics sections must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\rdebug_logging=true\n"),
            "a lone carriage return must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\r\ndebug_logging=true\n"),
            "mixed CRLF and LF diagnostics must fail closed");
    require(!dswros::parse_event_log_enabled(
                "[diagnostics]\ndebug_logging=true\r\n"),
            "mixed LF and CRLF diagnostics must fail closed in either order");
    require(!dswros::parse_event_log_enabled(
                std::string(dswros::kMaximumDiagnosticsConfigBytes + 1, 'x')),
            "oversized diagnostics input must fail closed");
    {
        std::array<char, dswros::kMaximumDiagnosticLogMetadataBytes> output{};
        const auto allocations_before = allocation_count;
        const auto written = dswros::format_diagnostic_log_metadata(
            output, 42U, 1780000000123ULL, 9876U);
        require(std::string_view{output.data(), written}
                    == " seq=42 utc_ms=1780000000123 elapsed_ms=9876",
                "diagnostic events must carry stable sequence and time fields");
        require(allocation_count == allocations_before,
                "diagnostic event metadata formatting must not allocate");
        std::array<char, 8> undersized{};
        require(dswros::format_diagnostic_log_metadata(
                    undersized, 1U, 2U, 3U) == 0U,
                "diagnostic metadata formatting must fail closed on capacity");
    }
    {
        using dswros::AreaQuestEligibilityProof;
        using dswros::AreaQuestState;
        require(!dswros::area_quest_compact_visible(AreaQuestState::None),
                "compact area quests must hide NONE");
        require(dswros::area_quest_compact_visible(AreaQuestState::Acceptable),
                "compact area quests must show ACCEPTABLE");
        require(dswros::area_quest_compact_visible(AreaQuestState::Progress),
                "compact area quests must show PROGRESS");
        require(!dswros::area_quest_compact_visible(AreaQuestState::End)
                    && !dswros::area_quest_compact_visible(
                        AreaQuestState::Fail)
                    && !dswros::area_quest_compact_visible(
                        AreaQuestState::Unknown),
                "compact area quests must fail closed for terminal or unknown states");

        require(dswros::area_quest_completion_transition(
                    AreaQuestState::Progress, AreaQuestState::End),
                "progress-to-end must prove a completed active task");
        require(!dswros::area_quest_completion_transition(
                    AreaQuestState::Progress, AreaQuestState::Fail)
                    && !dswros::area_quest_completion_transition(
                        AreaQuestState::Acceptable, AreaQuestState::End),
                "fail and non-progress transitions must not latch completion");
        require(dswros::area_quest_witnessed_completion_transition(
                    true, AreaQuestState::Fail, AreaQuestState::End)
                    && dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Acceptable,
                        AreaQuestState::End)
                    && dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::None,
                        AreaQuestState::End),
                "an exact quest identity may accept end over a stale snapshot");
        require(!dswros::area_quest_witnessed_completion_transition(
                    false, AreaQuestState::Fail, AreaQuestState::End)
                    && !dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Progress,
                        AreaQuestState::Fail)
                    && !dswros::area_quest_witnessed_completion_transition(
                        true, AreaQuestState::Progress,
                        AreaQuestState::Progress),
                "witnessed completion must require exact identity and end");
        require(dswros::area_quest_active_sample_may_rearm(0, 0, true)
                    && dswros::area_quest_active_sample_may_rearm(1, 1, true),
                "a current scan with a settled inactive boundary may rearm a repeatable task");
        require(!dswros::area_quest_active_sample_may_rearm(0, 1, true)
                    && !dswros::area_quest_active_sample_may_rearm(2, 1, true)
                    && !dswros::area_quest_active_sample_may_rearm(1, 1, false),
                "stale or not-yet-settled active samples must not overwrite exact completion");
        require(dswros::area_quest_inactive_sample_arms_reactivation(
                    AreaQuestState::None)
                    && dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::End)
                    && !dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::Fail)
                    && !dswros::area_quest_inactive_sample_arms_reactivation(
                        AreaQuestState::Progress),
                "only a proven inactive NONE or END sample may arm repeatable reactivation");
        bool completion_latched = true;
        bool reactivation_armed = false;
        if (dswros::area_quest_active_sample_may_rearm(
                0, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(completion_latched,
                "scan start then exact completion then stale progress must preserve completion");
        if (dswros::area_quest_active_sample_may_rearm(
                1, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(completion_latched,
                "post-completion stale progress must remain hidden before an inactive boundary");
        reactivation_armed =
            dswros::area_quest_inactive_sample_arms_reactivation(
                AreaQuestState::End);
        if (dswros::area_quest_active_sample_may_rearm(
                1, 1, reactivation_armed)) {
            completion_latched = false;
        }
        require(!completion_latched,
                "inactive boundary then later active state may rearm a repeatable task");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "a proven eligible unfinished quest must be globally visible");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, true, false, false,
                    AreaQuestEligibilityProof::Unknown),
                "an unfinished quest without prerequisite proof must fail closed");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, false, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "NONE without a completion snapshot must remain hidden");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, false, false, false,
                    AreaQuestEligibilityProof::Eligible),
                "UNKNOWN without a completion snapshot must remain hidden");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, true, false,
                    AreaQuestEligibilityProof::Eligible),
                "a completed saved quest must remain hidden");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::Acceptable, false, true, true,
                    AreaQuestEligibilityProof::Unknown)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Progress, false, true, true,
                        AreaQuestEligibilityProof::Unknown),
                "strict runtime eligibility must remain visible on the world map");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::End, true, false, false,
                    AreaQuestEligibilityProof::Eligible)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Fail, true, false, false,
                        AreaQuestEligibilityProof::Eligible),
                "unconfirmed terminal samples must not hide an unfinished proven quest");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::End, true, false, true,
                    AreaQuestEligibilityProof::Eligible)
                    && !dswros::area_quest_world_map_visible(
                        AreaQuestState::Fail, true, false, true,
                        AreaQuestEligibilityProof::Eligible),
            "a confirmed runtime completion must hide the quest");

        require(dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 0, 1)
                && dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 1, 2),
            "an exact requested save row must grow beyond its verified per-ID baseline");
        require(!dswros::accept_area_quest_save_confirmation(
                    false, true, false, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, false, false, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, true, true, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, false, 0, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true,
                    dswros::kUnknownAreaQuestSaveCompletionCount, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 1, 1)
                && !dswros::accept_area_quest_save_confirmation(
                    true, true, false, true, 2, 1),
            "save confirmation must reject unrequested, unavailable, ambiguous, mismatched, unknown-baseline, unchanged, and regressed rows");
        require(dswros::area_quest_completion_generation_may_arm(false)
                && !dswros::area_quest_completion_generation_may_arm(true),
            "a failed area-quest generation must suppress repeated same-cycle witnesses");
        require(dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::None)
                && dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::End)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::Fail)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_arms_reactivation(
                    false, AreaQuestState::None),
            "only a locked generation followed by NONE or END may arm reactivation");
        require(dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Acceptable)
                && dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    false, true, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, false, AreaQuestState::Progress)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::None)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::End)
                && !dswros::area_quest_completion_generation_may_unlock(
                    true, true, AreaQuestState::Fail),
            "a failed area-quest generation must unlock only after an inactive NONE/END sample followed by a fresh active sample");
        require(dswros::retry_area_quest_save_confirmation(
                    true, false, 1, 3)
                && dswros::retry_area_quest_save_confirmation(
                    true, false, 2, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    false, false, 1, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    true, true, 1, 3)
                && !dswros::retry_area_quest_save_confirmation(
                    true, false, 3, 3),
            "area-quest save fallback must be positive-only and bounded to three event-driven attempts");
        require(dswros::accept_treasure_save_confirmation(
                    true, true, true)
                && !dswros::accept_treasure_save_confirmation(
                    false, true, true)
                && !dswros::accept_treasure_save_confirmation(
                    true, false, true)
                && !dswros::accept_treasure_save_confirmation(
                    true, true, false)
                && dswros::retry_treasure_save_confirmation(
                    true, false, 1, 2)
                && !dswros::retry_treasure_save_confirmation(
                    true, true, 1, 2)
                && !dswros::retry_treasure_save_confirmation(
                    true, false, 2, 2),
            "underwater save confirmation must require an exact positive requested bit and allow only one bounded retry");
        require(dswros::accept_mounted_treasure_interactor(
                    true, true, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    false, true, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, false, true, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, true, false, true)
                && !dswros::accept_mounted_treasure_interactor(
                    true, true, true, false),
            "mounted underwater treasure completion must require the mount-only receiver, exact current Rider, same world, and nearby exact receiver");
        require(dswros::area_quest_world_map_visible(
                    AreaQuestState::Acceptable, true, true, true,
                    AreaQuestEligibilityProof::Unknown)
                    && dswros::area_quest_world_map_visible(
                        AreaQuestState::Progress, true, true, true,
                        AreaQuestEligibilityProof::Unknown),
                "current active state must override an earlier repeatable completion");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::None, true, false, true,
                    AreaQuestEligibilityProof::Eligible),
                "a completion observed in this activation must override an unfinished snapshot");
        require(!dswros::area_quest_world_map_visible(
                    AreaQuestState::Unknown, true, false, true,
                    AreaQuestEligibilityProof::Eligible),
                "an unavailable local object must not revive a completion observed in this activation");
        require(dswros::area_quest_visible_for_display_mode(
                    false, true, AreaQuestState::None,
                    true, false, false)
                    && !dswros::area_quest_visible_for_display_mode(
                        false, false, AreaQuestState::None,
                        true, false, false),
                "AVAILABLE mode must preserve prerequisite-proven visibility exactly");
        require(dswros::area_quest_visible_for_display_mode(
                    true, false, AreaQuestState::None,
                    true, false, false)
                    && dswros::area_quest_visible_for_display_mode(
                        true, false, AreaQuestState::Unknown,
                        false, false, false),
                "ALL mode must expose unfinished tasks even when prerequisite or snapshot proof is unavailable");
        require(!dswros::area_quest_visible_for_display_mode(
                    true, true, AreaQuestState::None,
                    true, true, false)
                    && !dswros::area_quest_visible_for_display_mode(
                        true, true, AreaQuestState::None,
                        false, false, true),
                "ALL mode must still hide saved or runtime-confirmed completions");
        require(dswros::area_quest_visible_for_display_mode(
                    true, false, AreaQuestState::Progress,
                    true, true, true),
                "a currently active repeatable task must override an older completion cycle in ALL mode");
    }
    {
        auto value = tracker();
        const auto id = value.observe({4, 8}, "TreasureBox01_C", {1001.0, 1998.0, 2999.0});
        require(id && *id == 1001, "exact class and nearby coordinate must match");
        const auto event = value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 7, 11);
        require(event && event->kind == dswros::EventKind::TreasureOpened && event->id == 1001,
                "destroyed observed nearby treasure must publish opened event");
    }
    {
        dswros::DisappearanceConfirmation gate;
        require(!gate.sample(false, true), "first missing sample must not complete");
        require(gate.sample(false, true), "second missing sample must complete");
        gate.reset();
        require(!gate.sample(false, true), "reset must clear missing history");
        require(!gate.sample(true, true), "reappearance must cancel missing history");
        require(!gate.sample(false, false), "invalid context must fail closed");
        gate.mark_eligible_end();
        require(gate.sample(false, true), "eligible EndPlay plus missing probe must complete");
    }
    {
        dswros::UnobservedEncounterEndEvidence evidence{
            true, false, false, true, true, true, true, true, true,
            true, false};
        require(dswros::accept_unobserved_encounter_end(evidence),
                "destroyed exact nearby available encounter must recover");
        auto rejected = evidence;
        rejected.destroyed = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "streaming removal must not recover an unobserved encounter");
        rejected = evidence;
        rejected.exact_catalog_class = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "non-target actor class must not recover an encounter");
        rejected = evidence;
        rejected.player_near_actor = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "distant actor destruction must not recover an encounter");
        rejected.weak_identity_valid = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "invalid weak identity must not recover an encounter");
        rejected = evidence;
        rejected.activity_suppressed = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "suppressed activity must fail closed");
        rejected = evidence;
        rejected.transition_active = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "travel destruction must fail closed");
        rejected = evidence;
        rejected.encounter_available = false;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "cooling-down encounter must not recover twice");
        rejected = evidence;
        rejected.duplicate_observation = true;
        require(!dswros::accept_unobserved_encounter_end(rejected),
                "existing positive observation must own completion");
        require(dswros::encounter_available_now(
                    true, true, false, 0, 1000),
                "encounter without cooldown must be available");
        require(!dswros::encounter_available_now(
                    true, false, false, 0, 1000),
                "expired time-window encounter must reject completion");
        require(!dswros::encounter_available_now(
                    false, true, false, 0, 1000),
                "unready encounter state must reject completion");
        require(dswros::encounter_activity_edge_requires_reset(true, true),
                "activity suppression entry must reset encounter observations");
        require(!dswros::encounter_activity_edge_requires_reset(false, true),
                "unchanged activity suppression must not repeat encounter cleanup");
        require(!dswros::encounter_activity_edge_requires_reset(true, false),
                "open-world return must preserve fresh encounter candidates");
        require(!dswros::encounter_available_now(
                    true, true, true, 1100, 1000),
                "future cooldown must reject duplicate completion");
        require(!dswros::encounter_cooldown_write_allowed(
                    true, 1100, 1000),
                "future cooldown must reject duplicate cooldown write");
        require(dswros::encounter_available_now(
                    true, true, true, 1000, 1000),
                "expired cooldown must allow a new completion");
        require(!dswros::encounter_visible_for_display_mode(
                    true, true, false, false, false, 0, 1000),
                "AVAILABLE Assault display must preserve the live time window");
        require(dswros::encounter_visible_for_display_mode(
                    true, true, true, false, false, 0, 1000),
                "ALL Assault display must expose an out-of-window static record");
        require(dswros::encounter_visible_for_display_mode(
                    true, true, true, false, true, 1100, 1000),
                "ALL Assault display must include cooling-down Assaults");
        require(dswros::encounter_visible_for_display_mode(
                    false, true, true, false, true, 1100, 1000),
                "ALL Assault display must expose the static catalog before save state is ready");
        require(!dswros::encounter_visible_for_display_mode(
                    true, false, true, false, false, 0, 1000),
                "ALL Assault display must not bypass a Boss time condition");
        require(!dswros::encounter_visible_for_display_mode(
                    false, true, false, true, false, 0, 1000),
                "AVAILABLE Assault display must fail closed before state is ready");
        require(dswros::encounter_cooldown_write_allowed(
                    true, 1000, 1000),
                "expired cooldown must allow a new cooldown write");
        require(dswros::merge_encounter_cooldown(1100, 1050) == 1100,
                "an older save snapshot must not roll back a newer runtime cooldown");
        require(dswros::merge_encounter_cooldown(1100, 1200) == 1200,
                "a newer save snapshot must advance the effective cooldown");
    }
    {
        dswros::EncounterDisappearanceEvidence evidence{
            true, false, true, true};
        require(dswros::accept_encounter_disappearance(evidence),
                "a visible encounter may complete after disappearing while the player remains nearby");
        auto rejected = evidence;
        rejected.player_near_last_actor = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "leaving encounter streaming range must never count as a defeat");
        rejected = evidence;
        rejected.visible_seen = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "an unseen encounter placeholder must not count as a defeat");
        rejected = evidence;
        rejected.lifecycle_valid = false;
        require(!dswros::accept_encounter_disappearance(rejected),
                "travel and suppression must invalidate disappearance evidence");
        rejected = evidence;
        rejected.visible_seen = false;
        rejected.destroyed_end = true;
        require(dswros::accept_encounter_disappearance(rejected),
                "an exact nearby Destroyed EndPlay remains positive evidence");
        require(dswros::preserve_observed_encounter_removal(
                    true, true, true),
                "an observed nearby encounter removal must retain its numeric confirmation gate");
        require(!dswros::preserve_observed_encounter_removal(
                    false, true, true),
                "an unobserved streaming removal must remain insufficient evidence");
        require(!dswros::preserve_observed_encounter_removal(
                    true, true, false),
                "travel must reject retained encounter removal evidence");
        require(!dswros::encounter_cursor_context_allowed(true, false),
                "a live weak disappearance must reset while a menu owns the cursor");
        require(dswros::encounter_cursor_context_allowed(true, true),
                "an exact ended numeric observation must survive opening the map");
        require(dswros::encounter_cursor_context_allowed(false, false),
                "normal gameplay must allow ordinary encounter observation");

        dswros::EncounterDeathNotificationEvidence death_evidence{
            true, true, true, true, true, true, true};
        require(dswros::accept_encounter_death_notification(death_evidence),
                "an exact observed nearby monster death notification must complete");
        auto rejected_death = death_evidence;
        rejected_death.lifecycle_valid = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "travel or suppression must reject a death notification");
        rejected_death = death_evidence;
        rejected_death.encounter_currently_available = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unavailable encounter must reject a duplicate death notification");
        rejected_death = death_evidence;
        rejected_death.exact_observed_actor = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unobserved receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.exact_catalog_class = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a non-catalog receiver class must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.monster_character = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a non-monster receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.visible_seen = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "an unseen receiver must not complete an encounter");
        rejected_death = death_evidence;
        rejected_death.player_near_actor = false;
        require(!dswros::accept_encounter_death_notification(rejected_death),
                "a death outside the observed streaming context must fail closed");

        require(dswros::encounter_death_process_is_terminal(3)
                    && !dswros::encounter_death_process_is_terminal(0)
                    && !dswros::encounter_death_process_is_terminal(1)
                    && !dswros::encounter_death_process_is_terminal(2)
                    && !dswros::encounter_death_process_is_terminal(4),
                "only the exact death-process End state may publish completion");

        const dswros::PendingEncounterDeathConsumptionContext
            normal_death_handoff{false, true, true, false, false, true};
        require(dswros::can_consume_pending_encounter_death(
                    normal_death_handoff),
                "a valid active context must consume an accepted encounter death bit");
        auto deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.transition_active = true;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "travel must defer ordinary accepted encounter death consumption");
        deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.activity_suppressed = true;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "activity suppression must defer ordinary accepted encounter death consumption");
        deferred_death_handoff = normal_death_handoff;
        deferred_death_handoff.player_position_valid = false;
        require(!dswros::can_consume_pending_encounter_death(
                    deferred_death_handoff),
                "an invalid player position must defer ordinary accepted encounter death consumption");
        dswros::PendingEncounterDeathConsumptionContext boundary_death_handoff{
            true, false, true, true, true, false};
        require(dswros::can_consume_pending_encounter_death(
                    boundary_death_handoff),
                "an authoritative boundary must preserve a bit after Engine Tick disables itself");
        boundary_death_handoff.encounter_state_ready = false;
        require(!dswros::can_consume_pending_encounter_death(
                    boundary_death_handoff),
                "an uninitialized encounter state must reject even boundary consumption");

        dswros::EncounterDisappearanceConfirmation fast_end_gate;
        require(!fast_end_gate.sample(true, true, false, 100),
                "one visible sample must not arm ordinary encounter confirmation");
        fast_end_gate.arm_observed_end_fallback(200);
        for (std::uint32_t sample = 1;
             sample < dswros::EncounterDisappearanceConfirmation::kRequiredMissingSamples;
             ++sample) {
            require(!fast_end_gate.sample(
                        false, true, false,
                        200 + static_cast<std::int64_t>(sample) * 250),
                    "observed EndPlay fallback must retain the ten-second gate");
        }
        require(fast_end_gate.sample(false, true, false, 10'200),
                "observed EndPlay fallback must complete after forty missing samples and ten seconds");

        dswros::EncounterDisappearanceConfirmation returned_gate;
        returned_gate.arm_observed_end_fallback(1'000);
        require(!returned_gate.sample(false, true, false, 1'250),
                "observed EndPlay fallback must begin as missing");
        require(!returned_gate.sample(true, true, false, 1'500),
                "a returned encounter must cancel the missing sequence");
        require(!returned_gate.pending(),
                "a returned encounter must clear pending EndPlay evidence");

        dswros::EncounterDisappearanceConfirmation gate;
        require(!gate.sample(true, true, false, 0),
                "the first visible sample must not arm encounter fallback");
        require(!gate.sample(true, true, false, 250),
                "short visibility must not arm encounter fallback");
        require(!gate.sample(true, true, false, 500),
                "short visibility must remain unarmed");
        for (std::uint32_t sample = 0; sample < 50; ++sample) {
            require(!gate.sample(false, true, false,
                        750 + static_cast<std::int64_t>(sample) * 250),
                    "an encounter that was not stably present must not complete");
        }
        require(!gate.sample(false, false, false, 20'000),
                "context reset must clear the unarmed history");
        for (std::int64_t now = 21'000; now <= 22'000; now += 250) {
            require(!gate.sample(true, true, false, now),
                    "stable presence only arms the encounter fallback");
        }
        for (std::int64_t now = 22'250; now < 32'250; now += 250) {
            require(!gate.sample(false, true, false, now),
                    "less than ten missing seconds must fail closed");
        }
        require(gate.sample(false, true, false, 32'250),
                "ten seconds plus enough missing probes while nearby may complete an encounter");
        require(!gate.sample(true, true, false, 32'500),
                "reappearance must reset missing encounter evidence");
        require(!gate.sample(false, true, false, 32'750),
                "a fresh disappearance must restart confirmation");
        require(!gate.sample(false, false, false, 33'000),
                "leaving range must cancel a pending encounter disappearance");
        require(!gate.pending(),
                "a rejected out-of-range disappearance must not accumulate");
        require(gate.sample(false, true, true, 33'250),
                "an exact nearby Destroyed EndPlay may complete immediately");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, false, false, {1000.0, 2000.0, 3000.0}, 7, 11),
                "streaming removal must not publish opened event");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, true, {1000.0, 2000.0, 3000.0}, 7, 11),
                "transition destruction must fail closed");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {1000.0, 2000.0, 3000.0}, 8, 11),
                "stale activation must not publish");
    }
    {
        auto value = tracker();
        static_cast<void>(value.observe({4, 8}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}));
        require(!value.end({4, 8}, true, false, {10000.0, 2000.0, 3000.0}, 7, 11),
                "distant destruction must not publish");
    }
    {
        auto value = tracker();
        require(!value.observe({4, 8}, "TreasureBox02_C", {1000.0, 2000.0, 3000.0}),
                "wrong exact class must not match");
        require(!value.observe({4, 8}, "TreasureBox01_C", {2000.0, 2000.0, 3000.0}),
                "coordinate outside match radius must not match");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {2001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {2002, "TreasureBox01_C", {1000.0, 2000.0, 3500.0}},
            {2003, "TreasureBox01_C", {1181.0, 2013.0, 3500.0}},
        });
        value.reset(7, 11);
        const auto upper = value.observe(
            {20, 40}, "TreasureBox01_C", {1000.0, 2000.0, 3500.0});
        const auto adjacent = value.observe(
            {21, 41}, "TreasureBox01_C", {1181.0, 2013.0, 3500.0});
        const auto lower = value.observe(
            {22, 42}, "TreasureBox01_C", {1000.0, 2000.0, 3000.0});
        require(upper && *upper == 2002,
                "XY-overlapping treasure actors must resolve by exact 3D position");
        require(adjacent && *adjacent == 2003,
                "adjacent same-class treasure actors must retain distinct IDs");
        require(lower && *lower == 2001,
                "the lower XY-overlapping treasure must retain its own ID");
        require(value.observed_count() == 3,
                "adjacent treasure weak identities must remain independently observed");
        const auto upper_end = value.end(
            {20, 40}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        const auto adjacent_end = value.end(
            {21, 41}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        const auto lower_end = value.end(
            {22, 42}, true, false, {1000.0, 2000.0, 3250.0}, 7, 11);
        require(upper_end && upper_end->id == 2002,
                "the upper treasure lifecycle must publish only its own ID");
        require(adjacent_end && adjacent_end->id == 2003,
                "the adjacent treasure lifecycle must publish only its own ID");
        require(lower_end && lower_end->id == 2001,
                "the lower treasure lifecycle must publish only its own ID");
        require(value.observed_count() == 0,
                "completed adjacent treasure observations must not accumulate");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {3001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {3002, "TreasureBox01_C", {1000.0, 2000.0, 3500.0}},
            {3003, "TreasureBox02_C", {1181.0, 2013.0, 3500.0}},
        });
        value.reset(7, 11);
        const auto lower = value.observe_reported_id(
            {30, 50}, "TreasureBox01_C",
            {1000.0, 2000.0, 3000.0}, 3001);
        const auto upper = value.observe_reported_id(
            {31, 51}, "TreasureBox01_C",
            {1000.0, 2000.0, 3500.0}, 3002);
        require(lower && *lower == 3001 && upper && *upper == 3002,
                "runtime ObjectID plus class and 3D position must keep overlapping treasures distinct");
        require(!value.observe_reported_id(
                    {32, 52}, "TreasureBox01_C",
                    {1181.0, 2013.0, 3500.0}, 3003),
                "runtime ObjectID class conflict must fail closed without spatial fallback");
        require(!value.observe_reported_id(
                    {33, 53}, "TreasureBox01_C",
                    {1000.0, 2000.0, 3000.0}, 9999),
                "unknown runtime ObjectID must fail closed");
        require(!value.observe_reported_id(
                    {34, 54}, "TreasureBox01_C",
                    {2000.0, 2000.0, 3000.0}, 3001),
                "runtime ObjectID outside the catalog 3D bound must fail closed");
    }
    {
        dswros::ObjectStateTracker value;
        value.set_catalog({
            {4001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
            {4001, "TreasureBox01_C", {1000.0, 2000.0, 3000.0}},
        });
        value.reset(7, 11);
        require(!value.observe_reported_id(
                    {40, 60}, "TreasureBox01_C",
                    {1000.0, 2000.0, 3000.0}, 4001),
                "duplicate reported treasure IDs must fail closed");
    }
    {
        auto value = tracker();
        const auto nearby = value.nearby_classes({900.0, 2000.0, 3000.0}, 3000.0);
        require(nearby.size() == 1 && nearby.front() == "TreasureBox01_C",
                "catch-up must request only classes near the player");
    }
    {
        auto value = tracker();
        const auto nearest = value.nearest_point({1200.0, 2100.0, 3000.0}, 10000.0);
        require(nearest && nearest->id == 1001,
                "native render target selection must return the closest catalog point");
        require(!value.nearest_point({50000.0, 50000.0, 50000.0}, 1000.0),
                "native render target selection must respect the bounded radius");
        const auto annulus = value.nearest_planar_point_in_annulus(
            {1000.0, 2000.0, 3000.0}, 3000.0, 20000.0);
        require(annulus && annulus->id == 1002,
                "validation target selection must avoid markers hidden at the player center");
        require(!value.nearest_planar_point_in_annulus(
                    {1000.0, 2000.0, 3000.0}, 10000.0, 20000.0),
                "validation target selection must enforce both annulus bounds");
    }
    {
        const auto center = dswros::project_compact_radar_point(
            {1000.0, 2000.0, 0.0}, {1000.0, 2000.0, 0.0}, 20000.0, 2560, 1440);
        require(center && center->x == 2340.0 && center->y == 217.0
                    && center->display_scale == 1.0,
                "native compact projection must preserve accepted 2560x1440 geometry");
        const auto east_edge = dswros::project_compact_radar_point(
            {0.0, 0.0, 0.0}, {20000.0, 0.0, 0.0}, 20000.0, 2560, 1440);
        require(east_edge && east_edge->x == 2510.0 && east_edge->y == 217.0,
                "native compact projection must place the positive-X radius at the right edge");
        require(!dswros::project_compact_radar_point(
                    {0.0, 0.0, 0.0}, {20001.0, 0.0, 0.0}, 20000.0, 2560, 1440),
                "native compact projection must reject points outside the configured radius");
    }
    {
        const auto fullscreen = dswros::calculate_compact_viewport_layout(
            3840.0, 2160.0, 1.5, 1.0);
        require(fullscreen && near(fullscreen->display_scale, 1.5)
                    && near(fullscreen->umg_unit_scale, 1.0)
                    && near(fullscreen->host_origin_x, 3146.625)
                    && near(fullscreen->host_origin_y, -37.875)
                    && near(fullscreen->host_render_scale, 1.0),
                "compact fullscreen layout must preserve the accepted physical geometry");

        const auto windowed = dswros::calculate_compact_viewport_layout(
            2560.0, 1600.0, 1.0, 1.0);
        require(windowed && near(windowed->display_scale, 1.0)
                    && near(windowed->umg_unit_scale, 1.0)
                    && near(windowed->host_origin_x, 2097.75)
                    && near(windowed->host_origin_y, -25.25)
                    && near(windowed->host_render_scale, 1.0),
                "a retained compact host must move back on-screen after a window-mode resize");

        const auto native_ultrawide =
            dswros::calculate_compact_viewport_layout(
                3440.0, 1440.0, 1.0, 1.0);
        require(native_ultrawide
                    && near(native_ultrawide->display_scale, 1.0)
                    && near(native_ultrawide->host_origin_x, 2977.75)
                    && near(native_ultrawide->host_origin_y, -25.25),
                "native 21:9 must use the live game viewport instead of desktop geometry");

        const auto ultrawide_content_viewport =
            dswros::calculate_compact_viewport_layout(
                3840.0, 1600.0, 1.0, 1.0);
        require(ultrawide_content_viewport
                    && near(ultrawide_content_viewport->display_scale, 10.0 / 9.0)
                    && near(ultrawide_content_viewport->host_origin_x,
                            3326.388888888889)
                    && near(ultrawide_content_viewport->host_origin_y,
                            -28.055555555556)
                    && near(ultrawide_content_viewport->host_render_scale,
                            10.0 / 9.0),
                "a true 3840x1600 game viewport must preserve its compact HUD geometry without implying a desktop-sized viewport");

        const auto ultrawide_window =
            dswros::calculate_compact_viewport_layout(
                2560.0, 1080.0, 1.0, 1.0);
        require(ultrawide_window
                    && near(ultrawide_window->display_scale, 0.75)
                    && near(ultrawide_window->host_origin_x, 2213.3125)
                    && near(ultrawide_window->host_origin_y, -18.9375),
                "windowed 21:9 must reflow from its live client viewport");

        const auto dpi_resize = dswros::calculate_compact_viewport_layout(
            1920.0, 1080.0, 1.25, 1.0);
        require(dpi_resize && near(dpi_resize->display_scale, 0.75)
                    && near(dpi_resize->umg_unit_scale, 0.6)
                    && near(dpi_resize->host_render_scale, 0.6),
                "compact reflow must correct a retained host for live DPI changes");
        require(!dswros::calculate_compact_viewport_layout(
                    1920.0, 1080.0, 0.0, 1.0),
                "compact viewport layout must reject an invalid DPI scale");
    }
    {
        require(dswros::compact_time_phase(5U * 3600U + 59U * 60U)
                        == dswros::CompactTimePhase::Night
                    && dswros::compact_time_phase(6U * 3600U)
                        == dswros::CompactTimePhase::Morning
                    && dswros::compact_time_phase(12U * 3600U)
                        == dswros::CompactTimePhase::Afternoon
                    && dswros::compact_time_phase(18U * 3600U)
                        == dswros::CompactTimePhase::Evening
                    && dswros::compact_time_phase(21U * 3600U)
                        == dswros::CompactTimePhase::Night
                    && dswros::compact_time_phase(30U * 3600U)
                        == dswros::CompactTimePhase::Morning,
                "the four presentation bands must retain exact boundaries and day wrapping");
    }
    {
        const auto same_parent = dswros::rebase_world_map_atlas_placement(
            {-410.0, -275.0, 2048.0, 2048.0},
            {2460.347, 2001.596, 3000.0, 3000.0},
            {2460.347, 2001.596, 3000.0, 3000.0});
        require(same_parent && near(same_parent->left, -410.0)
                    && near(same_parent->top, -275.0)
                    && near(same_parent->width, 2048.0)
                    && near(same_parent->height, 2048.0),
                "same-anchor world-map reparenting must preserve atlas bounds");

        const auto translated_parent =
            dswros::rebase_world_map_atlas_placement(
                {-410.0, -275.0, 2048.0, 2048.0},
                {2460.347, 2001.596, 3000.0, 3000.0},
                {2480.347, 1991.596, 3000.0, 3000.0});
        require(translated_parent && near(translated_parent->left, -390.0)
                    && near(translated_parent->top, -285.0)
                    && near(translated_parent->width, 2048.0)
                    && near(translated_parent->height, 2048.0),
                "same-extent Canvas replacement must apply only the live player-anchor delta");

        require(!dswros::rebase_world_map_atlas_placement(
                    {-410.0, -275.0, 2048.0, 2048.0},
                    {2460.347, 2001.596, 3000.0, 3000.0},
                    {3145.620, 2559.683, 3840.0, 3840.0}),
                "a 3000-to-3840 parent-local extent change must request a fresh atlas instead of scaling marker glyphs");

        require(!dswros::rebase_world_map_atlas_placement(
                    {-410.0, -275.0, 0.0, 2048.0},
                    {2460.347, 2001.596, 3000.0, 3000.0},
                    {2480.347, 1991.596, 3000.0, 3000.0})
                    && !dswros::rebase_world_map_atlas_placement(
                        {-410.0, -275.0, 2048.0, 2048.0},
                        {2460.347, 2001.596, 3000.0, 3000.0},
                        {std::numeric_limits<double>::infinity(),
                         1991.596, 3000.0, 3000.0}),
                "world-map reparenting must reject invalid retained bounds or anchors");
    }
    {
        const auto unchanged = dswros::world_map_geometry_maximum_delta(
            {1500.0, 1500.0, 3000.0, 3000.0},
            {1500.25, 1499.75, 3000.0, 3000.0});
        const auto logged_parent_extent_change =
            dswros::world_map_geometry_maximum_delta(
                {2460.347, 2001.596, 3000.0, 3000.0},
                {3145.620, 2559.683, 3840.0, 3840.0});
        require(unchanged && near(*unchanged, 0.25)
                    && logged_parent_extent_change
                    && near(*logged_parent_extent_change, 840.0)
                    && !dswros::world_map_geometry_maximum_delta(
                        {1500.0, 1500.0, 0.0, 3000.0},
                        {1500.0, 1500.0, 3000.0, 3000.0}),
                "world-map geometry drift must distinguish sub-pixel stability from the logged 3000-to-3840 parent-local reflow");
    }
    {
        bool sample_valid{};
        dswros::WorldMapGeometrySample retained{};
        double maximum_delta{-1.0};
        const auto seeded = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {1940.0, 720.0, 3440.0, 1440.0}, maximum_delta);
        require(seeded
                    == dswros::WorldMapGeometryStabilityResult::Seeded
                    && sample_valid && near(maximum_delta, 0.0)
                    && near(retained.player_canvas_x, 1940.0)
                    && near(retained.parent_width, 3440.0),
                "the first valid world-map geometry sample must seed numeric state without accepting attachment");

        const auto replaced = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.0, 720.0, 3440.0, 1440.0}, maximum_delta);
        require(replaced
                    == dswros::WorldMapGeometryStabilityResult::Replaced
                    && near(maximum_delta, 300.0)
                    && near(retained.player_canvas_x, 2240.0),
                "a changed second world-map geometry sample must replace numeric state without accepting attachment");

        const auto stable = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.25, 719.75, 3440.25, 1439.75}, maximum_delta);
        require(stable
                    == dswros::WorldMapGeometryStabilityResult::Stable
                    && near(maximum_delta, 0.25)
                    && std::string_view{
                        dswros::world_map_geometry_stability_name(stable)}
                        == "stable",
                "the third existing attempt may accept geometry stable within half a Slate unit");

        const auto invalid = dswros::observe_world_map_geometry_sample(
            sample_valid, retained,
            {2240.0, 720.0, 0.0, 1440.0}, maximum_delta);
        require(invalid
                    == dswros::WorldMapGeometryStabilityResult::None
                    && near(retained.parent_width, 3440.25),
                "an invalid geometry observation must not overwrite the retained numeric sample");
    }
    {
        require(dswros::world_map_attach_failure_retryable(3U)
                    && dswros::world_map_attach_failure_retryable(9U)
                    && dswros::world_map_attach_failure_retryable(24U)
                    && !dswros::world_map_attach_failure_retryable(2U)
                    && !dswros::world_map_attach_failure_retryable(10U)
                    && !dswros::world_map_attach_failure_retryable(100U),
                "world-map attachment must use one retryable-failure policy for widget and Slate geometry readiness");
        require(!dswros::world_map_attach_attempt_is_terminal(false, 24U),
                "a failed Slate-geometry attachment must leave the current-session retry latch open");
        require(!dswros::world_map_attach_attempt_is_terminal(false, 3U)
                    && dswros::world_map_attach_attempt_is_terminal(false, 2U)
                    && dswros::world_map_attach_attempt_is_terminal(false, 10U)
                    && dswros::world_map_attach_attempt_is_terminal(true, 24U),
                "world-map attachment must latch only success or a non-retryable failure");
    }
    {
        const auto standard_anchor =
            dswros::validate_world_map_canvas_anchor(
                1280.0, 720.0, 2560.0, 1440.0, 3000.0);
        require(standard_anchor && near(standard_anchor->x, 1280.0)
                    && near(standard_anchor->y, 720.0),
                "world-map geometry validation must accept a live 16:9 Canvas extent");

        const auto ultrawide_anchor =
            dswros::validate_world_map_canvas_anchor(
                1940.0, 720.0, 3440.0, 1440.0, 3000.0);
        require(ultrawide_anchor && near(ultrawide_anchor->x, 1940.0)
                    && near(ultrawide_anchor->y, 720.0),
                "world-map geometry validation must accept a live 21:9 Canvas extent and offset");

        const auto sixteen_ten_anchor =
            dswros::validate_world_map_canvas_anchor(
                1280.0, 900.0, 2560.0, 1600.0, 3000.0);
        require(sixteen_ten_anchor && near(sixteen_ten_anchor->x, 1280.0)
                    && near(sixteen_ten_anchor->y, 900.0),
                "world-map geometry validation must accept a live 16:10 Canvas extent and offset");

        const auto arbitrary_positive_extent =
            dswros::validate_world_map_canvas_anchor(
                0.5, 0.5, 1.0, 1.0, 3000.0);
        require(arbitrary_positive_extent
                    && near(arbitrary_positive_extent->x, 0.5)
                    && near(arbitrary_positive_extent->y, 0.5),
                "world-map geometry validation must not impose an authored-size assumption on a finite positive Canvas");

        require(!dswros::validate_world_map_canvas_anchor(
                    -200.0, 720.0, 3440.0, 1440.0, 3000.0),
                "world-map geometry validation must reject an implausible stale player anchor");
        require(!dswros::validate_world_map_canvas_anchor(
                    3500.0, 720.0, 3440.0, 1440.0, 3000.0),
                "world-map geometry validation must reject a point outside its exact native Canvas");
        require(!dswros::validate_world_map_canvas_anchor(
                    0.0, 0.0, 0.0, 1440.0, 3000.0),
                "world-map geometry validation must reject a non-positive parent extent");

        const auto player = dswros::project_world_map_point(
            3145.620, 2559.683,
            {100000.0, 200000.0, 0.0},
            {100000.0, 200000.0, 0.0},
            570000.0, 3840.0, 3840.0);
        require(player && near(player->x, 3145.620)
                    && near(player->y, 2559.683),
                "world-map projection must preserve the logged 21:9 player anchor in the exact 3840-square parent-local space");

        const auto sixteen_nine_offset = dswros::project_world_map_point(
            2460.347, 2001.596,
            {100000.0, 200000.0, 0.0},
            {157000.0, 86000.0, 0.0},
            570000.0, 3000.0, 3000.0);
        require(sixteen_nine_offset
                    && near(sixteen_nine_offset->x, 2760.347)
                    && near(sixteen_nine_offset->y, 1401.596),
                "the logged 16:9 parent-local extent must scale both world deltas by 3000");

        const auto twenty_one_nine_offset = dswros::project_world_map_point(
            3145.620, 2559.683,
            {100000.0, 200000.0, 0.0},
            {157000.0, 86000.0, 0.0},
            570000.0, 3840.0, 3840.0);
        require(twenty_one_nine_offset
                    && near(twenty_one_nine_offset->x, 3529.620)
                    && near(twenty_one_nine_offset->y, 1791.683),
                "the logged 21:9 parent-local extent must scale world deltas by 3840 instead of authored ui_size 3000");
        require(!dswros::project_world_map_point(
                    1940.0, 720.0,
                    {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                    0.0, 3840.0, 3840.0)
                    && !dswros::project_world_map_point(
                        1940.0, 720.0,
                        {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
                        570000.0, 0.0, 3840.0),
                "world-map projection must reject invalid map dimensions");
    }
    {
        dswros::CompactRenderModel model;
        std::array<dswros::CompactTreasureMarker, 1> output{};
        const std::array<std::uint8_t, 0> eligibility{};
        const auto result = model.refresh(
            {0.0, 0.0, 0.0}, true, 12500.0, eligibility, output);
        require(result.status == dswros::CompactRefreshStatus::NotInitialized,
                "compact refresh must fail closed before catalog initialization");

        const std::array<dswros::CompactTreasureCatalogEntry, 1> invalid{{
            {1, 100, {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}, true},
        }};
        require(!model.initialize(invalid) && !model.initialized(),
                "compact catalog initialization must reject invalid coordinates");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 6> catalog{{
            {1, 100, {0.0, -12500.0, 0.0}, false,
             dswros::CompactTreasureKind::Puzzle},
            {2, 100, {0.0, -12500.1, 0.0}, false},
            {3, 200, {100.0, 0.0, 0.0}, false},
            {4, 100, {200.0, 0.0, 0.0}, false},
            {5, 100, {22500.0, 0.0, 0.0}, false},
            {6, 100, {22500.1, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 6> eligibility{{1, 1, 1, 0, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 80> output{};
        require(model.initialize(catalog),
                "compact catalog initialization must accept finite entries");

        const auto compact = model.refresh(
            {0.0, 0.0, 0.0}, false, 12500.0, eligibility, output);
        require(compact.ok() && compact.count == 1 && output[0].id == 1,
                "125 metre planar boundary must be inclusive and filter map and eligibility");
        require(near(output[0].normalized_x, 0.0)
                    && near(output[0].normalized_y, -1.0),
                "compact output must expose normalized XY coordinates");
        require(output[0].kind
                    == dswros::CompactTreasureKind::Puzzle,
                "compact output must preserve the install-time treasure kind");

        const auto expanded = model.refresh(
            {0.0, 0.0, 0.0}, false, 22500.0, eligibility, output);
        require(expanded.ok() && expanded.count == 3,
                "225 metre planar boundary must be inclusive");
        require(marker_by_id(
                    {output.data(), expanded.count}, 5) != nullptr
                    && marker_by_id(
                        {output.data(), expanded.count}, 6) == nullptr,
                "points immediately beyond the 225 metre boundary must be excluded");
        require(marker_by_id(
                    {output.data(), expanded.count}, 3) == nullptr,
                "compact selection must accept only map_id 100");
        require(marker_by_id(
                    {output.data(), expanded.count}, 4) == nullptr,
                "compact selection must honor caller eligibility");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 3> catalog{{
            {10, 100, {300.0, 400.0, 0.0}, false},
            {11, 100, {0.0, 500.0, 0.0}, false},
            {12, 100, {1000.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 3> eligibility{{1, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 3> output{};
        std::array<dswros::CompactTreasureMarker, 3> repeated{};
        require(model.initialize(catalog),
                "tie-order catalog must initialize");
        const auto first = model.refresh(
            {0.0, 0.0, 0.0}, false, 2000.0, eligibility, output);
        const auto second = model.refresh(
            {0.0, 0.0, 0.0}, false, 2000.0, eligibility, repeated);
        require(first.ok() && first.count == 3
                    && output[0].id == 10 && output[1].id == 11,
                "equal 3D ranks must retain deterministic catalog order");
        require(second.ok() && repeated[0].id == output[0].id
                    && repeated[1].id == output[1].id
                    && repeated[2].id == output[2].id,
                "compact refresh ordering must be stable across samples");
        std::size_t nearest_count = 0;
        for (std::size_t index = 0; index < first.count; ++index) {
            nearest_count += output[index].nearest ? 1U : 0U;
        }
        require(nearest_count == 1 && output[0].nearest,
                "exactly the closest selected treasure must be marked nearest");
        require(!output[1].nearest,
                "the second closest treasure must not receive nearest emphasis");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 2> catalog{{
            {20, 100, {100.0, 0.0, 850.0}, true},
            {21, 100, {50.0, 0.0, 1000.0}, true},
        }};
        const std::array<std::uint8_t, 2> eligibility{{1, 1}};
        std::array<dswros::CompactTreasureMarker, 2> output{};
        require(model.initialize(catalog),
                "3D rank catalog must initialize");
        const auto result = model.refresh(
            {0.0, 0.0, 1000.0}, true, 1000.0, eligibility, output);
        require(result.ok() && result.count == 2 && output[0].id == 20,
                "3D nearest rank must compare treasure Z with player Z minus 150");
        require(near(output[0].ranking_distance_squared, 10000.0),
                "adjusted player Z must contribute correctly to 3D distance");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 5> catalog{{
            {30, 100, {10.0, 0.0, 950.0}, true},
            {31, 100, {20.0, 0.0, 951.0}, true},
            {32, 100, {30.0, 0.0, 1.0e12}, true},
            {33, 100, {40.0, 0.0, -1.0e12}, true},
            {34, 100, {50.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 5> eligibility{{1, 1, 1, 1, 1}};
        std::array<dswros::CompactTreasureMarker, 5> output{};
        require(model.initialize(catalog),
                "height catalog must initialize");
        const auto result = model.refresh(
            {0.0, 0.0, 1000.0}, true, 100.0, eligibility, output);
        require(result.ok() && result.count == 5,
                "height test entries must remain inside the planar range");
        const auto markers = std::span<const dswros::CompactTreasureMarker>(
            output.data(), result.count);
        const auto* dead_zone = marker_by_id(markers, 30);
        const auto* above = marker_by_id(markers, 31);
        const auto* upper_clamp = marker_by_id(markers, 32);
        const auto* lower_clamp = marker_by_id(markers, 33);
        const auto* no_height = marker_by_id(markers, 34);
        require(dead_zone && dead_zone->height_available
                    && near(dead_zone->height_target_z, 950.0)
                    && near(dead_zone->height_delta, 0.0)
                    && near(dead_zone->height_angle_degrees, 0.0),
                "height delta at the 100 unit deadzone boundary must collapse to zero");
        const double expected_above_angle =
            -std::atan(1.0 / dswros::CompactRenderModel::kHeightSensitivity)
            * 57.2957795130823208768;
        require(above && near(above->height_delta, 101.0)
                    && near(above->height_angle_degrees, expected_above_angle),
                "height angle outside the deadzone must use the accepted atan mapping");
        require(upper_clamp
                    && near(upper_clamp->height_angle_degrees, -85.0),
                "positive height delta angle must clamp at negative 85 degrees");
        require(lower_clamp
                    && near(lower_clamp->height_angle_degrees, 85.0),
                "negative height delta angle must clamp at positive 85 degrees");
        require(no_height && !no_height->height_available
                    && near(no_height->height_delta, 0.0)
                    && near(no_height->height_angle_degrees, 0.0),                "entries without Z must not synthesize height output");
        require(near(
                    dswros::CompactRenderModel::height_angle_from_delta(100.0),
                    0.0)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(101.0),
                        expected_above_angle)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(-101.0),
                        -expected_above_angle)
                    && near(
                        dswros::CompactRenderModel::height_angle_from_delta(
                            std::numeric_limits<double>::quiet_NaN()),
                        0.0),
                "scalar height updates must be continuous at the deadzone and fail closed");
    }
    {
        std::array<dswros::CompactTreasureMarker, 3> candidates{};
        candidates[0].id = 1;
        candidates[0].ranking_distance_squared = 1000.0 * 1000.0;
        candidates[1].id = 2;
        candidates[1].ranking_distance_squared = 1050.0 * 1050.0;
        candidates[2].id = 3;
        candidates[2].ranking_distance_squared = 1400.0 * 1400.0;
        const auto retained = dswros::choose_compact_nearest(
            candidates, 2, 100.0);
        require(retained.index == 1 && retained.retained_non_best,
                "one-metre nearest hysteresis must retain a near-tied target");
        const auto switched = dswros::choose_compact_nearest(
            candidates, 2, 20.0);
        require(switched.index == 0 && !switched.retained_non_best,
                "a clearly closer target must bypass nearest hysteresis");
        const auto missing = dswros::choose_compact_nearest(
            candidates, 99, 100.0);
        require(missing.index == 0 && !missing.retained_non_best,
                "an unavailable retained target must switch immediately");
        const auto exact_boundary = dswros::choose_compact_nearest(
            candidates, 2, 50.0);
        require(exact_boundary.index == 1
                    && exact_boundary.retained_non_best,
                "the exact nearest-switch boundary must retain the target");
        const auto beyond_boundary = dswros::choose_compact_nearest(
            candidates, 2, 49.999);
        require(beyond_boundary.index == 0
                    && !beyond_boundary.retained_non_best,
                "a target beyond the nearest-switch boundary must switch");

        std::array<dswros::CompactTreasureMarker, 3> ordered{};
        ordered[0].id = 1;
        ordered[0].nearest = true;
        ordered[0].ranking_distance_squared = 1000.0 * 1000.0;
        ordered[1].id = 2;
        ordered[1].ranking_distance_squared = 1020.0 * 1020.0;
        ordered[2].id = 3;
        ordered[2].ranking_distance_squared = 1040.0 * 1040.0;
        require(dswros::promote_compact_nearest(ordered, 2),
                "a valid retained target must be promoted");
        require(ordered[0].id == 3 && ordered[0].nearest
                    && ordered[1].id == 1 && !ordered[1].nearest
                    && ordered[2].id == 2 && !ordered[2].nearest,
                "promotion must preserve the remaining distance order");
        require(!dswros::promote_compact_nearest(ordered, ordered.size()),
                "an invalid promotion index must fail closed");
    }
    {
        dswros::CompactRenderModel model;
        std::vector<dswros::CompactTreasureCatalogEntry> catalog;
        catalog.reserve(100);
        for (std::int64_t id = 1; id <= 100; ++id) {
            catalog.push_back({id, 100,
                               {static_cast<double>(id), 0.0, 0.0}, false});
        }
        const std::vector<std::uint8_t> eligibility(100, 1);
        std::array<dswros::CompactTreasureMarker, 80> output{};
        require(model.initialize(catalog),
                "maximum-selection catalog must initialize");
        const std::size_t allocations_before = allocation_count;
        const auto result = model.refresh(
            {0.0, 0.0, 0.0}, false, 200.0, eligibility, output);
        const std::size_t allocations_after = allocation_count;
        require(result.ok() && result.count == 80,
                "compact refresh must cap selected treasures at caller capacity 80");
        require(output.front().id == 1 && output.back().id == 80,
                "bounded selection must retain the nearest 80 treasures");
        require(allocations_after == allocations_before,
                "compact refresh must perform no dynamic allocation");
    }
    {
        dswros::CompactRenderModel model;
        const std::array<dswros::CompactTreasureCatalogEntry, 1> catalog{{
            {1, 100, {10.0, 0.0, 0.0}, false},
        }};
        const std::array<std::uint8_t, 1> eligibility{{1}};
        std::array<dswros::CompactTreasureMarker, 1> output{};
        std::array<dswros::CompactTreasureMarker, 81> oversized{};
        require(model.initialize(catalog),
                "invalid-input catalog must initialize");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 0.0, eligibility, output).status
                    == dswros::CompactRefreshStatus::InvalidInput,
                "zero compact radius must fail closed");
        require(model.refresh(
                    {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0},
                    false, 100.0, eligibility, output).status
                    == dswros::CompactRefreshStatus::InvalidInput,
                "non-finite player coordinates must fail closed");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0,
                    std::span<const std::uint8_t>{}, output).status
                    == dswros::CompactRefreshStatus::InvalidEligibility,
                "eligibility size must match the initialized catalog");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0, eligibility,
                    std::span<dswros::CompactTreasureMarker>{}).status
                    == dswros::CompactRefreshStatus::InvalidOutputCapacity,
                "zero output capacity must fail closed");
        require(model.refresh(
                    {0.0, 0.0, 0.0}, false, 100.0, eligibility,
                    oversized).status
                    == dswros::CompactRefreshStatus::InvalidOutputCapacity,
                "output capacity above 80 must fail closed");
    }
    {
        dswros::LiveMarkerPresenceGate gate;
        const std::size_t allocations_before = allocation_count;
        require(gate.sample(true, 1'000) && gate.visible(),
                "a live runtime marker must become visible immediately");
        require(gate.sample(false, 1'100) && gate.missing_pending(),
                "the first missing edge must start the debounce window");
        require(gate.sample(false, 1'499),
                "a runtime marker must survive the first 399 missing milliseconds");
        require(!gate.sample(false, 1'500) && !gate.visible(),
                "a runtime marker must disappear after 400 continuous missing milliseconds");
        require(!gate.sample(false, 2'000),
                "a marker that was never visible must remain absent");
        require(gate.sample(true, 3'000),
                "a new live sample must reactivate the marker");
        require(gate.sample(false, 3'050),
                "a second missing window must arm independently");
        require(gate.sample(true, 3'200) && !gate.missing_pending(),
                "a recovered live object must cancel the missing window");
        require(gate.sample(false, 3'250),
                "a later missing edge must start a fresh window");
        require(gate.sample(false, 3'100),
                "a backwards clock sample must fail safe by restarting the window");
        require(gate.sample(false, 3'499),
                "a restarted missing window must retain the marker for 399 milliseconds");
        require(!gate.sample(false, 3'500),
                "a restarted missing window must expire at exactly 400 milliseconds");
        require(gate.sample(true, 4'000),
                "a marker must reactivate after a completed disappearance window");
        gate.reset();
        require(!gate.visible() && !gate.missing_pending(),
                "an explicit lifecycle reset must clear all presence state");
        require(allocation_count == allocations_before,
                "live marker presence sampling must not allocate");
    }
    {
        const std::size_t allocations_before = allocation_count;
        require(dswros::bird_egg_interaction_available(2, 2),
                "the exact enum-backed On and NormalGather values must be available");
        require(!dswros::bird_egg_interaction_available(1, 2),
                "an Off bird-egg interaction switch must be unavailable");
        require(!dswros::bird_egg_interaction_available(2, 4),
                "a non-NormalGather bird-egg interaction type must be unavailable");
        require(dswros::bird_egg_runtime_present(
                    true, true, true, true),
                "an exact same-world interactable bird egg must be present");
        require(!dswros::bird_egg_runtime_present(
                    true, true, false, false),
                "an unknown bird-egg interaction state must fail closed");
        require(!dswros::bird_egg_runtime_present(
                    true, true, true, false),
                "a non-interactable bird egg must be absent");
        require(!dswros::bird_egg_runtime_present(
                    false, true, true, true),
                "an invalid bird-egg weak identity must be absent");
        require(!dswros::bird_egg_runtime_present(
                    true, false, true, true),
                "a bird egg from another world must be absent");
        require(allocation_count == allocations_before,
                "bird-egg enum-state classification must not allocate");
        require(dswros::is_main_menu_world_identity(
                    "World /Game/Title/TitleMap/DS_Title.DS_Title"),
                "the exact title world must be a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Maps/TransitionMap/EmptyTransitionMap.EmptyTransitionMap"),
                "the transition map must not become a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Art/Environment_Art/Maps/World/world_01_Main_WP/world_01_main_WP.World_01_Main_WP"),
                "the open world must not become a save-owner boundary");
        require(!dswros::is_main_menu_world_identity(
                    "World /Game/Title/OtherMap.OtherMap"),
                "a broad title-path match must fail closed");
        require(allocation_count == allocations_before,
                "bird-egg and title-world policy checks must not allocate");
    }
    std::cout << "NATIVE_STATE_TESTS_OK assertions="
              << assertion_count << '\n';
    return 0;
}
