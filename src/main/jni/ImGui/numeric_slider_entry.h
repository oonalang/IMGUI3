#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "../System/UI/Keyboard.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unordered_map>

namespace numeric_slider_entry {
    struct State {
        bool editing = false;
        bool invalid = false;
        bool keyboard = false;
        bool just_opened = false;
        float opacity = 0.0f;
        char buffer[48] = {};
    };

    inline std::unordered_map<ImGuiID, State>& Fields() {
        static std::unordered_map<ImGuiID, State> fields;
        return fields;
    }

    inline ImGuiID& OpenId() {
        static ImGuiID open = 0;
        return open;
    }

    inline State& Get(ImGuiID id) { return Fields()[id]; }

    inline bool Active(ImGuiID id) { return Get(id).editing; }

    inline void Close(State& s) {
        s.editing = false;
        s.invalid = false;
        s.keyboard = false;
    }

    inline bool Blank(const char* text) {
        if (text == nullptr) return true;
        for (const char* c = text; *c; ++c) {
            if (!std::isspace((unsigned char)*c)) return false;
        }
        return true;
    }

    inline void Sanitize(char* buffer, size_t capacity, bool integer) {
        if (buffer == nullptr || capacity < 2) return;
        size_t write = 0;
        bool seen_dot = false;
        for (size_t read = 0; buffer[read] != '\0' && write + 1 < capacity; ++read) {
            const char c = buffer[read];
            if (c >= '0' && c <= '9') {
                buffer[write++] = c;
            } else if (c == '-' && write == 0) {
                buffer[write++] = c;
            } else if (c == '.' && !integer && !seen_dot) {
                buffer[write++] = c;
                seen_dot = true;
            }
        }
        buffer[write] = '\0';
    }

    inline bool Open(ImGuiID id, double value, bool integer) {
        const ImGuiID previous = OpenId();
        if (previous != 0 && previous != id) Close(Get(previous));
        OpenId() = id;

        State& s = Get(id);
        s.editing = true;
        s.invalid = false;
        s.keyboard = true;
        s.just_opened = true;
        s.opacity = 0.0f;
        if (integer) std::snprintf(s.buffer, sizeof(s.buffer), "%.0f", value);
        else std::snprintf(s.buffer, sizeof(s.buffer), "%.6g", value);
        return true;
    }

    inline bool TapTracker(ImGuiID id, bool pressed) {
        static std::unordered_map<ImGuiID, double> last_tap;
        if (!pressed) return false;
        const double now = (double)ImGui::GetTime();
        double& last = last_tap[id];
        const bool second = last > 0.0 && (now - last) <= 0.5;
        last = second ? 0.0 : now;
        return second;
    }

    inline void OpenOnDoubleTap(ImGuiID id, bool pressed, double value, bool integer) {
        State& s = Get(id);
        if (s.editing) return;
        if (!TapTracker(id, pressed)) return;
        Open(id, value, integer);
    }

    inline bool ParseValue(const char* text, double min, double max,
                           bool integer, double& result) {
        if (Blank(text)) return false;
        char* end = nullptr;
        const double parsed = std::strtod(text, &end);
        if (end == text || end == nullptr) return false;
        while (*end && std::isspace((unsigned char)*end)) ++end;
        if (*end || !std::isfinite(parsed)) return false;
        const double bounded = ImClamp(parsed, ImMin(min, max), ImMax(min, max));
        result = integer ? std::round(bounded) : bounded;
        return true;
    }

    inline bool Render(ImGuiID id, ImVec2 anchor, ImVec2 size,
                       double& value, double min, double max, bool integer) {
        State& s = Get(id);
        if (!s.editing) return false;

        ImGuiIO& io = ImGui::GetIO();
        s.opacity = ImMin(1.0f, s.opacity + ImMax(0.0f, io.DeltaTime) * 16.0f);

        const float height = ImMax(34.0f, size.y);
        const float width = ImClamp(size.x * 1.9f, 190.0f,
                                    ImMin(ImMax(190.0f, io.DisplaySize.x - 24.0f), 460.0f));
        const float apply_w = 40.0f;
        const float field_w = width - apply_w - 10.0f;
        const float sheet_y = ImMax(8.0f,
            io.DisplaySize.y - VirtualKeyboardHeight(io.DisplaySize.y) - height - 10.0f);
        const float sheet_x = ImClamp(anchor.x, 8.0f,
            ImMax(8.0f, io.DisplaySize.x - width - 8.0f));
        const ImVec2 sheet_pos(sheet_x, sheet_y);
        const ImVec2 sheet_max(sheet_x + width, sheet_y + height);

        ImGui::PushID((int)id);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s.opacity);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 24, 46, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, s.invalid ? IM_COL32(245, 112, 144, 255)
                                                         : IM_COL32(118, 94, 186, 190));
        ImGui::SetNextWindowPos(sheet_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
        ImGui::SetNextWindowFocus();
        ImGui::Begin("##numeric_entry_sheet", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav);

        const ImVec2 field_min = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##field", ImVec2(ImMax(24.0f, field_w), ImMax(20.0f, height - 12.0f)));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) s.keyboard = true;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(field_min, ImVec2(field_min.x + field_w, field_min.y + height - 12.0f), true);
        dl->AddText(ImVec2(field_min.x + 2.0f,
                           field_min.y + (height - 12.0f - ImGui::GetFontSize()) * 0.5f),
                    IM_COL32(236, 232, 248, 255), s.buffer);
        dl->PopClipRect();

        ImGui::SetCursorScreenPos(ImVec2(sheet_max.x - apply_w - 6.0f, field_min.y));
        const bool confirm = ImGui::Button("OK##apply", ImVec2(apply_w, ImMax(20.0f, height - 12.0f)));

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        RenderVirtualKeyboard("##numeric_entry_keyboard", s.buffer, sizeof(s.buffer), &s.keyboard);
        Sanitize(s.buffer, sizeof(s.buffer), integer);

        bool changed = false;
        const bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape);
        if (confirm || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            double next = value;
            if (Blank(s.buffer)) {
                Close(s);
            } else if (ParseValue(s.buffer, min, max, integer, next)) {
                changed = value != next;
                value = next;
                Close(s);
            } else {
                s.invalid = true;
            }
        }
        if (cancel) Close(s);

        if (s.editing && s.just_opened) {
            s.just_opened = false;
        } else if (s.editing && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2& mouse = io.MousePos;
            const bool inside_sheet = mouse.x >= sheet_pos.x - 2.0f && mouse.x <= sheet_max.x + 2.0f &&
                                      mouse.y >= sheet_pos.y - 2.0f && mouse.y <= sheet_max.y + 2.0f;
            const bool over_keyboard =
                mouse.y >= io.DisplaySize.y - VirtualKeyboardHeight(io.DisplaySize.y);
            if (!inside_sheet && !over_keyboard) Close(s);
        }

        if (!s.editing) OpenId() = 0;
        ImGui::PopID();
        return changed;
    }
}
