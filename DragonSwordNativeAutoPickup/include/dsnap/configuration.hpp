#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace dsnap {

struct Configuration {
    bool enabled_on_launch{false};
    bool passive_observation{true};
    std::string toggle_hotkey{"F9"};
    double radius_meters{4.5};
    std::size_t max_queue{128};
    std::size_t max_retries{2};
    int action_interval_ms{150};
    int retry_backoff_ms{500};
    int perf_log_interval_seconds{30};
};

struct ConfigurationResult {
    Configuration value{};
    std::vector<std::string> errors{};

    [[nodiscard]] bool valid() const noexcept { return errors.empty(); }
};

[[nodiscard]] ConfigurationResult parse_configuration_text(std::string_view text);
[[nodiscard]] ConfigurationResult load_configuration(const std::filesystem::path& path);

} // namespace dsnap
