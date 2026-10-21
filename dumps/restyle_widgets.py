#!/usr/bin/env python3
"""Rewrite the drawing parts of ImGui/custom_widgets.cpp to the IMGUI5 style.

Every edit replaces the range between two anchor strings (inclusive). Anchors are
short and unique, so the ImGui interaction code around them is untouched.
"""
import sys

path = "src/main/jni/ImGui/custom_widgets.cpp"
src = open(path, encoding="utf-8", errors="surrogateescape").read()
edits = []


def rep(start, end, new, what, after=None):
    global src
    base = 0
    if after is not None:
        base = src.find(after)
        if base < 0:
            raise SystemExit("scope marker missing (%s): %r" % (what, after))
    i = src.find(start, base)
    if i < 0:
        raise SystemExit("anchor missing (%s): %r" % (what, start[:70]))
    j = src.find(end, i)
    if j < 0:
        raise SystemExit("end anchor missing (%s): %r" % (what, end[:70]))
    j += len(end)
    edits.append(what)
    src = src[:i] + new + src[j:]


# -- 0: pull in the style module -------------------------------------------
rep('#include "custom_widgets.hpp"\n#include "imgui_settings.h"',
    '#include "custom_widgets.hpp"\n#include "imgui_settings.h"',
    '#include "custom_widgets.hpp"\n#include "imgui_settings.h"\n#include "nl_style.h"',
    "include nl_style.h")

# -- 1: checkbox -> IMGUI5 switch ------------------------------------------
rep("        const float row_overlay_alpha = 0.08f",
    "                GetWindowDrawList()->AddLine(p2, ImLerp(p2, p3, second_t), GetColorU32(c::checkbox::mark, it_anim->second.alpha_mark), 1.55f * control_scale);",
    """        // IMGUI5 switch (neverlose_menu.cpp Toggle): a 29x18 track, r9, with a
        // r7 knob - C(29,33,43) to the accent when on, knob C(133,144,156) to
        // C(248,249,252) - and the 1px accent glow around the track on hover.
        // The animated value IS the track's position, so it slides.
        const float toggle_scale = control_scale;
        const ImVec2 toggle_pos(total_bb.Max.x - 29.0f * toggle_scale - 2.0f * toggle_scale,
                                total_bb.Min.y + (height - 18.0f * toggle_scale) * 0.5f);
        nl5::Toggle(GetWindowDrawList(), toggle_pos, it_anim->second.alpha_mark,
                    it_anim->second.hover_t, true, toggle_scale);""",
    "checkbox -> IMGUI5 switch",
    'bool Checkbox(const char* label, bool* v)')

# -- 2: button label colour ------------------------------------------------
rep("""        const ImVec4 target_text = held
            ? c::text::text_active
            : hovered ? c::accent : c::text::text_active;""",
    """        const ImVec4 target_text = held
            ? c::text::text_active
            : hovered ? c::accent : c::text::text_active;""",
    """        // IMGUI5 frames keep their text light and let the border/fill carry the
        // hover, so the label goes from dim to bright instead of turning accent.
        const ImVec4 target_text = held
            ? c::text::text_active
            : hovered ? c::text::text_active : c::text::text_hov;""",
    "button label colour",
    'bool Button(const char* label, const ImVec2& size_arg)')

# -- 3: page/tab row + drop the underline ---------------------------------
rep("        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);",
    "GetColorU32(c::accent, it_anim->second.alpha_line), c::page::rounding * c::scale, ImDrawFlags_RoundCornersTop);",
    """        // IMGUI5 sidebar row (neverlose_menu.cpp nav()): flat until it is
        // hovered or selected, then the C(39,43,54) fill at r6. The accent
        // underline this used to draw is not part of that language.
        (void)show_active_line;
        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);""",
    "page row fill",
    'bool Page(bool selected, const char* icon')

rep("        it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::accent : hovered ? c::page::text_hov : c::page::text, g.IO.DeltaTime * 14.f);",
    "        it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::accent : hovered ? c::page::text_hov : c::page::text, g.IO.DeltaTime * 14.f);",
    "        // Selected and hovered rows share the bright label; the fill is what\n"
    "        // marks the selection.\n"
    "        it_anim->second.text = ImLerp(it_anim->second.text, selected || hovered ? c::page::text_hov : c::page::text, g.IO.DeltaTime * 14.f);",
    "page label colour",
    'bool Page(bool selected, const char* icon')

