#include "raspody_font.h"

// Inter SemiBold is already compiled into the app for its existing HUD
// (Fonts/fonts::h -> MainFeatureIncludes.h). It is a plain global array in the
// Main.cpp translation unit; declaring it here avoids pulling in that header a
// second time (which would define the array twice).
extern unsigned char inter_semibold[];
static constexpr int kInterSemiboldBytes = 316220;

namespace raspody
{
    void LoadFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;

        // Cyrillic is included on purpose: the app's existing license strings
        // can contain it, and Inter carries the glyphs anyway.
        const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();

        fonts::small = io.Fonts->AddFontFromMemoryTTF(inter_semibold, kInterSemiboldBytes, 16.0f, &cfg, ranges);
        fonts::body  = io.Fonts->AddFontFromMemoryTTF(inter_semibold, kInterSemiboldBytes, 20.0f, &cfg, ranges);
        fonts::label = io.Fonts->AddFontFromMemoryTTF(inter_semibold, kInterSemiboldBytes, 24.0f, &cfg, ranges);
        fonts::title = io.Fonts->AddFontFromMemoryTTF(inter_semibold, kInterSemiboldBytes, 30.0f, &cfg, ranges);
        fonts::brand = io.Fonts->AddFontFromMemoryTTF(inter_semibold, kInterSemiboldBytes, 40.0f, &cfg, ranges);

        // A failed custom payload must never leave null fonts behind: every
        // Raspody drawing path uses these pointers.
        if (!fonts::small || !fonts::body || !fonts::label || !fonts::title || !fonts::brand)
        {
            ImFont* fallback = io.FontDefault;
            if (!fallback)
                fallback = io.Fonts->AddFontDefault();

            if (!fonts::small) fonts::small = fallback;
            if (!fonts::body)  fonts::body  = fallback;
            if (!fonts::label) fonts::label = fallback;
            if (!fonts::title) fonts::title = fallback;
            if (!fonts::brand) fonts::brand = fallback;
        }

        // Leave io.FontDefault alone: the rest of the app (login window, HUD)
        // was configured around the existing 16 px Inter default.
    }
}
