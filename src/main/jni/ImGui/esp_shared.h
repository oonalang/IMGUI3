#pragma once

#include "imgui.h"

namespace esp_shared
{

    enum BoxStyle
    {
        BoxFill    = 0,
        BoxOutline = 1,
        BoxCorner  = 2,
        BoxThreeD  = 3
    };

    enum HealthStyle
    {
        HealthTop  = 0,
        HealthSide = 1
    };

    enum LineStyle
    {
        LineTop    = 0,
        LineCenter = 1,
        LineBottom = 2
    };

    inline ImU32 HealthColor(int curHP, int maxHP)
    {
        if (maxHP <= 0) return IM_COL32(0, 255, 0, 255);
        const float ratio = ImClamp(curHP / (float)maxHP, 0.0f, 1.0f);
        const int r = (int)ImMin(510.0f * (1.0f - ratio), 255.0f);
        const int g = (int)ImMin(510.0f * ratio, 255.0f);
        return IM_COL32(r, g, 0, 255);
    }

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
