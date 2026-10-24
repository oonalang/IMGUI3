

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

    Check("no video texture before anything starts", ui_media::VideoTexture() == nullptr);
    Check("the video is not ready before anything starts", !ui_media::VideoReady());

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

    Check("starting the video without a JVM fails", !ui_media::StartVideo());
    ui_media::Tick();
    Check("a failed start shows no texture", ui_media::VideoTexture() == nullptr);
    Check("a failed start is not ready", !ui_media::VideoReady());

    Check("starting the sound without a JVM does not crash", !ui_media::StartAudio());

    ui_media::Shutdown();
    ui_media::Shutdown();
    Check("no texture after shutdown", ui_media::VideoTexture() == nullptr);
    Check("not ready after shutdown", !ui_media::VideoReady());

    Check("the worker is not playing after shutdown", ui_media::g_workerState != 2);

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }
    std::printf("loading media: all checks passed\n");
    return 0;
}
