#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace dsnap {

inline constexpr std::array<std::string_view, 2> kExpectedGameSha256{
    "0C9D54A35D7160E671A3DB10C5E645140FDAD15A7293314681E9EDA7E360C1FE",
    "3DDDCEE474825310000A4CD24239AE5C8B76EF81BAC223C9F3D52565816A0CEA",
};
inline constexpr const char* kExpectedUe4ssSha256 = "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1";
inline constexpr const char* kExpectedUe4ssGitSha = "1c1a1497f942c707f47ba668db75b25e86f6c08a";

[[nodiscard]] inline constexpr bool expected_game_sha256(std::string_view value) noexcept {
    for (const auto expected : kExpectedGameSha256) {
        if (value == expected) return true;
    }
    return false;
}

struct FingerprintResult {
    std::string game_sha256{};
    std::string ue4ss_sha256{};
    bool trusted{};
    std::string error{};
};

[[nodiscard]] std::optional<std::string> sha256_file(const std::filesystem::path& path);
[[nodiscard]] FingerprintResult verify_build_fingerprint(const std::filesystem::path& binary_directory);
[[nodiscard]] bool atomic_replace_text(const std::filesystem::path& path, std::string_view text);

} // namespace dsnap
