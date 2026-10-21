#pragma once

static inline void DrawDotCrosshair(ImDrawList *draw, float screenW, float screenH)
{
    if (!Config.Crosshair.Enabled || draw == nullptr) return;

    const float size = Config.Crosshair.Size;
    if (!(size > 0.0f)) return;

    float cx = 0.0f, cy = 0.0f;
    NewFeatures::CrosshairCentrePx(cx, cy);

    if (cx < 0.0f || cy < 0.0f || cx > screenW || cy > screenH) return;

    draw->AddCircleFilled(ImVec2(cx, cy), size * 0.5f, ToColor(Config.Crosshair.Color), 32);
}

