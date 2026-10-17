#ifndef IMGUI_SETTINGS_H
#define IMGUI_SETTINGS_H

#include "imgui.h"

extern float menu[4];
extern ImFont* F50;
extern ImFont* F107;

namespace font
{
    extern ImFont* icomoon_logo;
    extern ImFont* icomoon_page;
    extern ImFont* icon_notify;
    extern ImFont* regular_m;
    extern ImFont* regular_l;
    extern ImFont* inter_medium;
    extern ImFont* inter_semibold;
    extern ImFont* bold_font;
    extern ImFont* esp_font;

    inline float ui_scale = 1.0f;

    inline void SetDensity(float display_density)
    {
        ui_scale = display_density > 0.0f ? ImClamp(display_density / 2.75f, 0.8f, 2.5f) : 1.0f;
    }

    inline float UiScale()
    {
        return ui_scale;
    }
}

namespace c
{
    inline float scale = 1.5f;
    inline float widget_scale = 1.0f;
    inline ImVec4 accent = ImColor(173, 143, 233);
    inline ImVec4 second = ImColor(100, 92, 122);
    inline ImVec4 separator = ImColor(38, 42, 46);

    namespace bg
    {
        inline ImVec4 background = ImColor(22, 22, 22, 181);
        inline ImVec2 size = ImVec2(850, 596);
        inline float rounding = 15.f;
    }

    namespace child
    {
        inline ImVec4 background = ImColor(60, 60, 60, 64);
        inline ImVec4 cap = ImColor(30, 30, 32);
        inline float rounding = 8.f;
        inline float padding = 13.f;
        inline float spacing = 13.f;
    }

    namespace page
    {
        inline ImVec4 background_active = ImColor(173, 143, 233, 56);
        inline ImVec4 background = ImColor(38, 38, 42);

        inline ImVec4 text_hov = ImColor(205, 205, 205, 235);
        inline ImVec4 text = ImColor(150, 150, 150, 220);

        inline float rounding = 4.f;
    }

    namespace elements
    {
        inline ImVec4 background_hovered = ImColor(46, 46, 50);
        inline ImVec4 background = ImColor(34, 34, 37);
        inline float rounding = 2.5f;
    }

    namespace checkbox
    {
        inline ImVec4 mark = ImColor(22, 22, 22);
        inline ImVec4 background_on = ImColor(173, 143, 233);
        inline ImVec4 background_off = ImColor(46, 46, 50);
        inline ImVec4 circle_inactive = ImColor(100, 92, 122);
        inline float rounding = 2.5f;
    }

    namespace text
    {
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hov = ImColor(205, 205, 205, 235);
        inline ImVec4 text = ImColor(150, 150, 150, 220);
    }

    namespace widget
    {
        inline ImVec2 size = ImVec2(0, 34.f);
        inline ImVec4 background = ImColor(34, 34, 37);
        inline ImVec4 outlinecolor = ImColor(100, 92, 122, 160);
        inline float rounding = 2.5f;
        inline float outline = 1.f;
    }

    namespace button
    {
        inline ImVec4 background = ImColor(34, 34, 37);
        inline ImVec4 background_hovered = ImColor(46, 46, 50);
        inline ImVec4 background_active = ImColor(60, 60, 64);
        inline ImVec4 outline = ImColor(100, 92, 122, 190);
        inline float rounding = 2.5f;
    }

    namespace scrollbar
    {
        inline float hitbox_area = 24.f;
        inline float hitbox_extra = 24.f;
        inline bool left_side = false;
        inline float gutter_spacing = 4.f;
    }

    inline void ApplyMainWindowStyle(ImGuiStyle& style)
    {
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.ItemSpacing = ImVec2(10.0f * scale, 10.0f * scale);
        style.WindowBorderSize = 0.0f;
        style.ScrollbarSize = 8.0f * scale;
    }

