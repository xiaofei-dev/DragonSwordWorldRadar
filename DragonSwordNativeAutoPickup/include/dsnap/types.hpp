#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace dsnap {

inline constexpr const char* kAllowedClassPath = "/Script/DS.DropItemActor";
inline constexpr std::uint8_t kDropItemInteractType = 7;
// Structural object-dump evidence only. Runtime capture has not accepted this value.
inline constexpr std::uint8_t kDropItemKeyActionEnumCandidate = 13;

struct WeakObjectId {
    std::int32_t object_index{-1};
    std::int32_t serial_number{0};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return object_index >= 0 && serial_number > 0;
    }

    auto operator<=>(const WeakObjectId&) const = default;
};

struct WeakObjectIdHash {
    [[nodiscard]] std::size_t operator()(const WeakObjectId& value) const noexcept {
        const auto high = static_cast<std::uint64_t>(static_cast<std::uint32_t>(value.object_index)) << 32U;
        const auto low = static_cast<std::uint32_t>(value.serial_number);
        return static_cast<std::size_t>(high | low);
    }
};

struct CandidateValidation {
    bool object_valid{};
    bool exact_allowed_class{};
    bool current_world{};
    bool pawn_valid{};
    bool controller_valid{};
    bool component_valid{};
    bool interactable_state_proven{};
    std::uint8_t interact_type{};
    double distance_meters{};

    [[nodiscard]] bool eligible(double radius_meters) const noexcept {
        return object_valid && exact_allowed_class && current_world && pawn_valid && controller_valid &&
               component_valid && interactable_state_proven && interact_type == kDropItemInteractType &&
               distance_meters >= 0.0 && distance_meters <= radius_meters;
    }
};

enum class ActionResult {
    Succeeded,
    Rejected,
    Invalidated,
    TransientFailure,
};

using MonotonicTime = std::chrono::steady_clock::time_point;

} // namespace dsnap
