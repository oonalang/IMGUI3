// Renders the shipped login panel at a phone surface (2340x1080) and composites
// the real IMAGE/miku.h artwork into the artwork well, at the rectangle the
// shipped Render() computes - the software rasteriser cannot sample an app
// texture, so the picture is pasted in afterwards from the same decode path the
// device uses (stb_image, the vendored build).
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/login_screen.h"
#include "ImGui/stb_image.h"
#include "IMAGE/miku.h"
#include "../dumps/im5_soft_render.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;

static void CompositeArtwork(const char* rawPath, float W, float H)
{
    int iw = 0, ih = 0, comp = 0;
    unsigned char* img = stbi_load_from_memory(miku_data, (int)sizeof(miku_data), &iw, &ih, &comp, 4);
    if (!img) { std::printf("artwork decode failed: %s\n", stbi_failure_reason()); return; }

    const login_screen::Layout L = login_screen::ComputeLayout(ImVec2(0, 0), W, H, false);
    const float colW = L.wellMax.x - L.wellMin.x;
    const float colH = L.wellMax.y - L.wellMin.y;
    const float k = ImMin(colW / (float)iw, colH / (float)ih);
    const float dw = iw * k, dh = ih * k;
    const float x0 = L.wellMin.x + (colW - dw) * 0.62f;
    const float y0 = L.wellMax.y - dh;

    std::printf("artwork well %.0fx%.0f, image %dx%d -> %.0fx%.0f at %.0f,%.0f\n",
                colW, colH, iw, ih, dw, dh, x0, y0);

    FILE* f = std::fopen(rawPath, "r+b");
    if (!f) { stbi_image_free(img); return; }
    const int w = (int)W, h = (int)H;
    std::vector<unsigned char> px((size_t)w * h * 4);
    std::fread(px.data(), 1, px.size(), f);

    for (int y = (int)y0; y < (int)(y0 + dh); ++y)
    {
        if (y < 0 || y >= h) continue;
        for (int x = (int)x0; x < (int)(x0 + dw); ++x)
        {
            if (x < 0 || x >= w) continue;
            const float u = (x - x0) / dw * (iw - 1);
            const float v = (y - y0) / dh * (ih - 1);
            const int u0 = (int)u, v0 = (int)v;
            const unsigned char* s = img + ((size_t)v0 * iw + u0) * 4;
            unsigned char* d = px.data() + ((size_t)y * w + x) * 4;
            // The picture is opaque (baked onto kArtBg), so a straight copy is
            // what the device draws.
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
        }
    }

    std::fseek(f, 0, SEEK_SET);
    std::fwrite(px.data(), 1, px.size(), f);
    std::fclose(f);
    stbi_image_free(img);
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(2340.0f, 1080.0f);
    io.DeltaTime = 1.0f / 60.0f;

    login_screen::LoadFonts(io);
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    static char key[128] = "ETHNIR-AB12CD34";
    static bool reveal = false;
    static bool remember = true;

    for (int i = 0; i < 3; ++i)
    {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("##login_shot", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove);

        login_screen::Params p;
        p.pos = ImVec2(0, 0);
        p.W = io.DisplaySize.x;
        p.H = io.DisplaySize.y;
        p.key = key;
        p.keySize = (int)sizeof(key);
        p.reveal = &reveal;
        p.remember = &remember;
        // No texture in a headless run: the placeholder would hide the column we
        // are about to composite the real artwork into.
        login_screen::Render(p);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::Render();
    }

    const bool ok = im5shot::RenderToRaw(".tmp_harness/login.rgba", 1.0f, IM_COL32(26, 34, 30, 255));

    // Before DestroyContext: ComputeLayout falls back through ImGui::GetFont(),
    // which needs a live context.
    CompositeArtwork(".tmp_harness/login.rgba", 2340.0f, 1080.0f);
    ImGui::DestroyContext();
    return ok ? 0 : 1;
}
