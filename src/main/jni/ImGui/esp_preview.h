#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "esp_model_preview.h"
#include "raspody_style.h"

namespace esp_preview
{
    inline void ModelArea(ImVec2 pos, float width, float height, ImVec2& outMin, ImVec2& outSize)
    {
        const float pad = 6.0f;
        outMin = ImVec2(pos.x + pad, pos.y + pad);
        outSize = ImVec2(ImMax(1.0f, width - pad * 2.0f),
                         ImMax(1.0f, height - pad * 2.0f));
    }

    // Pairs of indices into esp_model_preview::Result::joint[].
    static const int kSkeletonLinks[][2] = {
        { 0,  1 },              // head      -> neck
        { 1,  2 }, { 1,  3 },   // neck      -> shoulders
        { 2,  4 }, { 3,  5 },   // shoulders -> elbows
        { 4,  6 }, { 5,  7 },   // elbows    -> wrists
        { 1,  8 },              // neck      -> chest
        { 8,  9 },              // chest     -> waist
        { 9, 10 }, { 9, 11 },   // waist     -> hips
        { 10, 12 }, { 11, 13 }, // hips      -> knees
        { 12, 14 }, { 13, 15 }, // knees     -> ankles
    };
    static const int kSkeletonLinkCount =
        (int)(sizeof(kSkeletonLinks) / sizeof(kSkeletonLinks[0]));

    inline void Draw(ImDrawList* draw, ImVec2 pos, float width, float height,
                     bool drawBox, bool drawLine, bool drawSkeleton,
                     bool drawHealth, bool drawName, bool drawDistance,
                     ImU32 lineColor, ImU32 boxColor, ImU32 healthColor,
                     ImU32 nameColor, ImU32 distColor)
    {
        if (!draw || width <= 0.0f || height <= 0.0f)
            return;

        const ImVec2 cardMin(pos.x, pos.y);
        const ImVec2 cardMax(pos.x + width, pos.y + height);
        const float rounding = raspody::S(30.0f);

        draw->AddRectFilled(cardMin, cardMax, raspody::Col(raspody::clr.layout), rounding);
        draw->AddRect(cardMin, cardMax, raspody::Col(raspody::clr.separator, 0.55f),
                      rounding, 0, 1.0f);

        ImVec2 modelPos, modelSize;
        ModelArea(cardMin, width, height, modelPos, modelSize);
        const esp_model_preview::Result r = esp_model_preview::DrawModel(draw, modelPos, modelSize);
        if (!r.valid)
            return;

        const float thick = ImMax(1.0f, raspody::S(1.5f));

        // Overlays stay inside the card, so a dragged card at a screen edge
        // cannot spray lines past its own frame.
        draw->PushClipRect(cardMin, cardMax, true);

        // ESP Box
        if (drawBox && r.min.x < r.max.x && r.min.y < r.max.y)
        {
            const float pad = raspody::S(2.0f);
            draw->AddRect(ImVec2(r.min.x - pad, r.min.y - pad),
                          ImVec2(r.max.x + pad, r.max.y + pad),
                          boxColor, raspody::S(3.0f), 0, thick);
        }

        // ESP line: drops from above the head down past the feet.
        if (drawLine)
        {
            const float top = r.min.y - raspody::S(10.0f);
            const float bottom = ImMin(cardMax.y - raspody::S(10.0f),
                                       r.max.y + raspody::S(12.0f));
            draw->AddLine(ImVec2(r.center.x, top), ImVec2(r.center.x, bottom),
                          lineColor, thick);
        }

        // ESP skeleton
        if (drawSkeleton)
        {
            for (int i = 0; i < kSkeletonLinkCount; ++i)
            {
                const ImVec2 a = r.joint[kSkeletonLinks[i][0]];
                const ImVec2 b = r.joint[kSkeletonLinks[i][1]];
                if (a.x < 0.0f || a.y < 0.0f || b.x < 0.0f || b.y < 0.0f)
                    continue;
                draw->AddLine(a, b, lineColor, thick);
            }
        }

        // ESP health bar: half full, standing off the left of the box.
        if (drawHealth)
        {
            const float barW = raspody::S(6.0f);
            const float fullH = ImMax(raspody::S(4.0f), (r.max.y - r.min.y) * 0.5f);
            const ImVec2 hmin(r.min.x - barW - raspody::S(4.0f), r.center.y - fullH * 0.5f);
            const ImVec2 hmax(r.min.x - raspody::S(4.0f), r.center.y + fullH * 0.5f);
            draw->AddRectFilled(hmin, hmax, IM_COL32(0, 0, 0, 120), raspody::S(2.0f));
            draw->AddRectFilled(ImVec2(hmin.x + 1.0f, hmin.y + 1.0f),
                                ImVec2(hmax.x - 1.0f, hmax.y - 1.0f - (fullH - 2.0f) * 0.5f),
                                healthColor, raspody::S(1.0f));
        }

        // ESP name / distance labels
        ImFont* font = ImGui::GetFont();
        const float fontH = ImGui::GetFontSize();
        if (font != nullptr)
        {
            if (drawName)
            {
                const char* label = "Preview";
                const float tw = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, label).x;
                draw->AddText(font, fontH,
                              ImVec2(r.center.x - tw * 0.5f,
                                     r.min.y - fontH - raspody::S(5.0f)),
                              nameColor, label);
            }
            if (drawDistance)
            {
                const char* label = "12m";
                const float tw = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, label).x;
                draw->AddText(font, fontH,
                              ImVec2(r.center.x - tw * 0.5f, r.max.y + raspody::S(6.0f)),
                              distColor, label);
            }
        }

        draw->PopClipRect();
    }
}
