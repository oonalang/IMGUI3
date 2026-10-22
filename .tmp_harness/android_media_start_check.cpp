// Runs the Android branch of the loading media layer against fake JNI + GL.
// The point of the test: with only a file on disk (no embedded asset header),
// the video and audio players must actually be created and started.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include "System/UI/LoadingMedia.h"

JavaVM* jvm = nullptr;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

static void WriteFile(const std::string& path, const char* data, size_t size)
{
    FILE* f = fopen(path.c_str(), "wb");
    if (f == nullptr) return;
    fwrite(data, 1, size, f);
    fclose(f);
}

static int CallCount(const char* name)
{
    auto it = fakejni::st().voidCalls.find(name);
    return it == fakejni::st().voidCalls.end() ? 0 : it->second;
}

static void ResetMediaState()
{
    ui_media::Shutdown();
    ui_media::g_inited = false;
    ui_media::g_videoLive = false;
    ui_media::g_audioLive = false;
    ui_media::g_videoW = 0;
    ui_media::g_videoH = 0;
    ui_media::g_targetTex = 0;
    ui_media::g_fbo = 0;
    ui_media::g_oesTexture = 0;
    ui_media::g_videoTex = nullptr;
    fakejni::reset();
}

int main()
{
    static JavaVM vm;
    jvm = &vm;

    char tmpl[] = "/tmp/ethnir_start_XXXXXX";
    const char* made = mkdtemp(tmpl);
    if (made == nullptr) { printf("FAIL: no temp dir\n"); return 1; }
    const std::string dir = made;
    setenv("TMPDIR", dir.c_str(), 1);

    // --- nothing on disk: no player, loader keeps its procedural backdrop ----
    ResetMediaState();
    ui_media::Tick();
    CHECK(!ui_media::VideoReady(), "no media on disk -> no video texture");
    CHECK(fakejni::st().newObjects == 0,
          "no media on disk -> MediaPlayer is never even constructed");

    // --- video only ---------------------------------------------------------
    WriteFile(dir + "/loading.mp4", "not-really-an-mp4", 17);
    ResetMediaState();
    ui_media::Tick();

    CHECK(ui_media::VideoReady(), "a pushed loading.mp4 starts without any embedded asset header");
    CHECK(fakejni::st().lastString == dir + "/loading.mp4",
          "the player was pointed at the file that was found on disk");
    CHECK(CallCount("setDataSource") == 1, "setDataSource was called once for the video");
    CHECK(CallCount("setSurface") == 1, "the SurfaceTexture-backed Surface was attached");
    CHECK(CallCount("setLooping") == 1, "the video loops");
    CHECK(CallCount("prepare") == 1 && CallCount("start") == 1,
          "prepare + start both happened");
    CHECK(ui_media::g_videoW == 1280 && ui_media::g_videoH == 720,
          "the decoder's reported size was picked up");
    CHECK(ui_media::VideoAspect() > 1.77f && ui_media::VideoAspect() < 1.78f,
          "aspect comes from that size (16:9)");
    CHECK(ui_media::VideoTexture() != nullptr, "the RGBA blit target is exposed as an ImGui texture");
    CHECK(fakegl::st().draws >= 1, "one OES blit was issued for the frame");

    // --- video + audio ------------------------------------------------------
    WriteFile(dir + "/loading.mp3", "not-really-an-mp3", 17);
    ResetMediaState();
    ui_media::Tick();

    CHECK(ui_media::VideoReady() && ui_media::g_audioLive,
          "with both files present the video and the audio both start");
    CHECK(fakejni::st().newObjects == 4,
          "one SurfaceTexture, one Surface and two MediaPlayers were built");
    CHECK(CallCount("setLooping") == 2,
          "both players had looping set (video on, audio off - same entry point)");
    CHECK(fakejni::st().strings.size() == 2 &&
          fakejni::st().strings[0] == dir + "/loading.mp4" &&
          fakejni::st().strings[1] == dir + "/loading.mp3",
          "both players were given their own file");

    // --- the audio is released when the loader finishes ---------------------
    ui_media::StopAudio();
    CHECK(!ui_media::g_audioLive && CallCount("release") >= 1,
          "StopAudio releases the audio player");
    CHECK(ui_media::VideoReady(), "... and leaves the video alone");

    ResetMediaState();
    CHECK(!ui_media::VideoReady(), "Shutdown drops the video state");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all media start checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
