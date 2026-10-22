#pragma once

#include "ImGui/Call_ImGui.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/menu_layout.h"
#include "LoadingMedia.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cfloat>

extern ImFont* F50;
namespace font { extern ImFont* inter_semibold; }

namespace ui_loading {

static bool  g_Active   = false;
static float g_Timer    = 0.0f;
static float g_Duration = 7.0f;
static float g_Time     = 0.0f;

// Same card as the menu and the login screen.
static constexpr float kCardW      = kUiCardW;
static constexpr float kCardH      = kUiCardH;
static constexpr float kCardRound  = 28.0f;
static constexpr float kPromoEvery = 2.5f;
static constexpr float kPromoFade  = 0.38f;

namespace C {
    static const ImVec4 Purple     = {0.65f, 0.35f, 1.00f, 1.000f};
    static const ImVec4 PurpleBrt  = {0.80f, 0.50f, 1.00f, 1.000f};
    static const ImVec4 PurpleDeep = {0.45f, 0.20f, 0.80f, 1.000f};
    static const ImVec4 White      = {0.961f, 0.941f, 0.882f, 1.000f};
    static const ImVec4 TextDim    = {0.600f, 0.550f, 0.700f, 1.000f};
    static const ImVec4 BgDeep     = {0.040f, 0.010f, 0.080f, 1.000f};
    static const ImVec4 Border     = {0.650f, 0.350f, 1.000f, 0.600f};

    static ImU32 UA(ImVec4 c, float a) { return IM_COL32((int)(c.x*255),(int)(c.y*255),(int)(c.z*255),(int)(a*255)); }
    static ImU32 U(ImVec4 c)           { return UA(c, 1.0f); }
}

static const char* kPromos[3] = {
    "Buy Ethnir Premium, It's good!",
    "Hey, why not buy today?",
    "Premium has lots of great features!"
};

static float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
static float EaseOut(float t) { t = Clamp01(t); float i = 1.0f - t; return 1.0f - i*i*i; }
static float Pulse(float t, float f) { return std::sin(t * f * 6.2831853f) * 0.5f + 0.5f; }

static float TextW(ImFont* font, float size, const char* text)
{
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
}

static void TextCentered(ImDrawList* dl, ImFont* font, float size, float centerX, float topY,
                         ImU32 col, const char* text)
{
    dl->AddText(font, size, ImVec2(centerX - TextW(font, size, text) * 0.5f, topY), col, text);
}

static void TextTracked(ImDrawList* dl, ImFont* font, float size, float centerX, float topY,
                        ImU32 col, const char* text, float tracking)
{
    float width = 0.0f;
    for (const char* c = text; *c; ++c)
    {
        char one[2] = { *c, 0 };
        width += TextW(font, size, one) + tracking;
    }
    if (width > 0.0f) width -= tracking;

    ImVec2 pos(centerX - width * 0.5f, topY);
    for (const char* c = text; *c; ++c)
    {
        char one[2] = { *c, 0 };
        dl->AddText(font, size, pos, col, one);
        pos.x += TextW(font, size, one) + tracking;
    }
}

static void CoverImage(ImDrawList* dl, ImTextureID tex, ImVec2 mn, ImVec2 mx, float aspect)
{
    const float boxW = mx.x - mn.x;
    const float boxH = mx.y - mn.y;
    if (!(boxW > 0.0f) || !(boxH > 0.0f) || aspect <= 0.0f) return;

    float w = boxW;
    float h = w / aspect;
    if (h < boxH) { h = boxH; w = h * aspect; }

    const ImVec2 texMin(mn.x + (boxW - w) * 0.5f, mn.y + (boxH - h) * 0.5f);
    const ImVec2 texMax(texMin.x + w, texMin.y + h);
    dl->AddImage(tex, texMin, texMax, ImVec2(0, 0), ImVec2(1, 1));
}

static void Backdrop(ImDrawList* dl, ImVec2 display, ImTextureID still)
{
    const ImTextureID video = ui_media::VideoTexture();
    const ImTextureID source = video != nullptr ? video : still;
    const float aspect = video != nullptr ? ui_media::VideoAspect()
                                          : (display.x > 0.0f ? display.x / display.y : 16.0f / 9.0f);

    dl->AddRectFilled(ImVec2(0, 0), display, C::U(C::BgDeep));

    if (source != nullptr)
    {
        CoverImage(dl, source, ImVec2(0, 0), display, aspect);
    }
    else
    {
        const float drift = std::sin(g_Time * 0.28f) * display.x * 0.06f;
        const float rise  = std::cos(g_Time * 0.21f) * display.y * 0.04f;
        dl->AddRectFilledMultiColor(ImVec2(0, 0), display,
            IM_COL32(10, 4, 22, 255), IM_COL32(14, 6, 28, 255),
            IM_COL32(4, 1, 12, 255),   IM_COL32(6, 2, 16, 255));

        for (int i = 0; i < 3; ++i)
        {
            const float phase = g_Time * (0.16f + i * 0.05f) + i * 2.1f;
            const ImVec2 c(display.x * (0.5f + 0.26f * std::sin(phase)) + drift * 0.35f,
                           display.y * (0.42f + 0.22f * std::cos(phase * 0.8f)) + rise * 0.35f);
            const float r = display.x * (0.22f + 0.05f * i);
            dl->AddCircleFilled(c, r, C::UA(C::PurpleDeep, 0.10f - i * 0.02f), 64);
            dl->AddCircleFilled(c, r * 0.55f, C::UA(C::Purple, 0.06f - i * 0.015f), 64);
        }
    }

    // The video is the background: only dim it enough to keep the card legible.
    dl->AddRectFilled(ImVec2(0, 0), display,
                      IM_COL32(6, 2, 14, source != nullptr ? 72 : 104));
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(display.x, display.y * 0.55f),
        IM_COL32(26, 10, 48, 62), IM_COL32(26, 10, 48, 62),
        IM_COL32(0, 0, 0, 0),     IM_COL32(0, 0, 0, 0));
    dl->AddRectFilledMultiColor(ImVec2(0, display.y * 0.55f), display,
        IM_COL32(0, 0, 0, 0),           IM_COL32(0, 0, 0, 0),
        IM_COL32(3, 1, 9, 176),         IM_COL32(3, 1, 9, 176));
}

