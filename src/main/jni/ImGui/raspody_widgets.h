#pragma once

// ---------------------------------------------------------------------------
// Raspody widgets - Android port.
//
// Upstream (github.com/eliwoahzja/raspody framework/widgets/*.cpp) resolves
// every control through a config database and draws with DirectX-backed draw
// helpers. This port keeps the exact visual language but takes the bound value
// directly, so the menu writes straight into sConfig / the app's feature state
// and every hack keeps reading the same fields it always did.
//
// No Windows, D3D11, FreeType or file-system dependency lives in this layer.
// ---------------------------------------------------------------------------

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "raspody_style.h"

#include <string>

namespace raspody
{
    // Theme / frame -----------------------------------------------------------------
    void  Style();
    void  SetScale(float scale);
    float Scale();
    void  BeginFrame();

    // Text helpers ------------------------------------------------------------------
    ImVec2 TextSize(ImFont* font, float size, const char* text);
    void   DrawText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
                    ImU32 color, const char* text, ImVec2 align = ImVec2(0.0f, 0.5f));
    void   DrawTextClipped(ImDrawList* dl, ImFont* font, float size,
                           ImVec2 min, ImVec2 max, ImU32 color, const char* text,
                           ImVec2 align = ImVec2(0.0f, 0.5f));
    void   DrawSeparator(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 color);

    // Vector icons (the upstream icon font is replaced with draw primitives) --------
    void DrawTabIcon(ImDrawList* dl, int icon, ImVec2 center, float size, ImU32 color);
    void DrawSearchIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
    void DrawChevron(ImDrawList* dl, ImVec2 center, float size, ImU32 color, float angleRad);
    void DrawMinusIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
    void DrawGearIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color);
    void DrawUserIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color);

    // Header ------------------------------------------------------------------------
    void Brand(ImVec2 pos, float height, const char* name, const char* tagline);
    bool ProfileButton(ImVec2 pos, float height, const char* name, const char* subtitle);
    bool MinimizeButton(ImVec2 pos, float size);
    bool TabButton(const char* label, int icon, bool selected, ImVec2 pos, ImVec2 size);

    // Layout / cards ----------------------------------------------------------------
    void  BeginCard(const char* title, ImVec2 pos, float width);
    float EndCard();                 // returns the card's bottom y
    float CardInnerWidth();
    void  SetColumnWidth(float width);
    void  Spacer(float height);
    void  SectionLabel(const char* text);
    void  ContentShadow(ImVec2 min, ImVec2 max, bool top);

    // Rows --------------------------------------------------------------------------
    bool Checkbox(const char* label, const char* description, bool* value, bool settings = false);
    void CheckboxSettings(const char* label, bool* option1, const char* option1Label,
                          bool* option2 = nullptr, const char* option2Label = nullptr);
    bool SliderFloat(const char* label, const char* description, float* value,
                     float min, float max, const char* format = "%.0f");
    bool SliderInt(const char* label, const char* description, int* value,
                   int min, int max, const char* format = "%d");
    bool Range(const char* label, const char* description, float* valueMin, float* valueMax,
               float min, float max, const char* format = "%.0f");
    bool Dropdown(const char* label, const char* description, int* current,
                  const char* const* items, int itemCount);
    bool ColorRow(const char* label, const char* description, float color[4]);

    // Popups / generic list rows ----------------------------------------------------
    void OpenPopup(const char* name, ImVec2 pos = ImVec2(0, 0));
    void ClosePopup(const char* name);
    bool IsPopupOpen(const char* name);
    bool BeginPopup(const char* name, ImVec2 size, ImVec2 pos,
                    ImVec2 padding = ImVec2(15, 15), bool stayOpen = false);
    void EndPopup();
    bool AnyPopupOpen();
    void TickPopups();
    void DrawBlackout(float alpha);

    bool Selectable(const char* label, bool selected);
    void KeybindChip(const char* key, ImVec2 pos, float width);
    bool ConfigButton(const char* name, const char* created, bool loaded,
                      bool& loadPressed, bool& savePressed);

    // Search ------------------------------------------------------------------------
    bool SearchButton(ImVec2 pos, float size, bool open);
    bool SearchField(char* buffer, int bufferSize, ImVec2 pos, float width);
    bool MatchesSearch(const char* label, const char* description);
    void ClearSearch();

    // ESP preview hook (Main.cpp supplies its live DrawEspPreview) ------------------
    typedef void (*EspPreviewFn)(ImDrawList*, ImVec2, float, float);
    void SetEspPreview(EspPreviewFn fn);
    void DrawEspPreview(ImDrawList* dl, ImVec2 pos, float width, float height);
}
