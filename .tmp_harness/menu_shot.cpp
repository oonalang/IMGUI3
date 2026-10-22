// Host side visual check for the whole menu including the ESP preview card.
// Renders raspody::Render() through the real ImGui and rasterises the draw
// data into a PNG so the card can be inspected without a device.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

struct Vector2 { float x, y; };

#include "Hacks/Feature/Init_Hacks.h"
#include "Fonts/fonts.h"
#include "System/Core/SaveConfig.h"
#include "ImGui/imgui.h"

float GetLogoOpacity();
void  SetLogoOpacity(float v);
float GetLogoSizeMultiplier();
void  SetLogoSizeMultiplier(float v);

#include "ImGui/raspody_menu.h"
#include "ImGui/esp_model_preview.h"
#include "ImGui/esp_preview.h"

bool  HIDEESP = false;
bool  UnlimitedFly = false;
float SnowBsize = 0.0f;
float speedHackMultiplier = 1.0f;
float jumpHeightMultiplier = 1.0f;
float menu[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
bool  isJumpAdjustmentEnabled = false;
bool  SnowB = false;
bool  isSpeedHackEnabled = false;

std::string usedKey = "HOST-TEST";
std::string userType = "PREMIUM";

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static float g_logoOpacity = 1.0f;
static float g_logoSize = 1.0f;
float GetLogoOpacity() { return g_logoOpacity; }
void  SetLogoOpacity(float v) { g_logoOpacity = v; }
float GetLogoSizeMultiplier() { return g_logoSize; }
void  SetLogoSizeMultiplier(float v) { g_logoSize = v; }

// ---------------------------------------------------------------- rasterizer

static int g_w = 0, g_h = 0;
static std::vector<uint32_t> g_pix;
static const unsigned char* g_atlas = nullptr;   // alpha8 coverage
static int g_atlasW = 0, g_atlasH = 0;

static void Put(int x, int y, uint32_t c, float a)
{
    if (x < 0 || y < 0 || x >= g_w || y >= g_h) return;
    const float dr = (float)(c & 0xFF);
    const float dg = (float)((c >> 8) & 0xFF);
    const float db = (float)((c >> 16) & 0xFF);
    const float da = ((c >> 24) & 0xFF) / 255.0f * a;
    uint32_t& p = g_pix[(size_t)y * g_w + x];
    const float pr = (float)(p & 0xFF);
    const float pg = (float)((p >> 8) & 0xFF);
    const float pb = (float)((p >> 16) & 0xFF);
    p = (uint32_t)(pr + (dr - pr) * da)
      | ((uint32_t)(pg + (dg - pg) * da) << 8)
      | ((uint32_t)(pb + (db - pb) * da) << 16)
      | 0xFF000000u;
}

struct Vtx
{
    float x, y, u, v;
    uint32_t col;
};

static void TriTextured(const Vtx& a, const Vtx& b, const Vtx& c, const ImVec4& clip)
{
    const float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (std::fabs(area) < 1e-6f) return;
    const float minx = std::fmin(a.x, std::fmin(b.x, c.x)), maxx = std::fmax(a.x, std::fmax(b.x, c.x));
    const float miny = std::fmin(a.y, std::fmin(b.y, c.y)), maxy = std::fmax(a.y, std::fmax(b.y, c.y));
    const int x0 = std::max((int)std::floor(minx), (int)std::floor(clip.x));
    const int x1 = std::min((int)std::ceil(maxx), (int)std::ceil(clip.z));
    const int y0 = std::max((int)std::floor(miny), (int)std::floor(clip.y));
    const int y1 = std::min((int)std::ceil(maxy), (int)std::ceil(clip.w));

    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            const float px = x + 0.5f, py = y + 0.5f;
            const float wa = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
            const float wb = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
            const float wc = ((a.x - px) * (b.y - py) - (b.x - px) * (a.y - py)) / area;
            if (wa < -0.002f || wb < -0.002f || wc < -0.002f) continue;
            const float u = a.u * wa + b.u * wb + c.u * wc;
            const float v = a.v * wa + b.v * wb + c.v * wc;
            const int tx = (int)(u * (float)g_atlasW);
            const int ty = (int)(v * (float)g_atlasH);
            if (tx < 0 || ty < 0 || tx >= g_atlasW || ty >= g_atlasH) continue;
            const float cov = (float)g_atlas[(size_t)ty * g_atlasW + tx] / 255.0f;
            if (cov <= 0.0f) continue;
            const uint32_t col = a.col;
            Put(x, y, col, cov);
        }
    }
}

