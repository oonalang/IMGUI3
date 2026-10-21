// Host check for the IMGUI5 restyle: drives the real widget layer through
// ImGui frames with no backend (the null renderer ImGui supports) and asserts
// the two things a restyle can quietly break:
//
//   * the style stacks balance - every Push inside a widget is matched. Each
//     control is exercised on its own frame and g.StyleVarStack.Size /
//     g.ColorStack.Size are compared before and after the call, so a leak names
//     the widget instead of just failing the frame;
//   * the widgets run at all, with ImGui's own IM_ASSERTs live (built without
//     NDEBUG), which is what catches a bad Begin/End, child or popup pairing.
//
// It also pins the restyle's two visible promises: the accent starts on the
// IMGUI5 blue and the hue card still moves it.
//
// Build (from src/main/jni):
//   g++ -std=c++17 -g -DIMGUI_DEFINE_MATH_OPERATORS -I. -IImGui -IIncludes \
//       ../../../dumps/im5_host_check.cpp ImGui/custom_widgets.cpp ImGui/imgui.cpp \
//       ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp \
//       -o /tmp/im5_host_check && /tmp/im5_host_check
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGui/custom_widgets.hpp"
#include "ImGui/imgui_settings.h"
#include "ImGui/nl_style.h"
#include "Fonts/fonts.h"          // inter_semibold / icomoon_page, the app's own fonts
#include "Fonts/Iconcpp.h"        // FontAwesome, for the sidebar / header icons
#include "Fonts/SPECIAL.h"        // F50, the title font
#include "im5_soft_render.h"

// Globals the widget layer expects the app to provide.
ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = { 75.0f / 255.0f, 126.0f / 255.0f, 255.0f / 255.0f, 1.0f };
namespace font { ImFont* inter_semibold = nullptr; ImFont* icomoon_page = nullptr; }

#include <cstdio>
#include <cmath>
#include <cstring>

static int g_failures = 0;
#define CHECK(cond, msg)                                                \
    do {                                                                \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);   \
                       g_failures++; }                                  \
        else         { printf("ok  : %s\n", msg); }                     \
    } while (0)

static bool  g_checkbox = false;
static float g_slider = 0.5f;
static int   g_combo = 1;
static bool  g_multi[4] = { true, false, true, false };
static int   g_key = 0, g_key_mode = 0;
static float g_color[4] = { 0.3f, 0.5f, 0.9f, 1.0f };

static const char* kItems[] = { "Alpha", "Bravo", "Charlie", "Delta" };
static const char* kMulti[] = { "One", "Two", "Three", "Four" };

static ImGuiContext* g_ctx = nullptr;

// ------------------------------------------------------------------ cases ---
static void W_Checkbox()      { custom::Checkbox("checkbox", &g_checkbox); }
static void W_CheckboxClick() { custom::CheckboxClicked("clicked", &g_checkbox); }
static void W_Slider()        { custom::SliderFloat("slider", &g_slider, 0.0f, 1.0f, "%.2f"); }
static void W_Knob()          { custom::KnobFloat("knob", &g_slider, 0.0f, 1.0f, "%.1f"); }
static void W_Combo()         { custom::Combo("combo", &g_combo, kItems, IM_ARRAYSIZE(kItems)); }
static void W_MultiCombo()    { custom::MultiCombo("multi", g_multi, kMulti, IM_ARRAYSIZE(kMulti)); }
static void W_ColorButton()   { custom::ColorButton("colour", ImVec4(g_color[0], g_color[1], g_color[2], g_color[3])); }
static void W_Keybind()       { custom::Keybind("keybind", &g_key, &g_key_mode); }
static void W_Button()        { custom::Button("action", ImVec2(160.0f, 34.0f)); }
static void W_ThemeButton()   { custom::ThemeButton("theme", true, ImVec2(120.0f, 30.0f)); }
static void W_Page()          { custom::Page(true, "p", ImVec2(120.0f, 30.0f), false, true); }
static void W_Separator()     { custom::Separator(); }
static void W_Selectable()    { custom::Selectable("selectable"); }

static void W_BeginCombo()
{
    if (custom::BeginCombo("begin_combo", "preview", 0, false))
    {
        custom::Selectable("row");
        custom::EndCombo();
    }
}

