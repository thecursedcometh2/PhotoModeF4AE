#include "ImGui/WindowProcedure.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    constexpr UINT probe = WM_APP + 77;
    struct Chain
    {
        WNDPROC beforePhoto{}, beforePeer{}, beforeTop{};
        bool wide{}, systemDefault{};
        unsigned originalCalls{}, peerCalls{}, topCalls{};
    };
    Chain* chain{};
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    LRESULT CALLBACK original(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == probe) { ++chain->originalCalls; return 1234; }
        return chain->wide ? DefWindowProcW(window, message, wParam, lParam) : DefWindowProcA(window, message, wParam, lParam);
    }
    LRESULT CALLBACK peer(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == probe) ++chain->peerCalls;
        return chain->wide ? CallWindowProcW(chain->beforePeer, window, message, wParam, lParam) :
                             CallWindowProcA(chain->beforePeer, window, message, wParam, lParam);
    }
    LRESULT CALLBACK photo(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        return ImGui::WindowProcedure::Forward(chain->beforePhoto, window, message, wParam, lParam);
    }
    LRESULT CALLBACK top(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == probe) ++chain->topCalls;
        return CallWindowProcA(chain->beforeTop, window, message, wParam, lParam);
    }
    void run(bool wide, bool systemDefault)
    {
        Chain state{};
        chain = &state;
        state.wide = wide;
        state.systemDefault = systemDefault;
        const auto instance = GetModuleHandleW(nullptr);
        constexpr wchar_t wideClass[] = L"PhotoModeForwardingWide";
        constexpr char ansiClass[] = "PhotoModeForwardingAnsi";
        if (wide) {
            WNDCLASSW cls{};
            cls.hInstance = instance; cls.lpszClassName = wideClass;
            cls.lpfnWndProc = systemDefault ? DefWindowProcW : original;
            require(RegisterClassW(&cls) != 0, "register Unicode class");
        } else {
            WNDCLASSA cls{};
            cls.hInstance = instance; cls.lpszClassName = ansiClass; cls.lpfnWndProc = original;
            require(RegisterClassA(&cls) != 0, "register ANSI class");
        }
        const auto window = wide ? CreateWindowExW(0, wideClass, L"Photo Mode forwarding", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr) :
                                   CreateWindowExA(0, ansiClass, "Photo Mode forwarding", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
        require(window != nullptr, "create hidden message window");
        state.beforePeer = reinterpret_cast<WNDPROC>(wide ?
            SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&peer)) :
            SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&peer)));
        require(state.beforePeer != nullptr, "install earlier peer");
        state.beforePhoto = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&photo)));
        require(state.beforePhoto != nullptr, "install Photo Mode subclass");
        state.beforeTop = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&top)));
        require(state.beforeTop != nullptr, "install later peer");
        std::array<char, 64> text{};
        const auto length = SendMessageA(window, WM_GETTEXT, text.size(), reinterpret_cast<LPARAM>(text.data()));
        require(length == 21 && std::string(text.data()) == "Photo Mode forwarding", "ANSI/Unicode text forwarding");
        require(SendMessageA(window, probe, 0, 0) == (systemDefault ? 0 : 1234), "forwarded return value");
        require(state.peerCalls == 1 && state.topCalls == 1 && state.originalCalls == (systemDefault ? 0u : 1u), "chain called once");
        require(ImGui::WindowProcedure::Forward(nullptr, window, probe, 0, 0) == 0, "null previous-procedure fallback");
        require(DestroyWindow(window) != 0, "destroy window");
        require((wide ? UnregisterClassW(wideClass, instance) : UnregisterClassA(ansiClass, instance)) != 0, "unregister class");
        chain = nullptr;
    }
}
int main()
{
    try {
        run(false, false);
        run(true, false);
        run(true, true);
        std::cout << "Window forwarding checks passed: ANSI, Unicode, system default procedure, earlier/later subclasses, text conversion, exactly-once dispatch, return value and null fallback\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
