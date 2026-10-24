#include <cstdio>
#include <cstring>
#include <cfloat>
#include <cmath>

#include "ImGui/imgui.h"
#include "ImGui/numeric_slider_entry.h"

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static int g_failures = 0;

static void Check(bool ok, const char* what)
{
    std::printf("%-58s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok)
        ++g_failures;
}

static ImVec2 g_mouse(-FLT_MAX, -FLT_MAX);
static bool   g_down = false;
static float  g_value = 10.0f;
static bool   g_changed = false;
static bool   g_changed_any = false;
static ImGuiID g_id = 0;
static const ImVec2 kPanelPos(100.0f, 100.0f);
static const ImVec2 kPanelSize(220.0f, 27.0f);

static void Frame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.MousePos = g_mouse;
    io.MouseDown[0] = g_down;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(800.0f, 600.0f));
    ImGui::Begin("##host", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::SetCursorScreenPos(kPanelPos);
    ImGui::InvisibleButton("##bar", ImVec2(kPanelSize.x, 24.0f));
    if (g_id == 0)
        g_id = ImGui::GetItemID();

    numeric_slider_entry::OpenOnDoubleTap(g_id, ImGui::IsItemClicked(ImGuiMouseButton_Left),
                                          (double)g_value, false);

    double value = (double)g_value;
    g_changed = numeric_slider_entry::Render(g_id, kPanelPos, kPanelSize, value, 0.0, 100.0, false);
    g_changed_any = g_changed_any || g_changed;
    g_value = (float)value;

    ImGui::End();
    ImGui::Render();
}

static void Step(int frames)
{
    for (int i = 0; i < frames; ++i)
        Frame();
}

static bool SheetRect(ImVec2& min, ImVec2& max)
{
    ImGuiWindow* w = ImGui::FindWindowByName("##numeric_entry_sheet");
    if (w == nullptr)
        return false;
    min = w->Pos;
    max = ImVec2(w->Pos.x + w->Size.x, w->Pos.y + w->Size.y);
    return true;
}

static ImVec2 FieldTap()
{
    ImVec2 min, max;
    if (!SheetRect(min, max))
        return ImVec2(-FLT_MAX, -FLT_MAX);
    return ImVec2(min.x + 40.0f, (min.y + max.y) * 0.5f);
}

static ImVec2 ApplyTap()
{
    ImVec2 min, max;
    if (!SheetRect(min, max))
        return ImVec2(-FLT_MAX, -FLT_MAX);
    return ImVec2(max.x - 26.0f, (min.y + max.y) * 0.5f);
}

static void Tap(ImVec2 at, int hold_frames = 1)
{
    g_mouse = at;
    g_down = false;
    Step(1);
    g_down = true;
    Step(hold_frames);
    g_down = false;
    Step(1);
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    Step(3);
    Check(!numeric_slider_entry::Active(g_id), "the numeric field starts closed");

    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Step(1);
    Check(!numeric_slider_entry::Active(g_id), "a single tap does not open the numeric field");

    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Step(2);
    Check(numeric_slider_entry::Active(g_id), "a double tap opens the numeric field");
    Check(numeric_slider_entry::Get(g_id).keyboard, "the on-screen keyboard is requested on open");
    Check(numeric_slider_entry::Get(g_id).buffer[0] != '\0', "the field is prefilled with the value");

    Step(8);
    Tap(ImVec2(362.0f, 362.0f));
    Step(1);
    Check(std::strcmp(numeric_slider_entry::Get(g_id).buffer, "105") == 0,
          "the on-screen keyboard still types into the field");

    Tap(FieldTap());
    Step(2);
    Check(numeric_slider_entry::Active(g_id), "tapping inside the field keeps it open");
    Check(numeric_slider_entry::Get(g_id).keyboard, "tapping inside the field keeps the keyboard up");

    std::snprintf(numeric_slider_entry::Get(g_id).buffer,
                  sizeof(numeric_slider_entry::Get(g_id).buffer), "42abc");
    Step(2);
    Check(std::strcmp(numeric_slider_entry::Get(g_id).buffer, "42") == 0,
          "letters typed on the keyboard are filtered out");

    std::snprintf(numeric_slider_entry::Get(g_id).buffer,
                  sizeof(numeric_slider_entry::Get(g_id).buffer), "1.2.3");
    Step(2);
    Check(std::strcmp(numeric_slider_entry::Get(g_id).buffer, "1.23") == 0,
          "only one decimal separator survives");

    std::snprintf(numeric_slider_entry::Get(g_id).buffer,
                  sizeof(numeric_slider_entry::Get(g_id).buffer), "42.");
    Step(2);
    Tap(FieldTap());
    Step(1);
    const bool was_active = numeric_slider_entry::Active(g_id);
    Tap(ApplyTap());
    Step(2);
    Check(was_active && !numeric_slider_entry::Active(g_id), "tapping > closes the field");
    Check(std::fabs(g_value - 42.0f) < 0.001f, "tapping > applies the typed value");
    Check(g_changed_any, "applying reports a value change");

    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Step(2);
    Check(numeric_slider_entry::Active(g_id), "the field reopens on a second double tap");

    numeric_slider_entry::Get(g_id).buffer[0] = '\0';
    Step(2);
    const float before = g_value;
    Tap(ApplyTap());
    Step(2);
    Check(!numeric_slider_entry::Active(g_id), "confirming an empty field closes it");
    Check(std::fabs(g_value - before) < 0.001f, "an empty field leaves the value untouched");
    Check(!numeric_slider_entry::Get(g_id).invalid, "an empty field is not flagged as invalid");

    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Tap(ImVec2(kPanelPos.x + 40.0f, kPanelPos.y + 12.0f));
    Step(2);
    std::snprintf(numeric_slider_entry::Get(g_id).buffer,
                  sizeof(numeric_slider_entry::Get(g_id).buffer), "-");
    Step(2);
    Tap(ApplyTap());
    Step(2);
    Check(numeric_slider_entry::Get(g_id).invalid, "an unparsable entry is flagged and stays open");
    Check(numeric_slider_entry::Active(g_id), "an unparsable entry keeps the field open");

    numeric_slider_entry::Get(g_id).buffer[0] = '\0';
    Tap(ImVec2(700.0f, 60.0f));
    Step(2);
    Check(!numeric_slider_entry::Active(g_id), "tapping outside closes the field");

    double parsed = 0.0;
    Check(numeric_slider_entry::ParseValue("250", 0.0, 100.0, false, parsed) && std::fabs(parsed - 100.0) < 0.001,
          "out of range values are clamped");
    Check(!numeric_slider_entry::ParseValue("", 0.0, 100.0, false, parsed),
          "empty text is rejected");
    Check(!numeric_slider_entry::ParseValue("12x", 0.0, 100.0, false, parsed),
          "trailing junk is rejected");
    Check(numeric_slider_entry::ParseValue("7", 0.0, 100.0, true, parsed) && parsed == 7.0,
          "integer entries parse");

    std::printf("\n%s (%d failure%s)\n", g_failures ? "FAILED" : "OK", g_failures,
                g_failures == 1 ? "" : "s");
    ImGui::DestroyContext();
    return g_failures ? 1 : 0;
}
