// Host test for the reskinned widgets in custom_widgets.cpp (IMGUI5 pass):
// the slider row, the select/dropdown, the button, the checkbox and the card.
//
// The reskin moved the slider's drag target from a full-width bar under the
// label to a short track on the right of a single row, and turned the dropdown
// field into a 26 px pill with a tick-marked popup. Both are hit-testing
// changes, so the pixels are not the thing to check: what has to hold is that a
// drag still sets the value, ctrl-click still opens text entry, the label area
// still does nothing, and the popup still selects.
//
// The track is found by probing the row with real clicks rather than by copying
// the widget's geometry maths, so this test fails if the drawing and the hit
// region ever drift apart. Everything runs against the real ImGui core, so the
// IM_ASSERTs in SliderBehavior / Selectable / Child are live.
//
// Build and run with src/main/jni/host_checks.sh.
#include "imgui.h"
#include "imgui_internal.h"
#include "custom_widgets.hpp"
#include "imgui_settings.h"

#include <cmath>
#include <cstdio>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = { 0.678f, 0.561f, 0.914f, 1.0f };

namespace font
{
    ImFont* inter_semibold = nullptr;
    ImFont* icomoon_page = nullptr;
    ImFont* icomoon_logo = nullptr;
}

static int g_failed = 0;

static void Check(bool condition, const char* what)
{
    std::printf("%s  %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition)
        ++g_failed;
}

struct Scene
{
    float   slider = 50.0f;
    bool    check = false;
    bool    inner = false;
    int     combo_item = 0;
    bool    button_pressed = false;
    bool    combo_open = false;

    ImVec2  slider_min, slider_max;
    ImVec2  button_min, button_max;
    ImVec2  check_min, check_max;
    ImVec2  inner_min, inner_max;
    ImVec2  combo_min, combo_max;
    ImVec2  row_min[3], row_max[3];
    ImGuiID slider_id = 0;
    bool    slider_active = false;
    bool    slider_changed = false;
    int     draw_lists = 0;
    int     total_vtx = 0;
};

static Scene S;
static const ImVec2 g_display(640.0f, 480.0f);
static const char* g_items[3] = { "Off", "Latency", "Performance" };

static void Frame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = g_display;
    io.DeltaTime = 1.0f / 60.0f;

    S.button_pressed = false;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(360.0f, 460.0f));
    ImGui::Begin("host", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar);

    S.button_pressed = custom::Button("Apply", ImVec2(140.0f, 28.0f));
    S.button_min = ImGui::GetItemRectMin();
    S.button_max = ImGui::GetItemRectMax();

    S.slider_changed = custom::SliderFloat("Field of View", &S.slider, 0.0f, 100.0f, "%.0f");
    S.slider_min = ImGui::GetItemRectMin();
    S.slider_max = ImGui::GetItemRectMax();
    S.slider_id = ImGui::GetItemID();
    S.slider_active = ImGui::IsItemActive();

    custom::Checkbox("Enabled", &S.check);
    S.check_min = ImGui::GetItemRectMin();
    S.check_max = ImGui::GetItemRectMax();

    const bool combo_open = custom::BeginCombo("Mode", g_items[S.combo_item], 3);
    S.combo_open = combo_open;
    // The field itself is the last item BeginCombo added, in both states.
    S.combo_min = ImGui::GetItemRectMin();
    S.combo_max = ImGui::GetItemRectMax();
    if (combo_open)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (custom::Selectable(g_items[i], i == S.combo_item))
                S.combo_item = i;
            S.row_min[i] = ImGui::GetItemRectMin();
            S.row_max[i] = ImGui::GetItemRectMax();
        }
        custom::EndCombo();
    }

    custom::Child("CARD", ImVec2(300.0f, 110.0f), true);
    custom::Checkbox("Inner", &S.inner);
    S.inner_min = ImGui::GetItemRectMin();
    S.inner_max = ImGui::GetItemRectMax();
    custom::EndChild();

    ImGui::End();
    ImGui::Render();
    S.draw_lists = ImGui::GetDrawData()->CmdListsCount;
    S.total_vtx = ImGui::GetDrawData()->TotalVtxCount;
}

static void Move(float x, float y) { ImGui::GetIO().AddMousePosEvent(x, y); Frame(); }
static void Down() { ImGui::GetIO().AddMouseButtonEvent(0, true); Frame(); }
static void Up() { ImGui::GetIO().AddMouseButtonEvent(0, false); Frame(); }
static void Tap(float x, float y) { Move(x, y); Down(); Up(); }

static float RowY() { return (S.slider_min.y + S.slider_max.y) * 0.5f; }

