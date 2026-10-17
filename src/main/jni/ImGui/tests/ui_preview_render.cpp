// Host preview renderer for the menu: draws the real custom:: widgets with the
// real shipping fonts, rasterizes ImGui's draw data in software and writes a
// raw RGBA buffer, so the look can be checked on a machine with no device.
//
// This is a dev tool, not a test: it proves nothing about Android, only what
// the same code paths put on screen.
//
//   g++ -std=c++17 -O2 -I. -IImGui -o /tmp/ui_preview ImGui/tests/ui_preview_render.cpp \
//       ImGui/custom_widgets.cpp ImGui/imgui.cpp ImGui/imgui_draw.cpp \
//       ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp
//   /tmp/ui_preview /tmp/ui_preview.rgba
//
// Then wrap the buffer into a PNG (width/height are printed on stdout).
#include "imgui.h"
#include "imgui_internal.h"
#include "custom_widgets.hpp"
#include "imgui_settings.h"
#include "nl_style.h"
#include "Fonts/nl_fonts.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = { 0.42f, 0.55f, 1.0f, 1.0f };

namespace font
{
    ImFont* inter_semibold = nullptr;
    ImFont* icomoon_page = nullptr;
    ImFont* icomoon_logo = nullptr;
}

struct PreviewBuffer
{
    int w = 0, h = 0;
    std::vector<float> rgb;   // linear-ish 0..1, three per pixel
};

static void FillRect(PreviewBuffer& img, int x0, int y0, int x1, int y1, const ImVec4& c)
{
    x0 = ImMax(x0, 0); y0 = ImMax(y0, 0);
    x1 = ImMin(x1, img.w); y1 = ImMin(y1, img.h);
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
            float* p = &img.rgb[(size_t)(y * img.w + x) * 3];
            p[0] = c.x; p[1] = c.y; p[2] = c.z;
        }
}

static void Triangle(PreviewBuffer& img, const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c,
                     const ImRect& clip, const unsigned char* tex, int tw, int th)
{
    const float minx = ImMax(ImMin(a.pos.x, ImMin(b.pos.x, c.pos.x)), clip.Min.x);
    const float maxx = ImMin(ImMax(a.pos.x, ImMax(b.pos.x, c.pos.x)), clip.Max.x);
    const float miny = ImMax(ImMin(a.pos.y, ImMin(b.pos.y, c.pos.y)), clip.Min.y);
    const float maxy = ImMin(ImMax(a.pos.y, ImMax(b.pos.y, c.pos.y)), clip.Max.y);
    if (maxx <= minx || maxy <= miny)
        return;

    const float area = (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y) - (b.pos.y - a.pos.y) * (c.pos.x - a.pos.x);
    if (std::fabs(area) < 1e-6f)
        return;

    const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a.col);
    const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b.col);
    const ImVec4 cc = ImGui::ColorConvertU32ToFloat4(c.col);

    const int x0 = (int)std::floor(minx), x1 = (int)std::ceil(maxx);
    const int y0 = (int)std::floor(miny), y1 = (int)std::ceil(maxy);

    for (int y = y0; y < y1; ++y)
    {
        const float py = (float)y + 0.5f;
        if (py < 0.0f || py >= (float)img.h)
            continue;
        for (int x = x0; x < x1; ++x)
        {
            const float px = (float)x + 0.5f;
            if (px < 0.0f || px >= (float)img.w)
                continue;

            const float w0 = ((b.pos.x - px) * (c.pos.y - py) - (b.pos.y - py) * (c.pos.x - px)) / area;
            const float w1 = ((c.pos.x - px) * (a.pos.y - py) - (c.pos.y - py) * (a.pos.x - px)) / area;
            const float w2 = 1.0f - w0 - w1;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            const float u = a.uv.x * w0 + b.uv.x * w1 + c.uv.x * w2;
            const float v = a.uv.y * w0 + b.uv.y * w1 + c.uv.y * w2;
            float alpha = 1.0f;
            if (tex != nullptr)
            {
                // Bilinear on the font atlas: it is white RGB with glyph coverage
                // in alpha, which is exactly what ImGui samples in a backend.
                const float fx = ImClamp(u * (float)tw - 0.5f, 0.0f, (float)tw - 1.0f);
                const float fy = ImClamp(v * (float)th - 0.5f, 0.0f, (float)th - 1.0f);
                const int ix = (int)fx, iy = (int)fy;
                const float tx = fx - (float)ix, ty = fy - (float)iy;
                const int ix1 = ImMin(ix + 1, tw - 1), iy1 = ImMin(iy + 1, th - 1);
                auto at = [&](int sx, int sy) { return (float)tex[((size_t)sy * tw + sx) * 4 + 3] / 255.0f; };
                alpha = (at(ix, iy) * (1 - tx) + at(ix1, iy) * tx) * (1 - ty) +
                        (at(ix, iy1) * (1 - tx) + at(ix1, iy1) * tx) * ty;
            }

            const float r = ca.x * w0 + cb.x * w1 + cc.x * w2;
            const float g = ca.y * w0 + cb.y * w1 + cc.y * w2;
            const float bl = ca.z * w0 + cb.z * w1 + cc.z * w2;
            const float a = (ca.w * w0 + cb.w * w1 + cc.w * w2) * alpha;
            if (a <= 0.001f)
                continue;

            float* p = &img.rgb[(size_t)(y * img.w + x) * 3];
            p[0] = r * a + p[0] * (1.0f - a);
            p[1] = g * a + p[1] * (1.0f - a);
            p[2] = bl * a + p[2] * (1.0f - a);
        }
    }
}

