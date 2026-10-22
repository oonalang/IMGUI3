// Host check for the loading screen.
//
// The loader is hooked in before the game has drawn a frame of its own, so
// every pixel the loader does not paint is the game's own black screen. The
// video used to be drawn *instead of* the field, and the video texture is empty
// until the first frame has been rendered into it - that is what painted the
// screen black. The field has to stay underneath the video.
//
// The check renders the loader headless and looks at the draw commands.

#include <cstdio>
#include <cstdint>
#include <vector>

#include "ImGui/imgui.h"
#include "ImGui/menu_layout.h"
#include "System/UI/LoadingAnimation.h"

namespace font { ImFont* inter_semibold = nullptr; }
ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
float menu[4] = {0, 0, 0, 0};

static int g_failures = 0;

static void Check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

static void Frame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();
    ui_loading::RenderWindow(nullptr);
    ImGui::Render();
}

struct Cmd
{
    float minX, minY, maxX, maxY;
    float maxAlpha;     // the most opaque vertex in the command
    bool  isImage;      // drawn with the fake video texture
    bool  coversScreen;
};

static void Collect(std::vector<Cmd>& out)
{
    const ImDrawData* dd = ImGui::GetDrawData();
    if (dd == nullptr) return;

    for (int n = 0; n < dd->CmdListsCount; ++n)
    {
        const ImDrawList* cl = dd->CmdLists[n];
        for (int i = 0; i < cl->CmdBuffer.Size; ++i)
        {
            const ImDrawCmd& c = cl->CmdBuffer[i];
            if (c.UserCallback != nullptr || c.ElemCount == 0) continue;

            const ImDrawVert* vtx = cl->VtxBuffer.Data + c.VtxOffset;
            const ImDrawIdx*  idx = cl->IdxBuffer.Data + c.IdxOffset;

            Cmd k{};
            k.minX = 1e9f; k.minY = 1e9f; k.maxX = -1e9f; k.maxY = -1e9f;
            k.maxAlpha = 0.0f;
            for (unsigned int e = 0; e < c.ElemCount; ++e)
            {
                const ImDrawVert& v = vtx[idx[e]];
                k.minX = ImMin(k.minX, v.pos.x); k.maxX = ImMax(k.maxX, v.pos.x);
                k.minY = ImMin(k.minY, v.pos.y); k.maxY = ImMax(k.maxY, v.pos.y);
                k.maxAlpha = ImMax(k.maxAlpha,
                                   (float)((v.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f);
            }
            // The font atlas sits on texture 0, so an unset fake texture would
            // make every command look like an image.
            k.isImage = (ui_media::g_hostVideoTex != nullptr &&
                        c.TextureId == ui_media::g_hostVideoTex);
            k.coversScreen = (k.minX <= 1.0f && k.minY <= 1.0f &&
                              k.maxX >= (float)ImGui::GetIO().DisplaySize.x - 1.0f &&
                              k.maxY >= (float)ImGui::GetIO().DisplaySize.y - 1.0f);
            out.push_back(k);
        }
    }
}

int main()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(1080.0f, 2400.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.BackendPlatformName = "headless";
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ui_loading::Start(12.0f);
    Check("the loader is active", ui_loading::IsActive());

    // 1. No video: the field must cover the whole screen.
    ui_media::g_hostVideoTex = nullptr;
    Frame();
    Frame();

    std::vector<Cmd> cmds;
    Collect(cmds);

    bool fieldWithoutVideo = false;
    for (const Cmd& k : cmds)
        // The dim overlays are translucent, so the field is the opaque part.
        if (k.coversScreen && !k.isImage && k.maxAlpha > 0.99f) fieldWithoutVideo = true;
    Check("the field covers the whole screen without a video", fieldWithoutVideo);

    // 2. With a video: the field must still be under it.
    const ImTextureID fakeVideo = (ImTextureID)(intptr_t)0x1234;
    ui_media::g_hostVideoTex = fakeVideo;
    ui_media::g_hostVideoW   = 3840;
    ui_media::g_hostVideoH   = 2160;

    cmds.clear();
    Frame();
    Frame();
    Collect(cmds);

    int fieldIndex = -1, videoIndex = -1;
    for (int i = 0; i < (int)cmds.size(); ++i)
    {
        const Cmd& k = cmds[i];
        if (!k.coversScreen) continue;
        if (k.isImage) { if (videoIndex < 0) videoIndex = i; }
        else if (k.maxAlpha > 0.99f) { if (fieldIndex < 0) fieldIndex = i; }
    }

    Check("the field is still painted under the video", fieldIndex >= 0);
    Check("the video is drawn on top of it", videoIndex >= 0);
    Check("the video comes after the field", fieldIndex >= 0 && videoIndex > fieldIndex);

    // 3. The card sits inside the screen and the field around it.
    const MenuWindow win = ComputeMenuWindow(io.DisplaySize);
    Check("the card is on screen",
          win.pos.x >= 0.0f && win.pos.y >= 0.0f &&
          win.pos.x + win.size.x <= io.DisplaySize.x + 0.5f &&
          win.pos.y + win.size.y <= io.DisplaySize.y + 0.5f);

    // 4. The loader ends on its own and releases the media.
    ui_loading::g_Timer = ui_loading::g_Duration;
    Frame();
    Check("the loader finishes", !ui_loading::IsActive());

    ImGui::DestroyContext();

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }
    std::printf("loading screen: all checks passed\n");
    return 0;
}
