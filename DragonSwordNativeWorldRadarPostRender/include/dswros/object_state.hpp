#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dswros {

enum class EventKind : std::uint32_t {
    TreasureOpened = 1,
    EncounterDefeated = 2,
};

struct Position {
    double x{};
    double y{};
    double z{};
};

struct WeakIdentity {
    std::int32_t index{-1};
    std::int32_t serial{};

    [[nodiscard]] bool valid() const noexcept { return index >= 0 && serial > 0; }
    [[nodiscard]] std::uint64_t packed() const noexcept {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(index)) << 32U)
            | static_cast<std::uint32_t>(serial);
    }
};

struct CatalogPoint {
    std::int64_t id{};
    std::string class_name;
    Position position{};
};

struct StateEvent {
    EventKind kind{};
    std::int64_t id{};
};

class DisappearanceConfirmation {
public:
    static constexpr std::uint32_t kRequiredMissingSamples = 2;

    void reset() noexcept { missing_samples_ = 0; }

    [[nodiscard]] bool sample(bool present, bool context_valid) noexcept {
        if (present || !context_valid) {
            missing_samples_ = 0;
            return false;
        }
        if (missing_samples_ < kRequiredMissingSamples) ++missing_samples_;
        return missing_samples_ >= kRequiredMissingSamples;
    }

    void mark_eligible_end() noexcept {
        if (missing_samples_ == 0) missing_samples_ = 1;
    }

    [[nodiscard]] std::uint32_t missing_samples() const noexcept { return missing_samples_; }

private:
    std::uint32_t missing_samples_{};
};

// Runtime-only markers such as bird eggs have no persistent catalog state.
// A live object makes the marker visible immediately, while a short missing
// edge is debounced so streaming or one transient weak lookup cannot flicker
// the compact radar. The caller owns lifecycle/world validation separately.
class LiveMarkerPresenceGate {
public:
    static constexpr std::int64_t kMissingDebounceMilliseconds = 400;

    void reset() noexcept {
        visible_ = false;
        missing_since_milliseconds_.reset();
    }

    [[nodiscard]] bool sample(
        bool present, std::int64_t now_milliseconds) noexcept {
        if (present) {
            visible_ = true;
            missing_since_milliseconds_.reset();
            return true;
        }
        if (!visible_) {
            missing_since_milliseconds_.reset();
            return false;
        }
        if (!missing_since_milliseconds_
            || now_milliseconds < *missing_since_milliseconds_) {
            missing_since_milliseconds_ = now_milliseconds;
            return true;
        }
        if (now_milliseconds - *missing_since_milliseconds_
            < kMissingDebounceMilliseconds) {
            return true;
        }
        reset();
        return false;
    }

    [[nodiscard]] bool visible() const noexcept { return visible_; }
    [[nodiscard]] bool missing_pending() const noexcept {
        return missing_since_milliseconds_.has_value();
    }

private:
    std::optional<std::int64_t> missing_since_milliseconds_{};
    bool visible_{};
};

// Bird-egg actors expose their actual collection state through the owned
// DInteractableComponent. Actor-level visibility is intentionally excluded:
// these Blueprint actors may stay hidden while their interaction component is
// live. Unknown reflection reads fail closed and are retried by the bounded
// runtime candidate service.
inline constexpr std::uint64_t kBirdEggRequiredInteractableValue = 2;
inline constexpr std::uint64_t kBirdEggRequiredInteractTypeValue = 2;

[[nodiscard]] constexpr bool bird_egg_interaction_available(
    std::uint64_t interactable_value,
    std::uint64_t interact_type_value) noexcept {
    return interactable_value == kBirdEggRequiredInteractableValue
        && interact_type_value == kBirdEggRequiredInteractTypeValue;
}

[[nodiscard]] constexpr bool bird_egg_runtime_present(
    bool weak_identity_valid,
    bool same_world,
    bool availability_known,
    bool interactable) noexcept {
    return weak_identity_valid && same_world && availability_known
        && interactable;
}

[[nodiscard]] constexpr bool is_main_menu_world_identity(
    std::string_view world_identity) noexcept {
    return world_identity
        == "World /Game/Title/TitleMap/DS_Title.DS_Title";
}

class EncounterDisappearanceConfirmation {
public:
    static constexpr std::uint32_t kRequiredPresentSamples = 4;
    static constexpr std::uint32_t kRequiredMissingSamples = 40;
    static constexpr std::int64_t kRequiredPresentMilliseconds = 1'000;
    static constexpr std::int64_t kRequiredMissingMilliseconds = 10'000;

