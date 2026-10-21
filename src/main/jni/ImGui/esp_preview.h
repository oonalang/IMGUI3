#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "esp_model_preview.h"

namespace esp_preview
{
    inline void ModelArea(ImVec2 pos, float width, float height, ImVec2& outMin, ImVec2& outSize)
    {
        const float pad = 9.0f;
        const float titleH = 24.0f;
        const float hintH = 18.0f;
        outMin = ImVec2(pos.x + pad, pos.y + titleH);
        outSize = ImVec2(ImMax(1.0f, width - pad * 2.0f),
                         ImMax(1.0f, height - titleH - hintH));
    }

    inline void Draw(ImDrawList* draw, ImVec2 pos, float width, float height)
    {
    if (!draw || width <= 0.0f || height <= 0.0f)
        return;

    const ImVec2 cardMin(pos.x, pos.y);
    const ImVec2 cardMax(pos.x + width, pos.y + height);
    const float cardRounding = 14.0f;

    draw->AddRectFilled(cardMin, cardMax, IM_COL32(14, 15, 20, 242), cardRounding);
    draw->AddRectFilledMultiColor(cardMin, cardMax,
        IM_COL32(168, 104, 255, 52), IM_COL32(168, 104, 255, 40),
        IM_COL32(12, 12, 18, 0), IM_COL32(12, 12, 18, 0), cardRounding);
    draw->AddRect(cardMin, cardMax, IM_COL32(168, 104, 255, 90), cardRounding, 0, 1.2f);

    const float pad = 9.0f;
    const float titleH = 24.0f;
    const float hintH = 18.0f;

    {
        ImFont* f = ImGui::GetFont();
        const float fs = ImGui::GetFontSize() * 0.86f;
        draw->AddText(f, fs, ImVec2(cardMin.x + pad, cardMin.y + 7.0f),
            IM_COL32(238, 238, 244, 245), "ESP PREVIEW");
        const char* hint = "DRAG TO SPIN";
        const ImVec2 hs = f->CalcTextSizeA(fs * 0.72f, FLT_MAX, 0.0f, hint);
        draw->AddText(f, fs * 0.72f, ImVec2(cardMax.x - pad - hs.x, cardMin.y + 9.0f),
            IM_COL32(168, 160, 166, 200), hint);
        draw->AddLine(ImVec2(cardMin.x + pad, cardMin.y + titleH),
            ImVec2(cardMax.x - pad, cardMin.y + titleH), IM_COL32(120, 112, 118, 60), 1.0f);
    }

    ImVec2 modelPos, modelSize;
    ModelArea(cardMin, width, height, modelPos, modelSize);

    ImGui::SetCursorScreenPos(modelPos);
    ImGui::InvisibleButton("##esp_preview_spin", modelSize);
    const bool spinning = ImGui::IsItemActive();
    esp_model_preview::SetDragging(spinning);
    if (spinning) {
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        if (d.x != 0.0f || d.y != 0.0f) esp_model_preview::DragDelta(d.x, d.y);
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) esp_model_preview::ResetView();

    const esp_model_preview::Result model =
        esp_model_preview::DrawModel(draw, modelPos, modelSize);

    if (!model.valid) return;

    auto safeColor = [](float* col) -> ImU32 {
        if (!col) return IM_COL32(255, 255, 255, 255);
        return IM_COL32((int)(col[0]*255), (int)(col[1]*255), (int)(col[2]*255), (int)(col[3]*255));
    };

    const float bx = model.min.x - 6.0f, by = model.min.y - 6.0f;
    const float bw = (model.max.x - model.min.x) + 12.0f;
    const float bh = (model.max.y - model.min.y) + 12.0f;

    if (Config.ESPMenu.Box) {
        ImU32 bc = safeColor(Config.sColorsESPPLAYER.BoxPLAYER);
        if (Config.ESPMenu.BoxType == EspBoxType::Outline) {
            draw->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), bc, 0.0f, 0, 2.0f);
        } else if (Config.ESPMenu.BoxType == EspBoxType::Fill) {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(bc);
            draw->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + bh),
                ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 0.15f)));
            draw->AddRect(ImVec2(bx, by), ImVec2(bx + bw, by + bh), bc, 0.0f, 0, 2.0f);
        } else if (Config.ESPMenu.BoxType == EspBoxType::Corner) {
            const float iw = bw / 4.0f, ih = bh / 4.0f;
            draw->AddLine(ImVec2(bx, by), ImVec2(bx + iw, by), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by), ImVec2(bx, by + ih), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw - iw, by), ImVec2(bx + bw, by), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw, by), ImVec2(bx + bw, by + ih), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by + bh - ih), ImVec2(bx, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx, by + bh), ImVec2(bx + iw, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw - iw, by + bh), ImVec2(bx + bw, by + bh), bc, 2.0f);
            draw->AddLine(ImVec2(bx + bw, by + bh - ih), ImVec2(bx + bw, by + bh), bc, 2.0f);
        }
    }

    if (Config.ESPMenu.isPlayerLine) {
        ImU32 lc = safeColor(Config.sColorsESPPLAYER.LinePLAYER);
        ImVec2 from;
        if (Config.ESPMenu.Target == LineTarget::Bottom) from = ImVec2(cardMin.x + width * 0.5f, cardMax.y - hintH);
        else if (Config.ESPMenu.Target == LineTarget::Center) from = ImVec2(cardMin.x + width * 0.5f, cardMin.y + height * 0.5f);
        else from = ImVec2(cardMin.x + width * 0.5f, cardMin.y + titleH + 2.0f);
        draw->AddLine(from, ImVec2(model.center.x, by), lc, 1.6f);
    }

    if (Config.ESPMenu.Skeleton) {
        ImU32 sc = safeColor(Config.sColorsESPPLAYER.SkeletonPLAYER);
        auto bone = [&](int a, int b) { draw->AddLine(model.joint[a], model.joint[b], sc, 1.6f); };
        bone(0, 1);
        bone(1, 2); bone(1, 3);
        bone(2, 4); bone(3, 5);
        bone(4, 6); bone(5, 7);
        bone(1, 8); bone(8, 9);
        bone(9, 10); bone(9, 11);
        bone(10, 12); bone(11, 13);
        bone(12, 14); bone(13, 15);
    }

    if (Config.ESPMenu.Health) {
        const float hp = 0.72f;
        const int r = (int)(255 * (1.0f - hp)), g = (int)(255 * hp);
        const ImU32 hpc = IM_COL32(r, g, 0, 255);
        const float hx = bx - 9.0f, hy = by, hw = 4.5f, hh = bh;
        draw->AddRectFilled(ImVec2(hx, hy), ImVec2(hx + hw, hy + hh), IM_COL32(0, 0, 0, 120));
        draw->AddRect(ImVec2(hx, hy), ImVec2(hx + hw, hy + hh), IM_COL32(0, 0, 0, 220), 0.0f, 0, 1.0f);
        draw->AddRectFilled(ImVec2(hx, hy + hh * (1.0f - hp)), ImVec2(hx + hw, hy + hh), hpc);
    }

    if (Config.ESPMenu.Name) {
        ImU32 nc = safeColor(Config.sColorsESPPLAYER.NamePLAYER);
        const char* name = "PLAYER";
        const ImVec2 ts = ImGui::CalcTextSize(name);
        draw->AddText(ImVec2(model.center.x - ts.x * 0.5f, by - ts.y - 5.0f), nc, name);
    }

    if (Config.ESPMenu.Distance) {
        ImU32 dc = safeColor(Config.sColorsESPPLAYER.DistancePLAYER);
        const char* dist = "50m";
        const ImVec2 ts = ImGui::CalcTextSize(dist);
        draw->AddText(ImVec2(model.center.x - ts.x * 0.5f, by + bh + 5.0f), dc, dist);
    }

    if (Config.ESPMenu.ShowFov) {
        const float radius = Config.Aim.Cross * 0.6f;
        if (radius > 0.0f)
            draw->AddCircle(model.center, radius, IM_COL32(168, 104, 255, 110), 0, 1.5f);
    }

    if (!spinning) {
        ImFont* f = ImGui::GetFont();
        const char* bottomHint = "drag to rotate  -  double tap to reset";
        const ImVec2 hs = f->CalcTextSizeA(ImGui::GetFontSize() * 0.66f, FLT_MAX, 0.0f, bottomHint);
        draw->AddText(f, ImGui::GetFontSize() * 0.66f,
            ImVec2(cardMin.x + (width - hs.x) * 0.5f, cardMax.y - hintH + 3.0f),
            IM_COL32(140, 134, 140, 190), bottomHint);
    }    }
}

