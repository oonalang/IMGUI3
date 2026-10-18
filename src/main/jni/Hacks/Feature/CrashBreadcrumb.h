#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <ucontext.h>

// ---------------------------------------------------------------------------
// Crash breadcrumb - "the triggerbot dies after ~15 s of use" needs a name.
//
// A tombstone would say where; logcat usually says nothing for a native SIGSEGV
// inside the game process. So the mod keeps ONE line of live triggerbot state
// (stage, last block reason, the three objects it holds, its counters) and a
// signal handler appends it, plus the fault address and the interrupted PC, to
// a file the user can pull. That turns the next report from "it crashes" into
// "stage=2 firingOn=0x... fault=0x..." - which is the difference between
// guessing and fixing.
//
// Rules this file keeps, because it runs inside a crashing process:
//   * no allocation, no stdio FILE, no mutex in the signal path - open/write/close
//     are async-signal-safe, snprintf over a stack buffer does not allocate;
//   * an alternate signal stack, so a stack overflow is still reportable;
//   * the game's own handler (Unity installs one) is chained to, never swallowed.
//
// The formatter is a pure function so a host test can drive it:
// Hacks/Feature/tests/crash_breadcrumb_check.cpp.
// ---------------------------------------------------------------------------
struct CrashBreadcrumbState
{
    volatile int  stage;        // 0 idle, 1 scanning, 2 pressing, 3 releasing
    volatile int  block;        // last FireBlock (int - the signal path must not need the enum)
    volatile int  scans;
    volatile int  fires;
    volatile int  sameFrameSkips;
    volatile int  kindSkips;
    volatile void *localPawn;
    volatile void *heldWeapon;
    volatile void *firingOn;
    volatile void *targetPawn;
};

inline CrashBreadcrumbState g_crashBreadcrumb{};
inline char g_crashBreadcrumbPath[256] = {0};

inline void CrashBreadcrumb_NoteStage(int stage)
{
    g_crashBreadcrumb.stage = stage;
}

inline void CrashBreadcrumb_NoteScan(int block, int scans, int fires,
                                     int sameFrameSkips, int kindSkips)
{
    g_crashBreadcrumb.stage = 1;
    g_crashBreadcrumb.block = block;
    g_crashBreadcrumb.scans = scans;
    g_crashBreadcrumb.fires = fires;
    g_crashBreadcrumb.sameFrameSkips = sameFrameSkips;
    g_crashBreadcrumb.kindSkips = kindSkips;
}

inline void CrashBreadcrumb_NoteObjects(void *localPawn, void *heldWeapon,
                                        void *firingOn, void *targetPawn)
{
    g_crashBreadcrumb.localPawn  = localPawn;
    g_crashBreadcrumb.heldWeapon = heldWeapon;
    g_crashBreadcrumb.firingOn   = firingOn;
    g_crashBreadcrumb.targetPawn = targetPawn;
}

// One line, always newline-terminated, never longer than `cap`. Returns the
// number of bytes written (0 when the buffer cannot hold even the header).
inline int CrashBreadcrumb_Format(char *out, int cap,
                                  int sig, const void *faultAddr, const void *pc,
                                  const CrashBreadcrumbState &s,
                                  const char *path)
{
    if (out == nullptr || cap <= 0)
        return 0;

    const int n = snprintf(
        out, (size_t)cap,
        "triggerbot crash sig=%d fault=%p pc=%p stage=%d block=%d scans=%d fires=%d"
        " skips=%d kind=%d local=%p held=%p firingOn=%p target=%p path=%s\n",
        sig, faultAddr, pc,
        (int)s.stage, (int)s.block, (int)s.scans, (int)s.fires,
        (int)s.sameFrameSkips, (int)s.kindSkips,
        (const void *)s.localPawn, (const void *)s.heldWeapon,
        (const void *)s.firingOn, (const void *)s.targetPawn,
        path != nullptr ? path : "");

    if (n < 0)
    {
        out[0] = 0;
        return 0;
    }
    return n >= cap ? cap - 1 : n;
}