static void RenderTo(PreviewBuffer& img)
{
    ImDrawData* draw = ImGui::GetDrawData();
    const ImVec2 scale = ImVec2((float)img.w / draw->DisplaySize.x, (float)img.h / draw->DisplaySize.y);

    static unsigned char* pixels = nullptr;
    int tex_w = 0, tex_h = 0;
    if (pixels == nullptr)
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &tex_w, &tex_h);

    for (int n = 0; n < draw->CmdListsCount; ++n)
    {
        const ImDrawList* list = draw->CmdLists[n];
        for (const ImDrawCmd& cmd : list->CmdBuffer)
        {
            if (cmd.UserCallback != nullptr)
                continue;
            const ImRect clip(ImVec2(cmd.ClipRect.x * scale.x, cmd.ClipRect.y * scale.y),
                              ImVec2(cmd.ClipRect.z * scale.x, cmd.ClipRect.w * scale.y));
            for (unsigned int i = 0; i + 2 < cmd.ElemCount; i += 3)
            {
                const ImDrawIdx* idx = list->IdxBuffer.Data + cmd.IdxOffset + i;
                const ImDrawVert* vtx = list->VtxBuffer.Data + cmd.VtxOffset;
                const ImDrawVert& a = vtx[idx[0]];
                const ImDrawVert& b = vtx[idx[1]];
                const ImDrawVert& c = vtx[idx[2]];
                Triangle(img, a, b, c, clip, pixels, tex_w, tex_h);
            }
        }
    }
}

