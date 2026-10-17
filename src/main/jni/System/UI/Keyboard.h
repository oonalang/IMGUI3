#pragma once

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/ime_target.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(__ANDROID__)
std::string getClipboard();
#endif

// The keyboard is drawn in its own window because the menu's content child is
// clipped: a panel docked to the bottom of the display cannot live inside it.
// Keys are hit-tested against their rects instead of being ImGui items, so a
// tap never moves the active id off the text field - writing into an ImGui
// item would end the edit session on the very tap meant to type.
namespace lanx_ime
{
    enum KeyAction
    {
        KeyAction_None = 0,
        KeyAction_Shift,
        KeyAction_Backspace,
        KeyAction_LayerLetters,
        KeyAction_LayerSymbols,
        KeyAction_LayerSymbols2,
        KeyAction_Space,
        KeyAction_Done,
        KeyAction_Paste,
    };

    enum Layer
    {
        Layer_Letters = 0,
        Layer_Symbols,
        Layer_Symbols2,
    };

    struct KeyDef
    {
        const char* label;
        char        ch;
        int         action;
        float       units;
    };

    struct State
    {
        bool  shift = false;
        bool  caps = false;
        int   layer = Layer_Letters;
        float shiftTapAt = -1.0f;
        int   pressedKey = -1;
        int   pressedLayer = -1;
        float pressedAt = -1.0f;
        int   heldKey = -1;
        float heldFor = 0.0f;
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    inline const KeyDef* LettersRow(int row, int* count)
    {
        static const KeyDef row0[] = {
            { "q", 'q', 0, 1 }, { "w", 'w', 0, 1 }, { "e", 'e', 0, 1 }, { "r", 'r', 0, 1 }, { "t", 't', 0, 1 },
            { "y", 'y', 0, 1 }, { "u", 'u', 0, 1 }, { "i", 'i', 0, 1 }, { "o", 'o', 0, 1 }, { "p", 'p', 0, 1 },
        };
        static const KeyDef row1[] = {
            { "a", 'a', 0, 1 }, { "s", 's', 0, 1 }, { "d", 'd', 0, 1 }, { "f", 'f', 0, 1 }, { "g", 'g', 0, 1 },
            { "h", 'h', 0, 1 }, { "j", 'j', 0, 1 }, { "k", 'k', 0, 1 }, { "l", 'l', 0, 1 },
        };
        static const KeyDef row2[] = {
            { "shift", 0, KeyAction_Shift, 1.5f },
            { "z", 'z', 0, 1 }, { "x", 'x', 0, 1 }, { "c", 'c', 0, 1 }, { "v", 'v', 0, 1 },
            { "b", 'b', 0, 1 }, { "n", 'n', 0, 1 }, { "m", 'm', 0, 1 },
            { "del", 0, KeyAction_Backspace, 1.5f },
        };
        static const KeyDef row3[] = {
            { "?123", 0, KeyAction_LayerSymbols, 1.6f },
            { ",", ',', 0, 1 },
            { "space", ' ', 0, 3.6f },
            { ".", '.', 0, 1 },
            { "paste", 0, KeyAction_Paste, 1.3f },
            { "done", 0, KeyAction_Done, 1.5f },
        };

        const KeyDef* rows[] = { row0, row1, row2, row3 };
        const int counts[] = { IM_ARRAYSIZE(row0), IM_ARRAYSIZE(row1), IM_ARRAYSIZE(row2), IM_ARRAYSIZE(row3) };
        *count = counts[row];
        return rows[row];
    }

    inline const KeyDef* SymbolsRow(int row, int* count)
    {
        static const KeyDef row0[] = {
            { "1", '1', 0, 1 }, { "2", '2', 0, 1 }, { "3", '3', 0, 1 }, { "4", '4', 0, 1 }, { "5", '5', 0, 1 },
            { "6", '6', 0, 1 }, { "7", '7', 0, 1 }, { "8", '8', 0, 1 }, { "9", '9', 0, 1 }, { "0", '0', 0, 1 },
        };
        static const KeyDef row1[] = {
            { "@", '@', 0, 1 }, { "#", '#', 0, 1 }, { "$", '$', 0, 1 }, { "%", '%', 0, 1 }, { "&", '&', 0, 1 },
            { "-", '-', 0, 1 }, { "+", '+', 0, 1 }, { "(", '(', 0, 1 }, { ")", ')', 0, 1 }, { "/", '/', 0, 1 },
        };
        static const KeyDef row2[] = {
            { "#+=", 0, KeyAction_LayerSymbols2, 1.5f },
            { "*", '*', 0, 1 }, { "\"", '"', 0, 1 }, { "'", '\'', 0, 1 }, { ":", ':', 0, 1 },
            { ";", ';', 0, 1 }, { "!", '!', 0, 1 }, { "?", '?', 0, 1 },
            { "del", 0, KeyAction_Backspace, 1.5f },
        };
        static const KeyDef row3[] = {
            { "abc", 0, KeyAction_LayerLetters, 1.6f },
            { ",", ',', 0, 1 },
            { "space", ' ', 0, 3.6f },
            { ".", '.', 0, 1 },
            { "paste", 0, KeyAction_Paste, 1.3f },
            { "done", 0, KeyAction_Done, 1.5f },
        };

        const KeyDef* rows[] = { row0, row1, row2, row3 };
        const int counts[] = { IM_ARRAYSIZE(row0), IM_ARRAYSIZE(row1), IM_ARRAYSIZE(row2), IM_ARRAYSIZE(row3) };
        *count = counts[row];
        return rows[row];
    }

