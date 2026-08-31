#include <dsnap/configuration.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>

namespace dsnap {
namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool parse_bool(const std::string& value, bool& output) {
    if (value == "true") {
        output = true;
        return true;
    }
    if (value == "false") {
        output = false;
        return true;
    }
    return false;
}

template <typename T>
bool parse_number(const std::string& value, T& output) {
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto result = std::from_chars(begin, end, output);
    return result.ec == std::errc{} && result.ptr == end;
}

} // namespace

std::optional<std::uint8_t> parse_toggle_hotkey(std::string_view value) noexcept {
    if (value.size() == 1) {
        const auto key = static_cast<unsigned char>(value.front());
        if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
            return static_cast<std::uint8_t>(key);
        }
    }

    if (value.size() >= 2 && value.front() == 'F') {
        unsigned int number{};
        const auto* begin = value.data() + 1;
        const auto* end = value.data() + value.size();
        const auto parsed = std::from_chars(begin, end, number);
        if (parsed.ec == std::errc{} && parsed.ptr == end && number >= 1 && number <= 24) {
            return static_cast<std::uint8_t>(0x70U + number - 1U);
        }
    }

    if (value.size() == 4 && value.starts_with("NUM") && value.back() >= '0' && value.back() <= '9') {
        return static_cast<std::uint8_t>(0x60U + static_cast<unsigned int>(value.back() - '0'));
    }

    struct NamedKey { std::string_view name; std::uint8_t code; };
    constexpr NamedKey named_keys[] = {
        {"HOME", 0x24}, {"END", 0x23}, {"PAGEUP", 0x21}, {"PAGEDOWN", 0x22},
        {"INSERT", 0x2D}, {"DELETE", 0x2E}, {"SPACE", 0x20},
    };
    for (const auto& key : named_keys) {
        if (value == key.name) return key.code;
    }
    return std::nullopt;
}

std::optional<int> parse_named_integer(std::string_view line, std::string_view name) {
    const auto marker = std::string{name} + "=";
    const auto start = line.find(marker);
    if (start == std::string_view::npos) return std::nullopt;

    const auto value_start = start + marker.size();
    const auto value_end = line.find_first_of(",)", value_start);
    const auto token = line.substr(value_start,
                                   value_end == std::string_view::npos
                                       ? std::string_view::npos
                                       : value_end - value_start);
    int value{};
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) return std::nullopt;
    return value;
}

struct SavedBindingEntry {
    int primary{};
    int primary_chord{};
    int secondary{};
    int secondary_chord{};
};

std::optional<SavedBindingEntry> parse_saved_binding_entry(std::string_view line) {
    const auto action = parse_named_integer(line, "ActionInputType");
    if (!action || *action != 91) return std::nullopt;

    const auto primary = parse_named_integer(line, "Bind1_Key1");
    const auto primary_chord = parse_named_integer(line, "Bind1_Key2");
    const auto secondary = parse_named_integer(line, "Bind2_Key1");
    const auto secondary_chord = parse_named_integer(line, "Bind2_Key2");
    if (!primary || !primary_chord || !secondary || !secondary_chord) return SavedBindingEntry{};
    return SavedBindingEntry{*primary, *primary_chord, *secondary, *secondary_chord};
}

std::optional<int> choose_unchorded_binding(const SavedBindingEntry& entry) {
    if (entry.primary != 0 && entry.primary_chord == 0) return entry.primary;
    if (entry.secondary != 0 && entry.secondary_chord == 0) return entry.secondary;
    return std::nullopt;
}

bool is_valid_interaction_key_name(std::string_view value) noexcept {
    if (value == "AUTO") return true;
    if (value.empty() || value.size() > 64) return false;

    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '_';
    });
}

bool is_valid_interaction_key_fallback_name(std::string_view value) noexcept {
    if (value.size() == 4 &&
        std::toupper(static_cast<unsigned char>(value[0])) == 'A' &&
        std::toupper(static_cast<unsigned char>(value[1])) == 'U' &&
        std::toupper(static_cast<unsigned char>(value[2])) == 'T' &&
        std::toupper(static_cast<unsigned char>(value[3])) == 'O') {
        return false;
    }
    return is_valid_interaction_key_name(value);
}

