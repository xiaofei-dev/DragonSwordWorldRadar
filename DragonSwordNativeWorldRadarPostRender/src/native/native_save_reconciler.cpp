#include "native_save_reconciler.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <format>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <windows.h>

namespace dsnwr {
namespace {

using Clock = std::chrono::steady_clock;

constexpr std::array<std::wstring_view, 4> kDatabaseSuffixes{
    L"", L"-wal", L"-shm", L"-journal"};
constexpr std::size_t kMaximumKeyCharacters = 256;
constexpr std::size_t kCopyBufferBytes = 64U * 1024U;
// A corrupted local snapshot must not turn one-shot reconciliation into an
// unbounded allocation. One treasure row carries 64 bits; the other caps are
// deliberately far above the expected lifetime record counts.
constexpr std::size_t kMaximumOpenedQueryRows = 4096;
constexpr std::size_t kMaximumEncounterQueryRows = 65536;
constexpr std::size_t kMaximumDynamicQuestCompletionQueryRows = 65536;
constexpr std::uint64_t kMaximumGameExecutableBytes = 512ULL * 1024ULL * 1024ULL;

class ReconcileFailure final : public std::exception {
public:
    explicit ReconcileFailure(SaveReconcileError error) : error_{error} {}
    [[nodiscard]] SaveReconcileError error() const noexcept { return error_; }

private:
    SaveReconcileError error_{};
};

struct FileStamp {
    bool exists{};
    std::uint64_t size{};
    std::uint64_t write_time{};

    [[nodiscard]] bool operator==(const FileStamp&) const = default;
};

struct OwnerPointerConfig {
    std::uint64_t executable_length{};
    std::uint64_t owner_pointer_rva{};
};

struct MappedExecutable {
    HANDLE file{INVALID_HANDLE_VALUE};
    HANDLE mapping{};
    const std::uint8_t* bytes{};
    std::size_t size{};

    MappedExecutable() = default;
    MappedExecutable(const MappedExecutable&) = delete;
    MappedExecutable& operator=(const MappedExecutable&) = delete;
    MappedExecutable(MappedExecutable&& other) noexcept
        : file{other.file}, mapping{other.mapping}, bytes{other.bytes},
          size{other.size} {
        other.file = INVALID_HANDLE_VALUE;
        other.mapping = nullptr;
        other.bytes = nullptr;
        other.size = 0U;
    }
    ~MappedExecutable() {
        if (bytes) {
            UnmapViewOfFile(bytes);
        }
        if (mapping) {
            CloseHandle(mapping);
        }
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }
    }
};

struct SqlCipherApi {
    using Open = int(__cdecl*)(const char*, void**, int, const char*);
    using Close = int(__cdecl*)(void*);
    using ExecCallback = int(__cdecl*)(void*, int, char**, char**);
    using Exec = int(__cdecl*)(
        void*, const char*, ExecCallback, void*, char**);
    using ErrorMessage = const char*(__cdecl*)(void*);
    using Free = void(__cdecl*)(void*);

    HMODULE module{};
    Open open{};
    Close close{};
    Exec exec{};
    ErrorMessage error_message{};
    Free free_memory{};

    SqlCipherApi() = default;
    SqlCipherApi(const SqlCipherApi&) = delete;
    SqlCipherApi& operator=(const SqlCipherApi&) = delete;
    ~SqlCipherApi() {
        if (module) {
            FreeLibrary(module);
        }
    }

    [[nodiscard]] bool load(const std::filesystem::path& path) noexcept {
        module = LoadLibraryExW(
            path.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            return false;
        }
        open = reinterpret_cast<Open>(GetProcAddress(module, "sqlite3_open_v2"));
        close = reinterpret_cast<Close>(GetProcAddress(module, "sqlite3_close_v2"));
        exec = reinterpret_cast<Exec>(GetProcAddress(module, "sqlite3_exec"));
        error_message = reinterpret_cast<ErrorMessage>(
            GetProcAddress(module, "sqlite3_errmsg"));
        free_memory = reinterpret_cast<Free>(GetProcAddress(module, "sqlite3_free"));
        return open && close && exec && error_message && free_memory;
    }
};

struct DatabaseHandle {
    SqlCipherApi* api{};
    void* value{};
    ~DatabaseHandle() {
        if (api && value) {
            api->close(value);
        }
    }
};

[[nodiscard]] std::uint64_t elapsed_microseconds(Clock::time_point start) {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - start).count());
}

[[nodiscard]] std::wstring lower_ascii(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return ch >= L'A' && ch <= L'Z'
            ? static_cast<wchar_t>(ch - L'A' + L'a')
            : ch;
    });
    return value;
}

[[nodiscard]] std::string utf8(const std::filesystem::path& path) {
    const std::wstring value = path.wstring();
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), size,
            nullptr, nullptr) != size) {
        throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
    }
    return result;
}

[[nodiscard]] std::filesystem::path game_executable_path() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    buffer.resize(length);
    return std::filesystem::path{buffer};
}

