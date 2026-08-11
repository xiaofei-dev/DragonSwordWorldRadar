#pragma once

#include <cstdint>

namespace dsnap {

class SessionCalibration {
public:
    [[nodiscard]] bool observe(std::int32_t object_index, bool exact_manual_match) noexcept {
        if (accepted_ || !exact_manual_match || object_index < 0) {
            return false;
        }
        if (first_object_index_ < 0) {
            first_object_index_ = object_index;
            matches_ = 1;
            return false;
        }
        if (object_index == first_object_index_) {
            return false;
        }
        matches_ = 2;
        accepted_ = true;
        return true;
    }

    [[nodiscard]] bool accepted() const noexcept { return accepted_; }
    [[nodiscard]] std::uint8_t matches() const noexcept { return matches_; }

private:
    std::int32_t first_object_index_{-1};
    std::uint8_t matches_{};
    bool accepted_{};
};

} // namespace dsnap