    void reset() noexcept {
        present_samples_ = 0;
        missing_samples_ = 0;
        present_since_milliseconds_.reset();
        missing_since_milliseconds_.reset();
        armed_ = false;
    }

    [[nodiscard]] bool sample(
        bool present, bool context_valid, bool destroyed_end,
        std::int64_t now_milliseconds) noexcept {
        if (!context_valid) {
            reset();
            return false;
        }
        if (destroyed_end) {
            return true;
        }
        if (present) {
            missing_samples_ = 0;
            missing_since_milliseconds_.reset();
            if (!armed_) {
                if (!present_since_milliseconds_
                    || now_milliseconds < *present_since_milliseconds_) {
                    present_since_milliseconds_ = now_milliseconds;
                    present_samples_ = 1;
                } else if (present_samples_ < kRequiredPresentSamples) {
                    ++present_samples_;
                }
                armed_ = present_samples_ >= kRequiredPresentSamples
                    && now_milliseconds - *present_since_milliseconds_
                        >= kRequiredPresentMilliseconds;
            }
            return false;
        }
        present_samples_ = 0;
        present_since_milliseconds_.reset();
        if (!armed_) {
            return false;
        }
        if (!missing_since_milliseconds_
            || now_milliseconds < *missing_since_milliseconds_) {
            missing_since_milliseconds_ = now_milliseconds;
            missing_samples_ = 1;
            return false;
        }
        if (missing_samples_ < kRequiredMissingSamples) {
            ++missing_samples_;
        }
        return missing_samples_ >= kRequiredMissingSamples
            && now_milliseconds - *missing_since_milliseconds_
                >= kRequiredMissingMilliseconds;
    }

    // An encounter can reach RemovedFromWorld before the 250 ms observer has
    // accumulated the ordinary one-second presence history (for example, a
    // short Assault killed immediately after entering the 100-metre window).
    // The exact already-observed EndPlay edge may start, but never complete,
    // the same ten-second missing window. A returned actor still cancels it.
    void arm_observed_end_fallback(
        std::int64_t now_milliseconds) noexcept {
        armed_ = true;
        present_samples_ = 0;
        present_since_milliseconds_.reset();
        if (!missing_since_milliseconds_
            || now_milliseconds < *missing_since_milliseconds_) {
            missing_since_milliseconds_ = now_milliseconds;
            missing_samples_ = 1;
        }
    }

    [[nodiscard]] bool pending() const noexcept {
        return missing_samples_ != 0;
    }

private:
    std::uint32_t present_samples_{};
    std::uint32_t missing_samples_{};
    std::optional<std::int64_t> present_since_milliseconds_{};
    std::optional<std::int64_t> missing_since_milliseconds_{};
    bool armed_{};
};

struct UnobservedEncounterEndEvidence {
    bool destroyed{};
    bool transition_active{};
    bool activity_suppressed{};
    bool player_position_valid{};
    bool actor_position_valid{};
    bool encounter_state_ready{};
    bool encounter_available{};
    bool exact_catalog_class{};
    bool player_near_actor{};
    bool weak_identity_valid{};
    bool duplicate_observation{};
};

struct EncounterDisappearanceEvidence {
    bool lifecycle_valid{};
    bool destroyed_end{};
    bool visible_seen{};
    bool player_near_last_actor{};
};

struct EncounterDeathNotificationEvidence {
    bool lifecycle_valid{};
    bool encounter_currently_available{};
    bool exact_observed_actor{};
    bool exact_catalog_class{};
    bool monster_character{};
    bool visible_seen{};
    bool player_near_actor{};
};

struct PendingEncounterDeathConsumptionContext {
    bool lifecycle_boundary{};
    bool enabled{};
    bool encounter_state_ready{};
    bool transition_active{};
    bool activity_suppressed{};
    bool player_position_valid{};
};

// RemovedFromWorld is not defeat evidence by itself, but an already observed
// encounter must keep its numeric disappearance gate after the UObject reaches
// its EndPlay boundary. The later ten-second, nearby, continuously missing
// confirmation remains the only path that can publish completion.
[[nodiscard]] constexpr bool preserve_observed_encounter_removal(
    bool observed_encounter, bool removed_from_world,
    bool lifecycle_valid) noexcept {
    return observed_encounter && removed_from_world && lifecycle_valid;
}