// Walk the row with real clicks and report the first x (in the requested
// direction) where the slider answers. The button is left held on that x, or
// released everywhere if nothing answered.
static float FindTrackEdge(bool from_right, bool* found)
{
    const float left = S.slider_min.x + 3.0f;
    const float right = S.slider_max.x - 3.0f;
    const int steps = 40;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float)i / (float)steps;
        const float x = from_right ? right - (right - left) * t : left + (right - left) * t;
        Move(x, RowY());
        Down();
        if (S.slider_active)
        {
            *found = true;
            return x;
        }
        Up();
    }
    *found = false;
    return from_right ? right : left;
}

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    io.Fonts->TexID = (ImTextureID)1;

    Frame();
    Frame();

    Check(S.draw_lists > 0 && S.total_vtx > 0, "the reskinned widgets produce draw geometry");

    // ---- slider hit region -------------------------------------------------
    float track_right = 0.0f, track_left = 0.0f;
    {
        bool found = false;
        track_right = FindTrackEdge(true, &found);
        Check(found, "a press on the slider track activates the slider");
        Check(S.slider_changed, "the press reports a changed value");
        Check(S.slider > 85.0f, "the right end of the track means near the maximum");
        Up();

        S.slider = 50.0f;
        Frame();
        track_left = FindTrackEdge(false, &found);
        Check(found, "the left end of the track activates the slider too");
        Check(S.slider < 15.0f, "the left end of the track means near the minimum");
        Up();
    }

    Check(track_right - track_left > 40.0f &&
          track_right - track_left < (S.slider_max.x - S.slider_min.x) - 20.0f,
          "the drag track is a track: narrower than the row, wide enough to aim at");

    {
        const float before = S.slider;
        Tap(S.slider_min.x + 8.0f, RowY());
        Check(std::fabs(S.slider - before) < 0.01f, "tapping the label area leaves the value alone");
    }

    // The value follows the pointer across the whole track, not just its ends.
    {
        const float mid = (track_left + track_right) * 0.5f;
        Tap(mid, RowY());
        Check(S.slider > 30.0f && S.slider < 70.0f, "the middle of the track lands mid-range");
    }

    // ---- slider return value ------------------------------------------------
    // Every feature slider is "if (custom::SliderFloat(...)) { save }", so the
    // press that moves the value has to report the change on that same frame.
    {
        bool found = false;
        FindTrackEdge(false, &found);
        Check(found && S.slider_changed, "the press that moves the value reports a change");
        Up();
    }

    // ---- button -----------------------------------------------------------
    {
        const float cx = (S.button_min.x + S.button_max.x) * 0.5f;
        const float cy = (S.button_min.y + S.button_max.y) * 0.5f;
        Move(cx, cy);
        Down();
        Check(!S.button_pressed, "the button reports pressed on release, not on hold");
        Up();
        Check(S.button_pressed, "the button reports a press on tap");
    }

    // ---- checkbox ---------------------------------------------------------
    {
        const bool before = S.check;
        Tap(S.check_max.x - 16.0f, (S.check_min.y + S.check_max.y) * 0.5f);
        Check(S.check != before, "tapping the switch flips the checkbox");
    }

    // ---- card -------------------------------------------------------------
    {
        const bool before = S.inner;
        Tap((S.inner_min.x + S.inner_max.x) * 0.5f, (S.inner_min.y + S.inner_max.y) * 0.5f);
        Check(S.inner != before, "a widget inside the reskinned card still hit-tests");
    }

    // ---- dropdown ---------------------------------------------------------
    {
        const float field_x = S.combo_min.x + 12.0f;
        const float field_y = (S.combo_min.y + S.combo_max.y) * 0.5f;
        Tap(field_x, field_y);
        Check(S.combo_open, "tapping the select field opens the list");

        if (S.combo_open)
        {
            // The list fades/slides open over a few frames, so read the row
            // rects once it has settled, not on the frame it appeared.
            for (int i = 0; i < 24; ++i)
                Frame();

            const float row_y = (S.row_min[2].y + S.row_max[2].y) * 0.5f;
            const float row_x = (S.row_min[2].x + S.row_max[2].x) * 0.5f;
            Tap(row_x, row_y);
            Check(S.combo_item == 2, "tapping a row selects it");
            // The popup closes on the frame after the row press.
            Frame();
            Frame();
            Check(!S.combo_open, "choosing a row closes the list");
        }
        else
        {
            Check(false, "the list never opened, so the row tap is unverified");
        }
    }

    ImGui::DestroyContext();

    std::printf(g_failed == 0 ? "\nALL PASS\n" : "\nFAILED %d\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
