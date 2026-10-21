// Minimal software rasterizer for ImDrawData, so a frame drawn by the widget
// layer can be turned into an image without a GPU or a windowing system.
//
// It handles what this menu actually uses: one texture (the font atlas), filled
// and AA'd triangles, and rect clips. Textures other than the atlas are ignored,
// which is all right here - the widget pass draws no images. Rendering happens at
// 2x and is box-downsampled, which is what makes the glyph edges and the 3px
// slider track legible in the output.
//
// Output: raw RGBA, width/height printed to stderr as "WIDTH HEIGHT".
#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

namespace im5shot
{
    inline void PutPixel(std::vector<float>& buf, int w, int h, int x, int y,
                         float r, float g, float b, float a)
    {
        if (x < 0 || y < 0 || x >= w || y >= h || a <= 0.0f)
            return;
        float* p = &buf[(size_t)(y * w + x) * 3];
        p[0] = r * a + p[0] * (1.0f - a);
        p[1] = g * a + p[1] * (1.0f - a);
        p[2] = b * a + p[2] * (1.0f - a);
    }

    // Writes the last rendered frame into `path` as raw RGBA at `ss` samples per
    // axis, downsampled to the display size. `bg` fills the areas no draw command
    // covers (the app's shell colour).
    inline bool RenderToRaw(const char* path, float ss, ImU32 bg)
    {
        ImDrawData* dd = ImGui::GetDrawData();
        if (dd == nullptr)
            return false;

        const int W = (int)dd->DisplaySize.x;
        const int H = (int)dd->DisplaySize.y;
        if (W <= 0 || H <= 0)
            return false;

        const int RW = (int)(W * ss);
        const int RH = (int)(H * ss);
        std::vector<float> hi((size_t)RW * RH * 3);

        const float br = ((bg >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
        const float bg_ = ((bg >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
        const float bb = ((bg >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
        for (size_t i = 0; i < hi.size(); i += 3)
        {
            hi[i + 0] = br;
            hi[i + 1] = bg_;
            hi[i + 2] = bb;
        }

        unsigned char* tex = nullptr;
        int tex_w = 0, tex_h = 0;
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex, &tex_w, &tex_h);

        for (int n = 0; n < dd->CmdListsCount; ++n)
        {
            const ImDrawList* list = dd->CmdLists[n];
            const ImDrawVert* vtx = list->VtxBuffer.Data;
            const ImDrawIdx* idx = list->IdxBuffer.Data;

            for (int c = 0; c < list->CmdBuffer.Size; ++c)
            {
                const ImDrawCmd* cmd = &list->CmdBuffer[c];
                if (cmd->UserCallback != nullptr || cmd->ElemCount == 0)
                    continue;
                // Every command here samples the font atlas: with no backend
                // installed there is no other texture, and TexID stays 0.

                const int cx0 = (int)ImMax(cmd->ClipRect.x * ss, 0.0f);
                const int cy0 = (int)ImMax(cmd->ClipRect.y * ss, 0.0f);
                const int cx1 = (int)ImMin(cmd->ClipRect.z * ss, (float)RW);
                const int cy1 = (int)ImMin(cmd->ClipRect.w * ss, (float)RH);
                if (cx1 <= cx0 || cy1 <= cy0)
                    continue;

                for (unsigned e = 0; e + 2 < cmd->ElemCount; e += 3)
                {
                    const ImDrawVert& a = vtx[idx[cmd->IdxOffset + e + 0] + cmd->VtxOffset];
                    const ImDrawVert& b = vtx[idx[cmd->IdxOffset + e + 1] + cmd->VtxOffset];
                    const ImDrawVert& d = vtx[idx[cmd->IdxOffset + e + 2] + cmd->VtxOffset];

                    const float ax = a.pos.x * ss, ay = a.pos.y * ss;
                    const float bx = b.pos.x * ss, by = b.pos.y * ss;
                    const float dx = d.pos.x * ss, dy = d.pos.y * ss;

                    const float area = (bx - ax) * (dy - ay) - (by - ay) * (dx - ax);
                    if (std::fabs(area) < 0.00001f)
                        continue;

                    const int minx = ImMax(cx0, (int)std::floor(ImMin(ax, ImMin(bx, dx))));
                    const int maxx = ImMin(cx1 - 1, (int)std::ceil(ImMax(ax, ImMax(bx, dx))));
                    const int miny = ImMax(cy0, (int)std::floor(ImMin(ay, ImMin(by, dy))));
                    const int maxy = ImMin(cy1 - 1, (int)std::ceil(ImMax(ay, ImMax(by, dy))));

                    const float inv_area = 1.0f / area;
                    for (int y = miny; y <= maxy; ++y)
                    {
                        for (int x = minx; x <= maxx; ++x)
                        {
                            const float px = x + 0.5f, py = y + 0.5f;
                            float w0 = ((bx - px) * (dy - py) - (by - py) * (dx - px)) * inv_area;
                            float w1 = ((px - ax) * (dy - ay) - (py - ay) * (dx - ax)) * inv_area;
                            float w2 = 1.0f - w0 - w1;
                            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                                continue;

                            const float u = w0 * a.uv.x + w1 * b.uv.x + w2 * d.uv.x;
                            const float v = w0 * a.uv.y + w1 * b.uv.y + w2 * d.uv.y;

                            float cr = 0.0f, cg = 0.0f, cb = 0.0f, ca = 0.0f;
                            const float weights[3] = { w0, w1, w2 };
                            const ImDrawVert* verts[3] = { &a, &b, &d };
                            for (int k = 0; k < 3; ++k)
                            {
                                const ImU32 col = verts[k]->col;
                                const float alpha = ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
                                ca += weights[k] * alpha;
                                cr += weights[k] * (((col >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f) * alpha;
                                cg += weights[k] * (((col >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f) * alpha;
                                cb += weights[k] * (((col >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f) * alpha;
                            }
                            if (ca <= 0.0f)
                                continue;
                            cr /= ca; cg /= ca; cb /= ca;

                            int tx = (int)(u * tex_w);
                            int ty = (int)(v * tex_h);
                            tx = ImClamp(tx, 0, tex_w - 1);
                            ty = ImClamp(ty, 0, tex_h - 1);
                            const unsigned char* t = &tex[((size_t)ty * tex_w + tx) * 4];
                            const float ta = t[3] / 255.0f;
                            const float out_a = ca * ta;
                            if (out_a <= 0.0f)
                                continue;

                            PutPixel(hi, RW, RH, x, y,
                                     (t[0] / 255.0f) * cr,
                                     (t[1] / 255.0f) * cg,
                                     (t[2] / 255.0f) * cb,
                                     out_a);
                        }
                    }
                }
            }
        }

        // Box downsample `ss`x`ss` -> 1, then write RGBA.
        std::vector<unsigned char> out((size_t)W * H * 4);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                float r = 0.0f, g = 0.0f, b = 0.0f;
                int samples = 0;
                for (int sy = 0; sy < (int)ss; ++sy)
                {
                    for (int sx = 0; sx < (int)ss; ++sx)
                    {
                        const int hx = x * (int)ss + sx;
                        const int hy = y * (int)ss + sy;
                        if (hx >= RW || hy >= RH)
                            continue;
                        const float* p = &hi[(size_t)(hy * RW + hx) * 3];
                        r += p[0]; g += p[1]; b += p[2];
                        samples++;
                    }
                }
                const float inv = samples > 0 ? 1.0f / samples : 0.0f;
                unsigned char* o = &out[((size_t)y * W + x) * 4];
                o[0] = (unsigned char)ImClamp((int)(r * inv * 255.0f + 0.5f), 0, 255);
                o[1] = (unsigned char)ImClamp((int)(g * inv * 255.0f + 0.5f), 0, 255);
                o[2] = (unsigned char)ImClamp((int)(b * inv * 255.0f + 0.5f), 0, 255);
                o[3] = 255;
            }
        }

        FILE* f = fopen(path, "wb");
        if (f == nullptr)
            return false;
        fwrite(out.data(), 1, out.size(), f);
        fclose(f);

        fprintf(stderr, "wrote %s (%dx%d)\n", path, W, H);
        return true;
    }
}