// A cursor-owning map/menu must reset an ordinary weak-pointer disappearance,
// but it must not erase an exact EndPlay boundary that already released the
// UObject and retained numeric evidence only. Travel/activity suppression are
// still represented by the separate lifecycle gate and remain fatal.
[[nodiscard]] constexpr bool encounter_cursor_context_allowed(
    bool mouse_cursor_visible, bool logical_end) noexcept {
    return !mouse_cursor_visible || logical_end;
}

// Weak-pointer disappearance is only meaningful while the player still owns
// the actor's local streaming context. Leaving that context must evict the
// observation instead of converting a streamed-out encounter into a defeat.
[[nodiscard]] constexpr bool accept_encounter_disappearance(
    const EncounterDisappearanceEvidence& evidence) noexcept {
    return evidence.lifecycle_valid
        && evidence.player_near_last_actor
        && (evidence.destroyed_end || evidence.visible_seen);
}

// The game-owned death notification is primary encounter completion evidence,
// but only for the exact actor already bound to a current catalog row. The
// receiver, class, lifecycle, availability, visibility, and local streaming
// context must all agree; no notification parameter is interpreted.
[[nodiscard]] constexpr bool accept_encounter_death_notification(
    const EncounterDeathNotificationEvidence& evidence) noexcept {
    return evidence.lifecycle_valid
        && evidence.encounter_currently_available
        && evidence.exact_observed_actor
        && evidence.exact_catalog_class
        && evidence.monster_character
        && evidence.visible_seen
        && evidence.player_near_actor;
}

[[nodiscard]] constexpr bool encounter_death_process_is_terminal(
    std::int64_t process_state) noexcept {
    // DENM_ProcessState::End. Keep the reflected game enum out of this
    // UObject-free policy header while still testing the exact accepted edge.
    return process_state == 3;
}

// A published bit already passed the exact actor, catalog, availability,
// visibility, and proximity checks. Ordinary control work waits for a valid
// active context. An authoritative reset edge may preserve that numeric proof
// even after Engine Tick has failed closed and lowered enabled, but it still
// requires the encounter catalog/state to have been established.
[[nodiscard]] constexpr bool can_consume_pending_encounter_death(
    const PendingEncounterDeathConsumptionContext& context) noexcept {
    return context.encounter_state_ready
        && (context.lifecycle_boundary
            || (context.enabled
                && !context.transition_active
                && !context.activity_suppressed
                && context.player_position_valid));
}

[[nodiscard]] constexpr bool accept_unobserved_encounter_end(
    const UnobservedEncounterEndEvidence& evidence) noexcept {
    return evidence.destroyed
        && !evidence.transition_active
        && !evidence.activity_suppressed
        && evidence.player_position_valid
        && evidence.actor_position_valid
        && evidence.encounter_state_ready
        && evidence.encounter_available
        && evidence.exact_catalog_class
        && evidence.player_near_actor
        && evidence.weak_identity_valid
        && !evidence.duplicate_observation;
}

[[nodiscard]] constexpr bool encounter_cooldown_write_allowed(
    bool has_cooldown, std::int64_t next_available_unix_seconds,
    std::int64_t now_unix_seconds) noexcept {
    return !has_cooldown
        || now_unix_seconds >= next_available_unix_seconds;
}

[[nodiscard]] constexpr bool encounter_available_now(
    bool state_ready, bool time_condition_matches,
    bool has_cooldown, std::int64_t next_available_unix_seconds,
    std::int64_t now_unix_seconds) noexcept {
    return state_ready && time_condition_matches
        && encounter_cooldown_write_allowed(
            has_cooldown, next_available_unix_seconds,
            now_unix_seconds);
}

// Rendering may expose the immutable Assault catalog without weakening the
// authoritative completion/cooldown path. ALL is a presentation-only static
// view: it bypasses state readiness, live time conditions, and cooldowns for
// Assaults only. Bosses and AVAILABLE mode retain every normal gate.
[[nodiscard]] constexpr bool encounter_visible_for_display_mode(
    bool state_ready, bool is_assault, bool show_all_assaults,
    bool time_condition_matches, bool has_cooldown,
    std::int64_t next_available_unix_seconds,
    std::int64_t now_unix_seconds) noexcept {
    if (is_assault && show_all_assaults) {
        return true;
    }
    return encounter_available_now(
        state_ready, time_condition_matches, has_cooldown,
        next_available_unix_seconds, now_unix_seconds);
}

