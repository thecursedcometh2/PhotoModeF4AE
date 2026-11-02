#pragma once
#include <imgui.h>
namespace ImGui {
// Widget identity follows its bound state, independently of translated labels.
class ScopedWidgetID {
public:
    explicit ScopedWidgetID(const void* state) { PushID(state); }
    ~ScopedWidgetID() { PopID(); }
    ScopedWidgetID(const ScopedWidgetID&) = delete;
    ScopedWidgetID& operator=(const ScopedWidgetID&) = delete;
};
}
