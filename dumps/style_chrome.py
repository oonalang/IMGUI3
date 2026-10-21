import sys

def patch(path, edits):
    s = open(path, encoding="utf-8").read()
    for old, new in edits:
        n = s.count(old)
        if n != 1:
            print("MISS %s (%d): %r" % (path, n, old[:90]))
            sys.exit(1)
        s = s.replace(old, new)
    open(path, "w", encoding="utf-8").write(s)
    print("patched", path, len(edits), "edits")


# ------------------------------------------------------- imgui_settings.h ---
# The accent stays a live value: the hue card in the header drives it, and it
# starts on the exact IMGUI5 blue C(75,126,255).
patch("src/main/jni/ImGui/imgui_settings.h", [
    ("""    inline float g_menuHue = 0.76f;  // violet default for the runtime UI""",
     """    // IMGUI5 blue C(75,126,255): hue 0.62 at the saturation/value below.
    inline float g_menuHue = 0.6195f;"""),

    ("""    inline void ApplyAccentFromHue()
    {
        // Default to warm amber hue (~0.10 in HSV)
        ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.90f, 0.92f, menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
    }""",
     """    inline void ApplyAccentFromHue()
    {
        // Saturation/value are picked so the default hue lands on the IMGUI5
        // blue exactly: HSV(0.6195, 0.706, 1.0) == C(75,126,255). Any other hue
        // gives that hue at the same brightness instead of a muddier one.
        ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.706f, 1.0f, menu[0], menu[1], menu[2]);
        menu[3] = 1.0f;
    }"""),

    ("""        // IMGUI5 accent (neverlose_menu.cpp C(75,126,255)). The runtime hue used
        // to drive this and with it every accent in the menu.
        menu[0] = 75.0f / 255.0f;
        menu[1] = 126.0f / 255.0f;
        menu[2] = 255.0f / 255.0f;
        menu[3] = 1.0f;
        c::accent = ImColor(75, 126, 255);""",
     """        // The accent is the menu's one live colour - the hue card in the header
        // drives it and it starts on the IMGUI5 blue (neverlose_menu.cpp
        // C(75,126,255)). Everything the IMGUI5 palette below cannot fix to a
        // constant reads this.
        ApplyAccentFromHue();
        c::accent = ImColor(menu[0], menu[1], menu[2], menu[3]);"""),

    ("""    inline void ApplyIMGUI5Palette()
    {
        accent = ImColor(75, 126, 255);
        separator = ImColor(28, 31, 40);""",
     """    inline void ApplyIMGUI5Palette()
    {
        // The accent is not pinned here: it follows the hue card (which defaults
        // to this same blue), so recolouring the menu keeps every IMGUI5 drawer
        // on the colour the user picked.
        accent = ImColor(menu[0], menu[1], menu[2], menu[3]);
        separator = ImColor(28, 31, 40);"""),
])


# --------------------------------------------------------------- Main.cpp ---
# Window chrome: flat IMGUI5 surfaces with hairline edges instead of the
# accent-tinted borders and the navy sidebar.
patch("src/main/jni/Main.cpp", [
    # Body panel + edge.
    ("""                        runtimeDrawList->AddRect(bodyMin, bodyMax,
                                                 main_runtime_theme::GetAccentTintU32(0.70f, 0.70f),
                                                 16.0f, 0, 1.0f);""",
     """                        runtimeDrawList->AddRect(bodyMin, bodyMax, nl5::ShellEdge,
                                                 14.0f, 0, 1.0f);"""),

    # Header card.
    ("""                    runtimeDrawList->AddRectFilled(headerMin, headerMax, C(11, 13, 20, 225), 11.0f);
                    runtimeDrawList->AddRect(headerMin, headerMax, main_runtime_theme::GetAccentTintU32(0.70f, 0.70f), 11.0f, 0, 1.0f);""",
     """                    runtimeDrawList->AddRectFilled(headerMin, headerMax, nl5::ToolbarBg, 12.0f);
                    runtimeDrawList->AddRect(headerMin, headerMax, nl5::ToolbarLine, 12.0f, 0, 1.0f);"""),

    # Title / subtitle.
    ("""                        ImVec2(headerMin.x + 56.0f, headerMin.y + 11.0f), IM_COL32(245, 245, 250, 255), titleA);""",
     """                        ImVec2(headerMin.x + 56.0f, headerMin.y + 11.0f), nl5::TextBright, titleA);"""),
    ("""                    runtimeDrawList->AddText(runtimeTitleFont, 10.0f, subtitlePos, IM_COL32(135, 143, 165, 220), "PLAY LIKE A PLAYER  |  NOT A CHEATER");""",
     """                    runtimeDrawList->AddText(runtimeTitleFont, 10.0f, subtitlePos, nl5::Caption, "PLAY LIKE A PLAYER  |  NOT A CHEATER");"""),

    # Hue slider knob.
    ("""                    runtimeDrawList->AddCircleFilled(ImVec2(hueX, trackMin.y + 4.0f), 5.0f, IM_COL32(245,245,250,255), 20);""",
     """                    runtimeDrawList->AddCircleFilled(ImVec2(hueX, trackMin.y + 4.0f), 5.5f, nl5::SliderKnob, 20);"""),

    # Host panel: IMGUI5 sidebar grey plus its hairline.
    ("""                    runtimeDrawList->AddRectFilled(hostMin, hostMax, ImGui::GetColorU32(main_runtime_theme::GetSidebarShellBackgroundColor()), 11.0f);""",
     """                    runtimeDrawList->AddRectFilled(hostMin, hostMax, nl5::SidebarBg, 12.0f);
                    runtimeDrawList->AddRect(hostMin, hostMax, nl5::SidebarLine, 12.0f, 0, 1.0f);"""),

    # Sidebar panel.
    ("""                        runtimeDrawList->AddRectFilled(sidebarMin, sidebarMax,
                            ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor()), 9.0f);
                        runtimeDrawList->AddRect(sidebarMin, sidebarMax,
                            main_runtime_theme::GetAccentTintU32(0.22f, 0.35f), 9.0f, 0, 1.0f);""",
     """                        runtimeDrawList->AddRectFilled(sidebarMin, sidebarMax, nl5::CardBg, 10.0f);
                        runtimeDrawList->AddRect(sidebarMin, sidebarMax, nl5::CardEdge, 10.0f, 0, 1.0f);"""),

    # Sidebar rows: IMGUI5 nav fill, rounded, muted labels.
    ("""                                    act ? ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor())
                                        : IM_COL32(18, 20, 29, 150), 8.0f);""",
     """                                    act ? nl5::NavFill
                                        : IM_COL32(25, 28, 38, 200), 10.0f);"""),
    ("""                                    act ? main_runtime_theme::GetAccentTintU32(0.98f, 1.0f)
                                        : IM_COL32(154, 160, 182, hov ? 245 : 220),""",
     """                                    act ? nl5::AccentIcon()
                                        : (hov ? nl5::RowText : nl5::Ghost),"""),
    ("""                                    act ? IM_COL32(245, 245, 250, 255)
                                        : IM_COL32(154, 160, 182, hov ? 245 : 205),""",
     """                                    act ? nl5::TextBright
                                        : (hov ? nl5::RowText : nl5::Muted),"""),
])