    inline const KeyDef* Symbols2Row(int row, int* count)
    {
        static const KeyDef row0[] = {
            { "~", '~', 0, 1 }, { "`", '`', 0, 1 }, { "|", '|', 0, 1 }, { "^", '^', 0, 1 }, { "=", '=', 0, 1 },
            { "[", '[', 0, 1 }, { "]", ']', 0, 1 }, { "{", '{', 0, 1 }, { "}", '}', 0, 1 }, { "\\", '\\', 0, 1 },
        };
        static const KeyDef row1[] = {
            { "<", '<', 0, 1 }, { ">", '>', 0, 1 }, { "_", '_', 0, 1 }, { "@", '@', 0, 1 }, { "=", '=', 0, 1 },
            { "|", '|', 0, 1 }, { "~", '~', 0, 1 }, { "[", '[', 0, 1 }, { "]", ']', 0, 1 }, { "\\", '\\', 0, 1 },
        };
        static const KeyDef row2[] = {
            { "?123", 0, KeyAction_LayerSymbols, 1.5f },
            { ":", ':', 0, 1 }, { ";", ';', 0, 1 }, { "!", '!', 0, 1 }, { "?", '?', 0, 1 },
            { "\"", '"', 0, 1 }, { "'", '\'', 0, 1 }, { "*", '*', 0, 1 },
            { "del", 0, KeyAction_Backspace, 1.5f },
        };
        static const KeyDef row3[] = {
            { "abc", 0, KeyAction_LayerLetters, 1.6f },
            { ",", ',', 0, 1 },
            { "space", ' ', 0, 3.6f },
            { ".", '.', 0, 1 },
            { "paste", 0, KeyAction_Paste, 1.3f },
            { "done", 0, KeyAction_Done, 1.5f },
        };

        const KeyDef* rows[] = { row0, row1, row2, row3 };
        const int counts[] = { IM_ARRAYSIZE(row0), IM_ARRAYSIZE(row1), IM_ARRAYSIZE(row2), IM_ARRAYSIZE(row3) };
        *count = counts[row];
        return rows[row];
    }

    inline const KeyDef* LayerRow(int layer, int row, int* count)
    {
        if (layer == Layer_Symbols2)
            return Symbols2Row(row, count);
        if (layer == Layer_Symbols)
            return SymbolsRow(row, count);
        return LettersRow(row, count);
    }

    inline void EmitChar(char c)
    {
        char text[2] = { c, '\0' };
        ImGui::GetIO().AddInputCharactersUTF8(text);
    }

    inline void EmitKey(ImGuiKey key)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(key, true);
        io.AddKeyEvent(key, false);
    }

    inline void Close()
    {
        custom_ime::Forget();
        ImGui::ClearActiveID();
    }

    inline void HandleKey(const KeyDef& key, State& state, float now)
    {
        if (key.ch != 0)
        {
            char c = key.ch;
            if (c >= 'a' && c <= 'z')
                c = (char)((state.shift != state.caps) ? (c - 'a' + 'A') : c);
            EmitChar(c);
            if (state.shift && !state.caps)
                state.shift = false;
            return;
        }

        switch (key.action)
        {
        case KeyAction_Shift:
            if (state.shiftTapAt >= 0.0f && (now - state.shiftTapAt) < 0.30f)
            {
                state.caps = !state.caps;
                state.shift = false;
                state.shiftTapAt = -1.0f;
            }
            else
            {
                state.shift = !state.shift;
                state.shiftTapAt = now;
            }
            break;
        case KeyAction_Backspace:
            EmitKey(ImGuiKey_Backspace);
            break;
        case KeyAction_Space:
            EmitChar(' ');
            break;
        case KeyAction_LayerLetters:
            state.layer = Layer_Letters;
            state.shift = false;
            break;
        case KeyAction_LayerSymbols:
            state.layer = Layer_Symbols;
            break;
        case KeyAction_LayerSymbols2:
            state.layer = Layer_Symbols2;
            break;
        case KeyAction_Paste:
#if defined(__ANDROID__)
            ImGui::GetIO().AddInputCharactersUTF8(getClipboard().c_str());
#endif
            break;
        case KeyAction_Done:
            EmitKey(ImGuiKey_Enter);
            Close();
            break;
        default:
            break;
        }
    }

