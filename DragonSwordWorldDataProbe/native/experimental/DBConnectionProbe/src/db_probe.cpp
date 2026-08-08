#include "db_probe.hpp"

#include <psapi.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

#pragma comment(lib, "Psapi.lib")

namespace dsw::dbprobe {
namespace {

std::string utc_timestamp() {
    SYSTEMTIME st{};
    GetSystemTime(&st);
    char buffer[64]{};
    std::snprintf(
        buffer, sizeof(buffer),
        "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
    );
    return buffer;
}

std::string narrow(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr
    );
    if (size <= 0) return {};
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        result.data(), size, nullptr, nullptr
    );
    return result;
}

std::string module_path(HMODULE module) {
    std::wstring buffer(32768, L'\0');
    const DWORD len = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) return {};
    buffer.resize(len);
    return narrow(buffer);
}

template <typename T>
T proc(HMODULE module, const char* name) {
    return reinterpret_cast<T>(GetProcAddress(module, name));
}

bool contains_ci(std::string haystack, std::string needle) {
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

bool SqliteApi::can_query() const noexcept {
    return prepare_v2 && step && finalize && column_count && column_name && column_text;
}

Logger::Logger(std::filesystem::path path) : path_(std::move(path)) {
    std::error_code ec;
    std::filesystem::create_directories(path_.parent_path(), ec);
}

void Logger::line(const std::string& event, const std::string& details) {
    std::lock_guard lock(mutex_);
    std::ofstream out(path_, std::ios::app | std::ios::binary);
    if (!out) return;
    out << utc_timestamp() << '\t' << event << '\t' << details << "\r\n";
}

Probe::Probe(HMODULE self) : self_(self) {}

Probe::~Probe() {
    stop();
}

bool Probe::start() {
    if (worker_) return true;
    stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stop_event_) return false;

    const auto log_path = runtime_log_directory() / "native-db-probe.log";
    logger_ = std::make_unique<Logger>(log_path);
    logger_->line("PROBE_START", "mode=passive,version=0.6.1");

    worker_ = CreateThread(
        nullptr, 0,
        [](LPVOID param) -> DWORD {
            static_cast<Probe*>(param)->run();
            return 0;
        },
        this, 0, nullptr
    );
    return worker_ != nullptr;
}

void Probe::stop() {
    if (stop_event_) SetEvent(stop_event_);
    if (worker_) {
        WaitForSingleObject(worker_, 3000);
        CloseHandle(worker_);
        worker_ = nullptr;
    }
    if (stop_event_) {
        CloseHandle(stop_event_);
        stop_event_ = nullptr;
    }
}

void Probe::run() {
    // Delay until the game has initialized and opened its save database.
    if (WaitForSingleObject(stop_event_, 30000) == WAIT_OBJECT_0) return;

    enumerate_modules();
    resolve_sqlite_apis();
    inspect_candidate_modules();
    try_read_existing_database_files();
    write_summary();

    // Repeat module/API discovery slowly because some modules may load late.
    while (WaitForSingleObject(stop_event_, 15000) == WAIT_TIMEOUT) {
        enumerate_modules();
        resolve_sqlite_apis();
        inspect_candidate_modules();
    }
    if (logger_) logger_->line("PROBE_STOP", "normal=1");
}

void Probe::enumerate_modules() {
    HMODULE handles[2048]{};
    DWORD required = 0;
    if (!EnumProcessModules(GetCurrentProcess(), handles, sizeof(handles), &required)) {
        logger_->line("MODULE_ENUM_FAILED", "error=" + std::to_string(GetLastError()));
        return;
    }

    const size_t count = std::min<size_t>(required / sizeof(HMODULE), std::size(handles));
    std::vector<std::string> current;
    current.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const auto path = module_path(handles[i]);
        if (path.empty()) continue;
        current.push_back(path);
        if (std::find(modules_.begin(), modules_.end(), path) == modules_.end()) {
            logger_->line(
                "MODULE_LOADED",
                "base=" + std::to_string(reinterpret_cast<std::uintptr_t>(handles[i])) +
                ",path=" + path
            );
        }
    }
    modules_ = std::move(current);
}

