// Host test for custom::InputText and the on-screen keyboard: the tap that
// focuses the field, the keyboard appearing on io.WantTextInput, and every key
// (letters, shift, caps, symbols, del, done) reaching the buffer. The keyboard
// window is a separate window from the field, so this also covers the tap that
// lands outside the field's window and must not end the edit session.
//
// Build and run with src/main/jni/host_checks.sh.
#include "imgui.h"
#include "imgui_internal.h"
#include "custom_widgets.hpp"
#include "imgui_settings.h"
#include "../../System/UI/Keyboard.h"

#include <cstdio>
#include <cstring>

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
static const ImVec2 g_display(1280, 720);

static void Check(bool condition, const char* what)
{
    std::printf("%s  %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition)
        ++g_failed;
}

struct FieldState
{
    bool active = false;
};

static FieldState DrawFrame(char* buffer)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = g_display;
    io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(500, 200));
    ImGui::Begin("host");

    FieldState state;
    custom::InputTextWithHint("##search", "Search skin", buffer, 64, 0, ImVec2(400, 44));
    state.active = ImGui::IsItemActive();

    ImGui::End();
    lanx_ime::Render();
    ImGui::Render();
    return state;
}

static FieldState Tap(char* buffer, float x, float y)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    DrawFrame(buffer);
    io.AddMouseButtonEvent(0, true);
    DrawFrame(buffer);
    io.AddMouseButtonEvent(0, false);
    DrawFrame(buffer);
    return DrawFrame(buffer);
}

static void KeyPosition(int row, int index, float* outX, float* outY)
{
    const float scale = c::scale;
    const float pad = 8.0f * scale;
    const float gap = 6.0f * scale;
    const float keyHeight = ImClamp(g_display.y * 0.045f, 44.0f * scale, 62.0f * scale);
    const float panelHeight = pad * 2.0f + keyHeight * 4.0f + gap * 3.0f;
    const float unit = (g_display.x - pad * 2.0f - gap * 9.0f) / 10.0f;

    int count = 0;
    const lanx_ime::KeyDef* keys = lanx_ime::LayerRow(lanx_ime::Get().layer, row, &count);
    float rowWidth = gap * (float)(count - 1);
    for (int i = 0; i < count; ++i)
        rowWidth += keys[i].units * unit;

    float x = (g_display.x - rowWidth) * 0.5f;
    for (int i = 0; i < index; ++i)
        x += keys[i].units * unit + gap;

    *outX = x + keys[index].units * unit * 0.5f;
    *outY = g_display.y - panelHeight + pad + (float)row * (keyHeight + gap) + keyHeight * 0.5f;
}

static FieldState TapKey(char* buffer, int row, int index)
{
    float x = 0.0f, y = 0.0f;
    KeyPosition(row, index, &x, &y);
    return Tap(buffer, x, y);
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

    char buffer[64] = "";

    DrawFrame(buffer);
    DrawFrame(buffer);
    FieldState state = Tap(buffer, 60, 40);

    Check(state.active, "tapping the textbox activates it");
    Check(io.WantTextInput, "an active textbox asks for the on-screen keyboard");
    Check(buffer[0] == '\0', "the field starts empty");

    TapKey(buffer, 0, 0);
    Check(std::strcmp(buffer, "q") == 0, "tapping 'q' types q into the focused textbox");

    TapKey(buffer, 0, 1);
    Check(std::strcmp(buffer, "qw") == 0, "keys keep reaching the textbox after a tap");

    TapKey(buffer, 2, 0);
    TapKey(buffer, 0, 0);
    Check(std::strcmp(buffer, "qwQ") == 0, "shift uppercases the next letter only");

    TapKey(buffer, 0, 4);
    Check(std::strcmp(buffer, "qwQt") == 0, "shift clears itself after one character");

    int count = 0;
    lanx_ime::LayerRow(lanx_ime::Get().layer, 2, &count);
    TapKey(buffer, 2, count - 1);
    Check(std::strcmp(buffer, "qwQ") == 0, "del removes the last character");

    TapKey(buffer, 3, 0);
    Check(lanx_ime::Get().layer == lanx_ime::Layer_Symbols, "the ?123 key switches to the symbol layer");

    TapKey(buffer, 0, 3);
    Check(std::strcmp(buffer, "qwQ4") == 0, "the symbol layer types digits");

    lanx_ime::LayerRow(lanx_ime::Get().layer, 2, &count);
    TapKey(buffer, 2, count - 1);
    Check(std::strcmp(buffer, "qwQ") == 0, "del works on the symbol layer too");

    TapKey(buffer, 3, 0);
    Check(lanx_ime::Get().layer == lanx_ime::Layer_Letters, "the abc key returns to the letter layer");

    int actionCount = 0;
    lanx_ime::LayerRow(lanx_ime::Get().layer, 3, &actionCount);
    state = TapKey(buffer, 3, actionCount - 1);
    Check(!state.active && !io.WantTextInput, "done ends the edit session and hides the keyboard");

    std::printf("\n%s (%d failed)\n", g_failed == 0 ? "ALL PASS" : "FAILURES", g_failed);
    ImGui::DestroyContext();
    return g_failed == 0 ? 0 : 1;
}
