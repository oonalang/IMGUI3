#define IMGUI_DEFINE_MATH_OPERATORS
// Host smoke test: lanx core + FreeType + ported widgets behave.
#include "imgui_settings.h"
#include "imgui_freetype.h"
#include "font_defines.h"
#include "imgui_internal.h"
#include "custom_widgets.hpp"
#include <cstdio>
#include <cstring>


int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } else printf("ok:   %s\n", msg); } while (0)

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // FreeType builder must be selected explicitly (imconfig way: compile-time flag).
    unsigned int ft_flags = ImGuiFreeTypeBuilderFlags_NoHinting | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;
    // Load lanx Poppins from the embedded byte arrays with icomoon merged at 0x1..0x10FFFD (lanx ranges).
    ImFontConfig cfg; cfg.FontBuilderFlags = ft_flags;
    font::inter_semibold = io.Fonts->AddFontDefault(&cfg);
    static ImWchar icomoon_ranges[] = { 0xE900, 0xF8FF, 0 };  // BMP-safe on IMGUI_USE_WCHAR32=off (matches the lanx icon PUA range actually used)
    static ImFontConfig icomoon_config;
    icomoon_config.OversampleH = icomoon_config.OversampleV = 1;
    icomoon_config.MergeMode = true;
    icomoon_config.GlyphOffset.y = 2;
    CHECK(io.Fonts->AddFontFromMemoryCompressedBase85TTF(icomoon_compressed_data_base85, 18.f, &icomoon_config, icomoon_ranges) != nullptr, "icomoon icons merged via FreeType");

    bool built = io.Fonts->Build();
    CHECK(built, "atlas Build() with FreeType builder");
    CHECK(!io.Fonts->TexPixelsAlpha8 && io.Fonts->TexPixelsRGBA32 != nullptr, "RGBA32 atlas (LoadColor path)");

    // Some glyphs must exist in the merged icomoon glyph set
    ImFont* f = font::inter_semibold;
    CHECK(f && f->FindGlyphNoFallback(0xEC67) != nullptr, "icomoon U+EC67 glyph present");

    float fbw = 1080, fbh = 2340;
    io.DisplaySize = ImVec2(fbw, fbh);
    io.DeltaTime = 1.0f / 60.f;

    main_runtime_theme::ApplyThemeState();  // sets c::scale etc. for touch

    ImGui::NewFrame();

    // Mimic the shell: one window, sidebar child, content child.
    ImGui::SetNextWindowPos(ImVec2(40, 40));
    ImGui::SetNextWindowSize(ImVec2(680, 520));
    ImGui::Begin("@menu_test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    {
        c::anim::speed = ImGui::GetIO().DeltaTime * 14.f;
        c::second_color = utils::GetDarkColor(c::main_color);

        // --- sidebar (lanx c_tabs shape)
        static int current = 1;
        ImGui::SetCursorPos(ImVec2(20, 85));
        ImGui::BeginChild("Tabs", ImVec2(160, ImGui::GetContentRegionAvail().y));
        {
            for (int i = 1; i <= 6; ++i) {
                char lbl[32]; snprintf(lbl, sizeof(lbl), "Page %d", i);
                custom::Tab(lbl, &current, i);
            }
        }
        ImGui::EndChild();

        int wanted = page_is_changing ? wanted_idx : 0;
        CHECK(wanted == 0, "no phantom tab clicks without touch");

        // --- content: widgets render without crash
        ImGui::SetCursorPos(ImVec2(200, 85));
        custom::Child("Content##t", ImVec2(400, 360), true);
        static bool b1 = false, b2 = true;
        int cmap = 0; int si = 25;
        custom::Checkbox("Aimbot", &b1);
        custom::Checkbox("ESP", &b2);
        static const char* items[] = { "Head", "Chest", "Body" };
        custom::Combo("Target", &cmap, items, 3);
        custom::SliderInt("FOV", &si, 0, 100, "%d");
        float sf = 25.f;
        custom::SliderFloat("Smooth", &sf, 0.f, 100.f, "%.0f");
        custom::SliderInt("Delay", (int*)&si, 0, 100, "%d");
        float colval[4] = {1.f, 0.f, 0.f, 1.f};
        custom::ColorEdit4("Color", colval, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
        custom::Button("Save", ImVec2(0, 0));
        custom::Separator();
        ImGui::Text("glyphs: ^159.2 fps");  // lanx text color codes inside RenderText
        custom::EndChild();
    }
    ImGui::End();

    ImGui::EndFrame();  // end frame 1

    // Frame 2: finger DOWN over the sidebar.
    io.MousePos = ImVec2(90, 150);
    io.MouseDown[0] = true;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(40, 40));
    ImGui::SetNextWindowSize(ImVec2(680, 520));
    ImGui::Begin("@menu_test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        static int current = 1;
        ImGui::SetCursorPos(ImVec2(20, 85));
        ImGui::BeginChild("Tabs", ImVec2(160, ImGui::GetContentRegionAvail().y));
        for (int i = 1; i <= 6; ++i) { char lbl[32]; snprintf(lbl, sizeof(lbl), "Page %d", i); custom::Tab(lbl, &current, i); }
        ImGui::EndChild();
        printf("dbg: frame2 hoverRow=%d\n", (int)ImGui::GetHoveredID() != 0);
    }
    ImGui::End();
    ImGui::EndFrame();  // end frame 2 (finger still down)

    // Frame 3: finger UP -> click+release semantics deliver the press.
    io.MouseDown[0] = false;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(40, 40));
    ImGui::SetNextWindowSize(ImVec2(680, 520));
    ImGui::Begin("@menu_test", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
    {
        static int current = 1;
        ImGui::SetCursorPos(ImVec2(20, 85));
        ImGui::BeginChild("Tabs", ImVec2(160, ImGui::GetContentRegionAvail().y));
        for (int i = 1; i <= 6; ++i) { char lbl[32]; snprintf(lbl, sizeof(lbl), "Page %d", i); custom::Tab(lbl, &current, i); }
        ImGui::EndChild();
        printf("dbg: frame3 page_is_changing=%d wanted_idx=%d\n", page_is_changing, wanted_idx);
    }
    ImGui::End();
    CHECK(wanted_idx > 0 && page_is_changing, "sidebar Tab registers a wanted page on finger release (click semantics)");

    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    ImGui::Render();
    CHECK(ImGui::GetDrawData()->Valid, "draw data valid");

    ImGui::DestroyContext();
    printf(failures ? "\n%d FAILED\n" : "\nALL PASS\n", failures);
    return failures ? 1 : 0;
}
