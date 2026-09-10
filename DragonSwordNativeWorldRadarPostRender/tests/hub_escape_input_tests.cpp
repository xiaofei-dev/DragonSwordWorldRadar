#include <dswros/escape_input_model.hpp>
#include "hub_escape_input.hpp"
#include <Windows.h>

#include <cstdlib>
#include <iostream>

namespace {
unsigned checks{};
unsigned escape_down{};
unsigned escape_up{};
unsigned escape_char{};
unsigned other_key{};
void check(bool condition, const char* name) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << name << '\n';
        std::exit(1);
    }
}
LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM key, LPARAM data) {
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        if (key == VK_ESCAPE) ++escape_down;
        else ++other_key;
    } else if ((message == WM_KEYUP || message == WM_SYSKEYUP) && key == VK_ESCAPE) {
        ++escape_up;
    } else if ((message == WM_CHAR || message == WM_SYSCHAR) && key == VK_ESCAPE) {
        ++escape_char;
    }
    return DefWindowProcW(window, message, key, data);
}
void drain() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
void post(HWND window, UINT message, WPARAM key = VK_ESCAPE, LPARAM flags = 1) {
    check(PostMessageW(window, message, key, flags) != FALSE, "post to isolated window");
    drain();
}
} // namespace

int main() {
    using dswros::EscapeInputModel;
    using namespace dsnwr::hub_escape_input;
    EscapeInputModel state;
    check(!state.key(true) && !state.key(false), "closed panel passes Escape");
    state.open();
    check(state.key(true, false), "peek reports consumption");
    check(!state.press_owned() && !state.close_requested(), "peek does not advance state");
    check(state.key(true) && state.take_close_request(), "first down consumes and closes");
    check(state.key(true) && !state.take_close_request(), "repeat coalesces close request");
    state.close();
    check(state.key(true) && state.active(), "repeat still consumed after close");
    check(state.key(false, false) && state.press_owned(), "peek up retains press ownership");
    check(state.key(false) && !state.active(), "release consumed and ownership cleared");
    check(!state.key(true), "next independent press passes");
    state.open();
    check(state.key(false) && state.panel_open(), "orphan release never closes fresh panel");
    state.lose_focus();
    state.lose_focus();
    check(state.take_close_request() && !state.press_owned(), "repeated focus loss keeps one close notification");
    check(state.active(), "focus return before UI close retains panel consumption");
    state.close();
    check(!state.key(true), "focus-loss panel close releases routing");
    state.open();
    (void)state.key(true);
    state.reset();
    check(!state.active() && !state.take_close_request(), "travel/reset clears all state");

    using dswros::EscapeCloseReason;
    state.open();
    (void)state.key(true);
    check(state.take_close_reason() == EscapeCloseReason::Escape, "modal cancel can distinguish an Escape press");
    check(state.panel_open() && state.press_owned(), "taking modal Escape leaves input ownership intact");
    check(state.key(true) && state.take_close_reason() == EscapeCloseReason::None, "held Escape cannot cancel modal then close underlying settings");
    check(state.key(false) && state.take_close_reason() == EscapeCloseReason::None, "modal cancel consumes its release without a second action");
    (void)state.key(true);
    state.lose_focus();
    check(state.take_close_reason() == EscapeCloseReason::FocusLost, "focus loss overrides a queued modal Escape");
    state.lose_focus();
    (void)state.key(true);
    check(state.take_close_reason() == EscapeCloseReason::FocusLost, "quick focus return and Escape cannot downgrade forced close");
    check(state.take_close_reason() == EscapeCloseReason::None, "focus cleanup notification is consumed once");
    state.reset();
    check(!state.active() && state.take_close_reason() == EscapeCloseReason::None, "travel discards pending modal input reason");

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW window_class{};
    window_class.hInstance = instance;
    window_class.lpszClassName = L"DragonSwordEscapeInputIsolatedTest";
    window_class.lpfnWndProc = window_proc;
    check(RegisterClassW(&window_class) != 0, "register isolated test class");
    HWND window = CreateWindowExW(0, window_class.lpszClassName, L"", WS_OVERLAPPED,
        0, 0, 100, 100, nullptr, nullptr, instance, nullptr);
    HWND child = CreateWindowExW(0, window_class.lpszClassName, L"", WS_CHILD,
        0, 0, 10, 10, window, nullptr, instance, nullptr);
    HWND unrelated = CreateWindowExW(0, window_class.lpszClassName, L"", WS_OVERLAPPED,
        0, 0, 100, 100, nullptr, nullptr, instance, nullptr);
    check(window && child && unrelated, "create hidden local windows without focus/input injection");
    check(!open_test_window(GetDesktopWindow()), "reject foreign-process desktop window");
    check(!open_test_window(child), "reject child as top-level owner");
    check(open_test_window(window) && installed_for_test(), "install local thread message filter");
    post(window, WM_KEYDOWN, 'A');
    check(other_key == 1, "unrelated key reaches target WndProc");
    post(unrelated, WM_KEYDOWN);
    check(escape_down == 1 && !close_requested(), "other top-level window remains untouched");
    const unsigned prior_escape_char = escape_char;
    post(child, WM_SYSKEYDOWN);
    check(escape_down == 1 && take_close_request(), "child/popup target Escape consumed before WndProc");
    post(window, WM_SYSCHAR);
    check(escape_char == prior_escape_char, "Escape character does not reach target");
    panel_closed();
    check(installed_for_test(), "closing page keeps guard while press is held");
    post(window, WM_KEYDOWN, VK_ESCAPE, static_cast<LPARAM>(0x40000001));
    check(escape_down == 1 && !close_requested(), "post-close repeat cannot open game menu");
    post(window, WM_SYSKEYUP, VK_ESCAPE, static_cast<LPARAM>(0xC0000001));
    check(escape_up == 0, "matching keyup cannot reach game");
    service();
    check(!installed_for_test(), "release uninstalls idle hook");
    post(window, WM_KEYDOWN);
    post(window, WM_KEYUP);
    check(escape_down == 2 && escape_up == 1, "new press/release restored to target");

    check(open_test_window(window), "reopen installs fresh guard");
    check(PostMessageW(window, WM_KEYDOWN, VK_ESCAPE, 1) != FALSE, "queue peek test");
    MSG peek{};
    check(PeekMessageW(&peek, window, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE) != FALSE,
          "peek sees queued key");
    check(peek.message == WM_NULL && !close_requested(), "actual non-removing peek is consumed without close");
    drain();
    check(take_close_request() && escape_down == 2, "removing message creates one close request");
    reset();
    check(!installed_for_test() && !close_requested(), "teardown unhooks and clears pending input");
    check(open_test_window(window), "reopen for synchronous focus notification");
    SendMessageW(window, WM_KILLFOCUS, reinterpret_cast<WPARAM>(unrelated), 0);
    check(take_close_request(), "sent focus loss detected without another GetMessage or tick");
    post(window, WM_KEYDOWN);
    check(escape_down == 2, "rapid focus return stays protected until actual UI close");
    // A second focus loss clears this fresh gesture even if its release went
    // to another app. No real foreground or input state is changed by this test.
    SendMessageW(window, WM_ACTIVATEAPP, FALSE, 0);
    panel_closed();
    check(!installed_for_test(), "synchronous focus loss cannot retain a stale pressed tail");
    check(open_test_window(window), "reopen before owner destruction");
    DestroyWindow(window);
    service();
    check(take_close_request(), "destroyed owner requests safe panel cleanup");
    panel_closed();
    check(!installed_for_test(), "destroyed owner removes hook");
    DestroyWindow(unrelated);
    UnregisterClassW(window_class.lpszClassName, instance);
    std::cout << "Hub Escape input: " << checks << " checks passed; isolated Win32 message consumption verified.\n";
}
