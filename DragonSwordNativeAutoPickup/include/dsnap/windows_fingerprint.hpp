#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace dsnap {

inline constexpr const char* kExpectedGameSha256 = "0C9D54A35D7160E671A3DB10C5E645140FDAD15A7293314681E9EDA7E360C1FE";
inline constexpr const char* kExpectedUe4ssSha256 = "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1";
inline constexpr const char* kExpectedUe4ssGitSha = "1c1a1497f942c707f47ba668db75b25e86f6c08a";

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
