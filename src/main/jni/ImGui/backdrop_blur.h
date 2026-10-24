#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

namespace backdrop_blur
{
    inline ImTextureID g_texture = nullptr;
    inline bool        g_ready   = false;
    inline int         g_screenW = 0;
    inline int         g_screenH = 0;

    inline ImTextureID Texture() { return g_texture; }
    inline bool        Ready()   { return g_ready && g_texture != nullptr; }

    void Capture(int screenW, int screenH);
    void Release();

    inline void UvForRect(const ImVec2& min, const ImVec2& max,
                          const ImVec2& display, ImVec2& uv0, ImVec2& uv1)
    {
        uv0 = ImVec2(0.0f, 0.0f);
        uv1 = ImVec2(1.0f, 1.0f);
        if (display.x <= 1.0f || display.y <= 1.0f) return;

        auto unit = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
        uv0 = ImVec2(unit(min.x / display.x), 1.0f - unit(max.y / display.y));
        uv1 = ImVec2(unit(max.x / display.x), 1.0f - unit(min.y / display.y));
        if (uv1.x <= uv0.x || uv1.y <= uv0.y) {
            uv0 = ImVec2(0.0f, 0.0f);
            uv1 = ImVec2(1.0f, 1.0f);
        }
    }
}
