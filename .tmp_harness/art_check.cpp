// Does the vendored decoder actually read the shipped login artwork?
#include <cstdio>
#include "IMAGE/miku.h"
#include "IMAGE/bg.h"
#include "ImGui/stb_image.h"

static void try_decode(const char* name, const unsigned char* data, int size)
{
    int w = 0, h = 0, comp = 0;
    unsigned char* px = stbi_load_from_memory(data, size, &w, &h, &comp, 4);
    if (!px) {
        ::printf("%-8s DECODE FAILED (%d bytes): %s\n", name, size, stbi_failure_reason());
        return;
    }
    // A flat/black decode is the other way this fails silently.
    long long sum = 0; int n = 0;
    for (int y = 0; y < h; y += 16)
        for (int x = 0; x < w; x += 16, ++n)
            sum += px[(size_t)(y * w + x) * 4];
    ::printf("%-8s %dx%d comp=%d bytes=%d meanR=%lld\n", name, w, h, comp, size, sum / (n ? n : 1));
    stbi_image_free(px);
}

int main()
{
    ::printf("miku_data sizeof=%zu declared miku_size=%u\n", sizeof(miku_data), miku_size);
    try_decode("miku", miku_data, (int)sizeof(miku_data));
    try_decode("bg", bg_data, (int)sizeof(bg_data));
    return 0;
}
