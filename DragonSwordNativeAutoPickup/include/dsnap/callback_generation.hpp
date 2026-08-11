#pragma once

#include <atomic>
#include <cstdint>

namespace dsnap {

class CallbackGenerationGate {
public:
    explicit CallbackGenerationGate(std::uint64_t generation) noexcept : generation_(generation) {}

    [[nodiscard]] std::uint64_t generation() const noexcept { return generation_; }

    [[nodiscard]] bool accepts(std::uint64_t callback_generation) const noexcept {
        return alive_.load(std::memory_order_acquire) && generation_ != 0 && callback_generation == generation_;
    }

    void invalidate() noexcept { alive_.store(false, std::memory_order_release); }

private:
    std::uint64_t generation_{};
    std::atomic<bool> alive_{true};
};

} // namespace dsnap
