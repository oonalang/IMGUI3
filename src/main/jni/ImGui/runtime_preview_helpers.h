#pragma once

#include "custom_widgets.hpp"
#include "imgui_settings.h"

struct ChildFrame {
    ImVec2 min;
    ImVec2 size;
    float rounding;
};

inline ChildFrame BeginContentChild(const char *id, const ImVec2 &size, ImGuiWindowFlags flags = 0) {
    ChildFrame frame{};
    frame.min = ImGui::GetCursorScreenPos();
    frame.size = size;
    frame.rounding = c::child::rounding * c::scale;
    custom::Child(id, size, true, flags);
    return frame;
}

inline void EndContentChild(const ChildFrame &frame) {
    (void)frame;
    custom::EndChild();
}

inline void ContentGap(float height = 10.0f) {
    ImGui::Dummy(ImVec2(0.0f, height));
}
