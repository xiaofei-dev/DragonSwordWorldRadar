#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dsnap {

struct Configuration {
    bool enabled_on_launch{false};
    bool automatic_pickup{true};
    std::string toggle_hotkey{"F9"};
    // AUTO reads the game's saved semantic INTERACT binding once whenever the
    // Mod is enabled. A concrete Unreal FKey name is a fail-closed manual
    // override for troubleshooting or future game builds.
    std::string interaction_key{"AUTO"};
    // Used only when AUTO cannot read a unique saved semantic INTERACT
    // keyboard binding. This remains an exact Unreal FKey name so live action
    // resolution still fails closed when the configured mapping is absent.
    std::string interaction_key_fallback{"F"};
    double radius_meters{4.5};
    std::size_t max_queue{128};
    // Retained only for the isolated legacy core model and its regression
    // tests. The native adapter does not read retry settings.
    std::size_t max_retries{2};
    int action_interval_ms{150};
    int retry_backoff_ms{500};
    // Public release builds stay quiet unless the installed config explicitly
    // opts into diagnostic logging.
    bool debug_logging{false};
    int perf_log_interval_seconds{10};
    int slow_scan_threshold_us{1000};
};

struct ConfigurationResult {
    Configuration value{};
    std::vector<std::string> errors{};

    [[nodiscard]] bool valid() const noexcept { return errors.empty(); }
};

struct GameInteractionBinding {
    std::optional<std::string> keyboard_key{};
    std::optional<std::string> gamepad_key{};
    int keyboard_key_code{};
    int gamepad_key_code{};
};

struct GameInteractionBindingResult {
    GameInteractionBinding value{};
    std::vector<std::string> errors{};

    [[nodiscard]] bool valid() const noexcept {
        return errors.empty() && (value.keyboard_key.has_value() || value.gamepad_key.has_value());
    }
};

[[nodiscard]] ConfigurationResult parse_configuration_text(std::string_view text);
[[nodiscard]] ConfigurationResult load_configuration(const std::filesystem::path& path);
[[nodiscard]] std::optional<std::uint8_t> parse_toggle_hotkey(std::string_view value) noexcept;
[[nodiscard]] bool is_valid_interaction_key_name(std::string_view value) noexcept;
[[nodiscard]] bool is_valid_interaction_key_fallback_name(std::string_view value) noexcept;
[[nodiscard]] std::optional<std::string> unreal_key_name_from_button_key_code(int value);
[[nodiscard]] GameInteractionBindingResult parse_game_interaction_binding(std::string_view text);
[[nodiscard]] GameInteractionBindingResult load_game_interaction_binding(
    const std::filesystem::path& path);

} // namespace dsnap
