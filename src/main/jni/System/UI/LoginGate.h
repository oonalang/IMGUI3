#pragma once

//  LoginGate.h  -  the license-key page that came back with the menu.
//
//  The old menu had this page inline in Main.cpp (isLogin / storedKey / the
//  AUTHENTICATE button); 9d10e44 replaced that menu and the page went with it,
//  leaving Login() (Includes/Jni_Stuff.h) with no caller. This is the same gate
//  rebuilt on the ImGui core alone: it draws its own card, hit-tests through the
//  project's own touch coordinates (main's touch block, not ImGui's io), and
//  calls out through injected function pointers so nothing here needs JNI, curl
//  or Android headers - which is what makes System/UI/tests/login_gate_check.cpp
//  able to run it on the host.
//
//  Flow: Locked() is true until authenticate(key) answers "OK".
//    - PASTE KEY      pulls the clipboard into the field (the old page's primary
//                     button; a 24-char key is not typed on an overlay).
//    - AUTHENTICATE   runs authenticate() on a worker thread. Login() attaches
//                     to the JVM and blocks on curl, so running it on the render
//                     thread would freeze the game until the server answered.
//    - on "OK"        the key is saved through save_key() and Locked() flips
//                     false; Main.cpp then draws the menu instead.
//    - on anything else the message is shown in the card and the gate stays up.
//
//  The saved key is loaded once through load_saved() so a returning user only
//  has to press AUTHENTICATE.

