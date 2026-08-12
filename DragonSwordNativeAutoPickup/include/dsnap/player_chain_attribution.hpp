#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace dsnap {

enum class PlayerChainReason : std::uint8_t {
    EngineOrOutputInvalid,
    GameViewportPropertyMissing,
    GameViewportValueNull,
    GameInstancePropertyMissing,
    GameInstanceValueNull,
    LocalPlayersPropertyMissing,
    LocalPlayersInvalidIndex,
    LocalPlayersNullData,
    LocalPlayerEntryNull,
    LocalPlayerControllerPropertyMissing,
    LocalPlayerControllerValueNull,
    ControllerClassMismatch,
    ControllerPlayerPropertyMissing,
    ControllerPlayerIdentityMismatch,
    PawnPropertyMissing,
    PawnValueNull,
    PlayerControllerPropertyMissing,
    PlayerControllerIdentityMismatch,
    LocationUnavailable,
    Success,
    GuardedException,
    Count,
};

enum class PlayerMode : std::uint8_t { ExpectedCharacter, ControllerBoundAlternatePawn };

[[nodiscard]] constexpr std::string_view player_mode_name(PlayerMode mode) noexcept {
    return mode == PlayerMode::ExpectedCharacter ? std::string_view{"expected_character"}
                                                 : std::string_view{"controller_bound_alternate_pawn"};
}

[[nodiscard]] constexpr std::optional<PlayerMode> classify_controlled_pawn(
    bool expected_character,
    bool controller_identity_proven,
    bool fresh_location) noexcept {
    if (!controller_identity_proven || !fresh_location) return std::nullopt;
    return expected_character ? PlayerMode::ExpectedCharacter : PlayerMode::ControllerBoundAlternatePawn;
}

inline constexpr std::size_t kPlayerChainReasonCount = static_cast<std::size_t>(PlayerChainReason::Count);
inline constexpr std::size_t kMaxPlayerChainDiagnosticLogs = 16;

[[nodiscard]] constexpr std::size_t player_chain_reason_index(PlayerChainReason reason) noexcept {
    return static_cast<std::size_t>(reason);
}

[[nodiscard]] constexpr std::string_view player_chain_reason_name(PlayerChainReason reason) noexcept {
    constexpr std::array names{
        std::string_view{"engine_or_output_invalid"},
        std::string_view{"game_viewport_property_missing"},
        std::string_view{"game_viewport_value_null"},
        std::string_view{"game_instance_property_missing"},
        std::string_view{"game_instance_value_null"},
        std::string_view{"local_players_property_missing"},
        std::string_view{"local_players_invalid_index"},
        std::string_view{"local_players_null_data"},
        std::string_view{"local_player_entry_null"},
        std::string_view{"local_player_controller_property_missing"},
        std::string_view{"local_player_controller_value_null"},
        std::string_view{"controller_class_mismatch"},
        std::string_view{"controller_player_property_missing"},
        std::string_view{"controller_player_identity_mismatch"},
        std::string_view{"pawn_property_missing"},
        std::string_view{"pawn_value_null"},
        std::string_view{"player_controller_property_missing"},
        std::string_view{"player_controller_identity_mismatch"},
        std::string_view{"location_unavailable"},
        std::string_view{"success"},
        std::string_view{"guarded_exception"},
    };
    const auto index = player_chain_reason_index(reason);
    return index < names.size() ? names[index] : std::string_view{"invalid_reason"};
}

class BoundedPlayerChainDiagnosticState {
public:
    [[nodiscard]] bool should_log(PlayerChainReason reason) noexcept {
        const bool changed = !has_reason_ || reason != last_reason_;
        last_reason_ = reason;
        has_reason_ = true;
        if (!changed || emitted_ >= kMaxPlayerChainDiagnosticLogs) return false;
        ++emitted_;
        return true;
    }

    [[nodiscard]] std::size_t emitted() const noexcept { return emitted_; }

private:
    PlayerChainReason last_reason_{PlayerChainReason::EngineOrOutputInvalid};
    std::size_t emitted_{};
    bool has_reason_{};
};

class MonotonicDebounce {
public:
    explicit MonotonicDebounce(std::chrono::milliseconds minimum_interval) noexcept
        : minimum_interval_ns_(std::chrono::duration_cast<std::chrono::nanoseconds>(minimum_interval).count()) {}

    [[nodiscard]] bool accept(std::chrono::steady_clock::time_point now) noexcept {
        const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        auto previous = last_accepted_ns_.load(std::memory_order_acquire);
        for (;;) {
            if (previous >= 0 && now_ns - previous < minimum_interval_ns_) return false;
            if (last_accepted_ns_.compare_exchange_weak(
                    previous, now_ns, std::memory_order_acq_rel, std::memory_order_acquire)) return true;
        }
    }

private:
    std::int64_t minimum_interval_ns_{};
    std::atomic<std::int64_t> last_accepted_ns_{-1};
};

} // namespace dsnap
