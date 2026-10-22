// Mirrors the lines Main.cpp now uses for the menu pre-pass, against the real
// menu_layout.h, so a typo or a changed ImGui signature fails here instead of
// in the NDK build - and checks the geometry the login panel shares with it.
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/menu_layout.h"

// Same font globals the repo's imgui_widgets.cpp draws through.
ImFont* F50 = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

int main()
{
    const ImVec2 ioDisplaySize(1080.0f, 2400.0f);
    const MenuWindow menuWin = ComputeMenuWindow(ioDisplaySize);

    const bool fits = menuWin.size.x <= 900.5f && menuWin.size.y <= 592.5f &&
                      menuWin.size.x <= ioDisplaySize.x && menuWin.size.y <= ioDisplaySize.y;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ioDisplaySize;
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(menuWin.pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(menuWin.size, ImGuiCond_Always);
    ImGui::Begin("##fragment", nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::End();
    ImGui::EndFrame();
    ImGui::DestroyContext();

    ::printf("%.0fx%.0f at %.0f,%.0f\n", menuWin.size.x, menuWin.size.y, menuWin.pos.x, menuWin.pos.y);
    return fits ? 0 : 1;
}
