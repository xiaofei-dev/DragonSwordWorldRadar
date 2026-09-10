#include "hub_escape_input.hpp"

#include <dswros/escape_input_model.hpp>
#include <Windows.h>

#include <atomic>
#include <cwchar>

namespace dsnwr::hub_escape_input {
namespace {

SRWLOCK state_lock = SRWLOCK_INIT;
HHOOK hook{};
HHOOK lifecycle_hook{};
HWND owner_window{};
DWORD owner_thread{};
bool owner_destroyed{};
bool require_foreground{true};
dswros::EscapeInputModel model{};
std::atomic<bool> ingress_enabled{};
std::atomic<std::uint32_t> error{};

class Lock final {
public:
    Lock() noexcept { AcquireSRWLockExclusive(&state_lock); }
    ~Lock() { ReleaseSRWLockExclusive(&state_lock); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
};

[[nodiscard]] bool foreground_matches() noexcept {
    return !require_foreground
        || GetAncestor(GetForegroundWindow(), GA_ROOT) == owner_window;
}

LRESULT CALLBACK filter_message(int code, WPARAM removal, LPARAM payload) {
    if (code >= 0 && payload && ingress_enabled.load(std::memory_order_acquire)) {
        auto* message = reinterpret_cast<MSG*>(payload);
        Lock lock;
        if (hook && GetCurrentThreadId() == owner_thread) {
            if (owner_destroyed || !foreground_matches()) {
                model.lose_focus();
            } else if (message->hwnd == owner_window
                       || IsChild(owner_window, message->hwnd)) {
                const bool down = message->message == WM_KEYDOWN
                    || message->message == WM_SYSKEYDOWN;
                const bool up = message->message == WM_KEYUP
                    || message->message == WM_SYSKEYUP;
                bool consume{};
                if ((down || up) && message->wParam == VK_ESCAPE) {
                    consume = model.key(down, removal == PM_REMOVE);
                } else if ((message->message == WM_CHAR
                            || message->message == WM_SYSCHAR
                            || message->message == WM_UNICHAR)
                           && message->wParam == VK_ESCAPE) {
                    consume = model.active();
                }
                if (consume) {
                    // WH_GETMESSAGE runs before TranslateMessage/DispatchMessage.
                    // Returning a hook result alone would NOT consume a message.
                    message->message = WM_NULL;
                    message->wParam = 0;
                    message->lParam = 0;
                }
            }
        }
    }
    return CallNextHookEx(nullptr, code, removal, payload);
}

LRESULT CALLBACK observe_window_lifecycle(int code, WPARAM sent, LPARAM payload) {
    if (code >= 0 && payload && ingress_enabled.load(std::memory_order_acquire)) {
        const auto* message = reinterpret_cast<const CWPSTRUCT*>(payload);
        Lock lock;
        if (lifecycle_hook && GetCurrentThreadId() == owner_thread
            && message->hwnd == owner_window) {
            const bool destroyed = message->message == WM_NCDESTROY;
            const bool lost_focus = message->message == WM_KILLFOCUS
                || (message->message == WM_ACTIVATEAPP && !message->wParam)
                || (message->message == WM_ACTIVATE
                    && LOWORD(message->wParam) == WA_INACTIVE);
            if (destroyed || lost_focus) {
                owner_destroyed = owner_destroyed || destroyed;
                model.lose_focus();
            }
        }
    }
    return CallNextHookEx(nullptr, code, sent, payload);
}

[[nodiscard]] bool attach(HWND window, bool check_foreground) noexcept {
    DWORD process{};
    const DWORD thread = GetWindowThreadProcessId(window, &process);
    if (!window || !IsWindow(window) || !thread || process != GetCurrentProcessId()
        || GetAncestor(window, GA_ROOT) != window) {
        error.store(ERROR_INVALID_WINDOW_HANDLE, std::memory_order_release);
        return false;
    }
    if (check_foreground) {
        wchar_t class_name[64]{};
        if (GetForegroundWindow() != window
            || !GetClassNameW(window, class_name, 64)
            || std::wcscmp(class_name, L"UnrealWindow") != 0) {
            error.store(ERROR_INVALID_WINDOW_HANDLE, std::memory_order_release);
            return false;
        }
    }
    reset();
    // A nonzero thread id limits this hook to one thread of this process.
    // No DLL injection and no global/low-level keyboard hook are used.
    HHOOK installed = SetWindowsHookExW(WH_GETMESSAGE, filter_message, nullptr, thread);
    if (!installed) {
        error.store(GetLastError(), std::memory_order_release);
        return false;
    }
    // Focus and destruction notifications are sent synchronously and do not
    // necessarily pass WH_GETMESSAGE. Observe them on the same local thread so
    // a fast Alt-Tab/release/return between ticks cannot retain a stale gesture.
    HHOOK lifecycle = SetWindowsHookExW(
        WH_CALLWNDPROC, observe_window_lifecycle, nullptr, thread);
    if (!lifecycle) {
        error.store(GetLastError(), std::memory_order_release);
        UnhookWindowsHookEx(installed);
        return false;
    }
    Lock lock;
    owner_window = window;
    owner_thread = thread;
    owner_destroyed = false;
    require_foreground = check_foreground;
    hook = installed;
    lifecycle_hook = lifecycle;
    model.open();
    error.store(0, std::memory_order_release);
    ingress_enabled.store(true, std::memory_order_release);
    return true;
}

} // namespace

bool open() noexcept { return attach(GetForegroundWindow(), true); }

void panel_closed() noexcept {
    {
        Lock lock;
        model.close();
    }
    service();
}

void service() noexcept {
    if (!ingress_enabled.load(std::memory_order_acquire)) return;
    bool remove{};
    {
        Lock lock;
        if (!hook) return;
        DWORD process{};
        if (owner_destroyed || !IsWindow(owner_window)
            || GetWindowThreadProcessId(owner_window, &process) != owner_thread
            || process != GetCurrentProcessId() || !foreground_matches()) {
            model.lose_focus();
        }
        // Keep the notification until the game-thread UI service takes it.
        remove = !model.active() && !model.close_requested();
    }
    if (remove) reset();
}

bool close_requested() noexcept {
    if (!ingress_enabled.load(std::memory_order_acquire)) return false;
    Lock lock;
    return model.close_requested();
}

bool take_close_request() noexcept {
    if (!ingress_enabled.load(std::memory_order_acquire)) return false;
    Lock lock;
    return model.take_close_request();
}

dswros::EscapeCloseReason take_close_reason() noexcept {
    if (!ingress_enabled.load(std::memory_order_acquire)) return dswros::EscapeCloseReason::None;
    Lock lock;
    return model.take_close_reason();
}

std::uint32_t last_error() noexcept { return error.load(std::memory_order_acquire); }

void reset() noexcept {
    ingress_enabled.store(false, std::memory_order_release);
    HHOOK removed{};
    HHOOK removed_lifecycle{};
    {
        Lock lock;
        removed = hook;
        removed_lifecycle = lifecycle_hook;
        hook = nullptr;
        lifecycle_hook = nullptr;
        owner_window = nullptr;
        owner_thread = 0;
        owner_destroyed = false;
        model.reset();
    }
    if (removed) UnhookWindowsHookEx(removed);
    if (removed_lifecycle) UnhookWindowsHookEx(removed_lifecycle);
}

void abandon_for_process_shutdown() noexcept {
    ingress_enabled.store(false, std::memory_order_release);
}

#if defined(DSNWRPR_ESCAPE_INPUT_TEST)
bool open_test_window(void* window) noexcept {
    return attach(static_cast<HWND>(window), false);
}
bool installed_for_test() noexcept {
    Lock lock;
    return hook != nullptr;
}
#endif

} // namespace dsnwr::hub_escape_input
