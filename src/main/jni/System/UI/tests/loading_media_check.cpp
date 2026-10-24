// Host check for the loading media layer.
//
// The loader used to crash the game the moment it appeared, and it painted the
// screen black. Both came from this layer, so the rules it has to keep are
// checked here:
//
//   - a video is only ever shown once a frame has really been drawn into it,
//   - a 4K source is scaled down instead of asking for a 33 MB texture,
//   - starting and stopping are safe even when there is no media at all.

#include <cstdio>

#include "System/UI/LoadingMedia.h"


static int g_failures = 0;

static void Check(const char* what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

int main()
{
    // Nothing has run yet, so there is nothing to show.
    Check("no video texture before anything starts", ui_media::VideoTexture() == nullptr);
    Check("the video is not ready before anything starts", !ui_media::VideoReady());

    // A 4K source must not turn into a 4K texture.
    int w = 3840, h = 2160;
    ui_media::detail::ClampTargetSize(&w, &h);
    Check("4K is scaled down to 1920x1080", w == 1920 && h == 1080);

    w = 1280; h = 720;
    ui_media::detail::ClampTargetSize(&w, &h);
    Check("720p is left alone", w == 1280 && h == 720);

    w = 3840; h = 1000;
    ui_media::detail::ClampTargetSize(&w, &h);
    Check("a wide source keeps its shape", w == 1920 && h == 500);

    w = 1000; h = 4000;
    ui_media::detail::ClampTargetSize(&w, &h);
    Check("a tall source keeps its shape", h == 1080 && w == 270);

    w = 0; h = 0;
    ui_media::detail::ClampTargetSize(&w, &h);
    Check("a broken size stays broken", w == 0 && h == 0);

    // Without a JVM there is no media, and asking for it must not take the
    // process down.
    Check("starting the video without a JVM fails", !ui_media::StartVideo());
    ui_media::Tick();
    Check("a failed start shows no texture", ui_media::VideoTexture() == nullptr);
    Check("a failed start is not ready", !ui_media::VideoReady());

    // The same for the sound.
    Check("starting the sound without a JVM does not crash", !ui_media::StartAudio());

    // Shutting down twice must be safe, and must not resurrect anything.
    ui_media::Shutdown();
    ui_media::Shutdown();
    Check("no texture after shutdown", ui_media::VideoTexture() == nullptr);
    Check("not ready after shutdown", !ui_media::VideoReady());

    // The worker must never be left in a state that says "playing".
    Check("the worker is not playing after shutdown", ui_media::g_workerState != 2);

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }
    std::printf("loading media: all checks passed\n");
    return 0;
}
