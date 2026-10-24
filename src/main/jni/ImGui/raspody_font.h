#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

namespace raspody
{
    namespace fonts
    {
        inline ImFont* small = nullptr;
        inline ImFont* body  = nullptr;
        inline ImFont* label = nullptr;
        inline ImFont* title = nullptr;
        inline ImFont* brand = nullptr;
    }

    void LoadFonts();
}
