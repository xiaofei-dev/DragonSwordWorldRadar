#pragma once

#include <dsnap/types.hpp>

#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace dsnap {

inline constexpr std::uint32_t kRuntimeContractSchema = 12;
inline constexpr std::size_t kMaxContractParameters = 8;
inline constexpr std::size_t kMaxRuntimeFunctionPathBytes = 512;

enum class RuntimeState : std::uint8_t { Off, Calibrating, ArmedReady, DisabledContractInvalid };
enum class ReplayFunction : std::uint8_t {
    CallActivePlayer = 1,
    ServerRunInteractV2 = 2,
    ServerInputInteractKeyAfterAction = 3,
    ServerInputInteractKeyAction = 4,
    ControllerInteract = 5,
    ControllerPressInteractionButton = 6,
};
enum class ReceiverRole : std::uint8_t {
    CandidateComponent = 1,
    CurrentController = 2,
    CurrentPawn = 3,
    CurrentControllerProperty = 4,
    CurrentPawnProperty = 5,
};
enum class ParameterKind : std::uint8_t { CandidateObject = 1, CurrentPawn = 2, CurrentController = 3, Bool = 4, Byte = 5, Int32 = 6 };

struct ContractParameter {
    std::uint32_t name_hash{};
    ParameterKind kind{};
    std::int32_t scalar{};
    auto operator<=>(const ContractParameter&) const = default;
};

struct RuntimeContract {
    std::uint32_t schema{kRuntimeContractSchema};
    std::string game_sha256{};
    std::string ue4ss_sha256{};
    std::string ue4ss_git_sha{};
    std::string function_path{};
    ReplayFunction function{};
    ReceiverRole receiver{};
    std::uint32_t receiver_property_hash{};
    std::uint8_t parameter_count{};
    std::array<ContractParameter, kMaxContractParameters> parameters{};

    [[nodiscard]] bool structurally_valid() const noexcept {
        if (!(schema == kRuntimeContractSchema && function_path.size() >= 3 &&
               function_path.size() <= kMaxRuntimeFunctionPathBytes && function_path.front() == '/' &&
               function_path.find(':') != std::string::npos &&
               function >= ReplayFunction::CallActivePlayer &&
               function <= ReplayFunction::ControllerPressInteractionButton &&
               receiver >= ReceiverRole::CandidateComponent && receiver <= ReceiverRole::CurrentPawnProperty &&
               parameter_count <= parameters.size())) return false;
        const bool property_receiver = receiver == ReceiverRole::CurrentControllerProperty ||
                                       receiver == ReceiverRole::CurrentPawnProperty;
        if (property_receiver != (receiver_property_hash != 0)) return false;
        for (const auto ch : function_path) {
            const auto byte = static_cast<unsigned char>(ch);
            if (byte < 0x20U || byte > 0x7eU) return false;
        }
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            const auto& parameter = parameters[index];
            if (index < parameter_count) {
                if (parameter.name_hash == 0 || parameter.kind < ParameterKind::CandidateObject ||
                    parameter.kind > ParameterKind::Int32 ||
                    (parameter.kind == ParameterKind::Bool && parameter.scalar != 0 && parameter.scalar != 1)) return false;
            } else if (parameter.name_hash != 0 || static_cast<std::uint8_t>(parameter.kind) != 0 || parameter.scalar != 0) return false;
        }
        return true;
    }
    auto operator<=>(const RuntimeContract&) const = default;
};

[[nodiscard]] constexpr std::uint32_t stable_name_hash(std::string_view value) noexcept {
    std::uint32_t hash = 2166136261U;
    for (const auto ch : value) {
        hash ^= static_cast<std::uint8_t>(ch);
        hash *= 16777619U;
    }
    return hash;
}

[[nodiscard]] inline bool fingerprint_matches(const RuntimeContract& contract, std::string_view game,
                                               std::string_view ue4ss, std::string_view git) noexcept {
    return contract.structurally_valid() && contract.game_sha256 == game &&
           contract.ue4ss_sha256 == ue4ss && contract.ue4ss_git_sha == git;
}

[[nodiscard]] inline bool is_sha256(std::string_view value) noexcept {
    if (value.size() != 64) return false;
    for (const auto ch : value) if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'))) return false;
    return true;
}

[[nodiscard]] inline bool is_hex40(std::string_view value) noexcept {
    if (value.size() != 40) return false;
    for (const auto ch : value) if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return false;
    return true;
}

[[nodiscard]] inline std::string serialize_contract(const RuntimeContract& contract) {
    std::string output = "schema=" + std::to_string(contract.schema) + "\n";
    output += "game=" + contract.game_sha256 + "\nue4ss=" + contract.ue4ss_sha256;
    output += "\ngit=" + contract.ue4ss_git_sha + "\nfunction=" +
              std::to_string(static_cast<unsigned>(contract.function));
    output += "\npath=" + contract.function_path;
    output += "\nreceiver=" + std::to_string(static_cast<unsigned>(contract.receiver));
    output += "\nreceiver_property=" + std::to_string(contract.receiver_property_hash);
    output += "\ncount=" + std::to_string(contract.parameter_count) + "\n";
    for (std::size_t index = 0; index < contract.parameter_count; ++index) {
        const auto& parameter = contract.parameters[index];
        output += "p=" + std::to_string(parameter.name_hash) + "," +
                  std::to_string(static_cast<unsigned>(parameter.kind)) + "," +
                  std::to_string(parameter.scalar) + "\n";
    }
    return output;
}