// ---- the menu, drawn with the same widgets the app uses --------------------
static void BuildScene(ImVec2 display)
{
    ImFont* body = ImGui::GetFont();
    ImFont* bold = ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : body;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(display);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("preview", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 body_min(0.0f, 0.0f);
    const ImVec2 body_max = display;

    dl->AddRectFilled(body_min, body_max, ImGui::GetColorU32(nl::Shell()), 14.0f);
    dl->AddRect(body_min, body_max, ImGui::GetColorU32(nl::CardBorder()), 14.0f, 0, 1.0f);

    const float outerPad = 14.0f;
    const float headerH = 60.0f;
    const ImVec2 headerMin(outerPad, outerPad);
    const ImVec2 headerMax(display.x - outerPad, outerPad + headerH);

    dl->AddRectFilled(headerMin, ImVec2(headerMin.x + 40.0f, headerMin.y + 34.0f), ImGui::GetColorU32(c::accent, 0.16f), 8.0f);
    dl->AddText(body, 18.0f, ImVec2(headerMin.x + 10.0f, headerMin.y + 8.0f), ImGui::GetColorU32(c::accent), "NL");
    dl->AddText(bold, 22.0f, ImVec2(headerMin.x + 52.0f, headerMin.y + 2.0f), ImGui::GetColorU32(nl::LabelBright()), "ETHNIR NOIR");
    dl->AddText(bold, 22.0f, ImVec2(headerMin.x + 52.0f + 148.0f, headerMin.y + 2.0f), ImGui::GetColorU32(c::accent), " V3");
    dl->AddText(body, 10.0f, ImVec2(headerMin.x + 53.0f, headerMin.y + 30.0f), ImGui::GetColorU32(nl::CardCaption()), "PLAY LIKE A PLAYER  |  NOT A CHEATER");

    // hue bar
    const ImVec2 trackMin(headerMax.x - 250.0f, headerMin.y + 20.0f);
    const ImVec2 trackMax(headerMax.x - 12.0f, trackMin.y + 3.0f);
    for (int i = 0; i < 8; ++i)
    {
        float r0, g0, b0, r1, g1, b1;
        ImGui::ColorConvertHSVtoRGB(i / 8.0f, 0.9f, 1.0f, r0, g0, b0);
        ImGui::ColorConvertHSVtoRGB((i + 1) / 8.0f, 0.9f, 1.0f, r1, g1, b1);
        const float x0 = trackMin.x + (trackMax.x - trackMin.x) * (i / 8.0f);
        const float x1 = trackMin.x + (trackMax.x - trackMin.x) * ((i + 1) / 8.0f);
        dl->AddRectFilledMultiColor(ImVec2(x0, trackMin.y), ImVec2(x1, trackMax.y),
            ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1)), ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1)),
            ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1)), ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1)));
    }
    dl->AddCircleFilled(ImVec2(trackMin.x + (trackMax.x - trackMin.x) * 0.42f, trackMin.y + 1.5f), 5.5f, IM_COL32(247, 248, 252, 255), 20);

    dl->AddLine(ImVec2(headerMin.x, headerMax.y), ImVec2(headerMax.x, headerMax.y), ImGui::GetColorU32(nl::RowDivider()));

    // tab strip
    const float pad = 10.0f;
    const ImVec2 hostMin(outerPad, headerMax.y + 10.0f);
    const ImVec2 hostMax(display.x - outerPad, display.y - outerPad);
    dl->AddRectFilled(hostMin, hostMax, ImGui::GetColorU32(nl::Rgb(13, 15, 22, 225)), 14.0f);
    dl->AddRect(hostMin, hostMax, ImGui::GetColorU32(nl::CardBorder()), 14.0f, 0, 1.0f);

    const ImVec2 barMin(hostMin.x + pad, hostMin.y + pad);
    const ImVec2 barMax(hostMax.x - pad, barMin.y + 36.0f);
    dl->AddRectFilled(barMin, barMax, IM_COL32(11, 13, 20, 225), 9.0f);
    dl->AddRect(barMin, barMax, ImGui::GetColorU32(nl::CardBorder()), 9.0f, 0, 1.0f);

    static const char* tabs[] = { "HOME", "ESP", "AIM", "SKINS", "MISC", "SETTINGS" };
    const int tabCount = 6;
    const float gap = 5.0f;
    const float tabW = ((barMax.x - barMin.x) - gap * (tabCount + 1)) / (float)tabCount;
    for (int i = 0; i < tabCount; ++i)
    {
        const ImVec2 pMin(barMin.x + gap + (tabW + gap) * i, barMin.y + 4.0f);
        const ImVec2 pMax(pMin.x + tabW, barMax.y - 4.0f);
        const bool act = (i == 3);
        if (act || i == 1)
            dl->AddRectFilled(pMin, pMax, ImGui::GetColorU32(act ? nl::NavHover() : nl::Rgb(26, 29, 39, 190)), 6.0f);
        const ImVec2 ts = ImGui::CalcTextSize(tabs[i]);
        dl->AddText(ImVec2(pMin.x + (tabW - ts.x) * 0.5f, pMin.y + (pMax.y - pMin.y - ts.y) * 0.5f),
                    act ? ImGui::GetColorU32(nl::LabelBright()) : ImGui::GetColorU32(nl::LabelMuted()), tabs[i]);
    }

    // two cards
    const ImVec2 contentMin(barMin.x, barMax.y + 8.0f);
    const ImVec2 contentMax(barMax.x, hostMax.y - pad);
    const float colGap = 10.0f;
    const float colW = ((contentMax.x - contentMin.x) - colGap) * 0.5f;

    ImGui::SetCursorScreenPos(contentMin);
    custom::Child("WEAPON##preview", ImVec2(colW, contentMax.y - contentMin.y), true);
    {
        static bool enabled = true, auto_fire = false;
        custom::Checkbox("Unlock Blueprints", &enabled);
        custom::Checkbox("Apply Skin On Equip", &auto_fire);
        static float fov = 72.0f;
        custom::SliderFloat("Field of View", &fov, 0.0f, 180.0f, "%.0f");
        if (custom::BeginCombo("Aim Mode", "Latency", 4))
        {
            custom::Selectable("Off", false);
            custom::Selectable("Latency", true);
            custom::Selectable("Performance", false);
            custom::EndCombo();
        }
        custom::Button("APPLY CONFIG", ImVec2(colW - 24.0f, 30.0f));
    }
    custom::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(contentMin.x + colW + colGap, contentMin.y));
    custom::Child("COMBAT##preview", ImVec2(colW, contentMax.y - contentMin.y), true);
    {
        static bool esp = true, boxes = true;
        custom::Checkbox("ESP Enabled", &esp);
        custom::Checkbox("Boxes", &boxes);
        static float dist = 120.0f;
        custom::SliderFloat("Render Distance", &dist, 0.0f, 400.0f, "%.0fm");
        custom::Button("RESET", ImVec2((colW - 34.0f) * 0.5f, 28.0f));
        ImGui::SameLine();
        custom::Button("SAVE", ImVec2((colW - 34.0f) * 0.5f, 28.0f));
    }
    custom::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
}

