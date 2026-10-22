#include <cstdint>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "System/UI/LoadingAnimation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static int g_w = 0, g_h = 0;
static std::vector<uint32_t> g_pix;
static const unsigned char* g_atlas = nullptr;
static int g_atlasW = 0, g_atlasH = 0;

// Stand-in for the decoded video frame: the real Android path hands the loader an
// ImTextureID from the OES -> RGBA blit, so the harness feeds a real frame from
// the mp4 through the same code.
static const unsigned char* g_img = nullptr;
static int g_imgW = 0, g_imgH = 0;
static const ImTextureID kImgTex = (ImTextureID)2;

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

struct Vtx { float x, y, u, v, a; uint32_t col; };

static void TriTextured(const Vtx& a, const Vtx& b, const Vtx& c, const ImVec4& clip, bool first, bool image)
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
        for (int x = x0; x <= x1; ++x)
        {
            const float px = x + 0.5f, py = y + 0.5f;
            const float wa = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
            const float wb = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
            const float wc = ((a.x - px) * (b.y - py) - (b.x - px) * (a.y - py)) / area;
            if (wa < -0.002f || wb < -0.002f || wc < -0.002f) continue;
            if (first && wb <= 1e-4f) continue;
            const float u = a.u * wa + b.u * wb + c.u * wc;
            const float v = a.v * wa + b.v * wb + c.v * wc;
            if (image)
            {
                int tx = (int)(u * (float)g_imgW);
                int ty = (int)(v * (float)g_imgH);
                tx = tx < 0 ? 0 : (tx >= g_imgW ? g_imgW - 1 : tx);
                ty = ty < 0 ? 0 : (ty >= g_imgH ? g_imgH - 1 : ty);
                const unsigned char* px = g_img + ((size_t)ty * g_imgW + tx) * 4;
                const uint32_t col = (uint32_t)px[0] | ((uint32_t)px[1] << 8) |
                                     ((uint32_t)px[2] << 16) | 0xFF000000u;
                Put(x, y, col, a.a * wa + b.a * wb + c.a * wc);
                continue;
            }
            const int tx = (int)(u * (float)g_atlasW);
            const int ty = (int)(v * (float)g_atlasH);
            if (tx < 0 || ty < 0 || tx >= g_atlasW || ty >= g_atlasH) continue;
            const float cov = (float)g_atlas[(size_t)ty * g_atlasW + tx] / 255.0f;
            if (cov <= 0.0f) continue;
            const float alpha = (a.a * wa + b.a * wb + c.a * wc) * cov;
            Put(x, y, a.col, alpha);
        }
}

static void TriSolid(const Vtx& a, const Vtx& b, const Vtx& c, const ImVec4& clip, bool first)
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
        for (int x = x0; x <= x1; ++x)
        {
            const float px = x + 0.5f, py = y + 0.5f;
            const float wa = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
            const float wb = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
            const float wc = ((a.x - px) * (b.y - py) - (b.x - px) * (a.y - py)) / area;
            if (wa < -0.002f || wb < -0.002f || wc < -0.002f) continue;
            if (first && wb <= 1e-4f) continue;
            const float alpha = a.a * wa + b.a * wb + c.a * wc;
            Put(x, y, a.col, alpha);
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
            const bool image = (cmd.GetTexID() == kImgTex && g_img != nullptr);
            const bool textured = image || (cmd.GetTexID() != 0 && g_atlas != nullptr);
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
                    v[k].a = ((s.col >> 24) & 0xFF) / 255.0f;
                    v[k].col = s.col;
                }
                if (textured) TriTextured(v[0], v[1], v[2], clip, false, image);
                else          TriSolid(v[0], v[1], v[2], clip, false);
            }
        }
    }
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


int main(int argc, char** argv)
{
    const float W = (argc > 1) ? (float)atof(argv[1]) : 1280.0f;
    const float H = (argc > 2) ? (float)atof(argv[2]) : 720.0f;
    const char* out = (argc > 3) ? argv[3] : ".tmp_harness/loader.png";
    const int frames = (argc > 4) ? atoi(argv[4]) : 150;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(W, H);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ImFontAtlas* atlas = io.Fonts;
    g_atlasW = atlas->TexWidth;
    g_atlasH = atlas->TexHeight;
    g_atlas = atlas->TexPixelsAlpha8;
    atlas->SetTexID((ImTextureID)1);

    {
        // LOADER_BG points at a raw RGBA frame (ffmpeg -pix_fmt rgba -f rawvideo),
        // with LOADER_BG_W / LOADER_BG_H giving its size.
        const char* bg = getenv("LOADER_BG");
        const char* bw = getenv("LOADER_BG_W");
        const char* bh = getenv("LOADER_BG_H");
        if (bg && bw && bh)
        {
            FILE* f = fopen(bg, "rb");
            if (f != nullptr)
            {
                const int w = atoi(bw);
                const int h = atoi(bh);
                unsigned char* img = (unsigned char*)malloc((size_t)w * (size_t)h * 4);
                if (img != nullptr && fread(img, 1, (size_t)w * (size_t)h * 4, f) == (size_t)w * (size_t)h * 4)
                {
                    g_img = img;
                    g_imgW = w;
                    g_imgH = h;
                }
                fclose(f);
            }
        }
    }

    ui_loading::Start(7.0f);

    bool finished = false;
    for (int i = 0; i < frames && !finished; ++i)
    {
        ImGui::NewFrame();
        finished = ui_loading::RenderWindow(g_img ? kImgTex : nullptr);
        ImGui::Render();
    }

    g_w = (int)io.DisplaySize.x;
    g_h = (int)io.DisplaySize.y;
    g_pix.assign((size_t)g_w * g_h, 0xFF000000u);
    RenderDrawData(ImGui::GetDrawData());

    const int written = stbi_write_png(out, g_w, g_h, 4, g_pix.data(), g_w * 4);
    std::printf("%s %dx%d frames=%d active=%d progress=%.2f\n",
                out, g_w, g_h, frames, (int)ui_loading::IsActive(),
                ui_loading::GetProgress(), written);

    ImGui::DestroyContext();
    return written ? 0 : 1;
}