static void TriSolid(const Vtx& a, const Vtx& b, const Vtx& c, const ImVec4& clip)
{
    const float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (std::fabs(area) < 1e-6f) return;
    const float minx = std::fmin(a.x, std::fmin(b.x, c.x)), maxx = std::fmax(a.x, std::fmax(b.x, c.x));
    const float miny = std::fmin(a.y, std::fmin(b.y, c.y)), maxy = std::fmax(a.y, std::fmax(b.y, c.y));
    const int x0 = std::max((int)std::floor(minx), (int)std::floor(clip.x));
    const int x1 = std::min((int)std::ceil(maxx), (int)std::ceil(clip.z));
    const int y0 = std::max((int)std::floor(miny), (int)std::floor(clip.y));
    const int y1 = std::min((int)std::ceil(maxy), (int)std::ceil(clip.w));

    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            const float px = x + 0.5f, py = y + 0.5f;
            const float wa = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
            const float wb = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
            const float wc = ((a.x - px) * (b.y - py) - (b.x - px) * (a.y - py)) / area;
            if (wa < -0.002f || wb < -0.002f || wc < -0.002f) continue;
            const float alpha = (float)((a.col >> 24) & 0xFF) / 255.0f;
            Put(x, y, a.col, alpha);
        }
    }
}

static void RenderDrawData(const ImDrawData* dd)
{
    for (int n = 0; n < dd->CmdListsCount; ++n)
    {
        const ImDrawList* cl = dd->CmdLists[n];
        for (const ImDrawCmd& cmd : cl->CmdBuffer)
        {
            if (cmd.UserCallback) continue;
            const ImVec4 clip(cmd.ClipRect.x - dd->DisplayPos.x, cmd.ClipRect.y - dd->DisplayPos.y,
                              cmd.ClipRect.z - dd->DisplayPos.x, cmd.ClipRect.w - dd->DisplayPos.y);
            const bool textured = cmd.GetTexID() != 0 && g_atlas != nullptr;
            const int vtx_off = (int)cmd.VtxOffset;
            const int idx_off = (int)cmd.IdxOffset;
            for (unsigned int e = 0; e + 2 < cmd.ElemCount; e += 3)
            {
                const ImDrawIdx* i = cl->IdxBuffer.Data + idx_off + e;
                Vtx v[3];
                for (int k = 0; k < 3; ++k)
                {
                    const ImDrawVert& s = cl->VtxBuffer.Data[vtx_off + i[k]];
                    v[k].x = s.pos.x - dd->DisplayPos.x;
                    v[k].y = s.pos.y - dd->DisplayPos.y;
                    v[k].u = s.uv.x;
                    v[k].v = s.uv.y;
                    v[k].col = s.col;
                }
                if (textured)
                    TriTextured(v[0], v[1], v[2], clip);
                else
                    TriSolid(v[0], v[1], v[2], clip);
            }
        }
    }
}

// ----------------------------------------------------------------------- main

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static float g_displayW = 1280.0f;
static float g_displayH = 720.0f;

static void Frame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_displayW, g_displayH);
    io.DeltaTime = 1.0f / 60.0f;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseDown[0] = false;
    ImGui::NewFrame();
    bool open = true;
    raspody::Render(&open);
    ImGui::Render();
}

int main(int argc, char** argv)
{
    const char* out = (argc > 1) ? argv[1] : "menu.png";
    const int frames = 30;
    if (argc > 4) { g_displayW = (float)atof(argv[3]); g_displayH = (float)atof(argv[4]); }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    raspody::LoadFonts();
    raspody::Style();
    io.Fonts->Build();

    ImFontAtlas* atlas = io.Fonts;
    g_atlasW = atlas->TexWidth;
    g_atlasH = atlas->TexHeight;
    g_atlas = atlas->TexPixelsAlpha8;
    atlas->SetTexID((ImTextureID)1);

    raspody::SetEspPreview(&esp_preview::Draw);

    int tabArg = (argc > 2) ? atoi(argv[2]) : (int)raspody::Tab_Visuals;
    if (tabArg < 0) tabArg = 0;
    if (tabArg >= (int)raspody::Tab_Count) tabArg = (int)raspody::Tab_Count - 1;
    raspody::var.tab.current = raspody::var.tab.stored = tabArg;
    int subArg = (argc > 5) ? atoi(argv[5]) : 0;
    if (subArg < 0 || subArg > 2) subArg = 0;
    raspody::var.subtab.current = raspody::var.subtab.stored = subArg;
    raspody::var.tab.alpha = 1.0f;
    raspody::var.subtab.alpha = 1.0f;

    Config.ESPMenu.Box = true;
    Config.ESPMenu.BoxType = EspBoxType::Corner;
    Config.ESPMenu.Name = true;
    Config.ESPMenu.Distance = true;
    Config.ESPMenu.Health = true;
    Config.ESPMenu.Skeleton = true;
    Config.ESPMenu.isPlayerLine = true;
    Config.ESPMenu.ShowFov = true;

    if (getenv("SHOT_KBD"))
    {
        raspody::ui.search[0] = '\0';
        raspody::ui.profileKeyboard = true;
    }

    for (int i = 0; i < frames; ++i)
        Frame();

    g_w = (int)io.DisplaySize.x;
    g_h = (int)io.DisplaySize.y;
    g_pix.assign((size_t)g_w * g_h, 0xFF101018u);
    RenderDrawData(ImGui::GetDrawData());
    stbi_write_png(out, g_w, g_h, 4, g_pix.data(), g_w * 4);
    std::printf("wrote %s (%dx%d)\n", out, g_w, g_h);
    ImGui::DestroyContext();
    return 0;
}