    inline void UpdateTheme(bool dark_mode, const float* accent_rgba, float dt)
    {
        bg::background = ImLerp(bg::background, dark_mode ? ImColor(22, 22, 22, 181) : ImColor(255, 255, 255), dt * 12.0f);
        separator = ImLerp(separator, dark_mode ? ImColor(22, 23, 26) : ImColor(222, 228, 244), dt * 12.0f);

        const ImVec4 accent_target = dark_mode
            ? (accent_rgba ? ImVec4(accent_rgba[0], accent_rgba[1], accent_rgba[2], 1.0f) : ImColor(173, 143, 233).Value)
            : ImColor(128, 105, 190).Value;
        accent = ImLerp(accent, accent_target, dt * 12.0f);

        elements::background_hovered = ImLerp(elements::background_hovered, dark_mode ? ImColor(46, 46, 50) : ImColor(197, 207, 232), dt * 25.0f);
        elements::background = ImLerp(elements::background, dark_mode ? ImColor(34, 34, 37) : ImColor(222, 228, 244), dt * 25.0f);

        widget::background = ImLerp(widget::background, dark_mode ? ImColor(34, 34, 37) : ImColor(236, 240, 250), dt * 25.0f);
        widget::outlinecolor = ImLerp(widget::outlinecolor, dark_mode ? ImColor(100, 92, 122, 160) : ImColor(194, 204, 228), dt * 25.0f);
        button::background = ImLerp(button::background, dark_mode ? ImColor(34, 34, 37) : ImColor(236, 240, 250), dt * 25.0f);
        button::background_hovered = ImLerp(button::background_hovered, dark_mode ? ImColor(46, 46, 50) : ImColor(213, 222, 242), dt * 25.0f);
        button::background_active = ImLerp(button::background_active, dark_mode ? ImColor(60, 60, 64) : ImColor(196, 206, 232), dt * 25.0f);
        button::outline = ImLerp(button::outline, dark_mode ? ImColor(100, 92, 122, 190) : ImColor(177, 188, 217), dt * 25.0f);

        checkbox::mark = ImLerp(checkbox::mark, dark_mode ? ImColor(22, 22, 22) : ImColor(255, 255, 255), dt * 12.0f);
        checkbox::background_off = ImLerp(checkbox::background_off, dark_mode ? ImColor(46, 46, 50) : ImColor(205, 214, 236), dt * 25.0f);
        checkbox::circle_inactive = ImLerp(checkbox::circle_inactive, dark_mode ? ImColor(100, 92, 122) : ImColor(120, 130, 158), dt * 25.0f);

        child::background = ImLerp(child::background, dark_mode ? ImColor(60, 60, 60, 64) : ImColor(241, 243, 249), dt * 12.0f);
        child::cap = ImLerp(child::cap, dark_mode ? ImColor(30, 30, 32) : ImColor(228, 235, 248), dt * 12.0f);

        page::text_hov = ImLerp(page::text_hov, dark_mode ? ImColor(205, 205, 205, 235) : ImColor(136, 145, 176), dt * 12.0f);
        page::text = ImLerp(page::text, dark_mode ? ImColor(150, 150, 150, 220) : ImColor(136, 145, 176), dt * 12.0f);
        page::background_active = ImLerp(page::background_active, dark_mode ? ImColor(173, 143, 233, 56) : ImColor(196, 205, 228), dt * 25.0f);
        page::background = ImLerp(page::background, dark_mode ? ImColor(38, 38, 42) : ImColor(222, 228, 244), dt * 25.0f);

        text::text_active = ImLerp(text::text_active, dark_mode ? ImColor(255, 255, 255) : ImColor(0, 0, 0), dt * 12.0f);
        text::text_hov = ImLerp(text::text_hov, dark_mode ? ImColor(205, 205, 205, 235) : ImColor(68, 71, 85), dt * 12.0f);
        text::text = ImLerp(text::text, dark_mode ? ImColor(150, 150, 150, 220) : ImColor(68, 71, 85), dt * 12.0f);
    }
}

namespace main_runtime_theme
{
    inline float g_menuHue = 0.7222f;

    inline ImVec4 GetAccentVec4(float alpha = 1.0f)
    {
        return ImVec4(menu[0], menu[1], menu[2], alpha);
    }

    inline ImU32 GetAccentU32(float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentVec4(alpha));
    }

    inline ImVec4 GetAccentTint(float strength, float alpha = 1.0f)
    {
        return ImVec4(menu[0] * strength, menu[1] * strength, menu[2] * strength, alpha);
    }

    inline ImU32 GetAccentTintU32(float strength, float alpha = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(GetAccentTint(strength, alpha));
    }

    inline void ApplyAccentFromHue()
    {
        ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.386f, 0.914f, menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
    }

    inline float GetContentPadding()
    {
        return 10.0f;
    }

    inline float GetColumnGap()
    {
        return 10.0f;
    }

    inline float GetChildPadding()
    {
        return 10.0f;
    }

    inline ImVec4 GetSidebarShellBackgroundColor()
    {
        return ImColor(0, 0, 0, 130);
    }

    inline ImVec4 GetActiveTabBackgroundColor()
    {
        return ImColor(0, 0, 0, 150);
    }

    inline void ApplyThemeState()
    {
        c::scale = 1.15f;
        c::widget_scale = 1.45f;
        c::accent = ImColor(GetAccentVec4());
        c::second = ImColor(100, 92, 122);
        c::separator = ImColor(c::second.x, c::second.y, c::second.z, 0.55f);

        c::bg::background = ImColor(22, 22, 22, 181);
        c::child::background = ImColor(60, 60, 60, 64);
        c::child::cap = ImColor(30, 30, 32, 220);
        c::child::padding = GetChildPadding() / c::scale;
        c::child::spacing = GetChildPadding() / c::scale;

        c::page::background_active = ImColor(GetAccentTint(1.0f, 0.22f));
        c::page::background = ImColor(38, 38, 42, 200);
        c::page::text_hov = ImColor(205, 205, 205, 235);
        c::page::text = ImColor(150, 150, 150, 220);

        c::elements::background_hovered = ImColor(46, 46, 50, 235);
        c::elements::background = ImColor(34, 34, 37, 225);

        c::checkbox::mark = ImColor(22, 22, 22);
        c::checkbox::background_on = ImColor(GetAccentTint(0.92f, 0.96f));
        c::checkbox::background_off = ImColor(46, 46, 50, 230);
        c::checkbox::circle_inactive = ImColor(100, 92, 122, 230);

        c::text::text_active = ImColor(255, 255, 255);
        c::text::text_hov = ImColor(205, 205, 205, 235);
        c::text::text = ImColor(150, 150, 150, 220);

        c::widget::background = ImColor(34, 34, 37, 235);
        c::widget::outlinecolor = ImColor(c::second.x, c::second.y, c::second.z, 0.63f);

        c::button::background = ImColor(34, 34, 37, 235);
        c::button::background_hovered = ImColor(46, 46, 50, 245);
        c::button::background_active = ImColor(60, 60, 64, 250);
        c::button::outline = ImColor(c::second.x, c::second.y, c::second.z, 0.75f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScrollbarSize = 5.0f;
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.72f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.68f, 0.56f, 0.91f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.75f, 0.65f, 0.95f, 0.90f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.68f, 0.56f, 0.91f, 1.0f);
    }
}

#endif // IMGUI_SETTINGS_H
