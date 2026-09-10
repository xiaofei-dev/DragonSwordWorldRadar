#pragma once

#include <cstdint>
#include <dswros/escape_input_model.hpp>

namespace dsnwr::hub_escape_input {

// Only the foreground UnrealWindow in this process is eligible. All callback
// state is native scalar storage; this module never calls Unreal or retains UI.
[[nodiscard]] bool open() noexcept;
void panel_closed() noexcept;
void service() noexcept;
[[nodiscard]] bool close_requested() noexcept;
[[nodiscard]] bool take_close_request() noexcept;
[[nodiscard]] dswros::EscapeCloseReason take_close_reason() noexcept;
[[nodiscard]] std::uint32_t last_error() noexcept;
void reset() noexcept;
// Loader/process shutdown cannot safely call User32. Disable the callback's
// ingress only; the OS owns removal of this process-local hook at process exit.
void abandon_for_process_shutdown() noexcept;

#if defined(DSNWRPR_ESCAPE_INPUT_TEST)
[[nodiscard]] bool open_test_window(void* window) noexcept;
[[nodiscard]] bool installed_for_test() noexcept;
#endif

} // namespace dsnwr::hub_escape_input
