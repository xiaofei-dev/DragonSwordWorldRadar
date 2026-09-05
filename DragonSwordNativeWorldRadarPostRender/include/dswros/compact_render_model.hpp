#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include <dswros/object_state.hpp>

namespace dswros {

enum class CompactTimePhase : std::uint8_t {
    Morning,
    Afternoon,
    Evening,
    Night,
};

enum class AreaQuestHeightIndicatorShape : std::uint8_t {
    Aligned,
    Above,
    Below,
    Unavailable,
};

enum class MiniGameHeightIndicatorShape : std::uint8_t {
    Hidden,
    Above,
    Below,
};

// Mini-game guidance consumes the already dead-zone-adjusted angle produced by
// CompactRenderModel::mini_game_height_angle_from_delta. Keeping this state mapping pure
// lets the renderer switch only on a discrete edge: no shaft rotation, UObject
// lookup, allocation, or geometry churn is needed while the player remains on
// the same side of the target height.
[[nodiscard]] inline MiniGameHeightIndicatorShape
mini_game_height_indicator_shape(double height_angle_degrees) noexcept {
    if (!std::isfinite(height_angle_degrees)
        || std::abs(height_angle_degrees) <= 1.0e-9) {
        return MiniGameHeightIndicatorShape::Hidden;
    }
    return height_angle_degrees < 0.0
        ? MiniGameHeightIndicatorShape::Above
        : MiniGameHeightIndicatorShape::Below;
}

inline constexpr std::size_t kAreaQuestHeightBandCapacity = 2;

struct AreaQuestHeightBand {
    double minimum_z{};
    double maximum_z{};
};

struct AreaQuestHeightProfile {
    std::array<AreaQuestHeightBand, kAreaQuestHeightBandCapacity> bands{};
    std::uint8_t band_count{};
};

// Area Quest height guidance is intentionally discrete: the existing task
// square means the player is within the accepted vertical band, while a
// triangle communicates only the remaining up/down direction. Area Quests
// deliberately use a wider band than the continuous Treasure/Mole arrows.
// Untrusted/non-finite sources fail closed and must never show the aligned dots.
inline constexpr double kAreaQuestHeightDeadZone = 500.0;

[[nodiscard]] inline bool area_quest_height_profile_valid(
    const AreaQuestHeightProfile& profile) noexcept {
    if (profile.band_count == 0
        || profile.band_count > profile.bands.size()) {
        return false;
    }
    for (std::size_t index = 0; index < profile.band_count; ++index) {
        const AreaQuestHeightBand& band = profile.bands[index];
        if (!std::isfinite(band.minimum_z)
            || !std::isfinite(band.maximum_z)
            || band.minimum_z > band.maximum_z
            || (index > 0
                && profile.bands[index - 1].maximum_z
                    >= band.minimum_z)) {
            return false;
        }
    }
    return true;
}

// A catalog marker can reference more than one task actor when a quest moves
// between stages.  The marker's authored Z is independent evidence of which
// actor band owns that map location.  Collapse a multi-band profile to the
// uniquely nearest band before comparing it with the player; an exact tie is
// ambiguous and fails closed instead of inventing a direction.
[[nodiscard]] inline AreaQuestHeightProfile
area_quest_height_profile_for_marker(
    const AreaQuestHeightProfile& profile,
    double marker_z) noexcept {
    if (!area_quest_height_profile_valid(profile)
        || !std::isfinite(marker_z)) {
        return {};
    }
    if (profile.band_count == 1) {
        return profile;
    }

    std::size_t nearest_index{};
    double nearest_distance = std::numeric_limits<double>::infinity();
    bool tie{};
    for (std::size_t index = 0; index < profile.band_count; ++index) {
        const AreaQuestHeightBand& band = profile.bands[index];
        const double distance = marker_z < band.minimum_z
            ? band.minimum_z - marker_z
            : (marker_z > band.maximum_z
                ? marker_z - band.maximum_z
                : 0.0);
        if (distance < nearest_distance) {
            nearest_index = index;
            nearest_distance = distance;
            tie = false;
        } else if (distance == nearest_distance) {
            tie = true;
        }
    }
    if (tie || !std::isfinite(nearest_distance)) {
        return {};
    }
    AreaQuestHeightProfile selected{};
    selected.bands[0] = profile.bands[nearest_index];
    selected.band_count = 1;
    return selected;
}

[[nodiscard]] inline AreaQuestHeightIndicatorShape
area_quest_height_indicator_shape(
    const AreaQuestHeightProfile& profile,
    double comparable_player_z) noexcept {
    if (!std::isfinite(comparable_player_z)
        || !area_quest_height_profile_valid(profile)) {
        return AreaQuestHeightIndicatorShape::Unavailable;
    }

    bool candidate_above{};
    bool candidate_below{};
    for (std::size_t index = 0; index < profile.band_count; ++index) {
        const AreaQuestHeightBand& band = profile.bands[index];
        if (comparable_player_z
                >= band.minimum_z - kAreaQuestHeightDeadZone
            && comparable_player_z
                <= band.maximum_z + kAreaQuestHeightDeadZone) {
            return AreaQuestHeightIndicatorShape::Aligned;
        }
        candidate_above = candidate_above
            || band.minimum_z
                > comparable_player_z + kAreaQuestHeightDeadZone;
        candidate_below = candidate_below
            || band.maximum_z
                < comparable_player_z - kAreaQuestHeightDeadZone;
    }
    if (candidate_above && !candidate_below) {
        return AreaQuestHeightIndicatorShape::Above;
    }
    if (candidate_below && !candidate_above) {
        return AreaQuestHeightIndicatorShape::Below;
    }
    return AreaQuestHeightIndicatorShape::Unavailable;
}

// Presentation bands derived from the proven numeric game clock. The game has
// not exposed an authoritative reflected enum or schedule naming these bands,
// so these boundaries must not be described as official gameplay phases.
[[nodiscard]] constexpr CompactTimePhase compact_time_phase(
    std::uint32_t seconds) noexcept {
    const std::uint32_t day_seconds = seconds % 86'400U;
    if (day_seconds >= 6U * 3600U && day_seconds < 12U * 3600U) {
        return CompactTimePhase::Morning;
    }
    if (day_seconds >= 12U * 3600U && day_seconds < 18U * 3600U) {
        return CompactTimePhase::Afternoon;
    }
    if (day_seconds >= 18U * 3600U && day_seconds < 21U * 3600U) {
        return CompactTimePhase::Evening;
    }
    return CompactTimePhase::Night;
}

enum class CompactTreasureKind : std::uint8_t {
    Other,
    MiniGame,
    Map,
    Puzzle,
};

struct CompactTreasureCatalogEntry {
    std::int64_t id{};
    std::int32_t map_id{};
    Position position{};
    bool has_z{};
    CompactTreasureKind kind{CompactTreasureKind::Other};
};

struct CompactTreasureMarker {
    std::size_t catalog_index{};
    std::int64_t id{};
    double normalized_x{};
    double normalized_y{};
    double ranking_distance_squared{};
    bool nearest{};
    bool height_available{};
    double height_target_z{};
    double height_delta{};
    double height_angle_degrees{};
    CompactTreasureKind kind{CompactTreasureKind::Other};
};

enum class CompactRefreshStatus : std::uint8_t {
    Ok,
    NotInitialized,
    InvalidInput,
    InvalidEligibility,
    InvalidOutputCapacity,
};

struct CompactRefreshResult {
    CompactRefreshStatus status{CompactRefreshStatus::NotInitialized};
    std::size_t count{};

