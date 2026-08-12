#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace dsnap {

enum class GateReason : std::uint8_t {
    OwnerInvalid,
    ClassInvalid,
    InteractComponentMissing,
    ComponentOwnershipMismatch,
    StateFieldMissing,
    StateValueMismatch,
    OwnerLocationUnavailable,
    NonFiniteDistance,
    OutsideRadius,
    Eligible,
    GuardedEvaluationException,
    Count,
};

inline constexpr std::size_t kGateReasonCount = static_cast<std::size_t>(GateReason::Count);
inline constexpr std::size_t kMaxGateDiagnosticLogsPerCandidate = 8;

[[nodiscard]] constexpr std::size_t gate_reason_index(GateReason reason) noexcept {
    return static_cast<std::size_t>(reason);
}

[[nodiscard]] constexpr std::string_view gate_reason_name(GateReason reason) noexcept {
    constexpr std::array names{
        std::string_view{"owner_invalid"},
        std::string_view{"class_invalid"},
        std::string_view{"interact_component_missing"},
        std::string_view{"component_ownership_mismatch"},
        std::string_view{"state_field_missing"},
        std::string_view{"state_value_mismatch"},
        std::string_view{"owner_location_unavailable"},
        std::string_view{"non_finite_distance"},
        std::string_view{"outside_radius"},
        std::string_view{"eligible"},
        std::string_view{"guarded_evaluation_exception"},
    };
    const auto index = gate_reason_index(reason);
    return index < names.size() ? names[index] : std::string_view{"invalid_reason"};
}

struct GateObservation {
    GateReason reason{GateReason::OwnerInvalid};
    bool has_state_values{};
    std::uint8_t interactable_value{};
    std::uint8_t interact_type_value{};
    bool has_distance{};
    double distance_meters{};

    auto operator<=>(const GateObservation&) const = default;
};

class BoundedGateDiagnosticState {
public:
    [[nodiscard]] bool should_log(const GateObservation& observation) noexcept {
        const bool changed = !has_observation_ || observation != last_observation_;
        last_observation_ = observation;
        has_observation_ = true;
        if (!changed || emitted_ >= kMaxGateDiagnosticLogsPerCandidate) return false;
        ++emitted_;
        return true;
    }

    [[nodiscard]] std::size_t emitted() const noexcept { return emitted_; }

private:
    GateObservation last_observation_{};
    std::size_t emitted_{};
    bool has_observation_{};
};

} // namespace dsnap
