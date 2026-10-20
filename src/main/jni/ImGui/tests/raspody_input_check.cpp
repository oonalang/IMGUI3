// Host input test for the Raspody menu.
//
// Renders the real menu and the real floating ESP card (ImGui/esp_preview.h)
// against the same ImGui sources Android.mk builds, then drives synthetic touch
// input through ImGui's own mouse state. It covers the things a still render
// cannot show: the character must not turn by itself, dragging it must turn it,
// the search button must open the field and the breadcrumb must walk back.
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
//       src/main/jni/ImGui/tests/raspody_input_check.cpp -o /tmp/raspody_input_check
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

static void Frame(Input& in, int /*unused*/ = 0)
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

// ImGui derives clicks from the MouseDown edge, so a tap is press + release.
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

    // ---- 1. The model must be still when nobody is dragging it -------------
    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::var.subtab.current = raspody::var.subtab.stored = 0;
    raspody::var.tab.alpha = 1.0f;
    raspody::var.subtab.alpha = 1.0f;

    Input in;
    for (int i = 0; i < 4; ++i)
        Frame(in);

    const float yawIdleStart = esp_model_preview::g_yaw;
    for (int i = 0; i < 30; ++i)
        Frame(in);
    const float yawIdleEnd = esp_model_preview::g_yaw;
    Check(std::fabs(yawIdleEnd - yawIdleStart) < 0.01f, "model does not spin on its own");

    // ---- 2. Dragging the model rotates it ---------------------------------
    // Centre of the floating card: beside the menu, roughly 1280 - 100 wide.
    ImVec2 modelCenter(1180.0f, 200.0f);   // inside the docked card's model area
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

    Check(std::fabs(yawAfterDrag - yawBeforeDrag) > 0.05f, "dragging the model rotates it");

    // ---- 3. Search button opens the field ---------------------------------
    raspody::var.tab.current = raspody::var.tab.stored = raspody::Tab_Visuals;
    raspody::ui.searchOpen = false;
    raspody::ui.search[0] = '\0';
    for (int i = 0; i < 3; ++i)
        Frame(in);

    // The search button sits at the right end of the content header.
    Input click;
    Tap(click, ImVec2(1050.0f, 167.0f));
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

    // ---- 4. Breadcrumb walks back one step --------------------------------
    raspody::var.subtab.current = raspody::var.subtab.stored = 1;
    raspody::var.subtab.alpha = 1.0f;
    for (int i = 0; i < 3; ++i)
        Frame(click);

    Input back;
    Tap(back, ImVec2(330.0f, 167.0f));   // the chevron inside the breadcrumb pill

    Check(raspody::var.subtab.stored == 0, "breadcrumb returns to the first subtab");

    // ---- 5. A second tap goes back to the previous tab --------------------
    raspody::RequestTab(raspody::Tab_Configs);
    for (int i = 0; i < 30; ++i)
        Frame(back);
    Check(raspody::var.tab.stored == raspody::Tab_Configs, "RequestTab switches to Configs");

    Tap(back, ImVec2(330.0f, 167.0f));
    for (int i = 0; i < 30; ++i)
        Frame(back);
    Check(raspody::var.tab.stored == raspody::Tab_Visuals,
          "breadcrumb on the first subtab returns to the previous tab");

    std::printf("\n%s (%d failure%s)\n", g_failures ? "FAILED" : "OK", g_failures,
                g_failures == 1 ? "" : "s");
    ImGui::DestroyContext();
    return g_failures ? 1 : 0;
}