void Probe::resolve_sqlite_apis() {
    HMODULE handles[2048]{};
    DWORD required = 0;
    if (!EnumProcessModules(GetCurrentProcess(), handles, sizeof(handles), &required)) return;
    const size_t count = std::min<size_t>(required / sizeof(HMODULE), std::size(handles));

    for (size_t i = 0; i < count; ++i) {
        HMODULE module = handles[i];
        const auto path = module_path(module);
        if (path.empty()) continue;

        const auto prepare = proc<SqliteApi::PrepareV2Fn>(module, "sqlite3_prepare_v2");
        const auto open_v2 = proc<SqliteApi::OpenV2Fn>(module, "sqlite3_open_v2");
        const auto key = proc<SqliteApi::KeyFn>(module, "sqlite3_key");
        const auto key_v2 = proc<SqliteApi::KeyV2Fn>(module, "sqlite3_key_v2");

        if (!prepare && !open_v2 && !key && !key_v2) continue;

        const bool already = std::any_of(
            apis_.begin(), apis_.end(),
            [module](const SqliteApi& api) { return api.module == module; }
        );
        if (already) continue;

        SqliteApi api{};
        api.module = module;
        api.module_name = path;
        api.open_v2 = open_v2;
        api.close = proc<SqliteApi::CloseFn>(module, "sqlite3_close");
        api.exec = proc<SqliteApi::ExecFn>(module, "sqlite3_exec");
        api.prepare_v2 = prepare;
        api.step = proc<SqliteApi::StepFn>(module, "sqlite3_step");
        api.finalize = proc<SqliteApi::FinalizeFn>(module, "sqlite3_finalize");
        api.column_count = proc<SqliteApi::ColumnCountFn>(module, "sqlite3_column_count");
        api.column_name = proc<SqliteApi::ColumnNameFn>(module, "sqlite3_column_name");
        api.column_text = proc<SqliteApi::ColumnTextFn>(module, "sqlite3_column_text");
        api.errmsg = proc<SqliteApi::ErrMsgFn>(module, "sqlite3_errmsg");
        api.db_filename = proc<SqliteApi::DbFilenameFn>(module, "sqlite3_db_filename");
        api.key = key;
        api.key_v2 = key_v2;

        logger_->line(
            "SQLITE_API_MODULE",
            "path=" + path +
            ",open_v2=" + std::to_string(api.open_v2 != nullptr) +
            ",prepare_v2=" + std::to_string(api.prepare_v2 != nullptr) +
            ",key=" + std::to_string(api.key != nullptr) +
            ",key_v2=" + std::to_string(api.key_v2 != nullptr) +
            ",can_query=" + std::to_string(api.can_query())
        );
        apis_.push_back(api);
    }
}

void Probe::inspect_candidate_modules() {
    for (const auto& path : modules_) {
        if (contains_ci(path, "sqlite") ||
            contains_ci(path, "sqlcipher") ||
            contains_ci(path, "cipher") ||
            contains_ci(path, "database") ||
            contains_ci(path, "save")) {
            logger_->line("DATABASE_MODULE_CANDIDATE", "path=" + path);
        }
    }
}

std::filesystem::path Probe::module_directory() const {
    std::wstring buffer(32768, L'\0');
    const DWORD len = GetModuleFileNameW(self_, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (len == 0) return std::filesystem::current_path();
    buffer.resize(len);
    return std::filesystem::path(buffer).parent_path();
}

std::filesystem::path Probe::runtime_log_directory() const {
    // Expected layout:
    // DragonSwordWorldDataProbe/native/DBConnectionProbe/bin/DBConnectionProbe.dll
    // Move up to the mod root and use runtime/logs.
    auto p = module_directory();
    for (int i = 0; i < 4 && p.has_parent_path(); ++i) p = p.parent_path();
    return p / "runtime" / "logs";
}

std::vector<std::filesystem::path> Probe::candidate_save_files() const {
    std::vector<std::filesystem::path> result;
    wchar_t* local_app_data = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&local_app_data, &len, L"LOCALAPPDATA") == 0 && local_app_data) {
        const std::filesystem::path root(local_app_data);
        free(local_app_data);
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            const auto name = entry.path().filename().wstring();
            if (name.find(L"_Slot1.db") != std::wstring::npos) {
                result.push_back(entry.path());
                if (result.size() >= 32) break;
            }
        }
    }
    return result;
}

void Probe::try_read_existing_database_files() {
    // This is intentionally non-invasive. Opening the encrypted file with an API module
    // is useful only if the same module applies an implicit key internally. Most builds
    // will reject it; the result still identifies the active SQLite implementation.
    constexpr int SQLITE_OK = 0;
    constexpr int SQLITE_OPEN_READONLY = 0x00000001;

    for (const auto& file : candidate_save_files()) {
        logger_->line("SAVE_DATABASE_CANDIDATE", "path=" + narrow(file.wstring()));
        for (const auto& api : apis_) {
            if (!api.open_v2 || !api.close) continue;
            SqliteApi::sqlite3* db = nullptr;
            const std::string utf8 = narrow(file.wstring());
            const int rc = api.open_v2(utf8.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
            logger_->line(
                "DIRECT_READONLY_OPEN",
                "module=" + api.module_name +
                ",path=" + utf8 +
                ",rc=" + std::to_string(rc) +
                ",db=" + std::to_string(reinterpret_cast<std::uintptr_t>(db))
            );
            if (rc == SQLITE_OK && db && api.can_query()) {
                SqliteApi::sqlite3_stmt* stmt = nullptr;
                const char* tail = nullptr;
                const int prc = api.prepare_v2(
                    db,
                    "SELECT name,type FROM sqlite_master ORDER BY type,name;",
                    -1, &stmt, &tail
                );
                std::string err;
                if (api.errmsg) {
                    const char* e = api.errmsg(db);
                    if (e) err = e;
                }
                logger_->line(
                    "DIRECT_SQLITE_MASTER_PREPARE",
                    "module=" + api.module_name +
                    ",rc=" + std::to_string(prc) +
                    ",error=" + err
                );
                if (stmt) api.finalize(stmt);
            }
            if (db) api.close(db);
        }
    }
}

void Probe::write_summary() {
    logger_->line(
        "NATIVE_DB_PROBE_SUMMARY",
        "module_count=" + std::to_string(modules_.size()) +
        ",sqlite_api_modules=" + std::to_string(apis_.size()) +
        ",next_step=" + std::string(
            apis_.empty()
                ? "static_link_signature_scan_required"
                : "export_hook_or_existing_connection_capture_required"
        )
    );
}

}  // namespace dsw::dbprobe