static void GlassCard(ImDrawList* dl, ImVec2 mn, ImVec2 mx)
{
    const float breathe = 0.85f + 0.15f * Pulse(g_Time, 0.35f);

    for (int i = 3; i >= 1; --i)
    {
        const float grow = (float)i * 9.0f;
        dl->AddRect(ImVec2(mn.x - grow, mn.y - grow), ImVec2(mx.x + grow, mx.y + grow),
                    C::UA(C::Purple, 0.024f * breathe / (float)i),
                    kCardRound + grow, 0, 14.0f);
    }

    dl->AddRectFilled(mn, mx, IM_COL32(16, 7, 30, 202), kCardRound);
    dl->AddRectFilled(mn, mx, C::UA(C::BgDeep, 0.46f), kCardRound);

    const ImVec2 sheenMax(mx.x, mn.y + (mx.y - mn.y) * 0.46f);
    dl->AddRectFilledMultiColor(ImVec2(mn.x + 2, mn.y + 2), ImVec2(sheenMax.x - 2, sheenMax.y),
        IM_COL32(255, 255, 255, 20), IM_COL32(255, 255, 255, 20),
        IM_COL32(255, 255, 255, 0),  IM_COL32(255, 255, 255, 0));

    dl->AddRectFilledMultiColor(ImVec2(mn.x + 2, mn.y + (mx.y - mn.y) * 0.62f), ImVec2(mx.x - 2, mx.y - 2),
        IM_COL32(0, 0, 0, 0),            IM_COL32(0, 0, 0, 0),
        C::UA(C::BgDeep, 0.55f),         C::UA(C::BgDeep, 0.55f));

    dl->AddRect(ImVec2(mn.x + 1.0f, mn.y + 1.0f), ImVec2(mx.x - 1.0f, mx.y - 1.0f),
                C::UA(C::Border, 0.42f * breathe), kCardRound - 1.0f, 0, 1.4f);
    dl->AddRect(ImVec2(mn.x + 7.0f, mn.y + 7.0f), ImVec2(mx.x - 7.0f, mx.y - 7.0f),
                IM_COL32(255, 255, 255, 12), kCardRound - 7.0f, 0, 1.0f);

    dl->AddLine(ImVec2(mn.x + kCardRound, mn.y + 1.5f), ImVec2(mx.x - kCardRound, mn.y + 1.5f),
                IM_COL32(255, 255, 255, 34), 1.2f);

    const float inset = 18.0f;
    dl->PathArcTo(ImVec2(mn.x + inset + kCardRound * 0.35f, mn.y + inset + kCardRound * 0.35f),
                  kCardRound * 0.7f, IM_PI * 0.98f, IM_PI * 1.52f, 16);
    dl->PathStroke(C::UA(C::PurpleBrt, 0.55f * breathe), 0, 1.6f);
    dl->PathArcTo(ImVec2(mx.x - inset - kCardRound * 0.35f, mx.y - inset - kCardRound * 0.35f),
                  kCardRound * 0.7f, IM_PI * 1.98f, IM_PI * 2.52f, 16);
    dl->PathStroke(C::UA(C::PurpleBrt, 0.55f * breathe), 0, 1.6f);

}