int main(int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : "/tmp/ui_preview.rgba";
    const int W = 1000, H = 560;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // The fonts the app actually ships.
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* body = io.Fonts->AddFontFromMemoryTTF((void*)nl_sst_medium, (int)nl_sst_medium_size, 15.0f, &cfg);
    ImFont* bold = io.Fonts->AddFontFromMemoryTTF((void*)nl_sst_bold, (int)nl_sst_bold_size, 21.0f, &cfg);
    (void)bold;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    io.FontDefault = body;
    io.Fonts->TexID = (ImTextureID)1;
    F50 = bold;

    c::scale = 1.5f;
    c::widget_scale = 1.0f;
    // The production chrome: without this the preview would keep ImGui's
    // default style (bright scrollbars, gray borders) and would not show what
    // the menu actually looks like on the phone.
    c::ApplyMainWindowStyle(ImGui::GetStyle());

    PreviewBuffer img;
    img.w = W; img.h = H;
    img.rgb.assign((size_t)W * H * 3, 0.0f);

    // A muted backdrop so the panel's translucency reads like it does in game.
    FillRect(img, 0, 0, W, H, ImVec4(0.045f, 0.05f, 0.07f, 1.0f));

    io.DisplaySize = ImVec2((float)W, (float)H);
    io.DeltaTime = 1.0f / 60.0f;
    for (int frame = 0; frame < 24; ++frame)
    {
        ImGui::NewFrame();
        BuildScene(io.DisplaySize);
        ImGui::Render();
        if (frame == 23)
            RenderTo(img);
    }

    // Composite the draw data over the backdrop: RenderTo already blends.
    std::vector<unsigned char> bytes((size_t)W * H * 3);
    for (size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = (unsigned char)ImClamp((int)(img.rgb[i] * 255.0f + 0.5f), 0, 255);

    FILE* f = std::fopen(out, "wb");
    if (f == nullptr)
    {
        std::printf("cannot open %s\n", out);
        return 1;
    }
    std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);

    std::printf("wrote %s (%dx%d RGB)\n", out, W, H);
    ImGui::DestroyContext();
    return 0;
}
