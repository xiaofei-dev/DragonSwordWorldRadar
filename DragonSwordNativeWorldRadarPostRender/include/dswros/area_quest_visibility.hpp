#pragma once

#include <cstdint>

namespace dswros {

enum class AreaQuestState : std::uint8_t {
    None = 0,
    Acceptable = 1,
    Progress = 2,
    End = 3,
    Fail = 4,
    Unknown = 0xFF,
};

enum class AreaQuestEligibilityProof : std::uint8_t {
    Unknown = 0,
    Eligible = 1,
    Ineligible = 2,
};

inline constexpr std::int64_t kUnknownAreaQuestSaveCompletionCount = -1;

[[nodiscard]] constexpr bool area_quest_compact_visible(
    AreaQuestState state) noexcept {
    return state == AreaQuestState::Acceptable
        || state == AreaQuestState::Progress;
}

[[nodiscard]] constexpr bool area_quest_completion_transition(
    AreaQuestState previous_state,
    AreaQuestState current_state) noexcept {
    // The generic blueprint-end callbacks also run after unrelated treasure
    // and mini-game teardown. Runtime evidence shows that path can expose a
    // transient PROGRESS -> FAIL sample for an otherwise available task.
    // END is the only terminal state that proves a completed active task.
    return previous_state == AreaQuestState::Progress
        && current_state == AreaQuestState::End;
}

[[nodiscard]] constexpr bool
area_quest_witnessed_completion_transition(
    bool exact_quest_identity_witnessed,
    AreaQuestState previous_state,
    AreaQuestState current_state) noexcept {
    // Some delivery-style dynamic quests advance directly from an unpublished
    // state to END. Once a native event or the exact EndPlayActor class proves
    // the catalog quest identity, END is authoritative even if the retained
    // snapshot was NONE, FAIL, ACCEPTABLE, or otherwise stale. Unwitnessed END
    // remains fail closed, so unrelated blueprint teardown cannot complete a
    // different task.
    (void)previous_state;
    return exact_quest_identity_witnessed
        && current_state == AreaQuestState::End;
}

[[nodiscard]] constexpr bool area_quest_active_sample_may_rearm(
    std::uint64_t scan_start_completion_revision,
    std::uint64_t current_completion_revision,
    bool inactive_state_observed_after_completion) noexcept {
    // A task completion can arrive while the transactional 147-item refresh
    // is still reading an older state snapshot. Only a scan that started at
    // the current exact-completion revision may treat ACCEPTABLE/PROGRESS as
    // proof that a repeatable task has become active again.
    return scan_start_completion_revision == current_completion_revision
        && inactive_state_observed_after_completion;
}

[[nodiscard]] constexpr bool area_quest_inactive_sample_arms_reactivation(
    AreaQuestState state) noexcept {
    // FAIL is deliberately excluded: unrelated blueprint teardown has exposed
    // transient FAIL samples. NONE or END after an exact completion proves the
    // old cycle is no longer active before a later active state can rearm it.
    return state == AreaQuestState::None || state == AreaQuestState::End;
}

[[nodiscard]] constexpr bool accept_area_quest_save_confirmation(
    bool exact_quest_requested,
    bool completion_query_available,
    bool completion_identity_ambiguous,
    bool exact_quest_id_matches,
    std::int64_t verified_baseline_count,
    std::int64_t complete_count) noexcept {
    return exact_quest_requested
        && completion_query_available
        && !completion_identity_ambiguous
        && exact_quest_id_matches
        && verified_baseline_count
            != kUnknownAreaQuestSaveCompletionCount
        && complete_count > verified_baseline_count;
}

[[nodiscard]] constexpr bool area_quest_completion_generation_may_arm(
    bool generation_locked) noexcept {
    return !generation_locked;
}

[[nodiscard]] constexpr bool
area_quest_completion_generation_arms_reactivation(
    bool generation_locked,
    AreaQuestState state) noexcept {
    return generation_locked
        && area_quest_inactive_sample_arms_reactivation(state);
}

[[nodiscard]] constexpr bool
area_quest_completion_generation_may_unlock(
    bool generation_locked,
    bool reactivation_armed,
    AreaQuestState state) noexcept {
    return generation_locked
        && reactivation_armed
        && area_quest_compact_visible(state);
}

[[nodiscard]] constexpr bool retry_area_quest_save_confirmation(
    bool exact_quest_requested,
    bool completion_accepted,
    std::uint8_t completed_attempts,
    std::uint8_t maximum_attempts) noexcept {
    return exact_quest_requested
        && !completion_accepted
        && completed_attempts < maximum_attempts;
}

[[nodiscard]] constexpr bool area_quest_world_map_visible(
    AreaQuestState state,
    bool completion_snapshot_available,
    bool completed_in_snapshot,
    bool completion_observed,
    AreaQuestEligibilityProof eligibility_proof) noexcept {
    if (state == AreaQuestState::Acceptable
        || state == AreaQuestState::Progress) {
        return true;
    }
    // END/FAIL is sampled through a global dynamic-quest query after a generic
    // blueprint teardown callback. Nearby mini-games can transiently expose
    // those values for an unrelated available task, so terminal state alone is
    // not completion evidence. A saved completion, a proven PROGRESS -> END
    // transition, or an exact catalog ID from the reflected task-class map
    // remains authoritative below.
    if (completion_observed) {
        return false;
    }
    return completion_snapshot_available
        && !completed_in_snapshot
        && eligibility_proof == AreaQuestEligibilityProof::Eligible;
}

[[nodiscard]] constexpr bool area_quest_visible_for_display_mode(
    bool show_all_unfinished,
    bool available_mode_visible,
    AreaQuestState state,
    bool completion_snapshot_available,
    bool completed_in_snapshot,
    bool completion_observed) noexcept {
    if (!show_all_unfinished) {
        return available_mode_visible;
    }
    // A currently acceptable/progressing repeatable task is actionable even
    // when the historical completion snapshot contains an older cycle.
    if (state == AreaQuestState::Acceptable
        || state == AreaQuestState::Progress) {
        return true;
    }
    if (completion_observed) {
        return false;
    }
    // ALL is an explicit user-selected diagnostic/completion mode. When the
    // snapshot is unavailable, fail open for display only; no task state is
    // mutated and normal AVAILABLE mode remains fail closed.
    return !completion_snapshot_available || !completed_in_snapshot;
}

} // namespace dswros
