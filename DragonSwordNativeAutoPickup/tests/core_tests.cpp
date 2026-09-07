#include <dsnap/action_evidence.hpp>
#include <dsnap/async_logger.hpp>
#include <dsnap/callback_generation.hpp>
#include <dsnap/candidate_queue.hpp>
#include <dsnap/configuration.hpp>
#include <dsnap/discovery_state.hpp>
#include <dsnap/gate_attribution.hpp>
#include <dsnap/pickup_controller.hpp>
#include <dsnap/player_chain_attribution.hpp>
#include <dsnap/runtime_contract.hpp>
#include <dsnap/selector_resolver.hpp>
#include <dsnap/session_calibration.hpp>
#include <dsnap/single_target_latch.hpp>
#include <dsnap/status_toast.hpp>
#include <dsnap/types.hpp>
#include <dsnap/windows_fingerprint.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace dsnap::tests {
[[nodiscard]] bool run_pe_runtime_tests();
}

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

dsnap::CandidateValidation valid_candidate(double distance = 2.0) {
    return dsnap::CandidateValidation{
        .object_valid = true,
        .exact_allowed_class = true,
        .current_world = true,
        .pawn_valid = true,
        .controller_valid = true,
        .component_valid = true,
        .interactable_state_proven = true,
        .interact_type = dsnap::kDropItemInteractType,
        .distance_meters = distance,
    };
}

void test_configuration() {
    const dsnap::Configuration defaults{};
    require(!defaults.enabled_on_launch, "public configuration default must keep pickup off until the hotkey");
    require(!defaults.debug_logging, "public configuration default must keep diagnostics disabled");
    require(defaults.interaction_key == "AUTO", "automatic interaction binding should be the default");
    require(defaults.interaction_key_fallback == "F", "F should be the default AUTO fallback key");

    const auto valid = dsnap::parse_configuration_text(R"(
[auto_pickup]
enabled_on_launch=false
automatic_pickup=true
toggle_hotkey=F9
interaction_key=AUTO
interaction_key_fallback=K
radius_meters=5.0
max_queue=64
perf_log_interval_seconds=30
debug_logging=true
slow_scan_threshold_us=1000
)");
    require(valid.valid(), "valid configuration should parse");
    require(!valid.value.enabled_on_launch, "launch-disabled default should parse");
    require(valid.value.radius_meters == 5.0, "radius should parse");
    require(valid.value.debug_logging, "debug timing mode should parse");
    require(valid.value.interaction_key == "AUTO", "AUTO interaction binding should parse");
    require(valid.value.interaction_key_fallback == "K", "explicit AUTO fallback key should parse");
    require(valid.value.slow_scan_threshold_us == 1000, "slow scan threshold should parse");

    const auto launch_enabled = dsnap::parse_configuration_text(
        "enabled_on_launch=true\nautomatic_pickup=true\ntoggle_hotkey=F9\n");
    require(!launch_enabled.valid(), "launch enable must remain fail-closed");
    const auto custom_hotkey = dsnap::parse_configuration_text(
        "enabled_on_launch=false\nautomatic_pickup=true\ntoggle_hotkey=F10\n");
    require(custom_hotkey.valid(), "a supported custom toggle hotkey should parse");
    require(dsnap::parse_toggle_hotkey("F9") == 0x78, "F9 should map to its UE4SS key code");
    require(dsnap::parse_toggle_hotkey("F24") == 0x87, "F24 should map to its UE4SS key code");
    require(dsnap::parse_toggle_hotkey("K") == 0x4B, "letter hotkeys should map directly");
    require(dsnap::parse_toggle_hotkey("NUM3") == 0x63, "numpad hotkeys should map directly");
    require(!dsnap::parse_toggle_hotkey("F25"), "unsupported function keys must fail closed");
    require(dsnap::is_valid_interaction_key_name("AUTO"), "AUTO interaction binding should be valid");
    require(dsnap::is_valid_interaction_key_name("E"), "keyboard interaction binding should be valid");
    require(dsnap::is_valid_interaction_key_name("Gamepad_FaceButton_Left"),
            "gamepad interaction binding should be valid");
    require(dsnap::is_valid_interaction_key_fallback_name("F"),
            "a concrete keyboard fallback should be valid");
    require(dsnap::is_valid_interaction_key_fallback_name("Gamepad_FaceButton_Left"),
            "a concrete gamepad fallback should be valid");
    require(!dsnap::is_valid_interaction_key_fallback_name("AUTO"),
            "AUTO cannot recursively serve as its own fallback");
    require(!dsnap::is_valid_interaction_key_fallback_name("auto"),
            "AUTO fallback rejection should be case-insensitive");
    const auto mixed_case_auto = dsnap::parse_configuration_text(
        "enabled_on_launch=false\nautomatic_pickup=true\ninteraction_key=Auto\n");
    require(mixed_case_auto.valid() && mixed_case_auto.value.interaction_key == "AUTO",
            "common AUTO spellings should normalize to the automatic mode");
    require(!dsnap::is_valid_interaction_key_name("CTRL+E"),
            "interaction binding chords should fail closed");
    const auto invalid_fallback = dsnap::parse_configuration_text(
        "enabled_on_launch=false\nautomatic_pickup=true\ninteraction_key=AUTO\ninteraction_key_fallback=AUTO\n");
    require(!invalid_fallback.valid(), "AUTO fallback must require a concrete Unreal FKey name");
    const auto unsupported_key = dsnap::parse_configuration_text("enabled_on_launch=false\nautomatic_pickup=true\ntoggle_hotkey=CTRL+F9\n");
    require(!unsupported_key.valid(), "unsupported hotkey syntax must fail closed");
    const auto legacy = dsnap::parse_configuration_text("enabled_on_launch=false\npassive_observation=true\ntoggle_hotkey=F9\n");
    require(legacy.valid(), "a prior passive=true config must remain a safe backward-compatible alias");
    const auto canary_legacy = dsnap::parse_configuration_text("enabled_on_launch=false\nsingle_target_canary=true\ntoggle_hotkey=F9\n");
    require(canary_legacy.valid(), "a prior canary=true config must remain safely diagnostic-only");
    const auto active_off = dsnap::parse_configuration_text("enabled_on_launch=false\nautomatic_pickup=false\ntoggle_hotkey=F9\n");
    require(!active_off.valid(), "active pickup cannot be silently disabled into an undefined mode");
    const auto old_config = dsnap::parse_configuration_text("enabled_on_launch=false\nread_only_diagnostic=true\ntoggle_hotkey=F9\n");
    require(old_config.valid(), "the previous read-only config must remain a safe migration input");
    const auto oversized_registry = dsnap::parse_configuration_text(
        "enabled_on_launch=false\nautomatic_pickup=true\ntoggle_hotkey=F9\nmax_queue=129\n");
    require(!oversized_registry.valid(), "complete same-pulse selection must reject a registry above 128 entries");

    const auto removed_range_key = dsnap::parse_configuration_text(
        "enabled_on_launch=false\nautomatic_pickup=true\ntoggle_hotkey=F9\nrange_multiplier=2.0\n");
    require(!removed_range_key.valid(),
            "the failed player-receiver range key must not survive as ignored configuration");
}

struct SelectorResolverFixture {
    static constexpr std::uintptr_t kDefaultBase = 0x140000000ULL;
    static constexpr std::size_t kExecOffset = 0x100;
    static constexpr std::size_t kServerThunkOffset = 0x200;
    static constexpr std::size_t kServerOffset = 0x300;
    static constexpr std::size_t kSelectorOffset = 0x700;
    static constexpr std::size_t kUiOffset = 0xC00;
    static constexpr std::size_t kUiCallOffset = 0x60;
    static constexpr std::size_t kUiScanLimit = 0x400;
    static constexpr std::size_t kAlternateTargetOffset = 0x1100;
    static constexpr std::int32_t kObjectOffset = 0x228;
    static constexpr std::int32_t kComponentOffset = 0x230;

    explicit SelectorResolverFixture(std::uintptr_t image_base = kDefaultBase)
        : base(image_base), text(0x1800, 0x90) {
        put(kExecOffset, {0x48, 0x8B, 0x42, 0x20, 0x45, 0x33, 0xC0, 0x48,
                          0x85, 0xC0, 0x41, 0x0F, 0x95, 0xC0, 0x4C, 0x03,
                          0xC0, 0x4C, 0x89, 0x42, 0x20, 0x48, 0x8B, 0x01,
                          0x48, 0xFF, 0xA0, 0xC0, 0x04, 0x00, 0x00, 0xCC});
        put(kServerThunkOffset, {0xE9, 0, 0, 0, 0});
        write_rel32(kServerThunkOffset + 1, address(kServerThunkOffset + 5), address(kServerOffset));
        exec_dispatch_offsets.push_back(21);

        put(kServerOffset, {0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83,
                            0xEC, 0x30, 0x48, 0x8B, 0xF9});
        add_selector_call(0x50, kSelectorOffset);
        put(kSelectorOffset, {0x48, 0x8B, 0xC4, 0x4C, 0x89, 0x40, 0x18, 0x48,
                              0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08});
        put(kSelectorOffset + 0x70,
            {0x33, 0xC0, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08});
        put(kUiOffset, {0x48, 0x89, 0x5C, 0x24, 0x10,
                        0x48, 0x89, 0x74, 0x24, 0x18,
                        0x48, 0x89, 0x7C, 0x24, 0x20, 0x55});
        add_ui_selector_call(kUiCallOffset, kSelectorOffset);
    }

    [[nodiscard]] std::uintptr_t address(std::size_t offset) const noexcept {
        return base + offset;
    }

    [[nodiscard]] dsnap::ExecutableTextView view() const noexcept {
        return {base, text};
    }

