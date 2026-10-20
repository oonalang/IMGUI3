#pragma once

// ---------------------------------------------------------------------------
// Raspody theme - palette / metrics / animation state.
//
// Ported from github.com/eliwoahzja/raspody framework/settings (colors.h,
// elements.h, variables.h), stripped of every Win32 / DirectX field and wrapped
// in namespace raspody. The look is the one in the reference screenshot, moved
// from the raspody coral to a violet scheme: deep purple-charcoal panels, muted
// lilac text and the accent (168, 104, 255).
//
// All sizes below are the 900x592 design-space values. The menu computes a
// single scale from ComputeMenuLayout() and multiplies them with S(), so the
// same layout works on a 640px render surface and on a 1440p tablet.
// ---------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"

namespace raspody
{
    // ---------------------------------------------------------------------
    // Design scale (set by Render() every frame).
    // ---------------------------------------------------------------------
    inline float g_scale = 1.0f;

    inline float S(float v) { return v * g_scale; }
    inline ImVec2 S(float x, float y) { return ImVec2(x * g_scale, y * g_scale); }
    inline ImVec2 S(const ImVec2& v) { return ImVec2(v.x * g_scale, v.y * g_scale); }

    // ---------------------------------------------------------------------
    // Colour helpers.
    // ---------------------------------------------------------------------
    inline ImVec4 Rgb(int r, int g, int b, float a = 1.0f)
    {
        return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, a);
    }

    // Multiplies the style alpha as well, so fading a page with PushStyleVar
    // fades every custom-drawn rectangle and text run with it.
    inline ImU32 Col(const ImVec4& color, float alpha = 1.0f)
    {
        ImVec4 c = color;
        c.w *= alpha * ImGui::GetStyle().Alpha;
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    inline ImU32 Col(int r, int g, int b, float alpha = 1.0f)
    {
        return Col(Rgb(r, g, b), alpha);
    }

    inline ImVec4 WithAlpha(const ImVec4& color, float alpha)
    {
        return ImVec4(color.x, color.y, color.z, color.w * alpha);
    }

    // Raspody's fixed_speed(): speed / framerate, i.e. speed * dt.
    inline float Ease(float& value, float target, float speed)
    {
        const float t = ImMin(1.0f, ImGui::GetIO().DeltaTime * speed);
        value = ImLerp(value, target, t);
        return value;
    }

    // ---------------------------------------------------------------------
    // Palette - values copied from raspody framework/settings/colors.h.
    // ---------------------------------------------------------------------
    struct Palette
    {
        ImVec4 layout              = Rgb(19, 16, 27);
        ImVec4 child               = Rgb(26, 22, 36);
        ImVec4 widget              = Rgb(39, 33, 52);
        ImVec4 checkboxHover       = Rgb(53, 45, 70);
        ImVec4 circleCheckboxHover = Rgb(124, 105, 162);
        ImVec4 white               = Rgb(255, 255, 255);
        ImVec4 black               = Rgb(0, 0, 0);
        ImVec4 accent              = Rgb(168, 104, 255);
        ImVec4 accentSoft          = Rgb(196, 150, 255);
        ImVec4 text                = Rgb(108, 100, 134);
        ImVec4 text2               = Rgb(141, 132, 171);
        ImVec4 text3               = Rgb(180, 172, 206);
        ImVec4 popup               = Rgb(29, 25, 40);
        ImVec4 popupScrollBg       = Rgb(35, 30, 47);
        ImVec4 popupScrollThumb    = Rgb(48, 41, 63);
        ImVec4 avatarBorder        = Rgb(27, 23, 36);
        ImVec4 separator           = Rgb(46, 39, 61);
    };

    inline Palette clr;

    // ---------------------------------------------------------------------
    // Metrics - raspody framework/settings/elements.h, design-space values.
    // ---------------------------------------------------------------------
    struct Elements
    {
        struct
        {
            const char* name = "ETHNIR";
            float width      = 900.0f;
            float height     = 592.0f;
            float padding    = 15.0f;
            float outline    = 8.0f;
            float rounding   = 30.0f;
        } window;

        struct { float height = 50.0f; } header;

        struct
        {
            float width   = 90.0f;
            float height  = 497.0f;
            float padding = 20.0f;
        } tabBar;

        struct
        {
            float width   = 765.0f;
            float height  = 497.0f;
            float padding = 0.0f;
            float spacing = 0.0f;
        } content;

        struct
        {
            float height       = 50.0f;
            float leftPadding  = 12.0f;
            float spacing      = 12.0f;
            float rightPadding = 18.0f;
            float rounding     = 20.0f;
            float logoSize     = 34.0f;
            float calcWidth    = 0.0f;
        } brand;

        struct
        {
            float height    = 50.0f;
            float spacing   = 14.0f;
            float avatar    = 42.0f;
            float gear      = 18.0f;
            float calcWidth = 0.0f;
            ImVec2 topRight = ImVec2(0, 0);
        } profile;

        struct
        {
            float width       = 240.0f;
            float headerHeight= 92.0f;
            float avatar      = 54.0f;
            float rounding    = 20.0f;
            float padding     = 15.0f;
        } profileMenu;
    };

    inline Elements elements;

    // ---------------------------------------------------------------------
    // Animation / menu state - the useful subset of raspody's c_variables.
    // ---------------------------------------------------------------------
    struct Variables
    {
        struct
        {
            int current = 1;
            int stored  = 1;
            float alpha = 1.0f;
        } tab;

        struct
        {
            int current = 0;
            int stored  = 0;
            float alpha = 1.0f;
        } subtab;

        struct
        {
            float alpha = 0.0f;
            bool active = false;
        } blackout;

        struct { float width = 0.0f; } child;

        struct
        {
            bool init = false;
            ImVec2 lineMin = ImVec2(0, 0);
            ImVec2 lineMax = ImVec2(0, 0);
            ImVec2 lineMinStored = ImVec2(0, 0);
            ImVec2 lineMaxStored = ImVec2(0, 0);
        } tabBar;

        ImVec4 accent = Rgb(168, 104, 255);
    };

    inline Variables var;

    // Tab indices, in the order of the rail (raspody gui.cpp uses the same six
    // pages, only with Aimbot/Visuals/Weapons/Scripts/Configs/Settings labels).
    enum MenuTab
    {
        Tab_Aimbot = 0,
        Tab_Visuals,
        Tab_Weapons,
        Tab_Scripts,
        Tab_Configs,
        Tab_Settings,
        Tab_Count
    };

    enum TabIcon
    {
        Icon_Aimbot = 0,
        Icon_Visuals,
        Icon_Weapons,
        Icon_Scripts,
        Icon_Configs,
        Icon_Settings
    };

    inline const char* kTabNames[Tab_Count] = {
        "Aimbot", "Visuals", "Weapons", "Scripts", "Configs", "Settings"
    };

    inline const char* kTabDescriptions[Tab_Count] = {
        "Aim assist, silent aim and target selection.",
        "Player ESP, world visuals and colour control.",
        "Weapon handling, ammo and movement exploits.",
        "Timing patches, triggerbot and crosshair.",
        "Save, load and switch menu profiles.",
        "Game, camera and account-side switches."
    };

    inline const char* kSubtabNames[Tab_Count][3] = {
        { "Aim Assist", "Triggerbot", nullptr },
        { "Players", "World", nullptr },
        { "Weapon", "Movement", nullptr },
        { "Patches", "Crosshair", nullptr },
        { "Profiles", "Account", nullptr },
        { "Game", "System", nullptr }
    };

    inline const char* kSubtabDescriptions[Tab_Count][3] = {
        { "Smoothing, FOV and target priority.", "Automatic fire and dot crosshair.", nullptr },
        { "Everything drawn on players and bots.", "World, camera and awareness toggles.", nullptr },
        { "Handling, recoil and attachments.", "Speed, flight and slide tuning.", nullptr },
        { "Firing-timing patches and movement fixes.", "The dot that the trigger aims through.", nullptr },
        { "Write, load and manage your profiles.", "License, build and menu appearance.", nullptr },
        { "World physics, smoke and camera.", "Reports, input and performance.", nullptr }
    };

    // Popup bookkeeping shared by the widgets and the render loop.
    inline bool g_anyPopupOpen = false;
    inline bool g_popupOpenedThisFrame = false;
} // namespace raspody