static void ProgressBar(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float progress)
{
    const float h = mx.y - mn.y;
    const float radius = h * 0.5f;
    const float pulse  = 0.85f + 0.15f * Pulse(g_Time, 0.6f);

    // Outer bloom: the glass sits in its own light.
    for (int i = 3; i >= 1; --i)
    {
        const float grow = (float)i * 3.5f;
        dl->AddRect(ImVec2(mn.x - grow, mn.y - grow), ImVec2(mx.x + grow, mx.y + grow),
                    C::UA(C::Purple, 0.022f / (float)i), radius + grow, 0, 5.0f);
    }

    // Track: tinted glass, bright rim, top specular band.
    dl->AddRectFilled(mn, mx, IM_COL32(255, 255, 255, 12), radius);
    dl->AddRectFilled(mn, mx, IM_COL32(12, 5, 24, 138), radius);

    dl->PushClipRect(ImVec2(mn.x + 0.5f, mn.y), ImVec2(mx.x - 0.5f, mx.y), true);
    dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y), ImVec2(mx.x, mn.y + h * 0.62f),
        IM_COL32(255, 255, 255, 60), IM_COL32(255, 255, 255, 44),
        IM_COL32(255, 255, 255, 0),  IM_COL32(255, 255, 255, 0));
    dl->PopClipRect();

    dl->AddRect(mn, mx, IM_COL32(255, 255, 255, 44), radius, 0, 1.0f);

    const float fillW = (mx.x - mn.x) * EaseOut(progress);
    if (fillW <= 2.0f) return;

    const ImVec2 fillMax(mn.x + fillW, mx.y);

    // Liquid body.
    dl->AddRectFilledMultiColor(mn, fillMax,
        C::UA(C::PurpleDeep, 0.92f), C::UA(C::PurpleBrt, 0.92f),
        C::UA(C::PurpleBrt, 0.92f), C::UA(C::PurpleDeep, 0.92f));

    // Glass on top of the liquid: specular band plus a travelling streak.
    dl->PushClipRect(ImVec2(mn.x, mn.y), ImVec2(fillMax.x, mx.y), true);
    dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y), ImVec2(fillMax.x, mn.y + h * 0.58f),
        IM_COL32(255, 255, 255, 118), IM_COL32(255, 255, 255, 88),
        IM_COL32(255, 255, 255, 0),   IM_COL32(255, 255, 255, 0));
    dl->AddRectFilledMultiColor(ImVec2(mn.x, mx.y - h * 0.30f), ImVec2(fillMax.x, mx.y),
        C::UA(C::PurpleDeep, 0.0f), C::UA(C::PurpleDeep, 0.0f),
        C::UA(C::PurpleDeep, 0.45f), C::UA(C::PurpleDeep, 0.45f));

    const float phase = g_Time * 0.42f;
    const float sweep = mn.x - 80.0f + (mx.x - mn.x + 160.0f) * (phase - std::floor(phase));
    dl->AddRectFilledMultiColor(ImVec2(sweep - 64.0f, mn.y), ImVec2(sweep, mx.y),
        IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 72),
        IM_COL32(255, 255, 255, 72), IM_COL32(255, 255, 255, 0));
    dl->PopClipRect();

    dl->AddRect(ImVec2(mn.x, mn.y), ImVec2(fillMax.x, mx.y),
                IM_COL32(255, 255, 255, 88), radius, 0, 1.0f);

    // The liquid head: bloom, core and a wet highlight.
    const ImVec2 head(fillMax.x, mn.y + radius);
    dl->AddCircleFilled(head, radius * 2.2f * pulse, C::UA(C::PurpleBrt, 0.15f), 24);
    dl->AddCircleFilled(head, radius * 1.05f * pulse, C::UA(C::PurpleBrt, 0.95f), 20);
    dl->AddCircleFilled(ImVec2(head.x - radius * 0.28f, head.y - radius * 0.30f),
                        radius * 0.34f, IM_COL32(255, 255, 255, 165), 14);
}

static void PromoText(ImDrawList* dl, ImFont* font, float centerX, float y, bool pill)
{
    const int count = 3;
    const float cycle = g_Time / kPromoEvery;
    const int index = ((int)cycle) % count;
    const float local = cycle - std::floor(cycle);
    const float fade = Clamp01(local / kPromoFade) * Clamp01((1.0f - local) / kPromoFade);

    const char* text = kPromos[index];

    if (pill)
    {
        const float w = TextW(font, 15.0f, text) * 0.5f + 18.0f;
        const ImVec2 pmn(centerX - w, y - 7.0f);
        const ImVec2 pmx(centerX + w, y + 30.0f);
        dl->AddRectFilled(pmn, pmx, IM_COL32(8, 3, 18, 118), 14.0f);
        dl->AddRect(pmn, pmx, IM_COL32(255, 255, 255, 16), 14.0f, 0, 1.0f);
    }

    TextCentered(dl, font, 15.0f, centerX, y, C::UA(C::White, 0.52f + 0.38f * fade), text);

    const float dotY = y + 20.0f;
    for (int i = 0; i < count; ++i)
    {
        const float dotX = centerX + (i - (count - 1) * 0.5f) * 14.0f;
        const bool active = (i == index);
        dl->AddCircleFilled(ImVec2(dotX, dotY), active ? 2.6f : 1.8f,
                            active ? C::UA(C::PurpleBrt, 0.45f + 0.45f * fade)
                                   : C::UA(C::TextDim, 0.35f), 12);
    }
}