[[nodiscard]] constexpr bool encounter_activity_edge_requires_reset(
    bool suppression_changed, bool activity_suppressed) noexcept {
    return suppression_changed && activity_suppressed;
}

[[nodiscard]] constexpr std::int64_t merge_encounter_cooldown(
    std::int64_t current_next_available_unix_seconds,
    std::int64_t candidate_next_available_unix_seconds) noexcept {
    return current_next_available_unix_seconds
               < candidate_next_available_unix_seconds
        ? candidate_next_available_unix_seconds
        : current_next_available_unix_seconds;
}

[[nodiscard]] constexpr bool accept_treasure_save_confirmation(
    bool requested,
    bool query_succeeded,
    bool exact_open_bit) noexcept {
    return requested && query_succeeded && exact_open_bit;
}

// Mounted underwater treasure interaction is emitted by the local rider while
// the PlayerController Pawn is the vehicle. Accept that alternate identity
// only when the receiver is a mount-only treasure and the callback actor is
// the exact Rider UObject owned by the fresh local Pawn in the same world.
[[nodiscard]] constexpr bool accept_mounted_treasure_interactor(
    bool mount_only_treasure,
    bool exact_current_pawn_rider,
    bool rider_world_matches_pawn,
    bool exact_receiver_near_local_player) noexcept {
    return mount_only_treasure
        && exact_current_pawn_rider
        && rider_world_matches_pawn
        && exact_receiver_near_local_player;
}

[[nodiscard]] constexpr bool retry_treasure_save_confirmation(
    bool requested,
    bool accepted,
    std::uint8_t completed_attempts,
    std::uint8_t maximum_attempts) noexcept {
    return requested && !accepted && completed_attempts > 0
        && completed_attempts < maximum_attempts;
}

class ObjectStateTracker {
public:
    static constexpr double kMatchRadiusXY = 600.0;
    static constexpr double kMatchRadiusZ = 600.0;
    static constexpr double kCompletionRadius = 3000.0;

    void set_catalog(std::vector<CatalogPoint> catalog) {
        catalog_ = std::move(catalog);
        observed_.clear();
    }

    void reset(std::uint32_t activation, std::uint32_t epoch) noexcept {
        activation_ = activation;
        epoch_ = epoch;
        observed_.clear();
    }

    [[nodiscard]] std::optional<std::int64_t> observe(
        WeakIdentity identity,
        const std::string& class_name,
        Position actor_position) {
        if (!identity.valid() || !finite(actor_position)) return std::nullopt;
        const CatalogPoint* match = nearest(class_name, actor_position);
        if (!match) return std::nullopt;
        observed_[identity.packed()] = Observed{match->id, match->position, activation_, epoch_};
        return match->id;
    }

    // Prefer the game-owned numeric prop identity when it is available, but
    // never trust that scalar by itself. The catalog class and full 3D
    // position must describe the same unique row; a conflicting runtime
    // ObjectID fails closed instead of falling back to a nearby chest.
    [[nodiscard]] std::optional<std::int64_t> observe_reported_id(
        WeakIdentity identity,
        const std::string& class_name,
        Position actor_position,
        std::int64_t reported_id) {
        if (!identity.valid() || reported_id <= 0 || !finite(actor_position)) {
            return std::nullopt;
        }
        const CatalogPoint* match{};
        for (const CatalogPoint& point : catalog_) {
            if (point.id != reported_id) continue;
            if (match || point.class_name != class_name
                || !within_match_bounds(point.position, actor_position)) {
                return std::nullopt;
            }
            match = &point;
        }
        if (!match) return std::nullopt;
        observed_[identity.packed()] = Observed{
            match->id, match->position, activation_, epoch_};
        return match->id;
    }

    [[nodiscard]] std::optional<StateEvent> end(
        WeakIdentity identity,
        bool destroyed,
        bool transition_active,
        Position player_position,
        std::uint32_t activation,
        std::uint32_t epoch) {
        const auto found = observed_.find(identity.packed());
        if (found == observed_.end()) return std::nullopt;
        const Observed observed = found->second;
        observed_.erase(found);
        if (!destroyed || transition_active || activation != observed.activation
            || epoch != observed.epoch || !finite(player_position)
            || distance_squared(player_position, observed.position) > kCompletionRadius * kCompletionRadius) {
            return std::nullopt;
        }
        return StateEvent{EventKind::TreasureOpened, observed.id};
    }