    [[nodiscard]] bool ok() const noexcept {
        return status == CompactRefreshStatus::Ok;
    }
};

struct CompactNearestChoice {
    std::size_t index{};
    bool retained_non_best{};
};

[[nodiscard]] inline CompactNearestChoice choose_compact_nearest(
    std::span<const CompactTreasureMarker> candidates,
    std::int64_t retained_id,
    double switch_advantage) noexcept {
    if (candidates.empty() || retained_id <= 0
        || !std::isfinite(switch_advantage) || switch_advantage < 0.0) {
        return {};
    }

    std::size_t retained_index = candidates.size();
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (candidates[index].id == retained_id) {
            retained_index = index;
            break;
        }
    }
    if (retained_index == candidates.size() || retained_index == 0) {
        return {};
    }

    const double best_squared = candidates[0].ranking_distance_squared;
    const double retained_squared =
        candidates[retained_index].ranking_distance_squared;
    if (!std::isfinite(best_squared) || best_squared < 0.0
        || !std::isfinite(retained_squared) || retained_squared < 0.0) {
        return {};
    }
    const double best_distance = std::sqrt(best_squared);
    const double retained_distance = std::sqrt(retained_squared);
    if (retained_distance <= best_distance + switch_advantage) {
        return {retained_index, true};
    }
    return {};
}

