import re, sys

# ---------------------------------------------------------------------------
# Shell surfaces: the IMGUI5 frame language is flat fills, hairline edges, and
# one live accent. These edits take the shell drawers and the window chrome off
# the old tinted-gradient look.
# ---------------------------------------------------------------------------

def patch(path, edits):
    s = open(path, encoding="utf-8").read()
    for old, new in edits:
        n = s.count(old)
        if n != 1:
            print("MISS %s (%d): %r" % (path, n, old[:80]))
            sys.exit(1)
        s = s.replace(old, new)
    open(path, "w", encoding="utf-8").write(s)
    print("patched", path, len(edits), "edits")


# ------------------------------------------------------------ nl_style.h ----
patch("src/main/jni/ImGui/nl_style.h", [
    # The accent becomes live: the header's hue slider is the one control that
    # recolours the menu, and every IMGUI5 drawer reads through these.
    ("""    inline constexpr ImU32 Accent       = IM_COL32(75, 126, 255, 255);
    inline constexpr ImU32 AccentIcon   = IM_COL32(82, 141, 255, 255);
    inline constexpr ImU32 NavFill      = IM_COL32(39, 43, 54, 255);""",
     """    inline constexpr ImU32 NavFill      = IM_COL32(39, 43, 54, 255);

    // The accent is the one colour the menu lets the user retune (the hue card
    // in the header). It starts on the IMGUI5 blue C(75,126,255) and these two
    // read the live value, so a recoloured menu keeps a single accent. They are
    // defined below Lift(), which they use."""),

    ("""    inline ImU32 Lift(ImU32 color, float t)           // toward white
    {
        return Mix(color, IM_COL32(255, 255, 255, (color >> IM_COL32_A_SHIFT) & 0xFFu), t);
    }""",
     """    inline ImU32 Lift(ImU32 color, float t)           // toward white
    {
        return Mix(color, IM_COL32(255, 255, 255, (color >> IM_COL32_A_SHIFT) & 0xFFu), t);
    }

    inline ImU32 AccentColor() { return ImGui::GetColorU32(c::accent); }
    inline ImU32 AccentIcon()  { return Lift(AccentColor(), 0.06f); }"""),

    # --- Toggle: read the live accent.
    ("""        const ImU32 fill = enabled ? Mix(ToggleOff, Accent, on) : Shade(ToggleOff, 0.35f);""",
     """        const ImU32 fill = enabled ? Mix(ToggleOff, AccentColor(), on) : Shade(ToggleOff, 0.35f);"""),
    ("""                             WithAlphaU32(Accent, 45.0f / 255.0f * hover), 10.0f * s);""",
     """                             WithAlphaU32(AccentColor(), 45.0f / 255.0f * hover), 10.0f * s);"""),

    # --- Select field / slider / popup row.
    ("""        d->AddRect(min, max, Mix(FieldEdge, Accent, hover), r, 0, 1.0f * s);""",
     """        d->AddRect(min, max, Mix(FieldEdge, AccentColor(), hover), r, 0, 1.0f * s);"""),
    ("""        d->AddRectFilled(start, start + ImVec2(filled, t), Accent, 2.0f * s);""",
     """        d->AddRectFilled(start, start + ImVec2(filled, t), AccentColor(), 2.0f * s);"""),
    ("""            d->AddRectFilled(min, max, WithAlphaU32(Accent, 25.0f / 255.0f * hover), 10.0f * s);""",
     """            d->AddRectFilled(min, max, WithAlphaU32(AccentColor(), 25.0f / 255.0f * hover), 10.0f * s);"""),
])

