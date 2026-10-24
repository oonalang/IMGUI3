#pragma once

// ---------------------------------------------------------------------------
// Raspody menu fonts.
//
// The upstream framework embeds an Inter payload plus a 2 MB icon font in
// framework/data/fonts.h and rasterises them with FreeType at DPI-scaled sizes.
// Android has neither FreeType nor a desktop DPI to query, so this port uses
// the Inter payload the app already ships (Fonts/fonts.h) with Dear ImGui's
// built-in stb_truetype rasteriser. Icons are drawn as vector primitives by
// raspody_widgets.cpp, so no icon font is needed at all.
//
// LoadFonts() must be called after the app's own fonts are registered and
// before io.Fonts->Build().
// ---------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

namespace raspody
{
    namespace fonts
    {
        inline ImFont* small = nullptr; // descriptions / meta (16 px atlas)
        inline ImFont* body  = nullptr; // body labels          (20 px atlas)
        inline ImFont* label = nullptr; // section headings     (24 px atlas)
        inline ImFont* title = nullptr; // card titles / values (30 px atlas)
        inline ImFont* brand = nullptr; // brand mark           (40 px atlas)
    }

    // Adds the five Inter sizes used by the menu to the current atlas.
    void LoadFonts();
}
