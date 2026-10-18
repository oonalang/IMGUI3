#pragma once

//  nl_style.h  -  the IMGUI5 look, as constants and drawers.
//
//  Every colour, radius and motion speed below is read off the origin of this
//  style: oonalang/IMGUI5, app/src/main/jni/menu/neverlose_menu.cpp. Its menu
//  paints itself with the draw list instead of ImGui widgets, so the numbers are
//  the source of truth and the widget layer in custom_widgets.cpp pulls its
//  shapes from here instead of inventing them.
//
//    shell        C(13,15,22,222) r14        sidebar  C(18,21,30,231)
//    toolbar      C(11,13,20,225)            hairline C(27,30,39)
//    card         C(17,19,27,224) r14        edge     C(31,34,44)
//    card title   C(89,94,106) 10px          row line C(28,31,40)
//    row label    C(207,209,218) 15px        disabled C(92,96,107)
//    accent       C(75,126,255)              icon hot C(82,141,255)
//    toggle       29x18 r9 off C(29,33,43) -> accent; knob r7 C(133,144,156)
//                 -> C(248,249,252), hover glow accent@45 r10 inflated 1px
//    field        23px r5 C(25,28,38) -> C(31,38,54), edge C(32,35,46) -> accent
//    slider       track 3px r2 C(34,38,48), fill accent, knob r5.5 C(247,248,252)
//    value pill   42x21 r5 C(25,28,38), text C(166,169,179)
//    popup        C(20,20,29,235) r16, edge C(57,61,76,205), shadow C(0,0,0,55)
//                 r18, top hairline C(255,255,255,22), row 32px, hover
//                 accent@25 r10, check C(230,233,241)
//
//  Motion is the same everywhere in IMGUI5 - value += (target - value) * (1 -
//  exp(-speed * dt)) - which is what Motion() below does; the per-widget speeds
//  are passed in and match neverlose_menu.cpp (14-22).
//
//  Sizes are the IMGUI5 numbers multiplied by c::scale, because this menu draws
//  directly in surface pixels while IMGUI5 draws on a 750x756 canvas.

#include "imgui.h"
#include "imgui_internal.h"      // ImSaturate / ImLerp used by the drawers below
#include "imgui_settings.h"       // c::scale, c::accent

#include <cmath>

