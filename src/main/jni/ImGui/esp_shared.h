#pragma once

// One implementation of the ESP box and the health bar.
//
// The in-game ESP (Hacks/Feature/DrawESP.h) and the Visuals-page preview
// (ImGui/esp_preview.h) both draw these, and they used to be two separate
// copies that had already drifted: the preview drew half-height health bars
// filled from the top with a flat colour, while the game drew a full-height
// bar filled from the bottom with a red-to-green ramp. The preview therefore
// showed something the game never draws. Everything that has a shape lives
// here now, so a change to one is a change to both.
//
// EspBoxType / EspHealthPosition are defined in Hacks/Feature/Init_Hacks.h;
// the values below mirror them so this header stays free of game headers and
// can be compiled on its own by the host harness.

#include "imgui.h"

namespace esp_shared
{
    // Mirrors EspBoxType { Fill = 0, Outline = 1, Corner = 2, ThreeD = 3 }.
    enum BoxStyle
    {
        BoxFill    = 0,
        BoxOutline = 1,
        BoxCorner  = 2,
        BoxThreeD  = 3
    };

    // Mirrors EspHealthPosition { HealthTop = 0, HealthSide = 1 }.
    enum HealthStyle
    {
        HealthTop  = 0,
        HealthSide = 1
    };

    // Mirrors LineTarget { Top = 0, Center = 1, Bottom = 2 }.
    enum LineStyle
    {
        LineTop    = 0,
        LineCenter = 1,
        LineBottom = 2
    };

    // The game's own ramp: full health is green, empty is red, linear between.
    inline ImU32 HealthColor(int curHP, int maxHP)
    {
        if (maxHP <= 0) return IM_COL32(0, 255, 0, 255);
        const float ratio = ImClamp(curHP / (float)maxHP, 0.0f, 1.0f);
        const int r = (int)ImMin(510.0f * (1.0f - ratio), 255.0f);
        const int g = (int)ImMin(510.0f * ratio, 255.0f);
        return IM_COL32(r, g, 0, 255);
    }

    // The side bar: a 4 px track pinned to the left of the box, the FULL height
    // of the box, filled from the bottom, black track plus a 1 px outline.
    inline void SideHealthBar(ImDrawList* draw, float x, float y, float h,
                              int curHP, int maxHP)
    {
        if (draw == nullptr || h <= 0.0f || maxHP <= 0) return;

        const float ratio = ImClamp(curHP / (float)maxHP, 0.0f, 1.0f);
        constexpr float barW = 4.0f;

        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 120));
        draw->AddRect(ImVec2(x, y), ImVec2(x + barW, y + h), IM_COL32(0, 0, 0, 220),
                      0.0f, 0, 1.0f);
        draw->AddRectFilled(ImVec2(x, y + h * (1.0f - ratio)), ImVec2(x + barW, y + h),
                            HealthColor(curHP, maxHP));
    }

    // The top bar: a thin strip under the name, inset from the container.
    inline void TopHealthBar(ImDrawList* draw, float x, float y, float w, float h,
                             int curHP, int maxHP)
    {
        if (draw == nullptr || w <= 0.0f || h <= 0.0f || maxHP <= 0) return;

        const float ratio = ImClamp(curHP / (float)maxHP, 0.0f, 1.0f);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 120));
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w * ratio, y + h), HealthColor(curHP, maxHP));
        draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
    }

    inline void DrawOutline(ImDrawList* draw, float x, float y, float w, float h,
                            ImU32 color, float thickness)
    {
        draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
    }

    inline void DrawFilled(ImDrawList* draw, float x, float y, float w, float h,
                           ImU32 color, float thickness)
    {
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.16f)));
        draw->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
    }

    inline void DrawCorner(ImDrawList* draw, float x, float y, float w, float h,
                           ImU32 color, float thickness)
    {
        const float iw = w / 4.0f;
        const float ih = h / 4.0f;
        draw->AddLine(ImVec2(x, y), ImVec2(x + iw, y), color, thickness);
        draw->AddLine(ImVec2(x + w - iw, y), ImVec2(x + w, y), color, thickness);
        draw->AddLine(ImVec2(x, y), ImVec2(x, y + ih), color, thickness);
        draw->AddLine(ImVec2(x + w - 1.0f, y), ImVec2(x + w - 1.0f, y + ih), color, thickness);
        draw->AddLine(ImVec2(x, y + h), ImVec2(x + iw, y + h), color, thickness);
        draw->AddLine(ImVec2(x + w - iw, y + h), ImVec2(x + w, y + h), color, thickness);
        draw->AddLine(ImVec2(x, y + h - ih), ImVec2(x, y + h), color, thickness);
        draw->AddLine(ImVec2(x + w - 1.0f, y + h - ih), ImVec2(x + w - 1.0f, y + h), color, thickness);
    }

    // The 3D box needs the pawn's world transform, which the preview does not
    // have; both callers fall back to the outline so the preview still shows a
    // box when the 3D style is selected in game.
    inline void Box(ImDrawList* draw, float x, float y, float w, float h,
                    ImU32 color, float thickness, int style)
    {
        if (draw == nullptr || w <= 0.0f || h <= 0.0f) return;
        switch (style)
        {
            case BoxFill:    DrawFilled(draw, x, y, w, h, color, thickness);  break;
            case BoxCorner:  DrawCorner(draw, x, y, w, h, color, thickness);  break;
            default:         DrawOutline(draw, x, y, w, h, color, thickness); break;
        }
    }
}