std::optional<std::string> unreal_key_name_from_button_key_code(int value) {
    if (value >= 1 && value <= 26) {
        return std::string(1, static_cast<char>('A' + value - 1));
    }
    if (value >= 31 && value <= 39) {
        return std::string(1, static_cast<char>('1' + value - 31));
    }
    if (value == 40) return std::string{"0"};
    if (value >= 51 && value <= 62) return "F" + std::to_string(value - 50);

    struct KeyName { int code; std::string_view name; };
    constexpr KeyName names[] = {
        {71, "Up"}, {72, "Down"}, {73, "Left"}, {74, "Right"},
        {81, "SpaceBar"}, {82, "LeftShift"}, {83, "Escape"}, {84, "Delete"},
        {85, "BackSpace"}, {86, "Enter"}, {87, "LeftControl"}, {88, "LeftAlt"},
        {89, "Tab"}, {90, "Slash"}, {91, "Apostrophe"}, {92, "Hyphen"},
        {131, "LeftMouseButton"}, {132, "RightMouseButton"},
        {133, "ThumbMouseButton"}, {134, "ThumbMouseButton2"},
        {141, "Gamepad_DPad_Up"}, {142, "Gamepad_DPad_Down"},
        {143, "Gamepad_DPad_Right"}, {144, "Gamepad_DPad_Left"},
        {151, "Gamepad_FaceButton_Top"}, {152, "Gamepad_FaceButton_Bottom"},
        {153, "Gamepad_FaceButton_Left"}, {154, "Gamepad_FaceButton_Right"},
        {161, "Gamepad_LeftShoulder"}, {162, "Gamepad_LeftTrigger"},
        {163, "Gamepad_RightShoulder"}, {164, "Gamepad_RightTrigger"},
        {171, "Gamepad_Special_Left"}, {172, "Gamepad_Special_Right"},
        {181, "Gamepad_LeftThumbstick"}, {191, "Gamepad_RightThumbstick"},
    };
    for (const auto& entry : names) {
        if (entry.code == value) return std::string{entry.name};
    }
    return std::nullopt;
}

GameInteractionBindingResult parse_game_interaction_binding(std::string_view text) {
    GameInteractionBindingResult result{};
    std::istringstream input{std::string{text}};
    std::string line;
    std::size_t keyboard_matches{};
    std::size_t gamepad_matches{};

    while (std::getline(input, line)) {
        const bool keyboard = line.starts_with("KeyboardConfigs=");
        const bool gamepad = line.starts_with("GamepadConfigs=");
        if (!keyboard && !gamepad) continue;
        if (line.find("ActionInputType=91") == std::string::npos) continue;

        const auto entry = parse_saved_binding_entry(line);
        if (!entry) continue;
        auto& matches = keyboard ? keyboard_matches : gamepad_matches;
        ++matches;
        if (matches > 1) continue;

        const auto selected = choose_unchorded_binding(*entry);
        if (!selected) {
            result.errors.emplace_back(keyboard
                ? "INTERACT keyboard binding is missing or uses an unsupported chord"
                : "INTERACT gamepad binding is missing or uses an unsupported chord");
            continue;
        }
        const auto key_name = unreal_key_name_from_button_key_code(*selected);
        if (!key_name) {
            result.errors.emplace_back(std::string{"INTERACT "} +
                (keyboard ? "keyboard" : "gamepad") +
                " binding code is unsupported: " + std::to_string(*selected));
            continue;
        }
        if (keyboard) {
            result.value.keyboard_key_code = *selected;
            result.value.keyboard_key = *key_name;
        } else {
            result.value.gamepad_key_code = *selected;
            result.value.gamepad_key = *key_name;
        }
    }

    if (keyboard_matches == 0) result.errors.emplace_back("INTERACT keyboard binding was not found");
    if (keyboard_matches > 1) result.errors.emplace_back("multiple INTERACT keyboard bindings were found");
    if (gamepad_matches > 1) result.errors.emplace_back("multiple INTERACT gamepad bindings were found");
    return result;
}

GameInteractionBindingResult load_game_interaction_binding(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        GameInteractionBindingResult result{};
        result.errors.emplace_back("game input settings file could not be opened");
        return result;
    }
    return parse_game_interaction_binding(std::string{std::istreambuf_iterator<char>{input}, {}});
}

