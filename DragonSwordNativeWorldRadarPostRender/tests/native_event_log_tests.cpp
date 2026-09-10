#include "native_event_log.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

template <typename Append>
void require_event_enqueued(Append&& append, const char* message) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (append()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    require(false, message);
}

std::string read_all(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    require(input.good(), "native event log was not created");
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

} // namespace

int main() {
    try {
        const auto unique = std::chrono::steady_clock::now()
            .time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path()
            / ("dsnwr-native-event-log-test-" + std::to_string(unique));
        const auto log = root / "runtime" / "logs"
            / "DragonSwordNativeWorldRadarPostRender.Native.log";

        dsnwr::begin_native_event_log_session(root, true);
        require(dsnwr::native_event_log_enabled(),
                "enabled writer did not become ready");

        for (int index = 0; index < 8; ++index) {
            dsnwr::append_native_event_log(
                "TEST_EVENT", "value=7 capture=producer");
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }

        dsnwr::NativeEngineTickProfileSample slow{};
        slow.activation = 3;
        slow.epoch = 4;
        slow.elapsed_us = {2500, 1, 2, 3, 4, 5, 6, 7, 8, 90};
        slow.area_quest_scan_active = true;
        slow.area_quest_scan_index = 12;
        slow.area_quest_catalog_size = 147;
        slow.compact_update_called = true;
        slow.discovery_called = true;
        slow.bird_egg_active_count = 2;
        require_event_enqueued(
            [&slow] {
                return dsnwr::append_native_engine_tick_slow(
                    slow, 2000, 1);
            },
            "structured slow record could not be enqueued");

        dsnwr::NativeEngineTickProfileReport profile{};
        profile.activation = 3;
        profile.epoch = 4;
        profile.interval_ms = 10000;
        for (std::size_t index = 0; index < profile.metrics.size(); ++index) {
            profile.metrics[index] = {
                10U + index, 20U + index, 30U + index};
        }
        profile.slow_ticks = 1;
        profile.slow_threshold_us = 2000;
        require_event_enqueued(
            [&profile] {
                return dsnwr::append_native_engine_tick_profile(profile);
            },
            "structured profile record could not be enqueued");
        dsnwr::flush_native_event_log();
        require(!dsnwr::native_event_log_enabled(),
                "shutdown did not disable producers");

        const auto document = read_all(log);
        require(document.find("SESSION_BEGIN seq=1") != std::string::npos,
                "session header or first sequence is missing");
        require(document.find("writer=fixed_queue_async") != std::string::npos,
                "asynchronous writer metadata is missing");
        require(document.find("TEST_EVENT") != std::string::npos,
                "ordinary queued event was not drained");
        require(document.find("ENGINE_TICK_SLOW") != std::string::npos,
                "structured slow record was not serialized");
        require(document.find("scene_umg_us=90") != std::string::npos,
                "scene timing must remain independently attributable");
        require(document.find("total_us=2500") != std::string::npos,
                "structured slow timing was changed");
        require(document.find("ENGINE_TICK_PROFILE") != std::string::npos,
                "structured profile record was not serialized");
        require(document.find("metric_format=calls_total_us_max_us")
                    != std::string::npos,
                "profile metric schema is missing");
        require(document.find("logger_dropped=") != std::string::npos,
                "logger drop health is missing");
        require(document.find("logger_truncated=") != std::string::npos,
                "logger truncation health is missing");

        std::error_code cleanup_error;
        std::filesystem::remove_all(root, cleanup_error);
        require(!cleanup_error, "temporary log tree cleanup failed");
        std::cout << "Native event log queue tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        dsnwr::flush_native_event_log();
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
