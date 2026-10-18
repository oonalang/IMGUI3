// nl_ui.cpp - the adapter bodies. Ported from oonalang/IMGUI5
// app/src/main/jni/main.cpp; the AAssetManager reads became the asset-reader
// indirection so the compiled-in buffers of nl_embedded_assets.cpp work too.
#include "nl_ui.hpp"

#include "imgui_internal.h"
#include "nl_layout.hpp"
#include "hashes.hpp"          // ICON_MIN_FA / ICON_MAX_FA
#include "hexsync_intro.hpp"
#include "neverlose_menu.hpp"
#include "backends/imgui_impl_opengl3.h"
#include "nl_embedded_assets.hpp"

#include <GLES3/gl3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nl_ui
{
static Config g_config;
static AssetReader g_reader = nl_embedded_asset_reader;

static nl::Layout g_layout;
static float g_fontScale = 0.0f;
static bool g_fontsLive = false;
static bool g_initialized = false;
static bool g_visible = true;

static ImTextureID g_Avatar = nullptr;
static ImTextureID g_Logo = nullptr;
static ImTextureID g_Glow = nullptr;
static ImFont* g_Wordmark = nullptr;
static double g_RevealStart = -1.0;

static std::vector<ImDrawList*> g_menuLists;

// Physical-pixel rect of the reopen chip, shared by Draw (where it is laid
// out) and OnTouch (where it is hit tested).

void SetConfig(const Config& config) { g_config = config; }
void SetAssetReader(AssetReader reader) { if (reader) g_reader = reader; }
bool IsReady() { return g_initialized; }
void SetVisible(bool visible) { g_visible = visible; }
bool Visible() { return g_visible; }

// ---- assets -----------------------------------------------------------------
static void* ReadAsset(const char* name, size_t* out_size)
{
    if (!g_reader) return nullptr;
    return g_reader(name, out_size);
}

// Ported from IMGUI5 main.cpp MakeTexture().
static ImTextureID MakeTexture(const unsigned char* rgba, int w, int h)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    return (ImTextureID)(intptr_t)tex;
}

// avatar.rgba / logo.rgba: uint32 width, uint32 height (little endian), then RGBA8 pixels.
static ImTextureID LoadRgbaAsset(const char* name)
{
    size_t size = 0;
    unsigned char* data = (unsigned char*)ReadAsset(name, &size);
    if (!data) return nullptr;
    ImTextureID tex = nullptr;
    if (size > 8)
    {
        uint32_t w, h;
        std::memcpy(&w, data, 4);
        std::memcpy(&h, data + 4, 4);
        if ((size_t)w * h * 4 + 8 == size)
            tex = MakeTexture(data + 8, (int)w, (int)h);
    }
    return tex;
}

static void LoadTextures()
{
    g_Avatar = LoadRgbaAsset("avatar.rgba");
    g_Logo = LoadRgbaAsset("logo.rgba");

    static std::uint32_t glow[128 * 128];
    BuildHexSyncGlowPixels(glow, 128);
    g_Glow = MakeTexture((const unsigned char*)glow, 128, 128);
}

static void FreeTextures()
{
    ImTextureID* all[] = { &g_Avatar, &g_Logo, &g_Glow };
    for (ImTextureID* t : all)
    {
        if (*t)
        {
            GLuint id = (GLuint)(intptr_t)*t;
            glDeleteTextures(1, &id);
            *t = nullptr;
        }
    }
}