# ------------------------------------------- custom_widgets.cpp shell -------
patch("src/main/jni/ImGui/custom_widgets.cpp", [
    ("""            drawList->AddRectFilled(
                min,
                max,
                ImGui::GetColorU32(highlighted ? main_runtime_theme::GetActiveTabBackgroundColor() : main_runtime_theme::GetSidebarShellBackgroundColor()),
                rounding
            );
            drawList->AddRectFilledMultiColor(
                min,
                ImVec2(max.x, min.y + (max.y - min.y) * 0.48f),
                main_runtime_theme::GetAccentTintU32(0.22f, highlighted ? 0.12f : 0.08f),
                main_runtime_theme::GetAccentTintU32(0.16f, highlighted ? 0.07f : 0.04f),
                IM_COL32(0, 0, 0, 0),
                IM_COL32(0, 0, 0, 0)
            );""",
     """            // IMGUI5 panel: flat C(17,19,27,224) with the C(31,34,44) hairline,
            // and the C(39,43,54) selected fill for the active card. The accent
            // gradient this used to wash over the top half is gone.
            drawList->AddRectFilled(min, max, highlighted ? nl5::NavFill : nl5::CardBg, rounding);
            drawList->AddRect(min, max, nl5::CardEdge, rounding, 0, 1.0f);"""),

    ("""                    active ? ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor()) : IM_COL32(18, 20, 29, 150),
                    9.0f""",
     """                    active ? nl5::NavFill : IM_COL32(25, 28, 38, 200),
                    10.0f"""),

    ("""                active ? main_runtime_theme::GetAccentTintU32(0.98f, 1.0f) : IM_COL32(154, 160, 182, hovered ? 245 : 220),
                icon""",
     """                active ? nl5::AccentIcon() : (hovered ? nl5::RowText : nl5::Ghost),
                icon"""),

    # --- Title card: bright label, accent suffix, no glow halo.
    ("""                // \"Zenx\" — static warm white
                drawList->AddText(
                    titleFont, titleFontSize,
                    ImVec2(startX, textY),
                    IM_COL32(228, 213, 185, 255),
                    title, pipe
                );""",
     """                // The static half is the IMGUI5 bright label; the suffix keeps
                // the accent. No glow any more - IMGUI5 text is flat.
                drawList->AddText(
                    titleFont, titleFontSize,
                    ImVec2(startX, textY),
                    nl5::TextBright,
                    title, pipe
                );"""),
    ("""                const ImVec4 acc = c::accent;
                const ImU32 accentCol = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 255
                );
                // Soft glow: two blurred shadow layers behind the accent text
                const ImU32 glowCol1 = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 55
                );
                const ImU32 glowCol2 = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 28
                );
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        if (dx == 0 && dy == 0) continue;
                        const float dist = (float)(dx * dx + dy * dy);
                        const ImU32 c = dist <= 2 ? glowCol1 : glowCol2;
                        drawList->AddText(titleFont, titleFontSize,
                            ImVec2(accentX + dx * 1.0f, textY + dy * 1.0f), c, accentPart);
                    }
                }
                drawList->AddText(titleFont, titleFontSize,
                    ImVec2(accentX, textY), accentCol, accentPart);""",
     """                drawList->AddText(titleFont, titleFontSize,
                    ImVec2(accentX, textY), nl5::AccentIcon(), accentPart);"""),
    ("""                    ImVec2(min.x + 14.0f, min.y + (size.y - titleSize.y) * 0.5f - 1.0f),
                    IM_COL32(228, 213, 185, 255),
                    title""",
     """                    ImVec2(min.x + 14.0f, min.y + (size.y - titleSize.y) * 0.5f - 1.0f),
                    nl5::TextBright,
                    title"""),

    # --- Hue card: IMGUI5 geometry (flat track, round white knob).
    ("""            const float knobX = ImLerp(trackMin.x, trackMax.x, main_runtime_theme::g_menuHue);
            const float knobHalfWidth = active ? 4.0f : 3.5f;
            const float knobHalfHeight = active ? 8.0f : 7.0f;
            const ImVec2 knobCenter(knobX, (trackMin.y + trackMax.y) * 0.5f);
            const ImVec2 knobMin(knobCenter.x - knobHalfWidth, knobCenter.y - knobHalfHeight);
            const ImVec2 knobMax(knobCenter.x + knobHalfWidth, knobCenter.y + knobHalfHeight);
            drawList->AddRectFilled(
                knobMin - ImVec2(2.0f, 2.0f),
                knobMax + ImVec2(2.0f, 2.0f),
                main_runtime_theme::GetAccentTintU32(0.18f, active ? 0.26f : 0.18f),
                6.0f
            );
            drawList->AddRectFilled(knobMin, knobMax, main_runtime_theme::GetAccentU32(), 4.0f);
            drawList->AddRectFilled(
                ImVec2(knobCenter.x - 1.0f, knobMin.y + 2.0f),
                ImVec2(knobCenter.x + 1.0f, knobMax.y - 2.0f),
                IM_COL32(246, 247, 251, 235),
                1.0f
            );""",
     """            // IMGUI5 slider knob: a plain white circle, r5.5, that grows a
            // little while it is being dragged.
            const float knobX = ImLerp(trackMin.x, trackMax.x, main_runtime_theme::g_menuHue);
            const ImVec2 knobCenter(knobX, (trackMin.y + trackMax.y) * 0.5f);
            drawList->AddCircleFilled(knobCenter, active ? 6.5f : 5.5f, nl5::SliderKnob);"""),

    ("""            drawList->AddRectFilled(
                ImVec2(trackMin.x - 4.0f, trackMin.y - 4.0f),
                ImVec2(trackMax.x + 4.0f, trackMax.y + 4.0f),
                IM_COL32(8, 9, 14, 220),
                5.0f
            );""",
     """            drawList->AddRectFilled(
                ImVec2(trackMin.x - 4.0f, trackMin.y - 4.0f),
                ImVec2(trackMax.x + 4.0f, trackMax.y + 4.0f),
                nl5::TrackBg,
                3.0f
            );"""),

    # --- Close card icon.
    ("""                hovered ? main_runtime_theme::GetAccentU32() : IM_COL32(238, 241, 249, 255),
                powerIcon""",
     """                hovered ? nl5::AccentIcon() : nl5::TextBright,
                powerIcon"""),
])
