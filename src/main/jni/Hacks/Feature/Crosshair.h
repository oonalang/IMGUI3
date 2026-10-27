#pragma once

static inline void DrawDotCrosshair(ImDrawList *draw, float screenW, float screenH)
{
    if (!Config.Crosshair.Enabled || draw == nullptr) return;

    const float size = Config.Crosshair.Size;
    if (!(size > 0.0f)) return;

    float cx = 0.0f, cy = 0.0f;
    NewFeatures::CrosshairCentrePx(cx, cy);

    if (cx < 0.0f || cy < 0.0f || cx > screenW || cy > screenH) return;

    const ImU32 color = (ImU32)ToColor(Config.Crosshair.Color);

    if (Config.Crosshair.Style == ReticleCross)
    {
        const float arm = size * 0.5f;
        float gap = size * 0.12f;
        if (gap < 1.0f) gap = 1.0f;
        float thickness = size * 0.12f;
        if (thickness < 1.0f) thickness = 1.0f;

        draw->AddLine(ImVec2(cx - arm, cy), ImVec2(cx - gap, cy), color, thickness);
        draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + arm, cy), color, thickness);
        draw->AddLine(ImVec2(cx, cy - arm), ImVec2(cx, cy - gap), color, thickness);
        draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + arm), color, thickness);
        return;
    }

    draw->AddCircleFilled(ImVec2(cx, cy), size * 0.5f, color, 32);
}