[[nodiscard]] std::map<std::string, std::string> parse_config(
    const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw ReconcileFailure{SaveReconcileError::ConfigurationInvalid};
    }
    std::map<std::string, std::string> fields;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#') {
            continue;
        }
        const auto separator = line.find('=', first);
        if (separator == std::string::npos) {
            throw ReconcileFailure{SaveReconcileError::ConfigurationInvalid};
        }
        fields.emplace(
            line.substr(first, separator - first),
            line.substr(separator + 1U));
    }
    return fields;
}

template <typename Integer>
[[nodiscard]] Integer parse_integer(
    std::string_view text, int base,
    SaveReconcileError error) {
    Integer value{};
    const auto result = std::from_chars(
        text.data(), text.data() + text.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw ReconcileFailure{error};
    }
    return value;
}

[[nodiscard]] OwnerPointerConfig load_owner_pointer_config(
    const std::filesystem::path& mod_directory,
    const std::filesystem::path& executable) {
    const auto fields = parse_config(
        mod_directory / "data" / "generated" / "save_owner_pointer.cfg");
    const auto required = [&fields](const char* name) -> const std::string& {
        const auto found = fields.find(name);
        if (found == fields.end() || found->second.empty()) {
            throw ReconcileFailure{SaveReconcileError::ConfigurationInvalid};
        }
        return found->second;
    };
    if (required("schema_version") != "1"
        || required("provenance")
            != "install-time-exact-executable-pattern") {
        throw ReconcileFailure{SaveReconcileError::ConfigurationInvalid};
    }
    const auto executable_length = parse_integer<std::uint64_t>(
        required("executable_length"), 10,
        SaveReconcileError::ConfigurationInvalid);
    std::string rva_text = required("owner_pointer_rva");
    if (!rva_text.starts_with("0x") && !rva_text.starts_with("0X")) {
        throw ReconcileFailure{SaveReconcileError::ConfigurationInvalid};
    }
    const auto rva = parse_integer<std::uint64_t>(
        std::string_view{rva_text}.substr(2), 16,
        SaveReconcileError::ConfigurationInvalid);
    std::error_code size_error;
    const auto current_size = std::filesystem::file_size(executable, size_error);
    if (size_error || current_size != executable_length
        || rva == 0 || rva >= executable_length) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    return {executable_length, rva};
}