#include "imgui.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace login_gate
{

constexpr int KeyCapacity = 256;

enum Phase
{
    Enter = 0,   // waiting for a key / for the user to press AUTHENTICATE
    Checking,    // authenticate() is running on the worker thread
    Accepted     // "OK" came back: the gate is open
};

struct Hooks
{
    // Returns "OK" or the message to show. Called on a worker thread.
    std::string (*authenticate)(const char* key) = nullptr;
    // Saved key, if any. Returns false when there is nothing stored.
    bool (*load_saved)(char* out, int capacity) = nullptr;
    bool (*save_key)(const char* key) = nullptr;
    std::string (*clipboard)() = nullptr;
};

namespace detail
{
inline Hooks           g_hooks{};
inline char            g_key[KeyCapacity] = {0};
inline std::string     g_error;
inline std::atomic<int> g_phase{Enter};
inline bool            g_savedLoaded = false;

// Worker -> render thread handoff. The worker writes both fields, then releases
// g_resultReady; the render thread reads them only after seeing it set.
inline std::mutex      g_resultMutex;
inline std::string     g_result;
inline std::atomic<bool> g_resultReady{false};

// Hit rectangles, recomputed by every Draw(). Zero until the first frame, so a
// touch that arrives before the card has been drawn hits nothing.
inline float g_pasteMinX = 0, g_pasteMinY = 0, g_pasteMaxX = 0, g_pasteMaxY = 0;
inline float g_authMinX = 0,  g_authMinY = 0,  g_authMaxX = 0,  g_authMaxY = 0;

inline int   g_pressed = -1;    // 0 = paste, 1 = authenticate, -1 = none
inline bool  g_down = false;

inline bool Inside(float x, float y, float minX, float minY, float maxX, float maxY)
{
    return x >= minX && x <= maxX && y >= minY && y <= maxY;
}

// Seeds the field from the saved key, once per process. Returns true when a key
// was loaded; a second call does nothing.
inline bool LoadSavedKeyOnce()
{
    if (g_savedLoaded || g_hooks.load_saved == nullptr)
        return false;
    g_savedLoaded = true;

    char saved[KeyCapacity] = {0};
    if (!g_hooks.load_saved(saved, KeyCapacity) || saved[0] == '\0')
        return false;

    std::strncpy(g_key, saved, KeyCapacity - 1);
    g_key[KeyCapacity - 1] = '\0';
    return true;
}
} // namespace detail

inline void Setup(const Hooks& hooks)
{
    detail::g_hooks = hooks;
}

// Testing seam: the two actions the buttons fire, callable directly.
inline void PasteFromClipboard()
{
    if (detail::g_phase.load() == Checking || detail::g_hooks.clipboard == nullptr)
        return;
    std::string clip = detail::g_hooks.clipboard();
    // Trim the whitespace every clipboard can pick up; a key never contains it.
    std::size_t begin = clip.find_first_not_of(" \t\r\n");
    std::size_t end   = clip.find_last_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return;
    clip = clip.substr(begin, end - begin + 1);

    std::strncpy(detail::g_key, clip.c_str(), KeyCapacity - 1);
    detail::g_key[KeyCapacity - 1] = '\0';
    detail::g_error.clear();
}

inline void Submit()
{
    if (detail::g_phase.load() != Enter)
        return;
    if (detail::g_key[0] == '\0')
    {
        detail::g_error = "Paste your license key first";
        return;
    }
    if (detail::g_hooks.authenticate == nullptr)
    {
        detail::g_error = "No authenticator installed";
        return;
    }

    detail::g_error.clear();
    detail::g_resultReady.store(false);
    detail::g_phase.store(Checking);

    const std::string key(detail::g_key);
    std::thread([key]()
    {
        std::string answer;
        try
        {
            answer = detail::g_hooks.authenticate(key.c_str());
        }
        catch (...)
        {
            answer = "Authentication failed";
        }
        {
            std::lock_guard<std::mutex> lock(detail::g_resultMutex);
            detail::g_result = answer;
        }
        detail::g_resultReady.store(true);
    }).detach();
}

// Moves a finished worker answer into the card's state. Returns the phase.
inline int Poll()
{
    if (detail::g_phase.load() == Checking && detail::g_resultReady.load())
    {
        std::string answer;
        {
            std::lock_guard<std::mutex> lock(detail::g_resultMutex);
            answer = detail::g_result;
        }
        detail::g_resultReady.store(false);

        if (answer == "OK")
        {
            if (detail::g_hooks.save_key != nullptr)
                detail::g_hooks.save_key(detail::g_key);
            detail::g_error.clear();
            detail::g_phase.store(Accepted);
        }
        else
        {
            detail::g_error = answer.empty() ? "Authentication failed" : answer;
            detail::g_phase.store(Enter);
        }
    }
    return detail::g_phase.load();
}

inline bool Locked()
{
    return Poll() != Accepted;
}

inline const char* Key()   { return detail::g_key; }
inline const char* Error() { return detail::g_error.c_str(); }
inline int CurrentPhase()  { return Poll(); }

// Test seam: back to an empty Enter gate (the saved-key load happens once).
inline void Reset()
{
    detail::g_key[0] = '\0';
    detail::g_error.clear();
    detail::g_phase.store(Enter);
    detail::g_savedLoaded = true;
    detail::g_resultReady.store(false);
    detail::g_pressed = -1;
    detail::g_down = false;
}

// ---------------------------------------------------------------------------
//  Input: the project's own touch coordinates (screen pixels), the same values
//  Main.cpp hands to nl_ui::OnTouch. Press records the button, release fires it
//  only if the finger is still on that button - the tap behaviour of the menu.
// ---------------------------------------------------------------------------
inline void OnTouch(float x, float y, bool down)
{
    using namespace detail;
    if (g_phase.load() != Enter)
        return;

    if (down)
    {
        g_down = true;
        g_pressed = -1;
        if (Inside(x, y, g_pasteMinX, g_pasteMinY, g_pasteMaxX, g_pasteMaxY)) g_pressed = 0;
        else if (Inside(x, y, g_authMinX, g_authMinY, g_authMaxX, g_authMaxY))  g_pressed = 1;
        return;
    }

    if (!g_down)
        return;
    g_down = false;

    const int pressed = g_pressed;
    g_pressed = -1;
    if (pressed == 0 && Inside(x, y, g_pasteMinX, g_pasteMinY, g_pasteMaxX, g_pasteMaxY))
        PasteFromClipboard();
    else if (pressed == 1 && Inside(x, y, g_authMinX, g_authMinY, g_authMaxX, g_authMaxY))
        Submit();
}

// ---------------------------------------------------------------------------
//  Draw: one card, drawn entirely with the draw list (no ImGui widgets, so the
//  card needs no io input of its own). Call after ImGui::NewFrame() and only
//  while Locked().
// ---------------------------------------------------------------------------
inline void Draw(float dt)
{
    using namespace detail;
    (void)dt;
    if (g_phase.load() == Accepted)
        return;

    detail::LoadSavedKeyOnce();

    ImGuiIO* io = &ImGui::GetIO();
    const float displayW = io->DisplaySize.x;
    const float displayH = io->DisplaySize.y;
    if (displayW <= 1.0f || displayH <= 1.0f)
        return;

    const float cardW = displayW * 0.90f < 420.0f ? displayW * 0.90f : 420.0f;
    const bool  narrow = cardW < 340.0f;    // buttons stack instead of sitting side by side
    const float pad = 22.0f;
    const float fieldH = 46.0f;
    const float buttonH = 44.0f;
    const float buttonsH = narrow ? buttonH * 2.0f + 10.0f : buttonH;
    // Space for the error line is always reserved so the card does not resize
    // the moment a message appears.
    const float cardH = pad + 34.0f + 18.0f + 14.0f + fieldH + 12.0f
                        + buttonsH + 14.0f + 40.0f + 14.0f + pad;
    const ImVec2 centre(displayW * 0.5f, displayH * 0.5f);
    const ImVec2 min(centre.x - cardW * 0.5f, centre.y - cardH * 0.5f);
    const ImVec2 max(min.x + cardW, min.y + cardH);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();

    const ImU32 cardBg    = IM_COL32(16, 18, 21, 242);
    const ImU32 accent    = IM_COL32(38, 166, 128, 255);
    const ImU32 accentDim = IM_COL32(38, 166, 128, 90);
    const ImU32 textCol   = IM_COL32(236, 239, 242, 255);
    const ImU32 dimCol    = IM_COL32(150, 156, 166, 220);
    const ImU32 fieldBg   = IM_COL32(28, 31, 36, 255);

    dl->AddRectFilled(min, max, cardBg, 18.0f);
    dl->AddRect(min, max, accentDim, 18.0f, 0, 1.5f);

    const float innerX = min.x + pad;
    const float innerW = cardW - pad * 2.0f;
    float y = min.y + pad;

    const char* brand = "ETHNIR NOIR";
    const ImVec2 brandSize = font->CalcTextSizeA(22.0f, FLT_MAX, 0.0f, brand);
    dl->AddText(font, 22.0f, ImVec2(min.x + (cardW - brandSize.x) * 0.5f, y), accent, brand);
    y += 34.0f;

    const char* sub = "AUTHENTICATION";
    const ImVec2 subSize = font->CalcTextSizeA(fontSize * 0.72f, FLT_MAX, 0.0f, sub);
    dl->AddText(font, fontSize * 0.72f, ImVec2(min.x + (cardW - subSize.x) * 0.5f, y), dimCol, sub);
    y += 18.0f;

    dl->AddLine(ImVec2(innerX, y), ImVec2(innerX + innerW, y), accentDim, 1.0f);
    y += 14.0f;

    // Key field: shows what is in the buffer, tail-clipped when the key is long.
    dl->AddRectFilled(ImVec2(innerX, y), ImVec2(innerX + innerW, y + fieldH), fieldBg, 8.0f);
    dl->AddRect(ImVec2(innerX, y), ImVec2(innerX + innerW, y + fieldH),
                g_key[0] ? accentDim : IM_COL32(70, 74, 82, 255), 8.0f, 0, 1.0f);
    {
        const char* shown = g_key[0] ? g_key : "no key pasted yet";
        const ImU32 col = g_key[0] ? textCol : IM_COL32(110, 115, 124, 255);
        dl->PushClipRect(ImVec2(innerX + 14.0f, y + 4.0f), ImVec2(innerX + innerW - 14.0f, y + fieldH - 4.0f), true);
        dl->AddText(font, fontSize, ImVec2(innerX + 14.0f, y + (fieldH - fontSize) * 0.5f), col, shown);
        dl->PopClipRect();
    }
    y += fieldH + 12.0f;

    const float halfGap = 10.0f;
    const float pasteW = narrow ? innerW : (innerW - halfGap) * 0.42f;
    const float authW  = narrow ? innerW : innerW - halfGap - pasteW;

    // PASTE KEY
    {
        const ImVec2 bMin(innerX, y), bMax(innerX + pasteW, y + buttonH);
        g_pasteMinX = bMin.x; g_pasteMinY = bMin.y;
        g_pasteMaxX = bMax.x; g_pasteMaxY = bMax.y;

        const bool hot = g_pressed == 0;
        dl->AddRectFilled(bMin, bMax, hot ? IM_COL32(42, 46, 54, 255) : IM_COL32(32, 35, 41, 255), 8.0f);
        dl->AddRect(bMin, bMax, IM_COL32(64, 69, 78, 255), 8.0f, 0, 1.0f);
        const char* label = "PASTE KEY";
        const ImVec2 size = font->CalcTextSizeA(fontSize * 0.82f, FLT_MAX, 0.0f, label);
        dl->AddText(font, fontSize * 0.82f,
                    ImVec2(bMin.x + (pasteW - size.x) * 0.5f, bMin.y + (buttonH - size.y) * 0.5f),
                    textCol, label);
    }

    // AUTHENTICATE (or the checking state)
    {
        const ImVec2 bMin(narrow ? innerX : innerX + pasteW + halfGap, narrow ? y + buttonH + 10.0f : y);
        const ImVec2 bMax(bMin.x + authW, bMin.y + buttonH);
        g_authMinX = bMin.x; g_authMinY = bMin.y;
        g_authMaxX = bMax.x; g_authMaxY = bMax.y;

        const bool checking = g_phase.load() == Checking;
        const bool hot = g_pressed == 1;
        dl->AddRectFilled(bMin, bMax,
                          checking ? IM_COL32(30, 60, 50, 255)
                                   : (hot ? IM_COL32(48, 190, 148, 255) : accent),
                          8.0f);
        const char* label = checking ? "AUTHENTICATING..." : "AUTHENTICATE";
        const ImVec2 size = font->CalcTextSizeA(fontSize * 0.82f, FLT_MAX, 0.0f, label);
        dl->AddText(font, fontSize * 0.82f,
                    ImVec2(bMin.x + (authW - size.x) * 0.5f, bMin.y + (buttonH - size.y) * 0.5f),
                    checking ? dimCol : IM_COL32(8, 14, 12, 255), label);
    }
    y += buttonH + (narrow ? buttonH + 10.0f : 0.0f) + 14.0f;

    // Error / status line.
    if (!g_error.empty())
    {
        dl->AddRectFilled(ImVec2(innerX, y), ImVec2(innerX + innerW, y + 32.0f),
                          IM_COL32(190, 62, 62, 40), 7.0f);
        dl->PushClipRect(ImVec2(innerX + 10.0f, y), ImVec2(innerX + innerW - 10.0f, y + 32.0f), true);
        dl->AddText(font, fontSize * 0.78f, ImVec2(innerX + 10.0f, y + 9.0f),
                    IM_COL32(255, 120, 120, 255), g_error.c_str());
        dl->PopClipRect();
        y += 32.0f + 8.0f;
    }

    const char* footer = "Key is checked on the panel and saved on this device";
    const float footerSize = fontSize * 0.66f;
    const ImVec2 footerDim = font->CalcTextSizeA(footerSize, FLT_MAX, 0.0f, footer);
    dl->AddText(font, footerSize,
                ImVec2(min.x + (cardW - footerDim.x) * 0.5f, max.y - pad - footerDim.y),
                IM_COL32(110, 115, 124, 180), footer);
}

} // namespace login_gate
