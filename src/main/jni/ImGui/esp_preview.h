#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "esp_model_preview.h"
#include "esp_shared.h"
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

    // The in-game ESP box, expressed the same way the game builds it:
    // width = 0.65 * height, centred on the pawn's own vertical axis - the
    // root's screen x in game, the model's centre line here. The old preview
    // drew the box around the figure's bounding box instead, which is wider
    // than 0.65 * height and off-centre (the hair ribbons sit on one side), so
    // the box looked crooked and the health bar hung off at an angle.
    struct BoxGeometry
    {
        float axisX = 0.0f;
        float left = 0.0f;
        float top = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    inline constexpr float kBoxWidthRatio = 0.65f;

    inline BoxGeometry ComputeBox(const esp_model_preview::Result& r,
                                  ImVec2 modelPos, ImVec2 modelSize)
    {
        BoxGeometry g;
        g.axisX = modelPos.x + modelSize.x * 0.5f;
        g.top = r.min.y;
        g.height = ImMax(1.0f, r.max.y - r.min.y);
        g.width = g.height * kBoxWidthRatio;
        g.left = g.axisX - g.width * 0.5f;
        return g;
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

    // `boxType`, `healthPosition` and `lineTarget` carry EspBoxType,
    // EspHealthPosition and LineTarget straight through, so the preview follows
    // the same dropdowns the in-game ESP reads.
    inline void Draw(ImDrawList* draw, ImVec2 pos, float width, float height,
                     bool drawBox, bool drawLine, bool drawSkeleton,
                     bool drawHealth, bool drawName, bool drawDistance,
                     ImU32 lineColor, ImU32 boxColor, ImU32 healthColor,
                     ImU32 nameColor, ImU32 distColor,
                     int boxType, int healthPosition, int lineTarget)
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

        const float s = raspody::S(1.0f);
        const float thick = ImMax(1.0f, s * 1.5f);
        const BoxGeometry box = ComputeBox(r, modelPos, modelSize);

        // A sample pawn, so the panels have something to show. 0.72 of full is
        // the same colour the game would paint at 72 HP out of 100.
        const int previewHP = 72;
        const int previewMaxHP = 100;

        // Overlays stay inside the card, so a dragged card at a screen edge
        // cannot spray lines past its own frame.
        draw->PushClipRect(cardMin, cardMax, true);

        // ESP line, from the same three anchors the game offers.
        if (drawLine)
        {
            ImVec2 from(cardMin.x + width * 0.5f, cardMin.y + s * 8.0f);
            ImVec2 to(box.axisX, box.top);
            if (lineTarget == esp_shared::LineCenter)
            {
                from = ImVec2(cardMin.x + width * 0.5f, cardMin.y + height * 0.5f);
            }
            else if (lineTarget == esp_shared::LineBottom)
            {
                from = ImVec2(cardMin.x + width * 0.5f, cardMax.y);
                to = ImVec2(box.axisX, box.top + box.height);
            }
            draw->AddLine(from, to, lineColor, thick);
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

        ImFont* font = ImGui::GetFont();
        const float fontH = ImGui::GetFontSize();

        // Name, and the health strip that sits under it in the "top" style:
        // both live in one dark container just above the box, as in game.
        const float containerW = ImMax(box.width * 1.6f, s * 70.0f);
        const float containerX = box.axisX - containerW * 0.5f;
        float textY = box.top - s * 26.0f;

        if (drawName || (drawHealth && healthPosition == esp_shared::HealthTop))
        {
            const float nameH = s * 20.0f;
            const float barH = (drawHealth && healthPosition == esp_shared::HealthTop) ? s * 6.0f : 0.0f;
            const float totalH = nameH + (barH > 0.0f ? barH + s * 3.0f : 0.0f);
            const float boxY = box.top - s * 6.0f - totalH;

            draw->AddRectFilled(ImVec2(containerX, boxY),
                                ImVec2(containerX + containerW, boxY + totalH),
                                IM_COL32(0, 0, 0, 120));
            textY = boxY;

            if (drawName && font != nullptr)
            {
                const char* label = "Preview";
                const float tw = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, label).x;
                draw->AddText(font, fontH,
                              ImVec2(box.axisX - tw * 0.5f, boxY + s * 3.0f),
                              nameColor, label);
            }

            if (barH > 0.0f)
            {
                const float pad = s * 5.0f;
                esp_shared::TopHealthBar(draw,
                                         containerX + pad,
                                         boxY + nameH,
                                         containerW - pad * 2.0f,
                                         barH,
                                         previewHP, previewMaxHP);
            }
        }

        // Side health bar: full box height, filled from the bottom.
        if (drawHealth && healthPosition == esp_shared::HealthSide)
        {
            // Same offset the game uses: the health track starts 8 px left of
            // the box, and the armour bar (when it is on) takes 14 px.
            esp_shared::SideHealthBar(draw, box.left - s * 8.0f, box.top, box.height,
                                      previewHP, previewMaxHP);
        }

        // Distance, under the box in its own container.
        if (drawDistance && font != nullptr)
        {
            const char* label = "12m";
            const float tw = font->CalcTextSizeA(fontH, FLT_MAX, 0.0f, label).x;
            const float padX = s * 5.0f;
            const float padY = s * 3.0f;
            const float w = tw + padX * 2.0f;
            const float h = fontH + padY * 2.0f;
            const float x = box.axisX - w * 0.5f;
            const float y = box.top + box.height + s * 5.0f;

            draw->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 120));
            draw->AddText(font, fontH, ImVec2(x + padX, y + padY), distColor, label);
        }

        if (drawBox)
        {
            esp_shared::Box(draw, box.left, box.top, box.width, box.height,
                            boxColor, thick, boxType);
        }

        draw->PopClipRect();

        (void)healthColor;   // the game's bar ramps red->green, so a fixed tint is unused
        (void)textY;
    }
}
