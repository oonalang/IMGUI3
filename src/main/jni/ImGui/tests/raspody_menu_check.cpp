// Host test for the Raspody menu port.
//
// It compiles the real raspody_menu.h / raspody_widgets.cpp against the same
// ImGui sources Android.mk builds, then renders every tab and subtab for a few
// frames with a null backend. That catches the two failure modes a device build
// hides until launch: API/type errors in the new menu code and ImGui stack
// misuse (mismatched Begin/End, PushStyleVar counts, popup lifetimes).
//
// Android.mk wildcards only ImGui/*.c*, never ImGui/tests/, so this file does
// not reach the APK.
//
// Build:
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS \
//       -I src/main/jni -I src/main/jni/ImGui \
//       src/main/jni/ImGui/imgui.cpp src/main/jni/ImGui/imgui_draw.cpp \
//       src/main/jni/ImGui/imgui_tables.cpp src/main/jni/ImGui/imgui_widgets.cpp \
//       src/main/jni/ImGui/raspody_widgets.cpp src/main/jni/ImGui/raspody_font.cpp \
//       src/main/jni/ImGui/tests/raspody_menu_check.cpp -o /tmp/raspody_menu_check
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Init_Hacks only needs Vector2 for the Touch struct when it is included on its
// own; the real build always has it from the SDK first.
struct Vector2 { float x, y; };

#include "Hacks/Feature/Init_Hacks.h"
#include "Fonts/fonts.h"

#include "ImGui/imgui.h"

// MainFeatureIncludes.h normally brings Logo.h in before the menu header.
float GetLogoOpacity();
void  SetLogoOpacity(float v);
float GetLogoSizeMultiplier();
void  SetLogoSizeMultiplier(float v);

#include "ImGui/raspody_menu.h"

bool  HIDEESP = false;
bool  UnlimitedFly = false;
float SnowBsize = 0.0f;
float speedHackMultiplier = 1.0f;
float jumpHeightMultiplier = 1.0f;

std::string usedKey = "HOST-TEST";
std::string userType = "PREMIUM";

// The app's customised imgui_widgets.cpp references these Main.cpp globals.
namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static float g_logoOpacity = 1.0f;
static float g_logoSize = 1.0f;

float GetLogoOpacity() { return g_logoOpacity; }
void  SetLogoOpacity(float v) { g_logoOpacity = v; }
float GetLogoSizeMultiplier() { return g_logoSize; }
void  SetLogoSizeMultiplier(float v) { g_logoSize = v; }

void SaveConfiguration(const std::string&) {}
bool LoadConfiguration(const std::string&) { return true; }

static int g_frames = 0;
static int g_lastVerts = 0;
static int g_lastCmds = 0;
static int g_lastLists = 0;

static void RenderFrames(int count)
{
    for (int i = 0; i < count; ++i)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime = 1.0f / 60.0f;

        ImGui::NewFrame();
        bool open = true;
        raspody::Render(&open);
        ImGui::Render();

        ImDrawData* dd = ImGui::GetDrawData();
        g_lastVerts = 0;
        g_lastCmds = 0;
        g_lastLists = dd ? dd->CmdListsCount : 0;
        if (dd)
        {
            for (int i = 0; i < dd->CmdListsCount; ++i)
            {
                g_lastVerts += dd->CmdLists[i]->VtxBuffer.Size;
                g_lastCmds += dd->CmdLists[i]->CmdBuffer.Size;
            }
        }

        ++g_frames;
    }
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;

    raspody::LoadFonts();
    raspody::Style();
    io.Fonts->Build();

    // First frames also exercise the fade-in path (alpha starts at 1 for the
    // initial tab, but every page below starts from its own state).
    RenderFrames(4);

    for (int tab = 0; tab < raspody::Tab_Count; ++tab)
    {
        for (int sub = 0; sub < 2; ++sub)
        {
            raspody::var.tab.current = tab;
            raspody::var.tab.stored = tab;
            raspody::var.tab.alpha = 1.0f;
            raspody::var.subtab.current = sub;
            raspody::var.subtab.stored = sub;
            raspody::var.subtab.alpha = 1.0f;
            RenderFrames(2);
        }
    }

    std::printf("draw data: %d lists, %d commands, %d vertices\n",
                g_lastLists, g_lastCmds, g_lastVerts);
    if (g_lastLists < 2 || g_lastCmds < 4 || g_lastVerts < 500)
    {
        std::printf("FAIL: the menu did not produce a real draw list\n");
        ImGui::DestroyContext();
        return 1;
    }

    // Popup paths: the dropdown, the row-settings menu, a colour picker and
    // the profile menu all begin top-level windows from inside the menu.
    struct PopupCase { int tab; int sub; const char* name; };
    const PopupCase popupCases[] = {
        { raspody::Tab_Aimbot, 0, "Aim trigger##Dropdown" },
        { raspody::Tab_Aimbot, 0, "Row settings##Aim Assist 360" },
        { raspody::Tab_Visuals, 0, "Player ESP colour##Color" },
        { raspody::Tab_Visuals, 0, "##raspody_profile_menu" },
    };

    for (const auto& popup : popupCases)
    {
        raspody::var.tab.current = raspody::var.tab.stored = popup.tab;
        raspody::var.tab.alpha = 1.0f;
        raspody::var.subtab.current = raspody::var.subtab.stored = popup.sub;
        raspody::var.subtab.alpha = 1.0f;
        raspody::OpenPopup(popup.name, ImVec2(420.0f, 260.0f));
        RenderFrames(3);
        raspody::ClosePopup(popup.name);
        RenderFrames(2);
    }

    // Tab fade path: swap the stored tab while the current page is visible.
    raspody::var.tab.current = raspody::Tab_Aimbot;
    raspody::var.tab.stored = raspody::Tab_Settings;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    RenderFrames(8);

    // Search filtering path with a query that hides most rows.
    raspody::ui.searchOpen = true;
    std::snprintf(raspody::ui.search, sizeof(raspody::ui.search), "zzzz-none");
    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    RenderFrames(2);

    raspody::ui.searchOpen = false;
    raspody::ui.search[0] = '\0';
    raspody::ClearSearch();
    RenderFrames(2);

    ImGui::DestroyContext();

    std::printf("OK: Raspody menu rendered %d frames across %d tabs\n",
                g_frames, (int)raspody::Tab_Count);
    return 0;
}
