#pragma once

#include "imgui.h"
#include "imgui_settings.h"

namespace custom
{
    inline ImVec4 MixColor(const ImVec4& a, const ImVec4& b, float t)
    {
        return ImVec4(
            ImLerp(a.x, b.x, t),
            ImLerp(a.y, b.y, t),
            ImLerp(a.z, b.z, t),
            ImLerp(a.w, b.w, t)
        );
    }

    inline ImVec4 AccentLift(float toward_white, float alpha = 1.0f)
    {
        ImVec4 col = MixColor(c::accent, ImVec4(1.0f, 1.0f, 1.0f, c::accent.w), ImClamp(toward_white, 0.0f, 1.0f));
        col.w *= alpha;
        return col;
    }

    inline ImVec4 AccentShade(float toward_black, float alpha = 1.0f)
    {
        ImVec4 col = MixColor(c::accent, ImVec4(0.0f, 0.0f, 0.0f, c::accent.w), ImClamp(toward_black, 0.0f, 1.0f));
        col.w *= alpha;
        return col;
    }

    inline ImVec4 WithAlpha(const ImVec4& col, float alpha)
    {
        ImVec4 out = col;
        out.w *= alpha;
        return out;
    }
}
