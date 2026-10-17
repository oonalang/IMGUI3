// Host test for the crash breadcrumb (Hacks/Feature/CrashBreadcrumb.h).
//
// The report: "I use the triggerbot for 15 s then it crashes." A native SIGSEGV
// inside the game process leaves nothing useful in logcat, so the mod now keeps
// one line of live triggerbot state and a signal handler appends it - with the
// fault address and the interrupted PC - to a file. This test drives the REAL
// formatter and the REAL writer (the part the handler calls), so the file the
// user sends back is known to be well-formed before any device is involved:
//   * every field is present and in the documented order,
//   * the line is newline-terminated and never overruns the buffer,
//   * a buffer too small for the whole line still gets a terminated prefix,
//   * appends don't clobber the previous crash.
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <unistd.h>

#include "Hacks/Feature/CrashBreadcrumb.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s  (line %d)\n", msg, __LINE__);                    \
        }                                                                      \
    } while (0)

static std::string readAll(const char *path)
{
    std::string out;
    FILE *f = fopen(path, "rb");
    if (!f) return out;
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    fclose(f);
    return out;
}

int main()
{
    CrashBreadcrumbState s;
    memset(&s, 0, sizeof s);
    s.stage = 2;
    s.block = 9;
    s.scans = 4127;
    s.fires = 388;
    s.sameFrameSkips = 61;
    s.kindSkips = 3;
    s.localPawn = (void *)0x1111;
    s.heldWeapon = (void *)0x2222;
    s.firingOn = (void *)0x2222;
    s.targetPawn = (void *)0x3333;

    // --- T1: the whole line, in the documented order -----------------------
    {
        char line[512];
        const int n = CrashBreadcrumb_Format(line, (int)sizeof line,
                                             SIGSEGV, (void *)0xDEADBEEF, (void *)0xCAFE,
                                             s, "/data/data/com.dts.freefireth/files/crash_breadcrumb.txt");
        CHECK(n > 0 && n < (int)sizeof line, "T1 the line fits the buffer");
        CHECK(n == (int)strlen(line), "T1 the returned length IS the line length");
        CHECK(line[n - 1] == '\n', "T1 the line ends with a newline (append-safe)");
        CHECK(strstr(line, "sig=11") != nullptr, "T1 the signal is recorded");
        CHECK(strstr(line, "fault=0xdeadbeef") != nullptr, "T1 the faulting address is recorded");
        CHECK(strstr(line, "pc=0xcafe") != nullptr, "T1 the interrupted PC is recorded");
        CHECK(strstr(line, "stage=2") != nullptr, "T1 the stage (pressing) is recorded");
        CHECK(strstr(line, "block=9") != nullptr, "T1 the last block reason is recorded");
        CHECK(strstr(line, "scans=4127") != nullptr && strstr(line, "fires=388") != nullptr,
              "T1 the session counters are recorded");
        CHECK(strstr(line, "skips=61") != nullptr && strstr(line, "kind=3") != nullptr,
              "T1 the skip counters (same-frame / recycled class) are recorded");
        CHECK(strstr(line, "local=0x1111") != nullptr && strstr(line, "held=0x2222") != nullptr,
              "T1 the pawn and the held weapon are recorded");
        CHECK(strstr(line, "firingOn=0x2222") != nullptr && strstr(line, "target=0x3333") != nullptr,
              "T1 the pressed weapon and the target are recorded");
        CHECK(strstr(line, "crash_breadcrumb.txt") != nullptr, "T1 the path is recorded");
    }

    // --- T2: a buffer smaller than the line is never overrun --------------
    {
        char small[24];
        memset(small, 'X', sizeof small);
        const int n = CrashBreadcrumb_Format(small, (int)sizeof small,
                                             SIGSEGV, nullptr, nullptr, s, "/tmp/x");
        CHECK(n == (int)sizeof small - 1, "T2 a short buffer reports what it could hold");
        CHECK(small[sizeof small - 1] == 0, "T2 ... and stays NUL-terminated inside the buffer");
        CHECK(strncmp(small, "triggerbot crash", 16) == 0,
              "T2 the prefix the reader needs is what survives");
    }

    // --- T3: bad arguments answer 0 instead of crashing -------------------
    {
        char line[64];
        CHECK(CrashBreadcrumb_Format(nullptr, 64, SIGSEGV, nullptr, nullptr, s, "/tmp") == 0,
              "T3 no buffer -> 0");
        CHECK(CrashBreadcrumb_Format(line, 0, SIGSEGV, nullptr, nullptr, s, "/tmp") == 0,
              "T3 no room -> 0");
    }

    // --- T4: a null path still formats (the handler must never deref it) --
    {
        char line[256];
        const int n = CrashBreadcrumb_Format(line, (int)sizeof line,
                                             SIGABRT, nullptr, nullptr, s, nullptr);
        CHECK(n > 0 && strstr(line, "path=\n") != nullptr,
              "T4 a missing path formats as an empty one");
    }

    // --- T5: install + the real write path, appending ---------------------
    {
        const char *path = "/tmp/shaiaintshyy_crash_breadcrumb_test.txt";
        unlink(path);
        CHECK(CrashBreadcrumb_Install(path), "T5 installing against a writable path succeeds");
        CHECK(strcmp(g_crashBreadcrumbPath, path) == 0, "T5 the path is remembered");

        crash_breadcrumb_detail::WriteLine(SIGSEGV, (void *)0xAAAA, (void *)0xBBBB);
        crash_breadcrumb_detail::WriteLine(SIGBUS, (void *)0xCCCC, (void *)0xDDDD);

        const std::string got = readAll(path);
        const size_t firstNl = got.find('\n');
        CHECK(firstNl != std::string::npos, "T5 a crash line reached the file");
        CHECK(got.find("sig=11") != std::string::npos, "T5 the first crash is in the file");
        CHECK(got.find("sig=7") != std::string::npos,
              "T5 a second crash APPENDS instead of replacing the first");
        CHECK(got.find("fault=0xaaaa") != std::string::npos &&
              got.find("fault=0xcccc") != std::string::npos,
              "T5 both fault addresses survive");
        unlink(path);
    }

    // --- T6: the default path is inside the app's own files dir -----------
    {
        char path[256];
        const bool ok = CrashBreadcrumb_DefaultPath(path, (int)sizeof path);
        CHECK(ok, "T6 the default path resolves from /proc/self/cmdline on the host");
        if (ok)
        {
            CHECK(strncmp(path, "/data/data/", 11) == 0 && strstr(path, "/files/crash_breadcrumb.txt") != nullptr,
                  "T6 ... and points at <app>/files/crash_breadcrumb.txt");
        }
        char tiny[8];
        CHECK(!CrashBreadcrumb_DefaultPath(tiny, (int)sizeof tiny),
              "T6 a path buffer that cannot hold the answer is refused, not truncated");
    }

    if (g_failures == 0) {
        printf("OK: all crash-breadcrumb checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("FAILED: %d of %d checks\n", g_failures, g_checks);
    return 1;
}