[[nodiscard]] inline bool promote_compact_nearest(
    std::span<CompactTreasureMarker> candidates,
    std::size_t selected_index) noexcept {
    if (candidates.empty() || selected_index >= candidates.size()) {
        return false;
    }
    if (selected_index == 0) {
        candidates[0].nearest = true;
        return true;
    }

    candidates[0].nearest = false;
    candidates[selected_index].nearest = true;
    std::rotate(
        candidates.begin(),
        candidates.begin() + static_cast<std::ptrdiff_t>(selected_index),
        candidates.begin() + static_cast<std::ptrdiff_t>(selected_index + 1U));
    return true;
}

class CompactRenderModel {
public:
    static constexpr std::int32_t kCompactMapId = 100;
    static constexpr std::size_t kMaximumSelectedTreasures = 80;
    static constexpr double kComparablePlayerZOffset = -150.0;
    static constexpr double kHeightDeadZone = 100.0;
    static constexpr double kMiniGameHeightDeadZone = 500.0;
    static constexpr double kHeightSensitivity = 2500.0;
    static constexpr double kMaximumHeightAngleDegrees = 85.0;

    [[nodiscard]] static double comparable_player_z(double player_z) noexcept {
        return std::isfinite(player_z)
            ? player_z + kComparablePlayerZOffset
            : std::numeric_limits<double>::quiet_NaN();
    }

    [[nodiscard]] static double height_angle_from_delta_with_dead_zone(
        double height_delta,
        double dead_zone) noexcept {
        if (!std::isfinite(height_delta) || !std::isfinite(dead_zone)
            || dead_zone < 0.0) {
            return 0.0;
        }
        const double effective_magnitude = std::max(
            std::abs(height_delta) - dead_zone, 0.0);
        if (effective_magnitude == 0.0) {
            return 0.0;
        }
        const double effective_delta = std::copysign(
            effective_magnitude, height_delta);
        return std::clamp(
            -std::atan(effective_delta / kHeightSensitivity)
                * kRadiansToDegrees,
            -kMaximumHeightAngleDegrees,
            kMaximumHeightAngleDegrees);
    }

    [[nodiscard]] static double height_angle_from_delta(
        double height_delta) noexcept {
        return height_angle_from_delta_with_dead_zone(
            height_delta, kHeightDeadZone);
    }

    [[nodiscard]] static double mini_game_height_angle_from_delta(
        double height_delta) noexcept {
        return height_angle_from_delta_with_dead_zone(
            height_delta, kMiniGameHeightDeadZone);
    }

    // Catalog initialization is the only operation in this model that may
    // allocate. A failed validation leaves the previous catalog unchanged.
    [[nodiscard]] bool initialize(
        std::span<const CompactTreasureCatalogEntry> catalog) {
        for (const CompactTreasureCatalogEntry& entry : catalog) {
            if (entry.id <= 0 || entry.map_id <= 0
                || !std::isfinite(entry.position.x)
                || !std::isfinite(entry.position.y)
                || (entry.has_z && !std::isfinite(entry.position.z))) {
                return false;
            }
        }

        std::vector<CompactTreasureCatalogEntry> replacement(
            catalog.begin(), catalog.end());
        catalog_.swap(replacement);
        initialized_ = true;
        return true;
    }