[[nodiscard]] inline std::optional<RuntimeContract> parse_contract(std::string_view text) {
    RuntimeContract result{};
    std::size_t parameter_index{};
    bool have_schema{}, have_game{}, have_ue4ss{}, have_git{}, have_function{}, have_path{}, have_receiver{},
         have_receiver_property{}, have_count{};
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) return std::nullopt;
        const auto key = line.substr(0, equals);
        const auto value = line.substr(equals + 1);
        auto parse_u32 = [](std::string_view input, std::uint32_t& output) {
            const auto parsed = std::from_chars(input.data(), input.data() + input.size(), output);
            return parsed.ec == std::errc{} && parsed.ptr == input.data() + input.size();
        };
        if (key == "schema") { if (have_schema) return std::nullopt; have_schema = parse_u32(value, result.schema); }
        else if (key == "game") { if (have_game) return std::nullopt; result.game_sha256 = value; have_game = is_sha256(value); }
        else if (key == "ue4ss") { if (have_ue4ss) return std::nullopt; result.ue4ss_sha256 = value; have_ue4ss = is_sha256(value); }
        else if (key == "git") { if (have_git) return std::nullopt; result.ue4ss_git_sha = value; have_git = is_hex40(value); }
        else if (key == "function") {
            if (have_function) return std::nullopt;
            std::uint32_t parsed{}; have_function = parse_u32(value, parsed) && parsed >= 1 && parsed <= 6;
            result.function = static_cast<ReplayFunction>(parsed);
        } else if (key == "receiver") {
            if (have_receiver) return std::nullopt;
            std::uint32_t parsed{}; have_receiver = parse_u32(value, parsed) && parsed >= 1 && parsed <= 5;
            result.receiver = static_cast<ReceiverRole>(parsed);
        } else if (key == "receiver_property") {
            if (have_receiver_property) return std::nullopt;
            have_receiver_property = parse_u32(value, result.receiver_property_hash);
        } else if (key == "path") {
            if (have_path) return std::nullopt;
            result.function_path = value;
            have_path = !value.empty() && value.size() <= kMaxRuntimeFunctionPathBytes;
        } else if (key == "count") {
            if (have_count) return std::nullopt;
            std::uint32_t parsed{}; have_count = parse_u32(value, parsed) && parsed <= kMaxContractParameters;
            result.parameter_count = static_cast<std::uint8_t>(parsed);
        } else if (key == "p") {
            if (parameter_index >= kMaxContractParameters) return std::nullopt;
            const auto first = value.find(','); const auto second = value.find(',', first + 1);
            if (first == std::string_view::npos || second == std::string_view::npos) return std::nullopt;
            std::uint32_t hash{}, kind{}; std::int32_t scalar{};
            const auto scalar_text = value.substr(second + 1);
            const auto scalar_result = std::from_chars(scalar_text.data(), scalar_text.data() + scalar_text.size(), scalar);
            if (!parse_u32(value.substr(0, first), hash) || !parse_u32(value.substr(first + 1, second - first - 1), kind) ||
                scalar_result.ec != std::errc{} || scalar_result.ptr != scalar_text.data() + scalar_text.size() ||
                kind < 1 || kind > 6) return std::nullopt;
            result.parameters[parameter_index++] = {hash, static_cast<ParameterKind>(kind), scalar};
        } else return std::nullopt;
    }
    if (!have_schema || !have_game || !have_ue4ss || !have_git || !have_function || !have_path || !have_receiver ||
        !have_receiver_property || !have_count ||
        parameter_index != result.parameter_count || !result.structurally_valid()) return std::nullopt;
    return result;
}

class CalibrationStateMachine {
public:
    [[nodiscard]] RuntimeState press_f9(bool trusted, bool contract_valid) noexcept {
        if (state_ == RuntimeState::DisabledContractInvalid) { state_ = RuntimeState::Off; return state_; }
        if (state_ != RuntimeState::Off) { state_ = RuntimeState::Off; return state_; }
        state_ = !trusted ? RuntimeState::DisabledContractInvalid
                          : (contract_valid ? RuntimeState::ArmedReady : RuntimeState::Calibrating);
        return state_;
    }
    void calibration_validated() noexcept { if (state_ == RuntimeState::Calibrating) state_ = RuntimeState::ArmedReady; }
    void invalidate_contract() noexcept { state_ = RuntimeState::DisabledContractInvalid; }
    void reset_off() noexcept { state_ = RuntimeState::Off; }
    [[nodiscard]] RuntimeState state() const noexcept { return state_; }
private:
    RuntimeState state_{RuntimeState::Off};
};

class RisingEdgeLatch {
public:
    [[nodiscard]] bool key_down() noexcept { if (down_) return false; down_ = true; return true; }
    void key_up() noexcept { down_ = false; }
    [[nodiscard]] bool down() const noexcept { return down_; }
private:
    bool down_{};
};

} // namespace dsnap
