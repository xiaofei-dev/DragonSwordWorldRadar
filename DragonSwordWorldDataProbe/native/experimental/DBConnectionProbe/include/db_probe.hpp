#pragma once
#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dsw::dbprobe {

struct SqliteApi {
    using sqlite3 = void;
    using sqlite3_stmt = void;

    using OpenV2Fn = int(__cdecl*)(const char*, sqlite3**, int, const char*);
    using CloseFn = int(__cdecl*)(sqlite3*);
    using ExecCallback = int(__cdecl*)(void*, int, char**, char**);
    using ExecFn = int(__cdecl*)(sqlite3*, const char*, ExecCallback, void*, char**);
    using PrepareV2Fn = int(__cdecl*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
    using StepFn = int(__cdecl*)(sqlite3_stmt*);
    using FinalizeFn = int(__cdecl*)(sqlite3_stmt*);
    using ColumnCountFn = int(__cdecl*)(sqlite3_stmt*);
    using ColumnNameFn = const char*(__cdecl*)(sqlite3_stmt*, int);
    using ColumnTextFn = const unsigned char*(__cdecl*)(sqlite3_stmt*, int);
    using ErrMsgFn = const char*(__cdecl*)(sqlite3*);
    using DbFilenameFn = const char*(__cdecl*)(sqlite3*, const char*);
    using KeyFn = int(__cdecl*)(sqlite3*, const void*, int);
    using KeyV2Fn = int(__cdecl*)(sqlite3*, const char*, const void*, int);

    HMODULE module{};
    std::string module_name;
    OpenV2Fn open_v2{};
    CloseFn close{};
    ExecFn exec{};
    PrepareV2Fn prepare_v2{};
    StepFn step{};
    FinalizeFn finalize{};
    ColumnCountFn column_count{};
    ColumnNameFn column_name{};
    ColumnTextFn column_text{};
    ErrMsgFn errmsg{};
    DbFilenameFn db_filename{};
    KeyFn key{};
    KeyV2Fn key_v2{};

    [[nodiscard]] bool can_query() const noexcept;
};

class Logger {
public:
    explicit Logger(std::filesystem::path path);
    void line(const std::string& event, const std::string& details);
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
    std::mutex mutex_;
};

class Probe {
public:
    explicit Probe(HMODULE self);
    ~Probe();

    bool start();
    void stop();

private:
    void run();
    void enumerate_modules();
    void resolve_sqlite_apis();
    void inspect_candidate_modules();
    void try_read_existing_database_files();
    void write_summary();

    [[nodiscard]] std::filesystem::path module_directory() const;
    [[nodiscard]] std::filesystem::path runtime_log_directory() const;
    [[nodiscard]] std::vector<std::filesystem::path> candidate_save_files() const;

    HMODULE self_{};
    HANDLE stop_event_{};
    HANDLE worker_{};
    std::unique_ptr<Logger> logger_;
    std::vector<SqliteApi> apis_;
    std::vector<std::string> modules_;
};

}  // namespace dsw::dbprobe