[[nodiscard]] MappedExecutable map_game_executable(
    const std::filesystem::path& executable) {
    MappedExecutable mapped{};
    mapped.file = CreateFileW(
        executable.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (mapped.file == INVALID_HANDLE_VALUE) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    LARGE_INTEGER length{};
    if (GetFileSizeEx(mapped.file, &length) == FALSE
        || length.QuadPart <= 0
        || static_cast<std::uint64_t>(length.QuadPart)
            > kMaximumGameExecutableBytes
        || static_cast<std::uint64_t>(length.QuadPart)
            > std::numeric_limits<std::size_t>::max()) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    mapped.mapping = CreateFileMappingW(
        mapped.file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapped.mapping) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    mapped.bytes = static_cast<const std::uint8_t*>(
        MapViewOfFile(mapped.mapping, FILE_MAP_READ, 0, 0, 0));
    if (!mapped.bytes) {
        throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
    }
    mapped.size = static_cast<std::size_t>(length.QuadPart);
    return mapped;
}

template <typename T>
[[nodiscard]] bool read_self_memory(std::uintptr_t address, T* value) noexcept {
    SIZE_T read{};
    return value && ReadProcessMemory(
        GetCurrentProcess(), reinterpret_cast<const void*>(address), value,
        sizeof(T), &read) != FALSE && read == sizeof(T);
}

[[nodiscard]] std::string read_save_key(
    const OwnerPointerConfig& config) {
    const auto module = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    std::uintptr_t owner{};
    if (!module || !read_self_memory(
            module + config.owner_pointer_rva, &owner)
        || owner == 0) {
        throw ReconcileFailure{SaveReconcileError::SaveKeyUnavailable};
    }
    std::uintptr_t key_pointer{};
    std::int32_t key_length{};
    if (!read_self_memory(owner + 0x120U, &key_pointer)
        || !read_self_memory(owner + 0x128U, &key_length)
        || key_pointer == 0 || key_length <= 1
        || key_length > static_cast<std::int32_t>(kMaximumKeyCharacters)) {
        throw ReconcileFailure{SaveReconcileError::SaveKeyUnavailable};
    }
    std::vector<wchar_t> characters(static_cast<std::size_t>(key_length));
    SIZE_T read{};
    const SIZE_T byte_count = characters.size() * sizeof(wchar_t);
    if (ReadProcessMemory(
            GetCurrentProcess(), reinterpret_cast<const void*>(key_pointer),
            characters.data(), byte_count, &read) == FALSE
        || read != byte_count) {
        throw ReconcileFailure{SaveReconcileError::SaveKeyUnavailable};
    }
    while (!characters.empty() && characters.back() == L'\0') {
        characters.pop_back();
    }
    std::string key;
    key.reserve(characters.size());
    for (const wchar_t character : characters) {
        if (character < 0x20 || character > 0x7E) {
            throw ReconcileFailure{SaveReconcileError::SaveKeyUnavailable};
        }
        key.push_back(static_cast<char>(character));
    }
    if (key.empty()) {
        throw ReconcileFailure{SaveReconcileError::SaveKeyUnavailable};
    }
    return key;
}

[[nodiscard]] FileStamp capture_stamp(const std::filesystem::path& path) noexcept {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (GetFileAttributesExW(
            path.c_str(), GetFileExInfoStandard, &data) == FALSE
        || (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return {};
    }
    ULARGE_INTEGER size{};
    size.HighPart = data.nFileSizeHigh;
    size.LowPart = data.nFileSizeLow;
    ULARGE_INTEGER time{};
    time.HighPart = data.ftLastWriteTime.dwHighDateTime;
    time.LowPart = data.ftLastWriteTime.dwLowDateTime;
    return {true, size.QuadPart, time.QuadPart};
}

[[nodiscard]] std::array<FileStamp, kDatabaseSuffixes.size()> capture_database(
    const std::filesystem::path& base) noexcept {
    std::array<FileStamp, kDatabaseSuffixes.size()> result{};
    for (std::size_t index = 0; index < kDatabaseSuffixes.size(); ++index) {
        result[index] = capture_stamp(
            std::filesystem::path{base.wstring() +
                std::wstring{kDatabaseSuffixes[index]}});
    }
    return result;
}

void copy_shared_file(
    const std::filesystem::path& source,
    const std::filesystem::path& destination) {
    HANDLE input = CreateFileW(
        source.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (input == INVALID_HANDLE_VALUE) {
        throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
    }
    HANDLE output = CreateFileW(
        destination.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE) {
        CloseHandle(input);
        throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
    }

    std::array<std::byte, kCopyBufferBytes> buffer{};
    bool success = true;
    for (;;) {
        DWORD bytes_read{};
        if (ReadFile(
                input, buffer.data(), static_cast<DWORD>(buffer.size()),
                &bytes_read, nullptr) == FALSE) {
            success = false;
            break;
        }
        if (bytes_read == 0) {
            break;
        }
        DWORD total_written{};
        while (total_written < bytes_read) {
            DWORD written{};
            if (WriteFile(
                    output, buffer.data() + total_written,
                    bytes_read - total_written, &written, nullptr) == FALSE
                || written == 0) {
                success = false;
                break;
            }
            total_written += written;
        }
        if (!success) {
            break;
        }
    }
    CloseHandle(output);
    CloseHandle(input);
    if (!success) {
        throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
    }
}

[[nodiscard]] std::filesystem::path copy_consistent_database(
    const std::filesystem::path& source,
    const std::filesystem::path& directory) {
    const auto destination = directory / source.filename();
    for (int attempt = 0; attempt < 3; ++attempt) {
        const auto before = capture_database(source);
        if (!before[0].exists) {
            throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
        }
        for (std::size_t index = 0; index < kDatabaseSuffixes.size(); ++index) {
            const auto source_part = std::filesystem::path{
                source.wstring() + std::wstring{kDatabaseSuffixes[index]}};
            const auto destination_part = std::filesystem::path{
                destination.wstring() + std::wstring{kDatabaseSuffixes[index]}};
            if (before[index].exists) {
                copy_shared_file(source_part, destination_part);
            } else {
                std::error_code ignored;
                std::filesystem::remove(destination_part, ignored);
            }
        }
        if (before == capture_database(source)) {
            return destination;
        }
    }
    throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
}

[[nodiscard]] std::vector<std::filesystem::path> find_active_slot_databases(
    const std::filesystem::path& executable) {
    const auto save_root = executable.parent_path().parent_path().parent_path()
        / "Saved" / "SaveGames";
    std::error_code exists_error;
    if (!std::filesystem::is_directory(save_root, exists_error)) {
        throw ReconcileFailure{SaveReconcileError::SaveSlotUnavailable};
    }

    struct Slot {
        std::filesystem::path stem{};
        std::filesystem::file_time_type newest{};
    };
    std::map<std::wstring, Slot> slots;
    std::error_code iterator_error;
    std::filesystem::recursive_directory_iterator iterator{
        save_root, std::filesystem::directory_options::skip_permission_denied,
        iterator_error};
    const std::filesystem::recursive_directory_iterator end;
    for (; !iterator_error && iterator != end;
         iterator.increment(iterator_error)) {
        std::error_code type_error;
        if (!iterator->is_regular_file(type_error) || type_error) {
            continue;
        }
        const auto path = iterator->path();
        const auto extension = lower_ascii(path.extension().wstring());
        const auto filename = lower_ascii(path.filename().wstring());
        if ((extension != L".db" && extension != L".bak")
            || filename.find(L"_slot") == std::wstring::npos) {
            continue;
        }
        const auto stem = path.parent_path() / path.stem();
        std::error_code time_error;
        const auto write_time = std::filesystem::last_write_time(path, time_error);
        if (time_error) {
            continue;
        }
        auto& slot = slots[lower_ascii(stem.wstring())];
        slot.stem = stem;
        slot.newest = std::max(slot.newest, write_time);
    }
    if (iterator_error || slots.empty()) {
        throw ReconcileFailure{SaveReconcileError::SaveSlotUnavailable};
    }
    const auto selected = std::max_element(
        slots.begin(), slots.end(), [](const auto& left, const auto& right) {
            return left.second.newest < right.second.newest;
        });

    std::vector<std::filesystem::path> databases;
    for (const auto* extension : {L".db", L".bak"}) {
        const auto path = std::filesystem::path{
            selected->second.stem.wstring() + extension};
        if (capture_stamp(path).exists) {
            databases.push_back(path);
        }
    }
    std::sort(databases.begin(), databases.end(), [](const auto& left, const auto& right) {
        return capture_stamp(left).write_time > capture_stamp(right).write_time;
    });
    if (databases.empty()) {
        throw ReconcileFailure{SaveReconcileError::SaveSlotUnavailable};
    }
    return databases;
}

void execute_sql(
    SqlCipherApi& api, void* database, const std::string& sql,
    SqlCipherApi::ExecCallback callback = nullptr,
    void* context = nullptr) {
    char* error_message{};
    const int result = api.exec(
        database, sql.c_str(), callback, context, &error_message);
    if (error_message) {
        api.free_memory(error_message);
    }
    if (result != 0) {
        throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
    }
}

[[nodiscard]] bool execute_optional_sql(
    SqlCipherApi& api, void* database, const char* sql,
    SqlCipherApi::ExecCallback callback, void* context) noexcept {
    char* error_message{};
    const int result = api.exec(
        database, sql, callback, context, &error_message);
    if (error_message) {
        api.free_memory(error_message);
    }
    return result == 0;
}

[[nodiscard]] bool parse_int64(
    const char* value, std::int64_t& parsed) noexcept {
    if (!value) {
        return false;
    }
    const std::string_view text{value};
    const auto result = std::from_chars(
        text.data(), text.data() + text.size(), parsed);
    return result.ec == std::errc{}
        && result.ptr == text.data() + text.size();
}

struct OpenedQueryContext {
    std::unordered_map<std::int64_t, std::uint64_t>* fields{};
    std::size_t row_count{};
    bool parse_failed{};
};

int __cdecl opened_query_callback(
    void* opaque, int count, char** values, char**) noexcept {
    auto* context = static_cast<OpenedQueryContext*>(opaque);
    if (!context) {
        return 1;
    }
    if (!context->fields || count < 2 || !values
        || !values[0] || !values[1]) {
        context->parse_failed = true;
        return 1;
    }
    try {
        if (context->row_count >= kMaximumOpenedQueryRows) {
            context->parse_failed = true;
            return 1;
        }
        ++context->row_count;
        std::int64_t category{};
        std::int64_t signed_bits{};
        const std::string_view category_text{values[0]};
        const std::string_view bits_text{values[1]};
        const auto category_result = std::from_chars(
            category_text.data(), category_text.data() + category_text.size(),
            category);
        const auto bits_result = std::from_chars(
            bits_text.data(), bits_text.data() + bits_text.size(), signed_bits);
        if (category_result.ec != std::errc{}
            || category_result.ptr
                != category_text.data() + category_text.size()
            || bits_result.ec != std::errc{}
            || bits_result.ptr != bits_text.data() + bits_text.size()) {
            context->parse_failed = true;
            return 1;
        }
        (*context->fields)[category] |=
            static_cast<std::uint64_t>(signed_bits);
        return 0;
    } catch (...) {
        context->parse_failed = true;
        return 1;
    }
}

struct EncounterQueryContext {
    std::unordered_map<std::int64_t, EncounterRespawnField>* fields{};
    std::size_t row_count{};
    bool parse_failed{};
};

int __cdecl encounter_query_callback(
    void* opaque, int count, char** values, char**) noexcept {
    auto* context = static_cast<EncounterQueryContext*>(opaque);
    if (!context) {
        return 1;
    }
    if (!context->fields || count < 3 || !values
        || !values[0] || !values[1] || !values[2]) {
        context->parse_failed = true;
        return 1;
    }
    try {
        if (context->row_count >= kMaximumEncounterQueryRows) {
            context->parse_failed = true;
            return 1;
        }
        ++context->row_count;
        std::int64_t id{};
        std::int32_t respawn_type{};
        std::int64_t destroy_time{};
        const std::string_view id_text{values[0]};
        const std::string_view respawn_text{values[1]};
        const std::string_view destroy_text{values[2]};
        const auto id_result = std::from_chars(
            id_text.data(), id_text.data() + id_text.size(), id);
        const auto respawn_result = std::from_chars(
            respawn_text.data(), respawn_text.data() + respawn_text.size(),
            respawn_type);
        const auto destroy_result = std::from_chars(
            destroy_text.data(), destroy_text.data() + destroy_text.size(),
            destroy_time);
        if (id_result.ec != std::errc{}
            || id_result.ptr != id_text.data() + id_text.size()
            || respawn_result.ec != std::errc{}
            || respawn_result.ptr != respawn_text.data() + respawn_text.size()
            || destroy_result.ec != std::errc{}
            || destroy_result.ptr != destroy_text.data() + destroy_text.size()
            || id <= 0) {
            context->parse_failed = true;
            return 1;
        }
        const EncounterRespawnField next{id, respawn_type, destroy_time};
        const auto found = context->fields->find(id);
        if (found == context->fields->end()
            || found->second.destroy_time_unix_seconds < destroy_time) {
            (*context->fields)[id] = next;
        }
        return 0;
    } catch (...) {
        context->parse_failed = true;
        return 1;
    }
}

struct DynamicQuestCompletionQueryContext {
    std::unordered_map<std::int64_t, DynamicQuestCompletionField>* fields{};
    std::optional<std::int64_t> user_dbid{};
    std::size_t row_count{};
    bool parse_failed{};
    bool user_identity_ambiguous{};
};

int __cdecl dynamic_quest_completion_query_callback(
    void* opaque, int count, char** values, char**) noexcept {
    auto* context = static_cast<DynamicQuestCompletionQueryContext*>(opaque);
    if (!context || !context->fields || count < 3 || !values) {
        if (context) {
            context->parse_failed = true;
        }
        return 1;
    }
    try {
        if (context->row_count
            >= kMaximumDynamicQuestCompletionQueryRows) {
            context->parse_failed = true;
            return 1;
        }
        ++context->row_count;
        std::int64_t user_dbid{};
        DynamicQuestCompletionField field{};
        if (!parse_int64(values[0], user_dbid)
            || !parse_int64(values[1], field.quest_id)
            || !parse_int64(values[2], field.complete_count)
            || user_dbid <= 0 || field.quest_id <= 0
            || field.complete_count < 0) {
            context->parse_failed = true;
            return 1;
        }
        if (context->user_dbid
            && *context->user_dbid != user_dbid) {
            context->parse_failed = true;
            context->user_identity_ambiguous = true;
            return 1;
        }
        context->user_dbid = user_dbid;
        const auto [existing, inserted] =
            context->fields->emplace(field.quest_id, field);
        if (!inserted
            && existing->second.complete_count != field.complete_count) {
            context->parse_failed = true;
            return 1;
        }
        return 0;
    } catch (...) {
        context->parse_failed = true;
        return 1;
    }
}

struct OptionalQuestQueryFields {
    std::unordered_map<std::int64_t, DynamicQuestCompletionField>
        completions{};
    std::optional<std::int64_t> user_dbid{};
    bool completion_query_available{};
    bool completion_identity_ambiguous{};
};
void query_save_fields(
    SqlCipherApi& api, const std::filesystem::path& path,
    const std::string& key,
    std::unordered_map<std::int64_t, std::uint64_t>* opened_fields,
    std::span<const std::int64_t> exact_treasure_ids,
    std::unordered_map<std::int64_t, EncounterRespawnField>*
        encounter_fields,
    OptionalQuestQueryFields* quest_fields) {
    const std::string path_utf8 = utf8(path);
    DatabaseHandle database{&api};
    if (api.open(path_utf8.c_str(), &database.value, 0x00000002, nullptr) != 0
        || !database.value) {
        throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
    }
    std::string escaped_key;
    escaped_key.reserve(key.size());
    for (const char character : key) {
        escaped_key.push_back(character);
        if (character == '\'') {
            escaped_key.push_back('\'');
        }
    }
    execute_sql(
        api, database.value,
        "PRAGMA key = '" + escaped_key
            + "';PRAGMA cipher_compatibility = 4;");
    if (opened_fields) {
        OpenedQueryContext context{opened_fields};
        std::string treasure_query{
            "SELECT CATEGORY,OPENED_BIT_FIELD FROM tb_treasure_box"};
        if (!exact_treasure_ids.empty()) {
            std::array<std::int64_t, kMaximumTreasureConfirmationIds>
                categories{};
            std::size_t category_count{};
            for (const std::int64_t save_id : exact_treasure_ids) {
                if (save_id <= 0) {
                    throw ReconcileFailure{
                        SaveReconcileError::ConfigurationInvalid};
                }
                const std::int64_t category = save_id / 64;
                const auto category_end =
                    categories.begin() + category_count;
                if (std::find(
                        categories.begin(), category_end, category)
                    != category_end) {
                    continue;
                }
                if (category_count >= categories.size()) {
                    throw ReconcileFailure{
                        SaveReconcileError::ConfigurationInvalid};
                }
                categories[category_count++] = category;
            }
            treasure_query += " WHERE CATEGORY IN (";
            for (std::size_t index = 0; index < category_count; ++index) {
                if (index != 0U) {
                    treasure_query.push_back(',');
                }
                treasure_query += std::to_string(categories[index]);
            }
            treasure_query.push_back(')');
        }
        treasure_query.push_back(';');
        execute_sql(
            api, database.value,
            treasure_query,
            &opened_query_callback, &context);
        if (context.parse_failed) {
            throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
        }
    }
    if (encounter_fields) {
        EncounterQueryContext encounter_context{encounter_fields};
        execute_sql(
            api, database.value,
            "SELECT ACTOR_CID,RESPAWN_TYPE,DESTROY_TIME "
            "FROM tb_actor_respawn;",
            &encounter_query_callback, &encounter_context);
        if (encounter_context.parse_failed) {
            throw ReconcileFailure{SaveReconcileError::SqlCipherQueryFailed};
        }
    }
    if (!quest_fields) {
        return;
    }
    if (!quest_fields->completion_query_available
        && !quest_fields->completion_identity_ambiguous) {
        std::unordered_map<std::int64_t, DynamicQuestCompletionField>
            current_completions;
        DynamicQuestCompletionQueryContext completion_context{
            &current_completions};
        const bool completion_query_succeeded = execute_optional_sql(
            api, database.value,
            "SELECT USER_DBID,QUEST_ID,COMPLETE_CNT "
            "FROM tb_dynamic_quest_complete;",
            &dynamic_quest_completion_query_callback, &completion_context);
        if (completion_context.user_identity_ambiguous) {
            // Never replace an ambiguous newest readable source with an older
            // single-user backup. Without an independently bound active user,
            // that fallback could render another character's quest history.
            quest_fields->completions.clear();
            quest_fields->user_dbid.reset();
            quest_fields->completion_identity_ambiguous = true;
        } else if (completion_query_succeeded
                   && !completion_context.parse_failed
                   && completion_context.user_dbid.has_value()) {
            quest_fields->completions = std::move(current_completions);
            quest_fields->user_dbid = completion_context.user_dbid;
            quest_fields->completion_query_available = true;
        }
    }

}

} // namespace

NativeSaveReconciler::~NativeSaveReconciler() {
    shutdown();
}

bool NativeSaveReconciler::initialize(
    std::filesystem::path mod_directory) noexcept {
    if (initialized_) {
        return true;
    }
    try {
        mod_directory_ = std::move(mod_directory);
        worker_ = std::jthread{
            [this](std::stop_token stop_token) { worker_loop(stop_token); }};
        initialized_ = true;
        return true;
    } catch (...) {
        mod_directory_.clear();
        initialized_ = false;
        return false;
    }
}

bool NativeSaveReconciler::request(
    std::uint32_t activation,
    std::uint32_t request_id,
    SaveReconcileScope scope,
    std::span<const std::int64_t> treasure_ids) noexcept {
    if (!initialized_ || activation == 0 || request_id == 0) {
        return false;
    }
    const bool treasure_confirmation =
        scope == SaveReconcileScope::TreasureConfirmation;
    if ((treasure_confirmation && treasure_ids.empty())
        || treasure_ids.size() > kMaximumTreasureConfirmationIds
        || (!treasure_confirmation && !treasure_ids.empty())
        || std::any_of(
            treasure_ids.begin(), treasure_ids.end(),
            [](std::int64_t id) { return id <= 0; })) {
        return false;
    }
    try {
        std::scoped_lock lock{mutex_};
        if (request_active_ || pending_.has_value()
            || completed_.has_value()) {
            return false;
        }
        request_active_ = true;
        Request request{};
        request.activation = activation;
        request.request_id = request_id;
        request.scope = scope;
        request.treasure_id_count = treasure_ids.size();
        std::copy(
            treasure_ids.begin(), treasure_ids.end(),
            request.treasure_ids.begin());
        pending_ = request;
        wake_.notify_one();
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<SaveReconcileResult> NativeSaveReconciler::try_take() noexcept {
    if (!completed_ready_.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    try {
        std::scoped_lock lock{mutex_};
        auto result = std::move(completed_);
        completed_.reset();
        request_active_ = false;
        completed_ready_.store(false, std::memory_order_release);
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

void NativeSaveReconciler::shutdown() noexcept {
    initialized_ = false;
    if (!worker_.joinable()) {
        return;
    }
    worker_.request_stop();
    wake_.notify_all();
    worker_.join();
}

void NativeSaveReconciler::worker_loop(std::stop_token stop_token) noexcept {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    while (!stop_token.stop_requested()) {
        Request request{};
        {
            std::unique_lock lock{mutex_};
            wake_.wait(lock, stop_token, [this] { return pending_.has_value(); });
            if (stop_token.stop_requested()) {
                break;
            }
            request = *pending_;
            pending_.reset();
        }
        auto result = run_request(request);
        {
            std::scoped_lock lock{mutex_};
            completed_ = std::move(result);
            completed_ready_.store(true, std::memory_order_release);
        }
    }
}

SaveReconcileResult NativeSaveReconciler::run_request(
    const Request& request) noexcept {
    SaveReconcileResult result{};
    result.activation = request.activation;
    result.request_id = request.request_id;
    result.scope = request.scope;
    result.requested_treasure_count =
        static_cast<std::uint32_t>(request.treasure_id_count);
    const auto total_start = Clock::now();
    const auto owner_pointer_start = Clock::now();
    try {
        const auto executable = game_executable_path();
        std::error_code executable_size_error;
        const std::uint64_t executable_length =
            std::filesystem::file_size(executable, executable_size_error);
        if (executable_size_error || executable_length == 0U
            || executable_length > kMaximumGameExecutableBytes) {
            throw ReconcileFailure{SaveReconcileError::GameImageMismatch};
        }
        result.owner_pointer_pattern_attempted =
            owner_pointer_pattern_attempted_;
        result.owner_pointer_pattern_status =
            owner_pointer_pattern_status_;

        std::string key;
        if (cached_owner_pointer_rva_ != 0U
            && cached_owner_pointer_executable_length_ == executable_length) {
            result.owner_pointer_route = cached_owner_pointer_route_;
            key = read_save_key({
                cached_owner_pointer_executable_length_,
                cached_owner_pointer_rva_});
        } else {
            cached_owner_pointer_executable_length_ = 0U;
            cached_owner_pointer_rva_ = 0U;
            cached_owner_pointer_route_ = SaveOwnerPointerRoute::None;

            try {
                const auto config = load_owner_pointer_config(
                    mod_directory_, executable);
                key = read_save_key(config);
                cached_owner_pointer_executable_length_ =
                    config.executable_length;
                cached_owner_pointer_rva_ = config.owner_pointer_rva;
                cached_owner_pointer_route_ =
                    SaveOwnerPointerRoute::PackagedConfig;
                result.owner_pointer_route = cached_owner_pointer_route_;
            } catch (const ReconcileFailure&) {
                if (!owner_pointer_pattern_attempted_) {
                    owner_pointer_pattern_attempted_ = true;
                    result.owner_pointer_pattern_attempted = true;
                    const auto mapped = map_game_executable(executable);
                    const auto resolved = dswros::resolve_owner_pointer_rva(
                        std::span<const std::uint8_t>{
                            mapped.bytes, mapped.size});
                    owner_pointer_pattern_status_ = resolved.status;
                    if (!resolved.success()) {
                        result.owner_pointer_pattern_attempted = true;
                        result.owner_pointer_pattern_status = resolved.status;
                        throw ReconcileFailure{
                            SaveReconcileError::GameImageMismatch};
                    }
                    // Cache the unique numeric candidate before reading the
                    // live key. If the owner is temporarily unavailable, a
                    // later F7 can retry without rescanning the executable.
                    cached_owner_pointer_executable_length_ =
                        executable_length;
                    cached_owner_pointer_rva_ = resolved.rva;
                    cached_owner_pointer_route_ =
                        SaveOwnerPointerRoute::RuntimePattern;
                }
                if (cached_owner_pointer_rva_ == 0U
                    || cached_owner_pointer_executable_length_
                        != executable_length
                    || cached_owner_pointer_route_
                        != SaveOwnerPointerRoute::RuntimePattern) {
                    throw ReconcileFailure{
                        SaveReconcileError::GameImageMismatch};
                }
                result.owner_pointer_route = cached_owner_pointer_route_;
                result.owner_pointer_pattern_attempted =
                    owner_pointer_pattern_attempted_;
                result.owner_pointer_pattern_status =
                    owner_pointer_pattern_status_;
                key = read_save_key({
                    cached_owner_pointer_executable_length_,
                    cached_owner_pointer_rva_});
            }
        }
        result.owner_pointer_pattern_attempted =
            owner_pointer_pattern_attempted_;
        result.owner_pointer_pattern_status =
            owner_pointer_pattern_status_;
        if (result.owner_pointer_route == SaveOwnerPointerRoute::None) {
            result.owner_pointer_resolution_us =
                elapsed_microseconds(owner_pointer_start);
        }
        const auto sources = find_active_slot_databases(executable);

        SqlCipherApi api;
        if (!api.load(
                mod_directory_ / "vendor" / "sqlcipher"
                    / "e_sqlcipher.dll")) {
            throw ReconcileFailure{SaveReconcileError::SqlCipherUnavailable};
        }

        std::wstring temporary_root(32768, L'\0');
        const DWORD temporary_length = GetTempPathW(
            static_cast<DWORD>(temporary_root.size()), temporary_root.data());
        if (temporary_length == 0 || temporary_length >= temporary_root.size()) {
            throw ReconcileFailure{SaveReconcileError::SnapshotCopyFailed};
        }
        temporary_root.resize(temporary_length);
        const auto temporary_directory = std::filesystem::path{temporary_root}
            / "DragonSwordNativeWorldRadarPostRender"
            / std::format(
                L"{}-{}-{}", GetCurrentProcessId(), request.activation,
                GetTickCount64());
        std::filesystem::create_directories(temporary_directory);

        struct Cleanup final {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code ignored;
                std::filesystem::remove_all(path, ignored);
            }
        } cleanup{temporary_directory};

        std::unordered_map<std::int64_t, std::uint64_t> fields;
        std::unordered_map<std::int64_t, EncounterRespawnField>
            encounter_fields;
        OptionalQuestQueryFields quest_fields;
        std::uint32_t loaded{};
        for (const auto& source : sources) {
            try {
                std::unordered_map<std::int64_t, std::uint64_t>
                    source_fields;
                std::unordered_map<std::int64_t, EncounterRespawnField>
                    source_encounter_fields;
                const auto copy_start = Clock::now();
                const auto snapshot = copy_consistent_database(
                    source, temporary_directory);
                result.copy_elapsed_us += elapsed_microseconds(copy_start);
                const auto query_start = Clock::now();
                query_save_fields(
                    api, snapshot, key,
                    request.scope == SaveReconcileScope::FullActivation
                            || request.scope
                                == SaveReconcileScope::TreasureConfirmation
                        ? &source_fields : nullptr,
                    std::span<const std::int64_t>{
                        request.treasure_ids.data(),
                        request.treasure_id_count},
                    request.scope
                            == SaveReconcileScope::TreasureConfirmation
                        ? nullptr : &source_encounter_fields,
                    request.scope
                            == SaveReconcileScope::TreasureConfirmation
                        ? nullptr : &quest_fields);
                result.query_elapsed_us += elapsed_microseconds(query_start);
                for (const auto& [category, bits] : source_fields) {
                    fields[category] |= bits;
                }
                for (const auto& [id, field] : source_encounter_fields) {
                    const auto found = encounter_fields.find(id);
                    if (found == encounter_fields.end()
                        || found->second.destroy_time_unix_seconds
                            < field.destroy_time_unix_seconds) {
                        encounter_fields[id] = field;
                    }
                }
                ++loaded;
            } catch (const ReconcileFailure&) {
            }
        }
        if (loaded == 0) {
            throw ReconcileFailure{SaveReconcileError::NoReadableDatabase};
        }
        result.database_count = loaded;
        result.opened_fields.reserve(fields.size());
        for (const auto& [category, bits] : fields) {
            result.opened_fields.push_back({category, bits});
            result.opened_bit_count += std::popcount(bits);
        }
        std::sort(
            result.opened_fields.begin(), result.opened_fields.end(),
            [](const auto& left, const auto& right) {
                return left.category < right.category;
            });
        result.encounter_respawns.reserve(encounter_fields.size());
        for (const auto& [id, field] : encounter_fields) {
            static_cast<void>(id);
            result.encounter_respawns.push_back(field);
        }
        std::sort(
            result.encounter_respawns.begin(), result.encounter_respawns.end(),
            [](const auto& left, const auto& right) {
                return left.id < right.id;
            });
        result.dynamic_quest_completion_query_available =
            quest_fields.completion_query_available;
        result.dynamic_quest_completion_identity_ambiguous =
            quest_fields.completion_identity_ambiguous;
        result.dynamic_quest_completion_user_dbid =
            quest_fields.user_dbid.value_or(0);
        result.dynamic_quest_completions.reserve(
            quest_fields.completions.size());
        for (const auto& [quest_id, field] : quest_fields.completions) {
            static_cast<void>(quest_id);
            result.dynamic_quest_completions.push_back(field);
        }
        std::sort(
            result.dynamic_quest_completions.begin(),
            result.dynamic_quest_completions.end(),
            [](const auto& left, const auto& right) {
                return left.quest_id < right.quest_id;
            });
        result.error = SaveReconcileError::None;
    } catch (const ReconcileFailure& failure) {
        result.owner_pointer_pattern_attempted =
            owner_pointer_pattern_attempted_;
        result.owner_pointer_pattern_status =
            owner_pointer_pattern_status_;
        if (result.owner_pointer_route == SaveOwnerPointerRoute::None) {
            result.owner_pointer_resolution_us =
                elapsed_microseconds(owner_pointer_start);
        }
        result.error = failure.error();
    } catch (...) {
        result.owner_pointer_pattern_attempted =
            owner_pointer_pattern_attempted_;
        result.owner_pointer_pattern_status =
            owner_pointer_pattern_status_;
        result.owner_pointer_resolution_us =
            elapsed_microseconds(owner_pointer_start);
        result.error = SaveReconcileError::NoReadableDatabase;
    }
    result.total_elapsed_us = elapsed_microseconds(total_start);
    return result;
}

} // namespace dsnwr
