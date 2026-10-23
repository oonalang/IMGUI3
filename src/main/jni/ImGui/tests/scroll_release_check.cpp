#include <cmath>
#include <cstdio>
#include "ImGui/imgui.h"

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static int failures = 0;
static float scrollY = 0.0f;
static float scrollMax = 0.0f;

static void Check(bool ok, const char* label)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", label);
    if (!ok) ++failures;
}

static void Frame(bool down, float target = -1.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = down;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(400, 400));
    ImGui::Begin("scroll release", nullptr, ImGuiWindowFlags_NoSavedSettings);
    ImGui::BeginChild("content", ImVec2(300, 300), false, ImGuiWindowFlags_NoScrollbar);
    scrollY = ImGui::GetScrollY();
    scrollMax = ImGui::GetScrollMaxY();
    if (target >= 0.0f) ImGui::SetScrollY(target);
    ImGui::Dummy(ImVec2(100, 2000));
    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(800, 600);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    for (int i = 0; i < 4; ++i) Frame(false);

    Frame(true, 800);
    Frame(true);
    Check(std::fabs(scrollY - 800) < 1, "fast drag reaches its requested position");
    Frame(true, 1000);
    Frame(false, 1100);
    Check(std::fabs(scrollY - 1000) < 1, "release does not snap back to a stale animation position");
    Frame(false);
    Check(std::fabs(scrollY - 1100) < 1, "momentum continues in the requested direction");
    for (int i = 0; i < 10; ++i) Frame(false);
    Check(std::fabs(scrollY - 1100) < 1, "scroll remains stable after momentum stops");

    Frame(false, 100000);
    Frame(false);
    Check(std::fabs(scrollY - scrollMax) < 1, "fast scrolling clamps to the bottom");
    Frame(false, 0);
    Frame(false);
    Check(scrollY == 0, "scrolling can return to the top without residual animation");
    ImGui::DestroyContext();
    return failures ? 1 : 0;
}
