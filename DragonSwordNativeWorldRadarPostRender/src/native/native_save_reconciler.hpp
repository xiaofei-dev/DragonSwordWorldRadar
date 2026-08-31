#pragma once

#include <dswros/owner_pointer_pattern.hpp>

#include <atomic>
#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <thread>
#include <vector>

namespace dsnwr {

inline constexpr std::size_t kMaximumTreasureConfirmationIds = 64U;

struct OpenedTreasureField {
    std::int64_t category{};
    std::uint64_t bits{};
};

struct EncounterRespawnField {
    std::int64_t id{};
    std::int32_t respawn_type{};
    std::int64_t destroy_time_unix_seconds{};
};

struct DynamicQuestCompletionField {
    std::int64_t quest_id{};
    std::int64_t complete_count{};
};

enum class SaveReconcileError : std::uint32_t {
    None,
    NotInitialized,
    ConfigurationInvalid,
    GameImageMismatch,
    SaveKeyUnavailable,
    SaveSlotUnavailable,
    SnapshotCopyFailed,
    SqlCipherUnavailable,
    SqlCipherQueryFailed,
    NoReadableDatabase,
};

enum class SaveReconcileScope : std::uint8_t {
    FullActivation,
    CompletionConfirmation,
    TreasureConfirmation,
};

enum class SaveOwnerPointerRoute : std::uint8_t {
    None,
    PackagedConfig,
    RuntimePattern,
};

struct SaveReconcileResult {
    std::uint32_t activation{};
    std::uint32_t request_id{};
    SaveReconcileScope scope{SaveReconcileScope::FullActivation};
    std::uint32_t requested_treasure_count{};
    SaveReconcileError error{SaveReconcileError::NotInitialized};
    std::vector<OpenedTreasureField> opened_fields{};
    std::vector<EncounterRespawnField> encounter_respawns{};
    std::vector<DynamicQuestCompletionField> dynamic_quest_completions{};
    std::int64_t dynamic_quest_completion_user_dbid{};
    std::uint32_t opened_bit_count{};
    std::uint32_t database_count{};
    bool dynamic_quest_completion_query_available{};
    bool dynamic_quest_completion_identity_ambiguous{};
    SaveOwnerPointerRoute owner_pointer_route{SaveOwnerPointerRoute::None};
    dswros::OwnerPointerPatternStatus owner_pointer_pattern_status{
        dswros::OwnerPointerPatternStatus::InvalidImage};
    bool owner_pointer_pattern_attempted{};
    std::uint64_t owner_pointer_resolution_us{};
    std::uint64_t copy_elapsed_us{};
    std::uint64_t query_elapsed_us{};
    std::uint64_t total_elapsed_us{};

    [[nodiscard]] bool success() const noexcept {
        return error == SaveReconcileError::None;
    }
};

// Owns one below-normal process-lifetime worker. The engine thread may enqueue
// one initial full request and later event-driven encounter/task or exact
// treasure confirmations.
// At most one request may be pending, running, or awaiting collection. The
// worker performs only filesystem, self-process numeric-memory, and SQLCipher
// work; it never receives or retains an Unreal object.
class NativeSaveReconciler final {
public:
    NativeSaveReconciler() = default;
    NativeSaveReconciler(const NativeSaveReconciler&) = delete;
    NativeSaveReconciler& operator=(const NativeSaveReconciler&) = delete;
    ~NativeSaveReconciler();

    [[nodiscard]] bool initialize(
        std::filesystem::path mod_directory) noexcept;
    [[nodiscard]] bool request(
        std::uint32_t activation,
        std::uint32_t request_id,
        SaveReconcileScope scope,
        std::span<const std::int64_t> treasure_ids = {}) noexcept;
    [[nodiscard]] std::optional<SaveReconcileResult> try_take() noexcept;
    void shutdown() noexcept;

private:
    struct Request {
        std::uint32_t activation{};
        std::uint32_t request_id{};
        SaveReconcileScope scope{SaveReconcileScope::FullActivation};
        std::array<std::int64_t, kMaximumTreasureConfirmationIds>
            treasure_ids{};
        std::size_t treasure_id_count{};
    };

    void worker_loop(std::stop_token stop_token) noexcept;
    [[nodiscard]] SaveReconcileResult run_request(
        const Request& request) noexcept;

    std::filesystem::path mod_directory_{};
    std::mutex mutex_{};
    std::condition_variable_any wake_{};
    std::optional<Request> pending_{};
    std::optional<SaveReconcileResult> completed_{};
    std::jthread worker_{};
    std::atomic<bool> completed_ready_{};
    std::uint64_t cached_owner_pointer_executable_length_{};
    std::uint64_t cached_owner_pointer_rva_{};
    SaveOwnerPointerRoute cached_owner_pointer_route_{
        SaveOwnerPointerRoute::None};
    dswros::OwnerPointerPatternStatus owner_pointer_pattern_status_{
        dswros::OwnerPointerPatternStatus::InvalidImage};
    bool owner_pointer_pattern_attempted_{};
    bool initialized_{};
    bool request_active_{};
};

} // namespace dsnwr
