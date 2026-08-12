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
        } else if (key == "read_only_diagnostic") {
            parsed = parse_bool(value, result.value.read_only_diagnostic);
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
        } else if (key == "radius_meters") {
            parsed = parse_number(value, result.value.radius_meters);
        } else if (key == "max_queue") {
            parsed = parse_number(value, result.value.max_queue);
        } else if (key == "perf_log_interval_seconds") {
            parsed = parse_number(value, result.value.perf_log_interval_seconds);
        } else {
            result.errors.push_back("line " + std::to_string(line_number) + ": unknown key " + key);
            continue;
        }

        if (!parsed) {
            result.errors.push_back("line " + std::to_string(line_number) + ": invalid value for " + key);
        }
    }

    if (result.value.enabled_on_launch) {
        result.errors.emplace_back("enabled_on_launch must remain false until the interaction contract is accepted");
    }
    if (!result.value.read_only_diagnostic) {
        result.errors.emplace_back("read_only_diagnostic must remain true in the bounded diagnostic build");
    }
    if (result.value.toggle_hotkey != "F9") {
        result.errors.emplace_back("toggle_hotkey must be exactly F9 in the evidence build");
    }
    if (result.value.radius_meters < 0.5 || result.value.radius_meters > 15.0) {
        result.errors.emplace_back("radius_meters must be between 0.5 and 15.0");
    }
    if (result.value.max_queue == 0 || result.value.max_queue > 1024) {
        result.errors.emplace_back("max_queue must be between 1 and 1024");
    }
    if (result.value.perf_log_interval_seconds < 5 || result.value.perf_log_interval_seconds > 600) {
        result.errors.emplace_back("perf_log_interval_seconds must be between 5 and 600");
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
