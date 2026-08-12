#pragma once

#include <dsnap/candidate_queue.hpp>
#include <dsnap/configuration.hpp>

#include <cstdint>
#include <optional>

namespace dsnap {

struct InteractionContract {
    bool positively_validated{};
    std::uint8_t replay_function{};

    [[nodiscard]] bool approved() const noexcept {
        return positively_validated && replay_function != 0;
    }
};

struct ActionRequest {
    WeakObjectId candidate{};
    std::uint64_t epoch{};
    std::uint8_t replay_function{};
};

class PickupController {
public:
    PickupController(Configuration configuration, InteractionContract contract);

    void set_build_trusted(bool trusted) noexcept;
    void reset_world();
    [[nodiscard]] bool observe_candidate(WeakObjectId id, std::string_view exact_class_path);
    [[nodiscard]] bool update_candidate(WeakObjectId id, const CandidateValidation& validation);
    void candidate_deleted(WeakObjectId id);
    [[nodiscard]] bool request_active(bool enabled) noexcept;
    [[nodiscard]] std::optional<ActionRequest> plan_action(MonotonicTime now);
    void record_action_result(WeakObjectId id, ActionResult result, MonotonicTime now);

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] bool build_trusted() const noexcept { return build_trusted_; }
    [[nodiscard]] std::uint64_t epoch() const noexcept { return epoch_; }
    [[nodiscard]] std::size_t queue_size() const noexcept { return queue_.size(); }

private:
    Configuration configuration_{};
    InteractionContract contract_{};
    CandidateQueue queue_;
    bool build_trusted_{};
    bool active_{};
    std::uint64_t epoch_{1};
    MonotonicTime next_global_action_{};
};

} // namespace dsnap
