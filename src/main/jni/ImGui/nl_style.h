#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>

// ---------------------------------------------------------------------------
// Neverlose / HexSync look, taken from github.com/oonalang/IMGUI5
// (app/src/main/jni/menu/neverlose_menu.cpp and menu/gui.hpp).
//
// The values below are that menu's, verbatim: its card/row geometry, its track
// and knob sizes, its neutrals, and its motion speeds. What is NOT copied is
// the accent: this menu has a user-picked hue (main_runtime_theme / c::accent),
// so every accent usage here reads c::accent and keeps that control working.
//
// Widgets in custom_widgets.cpp draw through this header, so the reskin changes
// pixels only - every tab, ESP option and config binding is untouched.
namespace nl
{
    // ---- motion (IMGUI5: exponential smoothing, speed 16-22) --------------
    inline float Motion(ImGuiID id, float target, float speed = 16.0f, float initial = -1.0f)
    {
        float* value = ImGui::GetStateStorage()->GetFloatRef(id, initial < 0.0f ? target : initial);
        *value = ImLerp(*value, target, 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
        if (std::fabs(*value - target) < 0.0005f) *value = target;
        return *value;
    }

    inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t)
    {
        return ImVec4(ImLerp(a.x, b.x, t), ImLerp(a.y, b.y, t),
                      ImLerp(a.z, b.z, t), ImLerp(a.w, b.w, t));
    }

    inline ImVec4 Rgb(int r, int g, int b, int a = 255)
    {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    // ---- text sizes (IMGUI5 kText*) -------------------------------------
    inline float TextBody()    { return 15.0f; }
    inline float TextControl() { return 14.0f; }
    inline float TextSmall()   { return 12.0f; }
    inline float TextCaption() { return 10.0f; }
    inline float TextTitle()   { return 16.0f; }
    inline float TextIcon()    { return 13.0f; }

    // ---- neutrals --------------------------------------------------------
    inline ImVec4 Shell()          { return Rgb(13, 15, 22, 222); }
    inline ImVec4 Sidebar()        { return Rgb(18, 21, 30, 231); }
    inline ImVec4 Toolbar()        { return Rgb(11, 13, 20, 225); }
    inline ImVec4 Card()           { return Rgb(17, 19, 27, 224); }
    inline ImVec4 CardBorder()     { return Rgb(31, 34, 44); }
    inline ImVec4 CardCaption()    { return Rgb(89, 94, 106); }
    inline ImVec4 RowDivider()     { return Rgb(28, 31, 40); }

    inline ImVec4 Label()          { return Rgb(207, 209, 218); }
    inline ImVec4 LabelDim()       { return Rgb(92, 96, 107); }
    inline ImVec4 LabelMuted()     { return Rgb(145, 149, 159); }
    inline ImVec4 LabelBright()    { return Rgb(226, 228, 235); }

    inline ImVec4 NavHover()       { return Rgb(39, 43, 54); }
    inline ImVec4 Chevron()        { return Rgb(139, 143, 154); }

    // ---- controls -------------------------------------------------------
    // Toggle: 29x18 track, 9 px radius, 7 px knob, knob centre 9 -> 20 px.
    inline float ToggleWidth()  { return 29.0f; }
    inline float ToggleHeight() { return 18.0f; }
    inline float ToggleRadius() { return 9.0f; }
    inline float ToggleKnob()   { return 7.0f; }
    inline ImVec4 ToggleOff()   { return Rgb(29, 33, 43); }
    inline ImVec4 ToggleKnobOff() { return Rgb(133, 144, 156); }
    inline ImVec4 ToggleKnobOn()  { return Rgb(248, 249, 252); }

    // Select / dropdown: 23 px tall, 5 px radius, 134 px cap, 0.48 of the row.
    inline float SelectHeight() { return 23.0f; }
    inline float SelectRadius() { return 5.0f; }
    inline float SelectMaxWidth() { return 134.0f; }
    inline ImVec4 SelectFill()  { return Rgb(25, 28, 38); }
    inline ImVec4 SelectFillHover() { return Rgb(31, 38, 54); }
    inline ImVec4 SelectBorder() { return Rgb(32, 35, 46); }
    inline ImVec4 SelectText()  { return Rgb(170, 173, 184); }

    // Slider: 3 px track, 5.5 px knob, 42x21 value pill.
    inline float SliderTrack()  { return 3.0f; }
    inline float SliderKnob()   { return 5.5f; }
    inline ImVec4 SliderTrackOff() { return Rgb(34, 38, 48); }
    inline ImVec4 SliderKnobFill() { return Rgb(247, 248, 252); }
    inline ImVec4 PillFill()    { return Rgb(25, 28, 38); }
    inline ImVec4 PillText()    { return Rgb(166, 169, 179); }

    // Popup / dropdown list: 16 px radius, 32 px rows, accent hover at 25 alpha.
    inline float PopupRadius()  { return 16.0f; }
    inline float PopupRowHeight() { return 32.0f; }
    inline ImVec4 PopupFill()   { return Rgb(20, 20, 29, 235); }
    inline ImVec4 PopupBorder() { return Rgb(57, 61, 76, 205); }

    // Card: 14 px radius.
    inline float CardRadius()   { return 14.0f; }
}
