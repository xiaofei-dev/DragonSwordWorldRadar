#include <dsnap/windows_fingerprint.hpp>

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace dsnap {

std::optional<std::string> sha256_file(const std::filesystem::path& path) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD object_size{};
    DWORD result_size{};
    DWORD hash_size{};

    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
        return std::nullopt;
    }
    const auto close_algorithm = [&] { BCryptCloseAlgorithmProvider(algorithm, 0); };
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size),
                          sizeof(object_size), &result_size, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hash_size),
                          sizeof(hash_size), &result_size, 0) < 0) {
        close_algorithm();
        return std::nullopt;
    }

    std::vector<UCHAR> object_buffer(object_size);
    std::vector<UCHAR> digest(hash_size);
    if (BCryptCreateHash(algorithm, &hash, object_buffer.data(), object_size, nullptr, 0, 0) < 0) {
        close_algorithm();
        return std::nullopt;
    }

    std::ifstream input{path, std::ios::binary};
    if (!input) {
        BCryptDestroyHash(hash);
        close_algorithm();
        return std::nullopt;
    }
    std::array<char, 1024 * 1024> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto read = input.gcount();
        if (read > 0 && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()),
                                      static_cast<ULONG>(read), 0) < 0) {
            BCryptDestroyHash(hash);
            close_algorithm();
            return std::nullopt;
        }
    }
    if (BCryptFinishHash(hash, digest.data(), hash_size, 0) < 0) {
        BCryptDestroyHash(hash);
        close_algorithm();
        return std::nullopt;
    }
    BCryptDestroyHash(hash);
    close_algorithm();

    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        output << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return output.str();
}

namespace {

[[nodiscard]] std::optional<std::filesystem::path> loaded_ue4ss_path() {
    const auto module = GetModuleHandleW(L"UE4SS.dll");
    if (!module) return std::nullopt;
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return std::nullopt;
    buffer.resize(length);
    return std::filesystem::path{buffer};
}

[[nodiscard]] bool same_existing_path(const std::filesystem::path& left,
                                      const std::filesystem::path& right) {
    std::error_code left_error;
    std::error_code right_error;
    const auto canonical_left = std::filesystem::weakly_canonical(left, left_error);
    const auto canonical_right = std::filesystem::weakly_canonical(right, right_error);
    if (left_error || right_error) return false;
    return _wcsicmp(canonical_left.c_str(), canonical_right.c_str()) == 0;
}

} // namespace

FingerprintResult verify_build_fingerprint(const std::filesystem::path& binary_directory) {
    const auto nested = binary_directory / "ue4ss";
    auto result = verify_build_fingerprint(binary_directory, nested);
    const auto loaded = loaded_ue4ss_path();
    if (!loaded) {
        result.trusted = false;
        result.error = "loaded UE4SS module path could not be resolved; passive/off mode required";
    } else if (!same_existing_path(*loaded, nested / "UE4SS.dll")) {
        result.trusted = false;
        result.error = "loaded UE4SS module is not the pinned ExperimentalNested path; passive/off mode required";
    }
    return result;
}

FingerprintResult verify_build_fingerprint(const std::filesystem::path& binary_directory,
                                           const std::filesystem::path& ue4ss_directory) {
    FingerprintResult result{};
    const auto game = sha256_file(binary_directory / "DSClient-Win64-Shipping.exe");
    const auto ue4ss = sha256_file(ue4ss_directory / "UE4SS.dll");
    if (!ue4ss) {
        result.error = "ExperimentalNested UE4SS fingerprint input could not be read";
        return result;
    }
    if (game) result.game_sha256 = *game;
    result.ue4ss_sha256 = *ue4ss;
    // The game executable hash is diagnostic evidence, not an ABI boundary.
    // Minor game revisions have retained the interaction contract in practice,
    // while UE4SS native plugin ABI mismatches can crash during load. Keep the
    // UE4SS fingerprint as the fail-closed gate and report the game hash so an
    // untested game revision remains visible in diagnostics.
    result.trusted = expected_ue4ss_sha256(result.ue4ss_sha256);
    if (!result.trusted) {
        result.error = "unknown UE4SS build; passive/off mode required";
    } else if (!game) {
        result.error = "game executable fingerprint unavailable; diagnostic only";
    }
    return result;
}

bool atomic_replace_text(const std::filesystem::path& path, std::string_view text) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    const auto temporary = path.wstring() + L".tmp";
    {
        std::ofstream output{temporary, std::ios::binary | std::ios::trunc};
        if (!output) return false;
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.flush();
        if (!output) return false;
    }
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

} // namespace dsnap
