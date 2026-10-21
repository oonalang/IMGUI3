#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

struct Vector2 { float x, y; };

#include "Hacks/Feature/Init_Hacks.h"
#include "Fonts/fonts.h"
#include "System/Core/SaveConfig.h"
#include "ImGui/imgui.h"

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
float menu[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
bool  isJumpAdjustmentEnabled = false;
bool  SnowB = false;
bool  isSpeedHackEnabled = false;

std::string usedKey = "HOST-TEST";
std::string userType = "PREMIUM";

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static float g_logoOpacity = 1.0f;
static float g_logoSize = 1.0f;

float GetLogoOpacity() { return g_logoOpacity; }
void  SetLogoOpacity(float v) { g_logoOpacity = v; }
float GetLogoSizeMultiplier() { return g_logoSize; }
void  SetLogoSizeMultiplier(float v) { g_logoSize = v; }

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

    raspody::var.tab.current = raspody::Tab_Aimbot;
    raspody::var.tab.stored = raspody::Tab_Settings;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    RenderFrames(8);

    raspody::ui.searchOpen = true;
    std::snprintf(raspody::ui.search, sizeof(raspody::ui.search), "zzzz-none");
    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    RenderFrames(2);

    raspody::ui.searchOpen = false;
    raspody::ui.search[0] = '\0';
    raspody::ClearSearch();
    RenderFrames(2);

    {
        androidFilesDir = "/tmp/raspody_test_configs/";
        mkdir(androidFilesDir.c_str(), 0777);
        mkdir((androidFilesDir + "configs").c_str(), 0777);

        Config.Aim.AimAssistSize = 42.0f;
        Config.ESPMenu.Box = true;
        Config.NewFeaturesMenu.NoHitFlinch = true;
        Config.ExtraMenu.Kinetic = true;
        Config.Crosshair.Size = 7.5f;

        SaveConfiguration("host_test");

        Config.Aim.AimAssistSize = 0.0f;
        Config.ESPMenu.Box = false;
        Config.NewFeaturesMenu.NoHitFlinch = false;
        Config.ExtraMenu.Kinetic = false;
        Config.Crosshair.Size = 0.0f;

        bool ok = LoadConfiguration("host_test");
        if (!ok || Config.Aim.AimAssistSize != 42.0f || !Config.ESPMenu.Box ||
            !Config.NewFeaturesMenu.NoHitFlinch || !Config.ExtraMenu.Kinetic ||
            Config.Crosshair.Size != 7.5f)
        {
            std::printf("FAIL: config round-trip (ok=%d)\n", (int)ok);
            ImGui::DestroyContext();
            return 1;
        }

        {
            std::ofstream f(androidFilesDir + "configs/old.json");
            f << "{\"ExtraMenu\":{\"KineticArmor\":true},\"NewFeatures\":{}}";
            f.close();
            Config.ExtraMenu.Kinetic = false;
            bool ok2 = LoadConfiguration("old");
            if (!ok2 || !Config.ExtraMenu.Kinetic)
            {
                std::printf("FAIL: older-file load (ok=%d)\n", (int)ok2);
                ImGui::DestroyContext();
                return 1;
            }
        }

        {
            std::vector<std::string> profs = ListProfiles();
            bool found = false;
            for (auto& p : profs) if (p == "host_test") found = true;
            if (!found)
            {
                std::printf("FAIL: Configs page does not list on-disk profile\n");
                ImGui::DestroyContext();
                return 1;
            }
        }

        std::printf("OK: config round-trip + older-file load + disk listing\n");
    }

    ImGui::DestroyContext();

    std::printf("OK: Raspody menu rendered %d frames across %d tabs\n",
                g_frames, (int)raspody::Tab_Count);
    return 0;
}

