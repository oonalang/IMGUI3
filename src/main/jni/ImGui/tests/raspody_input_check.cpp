#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
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
#include "ImGui/esp_model_preview.h"
#include "ImGui/esp_preview.h"

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

static int g_failures = 0;

static void Check(bool ok, const char* what)
{
    std::printf("%-58s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok)
        ++g_failures;
}

struct Input
{
    ImVec2 pos = ImVec2(-FLT_MAX, -FLT_MAX);
    bool   down = false;
};

static void Frame(Input& in, int  = 0)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.MousePos = in.pos;
    io.MouseDown[0] = in.down;

    ImGui::NewFrame();
    bool open = true;
    raspody::Render(&open);
    ImGui::Render();
}

static void Tap(Input& in, ImVec2 at)
{
    in.pos = at;
    in.down = false;  Frame(in);
    in.down = true;   Frame(in);
    in.down = false;  Frame(in);
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    raspody::LoadFonts();
    raspody::Style();
    io.Fonts->Build();
    raspody::SetEspPreview(&esp_preview::Draw);

    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    raspody::var.tab.alpha = 1.0f;
    raspody::var.subtab.alpha = 1.0f;

    Input in;
    for (int i = 0; i < 4; ++i)
        Frame(in);

    Check(std::fabs(esp_model_preview::g_yaw - IM_PI) < 0.001f,
          "the model faces the camera straight on");
    const float yawIdleStart = esp_model_preview::g_yaw;
    for (int i = 0; i < 30; ++i)
        Frame(in);
    const float yawIdleEnd = esp_model_preview::g_yaw;
    Check(std::fabs(yawIdleEnd - yawIdleStart) < 0.001f, "the model stays still");

    ImGuiWindow* espWin = ImGui::FindWindowByName("##raspody_esp_preview");
    if (espWin)
        Check(true, "the floating preview card exists");
    else
        Check(false, "the floating preview card exists");

    const MenuWindow menuWin = ComputeMenuWindow(io.DisplaySize);
    Check(espWin && std::fabs(espWin->Size.y - menuWin.size.y) < 1.0f,
          "the preview card is as tall as the menu");

    ImVec2 modelMin(0.0f, 0.0f), modelSize(0.0f, 0.0f);
    if (espWin)
        esp_preview::ModelArea(espWin->Pos, espWin->Size.x, espWin->Size.y,
                               modelMin, modelSize);

    ImVec2 modelCenter(modelMin.x + modelSize.x * 0.5f,
                       modelMin.y + modelSize.y * 0.5f);
    if (!espWin || modelSize.x <= 1.0f || modelSize.y <= 1.0f)
        modelCenter = ImVec2(-FLT_MAX, -FLT_MAX);
    in.pos = modelCenter;
    in.down = false;
    Frame(in);
    in.down = true;
    Frame(in);
    const float yawBeforeDrag = esp_model_preview::g_yaw;
    for (int i = 1; i <= 8; ++i)
    {
        in.pos = ImVec2(modelCenter.x - (float)i * 4.0f, modelCenter.y);
        Frame(in);
    }
    const float yawAfterDrag = esp_model_preview::g_yaw;
    in.down = false;
    Frame(in);

    Check(std::fabs(yawAfterDrag - yawBeforeDrag) < 0.001f, "dragging the model does nothing");

    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::ui.searchOpen = false;
    raspody::ui.search[0] = '\0';
    for (int i = 0; i < 3; ++i)
        Frame(in);

    // Hit points come from the same layout maths the menu uses, so the check does
    // not break when the card is resized.
    const float sc = ImClamp(ImMin(menuWin.size.x / kUiCardW, menuWin.size.y / kUiCardH), 0.70f, 1.45f);
    auto Sc = [&](float v) { return v * sc; };
    const float pad     = ImClamp(menuWin.size.x * 0.017f, Sc(10), Sc(18));
    const float headerH = ImClamp(menuWin.size.y * 0.088f, Sc(40), Sc(56));
    const float gap     = Sc(13);
    const float tabW    = ImClamp(menuWin.size.x * 0.095f, Sc(58), Sc(120));
    const float contentX = menuWin.pos.x + pad + tabW + gap;
    const float contentY = menuWin.pos.y + pad + headerH + gap;
    const float contentW = ImMax(Sc(120), menuWin.size.x - (contentX - menuWin.pos.x) - pad);
    const float centerY  = contentY + Sc(46) * 0.5f;
    const float searchX  = contentX + contentW - Sc(36);
    const ImVec2 searchCenter(searchX + Sc(18), centerY);
    const ImVec2 breadcrumbCenter(contentX + Sc(20), centerY);

    Input click;
    Tap(click, searchCenter);
    const bool searchOpened = raspody::ui.searchOpen;
    Check(searchOpened, "tapping the search button opens search");

    if (searchOpened)
    {
        for (int i = 0; i < 10; ++i)
            Frame(click);
    }

    raspody::ui.searchOpen = false;
    raspody::ui.search[0] = '\0';
    raspody::ClearSearch();
    Frame(click);

    raspody::var.subtab.current = raspody::var.subtab.stored = 1;
    raspody::var.subtab.alpha = 1.0f;
    for (int i = 0; i < 3; ++i)
        Frame(click);

    Input back;
    Tap(back, breadcrumbCenter);

    Check(raspody::var.subtab.stored == 0, "breadcrumb returns to the first subtab");

    raspody::RequestTab(raspody::Tab_Configs);
    for (int i = 0; i < 30; ++i)
        Frame(back);
    Check(raspody::var.tab.stored == raspody::Tab_Configs, "RequestTab switches to Configs");

    Tap(back, breadcrumbCenter);
    for (int i = 0; i < 30; ++i)
        Frame(back);
    Check(raspody::var.tab.stored == raspody::Tab_Visuals,
          "breadcrumb on the first subtab returns to the previous tab");

    std::printf("\n%s (%d failure%s)\n", g_failures ? "FAILED" : "OK", g_failures,
                g_failures == 1 ? "" : "s");
    ImGui::DestroyContext();
    return g_failures ? 1 : 0;
}