    inline float PanelHeight(const ImVec2& display)
    {
        const float scale = c::scale;
        const float keyHeight = ImClamp(display.y * 0.045f, 44.0f * scale, 62.0f * scale);
        return 8.0f * scale * 2.0f + keyHeight * 4.0f + 6.0f * scale * 3.0f;
    }

    inline void Render()
    {
        ImGuiIO& io = ImGui::GetIO();
        State& state = Get();

        if (!io.WantTextInput)
        {
            state.heldKey = -1;
            state.heldFor = 0.0f;
            state.pressedKey = -1;
            return;
        }

        const float scale = c::scale;
        const float pad = 8.0f * scale;
        const float gap = 6.0f * scale;
        const float keyHeight = ImClamp(io.DisplaySize.y * 0.045f, 44.0f * scale, 62.0f * scale);
        const float panelHeight = PanelHeight(io.DisplaySize);
        const float panelWidth = io.DisplaySize.x;
        const ImVec2 panelMin(0.0f, io.DisplaySize.y - panelHeight);

        ImGui::SetNextWindowPos(panelMin, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(22.0f / 255.0f, 22.0f / 255.0f, 22.0f / 255.0f, 250.0f / 255.0f));
        ImGui::Begin("##lanx_keyboard", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float now = (float)ImGui::GetTime();
        const float unit = (panelWidth - pad * 2.0f - gap * 9.0f) / 10.0f;

        float y = panelMin.y + pad;
        for (int row = 0; row < 4; ++row)
        {
            int count = 0;
            const KeyDef* keys = LayerRow(state.layer, row, &count);

            float rowWidth = gap * (float)(count - 1);
            for (int i = 0; i < count; ++i)
                rowWidth += keys[i].units * unit;

            float x = (panelWidth - rowWidth) * 0.5f;
            for (int i = 0; i < count; ++i)
            {
                const KeyDef& key = keys[i];
                const float width = key.units * unit;
                const ImVec2 keyMin(x, y);
                const ImVec2 keyMax(x + width, y + keyHeight);
                x += width + gap;

                const bool hovered = ImGui::IsMouseHoveringRect(keyMin, keyMax, false);
                const bool clicked = hovered && ImGui::IsMouseClicked(0);
                const bool mouseDown = hovered && ImGui::IsMouseDown(0);
                const bool isPressed = (state.pressedKey == i && state.pressedLayer == row &&
                                        (now - state.pressedAt) < 0.12f);

                const bool toggled = (key.action == KeyAction_Shift && (state.shift || state.caps)) ||
                                     (key.action == KeyAction_LayerSymbols && state.layer == Layer_Symbols) ||
                                     (key.action == KeyAction_LayerSymbols2 && state.layer == Layer_Symbols2);
                const bool accent = isPressed || toggled || key.action == KeyAction_Done;

                ImVec4 fill = accent ? c::accent
                                     : (hovered ? c::button::background_hovered : c::elements::background);
                if (accent && !isPressed)
                    fill.w *= 0.88f;

                draw->AddRectFilled(keyMin, keyMax, ImGui::GetColorU32(fill), c::elements::rounding * 2.0f);

                char label[16];
                std::snprintf(label, sizeof(label), "%c", key.ch);
                if (key.ch >= 'a' && key.ch <= 'z')
                {
                    const bool upper = (state.shift != state.caps);
                    label[0] = (char)(upper ? (key.ch - 'a' + 'A') : key.ch);
                }
                else if (key.ch == 0)
                {
                    std::snprintf(label, sizeof(label), "%s", key.label);
                }
                if (key.action == KeyAction_Shift)
                    std::snprintf(label, sizeof(label), "%s", state.caps ? "CAPS" : "shift");

                const ImVec2 textSize = ImGui::CalcTextSize(label);
                draw->AddText(ImVec2(keyMin.x + (width - textSize.x) * 0.5f, keyMin.y + (keyHeight - textSize.y) * 0.5f),
                              ImGui::GetColorU32(accent ? c::bg::background : c::text::text_active), label);

                if (clicked)
                {
                    state.pressedKey = i;
                    state.pressedLayer = row;
                    state.pressedAt = now;
                    HandleKey(key, state, now);
                    if (key.action == KeyAction_Backspace)
                    {
                        state.heldKey = i;
                        state.heldFor = 0.0f;
                    }
                    if (state.heldKey != i)
                    {
                        state.heldKey = -1;
                        state.heldFor = 0.0f;
                    }
                    custom_ime::Restore();
                }
                else if (mouseDown && state.heldKey == i && key.action == KeyAction_Backspace)
                {
                    state.heldFor += io.DeltaTime;
                    if (state.heldFor >= 0.40f)
                    {
                        EmitKey(ImGuiKey_Backspace);
                        state.heldFor -= 0.06f;
                    }
                }
            }
            y += keyHeight + gap;
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}
