#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <unordered_map>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

namespace numeric_slider_entry {
    struct State {
        bool editing = false;
        bool focus = false;
        bool invalid = false;
        float opacity = 0.0f;
        char buffer[48] = {};
    };

    inline State& Get(ImGuiID id) {
        static std::unordered_map<ImGuiID, State> fields;
        return fields[id];
    }

    inline bool Active(ImGuiID id) { return Get(id).editing; }

    inline void OpenOnDoubleTap(ImGuiID id, bool tapped, double value, bool integer) {
        State& s = Get(id);
        if (!tapped || s.editing) return;
        s.editing = true;
        s.focus = true;
        s.invalid = false;
        s.opacity = 0.0f;
        if (integer) std::snprintf(s.buffer, sizeof(s.buffer), "%.0f", value);
        else std::snprintf(s.buffer, sizeof(s.buffer), "%.6g", value);
    }

    inline bool ParseValue(const char* text, double min, double max,
                           bool integer, double& result) {
        if (!text) return false;
        char* end = nullptr;
        const double parsed = std::strtod(text, &end);
        while (end && *end && std::isspace((unsigned char)*end)) ++end;
        if (end == text || !end || *end || !std::isfinite(parsed)) return false;
        const double bounded = ImClamp(parsed, ImMin(min, max), ImMax(min, max));
        result = integer ? std::round(bounded) : bounded;
        return true;
    }

    inline bool Render(ImGuiID id, ImVec2 pos, ImVec2 size,
                       double& value, double min, double max, bool integer) {
        State& s = Get(id);
        if (!s.editing) return false;

        s.opacity += (1.0f - s.opacity) * ImClamp(ImGui::GetIO().DeltaTime * 18.0f, 0.0f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s.opacity);
        const float height = ImMax(25.0f, size.y);
        const float arrow = ImMin(30.0f, size.x * 0.30f);
        ImGui::PushID((int)id);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + height),
                          IM_COL32(25, 20, 39, 255), 6.0f);
        if (s.invalid)
            dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + height),
                        IM_COL32(245, 112, 144, 255), 6.0f);

        ImGui::SetCursorScreenPos(pos);
        ImGui::SetNextItemWidth(ImMax(28.0f, size.x - arrow - 3.0f));
        if (s.focus) ImGui::SetKeyboardFocusHere();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(35, 28, 52, 255));
        const bool enter = ImGui::InputText("##number", s.buffer, sizeof(s.buffer),
            ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll);
        s.focus = false;
        ImGui::SameLine(0.0f, 3.0f);
        const bool confirm = ImGui::Button(">##apply", ImVec2(arrow, height));
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        bool changed = false;
        if (enter || confirm) {
            double next = value;
            if (ParseValue(s.buffer, min, max, integer, next)) {
                changed = value != next;
                value = next;
                s.editing = false;
                s.invalid = false;
            } else {
                s.invalid = true;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            s.editing = false;
            s.invalid = false;
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        return changed;
    }
}