ConfigurationResult parse_configuration_text(std::string_view text) {
    ConfigurationResult result{};
    std::istringstream input{std::string{text}};
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        line = trim(std::move(line));
        if (line.empty() || line.starts_with('#') || line.starts_with(';') ||
            (line.starts_with('[') && line.ends_with(']'))) {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            result.errors.push_back("line " + std::to_string(line_number) + ": expected key=value");
            continue;
        }

        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));
        bool parsed = true;

        if (key == "enabled_on_launch") {
            parsed = parse_bool(value, result.value.enabled_on_launch);
        } else if (key == "automatic_pickup") {
            parsed = parse_bool(value, result.value.automatic_pickup);
        } else if (key == "read_only_diagnostic") {
            // Accept only the old safe value so an installed 0.6 config can be
            // upgraded deliberately. It does not disable the active contract.
            bool legacy_value{};
            parsed = parse_bool(value, legacy_value) && legacy_value;
        } else if (key == "single_target_canary") {
            // Safe migration path for an installed pre-0.6 config. Only true
            // is accepted; false cannot enable a different runtime mode.
            bool legacy_value{};
            parsed = parse_bool(value, legacy_value) && legacy_value;
        } else if (key == "passive_observation") {
            bool legacy_value{};
            parsed = parse_bool(value, legacy_value) && legacy_value;
        } else if (key == "toggle_hotkey") {
            result.value.toggle_hotkey = value;
        } else if (key == "interaction_key") {
            result.value.interaction_key =
                (value == "AUTO" || value == "Auto" || value == "auto") ? "AUTO" : value;
        } else if (key == "interaction_key_fallback") {
            result.value.interaction_key_fallback = value;
        } else if (key == "radius_meters") {
            parsed = parse_number(value, result.value.radius_meters);
        } else if (key == "max_queue") {
            parsed = parse_number(value, result.value.max_queue);
        } else if (key == "debug_logging") {
            parsed = parse_bool(value, result.value.debug_logging);
        } else if (key == "perf_log_interval_seconds") {
            parsed = parse_number(value, result.value.perf_log_interval_seconds);
        } else if (key == "slow_scan_threshold_us") {
            parsed = parse_number(value, result.value.slow_scan_threshold_us);
        } else {
            result.errors.push_back("line " + std::to_string(line_number) + ": unknown key " + key);
            continue;
        }

        if (!parsed) {
            result.errors.push_back("line " + std::to_string(line_number) + ": invalid value for " + key);
        }
    }

    if (result.value.enabled_on_launch) {
        result.errors.emplace_back("enabled_on_launch must remain false; the configured toggle hotkey is the enable source");
    }
    if (!result.value.automatic_pickup) {
        result.errors.emplace_back("automatic_pickup must remain true in the active pickup build");
    }
    if (!parse_toggle_hotkey(result.value.toggle_hotkey)) {
        result.errors.emplace_back("toggle_hotkey is not a supported key name");
    }
    if (!is_valid_interaction_key_name(result.value.interaction_key)) {
        result.errors.emplace_back(
            "interaction_key must be AUTO or an Unreal FKey name containing only letters, digits, and underscores");
    }
    if (!is_valid_interaction_key_fallback_name(result.value.interaction_key_fallback)) {
        result.errors.emplace_back(
            "interaction_key_fallback must be a concrete Unreal FKey name containing only letters, digits, and underscores");
    }
    if (result.value.radius_meters < 0.5 || result.value.radius_meters > 15.0) {
        result.errors.emplace_back("radius_meters must be between 0.5 and 15.0");
    }
    if (result.value.max_queue == 0 || result.value.max_queue > 128) {
        result.errors.emplace_back("max_queue must be between 1 and 128 for complete same-pulse selection");
    }
    if (result.value.perf_log_interval_seconds < 5 || result.value.perf_log_interval_seconds > 600) {
        result.errors.emplace_back("perf_log_interval_seconds must be between 5 and 600");
    }
    if (result.value.slow_scan_threshold_us < 100 || result.value.slow_scan_threshold_us > 100000) {
        result.errors.emplace_back("slow_scan_threshold_us must be between 100 and 100000");
    }

    return result;
}

ConfigurationResult load_configuration(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        ConfigurationResult result{};
        result.errors.emplace_back("configuration file could not be opened");
        return result;
    }
    return parse_configuration_text(std::string{std::istreambuf_iterator<char>{input}, {}});
}

} // namespace dsnap
