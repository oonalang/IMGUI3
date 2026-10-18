// ============================================================================
//  nl_ui  -  thin adapter that exposes IMGUI5's Neverlose/HexSync menu through
//  the call sequence documented in REPLACE_EVERYTHING.md:
//      Init / BeginFrame / Draw -> EndFrame / OnTouch / Shutdown
//  Every body is ported from IMGUI5's own app/src/main/jni/main.cpp:
//  font loading at the fitted scale, avatar/logo/glow textures, nl::Fit layout,
//  RenderHexSyncIntro -> RenderNeverloseMenu, nl::TransformDrawData, and the
//  touch -> logical io events. Nothing else of this project calls the menu.
// ============================================================================
#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include <cstddef>

namespace nl_ui
{
using AssetReader = void* (*)(const char* name, std::size_t* out_size);

// Overlay configuration, REPLACE_EVERYTHING.md's SetConfig({...}) shape:
//   fill            share of the screen the menu may occupy
//   user_scale      extra multiplier on top of the auto-fit
//   play_intro      HexSync logo animation at startup
//   draw_background full-screen gradient behind the menu (never in a game)
struct Config
{
    float fill = 0.97f;
    float user_scale = 1.0f;
    bool play_intro = false;
    bool draw_background = false;
};

// Call before Init. SetAssetReader(nl_embedded_asset_reader) is the embedded path.
void SetConfig(const Config& config);
void SetAssetReader(AssetReader reader);

bool Init(int surfaceW, int surfaceH);

// Every frame, between ImGui_ImplOpenGL3_NewFrame() and ImGui::NewFrame().
void BeginFrame(int surfaceW, int surfaceH);
void Draw();

// After ImGui::Render(): moves only the menu's own draw lists into screen
// pixels and leaves everything this project draws for the features alone.
void EndFrame();

namespace Touch
{
enum { Down = 0, Move = 1, Up = 2 };
}
// Screen pixel coordinates, per REPLACE_EVERYTHING.md.
void OnTouch(int action, float x, float y);

void Shutdown();
bool IsReady();
void SetVisible(bool visible);
bool Visible();

// The menu render modifies sConfig through nl_bind on the menu thread; the
// caller of Draw() flushes bindings afterwards if it cares to.
} // namespace nl_ui
