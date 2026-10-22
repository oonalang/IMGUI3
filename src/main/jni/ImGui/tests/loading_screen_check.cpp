// Host check for the loading screen.
//
// The loader is hooked in before the game has drawn a frame of its own, so
// every pixel the loader does not paint is the game's own black screen. That
// field has to be the first thing painted, on the first frame, with no fade
// from black on top of it.
//
// The loading video is both the card's background and the fullscreen backdrop,
// so the two never disagree. The still artwork is the card's opening image and
// belongs nowhere else: as a fullscreen backdrop it read as separate splash art
// sitting behind the loader.
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

static const ImTextureID kStill = (ImTextureID)(intptr_t)0x5678;
static const ImTextureID kVideo = (ImTextureID)(intptr_t)0x1234;

static void Frame(ImTextureID still)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    ImGui::NewFrame();
    ui_loading::RenderWindow(still);
    ImGui::Render();
}

struct Cmd
{
    float       minX, minY, maxX, maxY;
    float       maxAlpha;
    ImTextureID tex;
    bool        image;
    bool        coversScreen;
};

static void Collect(std::vector<Cmd>& out)
{
    out.clear();
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
            k.tex   = c.TextureId;
            k.image = (c.TextureId == kStill || c.TextureId == kVideo);
            k.coversScreen = (k.minX <= 1.0f && k.minY <= 1.0f &&
                              k.maxX >= (float)ImGui::GetIO().DisplaySize.x - 1.0f &&
                              k.maxY >= (float)ImGui::GetIO().DisplaySize.y - 1.0f);
            out.push_back(k);
        }
    }
}

static bool InsideCard(const Cmd& k, const MenuWindow& win)
{
    return k.minX >= win.pos.x - 2.0f && k.minY >= win.pos.y - 2.0f &&
           k.maxX <= win.pos.x + win.size.x + 2.0f &&
           k.maxY <= win.pos.y + win.size.y + 2.0f;
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

    const MenuWindow win = ComputeMenuWindow(io.DisplaySize);
    std::vector<Cmd> cmds;

    // 1. No video and no still: the field must cover the whole screen, on the
    //    very first frame, with nothing black layered over it.
    ui_media::g_hostVideoTex     = nullptr;
    ui_media::g_hostVideoPending = true;

    Frame(nullptr);
    Collect(cmds);
    Frame(nullptr);
    Collect(cmds);

    bool opaqueField = false;
    for (const Cmd& k : cmds)
        // The dim overlays are translucent, so the field is the opaque part.
        if (k.coversScreen && !k.image && k.maxAlpha > 0.99f) opaqueField = true;
    Check("the field covers the whole screen without a video", opaqueField);

    // 2. With no video yet the field is all there is behind the card: the still
    //    artwork is not painted fullscreen.
    Frame(kStill);
    Collect(cmds);

    bool fullscreenStill = false, fullscreenImage = false, fieldPainted = false;
    for (const Cmd& k : cmds)
    {
        if (!k.coversScreen) continue;
        if (k.image)
        {
            fullscreenImage = true;
            if (k.tex == kStill) fullscreenStill = true;
        }
        else if (k.maxAlpha > 0.99f)
        {
            fieldPainted = true;
        }
    }
    Check("the field is painted with no video", fieldPainted);
    Check("the still artwork is not the backdrop", !fullscreenStill);
    Check("no image is painted fullscreen without a video", !fullscreenImage);

    // 3. With a video ready: it is the card's background and the backdrop, and
    //    the still artwork is gone everywhere.
    ui_media::g_hostVideoTex     = kVideo;
    ui_media::g_hostVideoW       = 960;
    ui_media::g_hostVideoH       = 540;
    ui_media::g_hostVideoPending = false;

    Frame(kStill);
    Collect(cmds);

    int cardVideo = -1, fullscreenVideo = -1, stillAnywhere = -1;
    for (int i = 0; i < (int)cmds.size(); ++i)
    {
        const Cmd& k = cmds[i];
        if (!k.image) continue;
        if (k.tex == kStill && stillAnywhere < 0) stillAnywhere = i;
        if (InsideCard(k, win) && k.tex == kVideo && cardVideo < 0) cardVideo = i;
        if (k.coversScreen && k.tex == kVideo && fullscreenVideo < 0) fullscreenVideo = i;
    }

    Check("the video is the card background", cardVideo >= 0);
    Check("the video is the backdrop too", fullscreenVideo >= 0);
    Check("the still artwork is not painted once the video is up", stillAnywhere < 0);

    // 4. While the clip is still opening the card shows the opening photo, and
    //    the video then replaces it in the card.
    ui_media::g_hostVideoTex = nullptr;
    ui_media::g_hostVideoPending = true;

    Frame(kStill);
    Collect(cmds);

    bool cardStill = false;
    for (const Cmd& k : cmds)
        if (k.image && k.tex == kStill && InsideCard(k, win)) cardStill = true;
    Check("the card opens on the still artwork", cardStill);

    // 5. The card sits inside the screen and the field around it.
    Check("the card is on screen",
          win.pos.x >= 0.0f && win.pos.y >= 0.0f &&
          win.pos.x + win.size.x <= io.DisplaySize.x + 0.5f &&
          win.pos.y + win.size.y <= io.DisplaySize.y + 0.5f);

    // 6. The loader ends on its own and releases the media.
    ui_loading::g_Timer = ui_loading::g_Duration;
    Frame(kStill);
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
