// Host check for the Visuals-page ESP preview.
//
// Asserts the two things the device report was about: the box has to be the
// game's box (width = 0.65 * height, centred on the pawn's own axis, spanning
// head to feet) and the side health bar has to be the game's bar (full box
// height, filled from the bottom, in the red->green ramp). Then it rasterises
// the card into a PNG so the result can be looked at without a device.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "ImGui/esp_preview.h"
#include "ImGui/esp_shared.h"

// The patched imgui_widgets.cpp reads these two fonts.
namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static int g_failures = 0;

static void Check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

// ------------------------------------------------------------------ draw data

struct Vtx { float x, y, u, v; uint32_t col; };

static int g_w = 0, g_h = 0;
static std::vector<uint32_t> g_pix;
static const unsigned char* g_atlas = nullptr;
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
            Put(x, y, a.col, cov);
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
                if (textured) TriTextured(v[0], v[1], v[2], clip);
                else          TriSolid(v[0], v[1], v[2], clip);
            }
        }
    }
}

// ------------------------------------------------------------------ geometry

struct Bounds
{
    bool found = false;
    float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
    int count = 0;
};

static Bounds BoundsOfColour(const ImDrawList& dl, ImU32 col)
{
    Bounds b;
    for (int i = 0; i < dl.VtxBuffer.Size; ++i)
    {
        const ImDrawVert& v = dl.VtxBuffer[i];
        if (v.col != col) continue;
        if (!b.found)
        {
            b.found = true;
            b.minX = b.maxX = v.pos.x;
            b.minY = b.maxY = v.pos.y;
        }
        else
        {
            b.minX = ImMin(b.minX, v.pos.x);
            b.maxX = ImMax(b.maxX, v.pos.x);
            b.minY = ImMin(b.minY, v.pos.y);
            b.maxY = ImMax(b.maxY, v.pos.y);
        }
        b.count++;
    }
    return b;
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char** argv)
{
    const char* out = (argc > 1) ? argv[1] : nullptr;

    setvbuf(stdout, nullptr, _IONBF, 0);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(560.0f, 400.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.BackendPlatformName = "headless";
    // The preview measures text with ImGui::GetFont(); a context with no font
    // at all makes that read outside the atlas.
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ImGui::NewFrame();

    const ImVec2 pos(20.0f, 20.0f);
    const float W = 520.0f, H = 360.0f;

    ImVec2 mp, ms;
    esp_preview::ModelArea(pos, W, H, mp, ms);

    ImDrawList scratch(ImGui::GetDrawListSharedData());
    scratch._ResetForNewFrame();
    const esp_model_preview::Result r = esp_model_preview::DrawModel(&scratch, mp, ms);
    Check("the model renders", r.valid);
    if (!r.valid)
    {
        ImGui::EndFrame();
        return 1;
    }

    const esp_preview::BoxGeometry g = esp_preview::ComputeBox(r, mp, ms);

    // --- the box is the game's box
    Check("box width is 0.65 of its height",
          std::fabs(g.width - g.height * esp_preview::kBoxWidthRatio) < 0.01f);
    Check("box is centred on the model's own axis",
          std::fabs((g.left + g.width * 0.5f) - (mp.x + ms.x * 0.5f)) < 0.01f);
    Check("box spans head to feet",
          std::fabs(g.top - r.min.y) < 0.01f &&
          std::fabs((g.top + g.height) - r.max.y) < 0.01f);

    // The old preview framed the silhouette, which is not the game's box; the
    // box centre has to be the card's centre line.
    Check("box is centred on the card",
          std::fabs((g.left + g.width * 0.5f) - (pos.x + W * 0.5f)) < 0.01f);

    // --- the health ramp is the game's ramp
    const ImU32 full = esp_shared::HealthColor(100, 100);
    const ImU32 empty = esp_shared::HealthColor(0, 100);
    const ImU32 half = esp_shared::HealthColor(50, 100);
    Check("full health is green",
          ((full >> IM_COL32_G_SHIFT) & 0xFF) == 255 && ((full >> IM_COL32_R_SHIFT) & 0xFF) == 0);
    Check("empty health is red",
          ((empty >> IM_COL32_R_SHIFT) & 0xFF) == 255 && ((empty >> IM_COL32_G_SHIFT) & 0xFF) == 0);
    Check("half health is yellow",
          ((half >> IM_COL32_R_SHIFT) & 0xFF) == 255 && ((half >> IM_COL32_G_SHIFT) & 0xFF) == 255);

    // --- side bar: full box height, filled from the bottom
    ImDrawList dl(ImGui::GetDrawListSharedData());
    dl._ResetForNewFrame();
    const ImU32 boxCol = IM_COL32(255, 64, 64, 255);
    esp_preview::Draw(&dl, pos, W, H,
                      /*box*/ true, /*line*/ false, /*skeleton*/ false,
                      /*health*/ true, /*name*/ false, /*distance*/ false,
                      IM_COL32(255, 0, 0, 255), boxCol, IM_COL32(0, 255, 0, 255),
                      IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 0, 255),
                      esp_shared::BoxOutline, esp_shared::HealthSide, esp_shared::LineTop);

    const Bounds boxB = BoundsOfColour(dl, boxCol);
    Check("the box is drawn at the computed rect",
          boxB.found &&
          std::fabs(boxB.minX - g.left) < 1.0f &&
          std::fabs(boxB.maxX - (g.left + g.width)) < 1.0f &&
          std::fabs(boxB.minY - g.top) < 1.0f &&
          std::fabs(boxB.maxY - (g.top + g.height)) < 1.0f);

    const ImU32 fillCol = esp_shared::HealthColor(72, 100);
    const Bounds fill = BoundsOfColour(dl, fillCol);
    Check("the side health bar exists", fill.found);
    if (fill.found)
    {
        const float expectTop = g.top + g.height * (1.0f - 0.72f);
        Check("the side bar starts at the bottom of the box",
              std::fabs(fill.maxY - (g.top + g.height)) < 1.5f);
        Check("the fill height follows the health fraction",
              std::fabs((fill.maxY - fill.minY) - g.height * 0.72f) < 2.0f);
        Check("the fill starts at the ratio line",
              std::fabs(fill.minY - expectTop) < 2.0f);
        // Game: track starts 8 px left of the box, so its right edge is 4 px out.
        Check("the side bar stands off the left of the box",
              std::fabs((g.left - fill.maxX) - 4.0f) < 1.5f);
    }

    // --- top bar: a thin strip inside the name container
    ImDrawList dlTop(ImGui::GetDrawListSharedData());
    dlTop._ResetForNewFrame();
    esp_preview::Draw(&dlTop, pos, W, H, true, false, false, true, true, false,
                      IM_COL32(255, 0, 0, 255), boxCol, IM_COL32(0, 255, 0, 255),
                      IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 0, 255),
                      esp_shared::BoxOutline, esp_shared::HealthTop, esp_shared::LineTop);
    const Bounds topFill = BoundsOfColour(dlTop, fillCol);
    Check("the top health bar exists", topFill.found);
    if (topFill.found)
    {
        Check("the top bar is short, not a side bar",
              (topFill.maxY - topFill.minY) < (g.height * 0.2f));
        Check("the top bar is above the box", topFill.maxY < g.top);
    }

    // --- corner style emits eight segments, not a rectangle
    ImDrawList dlCorner(ImGui::GetDrawListSharedData());
    dlCorner._ResetForNewFrame();
    esp_preview::Draw(&dlCorner, pos, W, H, true, false, false, false, false, false,
                      IM_COL32(255, 0, 0, 255), boxCol, IM_COL32(0, 255, 0, 255),
                      IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 0, 255),
                      esp_shared::BoxCorner, esp_shared::HealthSide, esp_shared::LineTop);
    const Bounds cornerB = BoundsOfColour(dlCorner, boxCol);
    // Eight AddLine calls, four vertices each.
    Check("the corner style draws eight segments", cornerB.count == 32);

    // --- line anchors
    ImDrawList dlLineTop(ImGui::GetDrawListSharedData());
    dlLineTop._ResetForNewFrame();
    const ImU32 lineCol = IM_COL32(0, 200, 255, 255);
    esp_preview::Draw(&dlLineTop, pos, W, H, false, true, false, false, false, false,
                      lineCol, boxCol, IM_COL32(0, 255, 0, 255),
                      IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 0, 255),
                      esp_shared::BoxOutline, esp_shared::HealthSide, esp_shared::LineTop);
    ImDrawList dlLineBottom(ImGui::GetDrawListSharedData());
    dlLineBottom._ResetForNewFrame();
    esp_preview::Draw(&dlLineBottom, pos, W, H, false, true, false, false, false, false,
                      lineCol, boxCol, IM_COL32(0, 255, 0, 255),
                      IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 0, 255),
                      esp_shared::BoxOutline, esp_shared::HealthSide, esp_shared::LineBottom);
    const Bounds lineTop = BoundsOfColour(dlLineTop, lineCol);
    const Bounds lineBottom = BoundsOfColour(dlLineBottom, lineCol);
    Check("the top line starts at the top of the card",
          lineTop.found && lineTop.minY < pos.y + 12.0f);
    Check("the bottom line runs from the card's floor to the feet",
          lineBottom.found &&
          std::fabs(lineBottom.minY - (g.top + g.height)) < 2.0f &&
          lineBottom.maxY > (g.top + g.height) + 10.0f);

    if (out != nullptr)
    {
        ImGui::EndFrame();

        ImGui::NewFrame();
        ImDrawList* png = ImGui::GetBackgroundDrawList();
        png->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(12, 10, 18, 255));
        esp_preview::Draw(png, pos, W, H, true, true, true, true, true, true,
                          IM_COL32(255, 90, 90, 255), IM_COL32(255, 64, 64, 255),
                          IM_COL32(0, 255, 0, 255), IM_COL32(255, 255, 255, 255),
                          IM_COL32(255, 220, 90, 255),
                          esp_shared::BoxCorner, esp_shared::HealthSide,
                          esp_shared::LineTop);
        ImGui::Render();

        ImDrawData* dd = ImGui::GetDrawData();
        g_w = (int)io.DisplaySize.x;
        g_h = (int)io.DisplaySize.y;
        g_pix.assign((size_t)g_w * g_h, 0xFF000000u);
        ImFontAtlas* atlas = io.Fonts;
        g_atlasW = atlas->TexWidth;
        g_atlasH = atlas->TexHeight;
        g_atlas = atlas->TexPixelsAlpha8;
        atlas->SetTexID((ImTextureID)1);
        RenderDrawData(dd);

        std::vector<unsigned char> rgba((size_t)g_w * g_h * 4);
        for (size_t i = 0; i < g_pix.size(); ++i)
        {
            rgba[i * 4 + 0] = (unsigned char)(g_pix[i] & 0xFF);
            rgba[i * 4 + 1] = (unsigned char)((g_pix[i] >> 8) & 0xFF);
            rgba[i * 4 + 2] = (unsigned char)((g_pix[i] >> 16) & 0xFF);
            rgba[i * 4 + 3] = 255;
        }
        stbi_write_png(out, g_w, g_h, 4, rgba.data(), g_w * 4);
        std::printf("wrote %s (%dx%d)\n", out, g_w, g_h);
    }
    else
    {
        ImGui::EndFrame();
    }

    ImGui::DestroyContext();

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }
    std::printf("OK: ESP preview geometry\n");
    return 0;
}
