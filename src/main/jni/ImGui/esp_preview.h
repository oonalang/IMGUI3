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

    inline void Draw(ImDrawList* draw, ImVec2 pos, float width, float height)
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
        esp_model_preview::DrawModel(draw, modelPos, modelSize);
    }
}
