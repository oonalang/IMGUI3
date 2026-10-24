#pragma once

#include "imgui.h"
#include "imgui_internal.h"

constexpr float kUiCardW = 900.0f;
constexpr float kUiCardH = 532.0f;

struct MenuLayout
{
    float  menuW, menuH;
    ImVec2 menuPos;
};

inline MenuLayout ComputeMenuLayout(const ImVec2 &display)
{
    MenuLayout L{};
    const float pad    = ImMax(6.0f, ImMin(display.x, display.y) * 0.02f);
    const float availW = ImMax(1.0f, display.x - pad * 2.0f);
    const float availH = ImMax(1.0f, display.y - pad * 2.0f);

    L.menuW = ImClamp(display.x * 0.92f, ImMin(360.0f, availW), availW);
    L.menuH = ImClamp(display.y * 0.92f, ImMin(320.0f, availH), availH);

    L.menuPos.x = ImClamp((display.x - L.menuW) * 0.5f, 0.0f, ImMax(0.0f, display.x - L.menuW));
    L.menuPos.y = ImClamp((display.y - L.menuH) * 0.5f, 0.0f, ImMax(0.0f, display.y - L.menuH));
    return L;
}

struct MenuWindow
{
    ImVec2 pos;
    ImVec2 size;
};

inline MenuWindow ComputeMenuWindow(const ImVec2 &display)
{
    MenuWindow W{};
    const MenuLayout L = ComputeMenuLayout(display);

    W.size.x = ImMin(L.menuW, kUiCardW);
    W.size.y = ImMin(L.menuH, kUiCardH);

    W.pos.x = ImClamp((display.x - W.size.x) * 0.5f, 0.0f, ImMax(0.0f, display.x - W.size.x));
    W.pos.y = ImClamp((display.y - W.size.y) * 0.5f, 0.0f, ImMax(0.0f, display.y - W.size.y));
    return W;
}

struct SidebarLayout
{
    float railW;
    float panelW;
    float rowH;
    float rowGap;
    float topPad;
    float sidePad;
    float gap;
    int   rows;

    float rowsHeight() const   { return rows * rowH + (rows - 1) * rowGap; }
    float neededHeight() const { return topPad * 2.0f + rowsHeight(); }
    float totalW() const       { return railW + panelW * (panelW > 1.0f ? 1.0f : 0.0f) + gap; }
};

inline SidebarLayout ComputeSidebarLayout(const ImVec2 &hostSize, float anim)
{
    SidebarLayout S{};
    S.railW   = 44.0f;
    S.rowGap  = 6.0f;
    S.topPad  = 8.0f;
    S.sidePad = 6.0f;
    S.rows    = 6;

    const float panelMin = ImMin(120.0f, hostSize.x * 0.28f);
    S.panelW = ImClamp(hostSize.x * 0.20f, panelMin, 148.0f) * ImClamp(anim, 0.0f, 1.0f);
    if (S.panelW < 1.0f) S.panelW = 0.0f;
    S.gap = (S.panelW > 0.0f) ? 10.0f : 0.0f;

    const float availH  = ImMax(1.0f, hostSize.y - S.topPad * 2.0f);
    const float maxRowH = (availH - (float)(S.rows - 1) * S.rowGap) / (float)S.rows;
    S.rowH = ImClamp(maxRowH, 24.0f, 42.0f);

    return S;
}

