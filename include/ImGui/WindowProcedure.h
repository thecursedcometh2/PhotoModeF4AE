#pragma once
#include <windows.h>

namespace ImGui::WindowProcedure
{
    inline LRESULT Forward(WNDPROC previous, HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // Windows can return an opaque procedure token, not a callable address.
        return previous ? CallWindowProcA(previous, window, message, wParam, lParam) :
                          DefWindowProcA(window, message, wParam, lParam);
    }
}