    [[nodiscard]] std::vector<std::string> nearby_classes(Position player, double radius) const {
        std::vector<std::string> result;
        if (!finite(player) || !std::isfinite(radius) || radius <= 0.0) return result;
        const double radius_squared = radius * radius;
        for (const auto& point : catalog_) {
            if (distance_squared(player, point.position) > radius_squared) continue;
            if (std::find(result.begin(), result.end(), point.class_name) == result.end()) {
                result.push_back(point.class_name);
            }
        }
        return result;
    }

    [[nodiscard]] std::size_t observed_count() const noexcept { return observed_.size(); }
    [[nodiscard]] std::size_t catalog_count() const noexcept { return catalog_.size(); }

    [[nodiscard]] std::optional<CatalogPoint> nearest_point(Position player, double radius) const noexcept {
        if (!finite(player) || !std::isfinite(radius) || radius <= 0.0) return std::nullopt;
        const double radius_squared = radius * radius;
        const CatalogPoint* result{};
        double best = std::numeric_limits<double>::max();
        for (const auto& point : catalog_) {
            const double candidate = distance_squared(player, point.position);
            if (candidate <= radius_squared && candidate < best) {
                result = &point;
                best = candidate;
            }
        }
        return result ? std::optional<CatalogPoint>{*result} : std::nullopt;
    }

    [[nodiscard]] std::optional<CatalogPoint> nearest_planar_point_in_annulus(
        Position player, double minimum_radius, double maximum_radius) const noexcept {
        if (!finite(player) || !std::isfinite(minimum_radius) || minimum_radius < 0.0
            || !std::isfinite(maximum_radius) || maximum_radius <= minimum_radius) {
            return std::nullopt;
        }
        const double minimum_squared = minimum_radius * minimum_radius;
        const double maximum_squared = maximum_radius * maximum_radius;
        const CatalogPoint* result{};
        double best = std::numeric_limits<double>::max();
        for (const auto& point : catalog_) {
            const double delta_x = player.x - point.position.x;
            const double delta_y = player.y - point.position.y;
            const double candidate = delta_x * delta_x + delta_y * delta_y;
            if (candidate >= minimum_squared && candidate <= maximum_squared && candidate < best) {
                result = &point;
                best = candidate;
            }
        }
        return result ? std::optional<CatalogPoint>{*result} : std::nullopt;
    }

private:
    struct Observed {
        std::int64_t id{};
        Position position{};
        std::uint32_t activation{};
        std::uint32_t epoch{};
    };

    [[nodiscard]] const CatalogPoint* nearest(
        const std::string& class_name,
        Position actor_position) const noexcept {
        const CatalogPoint* result{};
        double best_distance_squared = std::numeric_limits<double>::max();
        for (const auto& point : catalog_) {
            if (point.class_name != class_name) continue;
            const double dx = actor_position.x - point.position.x;
            const double dy = actor_position.y - point.position.y;
            const double dz = std::abs(actor_position.z - point.position.z);
            const double xy_squared = dx * dx + dy * dy;
            const double distance = xy_squared + dz * dz;
            if (xy_squared <= kMatchRadiusXY * kMatchRadiusXY
                && dz <= kMatchRadiusZ
                && distance < best_distance_squared) {
                result = &point;
                best_distance_squared = distance;
            }
        }
        return result;
    }

    [[nodiscard]] static bool finite(Position value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    [[nodiscard]] static bool within_match_bounds(
        Position catalog_position, Position actor_position) noexcept {
        const double dx = catalog_position.x - actor_position.x;
        const double dy = catalog_position.y - actor_position.y;
        const double dz = catalog_position.z - actor_position.z;
        return dx * dx + dy * dy <= kMatchRadiusXY * kMatchRadiusXY
            && std::abs(dz) <= kMatchRadiusZ;
    }

    [[nodiscard]] static double distance_squared(Position left, Position right) noexcept {
        const double dx = left.x - right.x;
        const double dy = left.y - right.y;
        const double dz = left.z - right.z;
        return dx * dx + dy * dy + dz * dz;
    }

    std::vector<CatalogPoint> catalog_{};
    std::unordered_map<std::uint64_t, Observed> observed_{};
    std::uint32_t activation_{};
    std::uint32_t epoch_{};
};

} // namespace dswros