// ---- fonts ------------------------------------------------------------------
// Glyphs are rasterized at the physical size and the logical layout stays at
// 100%, so text is sharp after the draw data is scaled to the screen.
static void AddFontsAtScale(float scale)
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.FontGlobalScale = 1.0f / scale;
    g_Wordmark = nullptr;

    ImFontConfig text;
    text.OversampleH = 3;
    text.PixelSnapH = true;

    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons;
    icons.MergeMode = true;
    icons.PixelSnapH = true;

    size_t size = 0;
    void* data = ReadAsset("SSTMedium.TTF", &size);                       // font 0
    if (data)
    {
        text.FontDataOwnedByAtlas = false;     // the reader hands out static memory
        io.Fonts->AddFontFromMemoryTTF(data, (int)size, 15.0f * scale, &text);
    }
    else io.Fonts->AddFontDefault();

    data = ReadAsset("fa-solid-900.ttf", &size);                          // merged icons
    if (data)
    {
        icons.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(data, (int)size, 14.0f * scale, &icons, icon_ranges);
    }

    data = ReadAsset("SSTBold.TTF", &size);                               // font 1
    if (data)
    {
        text.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(data, (int)size, 16.0f * scale, &text);
    }

    ImFontConfig word;
    word.OversampleH = 3;
    word.PixelSnapH = true;
    word.FontDataOwnedByAtlas = false;
    data = ReadAsset("SpaceGrotesk-SemiBold.ttf", &size);                 // font 2
    if (data) g_Wordmark = io.Fonts->AddFontFromMemoryTTF(data, (int)size, 40.0f * scale, &word);

    g_fontScale = scale;
}

static void SyncFonts(float scale)
{
    if (g_fontScale > 0.0f && std::fabs(scale - g_fontScale) < 0.02f * g_fontScale)
        return;
    if (g_fontsLive)
        ImGui_ImplOpenGL3_DestroyFontsTexture();
    AddFontsAtScale(scale);
    if (g_fontsLive)
        ImGui_ImplOpenGL3_CreateFontsTexture();   // first upload happens inside NewFrame
}

// ---- lifetime ---------------------------------------------------------------
static ImVec2 g_chipMin(-1, -1), g_chipMax(-1, -1);

bool Init(int surfaceW, int surfaceH)
{
    if (g_initialized || surfaceW <= 0 || surfaceH <= 0) return g_initialized;
    g_layout = nl::Fit(surfaceW, surfaceH, g_config.fill, g_config.user_scale);
    SyncFonts(g_layout.scale);
    LoadTextures();
    g_RevealStart = -1.0;
    g_initialized = true;
    return true;
}

void Shutdown()
{
    if (!g_initialized) return;
    if (g_fontsLive) ImGui_ImplOpenGL3_DestroyFontsTexture();
    g_fontsLive = false;
    FreeTextures();
    g_fontScale = 0.0f;
    g_Wordmark = nullptr;
    g_menuLists.clear();
    g_initialized = false;
}

