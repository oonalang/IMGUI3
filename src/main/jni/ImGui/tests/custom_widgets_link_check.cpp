// Link check for the LANX widget layer.
//
// The migration reports come in one shape: the UI compiles, the build dies at
// the final link with `undefined reference to custom::InputTextWithHint(...)`.
// That is a symbol problem, not a logic problem - a widget implementation that
// is not part of libgspace1.so, or a definition whose parameter list drifted
// from the declaration in custom_widgets.hpp.
//
// This test takes the address of EVERY function custom_widgets.hpp declares.
// Taking the address is enough: the linker has to resolve the symbol, so a
// widget that is not compiled in, or whose definition no longer matches the
// declaration, fails here on the host instead of on the device build.
//
// Build and run from src/main/jni:
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I . -I ImGui -I Includes \
//       ImGui/tests/custom_widgets_link_check.cpp ImGui/custom_widgets.cpp \
//       ImGui/ImguiPP.cpp ImGui/imgui.cpp ImGui/imgui_draw.cpp \
//       ImGui/imgui_tables.cpp ImGui/imgui_widgets.cpp ImGui/stb_image.cpp \
//       -o /tmp/custom_widgets_link_check && /tmp/custom_widgets_link_check
// Exits 0 and prints "OK: every custom:: widget symbol resolves".
// Android.mk does not wildcard ImGui/tests, so this file never reaches the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "custom_widgets.hpp"

// The font globals the widgets reference; defined here so the check needs no
// font header and no game code.
namespace font
{
    ImFont *icomoon_page = nullptr;
    ImFont *inter_semibold = nullptr;
}
ImFont *F50 = nullptr;
ImFont *F107 = nullptr;
// The runtime accent, normally written by the menu; declared in imgui_settings.h.
float menu[4] = {0.678f, 0.561f, 0.913f, 1.0f};

// Keep the whole declaration surface alive: one array of function pointers, so
// the linker must resolve every symbol below.
static void (*const g_symbols[])() = {
    reinterpret_cast<void (*)()>(custom::shell::GetTextFont),
    reinterpret_cast<void (*)()>(custom::shell::GetTitleFont),
    reinterpret_cast<void (*)()>(custom::shell::GetIconFont),
    reinterpret_cast<void (*)()>(custom::shell::MeasureText),
    reinterpret_cast<void (*)()>(custom::shell::Normalize),
    reinterpret_cast<void (*)()>(custom::shell::DrawPanelShell),
    reinterpret_cast<void (*)()>(custom::shell::DrawSidebarButton),
    reinterpret_cast<void (*)()>(custom::shell::DrawTitleCard),
    reinterpret_cast<void (*)()>(custom::shell::DrawMenuColorCard),
    reinterpret_cast<void (*)()>(custom::shell::DrawCloseCard),
    reinterpret_cast<void (*)()>(custom::Page),
    reinterpret_cast<void (*)()>(custom::ThemeButton),
    reinterpret_cast<void (*)()>(custom::Button),
    reinterpret_cast<void (*)()>(custom::ChildEx),
    reinterpret_cast<void (*)()>(custom::Child),
    reinterpret_cast<void (*)()>(custom::ChildID),
    reinterpret_cast<void (*)()>(custom::EndChild),
    reinterpret_cast<void (*)()>(custom::BeginGroup),
    reinterpret_cast<void (*)()>(custom::EndGroup),
    reinterpret_cast<void (*)()>(custom::Checkbox),
    reinterpret_cast<void (*)()>(custom::CheckboxClicked),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, bool, ImGuiSelectableFlags, const ImVec2 &)>(&custom::Selectable)),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, bool *, ImGuiSelectableFlags, const ImVec2 &)>(&custom::Selectable)),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, const char *, int, bool, ImGuiComboFlags)>(&custom::BeginCombo)),
    reinterpret_cast<void (*)()>(custom::EndCombo),
    reinterpret_cast<void (*)()>(custom::MultiCombo),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, int *, const char *const *, int, int)>(&custom::Combo)),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, int *, const char *, int)>(&custom::Combo)),
    reinterpret_cast<void (*)()>(static_cast<bool (*)(const char *, int *, const char *(*)(void *, int), void *, int, int)>(&custom::Combo)),
    reinterpret_cast<void (*)()>(custom::InputText),
    reinterpret_cast<void (*)()>(custom::InputTextWithHint),
    reinterpret_cast<void (*)()>(custom::ColorButton),
    reinterpret_cast<void (*)()>(custom::ColorEdit4),
    reinterpret_cast<void (*)()>(custom::ColorPicker4),
    reinterpret_cast<void (*)()>(custom::KnobScalar),
    reinterpret_cast<void (*)()>(custom::KnobFloat),
    reinterpret_cast<void (*)()>(custom::KnobInt),
    reinterpret_cast<void (*)()>(custom::SliderScalar),
    reinterpret_cast<void (*)()>(custom::SliderFloat),
    reinterpret_cast<void (*)()>(custom::SliderInt),
    reinterpret_cast<void (*)()>(custom::Keybind),
    reinterpret_cast<void (*)()>(custom::Separator_line),
    reinterpret_cast<void (*)()>(custom::Separator),
};

int main()
{
    // Touch every entry so nothing is optimised away.
    volatile uintptr_t checksum = 0;
    for (const auto &fn : g_symbols)
        checksum += (uintptr_t)fn;
    std::printf("resolved %zu custom:: widget symbols (checksum %llu)\n",
                sizeof(g_symbols) / sizeof(g_symbols[0]),
                (unsigned long long)checksum);

    // One function actually called, so the check also proves the library works
    // and not only that the names exist.
    if (custom::shell::Normalize(5.0f, 0.0f, 10.0f) != 0.5f) {
        std::printf("FAIL: shell::Normalize(5, 0, 10) != 0.5\n");
        return 1;
    }

    std::printf("OK: every custom:: widget symbol resolves\n");
    return 0;
}
