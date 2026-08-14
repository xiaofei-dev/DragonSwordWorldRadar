#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
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
        double best_xy = std::numeric_limits<double>::max();
        for (const auto& point : catalog_) {
            if (point.class_name != class_name) continue;
            const double dx = actor_position.x - point.position.x;
            const double dy = actor_position.y - point.position.y;
            const double dz = std::abs(actor_position.z - point.position.z);
            const double xy = std::sqrt(dx * dx + dy * dy);
            if (xy <= kMatchRadiusXY && dz <= kMatchRadiusZ && xy < best_xy) {
                result = &point;
                best_xy = xy;
            }
        }
        return result;
    }

    [[nodiscard]] static bool finite(Position value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
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