// ---- frame ------------------------------------------------------------------
void BeginFrame(int surfaceW, int surfaceH)
{
    if (!g_initialized)
    {
        if (!Init(surfaceW, surfaceH)) return;
    }
    else if (surfaceW > 0 && surfaceH > 0)
    {
        g_layout = nl::Fit(surfaceW, surfaceH, g_config.fill, g_config.user_scale);
        SyncFonts(g_layout.scale);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(nl::kCanvasW, nl::kCanvasH);   // logical canvas, not the screen
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void Draw()
{
    if (!g_initialized) return;
    if (!g_visible)
    {
        // Reopen chip (the only control the menu itself adds): a small
        // rounded tab pinned to the canvas' top-right, so a hidden menu can
        // always be brought back. Drawn/ hit in physical pixels; not a menu.
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const ImVec2 size(30.0f * g_layout.scale, 30.0f * g_layout.scale);
        g_chipMin = ImVec2(g_layout.origin.x + g_layout.scale * (nl::kCanvasW - 14.0f) - size.x,
                           g_layout.origin.y + g_layout.scale * 14.0f);
        g_chipMax = g_chipMin + size;
        fg->AddRectFilled(g_chipMin, g_chipMax, IM_COL32(24, 27, 39, 215), size.y * 0.28f);
        fg->AddRect(g_chipMin, g_chipMax, IM_COL32(75, 126, 255, 200), size.y * 0.28f);
        const float cy = g_chipMin.y + size.y * 0.5f;
        for (int i = -1; i <= 1; ++i)
            fg->AddLine(ImVec2(g_chipMin.x + size.x * 0.28f, cy + i * size.y * 0.22f),
                        ImVec2(g_chipMin.x + size.x * 0.72f, cy + i * size.y * 0.22f),
                        IM_COL32(230, 232, 240, 235), g_layout.scale);
        return;
    }
    g_chipMin = g_chipMax = ImVec2(-1, -1);   // the chip is not on screen
    g_menuLists.clear();

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);

    if (g_config.play_intro)
    {
        const bool intro_active = RenderHexSyncIntro(g_Logo, g_Glow, g_Wordmark);
        if (intro_active) g_RevealStart = -1.0;
        else
        {
            if (g_RevealStart < 0.0) g_RevealStart = ImGui::GetTime();
            const float reveal = ImClamp((float)((ImGui::GetTime() - g_RevealStart) / 0.48), 0.0f, 1.0f);
            RenderNeverloseMenu(g_Avatar, reveal);
        }
    }
    else
    {
        if (g_RevealStart < 0.0) g_RevealStart = ImGui::GetTime();
        const float reveal = ImClamp((float)((ImGui::GetTime() - g_RevealStart) / 0.48), 0.0f, 1.0f);
        RenderNeverloseMenu(g_Avatar, reveal);
    }

    // Track exactly the draw lists the menu owns so EndFrame only moves those.
    if (const ImGuiWindow* menu = ImGui::FindWindowByName("Neverlose Menu"))
        g_menuLists.push_back(menu->DrawList);
    if (const ImGuiWindow* intro = ImGui::FindWindowByName("##hexsync_intro"))
        g_menuLists.push_back(intro->DrawList);
}

void EndFrame()
{
    ImDrawData* dd = ImGui::GetDrawData();
    if (!dd) return;
    const float fw = (float)g_layout.width, fh = (float)g_layout.height;

    for (int n = 0; n < dd->CmdListsCount; ++n)
    {
        ImDrawList* list = dd->CmdLists[n];
        const bool isMenuList = std::find(g_menuLists.begin(), g_menuLists.end(), list) != g_menuLists.end();
        if (!isMenuList) continue;
        // nl::TransformDrawData, applied to this one list only.
        for (int i = 0; i < list->VtxBuffer.Size; ++i)
        {
            ImDrawVert& v = list->VtxBuffer[i];
            v.pos.x = v.pos.x * g_layout.scale + g_layout.origin.x;
            v.pos.y = v.pos.y * g_layout.scale + g_layout.origin.y;
        }
        for (int c = 0; c < list->CmdBuffer.Size; ++c)
        {
            ImVec4& r = list->CmdBuffer[c].ClipRect;
            r.x = std::max(0.0f, r.x * g_layout.scale + g_layout.origin.x);
            r.y = std::max(0.0f, r.y * g_layout.scale + g_layout.origin.y);
            r.z = std::min(fw,   r.z * g_layout.scale + g_layout.origin.x);
            r.w = std::min(fh,   r.w * g_layout.scale + g_layout.origin.y);
        }
    }

    dd->DisplayPos = ImVec2(0, 0);
    dd->DisplaySize = ImVec2(fw, fh);
    dd->FramebufferScale = ImVec2(1, 1);
}

// ---- input ------------------------------------------------------------------
void OnTouch(int action, float x, float y)
{
    if (!g_initialized) return;
    if (action == Touch::Down)
    {
        if (!g_visible && x >= g_chipMin.x && x <= g_chipMax.x &&
            y >= g_chipMin.y && y <= g_chipMax.y)
        {
            // Tap on the chip brings the menu back and is consumed here, so the
            // same press cannot also land on a control inside the menu.
            SetVisible(true);
            return;
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    // Touch positions arrive in physical pixels; the UI lives in the logical canvas.
    const ImVec2 p = nl::ToLogical(g_layout, x, y);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    switch (action)
    {
    case Touch::Down:
        io.AddMousePosEvent(p.x, p.y);
        io.AddMouseButtonEvent(0, true);
        break;
    case Touch::Move:
        io.AddMousePosEvent(p.x, p.y);
        break;
    case Touch::Up:
        io.AddMousePosEvent(p.x, p.y);
        io.AddMouseButtonEvent(0, false);
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);   // no hover highlight after the finger lifts
        break;
    default:
        break;
    }
}

} // namespace nl_ui
