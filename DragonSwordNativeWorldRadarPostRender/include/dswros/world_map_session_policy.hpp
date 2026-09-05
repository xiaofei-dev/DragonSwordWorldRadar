#pragma once

#include <cstdint>
#include <limits>

namespace dswros {

enum class WorldMapCandidateTransition : std::uint8_t {
    RetainIdentity,
    ReplaceIdentity,
    BeginOpenSession,
    ReplaceAndBeginOpenSession,
    DuplicateOpenEvent,
};

struct WorldMapCandidateEvent final {
    bool candidate_available{};
    bool same_candidate{};
    bool set_image_event{};
    bool has_serial_bound_open_evidence{};
};

[[nodiscard]] constexpr WorldMapCandidateTransition
world_map_candidate_transition(WorldMapCandidateEvent event) noexcept {
    if (!event.candidate_available) {
        return WorldMapCandidateTransition::RetainIdentity;
    }
    if (!event.same_candidate) {
        return event.set_image_event
            ? WorldMapCandidateTransition::ReplaceAndBeginOpenSession
            : WorldMapCandidateTransition::ReplaceIdentity;
    }
    if (!event.set_image_event) {
        return WorldMapCandidateTransition::RetainIdentity;
    }
    return event.has_serial_bound_open_evidence
        ? WorldMapCandidateTransition::DuplicateOpenEvent
        : WorldMapCandidateTransition::BeginOpenSession;
}

[[nodiscard]] constexpr bool world_map_candidate_replaced(
    WorldMapCandidateTransition transition) noexcept {
    return transition == WorldMapCandidateTransition::ReplaceIdentity
        || transition
            == WorldMapCandidateTransition::ReplaceAndBeginOpenSession;
}

[[nodiscard]] constexpr bool world_map_open_session_started(
    WorldMapCandidateTransition transition) noexcept {
    return transition == WorldMapCandidateTransition::BeginOpenSession
        || transition
            == WorldMapCandidateTransition::ReplaceAndBeginOpenSession;
}

[[nodiscard]] constexpr std::uint64_t next_world_map_candidate_serial(
    std::uint64_t current_serial) noexcept {
    return current_serial == std::numeric_limits<std::uint64_t>::max()
        ? 1U
        : current_serial + 1U;
}

[[nodiscard]] constexpr bool world_map_serial_matches(
    bool candidate_available,
    std::uint64_t candidate_serial,
    std::uint64_t open_serial) noexcept {
    return candidate_available && candidate_serial != 0U
        && open_serial == candidate_serial;
}

[[nodiscard]] constexpr bool world_map_listener_open_probe_allowed(
    bool enabled,
    bool transition_active,
    bool activity_suppressed,
    bool exact_current_world) noexcept {
    return enabled && !transition_active && !activity_suppressed
        && exact_current_world;
}

enum class WorldMapVisibilitySample : std::uint8_t {
    Unknown,
    Hidden,
    Visible,
};

enum class WorldMapVisibilityAction : std::uint8_t {
    Ignore,
    Preserve,
    ConfirmVisible,
    CloseSession,
};

struct WorldMapVisibilityContext final {
    bool candidate_available{};
    bool exact_candidate_live{};
    bool current_world{};
    bool has_serial_bound_open_evidence{};
    bool previously_confirmed_visible{};
    bool compact_open_latched{};
    std::uint64_t milliseconds_since_open{};
    std::uint64_t open_visibility_grace_milliseconds{};
};

struct WorldMapVisibilityDecision final {
    WorldMapVisibilityAction action{WorldMapVisibilityAction::Ignore};
    bool recovery_edge{};
};

[[nodiscard]] constexpr WorldMapVisibilityDecision
decide_world_map_visibility(
    WorldMapVisibilityContext context,
    WorldMapVisibilitySample sample) noexcept {
    if (!context.candidate_available || !context.exact_candidate_live
        || !context.current_world
        || !context.has_serial_bound_open_evidence) {
        return {};
    }
    if (sample == WorldMapVisibilitySample::Unknown) {
        return {WorldMapVisibilityAction::Preserve, false};
    }
    if (sample == WorldMapVisibilitySample::Visible) {
        return {
            WorldMapVisibilityAction::ConfirmVisible,
            !context.previously_confirmed_visible
                || !context.compact_open_latched};
    }
    if (context.previously_confirmed_visible
        || context.milliseconds_since_open
            >= context.open_visibility_grace_milliseconds) {
        return {WorldMapVisibilityAction::CloseSession, false};
    }
    return {WorldMapVisibilityAction::Preserve, false};
}

enum class WorldMapWorkGate : std::uint8_t {
    Ignore,
    Hold,
    CloseSession,
    Exhausted,
    ConsumeAttempt,
};

struct WorldMapWorkContext final {
    bool content_intent{};
    bool candidate_available{};
    bool exact_candidate_live{};
    bool current_world{};
    bool has_serial_bound_open_evidence{};
    WorldMapVisibilitySample visibility{WorldMapVisibilitySample::Unknown};
    std::uint32_t attempts{};
    std::uint32_t maximum_attempts{};
};

[[nodiscard]] constexpr WorldMapWorkGate world_map_work_gate(
    WorldMapWorkContext context) noexcept {
    if (!context.content_intent || !context.candidate_available
        || !context.has_serial_bound_open_evidence) {
        return WorldMapWorkGate::Ignore;
    }
    if (!context.exact_candidate_live || !context.current_world
        || context.visibility == WorldMapVisibilitySample::Unknown) {
        return WorldMapWorkGate::Hold;
    }
    if (context.visibility == WorldMapVisibilitySample::Hidden) {
        return WorldMapWorkGate::CloseSession;
    }
    if (context.maximum_attempts == 0U
        || context.attempts >= context.maximum_attempts) {
        return WorldMapWorkGate::Exhausted;
    }
    return WorldMapWorkGate::ConsumeAttempt;
}

enum class WorldMapF7RearmAction : std::uint8_t {
    Preserve,
    Rearm,
};

[[nodiscard]] constexpr WorldMapF7RearmAction world_map_f7_rearm_action(
    bool candidate_available,
    bool exact_candidate_live,
    bool current_world,
    bool has_serial_bound_open_evidence,
    WorldMapVisibilitySample visibility) noexcept {
    return candidate_available && exact_candidate_live && current_world
            && has_serial_bound_open_evidence
            && visibility == WorldMapVisibilitySample::Visible
        ? WorldMapF7RearmAction::Rearm
        : WorldMapF7RearmAction::Preserve;
}

[[nodiscard]] constexpr bool preserve_world_map_evidence_on_f8(
    bool candidate_available,
    bool exact_candidate_live,
    bool current_world,
    bool has_serial_bound_open_evidence) noexcept {
    return candidate_available && exact_candidate_live && current_world
        && has_serial_bound_open_evidence;
}

} // namespace dswros