    void put(std::size_t offset, std::initializer_list<std::uint8_t> bytes) {
        require(offset + bytes.size() <= text.size(), "selector fixture write must fit");
        std::copy(bytes.begin(), bytes.end(), text.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    void write_i32(std::size_t offset, std::int32_t value) {
        require(offset + sizeof(value) <= text.size(), "selector fixture int32 write must fit");
        std::memcpy(text.data() + offset, &value, sizeof(value));
    }

    void write_rel32(std::size_t displacement_offset,
                     std::uintptr_t instruction_end,
                     std::uintptr_t target) {
        const auto displacement = static_cast<std::int64_t>(target) -
            static_cast<std::int64_t>(instruction_end);
        require(displacement >= std::numeric_limits<std::int32_t>::min() &&
                    displacement <= std::numeric_limits<std::int32_t>::max(),
                "selector fixture rel32 must fit");
        write_i32(displacement_offset, static_cast<std::int32_t>(displacement));
    }

    void add_selector_call(std::size_t local_offset, std::size_t selector_offset) {
        selector_call_offsets.push_back(local_offset);
        const auto offset = kServerOffset + local_offset;
        put(offset, {0x45, 0x33, 0xC0,
                     0x48, 0x8D, 0x54, 0x24, 0x20,
                     0x48, 0x8B, 0xCF,
                     0xE8, 0, 0, 0, 0,
                     0x48, 0x8B, 0x87, 0x28, 0x02, 0x00, 0x00,
                     0x48, 0x8B, 0x4C, 0x24, 0x20,
                     0x48, 0x3B, 0xC8, 0x74, 0x0A,
                     0x48, 0x89, 0x8F, 0x28, 0x02, 0x00, 0x00,
                     0x48, 0x8B, 0xC1,
                     0x48, 0x8B, 0x4C, 0x24, 0x28,
                     0x48, 0x3B, 0x8F, 0x30, 0x02, 0x00, 0x00,
                     0x74, 0x07,
                     0x48, 0x89, 0x8F, 0x30, 0x02, 0x00, 0x00});
        const auto call_offset = offset + 11;
        write_rel32(call_offset + 1, address(call_offset + 5), address(selector_offset));
    }

    void add_ui_selector_call(std::size_t local_offset, std::size_t selector_offset) {
        ui_selector_call_offsets.push_back(local_offset);
        const auto offset = kUiOffset + local_offset;
        put(offset, {0x4C, 0x8B, 0xC6,
                     0x48, 0x8D, 0x54, 0x24, 0x38,
                     0x49, 0x8B, 0xCD,
                     0xE8, 0, 0, 0, 0,
                     0x48, 0x8B, 0x44, 0x24, 0x38,
                     0x48, 0x85, 0xC0,
                     0x0F, 0x84, 0x46, 0x04, 0x00, 0x00,
                     0x8B, 0x40, 0x08,
                     0xA9, 0x00, 0x00, 0x00, 0x60,
                     0x0F, 0x85, 0x38, 0x04, 0x00, 0x00,
                     0x48, 0x8B, 0x44, 0x24, 0x40,
                     0x48, 0x85, 0xC0});
        const auto call_offset = offset + 11;
        write_rel32(call_offset + 1, address(call_offset + 5), address(selector_offset));
    }

    void retarget_ui_selector_call(std::size_t local_offset, std::size_t target_offset) {
        const auto call_offset = kUiOffset + local_offset + 11;
        write_rel32(call_offset + 1, address(call_offset + 5), address(target_offset));
    }

    void add_exec_dispatch(std::size_t local_offset, std::uint32_t slot_offset) {
        require(slot_offset <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()),
                "selector fixture dispatch slot must fit");
        put(kExecOffset + local_offset,
            {0x48, 0x8B, 0x01, 0x48, 0xFF, 0xA0, 0, 0, 0, 0});
        write_i32(kExecOffset + local_offset + 6, static_cast<std::int32_t>(slot_offset));
        exec_dispatch_offsets.push_back(local_offset);
    }

    [[nodiscard]] std::vector<std::uintptr_t> exec_instructions() const {
        constexpr std::size_t limit = 96;
        std::vector<bool> starts(limit, true);
        const auto mark = [&starts](std::size_t offset, std::size_t length) {
            require(offset + length <= starts.size(), "fixture exec instruction must fit");
            starts[offset] = true;
            for (std::size_t index = 1; index < length; ++index) starts[offset + index] = false;
        };
        mark(0, 4);
        mark(4, 3);
        mark(7, 3);
        mark(10, 4);
        mark(14, 3);
        mark(17, 4);
        for (const auto offset : exec_dispatch_offsets) {
            mark(offset, 3);
            mark(offset + 3, 7);
        }
        std::vector<std::uintptr_t> result{};
        for (std::size_t offset = 0; offset < starts.size(); ++offset) {
            if (starts[offset]) result.push_back(address(kExecOffset + offset));
        }
        return result;
    }

    [[nodiscard]] std::vector<std::uintptr_t> server_instructions() const {
        constexpr std::size_t limit = 0x600;
        std::vector<bool> starts(limit, true);
        const auto mark = [&starts](std::size_t offset, std::size_t length) {
            require(offset + length <= starts.size(), "fixture server instruction must fit");
            starts[offset] = true;
            for (std::size_t index = 1; index < length; ++index) starts[offset + index] = false;
        };
        mark(0, 5);
        mark(5, 1);
        mark(6, 4);
        mark(10, 3);
        constexpr std::array<std::size_t, 14> call_instruction_lengths{
            3, 5, 3, 5, 7, 5, 3, 2, 7, 3, 5, 7, 2, 7,
        };
        for (const auto call_offset : selector_call_offsets) {
            auto offset = call_offset;
            for (const auto length : call_instruction_lengths) {
                mark(offset, length);
                offset += length;
            }
        }
        std::vector<std::uintptr_t> result{};
        for (std::size_t offset = 0; offset < starts.size(); ++offset) {
            if (starts[offset]) result.push_back(address(kServerOffset + offset));
        }
        return result;
    }

    [[nodiscard]] std::vector<std::uintptr_t> ui_instructions() const {
        std::vector<bool> starts(kUiScanLimit, true);
        const auto mark = [&starts](std::size_t offset, std::size_t length) {
            require(offset + length <= starts.size(), "fixture UI instruction must fit");
            starts[offset] = true;
            for (std::size_t index = 1; index < length; ++index) starts[offset + index] = false;
        };
        mark(0, 5);
        mark(5, 5);
        mark(10, 5);
        mark(15, 1);
        constexpr std::array<std::size_t, 12> call_instruction_lengths{
            3, 5, 3, 5, 5, 3, 6, 3, 5, 6, 5, 3,
        };
        for (const auto call_offset : ui_selector_call_offsets) {
            auto offset = call_offset;
            for (const auto length : call_instruction_lengths) {
                mark(offset, length);
                offset += length;
            }
        }
        std::vector<std::uintptr_t> result{};
        for (std::size_t offset = 0; offset < starts.size(); ++offset) {
            if (starts[offset]) result.push_back(address(kUiOffset + offset));
        }
        return result;
    }

    std::uintptr_t base{};
    std::vector<std::uint8_t> text{};
    std::vector<std::size_t> exec_dispatch_offsets{};
    std::vector<std::size_t> selector_call_offsets{};
    std::vector<std::size_t> ui_selector_call_offsets{};
};

struct DirectCallCollectorFixture {
    static constexpr std::uintptr_t kDefaultBase = 0x140000000ULL;
    static constexpr std::size_t kEntryOffset = 0x40;
    static constexpr std::size_t kScanLimit = 0x180;
    static constexpr std::size_t kTargetOffset = 0x300;

    struct InstructionSpan {
        std::size_t offset{};
        std::size_t size{};
    };

    explicit DirectCallCollectorFixture(std::uintptr_t image_base = kDefaultBase,
                                        bool add_default_call = true)
        : base(image_base), text(0x400, 0x90) {
        if (add_default_call) add_call(0x10, address(kTargetOffset));
    }

    [[nodiscard]] std::uintptr_t address(std::size_t offset) const noexcept {
        return base + offset;
    }

    [[nodiscard]] dsnap::ExecutableTextView view() const noexcept {
        return {base, text};
    }

