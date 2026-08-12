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

FingerprintResult verify_build_fingerprint(const std::filesystem::path& binary_directory) {
    FingerprintResult result{};
    const auto game = sha256_file(binary_directory / "DSClient-Win64-Shipping.exe");
    const auto ue4ss = sha256_file(binary_directory / "ue4ss" / "UE4SS.dll");
    if (!game || !ue4ss) {
        result.error = "one or more fingerprint inputs could not be read";
        return result;
    }
    result.game_sha256 = *game;
    result.ue4ss_sha256 = *ue4ss;
    result.trusted = result.game_sha256 == kExpectedGameSha256 && result.ue4ss_sha256 == kExpectedUe4ssSha256;
    if (!result.trusted) {
        result.error = "unknown game or UE4SS build; passive/off mode required";
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
