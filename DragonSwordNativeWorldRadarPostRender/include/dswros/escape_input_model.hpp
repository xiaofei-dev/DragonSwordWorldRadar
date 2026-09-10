#pragma once

namespace dswros {

enum class EscapeCloseReason { None, Escape, FocusLost };

// One physical Escape gesture belongs to the panel until its release, even
// when the panel has already disappeared. Peeking never advances this state.
class EscapeInputModel final {
public:
    void open() noexcept { panel_open_ = true; close_reason_ = EscapeCloseReason::None; }
    void close() noexcept { panel_open_ = false; close_reason_ = EscapeCloseReason::None; }
    void reset() noexcept { *this = {}; }
    void lose_focus() noexcept {
        if (panel_open_ || close_requested()) close_reason_ = EscapeCloseReason::FocusLost;
        // Forget the physical gesture, but retain panel ownership until the
        // game thread actually removes UMG. A quick focus-return must not let
        // Escape leak through the still-visible page before that close runs.
        press_owned_ = false;
    }
    [[nodiscard]] bool key(bool down, bool remove = true) noexcept {
        const bool consume = panel_open_ || press_owned_;
        if (!consume || !remove) return consume;
        if (down) {
            if (!press_owned_ && close_reason_ != EscapeCloseReason::FocusLost)
                close_reason_ = EscapeCloseReason::Escape;
            press_owned_ = true;
        } else {
            press_owned_ = false;
        }
        return true;
    }
    [[nodiscard]] bool active() const noexcept { return panel_open_ || press_owned_; }
    [[nodiscard]] bool panel_open() const noexcept { return panel_open_; }
    [[nodiscard]] bool press_owned() const noexcept { return press_owned_; }
    [[nodiscard]] bool close_requested() const noexcept { return close_reason_ != EscapeCloseReason::None; }
    [[nodiscard]] EscapeCloseReason take_close_reason() noexcept {
        const auto reason = close_reason_;
        close_reason_ = EscapeCloseReason::None;
        return reason;
    }
    [[nodiscard]] bool take_close_request() noexcept {
        return take_close_reason() != EscapeCloseReason::None;
    }

private:
    bool panel_open_{};
    bool press_owned_{};
    EscapeCloseReason close_reason_{EscapeCloseReason::None};
};

} // namespace dswros