inline bool CrashBreadcrumb_DefaultPath(char *out, int cap)
{
    if (out == nullptr || cap <= 0)
        return false;

    char cmd[128];
    std::memset(cmd, 0, sizeof cmd);
    const int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd < 0)
        return false;
    const ssize_t got = read(fd, cmd, sizeof cmd - 1);
    close(fd);
    if (got <= 0)
        return false;
    cmd[got] = 0;

    // "com.dts.freefireth:remote" is still the same app's files dir.
    for (char *p = cmd; *p != 0; ++p)
        if (*p == ':') { *p = 0; break; }

    const int n = snprintf(out, (size_t)cap, "/data/data/%s/files/crash_breadcrumb.txt", cmd);
    return n > 0 && n < cap;
}

// ---------------------------------------------------------------------------
// The signal side. Kept in its own inline functions so the host test can link
// the formatter without ever installing anything.
// ---------------------------------------------------------------------------
namespace crash_breadcrumb_detail
{
    inline const int kSignals[5] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
    inline struct sigaction g_previous[5];
    inline bool g_havePrevious[5] = { false, false, false, false, false };
    inline char g_altStack[64 * 1024];

    inline int SlotForSignal(int sig)
    {
        for (int i = 0; i < 5; ++i)
            if (kSignals[i] == sig) return i;
        return -1;
    }

    inline void WriteLine(int sig, void *faultAddr, void *pc)
    {
        if (g_crashBreadcrumbPath[0] == 0)
            return;

        char line[512];
        const int n = CrashBreadcrumb_Format(line, (int)sizeof line, sig, faultAddr, pc,
                                             g_crashBreadcrumb, g_crashBreadcrumbPath);
        if (n <= 0)
            return;

        const int fd = open(g_crashBreadcrumbPath, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (fd < 0)
            return;
        const ssize_t ignored = write(fd, line, (size_t)n);
        (void)ignored;
        close(fd);
    }

    inline void Handler(int sig, siginfo_t *info, void *context)
    {
        void *faultAddr = (info != nullptr) ? info->si_addr : nullptr;
        void *pc = nullptr;
#if defined(__aarch64__) && !defined(HOST_SYNTAX_ONLY)
        if (context != nullptr)
            pc = (void *)((ucontext_t *)context)->uc_mcontext.pc;
#endif
        WriteLine(sig, faultAddr, pc);

        // Hand the signal back: Unity's own crash handler (when the game
        // installed one) still runs, and with nothing installed the process
        // dies exactly as it would have without us.
        const int slot = SlotForSignal(sig);
        if (slot >= 0 && g_havePrevious[slot])
        {
            const struct sigaction previous = g_previous[slot];
            sigaction(sig, &previous, nullptr);
            const bool chained = previous.sa_handler != SIG_DFL &&
                                 previous.sa_handler != SIG_IGN;
            raise(sig);
            if (!chained)
                return;
            return;
        }
        signal(sig, SIG_DFL);
        raise(sig);
    }

    inline bool InstallOne(int slot)
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof sa);
        sa.sa_sigaction = Handler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        g_havePrevious[slot] = (sigaction(kSignals[slot], &sa, &g_previous[slot]) == 0);
        return g_havePrevious[slot];
    }
}

// path == nullptr -> /data/data/<process package>/files/crash_breadcrumb.txt
inline bool CrashBreadcrumb_Install(const char *path = nullptr)
{
    if (path != nullptr && path[0] != 0)
    {
        const int n = snprintf(g_crashBreadcrumbPath, sizeof g_crashBreadcrumbPath, "%s", path);
        if (n <= 0 || n >= (int)sizeof g_crashBreadcrumbPath)
            return false;
    }
    else if (!CrashBreadcrumb_DefaultPath(g_crashBreadcrumbPath,
                                          (int)sizeof g_crashBreadcrumbPath))
    {
        return false;
    }

    stack_t ss;
    std::memset(&ss, 0, sizeof ss);
    ss.ss_sp = crash_breadcrumb_detail::g_altStack;
    ss.ss_size = sizeof crash_breadcrumb_detail::g_altStack;
    sigaltstack(&ss, nullptr);

    bool ok = false;
    for (int i = 0; i < 5; ++i)
        ok = crash_breadcrumb_detail::InstallOne(i) || ok;
    return ok;
}
