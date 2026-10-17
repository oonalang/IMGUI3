#include "imgui.h"
#include "Fonts/nl_fonts.h"
#include "Fonts/fonts.h"

#include <cstdio>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = { 0.678f, 0.561f, 0.914f, 1.0f };

namespace font
{
    ImFont* icomoon_logo = nullptr;
    ImFont* icomoon_page = nullptr;
    ImFont* icon_notify = nullptr;
    ImFont* regular_m = nullptr;
    ImFont* regular_l = nullptr;
    ImFont* inter_medium = nullptr;
    ImFont* inter_semibold = nullptr;
    ImFont* bold_font = nullptr;
    ImFont* esp_font = nullptr;
}

static int g_failed = 0;

static void Check(bool condition, const char* what)
{
    std::printf("%s  %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition)
        g_failed++;
}

static ImFont* LoadWeight(const char* name, const unsigned char* data, unsigned int size)
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF((void*)data, (int)size, 16.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    Check(font != nullptr, name);
    return font;
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // The faces Main.cpp actually installs now: SSTMedium for body, SSTBold for
    // titles (IMGUI5's two UI faces, subset to Latin + Cyrillic).
    ImFont* regular = LoadWeight("SSTMedium loads", (const unsigned char*)nl_sst_medium, nl_sst_medium_size);
    ImFont* medium = LoadWeight("SSTMedium loads (15 px slot)", (const unsigned char*)nl_sst_medium, nl_sst_medium_size);
    ImFont* semibold = LoadWeight("SSTBold loads", (const unsigned char*)nl_sst_bold, nl_sst_bold_size);
    ImFont* bold = LoadWeight("SSTBold loads (25 px slot)", (const unsigned char*)nl_sst_bold, nl_sst_bold_size);

    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Check(pixels != nullptr && width > 0 && height > 0, "font atlas builds");

    ImFontConfig iconCfg;
    iconCfg.FontDataOwnedByAtlas = false;
    ImFont* icons = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page, sizeof(icomoon_page), 18.0f, &iconCfg);
    Check(icons != nullptr, "LANX icon font loads at 18 px");

    ImFont* fonts[4] = { regular, medium, semibold, bold };
    for (int i = 0; i < 4; ++i)
    {
        if (fonts[i] == nullptr)
            continue;
        Check(fonts[i]->FindGlyph('A') != nullptr, "has Latin A");
        Check(fonts[i]->FindGlyph('0') != nullptr, "has digits");
        Check(fonts[i]->FindGlyph(0x0416) != nullptr, "has Cyrillic Zhe");
    }

    ImGui::DestroyContext();
    std::printf(g_failed == 0 ? "\nALL PASS\n" : "\nFAILED %d\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