static void W_PopupSelectable()
{
    // The popup row path: the check glyph and hover fill only run in a popup
    // window, which is where they leaked before.
    if (ImGui::BeginPopup("host_popup"))
    {
        custom::Selectable("row one");
        custom::Selectable("row two", true);
        ImGui::EndPopup();
    }
}

static void W_Child()
{
    const bool open = custom::Child("child", ImVec2(280.0f, 110.0f), true);
    if (open)
        custom::Checkbox("in child", &g_checkbox);
    custom::EndChild();
}

static void W_Group()
{
    custom::BeginGroup();
    custom::Checkbox("in group", &g_checkbox);
    custom::EndGroup();
}

static void W_ShellDrawers()
{
    ImDrawList* d = ImGui::GetWindowDrawList();
    custom::shell::DrawPanelShell(d, ImVec2(20.0f, 60.0f), ImVec2(260.0f, 130.0f), 14.0f, false);
    custom::shell::DrawPanelShell(d, ImVec2(280.0f, 60.0f), ImVec2(520.0f, 130.0f), 14.0f, true);
    custom::shell::DrawTitleCard(ImVec2(20.0f, 140.0f), ImVec2(500.0f, 40.0f), "ETHNIR|NOIR");
    custom::shell::DrawMenuColorCard(ImVec2(20.0f, 190.0f), ImVec2(500.0f, 36.0f));
    custom::shell::DrawCloseCard(ImVec2(20.0f, 235.0f), ImVec2(72.0f, 44.0f));
    custom::shell::DrawSidebarButton("##nav", "n", true, ImVec2(20.0f, 290.0f), ImVec2(40.0f, 40.0f));
}

// The one-frame pass: every control in a single frame, the way a tab draws them.
static void W_Everything()
{
    static const char* multi_labels[] = { "One", "Two", "Three", "Four" };
    custom::Checkbox("Hitbox", &g_checkbox);
    custom::Checkbox("No Recoil", &g_checkbox);
    custom::CheckboxClicked("Clicked", &g_checkbox);
    custom::SliderFloat("SpeedHack", &g_slider, 0.5f, 2.0f, "%.1fx");
    custom::KnobFloat("Knob", &g_slider, 0.0f, 1.0f, "%.1f");
    custom::Combo("Select", &g_combo, kItems, IM_ARRAYSIZE(kItems));
    custom::MultiCombo("Multi", g_multi, multi_labels, 4);
    custom::Button("Action", ImVec2(160.0f, 34.0f));
    custom::ThemeButton("theme", true, ImVec2(120.0f, 30.0f));
    custom::Page(true, "p", ImVec2(120.0f, 30.0f), false, true);
    custom::Page(false, "p", ImVec2(120.0f, 30.0f), false, true);
    custom::Separator();
    custom::ColorButton("colour", ImVec4(g_color[0], g_color[1], g_color[2], g_color[3]));
    custom::Keybind("Keybind", &g_key, &g_key_mode);
    W_ShellDrawers();
}

// ------------------------------------------------------------------ frame ---
struct WidgetCase
{
    const char* name;
    void (*fn)();
};

static const WidgetCase kCases[] = {
    { "Checkbox",        W_Checkbox      },
    { "CheckboxClicked", W_CheckboxClick },
    { "SliderFloat",     W_Slider        },
    { "KnobFloat",       W_Knob          },
    { "Combo",           W_Combo         },
    { "MultiCombo",      W_MultiCombo    },
    { "ColorButton",     W_ColorButton   },
    { "Keybind",         W_Keybind       },
    { "Button",          W_Button        },
    { "ThemeButton",     W_ThemeButton   },
    { "Page",            W_Page          },
    { "Separator",       W_Separator     },
    { "Selectable",      W_Selectable    },
    { "BeginCombo",      W_BeginCombo    },
    { "Child",           W_Child         },
    { "BeginGroup",      W_Group         },
    { "shell drawers",   W_ShellDrawers  },
};

// Runs one call in a real window/frame and reports whether it left style state
// behind. Returns true when the widget balanced its pushes.
static bool RunOne(const WidgetCase& c)
{
    const int style_before = g_ctx->StyleVarStack.Size;
    const int color_before = g_ctx->ColorStack.Size;

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
    ImGui::Begin("case", nullptr, ImGuiWindowFlags_NoSavedSettings);
    c.fn();
    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();

    const int style_after = g_ctx->StyleVarStack.Size;
    const int color_after = g_ctx->ColorStack.Size;
    if (style_after != style_before || color_after != color_before)
    {
        printf("FAIL: %s leaked style state (style %d->%d, color %d->%d)\n",
               c.name, style_before, style_after, color_before, color_after);
        return false;
    }
    return true;
}