# -- 4: combo field -> IMGUI5 select frame ---------------------------------
rep("        const float field_rounding = 6.0f * control_scale;",
    "        PopClipRect();",
    """        // IMGUI5 select (neverlose_menu.cpp RowControl::Select): a flat
        // C(25,28,38) field that brightens to C(31,38,54) and takes the accent
        // border on hover, with the two-line chevron on the right.
        GetWindowDrawList()->AddText(total_bb.Min, GetColorU32(WithAlpha(c::text::text, 0.5f)), label, label_end);
        nl5::SelectField(GetWindowDrawList(), field_bb.Min, field_bb.Max,
                         it_anim->second.field_emphasis, control_scale);
        PushClipRect(field_bb.Min, field_bb.Max - ImVec2(18.0f * control_scale, 0.0f), true);
        GetWindowDrawList()->AddText(
            field_bb.Min + ImVec2(7.0f * control_scale, (field_height - CalcTextSize(preview_value ? preview_value : "None").y) * 0.5f),
            GetColorU32(nl5::FieldText),
            preview_value ? preview_value : "None"
        );
        PopClipRect();""",
    "combo field",
    'bool BeginCombo(const char* label, const char* preview_value')

# -- 5: combo arrow (drawn by SelectField now) ----------------------------
rep("        const float arrow_pad = 14.0f * control_scale;",
    """        GetWindowDrawList()->AddLine(
            ImVec2(arrow_center.x, arrow_center.y + arrow_size - arrow_offset),
            ImVec2(arrow_center.x + arrow_size, arrow_center.y - arrow_offset),
            GetColorU32(c::text::text_active),
            1.4f * control_scale
        );""",
    "",
    "combo arrow",
    'bool BeginCombo(const char* label, const char* preview_value')

# -- 6: popup panel --------------------------------------------------------
rep("        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.90f));",
    "        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * control_scale, 6.0f * control_scale));",
    """        // IMGUI5 popup: C(20,20,29,235) at r16 with the C(57,61,76,205) edge.
        PushStyleColor(ImGuiCol_PopupBg, ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 29.0f / 255.0f, 235.0f / 255.0f));
        PushStyleColor(ImGuiCol_WindowBg, ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 29.0f / 255.0f, 235.0f / 255.0f));
        PushStyleColor(ImGuiCol_Border, ImVec4(57.0f / 255.0f, 61.0f / 255.0f, 76.0f / 255.0f, 205.0f / 255.0f));
        PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f * control_scale);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f * control_scale, 4.0f * control_scale));
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);""",
    "combo popup panel",
    'bool BeginCombo(const char* label, const char* preview_value')

# -- 7: selectable rows ----------------------------------------------------
rep("        const bool popup_selectable = (window->Flags & ImGuiWindowFlags_Popup) != 0;",
    """        if (selected) {
            const float mark = 7.0f * c::scale;
            const ImVec2 mark_min(bb.Max.x - (mark + 10.0f * c::scale), bb.Min.y + (bb.GetHeight() - mark) * 0.5f);
            GetWindowDrawList()->AddCircleFilled(mark_min + ImVec2(mark * 0.5f, mark * 0.5f), mark * 0.5f, GetColorU32(c::accent), 18);
        }""",
    """        // IMGUI5 popup row (neverlose_menu.cpp PopupLayer): accent at 10% for
        // the hover, a check glyph when taken, no border and no fill otherwise.
        const bool popup_selectable = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const float row_hover = ImSaturate((hovered ? 1.0f : 0.0f) + (selected && popup_selectable ? 0.35f : 0.0f));
        if (popup_selectable) {
            if (row_hover > 0.01f)
                GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max,
                    nl5::WithAlphaU32(nl5::Accent, 25.0f / 255.0f * row_hover), 10.0f * c::scale);
            if (selected)
                nl5::Check(GetWindowDrawList(), ImVec2(bb.Min.x + 14.0f * c::scale,
                                                       bb.Min.y + bb.GetHeight() * 0.5f), nl5::CheckMark, c::scale);
        } else if (row_hover > 0.01f) {
            GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(c::page::background_active, row_hover), 6.0f * c::scale);
        }""",
    "selectable rows",
    'bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags')

