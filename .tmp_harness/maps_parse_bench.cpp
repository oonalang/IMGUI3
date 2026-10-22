// Host benchmark for the /proc/self/maps parser in Tools::IsPtrValid.
//
// A refresh happens on whichever thread first touches an address that is not
// in the snapshot - normally the render thread inside DrawESP - so the parse
// cost is a frame-time cost. This compares the old sscanf loop with the
// hand-written parser now in Tools.cpp, over a synthetic maps file with the
// same shape as a Unity game process.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

static std::string g_file;

static void WriteSyntheticMaps(const char* path, int lines, int readablePercent)
{
    FILE* f = std::fopen(path, "w");
    uintptr_t addr = 0x100000000ull;
    for (int i = 0; i < lines; ++i)
    {
        const uintptr_t size = 0x1000ull * (1 + (i % 64));
        const bool readable = ((i * 100) / lines) < readablePercent;
        std::fprintf(f, "%016llx-%016llx %s 00000000 00:00 %d",
                     (unsigned long long)addr,
                     (unsigned long long)(addr + size),
                     readable ? "r-xp" : "---p",
                     i % 5 == 0 ? 0 : 1234);
        if (i % 3 == 0) std::fprintf(f, "  /system/lib64/libunity.so");
        else if (i % 3 == 1) std::fprintf(f, "  /data/app/com.garena.game.codm/base.apk");
        else std::fprintf(f, "  [anon:libc_malloc]");
        std::fputc('\n', f);
        addr += size + 0x1000ull;
    }
    std::fclose(f);
}

static int ParseOld(std::vector<std::pair<uintptr_t, uintptr_t>>& out)
{
    out.clear();
    FILE* f = std::fopen(g_file.c_str(), "r");
    if (!f) return 0;
    char line[512];
    while (std::fgets(line, sizeof line, f))
    {
        uintptr_t lo = 0, hi = 0;
        char perms[8] = {0};
        if (std::sscanf(line, "%lx-%lx %7s", &lo, &hi, perms) == 3 &&
            perms[0] == 'r' && hi > lo)
            out.emplace_back(lo, hi);
    }
    std::fclose(f);
    return (int)out.size();
}

static int ParseNew(std::vector<std::pair<uintptr_t, uintptr_t>>& out)
{
    out.clear();
    FILE* f = std::fopen(g_file.c_str(), "r");
    if (!f) return 0;
    char line[512];
    while (std::fgets(line, sizeof line, f))
    {
        const unsigned char* p = (const unsigned char*)line;
        auto hex = [](unsigned char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        uintptr_t a = 0, b = 0;
        int digits = 0;
        for (int d; (d = hex(*p)) >= 0; ++p, ++digits) a = (a << 4) | (uintptr_t)d;
        if (digits == 0 || *p != '-') continue;
        ++p;
        digits = 0;
        for (int d; (d = hex(*p)) >= 0; ++p, ++digits) b = (b << 4) | (uintptr_t)d;
        if (digits == 0) continue;
        if (*p == ' ') ++p;
        if (*p == 'r' && b > a) out.emplace_back(a, b);
    }
    std::fclose(f);
    return (int)out.size();
}

template <typename F>
static double BestMs(F fn, int runs)
{
    double best = 1e9;
    for (int i = 0; i < runs; ++i)
    {
        const auto t0 = std::chrono::steady_clock::now();
        fn();
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (ms < best) best = ms;
    }
    return best;
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    const char* path = "/tmp/synthetic_maps.txt";
    g_file = path;
    WriteSyntheticMaps(path, 6000, 70);

    std::vector<std::pair<uintptr_t, uintptr_t>> a, b;
    const int oldCount = ParseOld(a);
    const int newCount = ParseNew(b);

    if (oldCount != newCount)
    {
        std::printf("FAIL: parsers disagree (%d vs %d)\n", oldCount, newCount);
        return 1;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            std::printf("FAIL: range %zu differs\n", i);
            return 1;
        }
    }
    std::printf("PASS: both parsers return the same %d readable ranges\n", oldCount);

    const double oldMs = BestMs([&] { ParseOld(a); }, 20);
    const double newMs = BestMs([&] { ParseNew(b); }, 20);
    std::printf("old sscanf loop : %.3f ms\n", oldMs);
    std::printf("new manual parse: %.3f ms\n", newMs);
    std::printf("speedup         : %.1fx\n", oldMs / newMs);
    return 0;
}