// The dropdown's popup frame is a separate code path: the style pushes at the
// top of BeginCombo only run once the popup is actually open, so it needs a real
// click to reach them. This clicks the field with queued mouse input, keeps the
// popup open for a few frames and checks the stacks on every one of them.
// Returns: 1 when the popup path was reached and balanced, 0 otherwise.
static bool ComboPopupCheck()
{
    ImGuiIO& io = ImGui::GetIO();
    bool popup_seen = false;
    bool balanced = true;
    ImVec2 field_center(0.0f, 0.0f);

    for (int frame = 0; frame < 8; ++frame)
    {
        if (frame == 1) io.AddMousePosEvent(field_center.x, field_center.y);
        if (frame == 2) io.AddMouseButtonEvent(0, true);
        if (frame == 3) io.AddMouseButtonEvent(0, false);
        if (frame == 6) io.AddKeyEvent(ImGuiKey_Escape, true);
        if (frame == 7) io.AddKeyEvent(ImGuiKey_Escape, false);

        const int style_before = g_ctx->StyleVarStack.Size;
        const int color_before = g_ctx->ColorStack.Size;

        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
        ImGui::Begin("combo host", nullptr, ImGuiWindowFlags_NoSavedSettings);
        custom::Combo("Target", &g_combo, kItems, IM_ARRAYSIZE(kItems));
        field_center = (ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f;
        ImGui::End();
        ImGui::EndFrame();
        ImGui::Render();

        if (g_ctx->StyleVarStack.Size != style_before ||
            g_ctx->ColorStack.Size != color_before)
        {
            printf("FAIL: the dropdown popup leaked style state on frame %d "
                   "(style %d->%d, color %d->%d)\n", frame, style_before,
                   g_ctx->StyleVarStack.Size, color_before, g_ctx->ColorStack.Size);
            balanced = false;
        }

        if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            popup_seen = true;
    }

    if (!popup_seen)
        printf("FAIL: the dropdown popup never opened, so its push/pop pair was not tested\n");
    return popup_seen && balanced;
}

// ---------------------------------------------------------------- preview ---
// A menu-shaped frame: the header cards, two content cards and an open dropdown,
// drawn only with the widget layer and the shell drawers. Used by --shot to
// rasterize a picture of the restyle on the host (no GPU, no device).
static void DrawShotScene(int click_combo)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("shot", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float pad = 14.0f;

    // Shell + header, the way Main.cpp draws them.
    dl->AddRectFilled(ImVec2(pad, pad), ImVec2(io.DisplaySize.x - pad, io.DisplaySize.y - pad),
                      nl5::ShellBg, 14.0f);
    dl->AddRect(ImVec2(pad, pad), ImVec2(io.DisplaySize.x - pad, io.DisplaySize.y - pad),
                nl5::ShellEdge, 14.0f, 0, 1.0f);
    dl->AddRectFilled(ImVec2(pad, pad), ImVec2(io.DisplaySize.x - pad, pad + 64.0f),
                      nl5::ToolbarBg, 12.0f);
    dl->AddRect(ImVec2(pad, pad), ImVec2(io.DisplaySize.x - pad, pad + 64.0f),
                nl5::ToolbarLine, 12.0f, 0, 1.0f);

    ImFont* titleFont = F50 ? F50 : ImGui::GetFont();
    dl->AddText(titleFont, 22.0f, ImVec2(pad + 56.0f, pad + 11.0f), nl5::TextBright, "ETHNIR NOIR");
    dl->AddText(titleFont, 10.0f, ImVec2(pad + 57.0f, pad + 38.0f), nl5::Caption,
                "PLAY LIKE A PLAYER  |  NOT A CHEATER");

    custom::shell::DrawCloseCard(ImVec2(io.DisplaySize.x - pad - 72.0f, pad), ImVec2(72.0f, 64.0f));

    // Host area + sidebar, so the nav rows are in the picture too.
    const ImVec2 host_min(pad, pad + 64.0f + 8.0f);
    const ImVec2 host_max(io.DisplaySize.x - pad, io.DisplaySize.y - pad);
    dl->AddRectFilled(host_min, host_max, nl5::SidebarBg, 12.0f);
    dl->AddRect(host_min, host_max, nl5::SidebarLine, 12.0f, 0, 1.0f);

    const ImVec2 panel_min(host_min.x + 10.0f, host_min.y + 10.0f);
    const float panel_w = 150.0f;
    const ImVec2 panel_max(panel_min.x + panel_w, host_max.y - 10.0f);
    dl->AddRectFilled(panel_min, panel_max, nl5::CardBg, 10.0f);
    dl->AddRect(panel_min, panel_max, nl5::CardEdge, 10.0f, 0, 1.0f);

    static const char* nav_names[] = { "HOME", "ESP", "AIM", "SKINS", "MISC", "SETTINGS" };
    ImFont* navFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    for (int i = 0; i < 6; ++i)
    {
        const ImVec2 r_min(panel_min.x + 8.0f, panel_min.y + 8.0f + i * 40.0f);
        const ImVec2 r_max(panel_max.x - 8.0f, r_min.y + 34.0f);
        const bool act = (i == 1);
        if (act)
        {
            dl->AddRectFilled(r_min, r_max, nl5::NavFill, 10.0f);
            dl->AddRectFilled(r_min, ImVec2(r_min.x + 3.0f, r_max.y), nl5::AccentColor(), 2.0f);
        }
        dl->AddText(navFont, 12.5f, ImVec2(r_min.x + 12.0f, r_min.y + 10.0f),
                    act ? nl5::TextBright : nl5::Ghost, nav_names[i]);
    }

    const ImVec2 content_min(panel_max.x + 10.0f, host_min.y + 10.0f);
    ImGui::SetCursorScreenPos(content_min);
    ImGui::BeginChild("shot_content", ImVec2(host_max.x - content_min.x - 10.0f,
                                             host_max.y - content_min.y - 10.0f),
                      false, ImGuiWindowFlags_NoBackground);
    if (font::inter_semibold)
        ImGui::PushFont(font::inter_semibold);

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    const float col_w = (ImGui::GetContentRegionAvail().x - 10.0f) * 0.5f;

    // custom::Child with cap=true is what BeginContentChild wraps in the menu.
    const bool left_open = custom::Child("EXPLOITS", ImVec2(col_w, 300.0f), true);
    custom::Checkbox("Hitbox", &g_checkbox);
    custom::Checkbox("No Recoil", &g_checkbox);
    g_checkbox = true;
    custom::Checkbox("No Spread", &g_checkbox);
    g_checkbox = false;
    custom::Separator();
    custom::SliderFloat("SpeedHack", &g_slider, 0.5f, 2.0f, "%.2fx");
    custom::KnobFloat("High Jump", &g_slider, 0.5f, 5.0f, "%.2fx");
    custom::EndChild();

    ImGui::SameLine(0.0f, 10.0f);
    const bool right_open = custom::Child("COMBAT", ImVec2(col_w, 300.0f), true);
    custom::Combo("Target", &g_combo, kItems, IM_ARRAYSIZE(kItems));
    if (click_combo == 1)
    {
        // Queue a hover + press over the field: ImGui processes both at the next
        // NewFrame, which is where the dropdown opens.
        const ImVec2 c = (ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f;
        io.AddMousePosEvent(c.x, c.y);
        io.AddMouseButtonEvent(0, true);
    }
    else if (click_combo == 2)
    {
        io.AddMouseButtonEvent(0, false);
    }
    custom::Button("APPLY", ImVec2(col_w - 26.0f, 34.0f));
    custom::Page(true, "p", ImVec2(col_w - 26.0f, 30.0f), false, true);
    custom::Page(false, "p", ImVec2(col_w - 26.0f, 30.0f), false, true);
    custom::ColorButton("colour", ImVec4(g_color[0], g_color[1], g_color[2], g_color[3]));
    custom::Keybind("Keybind", &g_key, &g_key_mode);
    custom::EndChild();

    (void)left_open;
    (void)right_open;

    if (font::inter_semibold)
        ImGui::PopFont();
    ImGui::EndChild();
    ImGui::End();
}

int main(int argc, char** argv)
{
    if (argc == 3 && strcmp(argv[1], "--shot") == 0)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(900.0f, 560.0f);
        io.DeltaTime = 1.0f / 60.0f;

        // The app's real fonts (Fonts/fonts.h, Iconcpp.h, SPECIAL.h).
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.PixelSnapH = true;
        font::inter_semibold = io.Fonts->AddFontFromMemoryTTF((void*)inter_semibold, sizeof(inter_semibold), 16.0f, &cfg);
        static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
        ImFontConfig iconsCfg;
        iconsCfg.MergeMode = false;
        iconsCfg.FontDataOwnedByAtlas = false;
        F107 = io.Fonts->AddFontFromMemoryCompressedTTF((void*)font_awesome_data1, (int)font_awesome_size1, 25.0f, &iconsCfg, icons_ranges);
        // Owned=false for every font: these are static arrays in the repo, and
        // ImGui frees owned font data when the atlas is destroyed.
        ImFontConfig f50Cfg;
        f50Cfg.FontDataOwnedByAtlas = false;
        F50 = io.Fonts->AddFontFromMemoryTTF((void*)F50_data, F50_size, 30.0f, &f50Cfg, io.Fonts->GetGlyphRangesDefault());
        if (!F107) F107 = font::inter_semibold;
        if (font::inter_semibold) io.FontDefault = font::inter_semibold;
        io.Fonts->Build();

        main_runtime_theme::ApplyThemeState();
        c::ApplyMainWindowStyle(ImGui::GetStyle());

        // Long enough for the widgets' own lerps (10-14 per second) to settle:
        // the labels fade in, and a short run would rasterize them mid-fade.
        // Frame 1 presses on the dropdown, frame 2 releases, and the last frame
        // is the one rasterized - by then the popup is open.
        for (int frame = 0; frame < 60; ++frame)
        {
            ImGui::NewFrame();
            DrawShotScene(frame == 1 ? 1 : (frame == 2 ? 2 : 0));
            ImGui::EndFrame();
            ImGui::Render();
        }
        if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            fprintf(stderr, "NOTE: the dropdown popup is not open in the shot frame\n");
        const bool ok = im5shot::RenderToRaw(argv[2], 2.0f, IM_COL32(8, 9, 13, 255));
        ImGui::DestroyContext();
        return ok ? 0 : 1;
    }

    printf("== IMGUI5 restyle: widget layer host check ==\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    g_ctx = ImGui::GetCurrentContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    // The menu applies the IMGUI5 palette and its scales every frame.
    main_runtime_theme::ApplyThemeState();
    c::ApplyMainWindowStyle(ImGui::GetStyle());

    // --- the two visible promises of the restyle ---------------------------
    const ImU32 rendered_accent = main_runtime_theme::GetAccentU32();
    CHECK(rendered_accent == IM_COL32(75, 126, 255, 255),
          "the default accent renders as the IMGUI5 blue C(75,126,255)");
    CHECK(c::child::rounding == 14.0f && c::bg::rounding == 14.0f,
          "cards and shell carry the IMGUI5 14px rounding");
    CHECK(c::checkbox::background_on.x == 75.0f / 255.0f,
          "the checkbox palette is the IMGUI5 accent");
    CHECK(c::child::padding == 13.0f,
          "cards keep the IMGUI5 13px padding");

    // --- the native palette, so stock ImGui widgets match -------------------
    {
        ImGuiStyle& style = ImGui::GetStyle();
        const ImVec4 frame_bg = style.Colors[ImGuiCol_FrameBg];
        const ImVec4 popup_bg = style.Colors[ImGuiCol_PopupBg];
        CHECK(frame_bg.x > 24.0f / 255.0f && frame_bg.x < 26.0f / 255.0f &&
              frame_bg.y > 27.0f / 255.0f && frame_bg.y < 29.0f / 255.0f &&
              frame_bg.z > 37.0f / 255.0f && frame_bg.z < 39.0f / 255.0f,
              "stock ImGui frames take the IMGUI5 C(25,28,38) field grey");
        CHECK(popup_bg.x > 19.0f / 255.0f && popup_bg.x < 21.0f / 255.0f &&
              popup_bg.z > 28.0f / 255.0f && popup_bg.z < 30.0f / 255.0f,
              "stock ImGui popups take the IMGUI5 C(20,20,29) panel");
        // Sizes are the IMGUI5 numbers times c::scale, like the custom layer.
        CHECK(fabsf(style.WindowRounding - 14.0f * c::scale) < 0.01f &&
              fabsf(style.PopupRounding - 16.0f * c::scale) < 0.01f &&
              fabsf(style.FrameRounding - 5.0f * c::scale) < 0.01f,
              "native windows keep the IMGUI5 14/16/5px rounding at c::scale");
    }

    // --- the login page's own calls: native InputText + Button --------------
    {
        const int style_before = g_ctx->StyleVarStack.Size;
        const int color_before = g_ctx->ColorStack.Size;
        static char key[64] = "TEST-KEY";
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(420.0f, 380.0f), ImGuiCond_Always);
        ImGui::Begin("login", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushItemWidth(300.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(235 / 255.0f, 238 / 255.0f, 245 / 255.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::InputText("##license_key", key, sizeof(key), ImGuiInputTextFlags_CharsNoBlank);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(58 / 255.0f, 132 / 255.0f, 199 / 255.0f, 1.0f));
        ImGui::Button("AUTHENTICATE", ImVec2(300.0f, 40.0f));
        ImGui::PopStyleColor();
        if (custom::Child("login_child", ImVec2(280.0f, 90.0f), true))
            custom::Checkbox("stay signed in", &g_checkbox);
        custom::EndChild();
        ImGui::End();
        ImGui::EndFrame();
        ImGui::Render();
        CHECK(g_ctx->StyleVarStack.Size == style_before &&
              g_ctx->ColorStack.Size == color_before,
              "the license page's own pushes balance (native InputText + Button)");
    }

    // --- every control, one per frame --------------------------------------
    int leaked = 0;
    for (const WidgetCase& c : kCases)
        if (!RunOne(c))
            leaked++;
    CHECK(leaked == 0, "no control leaks a style push (each checked on its own frame)");

    // --- the popup row path, which only runs inside an open popup ----------
    {
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
        ImGui::Begin("popup host", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::OpenPopup("host_popup");
        ImGui::End();
        ImGui::EndFrame();
        ImGui::Render();

        const int style_before = g_ctx->StyleVarStack.Size;
        const int color_before = g_ctx->ColorStack.Size;
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
        ImGui::Begin("popup host", nullptr, ImGuiWindowFlags_NoSavedSettings);
        W_PopupSelectable();
        ImGui::End();
        ImGui::EndFrame();
        ImGui::Render();
        CHECK(g_ctx->StyleVarStack.Size == style_before &&
              g_ctx->ColorStack.Size == color_before,
              "the popup rows balance their pushes too");
    }

    // --- the dropdown popup path (needs a real click) ----------------------
    CHECK(ComboPopupCheck(),
          "the dropdown popup opens, balances its pushes and closes again");

    // --- a full tab pass, three frames -------------------------------------
    {
        const int style_before = g_ctx->StyleVarStack.Size;
        const int color_before = g_ctx->ColorStack.Size;
        for (int frame = 0; frame < 3; ++frame)
        {
            ImGui::NewFrame();
            ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
            ImGui::Begin("tab", nullptr, ImGuiWindowFlags_NoSavedSettings);
            W_Everything();
            ImGui::End();
            ImGui::EndFrame();
            ImGui::Render();
        }
        CHECK(g_ctx->StyleVarStack.Size == style_before &&
              g_ctx->ColorStack.Size == color_before,
              "three frames of a whole tab run with balanced style stacks");
    }

    // --- the hue card still drives the accent ------------------------------
    main_runtime_theme::g_menuHue = 0.0f;
    main_runtime_theme::ApplyAccentFromHue();
    main_runtime_theme::ApplyThemeState();
    CHECK(c::accent.x > 0.99f && c::accent.y < 0.40f && c::accent.z < 0.40f,
          "moving the hue card off the default retints the accent (red end)");

    main_runtime_theme::g_menuHue = 0.6195f;
    main_runtime_theme::ApplyAccentFromHue();
    main_runtime_theme::ApplyThemeState();
    CHECK(main_runtime_theme::GetAccentU32() == IM_COL32(75, 126, 255, 255),
          "the hue card's default lands back on the IMGUI5 blue");

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_Always);
    ImGui::Begin("after", nullptr, ImGuiWindowFlags_NoSavedSettings);
    W_Everything();
    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();
    CHECK(true, "the widgets still run after the accent moved");

    ImGui::DestroyContext();

    if (g_failures == 0)
        printf("OK: all IMGUI5 restyle host checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
