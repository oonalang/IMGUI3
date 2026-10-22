#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

// Frosted glass behind the menu.
//
// The panel used to sit on a flat fill, so the game behind it vanished. Here the
// render path keeps a downsampled copy of the last finished frame; drawing that
// copy back under the panel - with the panel's own tint over it - is a blur the
// device already knows how to do (bilinear filtering), with no shader of our
// own and no readback to the CPU.
//
// This header is deliberately free of GLES: the menu and the host renders are
// compiled without a GL context, and they only ever need the texture id. The
// capture itself lives in backdrop_blur.cpp.
namespace backdrop_blur
{
    inline ImTextureID g_texture = nullptr;   // 0 until the first capture
    inline bool        g_ready   = false;
    inline int         g_screenW = 0;
    inline int         g_screenH = 0;

    inline ImTextureID Texture() { return g_texture; }
    inline bool        Ready()   { return g_ready && g_texture != nullptr; }

    // Defined in backdrop_blur.cpp, which is the only file that needs GLES.
    void Capture(int screenW, int screenH);
    void Release();

    // Framebuffer objects come out bottom-up while ImGui's screen space is
    // top-down, so the rect's own place on the display is what goes into the
    // UVs: the panel samples the slice of the frame that is actually behind it,
    // not a stretched copy of the whole screen.
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
