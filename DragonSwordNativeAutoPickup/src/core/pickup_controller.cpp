#include <dsnap/pickup_controller.hpp>

namespace dsnap {

PickupController::PickupController(Configuration configuration, InteractionContract contract)
    : configuration_(std::move(configuration)),
      contract_(contract),
      queue_(configuration_.max_queue,
             configuration_.max_retries,
             std::chrono::milliseconds{configuration_.retry_backoff_ms}) {
    queue_.reset(epoch_);
}

void PickupController::set_build_trusted(bool trusted) noexcept {
    build_trusted_ = trusted;
    if (!trusted) {
        active_ = false;
    }
}

void PickupController::reset_world() {
    active_ = false;
    ++epoch_;
    queue_.reset(epoch_);
    next_global_action_ = {};
}

bool PickupController::observe_candidate(WeakObjectId id, std::string_view exact_class_path) {
    return queue_.register_candidate(id, exact_class_path, epoch_);
}

bool PickupController::update_candidate(WeakObjectId id, const CandidateValidation& validation) {
    return queue_.update_validation(id, validation);
}

void PickupController::candidate_deleted(WeakObjectId id) {
    queue_.erase(id);
}

bool PickupController::request_active(bool enabled) noexcept {
    if (!enabled) {
        active_ = false;
        return true;
    }
    active_ = build_trusted_ && contract_.approved();
    return active_;
}

std::optional<ActionRequest> PickupController::plan_action(MonotonicTime now) {
    if (!active_ || !build_trusted_ || !contract_.approved() || now < next_global_action_) {
        return std::nullopt;
    }
    const auto candidate = queue_.next_eligible(now, epoch_, configuration_.radius_meters);
    if (!candidate) {
        return std::nullopt;
    }
    next_global_action_ = now + std::chrono::milliseconds{configuration_.action_interval_ms};
    return ActionRequest{.candidate = candidate->id, .epoch = epoch_, .replay_function = contract_.replay_function};
}

void PickupController::record_action_result(WeakObjectId id, ActionResult result, MonotonicTime now) {
    queue_.record_result(id, result, now);
}

} // namespace dsnap
