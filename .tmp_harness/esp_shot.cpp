// Host side visual check for the ESP preview model.
// Renders esp_model_preview::DrawModel into a PNG with a tiny software
// rasterizer so the result can be inspected without a device.

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "esp_model_preview.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int g_w = 0, g_h = 0;
static std::vector<uint32_t> g_pix;

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

static void Tri(const ImVec2& a, const ImVec2& b, const ImVec2& c, uint32_t col, float alpha)
{
    const float area = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (fabsf(area) < 1e-6f) return;
    const float minx = fminf(a.x, fminf(b.x, c.x)), maxx = fmaxf(a.x, fmaxf(b.x, c.x));
    const float miny = fminf(a.y, fminf(b.y, c.y)), maxy = fmaxf(a.y, fmaxf(b.y, c.y));
    const int x0 = (int)floorf(minx), x1 = (int)ceilf(maxx);
    const int y0 = (int)floorf(miny), y1 = (int)ceilf(maxy);
    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            const float px = x + 0.5f, py = y + 0.5f;
            const float wa = ((b.x - px) * (c.y - py) - (c.x - px) * (b.y - py)) / area;
            const float wb = ((c.x - px) * (a.y - py) - (a.x - px) * (c.y - py)) / area;
            const float wc = ((a.x - px) * (b.y - py) - (b.x - px) * (a.y - py)) / area;
            if (wa < -0.002f || wb < -0.002f || wc < -0.002f) continue;
            Put(x, y, col, alpha);
        }
    }
}

static void RenderList(ImDrawList* dl)
{
    for (const ImDrawCmd& cmd : dl->CmdBuffer)
    {
        if (cmd.UserCallback) continue;
        const int vtx_off = (int)cmd.VtxOffset;
        const int idx_off = (int)cmd.IdxOffset;
        for (unsigned int e = 0; e + 2 < cmd.ElemCount; e += 3)
        {
            const ImDrawIdx* i = dl->IdxBuffer.Data + idx_off + e;
            const ImDrawVert& v0 = dl->VtxBuffer.Data[vtx_off + i[0]];
            const ImDrawVert& v1 = dl->VtxBuffer.Data[vtx_off + i[1]];
            const ImDrawVert& v2 = dl->VtxBuffer.Data[vtx_off + i[2]];
            const float alpha = (float)((v0.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
            Tri(v0.pos, v1.pos, v2.pos, v0.col, alpha);
        }
    }
}

int main(int argc, char** argv)
{
    const float yaw = (argc > 1) ? (float)atof(argv[1]) : 3.56f;
    const char* out = (argc > 2) ? argv[2] : "esp_model.png";
    const int width = (argc > 3) ? atoi(argv[3]) : 460;
    const int height = (argc > 4) ? atoi(argv[4]) : 1000;

    g_w = width;
    g_h = height;
    g_pix.assign((size_t)g_w * g_h, 0xFF000000u);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_w, (float)g_h);
    io.DeltaTime = 1.0f / 60.0f;

    ImDrawList* dl = new ImDrawList(ImGui::GetDrawListSharedData());
    dl->_ResetForNewFrame();
    dl->PushClipRect(ImVec2(0, 0), ImVec2((float)g_w, (float)g_h), true);
    dl->AddRectFilled(ImVec2(0, 0), ImVec2((float)g_w, (float)g_h), IM_COL32(19, 16, 27, 255));

    (void)yaw;   // the preview view is fixed now
    esp_model_preview::DrawModel(dl, ImVec2(20, 20), ImVec2((float)g_w - 40, (float)g_h - 40));

    std::printf("cmds=%d vtx=%d idx=%d\n", (int)dl->CmdBuffer.Size, (int)dl->VtxBuffer.Size, (int)dl->IdxBuffer.Size);
    for (int i = 0; i < (int)dl->CmdBuffer.Size && i < 6; ++i) {
        const ImDrawCmd& c = dl->CmdBuffer.Data[i];
        std::printf("cmd %d: clip=(%.0f,%.0f,%.0f,%.0f) vtxoff=%u idxoff=%u elem=%u\n", i,
            c.ClipRect.x, c.ClipRect.y, c.ClipRect.z, c.ClipRect.w, c.VtxOffset, c.IdxOffset, c.ElemCount);
    }
    for (int i = 0; i < 8 && i < (int)dl->VtxBuffer.Size; ++i) {
        const ImDrawVert& v = dl->VtxBuffer.Data[i];
        std::printf("vtx %d: pos=(%.1f,%.1f) col=%08x\n", i, v.pos.x, v.pos.y, v.col);
    }
    RenderList(dl);

    stbi_write_png(out, g_w, g_h, 4, g_pix.data(), g_w * 4);
    std::printf("wrote %s (%dx%d)\n", out, g_w, g_h);
    return 0;
}
