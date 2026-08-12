#pragma once

#include <cstddef>
#include <cstdint>

namespace dsnap {

class IncrementalDiscoveryState {
public:
    void begin(std::uint64_t epoch, std::int32_t upper_bound) noexcept {
        epoch_ = epoch; cursor_ = 0; upper_bound_ = upper_bound > 0 ? upper_bound : 0; active_ = upper_bound_ > 0;
    }
    [[nodiscard]] std::pair<std::int32_t, std::int32_t> next(std::int32_t batch) noexcept {
        if (!active_ || batch <= 0) return {0, 0};
        const auto end = (cursor_ + batch < upper_bound_) ? cursor_ + batch : upper_bound_;
        return {cursor_, end};
    }
    void commit(std::int32_t next_cursor) noexcept {
        if (!active_ || next_cursor < cursor_) return;
        cursor_ = next_cursor > upper_bound_ ? upper_bound_ : next_cursor;
        if (cursor_ >= upper_bound_) active_ = false;
    }
    void cancel() noexcept { active_ = false; cursor_ = 0; upper_bound_ = 0; }
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] std::uint64_t epoch() const noexcept { return epoch_; }
private:
    std::uint64_t epoch_{}; std::int32_t cursor_{}; std::int32_t upper_bound_{}; bool active_{};
};

} // namespace dsnap
