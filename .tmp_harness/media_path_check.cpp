// Host check for the loading media search path. The path helpers are platform
// neutral, so the resolution order and the embedded-asset fallback can be
// exercised without a device or a JNI environment.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include "System/UI/LoadingMedia.h"

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

int main()
{
    namespace d = ui_media::detail;

    char tmpl[] = "/tmp/ethnir_media_XXXXXX";
    const char* made = mkdtemp(tmpl);
    if (made == nullptr) { printf("FAIL: could not make a temp dir\n"); return 1; }
    const std::string dir = made;
    setenv("TMPDIR", dir.c_str(), 1);

    const char* names[2] = { "loading.mp4", "velina-aegard.3840x2160.mp4" };

    CHECK(d::MediaRoot() == "/storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/",
          "the documented drop folder is still the first place searched");
    const std::string root = d::MediaRoot();
    CHECK(!root.empty() && root[0] == '/' && root[root.size() - 1] == '/',
          "the drop folder is an absolute path ending in a separator");

    CHECK(d::ResolveMedia(names, 2, nullptr, 0).empty(),
          "no file anywhere -> no path, so the loader keeps its procedural backdrop");

    WriteFile(dir + "/loading.mp4", "video-bytes", 11);
    CHECK(d::ResolveMedia(names, 2, nullptr, 0) == dir + "/loading.mp4",
          "a pushed loading.mp4 is found and returned");

    unlink((dir + "/loading.mp4").c_str());
    WriteFile(dir + "/velina-aegard.3840x2160.mp4", "video-bytes", 11);
    CHECK(d::ResolveMedia(names, 2, nullptr, 0) == dir + "/velina-aegard.3840x2160.mp4",
          "the original repo file name works when it was not renamed");

    unlink((dir + "/velina-aegard.3840x2160.mp4").c_str());

    static const unsigned char blob[4] = { 1, 2, 3, 4 };
    const std::string written = d::ResolveMedia(names, 2, blob, sizeof(blob));
    CHECK(written == dir + "/loading.mp4",
          "with nothing on disk the embedded asset is written to the cache dir");
    long size = 0;
    CHECK(d::FileSize(written, &size) && size == 4,
          "... and it is the embedded bytes, not an empty file");

    WriteFile(dir + "/loading.mp4", "a-much-longer-body", 17);
    const std::string again = d::ResolveMedia(names, 2, blob, sizeof(blob));
    CHECK(again == dir + "/loading.mp4" && d::FileSize(again, &size) && size == 17,
          "an existing pushed file wins over the embedded copy (it is not overwritten)");

    CHECK(d::ResolveMedia(names, 2, nullptr, 0) == dir + "/loading.mp4",
          "and resolution stays stable across calls");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all media path checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
