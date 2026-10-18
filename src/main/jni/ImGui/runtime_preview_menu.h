#pragma once

#include "Call_ImGui.h"
#include "imgui_settings.h"
#include "../System/Core/SaveConfig.h"
#include "../System/UI/TextureLoader.h"
#include "../System/UI/Logo.h"
#include "../System/UI/SkinTab.h"
#include "../Fonts/Icon.h"
#include "../IMAGE/bg.h"

#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>

extern ImFont* F50;
extern ImFont* F107;
extern float menu[4];
extern float SnowBsize;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;
extern float SlideRange;

namespace font {
    extern ImFont* inter_semibold;
}

namespace runtime_preview_menu {

struct Texture {
    GLuint id = 0;
};

struct StateRefs {
    bool &dark;
    float &tabAlpha;
    float &tabAdd;
    int &page;
    int &activeTab;
    bool &windowCollapsed;
    bool &isMenuVisible;
    double &collapseBarLastActiveTime;
    float &collapseBarOpacityAnim;
    float &collapseBarPressAnim;
};

static Texture g_menuBackground;

static bool g_popupFocusVisible = false;
static ImVec2 g_popupFocusMin(0.0f, 0.0f);
static ImVec2 g_popupFocusMax(0.0f, 0.0f);

static int g_activeChangelogTab = 0;

inline void EnsureTexture(Texture &texture, const unsigned char *bytes, int size) {
    if (texture.id != 0) {
        return;
    }
    texture.id = LoadTextureFromMemory(bytes, size);
}

inline void EnsureTexturesLoaded() {
    EnsureTexture(g_menuBackground, bg_data, (int)sizeof(bg_data));
}

inline std::string ToUpperAscii(std::string text) {
    for (char &ch : text) {
        ch = (char)std::toupper((unsigned char)ch);
    }
    return text;
}

inline std::string BuildExpiryLabel() {
    const std::string countdown = getExpiryCountdown();
    if (!countdown.empty()) {
        return ToUpperAscii("EXPIRY " + countdown);
    }
    if (EXP == "No expiry") {
        return "NO EXPIRY";
    }
    if (!EXP.empty() && EXP != " ") {
        return ToUpperAscii(EXP);
    }
    return "NO EXPIRY";
}

inline std::string BuildFpsLabel() {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "FPS %d", (int)std::lround(ImGui::GetIO().Framerate));
    return buffer;
}

inline void ResetPopupFocusWindow() {
    g_popupFocusVisible = false;
    g_popupFocusMin = ImVec2(0.0f, 0.0f);
    g_popupFocusMax = ImVec2(0.0f, 0.0f);
}

inline void RegisterPopupFocusWindow() {
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 max(min.x + windowSize.x, min.y + windowSize.y);
    if (!g_popupFocusVisible) {
        g_popupFocusVisible = true;
        g_popupFocusMin = min;
        g_popupFocusMax = max;
        return;
    }

    g_popupFocusMin.x = ImMin(g_popupFocusMin.x, min.x);
    g_popupFocusMin.y = ImMin(g_popupFocusMin.y, min.y);
    g_popupFocusMax.x = ImMax(g_popupFocusMax.x, max.x);
    g_popupFocusMax.y = ImMax(g_popupFocusMax.y, max.y);
}

inline void DrawPopupBackdropFocusLayer(ImDrawList *drawList) {
    if (drawList == nullptr || !g_popupFocusVisible) {
        return;
    }

    drawList->PushClipRectFullScreen();
    const ImVec2 viewportMax = ImGui::GetIO().DisplaySize;
    const ImVec2 holeMin(
        ImClamp(g_popupFocusMin.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMin.y, 0.0f, viewportMax.y)
    );
    const ImVec2 holeMax(
        ImClamp(g_popupFocusMax.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMax.y, 0.0f, viewportMax.y)
    );
    const ImU32 dimColor = IM_COL32(8, 0, 14, 172);

    if (holeMin.y > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(viewportMax.x, holeMin.y), dimColor);
    if (holeMin.x > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, holeMin.y), ImVec2(holeMin.x, holeMax.y), dimColor);
    if (holeMax.x < viewportMax.x) drawList->AddRectFilled(ImVec2(holeMax.x, holeMin.y), ImVec2(viewportMax.x, holeMax.y), dimColor);
    if (holeMax.y < viewportMax.y) drawList->AddRectFilled(ImVec2(0.0f, holeMax.y), viewportMax, dimColor);

    drawList->AddRect(
        holeMin,
        holeMax,
        ImGui::GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.18f)),
        10.0f,
        0,
        1.0f
    );
    drawList->PopClipRect();
}

#include "runtime_preview_helpers.h"
#include "../System/UI/MiscTab.h"
#include "../System/UI/SettingsTab.h"

inline bool DrawRuntimeEspColorRow(const char *label, float *color) {
    if (label == nullptr || color == nullptr) {
        return false;
    }

    auto normalize = [](float v) {
        return (v <= 1.0f) ? ImClamp(v, 0.0f, 1.0f) : ImClamp(v / 255.0f, 0.0f, 1.0f);
    };

    float col[4] = {
        normalize(color[0]),
        normalize(color[1]),
        normalize(color[2]),
        normalize(color[3])
    };

    const bool changed = custom::ColorEdit4(
        label,
        col,
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_PickerHueWheel
    );

    if (changed) {
        color[0] = col[0] * 255.0f;
        color[1] = col[1] * 255.0f;
        color[2] = col[2] * 255.0f;
        color[3] = 255.0f;
    }

    return changed;
}

inline void CopyLinkedEspColors(float *lineColor, float *boxColor, float *nameColor, float *healthColor, float *distanceColor, float *skeletonColor) {
    if (lineColor == nullptr) {
        return;
    }

    if (boxColor) std::memcpy(boxColor, lineColor, sizeof(float) * 4);
    if (nameColor) std::memcpy(nameColor, lineColor, sizeof(float) * 4);
    if (healthColor) std::memcpy(healthColor, lineColor, sizeof(float) * 4);
    if (distanceColor) std::memcpy(distanceColor, lineColor, sizeof(float) * 4);
    if (skeletonColor) std::memcpy(skeletonColor, lineColor, sizeof(float) * 4);
}

inline void CollapseMenu(StateRefs &state) {
    state.windowCollapsed = true;
    state.isMenuVisible = false;
    state.collapseBarLastActiveTime = ImGui::GetTime();
    state.collapseBarOpacityAnim = 1.0f;
    state.collapseBarPressAnim = 0.0f;
}

} // namespace runtime_preview_menu