    // Refresh performs no dynamic allocation. Eligibility is a caller-owned
    // byte per catalog entry, allowing save/native state to change independently
    // of the immutable install-time catalog. Output capacity is deliberately
    // bounded so the renderer cannot grow marker work beyond the accepted cap.
    [[nodiscard]] CompactRefreshResult refresh(
        Position player,
        bool has_player_z,
        double world_radius,
        std::span<const std::uint8_t> eligibility,
        std::span<CompactTreasureMarker> output) const noexcept {
        if (!initialized_) {
            return {CompactRefreshStatus::NotInitialized, 0};
        }
        if (!std::isfinite(player.x) || !std::isfinite(player.y)
            || (has_player_z && !std::isfinite(player.z))
            || !std::isfinite(world_radius) || world_radius <= 0.0) {
            return {CompactRefreshStatus::InvalidInput, 0};
        }
        const double radius_squared = world_radius * world_radius;
        if (!std::isfinite(radius_squared)) {
            return {CompactRefreshStatus::InvalidInput, 0};
        }
        if (eligibility.size() != catalog_.size()) {
            return {CompactRefreshStatus::InvalidEligibility, 0};
        }
        if (output.empty()
            || output.size() > kMaximumSelectedTreasures) {
            return {CompactRefreshStatus::InvalidOutputCapacity, 0};
        }

        std::array<RankedCandidate, kMaximumSelectedTreasures> heap{};
        std::size_t heap_size = 0;
        const std::size_t capacity = output.size();
        const double adjusted_player_z = comparable_player_z(player.z);

        for (std::size_t index = 0; index < catalog_.size(); ++index) {
            const CompactTreasureCatalogEntry& entry = catalog_[index];
            if (eligibility[index] == 0
                || entry.map_id != kCompactMapId) {
                continue;
            }

            const double delta_x = entry.position.x - player.x;
            const double delta_y = entry.position.y - player.y;
            const double planar_distance_squared =
                delta_x * delta_x + delta_y * delta_y;
            if (!std::isfinite(planar_distance_squared)
                || planar_distance_squared > radius_squared) {
                continue;
            }

            double ranking_distance_squared = planar_distance_squared;
            if (has_player_z && entry.has_z) {
                const double delta_z =
                    entry.position.z - adjusted_player_z;
                ranking_distance_squared += delta_z * delta_z;
            }
            if (!std::isfinite(ranking_distance_squared)) {
                continue;
            }

            const RankedCandidate candidate{
                index,
                ranking_distance_squared,
            };
            if (heap_size < capacity) {
                heap[heap_size] = candidate;
                ++heap_size;
                std::push_heap(
                    heap.begin(),
                    heap.begin() + static_cast<std::ptrdiff_t>(heap_size),
                    rank_less);
            } else if (rank_less(candidate, heap[0])) {
                std::pop_heap(
                    heap.begin(),
                    heap.begin() + static_cast<std::ptrdiff_t>(heap_size),
                    rank_less);
                heap[heap_size - 1] = candidate;
                std::push_heap(
                    heap.begin(),
                    heap.begin() + static_cast<std::ptrdiff_t>(heap_size),
                    rank_less);
            }
        }

        std::sort_heap(
            heap.begin(),
            heap.begin() + static_cast<std::ptrdiff_t>(heap_size),
            rank_less);

        for (std::size_t output_index = 0;
             output_index < heap_size;
             ++output_index) {
            const RankedCandidate& candidate = heap[output_index];
            const CompactTreasureCatalogEntry& entry =
                catalog_[candidate.catalog_index];
            const double delta_x = entry.position.x - player.x;
            const double delta_y = entry.position.y - player.y;

            CompactTreasureMarker marker{};
            marker.catalog_index = candidate.catalog_index;
            marker.id = entry.id;
            marker.normalized_x = std::clamp(
                delta_x / world_radius, -1.0, 1.0);
            marker.normalized_y = std::clamp(
                delta_y / world_radius, -1.0, 1.0);
            marker.ranking_distance_squared =
                candidate.distance_squared;
            marker.nearest = output_index == 0;
            marker.kind = entry.kind;
            if (has_player_z && entry.has_z) {
                marker.height_available = true;
                marker.height_target_z = entry.position.z;
                marker.height_delta = entry.position.z
                    - adjusted_player_z;
                if (std::abs(marker.height_delta) <= kHeightDeadZone) {
                    marker.height_delta = 0.0;
                }
                marker.height_angle_degrees =
                    height_angle_from_delta(marker.height_delta);
            }
            output[output_index] = marker;
        }

        return {CompactRefreshStatus::Ok, heap_size};
    }

    [[nodiscard]] std::size_t catalog_size() const noexcept {
        return catalog_.size();
    }

    [[nodiscard]] bool initialized() const noexcept {
        return initialized_;
    }

private:
    struct RankedCandidate {
        std::size_t catalog_index{};
        double distance_squared{};
    };

    [[nodiscard]] static bool rank_less(
        const RankedCandidate& left,
        const RankedCandidate& right) noexcept {
        if (left.distance_squared != right.distance_squared) {
            return left.distance_squared < right.distance_squared;
        }
        return left.catalog_index < right.catalog_index;
    }

    static constexpr double kRadiansToDegrees =
        57.2957795130823208768;

    std::vector<CompactTreasureCatalogEntry> catalog_{};
    bool initialized_{};
};

} // namespace dswros