static void StatusRow(ImDrawList* dl, ImFont* font, ImVec2 mn, ImVec2 mx, float progress)
{
    const char* stage = (progress < 0.34f) ? "VERIFYING LICENSE"
                      : (progress < 0.68f) ? "SYNCING MODULES"
                      : (progress < 0.95f) ? "PREPARING INTERFACE"
                      :                      "READY";

    dl->AddText(font, 11.0f, ImVec2(mn.x, mn.y), IM_COL32(206, 196, 232, 232), stage);

    char pct[8];
    std::snprintf(pct, sizeof(pct), "%d%%", (int)(progress * 100.0f + 0.5f));
    const float w = TextW(font, 11.0f, pct);
    dl->AddText(font, 11.0f, ImVec2(mx.x - w, mn.y), C::UA(C::PurpleBrt, 0.98f), pct);
}

inline void Start(float durationSeconds = 7.0f) {
    g_Active   = true;
    g_Timer    = 0.0f;
    g_Time     = 0.0f;
    g_Duration = durationSeconds < 2.0f ? 2.0f : durationSeconds;
    ui_media::StartAudio();
}

inline bool  IsActive()    { return g_Active; }
inline float GetProgress() { return g_Duration <= 0.0f ? 1.0f : Clamp01(g_Timer / g_Duration); }

inline bool RenderWindow(ImTextureID tex = nullptr) {
    if (!g_Active) return false;

    ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    g_Time  += dt;
    g_Timer += dt;
    const float progress = GetProgress();

    ui_media::Tick();

    const ImVec2 display = io.DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(display, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    bool finished = false;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize     | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("##ethnir_loading", nullptr, flags))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        Backdrop(dl, display, tex);

        // The video fades up from black instead of popping in on the first frame.
        const float enter = EaseOut(Clamp01(g_Time / 0.55f));
        if (enter < 0.999f)
            dl->AddRectFilled(ImVec2(0, 0), display, IM_COL32(0, 0, 0, (int)((1.0f - enter) * 255.0f)));

        ImFont* body = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
        ImFont* disp = F50 ? F50 : body;

        // Entrance: the card settles from 96% and drops the last 16 px into place.
        const float enterK = 0.960f + 0.040f * enter;
        const float lift   = (1.0f - enter) * 16.0f;
        const float cardW = ImMin(kCardW, ImMax(320.0f, display.x - 32.0f)) * enterK;
        const float cardH = kCardH * enterK;
        const ImVec2 cardMin((display.x - cardW) * 0.5f,
                             ImMax(16.0f, (display.y - cardH) * 0.5f - 22.0f) + lift);
        const ImVec2 cardMax(cardMin.x + cardW, cardMin.y + cardH);
        const float cardCx = (cardMin.x + cardMax.x) * 0.5f;

        GlassCard(dl, cardMin, cardMax);

        const float wordTop = cardMin.y + cardH * 0.240f;
        TextTracked(dl, disp, 46.0f, cardCx, wordTop + 2.0f, IM_COL32(0, 0, 0, 120), "ETHNIR", 5.0f);
        TextTracked(dl, disp, 46.0f, cardCx, wordTop, C::UA(C::White, 0.35f + 0.65f * enter),
                    "ETHNIR", 5.0f);
        TextTracked(dl, body, 11.0f, cardCx, wordTop + 68.0f,
                    C::UA(C::PurpleBrt, 0.45f + 0.50f * enter), "FREE TRIAL", 4.0f);

        const float barW = cardW - 176.0f;
        const ImVec2 barMin(cardCx - barW * 0.5f, cardMin.y + cardH * 0.600f);
        const ImVec2 barMax(barMin.x + barW, barMin.y + 12.0f);
        ProgressBar(dl, barMin, barMax, progress);

        StatusRow(dl, body, ImVec2(barMin.x, barMax.y + 14.0f),
                  ImVec2(barMax.x, barMax.y + 14.0f), progress);

        PromoText(dl, body, cardCx, cardMax.y + 34.0f,
                  ui_media::VideoTexture() != nullptr || tex != nullptr);
    }
    ImGui::End();

    if (g_Timer >= g_Duration) {
        g_Active = false;
        g_Timer  = 0.0f;
        ui_media::StopAudio();
        finished = true;
    }
    return finished;
}
}