    void put(std::size_t offset, std::initializer_list<std::uint8_t> bytes) {
        require(offset + bytes.size() <= text.size(), "direct-call fixture write must fit");
        std::copy(bytes.begin(), bytes.end(),
                  text.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    void write_i32(std::size_t offset, std::int32_t value) {
        require(offset + sizeof(value) <= text.size(),
                "direct-call fixture int32 write must fit");
        std::memcpy(text.data() + offset, &value, sizeof(value));
    }

    void add_call(std::size_t local_offset, std::uintptr_t target) {
        const auto offset = kEntryOffset + local_offset;
        put(offset, {0xE8, 0, 0, 0, 0});
        const auto displacement = static_cast<std::int64_t>(target) -
            static_cast<std::int64_t>(address(offset + 5));
        require(displacement >= std::numeric_limits<std::int32_t>::min() &&
                    displacement <= std::numeric_limits<std::int32_t>::max(),
                "direct-call fixture rel32 must fit");
        write_i32(offset + 1, static_cast<std::int32_t>(displacement));
        instruction_spans.push_back({local_offset, 5});
    }

    void add_movabs_with_embedded_e8(std::size_t local_offset) {
        put(kEntryOffset + local_offset,
            {0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0xE8, 0x66, 0x77, 0x88});
        instruction_spans.push_back({local_offset, 10});
    }

    void add_terminal_epilogue(std::size_t local_offset) {
        put(kEntryOffset + local_offset,
            {0x48, 0x8B, 0x5C, 0x24, 0x30,
             0x48, 0x8B, 0x74, 0x24, 0x40,
             0x48, 0x83, 0xC4, 0x20,
             0x5F, 0xC3});
        instruction_spans.push_back({local_offset, 5});
        instruction_spans.push_back({local_offset + 5, 5});
        instruction_spans.push_back({local_offset + 10, 4});
        instruction_spans.push_back({local_offset + 14, 1});
        instruction_spans.push_back({local_offset + 15, 1});
    }

    [[nodiscard]] std::vector<std::uintptr_t> instructions(
        std::size_t scan_limit = kScanLimit) const {
        std::vector<bool> starts(scan_limit, true);
        for (const auto& instruction : instruction_spans) {
            if (instruction.offset >= starts.size()) continue;
            require(instruction.offset + instruction.size <= starts.size(),
                    "decoded direct-call fixture instruction must fit scan");
            for (std::size_t index = 1; index < instruction.size; ++index) {
                starts[instruction.offset + index] = false;
            }
        }
        std::vector<std::uintptr_t> result{};
        for (std::size_t offset = 0; offset < starts.size(); ++offset) {
            if (starts[offset]) result.push_back(address(kEntryOffset + offset));
        }
        return result;
    }

    std::uintptr_t base{};
    std::vector<std::uint8_t> text{};
    std::vector<InstructionSpan> instruction_spans{};
};

[[nodiscard]] bool direct_call_result_zeroed(
    const dsnap::DirectRel32CallCollectionResult& result) {
    return result.count == 0 &&
        std::all_of(result.calls.begin(), result.calls.end(),
                    [](const dsnap::DirectRel32Call& call) {
                        return call.call_site == 0 && call.target == 0;
                    });
}

void test_direct_rel32_call_collector() {
    DirectCallCollectorFixture fixture{};
    const auto result = dsnap::collect_direct_rel32_calls(
        fixture.view(), fixture.address(DirectCallCollectorFixture::kEntryOffset),
        fixture.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(result.resolved() && result.count == 1 &&
                result.calls[0].call_site == fixture.address(
                    DirectCallCollectorFixture::kEntryOffset + 0x10) &&
                result.calls[0].target ==
                    fixture.address(DirectCallCollectorFixture::kTargetOffset),
            "a decoded in-text E8 rel32 should be collected with its call site and target");

    DirectCallCollectorFixture multiple{};
    multiple.add_call(0x30,
                      multiple.address(DirectCallCollectorFixture::kTargetOffset + 0x10));
    const auto multiple_result = dsnap::collect_direct_rel32_calls(
        multiple.view(), multiple.address(DirectCallCollectorFixture::kEntryOffset),
        multiple.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(multiple_result.resolved() && multiple_result.count == 2 &&
                multiple_result.calls[0].target ==
                    multiple.address(DirectCallCollectorFixture::kTargetOffset) &&
                multiple_result.calls[1].target ==
                    multiple.address(DirectCallCollectorFixture::kTargetOffset + 0x10),
            "the collector should preserve multiple bounded calls for upper-layer validation");

    DirectCallCollectorFixture relocated{0x180000000ULL};
    const auto relocated_result = dsnap::collect_direct_rel32_calls(
        relocated.view(), relocated.address(DirectCallCollectorFixture::kEntryOffset),
        relocated.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(relocated_result.resolved() && relocated_result.count == 1 &&
                relocated_result.calls[0].target ==
                    relocated.address(DirectCallCollectorFixture::kTargetOffset),
            "the rel32 collector must resolve targets relative to a relocated image base");

    DirectCallCollectorFixture embedded{};
    embedded.add_movabs_with_embedded_e8(0x60);
    const auto embedded_result = dsnap::collect_direct_rel32_calls(
        embedded.view(), embedded.address(DirectCallCollectorFixture::kEntryOffset),
        embedded.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(embedded_result.resolved() && embedded_result.count == 1 &&
                embedded_result.calls[0].call_site == embedded.address(
                    DirectCallCollectorFixture::kEntryOffset + 0x10),
            "an E8 byte inside a decoded MOVABS immediate must not become a call candidate");

    DirectCallCollectorFixture out_of_text{};
    out_of_text.add_call(
        0x30, out_of_text.address(out_of_text.text.size() + 0x20));
    const auto out_of_text_result = dsnap::collect_direct_rel32_calls(
        out_of_text.view(),
        out_of_text.address(DirectCallCollectorFixture::kEntryOffset),
        out_of_text.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(out_of_text_result.status ==
                dsnap::SelectorResolverStatus::DirectCallTargetInvalid &&
                direct_call_result_zeroed(out_of_text_result),
            "one out-of-text target must reject and zero the entire collected call set");

    DirectCallCollectorFixture capacity{
        DirectCallCollectorFixture::kDefaultBase, false};
    for (std::size_t index = 0;
         index <= dsnap::kDirectRel32CallCapacity; ++index) {
        capacity.add_call(0x08 + index * 8,
                          capacity.address(DirectCallCollectorFixture::kTargetOffset));
    }
    const auto capacity_result = dsnap::collect_direct_rel32_calls(
        capacity.view(), capacity.address(DirectCallCollectorFixture::kEntryOffset),
        capacity.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(capacity_result.status ==
                dsnap::SelectorResolverStatus::DirectCallCapacityExceeded &&
                direct_call_result_zeroed(capacity_result) &&
                dsnap::selector_resolver_status_name(capacity_result.status) ==
                    "direct_call_capacity_exceeded",
            "exceeding fixed call capacity must fail closed without partial candidates");

    auto invalid_boundaries = fixture.instructions();
    invalid_boundaries.push_back(fixture.address(
        DirectCallCollectorFixture::kEntryOffset +
        DirectCallCollectorFixture::kScanLimit));
    const auto invalid_boundary_result = dsnap::collect_direct_rel32_calls(
        fixture.view(), fixture.address(DirectCallCollectorFixture::kEntryOffset),
        invalid_boundaries, DirectCallCollectorFixture::kScanLimit);
    require(invalid_boundary_result.status == dsnap::SelectorResolverStatus::InvalidInput &&
                direct_call_result_zeroed(invalid_boundary_result),
            "an instruction boundary at or beyond the bounded scan must invalidate all output");

    auto missing_call_end_boundary = fixture.instructions();
    missing_call_end_boundary.erase(
        std::remove(missing_call_end_boundary.begin(),
                    missing_call_end_boundary.end(),
                    fixture.address(DirectCallCollectorFixture::kEntryOffset + 0x15)),
        missing_call_end_boundary.end());
    const auto missing_call_end_result = dsnap::collect_direct_rel32_calls(
        fixture.view(), fixture.address(DirectCallCollectorFixture::kEntryOffset),
        missing_call_end_boundary, DirectCallCollectorFixture::kScanLimit);
    require(missing_call_end_result.status == dsnap::SelectorResolverStatus::InvalidInput &&
                direct_call_result_zeroed(missing_call_end_result),
            "a decoded E8 must be followed by its exact five-byte instruction boundary");

    std::array<std::uint8_t, 0x40> overflow_bytes{};
    const auto overflow_base = std::numeric_limits<std::uintptr_t>::max() - 0x20;
    const auto overflow_entry = overflow_base + 0x10;
    const std::array overflow_instructions{overflow_entry};
    const auto overflow_result = dsnap::collect_direct_rel32_calls(
        {overflow_base, overflow_bytes}, overflow_entry, overflow_instructions, 0x20);
    require(overflow_result.status == dsnap::SelectorResolverStatus::InvalidInput &&
                direct_call_result_zeroed(overflow_result),
            "a bounded scan whose checked end address overflows must fail closed");

    DirectCallCollectorFixture outside_scan{
        DirectCallCollectorFixture::kDefaultBase, false};
    outside_scan.add_call(
        DirectCallCollectorFixture::kScanLimit + 0x10,
        outside_scan.address(DirectCallCollectorFixture::kTargetOffset));
    const auto outside_scan_result = dsnap::collect_direct_rel32_calls(
        outside_scan.view(),
        outside_scan.address(DirectCallCollectorFixture::kEntryOffset),
        outside_scan.instructions(), DirectCallCollectorFixture::kScanLimit);
    require(outside_scan_result.status ==
                dsnap::SelectorResolverStatus::DirectCallNotFound &&
                direct_call_result_zeroed(outside_scan_result),
            "a direct call beyond the bounded scan must not be collected");
}

void test_unique_terminal_rel32_call_selector() {
    constexpr std::size_t kTerminalFragmentSize = 0x25;
    DirectCallCollectorFixture fixture{
        DirectCallCollectorFixture::kDefaultBase, false};
    fixture.add_call(0x10, fixture.address(DirectCallCollectorFixture::kTargetOffset));
    fixture.add_terminal_epilogue(0x15);
    const auto result = dsnap::select_unique_terminal_rel32_call(
        fixture.view(), fixture.address(DirectCallCollectorFixture::kEntryOffset),
        fixture.instructions(kTerminalFragmentSize), kTerminalFragmentSize);
    require(result.resolved() && result.candidates == 1 &&
                result.call.call_site == fixture.address(
                    DirectCallCollectorFixture::kEntryOffset + 0x10) &&
                result.call.target ==
                    fixture.address(DirectCallCollectorFixture::kTargetOffset),
            "one call followed only by restores, stack unwind, pop, and ret should resolve");

    DirectCallCollectorFixture relocated{
        0x180000000ULL, false};
    relocated.add_call(0x10,
                       relocated.address(DirectCallCollectorFixture::kTargetOffset));
    relocated.add_terminal_epilogue(0x15);
    const auto relocated_result = dsnap::select_unique_terminal_rel32_call(
        relocated.view(), relocated.address(DirectCallCollectorFixture::kEntryOffset),
        relocated.instructions(kTerminalFragmentSize), kTerminalFragmentSize);
    require(relocated_result.resolved() && relocated_result.call.target ==
                relocated.address(DirectCallCollectorFixture::kTargetOffset),
            "terminal-call selection must preserve rel32 relocation support");

    DirectCallCollectorFixture ordinary_call{};
    ordinary_call.put(DirectCallCollectorFixture::kEntryOffset + 0x16, {0xC3});
    constexpr std::size_t kOrdinaryFragmentSize = 0x17;
    const auto ordinary_result = dsnap::select_unique_terminal_rel32_call(
        ordinary_call.view(),
        ordinary_call.address(DirectCallCollectorFixture::kEntryOffset),
        ordinary_call.instructions(kOrdinaryFragmentSize), kOrdinaryFragmentSize);
    require(ordinary_result.status ==
                dsnap::SelectorResolverStatus::TerminalCallNotFound &&
                ordinary_result.call.call_site == 0 && ordinary_result.call.target == 0 &&
                ordinary_result.candidates == 0,
            "the last E8 alone must not qualify without a recognized terminal epilogue");

    DirectCallCollectorFixture business_mov{
        DirectCallCollectorFixture::kDefaultBase, false};
    business_mov.add_call(
        0x10, business_mov.address(DirectCallCollectorFixture::kTargetOffset));
    business_mov.put(DirectCallCollectorFixture::kEntryOffset + 0x15,
                     {0x48, 0x8B, 0xC1});
    business_mov.instruction_spans.push_back({0x15, 3});
    business_mov.add_terminal_epilogue(0x18);
    const auto business_mov_result = dsnap::select_unique_terminal_rel32_call(
        business_mov.view(),
        business_mov.address(DirectCallCollectorFixture::kEntryOffset),
        business_mov.instructions(0x28), 0x28);
    require(business_mov_result.status ==
                dsnap::SelectorResolverStatus::TerminalCallNotFound &&
                business_mov_result.call.call_site == 0 &&
                business_mov_result.call.target == 0,
            "a generic MOV after the call is business logic, not a callee-saved restore");

    DirectCallCollectorFixture exact_fragment_end{
        DirectCallCollectorFixture::kDefaultBase, false};
    exact_fragment_end.add_call(
        0x10, exact_fragment_end.address(DirectCallCollectorFixture::kTargetOffset));
    exact_fragment_end.add_terminal_epilogue(0x15);
    exact_fragment_end.add_call(
        0x60, exact_fragment_end.address(DirectCallCollectorFixture::kTargetOffset + 0x10));
    exact_fragment_end.add_terminal_epilogue(0x65);
    constexpr std::size_t kTwoEpilogueFragmentSize = 0x75;
    const auto exact_fragment_end_result = dsnap::select_unique_terminal_rel32_call(
        exact_fragment_end.view(),
        exact_fragment_end.address(DirectCallCollectorFixture::kEntryOffset),
        exact_fragment_end.instructions(kTwoEpilogueFragmentSize),
        kTwoEpilogueFragmentSize);
    require(exact_fragment_end_result.resolved() &&
                exact_fragment_end_result.candidates == 1 &&
                exact_fragment_end_result.call.call_site == exact_fragment_end.address(
                    DirectCallCollectorFixture::kEntryOffset + 0x60),
            "a mid-fragment RET must not make an earlier call terminal at the exact fragment boundary");
}

void test_dynamic_selector_resolver() {
    SelectorResolverFixture fixture{};
    const auto fixture_exec_instructions = fixture.exec_instructions();
    const auto fixture_server_instructions = fixture.server_instructions();
    const auto dispatch = dsnap::resolve_virtual_dispatch_slot(
        fixture.view(), fixture.address(SelectorResolverFixture::kExecOffset),
        fixture_exec_instructions);
    require(dispatch.resolved() && dispatch.slot_offset == 0x4C0,
            "reflection exec thunk should expose the current virtual dispatch slot");

    const auto server = dsnap::follow_direct_jump_chain(
        fixture.view(), fixture.address(SelectorResolverFixture::kServerThunkOffset));
    require(server.resolved() && server.address == fixture.address(SelectorResolverFixture::kServerOffset) &&
                server.jumps == 1,
            "the current vtable thunk should resolve to the native server implementation");

    const auto selector = dsnap::resolve_selector_call(
        fixture.view(), server.address,
        fixture_server_instructions,
        SelectorResolverFixture::kObjectOffset,
        SelectorResolverFixture::kComponentOffset);
    require(selector.resolved() &&
                selector.selector_address == fixture.address(SelectorResolverFixture::kSelectorOffset) &&
                selector.call_site == fixture.address(SelectorResolverFixture::kServerOffset + 0x50 + 11),
            "reflection-anchored local analysis should resolve the unique selector call");
    require(dsnap::selector_resolver_status_name(selector.status) == "resolved",
            "resolver status should have a stable diagnostic name");

    SelectorResolverFixture shifted{0x180000000ULL};
    const auto shifted_server_instructions = shifted.server_instructions();
    const auto shifted_selector = dsnap::resolve_selector_call(
        shifted.view(), shifted.address(SelectorResolverFixture::kServerOffset),
        shifted_server_instructions,
        SelectorResolverFixture::kObjectOffset,
        SelectorResolverFixture::kComponentOffset);
    require(shifted_selector.resolved() &&
                shifted_selector.selector_address == shifted.address(SelectorResolverFixture::kSelectorOffset),
            "resolver output must follow a relocated image without a fixed RVA");

    auto wrong_layout = dsnap::resolve_selector_call(
        fixture.view(), fixture.address(SelectorResolverFixture::kServerOffset),
        fixture_server_instructions, 0x220, 0x228);
    require(wrong_layout.status == dsnap::SelectorResolverStatus::SelectorNotFound &&
                wrong_layout.selector_address == 0,
            "a reflected property-layout mismatch must fail closed");
    const auto malformed_layout = dsnap::resolve_selector_call(
        fixture.view(), fixture.address(SelectorResolverFixture::kServerOffset),
        fixture_server_instructions, 0x228, 0x238);
    require(malformed_layout.status == dsnap::SelectorResolverStatus::PropertyLayoutInvalid &&
                malformed_layout.selector_address == 0,
            "non-adjacent reflected target properties must be rejected before scanning");

    SelectorResolverFixture ambiguous{};
    ambiguous.add_selector_call(0x100, SelectorResolverFixture::kSelectorOffset);
    const auto ambiguous_server_instructions = ambiguous.server_instructions();
    const auto ambiguous_result = dsnap::resolve_selector_call(
        ambiguous.view(), ambiguous.address(SelectorResolverFixture::kServerOffset),
        ambiguous_server_instructions,
        SelectorResolverFixture::kObjectOffset,
        SelectorResolverFixture::kComponentOffset);
    require(ambiguous_result.status == dsnap::SelectorResolverStatus::SelectorAmbiguous &&
                ambiguous_result.candidates == 2 && ambiguous_result.selector_address == 0,
            "multiple structurally valid calls must fail closed without choosing one");

    SelectorResolverFixture invalid_target{};
    invalid_target.text[SelectorResolverFixture::kSelectorOffset] = 0x90;
    const auto invalid_target_server_instructions = invalid_target.server_instructions();
    const auto invalid_target_result = dsnap::resolve_selector_call(
        invalid_target.view(), invalid_target.address(SelectorResolverFixture::kServerOffset),
        invalid_target_server_instructions,
        SelectorResolverFixture::kObjectOffset,
        SelectorResolverFixture::kComponentOffset);
    require(invalid_target_result.status == dsnap::SelectorResolverStatus::SelectorNotFound &&
                invalid_target_result.selector_address == 0,
            "a call target without the selector output contract must fail closed");

    SelectorResolverFixture duplicate_selector_contract{};
    duplicate_selector_contract.put(0xA00,
        {0x48, 0x8B, 0xC4, 0x4C, 0x89, 0x40, 0x18, 0x48,
         0x89, 0x50, 0x10, 0x48, 0x89, 0x48, 0x08});
    duplicate_selector_contract.put(0xA70,
        {0x33, 0xC0, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08});
    const auto duplicate_selector_instructions =
        duplicate_selector_contract.server_instructions();
    const auto duplicate_selector_result = dsnap::resolve_selector_call(
        duplicate_selector_contract.view(),
        duplicate_selector_contract.address(SelectorResolverFixture::kServerOffset),
        duplicate_selector_instructions,
        SelectorResolverFixture::kObjectOffset,
        SelectorResolverFixture::kComponentOffset);
    require(duplicate_selector_result.status == dsnap::SelectorResolverStatus::SelectorAmbiguous &&
                duplicate_selector_result.candidates == 2 &&
                duplicate_selector_result.selector_address == 0,
            "a second raw selector contract anywhere in executable text must fail closed");

    SelectorResolverFixture ambiguous_dispatch{};
    ambiguous_dispatch.add_exec_dispatch(0x28, 0x4C8);
    const auto ambiguous_exec_instructions = ambiguous_dispatch.exec_instructions();
    const auto ambiguous_slot = dsnap::resolve_virtual_dispatch_slot(
        ambiguous_dispatch.view(), ambiguous_dispatch.address(SelectorResolverFixture::kExecOffset),
        ambiguous_exec_instructions);
    require(ambiguous_slot.status == dsnap::SelectorResolverStatus::DispatchAmbiguous &&
                ambiguous_slot.slot_offset == 0,
            "multiple virtual dispatch slots near the reflection anchor must fail closed");

    SelectorResolverFixture duplicate_dispatch{};
    duplicate_dispatch.add_exec_dispatch(0x28, 0x4C0);
    const auto duplicate_exec_instructions = duplicate_dispatch.exec_instructions();
    const auto duplicate_slot = dsnap::resolve_virtual_dispatch_slot(
        duplicate_dispatch.view(), duplicate_dispatch.address(SelectorResolverFixture::kExecOffset),
        duplicate_exec_instructions);
    require(duplicate_slot.status == dsnap::SelectorResolverStatus::DispatchAmbiguous &&
                duplicate_slot.candidates == 2 && duplicate_slot.slot_offset == 0,
            "duplicate dispatch matches must remain ambiguous even when they name the same slot");

    SelectorResolverFixture embedded_dispatch{};
    embedded_dispatch.put(SelectorResolverFixture::kExecOffset + 0x40,
                          {0x48, 0x8B, 0x01, 0x48, 0xFF, 0xA0, 0xC8, 0x04, 0x00, 0x00});
    auto embedded_exec_instructions = embedded_dispatch.exec_instructions();
    embedded_exec_instructions.erase(
        std::remove(embedded_exec_instructions.begin(), embedded_exec_instructions.end(),
                    embedded_dispatch.address(SelectorResolverFixture::kExecOffset + 0x40)),
        embedded_exec_instructions.end());
    const auto embedded_slot = dsnap::resolve_virtual_dispatch_slot(
        embedded_dispatch.view(), embedded_dispatch.address(SelectorResolverFixture::kExecOffset),
        embedded_exec_instructions);
    require(embedded_slot.resolved() && embedded_slot.slot_offset == 0x4C0 &&
                embedded_slot.candidates == 1,
            "dispatch-like bytes inside a decoded instruction must not become a candidate");

    SelectorResolverFixture jump_loop{};
    jump_loop.write_rel32(SelectorResolverFixture::kServerThunkOffset + 1,
                          jump_loop.address(SelectorResolverFixture::kServerThunkOffset + 5),
                          jump_loop.address(SelectorResolverFixture::kServerThunkOffset));
    const auto loop_result = dsnap::follow_direct_jump_chain(
        jump_loop.view(), jump_loop.address(SelectorResolverFixture::kServerThunkOffset));
    require(loop_result.status == dsnap::SelectorResolverStatus::JumpTargetInvalid &&
                loop_result.address == 0,
            "a direct-jump loop must fail closed");
    const auto excessive_jump_limit = dsnap::follow_direct_jump_chain(
        fixture.view(), fixture.address(SelectorResolverFixture::kServerThunkOffset),
        std::numeric_limits<std::uint32_t>::max());
    require(excessive_jump_limit.status == dsnap::SelectorResolverStatus::InvalidInput &&
                excessive_jump_limit.address == 0,
            "an excessive jump limit must be rejected before its counter can wrap");
}

void test_ui_selector_consensus() {
    SelectorResolverFixture fixture{};
    const auto instructions = fixture.ui_instructions();
    const auto result = dsnap::resolve_ui_selector_call(
        fixture.view(), fixture.address(SelectorResolverFixture::kUiOffset),
        instructions, SelectorResolverFixture::kUiScanLimit);
    require(result.resolved() &&
                result.selector_address ==
                    fixture.address(SelectorResolverFixture::kSelectorOffset) &&
                result.call_site == fixture.address(
                    SelectorResolverFixture::kUiOffset +
                    SelectorResolverFixture::kUiCallOffset + 11) &&
                result.candidates == 1,
            "the independent UI caller should resolve the globally unique selector contract");

    SelectorResolverFixture relocated{0x180000000ULL};
    const auto relocated_result = dsnap::resolve_ui_selector_call(
        relocated.view(), relocated.address(SelectorResolverFixture::kUiOffset),
        relocated.ui_instructions(), SelectorResolverFixture::kUiScanLimit);
    require(relocated_result.resolved() &&
                relocated_result.selector_address ==
                    relocated.address(SelectorResolverFixture::kSelectorOffset),
            "the UI caller consensus must follow a relocated image and rel32 target");

    SelectorResolverFixture absent{};
    absent.text[SelectorResolverFixture::kUiOffset +
                SelectorResolverFixture::kUiCallOffset + 2] = 0xCE;
    const auto absent_result = dsnap::resolve_ui_selector_call(
        absent.view(), absent.address(SelectorResolverFixture::kUiOffset),
        absent.ui_instructions(), SelectorResolverFixture::kUiScanLimit);
    require(absent_result.status == dsnap::SelectorResolverStatus::SelectorNotFound &&
                absent_result.selector_address == 0 && absent_result.call_site == 0 &&
                absent_result.candidates == 0,
            "a UI call without the required third-argument setup must fail closed");

    SelectorResolverFixture ambiguous{};
    ambiguous.add_ui_selector_call(0x140, SelectorResolverFixture::kSelectorOffset);
    const auto ambiguous_result = dsnap::resolve_ui_selector_call(
        ambiguous.view(), ambiguous.address(SelectorResolverFixture::kUiOffset),
        ambiguous.ui_instructions(), SelectorResolverFixture::kUiScanLimit);
    require(ambiguous_result.status == dsnap::SelectorResolverStatus::SelectorAmbiguous &&
                ambiguous_result.selector_address == 0 && ambiguous_result.call_site == 0 &&
                ambiguous_result.candidates == 2,
            "multiple structurally valid UI selector callers must remain ambiguous");

    SelectorResolverFixture different_target{};
    different_target.retarget_ui_selector_call(
        SelectorResolverFixture::kUiCallOffset,
        SelectorResolverFixture::kAlternateTargetOffset);
    const auto different_target_result = dsnap::resolve_ui_selector_call(
        different_target.view(),
        different_target.address(SelectorResolverFixture::kUiOffset),
        different_target.ui_instructions(), SelectorResolverFixture::kUiScanLimit);
    require(different_target_result.status ==
                dsnap::SelectorResolverStatus::SelectorNotFound &&
                different_target_result.selector_address == 0 &&
                different_target_result.call_site == 0,
            "a UI caller targeting anything other than the unique selector contract must fail closed");

    SelectorResolverFixture embedded{};
    constexpr std::size_t kEmbeddedInstructionOffset = 0x1C0;
    embedded.put(SelectorResolverFixture::kUiOffset + kEmbeddedInstructionOffset,
                 {0x48, 0xB8, 0x11, 0x22, 0x33, 0x44, 0xE8, 0x66, 0x77, 0x88});
    auto embedded_instructions = embedded.ui_instructions();
    const auto embedded_start = embedded.address(
        SelectorResolverFixture::kUiOffset + kEmbeddedInstructionOffset);
    const auto embedded_end = embedded_start + 10;
    embedded_instructions.erase(
        std::remove_if(embedded_instructions.begin(), embedded_instructions.end(),
                       [embedded_start, embedded_end](std::uintptr_t address) {
                           return address > embedded_start && address < embedded_end;
                       }),
        embedded_instructions.end());
    const auto embedded_result = dsnap::resolve_ui_selector_call(
        embedded.view(), embedded.address(SelectorResolverFixture::kUiOffset),
        embedded_instructions, SelectorResolverFixture::kUiScanLimit);
    require(embedded_result.resolved() && embedded_result.candidates == 1 &&
                embedded_result.call_site == embedded.address(
                    SelectorResolverFixture::kUiOffset +
                    SelectorResolverFixture::kUiCallOffset + 11),
            "an E8 byte embedded inside a decoded instruction must not become a UI call candidate");

    SelectorResolverFixture jump_target{};
    jump_target.put(SelectorResolverFixture::kAlternateTargetOffset,
                    {0xE9, 0, 0, 0, 0});
    jump_target.write_rel32(
        SelectorResolverFixture::kAlternateTargetOffset + 1,
        jump_target.address(SelectorResolverFixture::kAlternateTargetOffset + 5),
        jump_target.address(SelectorResolverFixture::kSelectorOffset));
    jump_target.retarget_ui_selector_call(
        SelectorResolverFixture::kUiCallOffset,
        SelectorResolverFixture::kAlternateTargetOffset);
    const auto jump_target_result = dsnap::resolve_ui_selector_call(
        jump_target.view(), jump_target.address(SelectorResolverFixture::kUiOffset),
        jump_target.ui_instructions(), SelectorResolverFixture::kUiScanLimit);
    require(jump_target_result.resolved() &&
                jump_target_result.selector_address ==
                    jump_target.address(SelectorResolverFixture::kSelectorOffset),
            "the UI consensus should follow a bounded direct jump to the unique selector contract");
}

void test_game_interaction_binding() {
    const auto c_binding = dsnap::parse_game_interaction_binding(R"(
KeyboardConfigs=(ActionInputType=90,Bind1_Key1=5,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)
KeyboardConfigs=(ActionInputType=91,Bind1_Key1=3,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)
GamepadConfigs=(ActionInputType=91,Bind1_Key1=151,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)
)");
    require(c_binding.valid(), "the saved INTERACT binding should parse independently of adjacent actions");
    require(c_binding.value.keyboard_key == "C", "button key code 3 should resolve to Unreal key C");
    require(c_binding.value.keyboard_key_code == 3, "the saved keyboard code should be retained for diagnostics");
    require(c_binding.value.gamepad_key == "Gamepad_FaceButton_Top",
            "the saved controller binding should be retained for diagnostics");

    const auto k_binding = dsnap::parse_game_interaction_binding(
        "KeyboardConfigs=(ActionInputType=91,Bind1_Key1=11,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)\n");
    require(k_binding.valid() && k_binding.value.keyboard_key == "K",
            "button key code 11 should resolve to Unreal key K");

    const auto secondary_binding = dsnap::parse_game_interaction_binding(
        "KeyboardConfigs=(ActionInputType=91,Bind1_Key1=0,Bind1_Key2=0,Bind2_Key1=6,Bind2_Key2=0)\n");
    require(secondary_binding.valid() && secondary_binding.value.keyboard_key == "F",
            "an unchorded secondary INTERACT binding should be accepted");

    const auto missing = dsnap::parse_game_interaction_binding(
        "KeyboardConfigs=(ActionInputType=90,Bind1_Key1=3,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)\n");
    require(!missing.valid(), "AUTO must fail closed when semantic INTERACT is absent");

    const auto chorded = dsnap::parse_game_interaction_binding(
        "KeyboardConfigs=(ActionInputType=91,Bind1_Key1=3,Bind1_Key2=87,Bind2_Key1=0,Bind2_Key2=0)\n");
    require(!chorded.valid(), "unsupported chorded INTERACT bindings must fail closed");

    const auto duplicate = dsnap::parse_game_interaction_binding(R"(
KeyboardConfigs=(ActionInputType=91,Bind1_Key1=3,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)
KeyboardConfigs=(ActionInputType=91,Bind1_Key1=11,Bind1_Key2=0,Bind2_Key1=0,Bind2_Key2=0)
)");
    require(!duplicate.valid(), "ambiguous duplicate semantic INTERACT bindings must fail closed");
}

void test_session_calibration() {
    dsnap::SessionCalibration calibration{};
    require(!calibration.observe(10, false), "rejected manual calls must not calibrate");
    require(!calibration.observe(10, true), "first exact object starts calibration");
    require(calibration.matches() == 1, "first distinct match should be recorded once");
    require(!calibration.observe(10, true), "same object cannot satisfy the second sample");
    require(calibration.observe(11, true), "second distinct exact object accepts the session contract");
    require(calibration.accepted(), "session contract should remain accepted");
    require(calibration.matches() == 2, "accepted calibration requires exactly two distinct objects");
    require(!calibration.observe(12, true), "accepted session must not emit a second acceptance transition");
}

void test_callback_generation_gate() {
    dsnap::CallbackGenerationGate first{10};
    require(first.accepts(10), "current callback generation should be accepted");
    require(!first.accepts(9), "old callback generation must be rejected");
    require(!first.accepts(11), "future callback generation must be rejected");
    first.invalidate();
    require(!first.accepts(10), "invalidated instance must reject its own callbacks");

    dsnap::CallbackGenerationGate second{11};
    require(!second.accepts(10), "new instance must reject accumulated old callbacks");
    require(second.accepts(11), "new instance accepts only its own callback token");
}

void test_session_toggle_event_gate() {
    dsnap::SessionToggleEventGate events{};
    const auto initial_generation = events.current_generation();
    require(!events.record_key_event(),
            "a key before the game thread publishes playability must be rejected");
    require(events.mark_playable(initial_generation),
            "the current session should accept a game-thread playability publication");
    require(events.record_key_event(), "a key in a playable session should be recorded");
    require(events.record_key_event(), "a second key in a playable session should be recorded");
    const auto first_keys = events.drain_key_events();
    require(first_keys.generation != 0 && first_keys.count == 2,
            "current-session key events should coalesce without losing their generation");
    require(events.publish_toggle_request(first_keys.generation),
            "a current-session key batch should publish a toggle request");
    const auto first_toggles = events.drain_toggle_requests();
    require(first_toggles.generation == first_keys.generation && first_toggles.count == 1,
            "the game-thread toggle stage should retain the source generation");

    require(events.record_key_event(), "the playable session should accept another key");
    const auto stale_keys = events.drain_key_events();
    const auto next_generation = events.reset();
    require(next_generation != stale_keys.generation,
            "World reset should advance the session generation");
    require(!events.publish_toggle_request(stale_keys.generation),
            "a key captured before reset must not publish after reset");
    require(events.drain_toggle_requests().count == 0,
            "World reset should leave no stale toggle request");

    require(!events.record_key_event(),
            "a key during the new session's loading phase must be rejected");
    require(!events.mark_playable(stale_keys.generation),
            "the game thread must not publish an old session as playable");
    require(events.mark_playable(next_generation),
            "the game thread should publish the new session only after it becomes playable");
    require(events.record_key_event(), "a post-load key should bind to the ready session");
    const auto current_keys = events.drain_key_events();
    require(current_keys.generation == next_generation && current_keys.count == 1,
            "a post-reset key should bind to the new generation");
    require(events.publish_toggle_request(current_keys.generation),
            "a post-reset key should remain actionable in its own generation");
    static_cast<void>(events.reset());
    require(events.drain_toggle_requests().count == 0,
            "a reset racing the game-thread drain must clear the prior request");
}

void test_experimental_nested_fingerprint_policy() {
    require(dsnap::expected_ue4ss_sha256(
                "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1"),
            "the pinned ExperimentalNested UE4SS build should be accepted");
    require(!dsnap::expected_ue4ss_sha256(
                "8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26"),
            "the StableRoot UE4SS build must fail closed in the ExperimentalNested-only release");
    require(!dsnap::expected_ue4ss_sha256(std::string(64, '0')),
            "an unknown UE4SS build must fail closed");
}

void test_whitelist_and_bounds() {
    dsnap::CandidateQueue queue{2, 2, std::chrono::milliseconds{500}};
    queue.reset(5);
    require(!queue.register_candidate({1, 1}, "/Script/DS.TreasureBox", 5), "chests must be rejected");
    require(!queue.register_candidate({1, 1}, dsnap::kAllowedClassPath, 4), "stale epochs must be rejected");
    require(queue.register_candidate({1, 1}, dsnap::kAllowedClassPath, 5), "first allowed candidate should register");
    require(queue.register_candidate({2, 1}, dsnap::kAllowedClassPath, 5), "second allowed candidate should register");
    require(!queue.register_candidate({3, 1}, dsnap::kAllowedClassPath, 5), "queue bound must be enforced");
}

void test_epoch_invalidation() {
    dsnap::Configuration config{};
    dsnap::InteractionContract contract{true, 1};
    dsnap::PickupController controller{config, contract};
    controller.set_build_trusted(true);
    const dsnap::WeakObjectId object{10, 20};
    require(controller.observe_candidate(object, dsnap::kAllowedClassPath), "candidate should register in current epoch");
    require(controller.update_candidate(object, valid_candidate()), "candidate should validate");
    require(controller.request_active(true), "approved test-only contract should permit active state");
    require(controller.plan_action(std::chrono::steady_clock::now()).has_value(), "eligible candidate should plan");
    controller.reset_world();
    require(!controller.active(), "world reset must disable active mode");
    require(controller.queue_size() == 0, "world reset must clear candidates");
}

void test_fail_closed_validation() {
    dsnap::Configuration config{};
    dsnap::InteractionContract contract{true, 1};
    dsnap::PickupController controller{config, contract};
    controller.set_build_trusted(true);
    const dsnap::WeakObjectId object{12, 4};
    require(controller.observe_candidate(object, dsnap::kAllowedClassPath), "candidate should register");
    auto invalid = valid_candidate();
    invalid.interact_type = 4;
    require(controller.update_candidate(object, invalid), "invalid candidate state should still be recorded");
    require(controller.request_active(true), "test-only contract should activate");
    require(!controller.plan_action(std::chrono::steady_clock::now()).has_value(), "TreasureBox interact type must not plan");
}

void test_async_logger_batch_flush() {
    const auto root = std::filesystem::temp_directory_path() /
        ("dsnap-logger-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto user_log = root / "user.log";
    const auto debug_log = root / "debug.log";
    std::int64_t enqueue_upper_bound_ms{};
    {
        dsnap::AsyncLogger logger{user_log, debug_log, 8};
        logger.set_tick_sequence(41);
        logger.write(dsnap::LogAudience::User, "USER_ONE", "value=1");
        logger.write(dsnap::LogAudience::Debug, "DEBUG_ONE", "value=2");
        logger.set_tick_sequence(42);
        logger.write(dsnap::LogAudience::User, "USER_TWO", "value=3");
        enqueue_upper_bound_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
        require(logger.peak_queue_size() == 3, "batch logger should expose its bounded peak queue depth");
        require(logger.flush_all() == 3, "batch logger should flush every queued message");
        require(logger.failed_messages() == 0, "successful batch logging must report no failed messages");
        require(logger.dropped_messages() == 0, "a queue within capacity must report no dropped messages");
    }

    {
        dsnap::AsyncLogger logger{user_log, debug_log, 2};
        logger.write(dsnap::LogAudience::User, "BOUNDED_ONE");
        logger.write(dsnap::LogAudience::User, "BOUNDED_TWO");
        logger.write(dsnap::LogAudience::User, "BOUNDED_DROPPED");
        require(logger.peak_queue_size() == 2, "logger peak depth must never exceed the configured capacity");
        require(logger.dropped_messages() == 1, "logger should expose bounded-queue drops for PERF diagnostics");
        require(logger.flush_all() == 2, "bounded logger should flush only retained messages");
    }

    const auto read_all = [](const std::filesystem::path& path) {
        std::ifstream input{path, std::ios::binary};
        return std::string{std::istreambuf_iterator<char>{input}, {}};
    };
    const auto user_text = read_all(user_log);
    const auto debug_text = read_all(debug_log);
    std::smatch match{};
    require(std::regex_search(user_text, match,
                              std::regex{"USER_ONE\\tsequence=(\\d+) utc_unix_ms=(\\d+) "
                                         "session_elapsed_ms=(\\d+) tick_sequence=(\\d+)\\tvalue=1"}),
            "batch logger should emit enqueue metadata for the first user event");
    require(match[1].str() == "1", "logger sequence should start at one at enqueue");
    require(std::stoll(match[2].str()) <= enqueue_upper_bound_ms,
            "logger UTC time must be captured before the delayed flush");
    require(match[4].str() == "41", "logger should capture the current tick at enqueue");
    require(user_text.find("USER_TWO\tsequence=3") != std::string::npos,
            "logger sequence should span both audiences in enqueue order");
    require(user_text.find("tick_sequence=42\tvalue=3") != std::string::npos,
            "logger should capture a later tick for a later event");
    require(debug_text.find("DEBUG_ONE\tsequence=2") != std::string::npos,
            "debug output should preserve the global enqueue sequence");
    require(debug_text.find("tick_sequence=41\tvalue=2") != std::string::npos,
            "debug output should contain the enqueue-time tick sequence");
    std::error_code error;
    std::filesystem::remove_all(root, error);
}

void test_selector_target_type_policy() {
    require(dsnap::is_supported_selector_target(dsnap::kNormalGatherInteractType, false),
            "NormalGather must accept non-DropItem gatherables such as herbs and bird eggs");
    require(dsnap::is_supported_selector_target(dsnap::kNormalGatherInteractType, true),
            "NormalGather must remain accepted when an actor also derives from DropItemActor");
    require(dsnap::is_supported_selector_target(dsnap::kDropItemInteractType, true),
            "DropItemActor type must be accepted only with its class proof");
    require(!dsnap::is_supported_selector_target(dsnap::kDropItemInteractType, false),
            "type 7 without DropItemActor class proof must fail closed");
    require(dsnap::is_supported_selector_target(dsnap::kAnimalInteractType, false),
            "Animal type must be accepted for selector-returned fish such as trout and salmon");
    require(!dsnap::is_supported_selector_target(dsnap::kTreasureBoxInteractType, false),
            "TreasureBox must remain excluded");
    require(!dsnap::is_supported_selector_target(dsnap::kTreasureBoxInteractType, true),
            "TreasureBox must remain excluded even with unrelated class proof");

    for (std::uint16_t raw_type = 0; raw_type <= 255; ++raw_type) {
        const auto interact_type = static_cast<std::uint8_t>(raw_type);
        const bool expected_without_drop_class = interact_type == dsnap::kNormalGatherInteractType ||
            interact_type == dsnap::kAnimalInteractType;
        const bool expected_with_drop_class = expected_without_drop_class ||
            interact_type == dsnap::kDropItemInteractType;
        require(dsnap::is_supported_selector_target(interact_type, false) == expected_without_drop_class,
                "every non-DropItem actor interaction type must match the closed type-2-or-5 policy");
        require(dsnap::is_supported_selector_target(interact_type, true) == expected_with_drop_class,
                "every DropItem actor interaction type must match the closed type-2-or-5-or-7 policy");
    }
}

void test_contract_and_build_gates() {
    dsnap::Configuration config{};
    dsnap::PickupController no_contract{config, {false, 0}};
    no_contract.set_build_trusted(true);
    require(!no_contract.request_active(true), "unaccepted capture contract must fail closed");

    dsnap::PickupController unknown_build{config, {true, 1}};
    require(!unknown_build.request_active(true), "unknown build must fail closed");
}

void test_retry_backoff() {
    dsnap::CandidateQueue queue{4, 2, std::chrono::milliseconds{500}};
    queue.reset(1);
    const dsnap::WeakObjectId object{7, 9};
    require(queue.register_candidate(object, dsnap::kAllowedClassPath, 1), "candidate should register");
    require(queue.update_validation(object, valid_candidate()), "candidate should validate");
    const auto now = std::chrono::steady_clock::now();
    require(queue.next_eligible(now, 1, 4.5).has_value(), "candidate should initially be eligible");
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now);
    require(!queue.next_eligible(now + std::chrono::milliseconds{499}, 1, 4.5).has_value(), "first backoff must hold");
    require(queue.next_eligible(now + std::chrono::milliseconds{500}, 1, 4.5).has_value(), "first backoff should expire");
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now + std::chrono::milliseconds{500});
    queue.record_result(object, dsnap::ActionResult::TransientFailure, now + std::chrono::milliseconds{1500});
    require(queue.size() == 0, "candidate must be removed after retry limit");
}

void test_complete_gate_reason_attribution() {
    constexpr std::array expected{
        std::string_view{"owner_invalid"}, std::string_view{"class_invalid"},
        std::string_view{"interact_component_missing"}, std::string_view{"component_ownership_mismatch"},
        std::string_view{"state_field_missing"}, std::string_view{"state_value_mismatch"},
        std::string_view{"owner_location_unavailable"}, std::string_view{"non_finite_distance"},
        std::string_view{"outside_radius"}, std::string_view{"eligible"},
        std::string_view{"guarded_evaluation_exception"},
    };
    require(expected.size() == dsnap::kGateReasonCount, "every gate reason must have an aggregate slot");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto reason = static_cast<dsnap::GateReason>(index);
        require(dsnap::gate_reason_index(reason) == index, "gate reason indices must remain stable");
        require(dsnap::gate_reason_name(reason) == expected[index], "every gate reason must have a stable name");
    }
}

void test_bounded_change_only_gate_diagnostics() {
    dsnap::BoundedGateDiagnosticState state{};
    dsnap::GateObservation observation{.reason = dsnap::GateReason::InteractComponentMissing};
    require(state.should_log(observation), "first gate observation must log");
    require(!state.should_log(observation), "unchanged gate observation must not log again");
    for (std::size_t index = 1; index < dsnap::kMaxGateDiagnosticLogsPerCandidate; ++index) {
        observation.reason = index % 2 == 0 ? dsnap::GateReason::OutsideRadius : dsnap::GateReason::StateFieldMissing;
        observation.distance_meters = static_cast<double>(index);
        require(state.should_log(observation), "changed observation should log before the bound");
    }
    observation.reason = dsnap::GateReason::Eligible;
    require(!state.should_log(observation), "diagnostic logging must stop at the per-candidate bound");
    require(state.emitted() == dsnap::kMaxGateDiagnosticLogsPerCandidate, "diagnostic bound must be exact");
}

void test_complete_player_chain_reason_attribution() {
    constexpr std::array expected{
        std::string_view{"engine_or_output_invalid"}, std::string_view{"game_viewport_property_missing"},
        std::string_view{"game_viewport_value_null"}, std::string_view{"game_instance_property_missing"},
        std::string_view{"game_instance_value_null"}, std::string_view{"local_players_property_missing"},
        std::string_view{"local_players_invalid_index"}, std::string_view{"local_players_null_data"},
        std::string_view{"local_player_entry_null"}, std::string_view{"local_player_controller_property_missing"},
        std::string_view{"local_player_controller_value_null"}, std::string_view{"controller_class_mismatch"},
        std::string_view{"controller_player_property_missing"}, std::string_view{"controller_player_identity_mismatch"},
        std::string_view{"pawn_property_missing"}, std::string_view{"pawn_value_null"},
        std::string_view{"player_controller_property_missing"},
        std::string_view{"player_controller_identity_mismatch"}, std::string_view{"location_unavailable"},
        std::string_view{"success"}, std::string_view{"guarded_exception"},
    };
    require(expected.size() == dsnap::kPlayerChainReasonCount, "every player-chain exit must have an aggregate slot");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto reason = static_cast<dsnap::PlayerChainReason>(index);
        require(dsnap::player_chain_reason_index(reason) == index, "player-chain reason indices must remain stable");
        require(dsnap::player_chain_reason_name(reason) == expected[index], "every player-chain reason needs a stable name");
    }
}

void test_bounded_player_chain_diagnostics() {
    dsnap::BoundedPlayerChainDiagnosticState state{};
    require(state.should_log(dsnap::PlayerChainReason::PawnValueNull), "first player-chain reason must log");
    require(!state.should_log(dsnap::PlayerChainReason::PawnValueNull), "unchanged player-chain reason must not repeat");
    for (std::size_t index = 1; index < dsnap::kMaxPlayerChainDiagnosticLogs; ++index) {
        const auto reason = index % 2 == 0 ? dsnap::PlayerChainReason::PawnValueNull
                                          : dsnap::PlayerChainReason::LocationUnavailable;
        require(state.should_log(reason), "changed player-chain reason should log before the bound");
    }
    require(!state.should_log(dsnap::PlayerChainReason::Success), "player-chain logging must stop at its bound");
}

void test_controlled_alternate_pawn_acceptance() {
    const auto expected = dsnap::classify_controlled_pawn(true, true, true);
    require(expected == dsnap::PlayerMode::ExpectedCharacter, "expected character should retain its mode");
    require(expected && dsnap::supports_active_pickup(*expected),
            "the evidence-backed on-foot character may enter active pickup");
    const auto mounted = dsnap::classify_controlled_pawn(false, true, true);
    require(mounted == dsnap::PlayerMode::ControllerBoundAlternatePawn,
            "controller-bound alternate Pawn should be accepted for mounted play");
    require(mounted && dsnap::supports_active_pickup(*mounted),
            "a controller-bound alternate Pawn remains eligible for the accepted mounted route");
    require(!dsnap::classify_controlled_pawn(false, false, true).has_value(),
            "alternate Pawn without controller identity must fail closed");
    require(!dsnap::classify_controlled_pawn(false, true, false).has_value(),
            "alternate Pawn without a fresh location must fail closed");
}

void test_pending_action_confirmation() {
    using namespace std::chrono_literals;
    dsnap::PendingActionTracker tracker{};
    const dsnap::WeakObjectId candidate{42, 9};
    const auto now = std::chrono::steady_clock::time_point{1s};
    require(tracker.begin(candidate, now, 1.5), "first action may enter pending state");
    require(!tracker.begin({43, 10}, now, 2.0), "a second action must not start while one is pending");
    require(!tracker.confirm_delete({42, 10}).has_value(), "different serial must not confirm the pending action");
    require(tracker.pending(), "unrelated delete must preserve pending interest");
    require(tracker.confirm_delete(candidate).has_value(), "same candidate delete must confirm the action");
    require(!tracker.pending(), "confirmed action must leave pending state");
}

void test_pending_action_timeout_is_terminal() {
    using namespace std::chrono_literals;
    dsnap::PendingActionTracker tracker{};
    const dsnap::WeakObjectId candidate{7, 11};
    const auto now = std::chrono::steady_clock::time_point{2s};
    require(tracker.begin(candidate, now, 4.0), "action should enter pending state");
    require(!tracker.expire(now + dsnap::kActionConfirmationWindow - 1ms).has_value(), "pending action must retain its full window");
    const auto expired = tracker.expire(now + dsnap::kActionConfirmationWindow);
    require(expired.has_value() && expired->candidate == candidate, "timeout must return the exact pending candidate");
    require(!tracker.pending(),
            "the raw tracker must clear terminal evidence so AutomaticActionState can own retry and recovery backoff");
}

void test_qualified_f9_release() {
    using namespace std::chrono_literals;
    dsnap::QualifiedReleaseEdge input{250ms};
    const auto start = std::chrono::steady_clock::time_point{1s};
    require(input.sample(true, start), "the first physical F9 press should toggle");
    require(!input.sample(true, start + 2s), "a long-held F9 must never become another press");
    require(!input.sample(false, start + 2100ms), "release starts qualification but cannot rearm immediately");
    require(!input.sample(true, start + 2200ms), "a short false sample must not manufacture another press");
    require(!input.sample(false, start + 2300ms), "a new release interval must restart after bounce");
    require(!input.sample(false, start + 2550ms), "qualified release rearms without toggling");
    require(input.sample(true, start + 2600ms), "a new press after stable release should toggle");
}

void test_pending_action_exact_cancel() {
    using namespace std::chrono_literals;
    dsnap::PendingActionTracker tracker{};
    const dsnap::WeakObjectId candidate{17, 3};
    const auto now = std::chrono::steady_clock::time_point{3s};
    require(tracker.begin(candidate, now, 2.0), "action should enter pending state before rollback");
    require(!tracker.cancel({17, 4}), "rollback must not cancel a reused index with another serial");
    require(tracker.pending(), "mismatched rollback must preserve the pending action");
    require(tracker.cancel(candidate), "rollback must cancel the exact pending identity");
    require(!tracker.pending(), "exact rollback must leave no pending action");
}

void test_automatic_action_dispatch_releases_pending_and_isolates_cooldown() {
    using namespace std::chrono_literals;
    using Decision = dsnap::AutomaticActionDecision;

    dsnap::AutomaticActionState state{};
    const dsnap::WeakObjectId first{71, 5};
    const dsnap::WeakObjectId second{72, 6};
    const auto now = std::chrono::steady_clock::time_point{4s};

    require(state.begin({}, now, 1.0) == Decision::InvalidCandidate,
            "an invalid weak identity must never establish an automatic action");
    require(state.begin(first, now, -1.0) == Decision::InvalidCandidate,
            "invalid scalar evidence must never establish an automatic action");
    require(state.begin(first, now, std::numeric_limits<double>::infinity()) ==
                Decision::InvalidCandidate,
            "a non-finite scalar distance must never establish an automatic action");
    require(state.inspect(first, now, 1.0) == Decision::Ready,
            "a fresh exact component identity should pass read-only preflight");
    require(state.begin(first, now, 1.0) == Decision::Ready,
            "the first exact candidate may establish the one global pending action");
    require(state.pending(), "a ready automatic action must become pending before invocation");
    require(state.inspect(second, now + 1ms, 2.0) == Decision::Pending,
            "read-only preflight must expose the one-global-pending gate");
    require(state.begin(first, now + 1ms, 1.0) == Decision::Pending,
            "the same candidate must not be admitted again while pending");
    require(state.begin(second, now + 1ms, 2.0) == Decision::Pending,
            "a different candidate must not supersede the one global pending action");

    require(!state.observe_dispatch({first.object_index, first.serial_number + 1}, now + 2ms).has_value(),
            "a reused object index with another serial must not release the pending action");
    require(state.pending(), "an unrelated dispatch observation must preserve the pending action");

    const auto dispatch_at = now + 2ms;
    const auto dispatched = state.observe_dispatch(first, dispatch_at);
    require(dispatched.has_value() && dispatched->candidate == first,
            "only the exact pending weak identity may consume a game dispatch observation");
    require(!state.pending(), "a matching game dispatch must release the one global pending action");
    require(state.record_size() == 1,
            "the dispatched component must retain one bounded re-entry record");
    require(state.inspect(first,
                          dispatch_at + dsnap::kAutomaticActionDispatchReentryDelay - 1ms,
                          1.0) == Decision::Cooldown,
            "the dispatched component must retain its full 750ms re-entry delay");
    require(state.inspect(second, dispatch_at + 1ms, 2.0) == Decision::Ready,
            "one component's re-entry delay must not block another exact component");
    require(state.begin(second, dispatch_at + 1ms, 2.0) == Decision::Ready,
            "a different exact component may begin immediately after game dispatch releases pending");
    require(state.pending() && state.pending_candidate() == second,
            "the different component must become the sole pending action");
    require(state.cancel(second),
            "the different component may be rolled back without changing the first component's cooldown");

    const auto first_rearmed_at = dispatch_at + dsnap::kAutomaticActionDispatchReentryDelay;
    require(state.begin(first, first_rearmed_at, 1.0) == Decision::Ready,
            "the dispatched component must rearm automatically at the exact cooldown boundary");
    require(state.pending_attempt() == 1,
            "a completed dispatch cycle must rearm as a fresh first attempt");
    require(state.cancel(first), "the fresh post-dispatch attempt should support exact rollback");
}

void test_automatic_action_no_dispatch_retries_then_recovers_without_activation_reset() {
    using namespace std::chrono_literals;
    using Decision = dsnap::AutomaticActionDecision;

    dsnap::AutomaticActionState state{};
    const dsnap::WeakObjectId timed_out{81, 9};
    const dsnap::WeakObjectId other_candidate{82, 10};
    const auto now = std::chrono::steady_clock::time_point{5s};

    require(state.begin(timed_out, now, 3.0) == Decision::Ready,
            "a fresh exact candidate should enter pending before invocation");
    require(state.pending_attempt() == 1,
            "the first admission must be recorded as attempt one");
    require(!state.expire(now + dsnap::kActionConfirmationWindow - 1ms).has_value(),
            "the full confirmation window must remain pending");
    const auto expired = state.expire(now + dsnap::kActionConfirmationWindow);
    require(expired.has_value() && expired->candidate == timed_out,
            "timeout must release the exact pending action once");
    require(expired->attempt_ordinal == 1 && !state.pending(),
            "the first timeout must preserve its attempt number and clear global pending");
    require(state.record_size() == 1 && !state.fail_closed(),
            "one no-dispatch timeout must retain one bounded retry record without failing closed");
    require(state.begin(other_candidate, now + dsnap::kActionConfirmationWindow, 2.0) ==
                Decision::Ready,
            "one component's retry delay must not block a different exact component");
    require(state.cancel(other_candidate),
            "the different component should roll back without changing the timed-out record");
    require(state.inspect(timed_out,
                          now + dsnap::kActionConfirmationWindow +
                              dsnap::kAutomaticActionRetryDelay - 1ms,
                          3.0) == Decision::Cooldown,
            "read-only preflight must avoid expensive action resolution during retry cooldown");
    require(state.begin(timed_out,
                        now + dsnap::kActionConfirmationWindow +
                            dsnap::kAutomaticActionRetryDelay - 1ms,
                        3.0) == Decision::Cooldown,
            "the exact candidate must not be retried before the bounded cooldown ends");
    const auto retry_at = now + dsnap::kActionConfirmationWindow +
        dsnap::kAutomaticActionRetryDelay;
    require(state.begin(timed_out, retry_at, 3.0) == Decision::Ready &&
                state.pending_attempt() == 2,
            "the game may re-present the same exact candidate for one tracked retry");
    const auto retry_expired = state.expire(retry_at + dsnap::kActionConfirmationWindow);
    require(retry_expired.has_value() && retry_expired->attempt_ordinal == 2,
            "the retry timeout must remain attributable to attempt two");
    require(!state.pending() && state.record_size() == 1 && !state.fail_closed(),
            "a second no-dispatch timeout must enter bounded recovery rather than activation quarantine");

    const auto recovery_at = retry_at + dsnap::kActionConfirmationWindow +
        dsnap::kAutomaticActionFailureBackoff;
    require(state.inspect(timed_out, recovery_at - 1ms, 3.0) == Decision::Cooldown,
            "the twice-timed-out component must retain its full 1500ms recovery backoff");

    const dsnap::WeakObjectId reused_index{timed_out.object_index, timed_out.serial_number + 1};
    require(state.begin(reused_index, recovery_at - 1ms, 3.0) == Decision::Ready,
            "a recovery backoff must remain scoped to the exact object index and serial");
    require(state.cancel(reused_index),
            "the reused-index identity should roll back independently of the recovering identity");

    require(state.begin(timed_out, recovery_at, 3.0) == Decision::Ready,
            "the exact component must recover automatically without F9 or activation reset");
    require(state.pending_attempt() == 1,
            "a completed failure-backoff cycle must restart as a fresh first attempt");
    require(state.cancel(timed_out), "the automatically recovered attempt should support exact rollback");
}

void test_automatic_action_expired_records_are_recycled() {
    using namespace std::chrono_literals;
    using Decision = dsnap::AutomaticActionDecision;

    dsnap::AutomaticActionState state{};
    const auto now = std::chrono::steady_clock::time_point{6s};
    for (std::size_t index = 0; index < dsnap::kAutomaticActionRecordCapacity; ++index) {
        const dsnap::WeakObjectId candidate{
            static_cast<std::int32_t>(1000 + index),
            static_cast<std::int32_t>(2000 + index),
        };
        require(state.begin(candidate, now, 1.0) == Decision::Ready,
                "every identity within the fixed active-record capacity may begin once");
        require(state.observe_dispatch(candidate, now).has_value(),
                "every admitted identity should release pending through game dispatch evidence");
    }
    require(state.record_size() == dsnap::kAutomaticActionRecordCapacity && !state.fail_closed(),
            "the exact active-record capacity must remain usable without failing closed");

    const auto records_expire_at = now + dsnap::kAutomaticActionDispatchReentryDelay;
    const dsnap::WeakObjectId replacement{9000, 9001};
    require(state.begin(replacement, records_expire_at, 1.0) == Decision::Ready,
            "a fresh candidate must prune genuinely expired records before admission");
    require(state.record_size() == 0 && !state.fail_closed(),
            "expired records must be recycled instead of causing activation-wide fail-closed state");
    require(state.observe_dispatch(replacement, records_expire_at).has_value(),
            "the replacement action should still accept matching game dispatch evidence");
    require(state.record_size() == 1 && !state.fail_closed(),
            "recycled storage must retain only the replacement's active cooldown record");
}

void test_automatic_action_unused_retry_opportunity_expires() {
    using namespace std::chrono_literals;
    using Decision = dsnap::AutomaticActionDecision;

    dsnap::AutomaticActionState state{};
    const auto now = std::chrono::steady_clock::time_point{6500ms};
    const dsnap::WeakObjectId abandoned{9100, 9101};
    require(state.begin(abandoned, now, 1.0) == Decision::Ready,
            "a fresh candidate should begin before its first no-dispatch timeout");
    const auto timed_out_at = now + dsnap::kActionConfirmationWindow;
    require(state.expire(timed_out_at).has_value() && state.record_size() == 1,
            "the first timeout should retain one bounded retry opportunity");

    const auto retry_opportunity_expires = timed_out_at +
        dsnap::kAutomaticActionRetryDelay +
        dsnap::kAutomaticActionRetryOpportunityWindow;
    const dsnap::WeakObjectId replacement{9102, 9103};
    require(state.begin(replacement, retry_opportunity_expires, 1.0) == Decision::Ready,
            "an unrelated admission should prune an unused expired retry opportunity");
    require(state.record_size() == 0 && !state.fail_closed(),
            "an unused first-timeout record must not accumulate for the activation lifetime");
    require(state.cancel(replacement), "the replacement admission should support exact rollback");
}

void test_automatic_action_active_record_exhaustion_fails_closed() {
    using namespace std::chrono_literals;
    using Decision = dsnap::AutomaticActionDecision;

    dsnap::AutomaticActionState state{};
    const auto now = std::chrono::steady_clock::time_point{7s};
    for (std::size_t index = 0; index < dsnap::kAutomaticActionRecordCapacity; ++index) {
        const dsnap::WeakObjectId candidate{
            static_cast<std::int32_t>(3000 + index),
            static_cast<std::int32_t>(4000 + index),
        };
        require(state.begin(candidate, now, 1.0) == Decision::Ready,
                "every identity within active capacity should establish one pending action");
        require(state.observe_dispatch(candidate, now).has_value(),
                "every active-capacity action should release pending through dispatch");
    }
    require(state.record_size() == dsnap::kAutomaticActionRecordCapacity && !state.fail_closed(),
            "filling but not exceeding active capacity must remain valid");

    const dsnap::WeakObjectId overflow{9500, 9501};
    require(state.begin(overflow, now, 1.0) == Decision::Ready,
            "an overflow identity may establish pending before its outcome needs a record");
    require(state.observe_dispatch(overflow, now).has_value(),
            "the overflow identity must still release the exact pending action");
    require(state.fail_closed(),
            "exhausting storage with unexpired active records must fail closed rather than evict evidence");
    require(state.begin({9502, 9503}, now + 1ms, 1.0) == Decision::FailClosed,
            "active-record exhaustion must reject every later automatic action in the activation");

    state.reset_activation();
    require(!state.fail_closed() && state.record_size() == 0,
            "an explicit activation reset must recover from true active-record exhaustion");
}

void test_transient_target_ownership() {
    const auto empty = dsnap::transient_target_ownership(true, true);
    require(empty.target_object && empty.target_component,
            "the Mod may own both assignments only when both game fields were empty");
    const auto game_owned_object = dsnap::transient_target_ownership(false, true);
    require(!game_owned_object.target_object && game_owned_object.target_component,
            "an existing exact game target must remain game-owned per field");
    require(dsnap::should_clear_transient_target(true, true),
            "a Mod-owned field that still has the assigned value must be cleared");
    require(!dsnap::should_clear_transient_target(false, true),
            "an existing game-owned exact field must never be cleared");
    require(!dsnap::should_clear_transient_target(true, false),
            "a field changed by the game during ProcessEvent must never be overwritten");
}

void test_exact_one_selection_includes_previous_attempts() {
    using Decision = dsnap::ExactOneSelectionDecision;
    require(dsnap::decide_exact_one_selection(0, false) == Decision::NoEligibleCandidate,
            "zero eligible candidates must remain idle");
    require(dsnap::decide_exact_one_selection(1, false) == Decision::Ready,
            "one fresh eligible candidate may be selected");
    require(dsnap::decide_exact_one_selection(2, false) == Decision::Ambiguous,
            "every eligible candidate must count toward ambiguity");
    require(dsnap::decide_exact_one_selection(1, true) == Decision::PreviouslyAttempted,
            "a still-present attempted candidate must count but must never be invoked again");
}

void test_distinct_interaction_capture_logging() {
    dsnap::DistinctInteractionCaptureLog log{};
    require(log.first(dsnap::InteractionSource::Manual, 13), "first manual source/key tuple must log");
    require(!log.first(dsnap::InteractionSource::Manual, 13), "duplicate manual tuple must be suppressed");
    require(log.first(dsnap::InteractionSource::Automatic, 13), "automatic source is a distinct tuple");
    require(log.first(dsnap::InteractionSource::Manual, 14), "different key action is a distinct tuple");
}

dsnap::RuntimeContract valid_runtime_contract() {
    dsnap::RuntimeContract contract{};
    contract.game_sha256 = std::string(64, 'A');
    contract.ue4ss_sha256 = std::string(64, 'B');
    contract.ue4ss_git_sha = "1c1a1497f942c707f47ba668db75b25e86f6c08a";
    contract.function_path = "/Game/Blueprints/BP_DsPlayerController.BP_DsPlayerController_C:OnPressInteractionButton";
    contract.function = dsnap::ReplayFunction::ControllerPressInteractionButton;
    contract.receiver = dsnap::ReceiverRole::CurrentController;
    contract.parameter_count = 2;
    contract.parameters[0] = {dsnap::stable_name_hash("InteractActor"), dsnap::ParameterKind::CandidateObject, 0};
    contract.parameters[1] = {dsnap::stable_name_hash("CurrentPC"), dsnap::ParameterKind::CurrentController, 0};
    return contract;
}

void test_runtime_contract_validation_and_persistence() {
    const auto contract = valid_runtime_contract();
    require(contract.structurally_valid(), "canonical runtime contract should be valid");
    const auto serialized = dsnap::serialize_contract(contract);
    const auto parsed = dsnap::parse_contract(serialized);
    require(parsed.has_value() && *parsed == contract, "runtime contract must round-trip canonically");
    require(dsnap::fingerprint_matches(*parsed, contract.game_sha256, contract.ue4ss_sha256, contract.ue4ss_git_sha),
            "exact fingerprints should validate");
    require(!dsnap::parse_contract(serialized + "schema=12\n").has_value(), "duplicate keys must fail closed");
    require(!dsnap::parse_contract("schema=12\nunknown=x\n").has_value(), "unknown keys must fail closed");
    auto malformed = contract;
    malformed.parameters[0].name_hash = 0;
    require(!malformed.structurally_valid(), "zero parameter hashes must fail closed");
    malformed = contract;
    malformed.parameters[0].kind = static_cast<dsnap::ParameterKind>(99);
    require(!malformed.structurally_valid(), "unknown parameter kinds must fail closed");
    malformed = contract;
    malformed.function_path = "relative:OnPressInteractionButton";
    require(!malformed.structurally_valid(), "relative function paths must fail closed");
    malformed = contract;
    malformed.function_path = "/Game/Invalid\nFunction:OnPressInteractionButton";
    require(!malformed.structurally_valid(), "control characters in function paths must fail closed");
    require(!dsnap::parse_contract(serialized.substr(0, serialized.find("git=") + 4) + std::string(40, 'Z') +
                                   "\nfunction=6\npath=/Script/DS.DsPlayerController:OnPressInteractionButton\nreceiver=2\nreceiver_property=0\ncount=0\n").has_value(),
            "non-hex git fingerprints must fail closed");

    auto property_receiver = contract;
    property_receiver.function = dsnap::ReplayFunction::ServerRunInteractV2;
    property_receiver.function_path = "/Script/DS.DInteractableComponent:Server_RunInteractV2";
    property_receiver.receiver = dsnap::ReceiverRole::CurrentControllerProperty;
    property_receiver.receiver_property_hash = dsnap::stable_name_hash("PickupInteractComponent");
    property_receiver.parameter_count = 0;
    property_receiver.parameters = {};
    require(property_receiver.structurally_valid(), "exact controller property receiver should be valid");
    const auto property_serialized = dsnap::serialize_contract(property_receiver);
    const auto property_parsed = dsnap::parse_contract(property_serialized);
    require(property_parsed.has_value() && *property_parsed == property_receiver,
            "property-bound receiver contract must round-trip canonically");
    property_receiver.receiver_property_hash = 0;
    require(!property_receiver.structurally_valid(), "property receiver must require a nonzero property hash");
    property_receiver = contract;
    property_receiver.receiver_property_hash = dsnap::stable_name_hash("UnexpectedProperty");
    require(!property_receiver.structurally_valid(), "direct receiver must reject a stray property hash");

    const auto root = std::filesystem::temp_directory_path() / "dsnap-contract-test";
    const auto path = root / "contract";
    require(dsnap::atomic_replace_text(path, serialized), "atomic contract persistence should succeed");
    std::ifstream input{path, std::ios::binary};
    const std::string persisted{std::istreambuf_iterator<char>{input}, {}};
    require(persisted == serialized, "atomic persistence must preserve exact canonical bytes");
    std::error_code error; std::filesystem::remove_all(root, error);
}

void test_calibration_state_and_true_edge() {
    dsnap::CalibrationStateMachine state{};
    require(state.press_f9(true, false) == dsnap::RuntimeState::Calibrating, "missing contract should enter calibration");
    state.calibration_validated();
    require(state.state() == dsnap::RuntimeState::ArmedReady, "positive delete validation should arm ready");
    state.invalidate_contract();
    require(state.press_f9(true, false) == dsnap::RuntimeState::Off, "invalid state must remain disarmable");
    require(state.press_f9(true, false) == dsnap::RuntimeState::Calibrating, "later F9 should permit fresh calibration");
    dsnap::AtomicPhysicalKeyEdge edge{};
    require(edge.key_down(), "first physical down is a rising edge");
    require(edge.latched(), "runtime-used atomic edge must remain latched while held");
    require(!edge.key_down(), "OS repeat while held must not toggle");
    require(edge.key_up(), "physical release must clear an established atomic latch");
    require(!edge.latched(), "released atomic edge must report unlatched");
    require(!edge.key_up(), "duplicate release must not report a second rearm");
    require(edge.key_down(), "release enables the next physical press");
}

void test_discovery_epoch_and_cancellation() {
    dsnap::IncrementalDiscoveryState discovery{};
    discovery.begin(7, 100);
    require(discovery.next(30) == std::pair<std::int32_t, std::int32_t>{0, 30}, "first discovery batch should be bounded");
    discovery.commit(20);
    require(discovery.next(30) == std::pair<std::int32_t, std::int32_t>{20, 50}, "time-budget stop must resume without gaps");
    require(discovery.epoch() == 7, "discovery must retain its world epoch");
    require(!discovery.complete(), "a partial baseline must never be action-ready");
    discovery.commit(100);
    require(discovery.complete() && !discovery.active(), "only the full fixed snapshot may become action-ready");
    require(discovery.extend_upper_bound(140), "a larger UObject upper bound should start a tail-only extension");
    require(!discovery.complete() && discovery.active(), "new tail indices must block action readiness until scanned");
    require(discovery.next(30) == std::pair<std::int32_t, std::int32_t>{100, 130},
            "tail discovery must resume at the previous upper bound without rescanning old indices");
    discovery.commit(140);
    require(discovery.complete() && !discovery.active(), "the complete tail extension should restore readiness");
    require(!discovery.extend_upper_bound(140), "an unchanged upper bound must not restart discovery");
    discovery.cancel();
    require(!discovery.active() && !discovery.complete(), "disarm must cancel and invalidate discovery");
    discovery.begin(8, 0);
    require(discovery.complete() && !discovery.active(), "an explicitly empty fixed snapshot is complete");
}

void test_single_target_invocation_latch() {
    dsnap::SingleTargetInvocationLatch latch{};
    const dsnap::WeakObjectId first{101, 7};
    const dsnap::WeakObjectId second{102, 8};

    require(latch.observe(std::nullopt).decision == dsnap::TargetObservationDecision::NoTarget,
            "an empty latch with no target must remain idle");
    require(latch.observe(first).decision == dsnap::TargetObservationDecision::Ready,
            "a fresh valid target must be ready");
    require(latch.mark_invoked(first), "the first target may be marked exactly once");
    require(!latch.mark_invoked(first), "the same pending target must not be marked twice");
    require(latch.observe(first).decision == dsnap::TargetObservationDecision::Debounce,
            "an unchanged target must debounce every later pulse");

    const auto changed = latch.observe(second);
    require(changed.decision == dsnap::TargetObservationDecision::PreviousChanged &&
                changed.previous == first && changed.current == second,
            "a different target must release the previous latch and report both identities");
    require(latch.mark_invoked(second), "the changed target may be invoked once after validation");
    const auto cleared = latch.observe(std::nullopt);
    require(cleared.decision == dsnap::TargetObservationDecision::PreviousCleared &&
                cleared.previous == second,
            "a cleared game target must confirm the pending target transition");
    require(!latch.pending(), "target clearing must leave no pending invocation");
}

void test_single_target_exact_rollback() {
    dsnap::SingleTargetInvocationLatch latch{};
    const dsnap::WeakObjectId candidate{201, 11};
    require(latch.mark_invoked(candidate), "test target should enter the invocation latch");
    require(!latch.clear({201, 12}), "rollback must not clear a reused index with another serial");
    require(latch.pending(), "mismatched rollback must preserve the invocation latch");
    require(latch.clear(candidate), "rollback must clear the exact invocation identity");
    require(!latch.pending(), "exact rollback must leave the latch empty");
}

void test_status_toast_timeline() {
    using namespace std::chrono_literals;
    using Toast = dsnap::StatusToastKind;
    dsnap::StatusToastTimeline timeline{};
    const auto start = dsnap::StatusToastTimeline::Clock::time_point{10s};

    timeline.notify(Toast::Starting, start);
    timeline.notify(Toast::Enabled, start);
    const auto starting = timeline.frame(start + 160ms);
    require(starting.visible && starting.kind == Toast::Starting,
            "successful enable must preserve a readable starting phase");
    require(starting.opacity > 0.99 && starting.vertical_offset > -0.01,
            "starting card must finish its fade and slide before the result");
    const auto eased_midpoint = timeline.frame(start + 60ms);
    require(eased_midpoint.opacity > 0.49 && eased_midpoint.opacity < 0.51
                && eased_midpoint.vertical_offset > -4.1
                && eased_midpoint.vertical_offset < -3.9,
            "status card must use a centered smoothstep reveal");

    const auto enabled_begin = timeline.frame(
        start + dsnap::StatusToastTimeline::kStartingMinimum);
    require(enabled_begin.visible && enabled_begin.kind == Toast::Enabled,
            "queued enable result must follow the starting phase");
    require(enabled_begin.opacity == 0.0,
            "result card must begin with an independent fade-in");
    const auto enabled_visible = timeline.frame(
        start + dsnap::StatusToastTimeline::kStartingMinimum + 180ms);
    require(enabled_visible.opacity > 0.99,
            "enabled result must become fully visible");
    const auto enabled_expired = timeline.frame(
        start + dsnap::StatusToastTimeline::kStartingMinimum
            + dsnap::StatusToastTimeline::kResultLifetime);
    require(!enabled_expired.visible,
            "enabled result must expire without persistent screen work");

    const auto disabled_at = start + 3s;
    timeline.notify(Toast::Disabled, disabled_at);
    const auto disabled = timeline.frame(disabled_at + 120ms);
    require(disabled.visible && disabled.kind == Toast::Disabled
                && disabled.opacity > 0.99,
            "disable must replace idle state with a visible result");

    const auto retry_at = start + 4s;
    timeline.notify(Toast::Starting, retry_at);
    timeline.notify(Toast::Unavailable, retry_at);
    require(timeline.frame(retry_at + 200ms).kind == Toast::Starting,
            "a rejected enable must still expose the starting phase");
    require(timeline.frame(
                retry_at + dsnap::StatusToastTimeline::kStartingMinimum)
                .kind == Toast::Unavailable,
            "a rejected enable must resolve to not-ready after starting");

    timeline.clear();
    require(!timeline.frame(retry_at + 1s).visible,
            "world travel cleanup must clear every pending toast");
}

} // namespace

int main() {
    require(dsnap::tests::run_pe_runtime_tests(), "PE runtime and chained-function fixtures should pass");
    test_direct_rel32_call_collector();
    test_unique_terminal_rel32_call_selector();
    test_dynamic_selector_resolver();
    test_ui_selector_consensus();
    test_configuration();
    test_game_interaction_binding();
    test_async_logger_batch_flush();
    test_session_calibration();
    test_callback_generation_gate();
    test_session_toggle_event_gate();
    test_experimental_nested_fingerprint_policy();
    test_whitelist_and_bounds();
    test_epoch_invalidation();
    test_fail_closed_validation();
    test_selector_target_type_policy();
    test_contract_and_build_gates();
    test_retry_backoff();
    test_complete_gate_reason_attribution();
    test_bounded_change_only_gate_diagnostics();
    test_complete_player_chain_reason_attribution();
    test_bounded_player_chain_diagnostics();
    test_qualified_f9_release();
    test_controlled_alternate_pawn_acceptance();
    test_pending_action_confirmation();
    test_pending_action_timeout_is_terminal();
    test_pending_action_exact_cancel();
    test_automatic_action_dispatch_releases_pending_and_isolates_cooldown();
    test_automatic_action_no_dispatch_retries_then_recovers_without_activation_reset();
    test_automatic_action_expired_records_are_recycled();
    test_automatic_action_unused_retry_opportunity_expires();
    test_automatic_action_active_record_exhaustion_fails_closed();
    test_transient_target_ownership();
    test_exact_one_selection_includes_previous_attempts();
    test_distinct_interaction_capture_logging();
    test_runtime_contract_validation_and_persistence();
    test_calibration_state_and_true_edge();
    test_discovery_epoch_and_cancellation();
    test_single_target_invocation_latch();
    test_single_target_exact_rollback();
    test_status_toast_timeline();
    std::cout << "All DragonSwordNativeAutoPickup core tests passed.\n";
    return 0;
}