# -- 8: slider value -> pill ----------------------------------------------
rep("""        GetWindowDrawList()->AddText(
            ImVec2(total_bb.Max.x - value_size.x, total_bb.Min.y + (header_height - value_size.y) * 0.5f),
            GetColorU32(WithAlpha(c::text::text_active, it_anim->second.value_alpha)),
            value_buf
        );""",
    """            value_buf
        );""",
    """        // IMGUI5 slider value: the 42x21 pill, right-aligned on the header row.
        nl5::ValuePill(GetWindowDrawList(),
                       ImVec2(total_bb.Max.x - 42.0f * control_scale, total_bb.Min.y),
                       value_buf, nullptr, control_scale);""",
    "slider value pill",
    'bool SliderScalar(const char* label')

# -- 9: slider track -------------------------------------------------------
rep("        const float track_thickness = ImLerp(3.0f * control_scale,",
    "        GetWindowDrawList()->AddRect(knob_min, knob_max, GetColorU32(WithAlpha(c::widget::outlinecolor, 0.72f + it_anim->second.active_t * 0.16f)), 3.5f * control_scale, 0, 1.0f * control_scale);",
    """        // IMGUI5 slider: 3px C(34,38,48) track, accent fill, r5.5 white knob.
        const ImVec2 track_start(slider_bb.Min.x + track_margin, frame_bb.Min.y + box_height * 0.5f);
        const float track_capacity = ImMax(1.0f, slider_bb.GetWidth() - track_margin * 2.0f);
        nl5::SliderTrack(GetWindowDrawList(), track_start, track_capacity,
                         it_anim->second.slow / track_capacity, control_scale);""",
    "slider track",
    'bool SliderScalar(const char* label')

# -- 10: child panel -> IMGUI5 card ---------------------------------------
rep("        const float rounding = 7.0f * c::scale;",
    "        const float rounding = 7.0f * c::scale;",
    """        const float rounding = 14.0f * c::scale;    // IMGUI5 card rounding""",
    "card rounding",
    'bool ChildEx(const char* name')

rep("        draw->AddRectFilled(panel_min, panel_max, GetColorU32(c::child::background), rounding);",
    "        draw->AddRectFilled(panel_min, panel_max, GetColorU32(c::child::background), rounding);",
    """        // IMGUI5 card: C(17,19,27,224) fill plus the C(31,34,44) edge.
        nl5::Card(draw, panel_min, panel_max, c::scale);""",
    "card fill",
    'bool ChildEx(const char* name')

rep("""                draw->AddText(
                    panel_min + ImVec2(text_x, ImMax(6.0f * c::scale, (cap_height - title_size.y) * 0.5f - 2.0f * c::scale)),
                    GetColorU32(c::accent),
                    name,
                    name_end
                );""",
    """                draw->AddText(
                    panel_min + ImVec2(text_x, ImMax(6.0f * c::scale, (cap_height - title_size.y) * 0.5f - 2.0f * c::scale)),
                    GetColorU32(c::accent),
                    name,
                    name_end
                );""",
    """                // IMGUI5 titles are dim grey; the icon keeps the accent.
                draw->AddText(
                    panel_min + ImVec2(text_x, ImMax(6.0f * c::scale, (cap_height - title_size.y) * 0.5f - 2.0f * c::scale)),
                    GetColorU32(c::text::text_active),
                    name,
                    name_end
                );""",
    "card title colour",
    'bool ChildEx(const char* name')

# -- 11: knob track colour -------------------------------------------------
rep("PathStroke(GetColorU32(c::elements::background), 0, thickness);",
    "PathStroke(GetColorU32(c::elements::background), 0, thickness);",
    "PathStroke(nl5::TrackBg, 0, thickness);",
    "knob track",
    'bool KnobScalar(const char* label')

open(path, "w", encoding="utf-8", errors="surrogateescape").write(src)
print("edits applied: %d" % len(edits))
for e in edits:
    print("  - %s" % e)
