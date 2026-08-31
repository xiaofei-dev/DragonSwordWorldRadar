#pragma once

#include <atomic>
#include <cstdint>
#include <limits>

namespace dsnap {

// A second, explicitly testable physical-key edge guard around the UE4SS
// keydown callback. Keydown may be produced on the UE4SS update thread while
// release is observed from on_update, so the latch itself must be atomic.
class AtomicPhysicalKeyEdge {
public:
    [[nodiscard]] bool key_down() noexcept {
        bool expected{};
        return down_.compare_exchange_strong(expected, true,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire);
    }

    [[nodiscard]] bool latched() const noexcept {
        return down_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool key_up() noexcept {
        return down_.exchange(false, std::memory_order_acq_rel);
    }

private:
    std::atomic<bool> down_{};
};

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

struct SessionEventBatch {
    std::uint32_t generation{};
    std::uint32_t count{};
};

// Carries key events across the UE4SS update thread and game thread without
// allowing a request captured before a World reset to become actionable after
// that reset. The reset publishes a new generation before clearing both event
// stages, so an overlapping producer either loses the race to the clear or is
// rejected by the generation check at the next stage.
class SessionToggleEventGate {
public:
    SessionToggleEventGate() noexcept {
        const auto initial = pack(kInitialGeneration, 0);
        key_events_.store(initial, std::memory_order_relaxed);
        toggle_requests_.store(initial, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint32_t current_generation() const noexcept {
        return generation_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool mark_playable(std::uint32_t generation) noexcept {
        if (!is_current(generation)) return false;
        playable_generation_.store(generation, std::memory_order_release);
        if (is_current(generation)) return true;
        auto expected = generation;
        playable_generation_.compare_exchange_strong(expected, 0,
                                                      std::memory_order_acq_rel,
                                                      std::memory_order_acquire);
        return false;
    }

    [[nodiscard]] bool record_key_event() noexcept {
        const auto generation = current_generation();
        if (playable_generation_.load(std::memory_order_acquire) != generation) return false;
        return enqueue_for_generation(key_events_, generation);
    }

    [[nodiscard]] SessionEventBatch drain_key_events() noexcept {
        return drain_current_generation(key_events_);
    }

    [[nodiscard]] bool publish_toggle_request(std::uint32_t source_generation) noexcept {
        return enqueue_for_generation(toggle_requests_, source_generation);
    }

    [[nodiscard]] SessionEventBatch drain_toggle_requests() noexcept {
        return drain_current_generation(toggle_requests_);
    }

    [[nodiscard]] bool is_current(std::uint32_t generation) const noexcept {
        return generation != 0 && generation_.load(std::memory_order_acquire) == generation;
    }

    [[nodiscard]] std::uint32_t reset() noexcept {
        auto next = generation_.fetch_add(1, std::memory_order_acq_rel) + 1;
        playable_generation_.store(0, std::memory_order_release);
        // A zero generation is reserved for an invalid/default batch. Reaching
        // this branch would require more than four billion World resets.
        if (next == 0) {
            generation_.store(kInitialGeneration, std::memory_order_release);
            next = kInitialGeneration;
        }
        const auto empty = pack(next, 0);
        key_events_.store(empty, std::memory_order_release);
        toggle_requests_.store(empty, std::memory_order_release);
        return next;
    }

private:
    static constexpr std::uint32_t kInitialGeneration = 1;
    static constexpr std::uint64_t kCountMask = 0xFFFFFFFFULL;

    [[nodiscard]] static constexpr std::uint64_t pack(std::uint32_t generation,
                                                       std::uint32_t count) noexcept {
        return (static_cast<std::uint64_t>(generation) << 32U) | count;
    }

    [[nodiscard]] static constexpr SessionEventBatch unpack(std::uint64_t value) noexcept {
        return SessionEventBatch{
            static_cast<std::uint32_t>(value >> 32U),
            static_cast<std::uint32_t>(value & kCountMask),
        };
    }

    [[nodiscard]] bool enqueue_for_generation(std::atomic<std::uint64_t>& state,
                                              std::uint32_t generation) noexcept {
        if (!is_current(generation)) return false;
        auto observed = state.load(std::memory_order_acquire);
        for (;;) {
            if (!is_current(generation)) return false;
            const auto batch = unpack(observed);
            const auto count = batch.generation == generation ? batch.count : 0;
            constexpr auto maximum = std::numeric_limits<std::uint32_t>::max();
            const auto incremented = count == maximum ? maximum : count + 1;
            const auto desired = pack(generation, incremented);
            if (state.compare_exchange_weak(observed, desired,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
                // If reset advanced the generation between the pre-CAS check
                // and publication, its subsequent clear owns the final state.
                return is_current(generation);
            }
        }
    }

    [[nodiscard]] SessionEventBatch drain_current_generation(
        std::atomic<std::uint64_t>& state) noexcept {
        for (;;) {
            const auto generation = generation_.load(std::memory_order_acquire);
            auto observed = state.load(std::memory_order_acquire);
            const auto desired = pack(generation, 0);
            if (!state.compare_exchange_weak(observed, desired,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire)) {
                continue;
            }
            const auto batch = unpack(observed);
            if (!is_current(generation) || batch.generation != generation) {
                return SessionEventBatch{generation, 0};
            }
            return batch;
        }
    }

    std::atomic<std::uint32_t> generation_{kInitialGeneration};
    std::atomic<std::uint32_t> playable_generation_{};
    std::atomic<std::uint64_t> key_events_{};
    std::atomic<std::uint64_t> toggle_requests_{};
};

} // namespace dsnap