namespace nl5
{
    // ---------------------------------------------------------------- palette
    inline ImU32 C(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

    inline constexpr ImU32 ShellBg      = IM_COL32(13, 15, 22, 222);
    inline constexpr ImU32 ShellEdge    = IM_COL32(31, 34, 44, 255);
    inline constexpr ImU32 SidebarBg    = IM_COL32(18, 21, 30, 231);
    inline constexpr ImU32 SidebarLine  = IM_COL32(30, 33, 43, 255);
    inline constexpr ImU32 ToolbarBg    = IM_COL32(11, 13, 20, 225);
    inline constexpr ImU32 ToolbarLine  = IM_COL32(27, 30, 39, 255);

    inline constexpr ImU32 CardBg       = IM_COL32(17, 19, 27, 224);
    inline constexpr ImU32 CardEdge     = IM_COL32(31, 34, 44, 255);
    inline constexpr ImU32 TitleColor   = IM_COL32(89, 94, 106, 255);
    inline constexpr ImU32 CardCapBg    = IM_COL32(20, 23, 31, 255);
    inline constexpr ImU32 RowLine      = IM_COL32(28, 31, 40, 255);
    inline constexpr ImU32 RowText      = IM_COL32(207, 209, 218, 255);
    inline constexpr ImU32 RowDisabled  = IM_COL32(92, 96, 107, 255);
    inline constexpr ImU32 Caption      = IM_COL32(91, 96, 108, 255);
    inline constexpr ImU32 Muted        = IM_COL32(145, 149, 159, 255);
    inline constexpr ImU32 Ghost        = IM_COL32(137, 142, 153, 255);
    inline constexpr ImU32 ChevronCol   = IM_COL32(187, 190, 199, 255);
    inline constexpr ImU32 TextBright   = IM_COL32(226, 228, 235, 255);

    inline constexpr ImU32 NavFill      = IM_COL32(39, 43, 54, 255);

    // The accent is the one colour the menu lets the user retune (the hue card
    // in the header). It starts on the IMGUI5 blue C(75,126,255) and these two
    // read the live value, so a recoloured menu keeps a single accent. They are
    // defined below Lift(), which they use.

    inline constexpr ImU32 ToggleOff    = IM_COL32(29, 33, 43, 255);
    inline constexpr ImU32 ToggleKnobOff= IM_COL32(133, 144, 156, 255);
    inline constexpr ImU32 ToggleKnobOn = IM_COL32(248, 249, 252, 255);

    inline constexpr ImU32 FieldBg      = IM_COL32(25, 28, 38, 255);
    inline constexpr ImU32 FieldBgHover = IM_COL32(31, 38, 54, 255);
    inline constexpr ImU32 FieldEdge    = IM_COL32(32, 35, 46, 255);
    inline constexpr ImU32 FieldText    = IM_COL32(170, 173, 184, 255);

    inline constexpr ImU32 TrackBg      = IM_COL32(34, 38, 48, 255);
    inline constexpr ImU32 SliderKnob   = IM_COL32(247, 248, 252, 255);
    inline constexpr ImU32 PillText     = IM_COL32(166, 169, 179, 255);

    inline constexpr ImU32 PopupBg      = IM_COL32(20, 20, 29, 235);
    inline constexpr ImU32 PopupEdge    = IM_COL32(57, 61, 76, 205);
    inline constexpr ImU32 PopupShadow  = IM_COL32(0, 0, 0, 55);
    inline constexpr ImU32 PopupHigh    = IM_COL32(255, 255, 255, 22);
    inline constexpr ImU32 PopupText    = IM_COL32(182, 185, 196, 255);
    inline constexpr ImU32 PopupSel     = IM_COL32(224, 229, 243, 255);
    inline constexpr ImU32 CheckMark    = IM_COL32(230, 233, 241, 255);

    // ------------------------------------------------------------ text sizes
    inline constexpr float TextBody     = 15.0f;
    inline constexpr float TextControl  = 14.0f;
    inline constexpr float TextSmall    = 12.0f;
    inline constexpr float TextCaption  = 10.0f;
    inline constexpr float TextTitle    = 16.0f;
    inline constexpr float TextIcon     = 13.0f;

    // --------------------------------------------------------------- motion
    // IMGUI5 neverlose_menu.cpp Motion(): frame-rate independent approach that
    // settles to the target instead of easing a fixed step.
    inline float Motion(ImGuiID id, float target, float speed = 16.0f, float initial = -1.0f)
    {
        float* value = ImGui::GetStateStorage()->GetFloatRef(
            id, initial < 0.0f ? target : initial);
        *value = ImLerp(*value, target, 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
        if (std::fabs(*value - target) < 0.0005f)
            *value = target;
        return *value;
    }

    inline float MotionExpect(ImGuiID id, float target, float speed, float initial)
    {
        return Motion(id, target, speed, initial);
    }

    inline ImU32 Mix(ImU32 a, ImU32 b, float t)
    {
        return ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a),
                                                     ImGui::ColorConvertU32ToFloat4(b),
                                                     ImSaturate(t)));
    }

    inline ImU32 WithAlphaU32(ImU32 color, float alpha_mul)
    {
        const ImU32 a = (ImU32)(((color >> IM_COL32_A_SHIFT) & 0xFFu) * ImSaturate(alpha_mul));
        return (color & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
    }

    inline ImU32 Shade(ImU32 color, float t)          // toward black
    {
        return Mix(color, IM_COL32(0, 0, 0, (color >> IM_COL32_A_SHIFT) & 0xFFu), t);
    }

    inline ImU32 Lift(ImU32 color, float t)           // toward white
    {
        return Mix(color, IM_COL32(255, 255, 255, (color >> IM_COL32_A_SHIFT) & 0xFFu), t);
    }

    inline ImU32 AccentColor() { return ImGui::GetColorU32(c::accent); }
    inline ImU32 AccentIcon()  { return Lift(AccentColor(), 0.06f); }

    // --------------------------------------------------------------- drawers
    // Track + knob of the IMGUI5 switch. `p` is the top-left of the 29x18 track.
    inline void Toggle(ImDrawList* d, ImVec2 p, float on, float hover, bool enabled, float s = 1.0f)
    {
        const ImVec2 track(29.0f * s, 18.0f * s);
        const ImU32 fill = enabled ? Mix(ToggleOff, AccentColor(), on) : Shade(ToggleOff, 0.35f);
        const ImU32 knob = enabled ? Mix(ToggleKnobOff, ToggleKnobOn, on) : Muted;

        if (enabled && hover > 0.001f)
            d->AddRectFilled(p - ImVec2(1.0f * s, 1.0f * s) - ImVec2(hover * s, hover * s),
                             p + track + ImVec2(1.0f * s + hover * s, 1.0f * s + hover * s),
                             WithAlphaU32(AccentColor(), 45.0f / 255.0f * hover), 10.0f * s);

        d->AddRectFilled(p, p + track, fill, 9.0f * s);
        d->AddCircleFilled(p + ImVec2(ImLerp(9.0f * s, 20.0f * s, on), track.y * 0.5f),
                           7.0f * s, knob);
    }

    // The two-line chevron and check glyphs IMGUI5 draws instead of font icons.
    inline void Chevron(ImDrawList* d, ImVec2 p, ImU32 color = ChevronCol, float s = 1.0f)
    {
        d->AddLine(p, p + ImVec2(3.0f * s, 3.0f * s), color, 1.3f * s);
        d->AddLine(p + ImVec2(3.0f * s, 3.0f * s), p + ImVec2(0.0f, 6.0f * s), color, 1.3f * s);
    }

    inline void Check(ImDrawList* d, ImVec2 p, ImU32 color = CheckMark, float s = 1.0f)
    {
        d->AddLine(p, p + ImVec2(3.0f * s, 3.0f * s), color, 1.5f * s);
        d->AddLine(p + ImVec2(3.0f * s, 3.0f * s), p + ImVec2(8.0f * s, -3.0f * s), color, 1.5f * s);
    }

    // Card shell: 14px rounding, 1px edge. Takes the same (min, max) pair as its
    // siblings below - it used to take a size, and the one caller passed a max,
    // which drew every card a whole card too wide and too tall.
    inline void Card(ImDrawList* d, ImVec2 min, ImVec2 max, float s = 1.0f)
    {
        d->AddRectFilled(min, max, CardBg, 14.0f * s);
        d->AddRect(min, max, CardEdge, 14.0f * s, 0, 1.0f * s);
    }

    inline void CardTitle(ImDrawList* d, ImVec2 p, const char* title, ImFont* font, float s = 1.0f)
    {
        d->AddText(font ? font : ImGui::GetFont(), TextCaption * s,
                   p + ImVec2(12.0f * s, -18.0f * s), TitleColor, title);
    }

    // The dropdown / select frame: filled field that brightens and takes the
    // accent border on hover, with the chevron on the right.
    inline void SelectField(ImDrawList* d, ImVec2 min, ImVec2 max, float hover, float s = 1.0f)
    {
        const float r = 5.0f * s;
        d->AddRectFilled(min, max, Mix(FieldBg, FieldBgHover, hover), r);
        d->AddRect(min, max, Mix(FieldEdge, AccentColor(), hover), r, 0, 1.0f * s);
        const ImVec2 c(max.x - 11.0f * s, min.y + (max.y - min.y) * 0.5f);
        d->AddLine(c - ImVec2(4.0f * s, 2.0f * s), c + ImVec2(0.0f, 2.0f * s), ChevronCol, 1.0f * s);
        d->AddLine(c + ImVec2(0.0f, 2.0f * s), c + ImVec2(4.0f * s, -2.0f * s), ChevronCol, 1.0f * s);
    }

    // Slider: 3px track, accent fill, round white knob, value pill on the right.
    inline void SliderTrack(ImDrawList* d, ImVec2 start, float width, float amount, float s = 1.0f)
    {
        const float t = 3.0f * s;
        const float filled = ImClamp(amount, 0.0f, 1.0f) * width;
        d->AddRectFilled(start, start + ImVec2(width, t), TrackBg, 2.0f * s);
        d->AddRectFilled(start, start + ImVec2(filled, t), AccentColor(), 2.0f * s);
        d->AddCircleFilled(start + ImVec2(filled, t * 0.5f), 5.5f * s, SliderKnob);
    }

    inline void ValuePill(ImDrawList* d, ImVec2 p, const char* text, ImFont* font, float s = 1.0f)
    {
        const ImVec2 size(42.0f * s, 21.0f * s);
        d->AddRectFilled(p, p + size, FieldBg, 5.0f * s);
        ImFont* f = font ? font : ImGui::GetFont();
        const float tw = f->CalcTextSizeA(TextControl * s, FLT_MAX, 0.0f, text).x;
        const float th = f->CalcTextSizeA(TextControl * s, FLT_MAX, 0.0f, text).y;
        d->AddText(f, TextControl * s,
                   ImVec2(p.x + (size.x - tw) * 0.5f, p.y + (size.y - th) * 0.5f), PillText, text);
    }

    // Popup panel: shadow, 16px rounding, hairline highlight under the top edge.
    inline void PopupFrame(ImDrawList* d, ImVec2 p, ImVec2 size, float s = 1.0f)
    {
        d->AddRectFilled(p - ImVec2(5.0f * s, 2.0f * s), p + size + ImVec2(5.0f * s, 8.0f * s),
                         PopupShadow, 18.0f * s);
        d->AddRectFilled(p, p + size, PopupBg, 16.0f * s);
        d->AddRect(p, p + size, PopupEdge, 16.0f * s, 0, 1.0f * s);
        d->AddLine(p + ImVec2(16.0f * s, 1.0f * s), p + ImVec2(size.x - 16.0f * s, 1.0f * s),
                   PopupHigh, 1.0f * s);
    }

    inline void PopupRow(ImDrawList* d, ImVec2 min, ImVec2 max, float hover, bool selected,
                         const char* label, ImFont* font, float s = 1.0f)
    {
        if (hover > 0.001f)
            d->AddRectFilled(min, max, WithAlphaU32(AccentColor(), 25.0f / 255.0f * hover), 10.0f * s);
        if (selected)
            Check(d, min + ImVec2(14.0f * s, (max.y - min.y) * 0.5f), CheckMark, s);
        ImFont* f = font ? font : ImGui::GetFont();
        const float th = f->CalcTextSizeA(TextControl * s, FLT_MAX, 0.0f, label).y;
        d->AddText(f, TextControl * s,
                   ImVec2(min.x + 35.0f * s, min.y + ((max.y - min.y) - th) * 0.5f),
                   selected ? PopupSel : PopupText, label);
    }
}
