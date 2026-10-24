#pragma once

#include <cstdint>
#include <cstddef>

#include <setjmp.h>
#include <signal.h>
#include <ucontext.h>

#include <time.h>

#if defined(__ANDROID__)
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#else
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
    __attribute__((weak));
#endif

namespace fault_guard
{
    using HandlerFn = void (*)(int, siginfo_t *, void *);

    inline thread_local sigjmp_buf g_jmp;
    inline thread_local volatile sig_atomic_t g_armed = 0;
    inline bool g_installed = false;

    // Base of libunity.so. A crash report names the offset inside the module
    // rather than the absolute address, because the absolute address is
    // different on every launch while the offset can be looked up in a dump.
    inline uintptr_t g_moduleBase = 0;
    inline const char *g_moduleName = "libunity.so";

    // The phase the mod was in, so a crash that lands in game code still says
    // which feature was running at the time. Only string literals are stored,
    // which keeps this readable from the signal handler.
    inline thread_local const char *g_phase = "init";

    inline void SetModuleBase(void *base)
    {
        g_moduleBase = (uintptr_t)base;
    }

    inline void NotePhase(const char *phase)
    {
        if (phase != nullptr)
            g_phase = phase;
    }

    inline void GuardLog(const char *msg)
    {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, "EthnirCrash", "%s", msg);
#else
        if (__android_log_print != nullptr)
            __android_log_print(6, "EthnirCrash", "%s", msg);
#endif
    }

    // ------------------------------------------------------------------
    // File-backed trace, because logcat is not always reachable.
    //
    // A crash that kills the process within seconds of launch leaves nothing in
    // the UI and needs adb to read from logcat. This writes the same information
    // into the process's own private files directory instead, so the last line
    // in the file names the stage the process died in and the fault line names
    // the offending offset inside libunity.so. Both are readable from a file
    // manager or through the app's own storage without a debugger attached.
    //
    // Only open/write/close are used, which are async-signal-safe, so the same
    // writer is called from the signal handler.
    // ------------------------------------------------------------------
#if defined(__ANDROID__)
    inline char *TraceDir()
    {
        static char dir[160] = {0};
        if (dir[0] != '\0') return dir;

        // The package name is the first NUL-terminated field of cmdline.
        char pkg[96] = {0};
        const int fd = ::open("/proc/self/cmdline", O_RDONLY);
        if (fd >= 0) {
            const ssize_t n = ::read(fd, pkg, sizeof(pkg) - 1);
            ::close(fd);
            if (n <= 0) return nullptr;
        } else {
            return nullptr;
        }

        // Reject anything that is not a plausible package name so a weird
        // cmdline cannot steer the write somewhere else.
        int i = 0;
        for (; pkg[i] != '\0' && i < (int)sizeof(pkg) - 1; i++) {
            const char c = pkg[i];
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') || c == '.' || c == '_';
            if (!ok) break;
        }
        if (i < 3) return nullptr;
        pkg[i] = '\0';

        // Built by hand rather than with AppendText: this block sits above the
        // formatting helpers further down the header, and a forward use would
        // only compile on the host branch.
        size_t w = 0;
        const char *pre = "/data/data/";
        while (*pre != '\0' && w < sizeof(dir) - 32) dir[w++] = *pre++;
        for (int k = 0; pkg[k] != '\0' && w < sizeof(dir) - 32; k++)
            dir[w++] = pkg[k];
        const char *post = "/files/startup_trace.log";
        while (*post != '\0' && w < sizeof(dir) - 1) dir[w++] = *post++;
        dir[w] = '\0';
        return dir;
    }

    // One line per call, appended and flushed by the close. Deliberately not
    // buffered: the point is that the file survives the process dying.
    inline void TraceWrite(const char *text)
    {
        if (text == nullptr) return;
        const char *path = TraceDir();
        if (path == nullptr) return;

        const int fd = ::open(path, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (fd < 0) return;

        size_t len = 0;
        while (text[len] != '\0') len++;
        if (len > 0) (void)!::write(fd, text, len);

        const char nl = '\n';
        (void)!::write(fd, &nl, 1);
        ::close(fd);
    }
#else
    inline char *TraceDir() { return nullptr; }
    inline void TraceWrite(const char *text) { (void)text; }
#endif

    // Milestone marker for the startup sequence. Called before the signal
    // handler is installed so the file exists even if the first hook crashes.
    inline void Mark(const char *stage)
    {
        TraceWrite(stage);
        if (stage != nullptr) g_phase = stage;
    }

    inline char *AppendText(char *w, char *end, const char *s)
    {
        while (*s != '\0' && w < end) *w++ = *s++;
        return w;
    }

    inline char *AppendHex(char *w, char *end, uintptr_t v)
    {
        char tmp[2 * sizeof(uintptr_t)];
        int n = 0;
        do {
            const int digit = (int)(v & 0xF);
            tmp[n++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
            v >>= 4;
        } while (v != 0 && n < (int)sizeof(tmp));

        if (w < end) *w++ = '0';
        if (w < end) *w++ = 'x';
        while (n > 0 && w < end) *w++ = tmp[--n];
        return w;
    }

    inline char *AppendDec(char *w, char *end, long v)
    {
        char tmp[24];
        int n = 0;
        unsigned long u = v < 0 ? (unsigned long)(-v) : (unsigned long)v;
        do {
            tmp[n++] = (char)('0' + (u % 10));
            u /= 10;
        } while (u != 0 && n < (int)sizeof(tmp));

        if (v < 0 && w < end) *w++ = '-';
        while (n > 0 && w < end) *w++ = tmp[--n];
        return w;
    }

    inline void FormatAddress(char *out, size_t outSize, uintptr_t addr)
    {
        if (out == nullptr || outSize < 2) return;

        char *w = out;
        char *end = out + outSize - 1;
        if (g_moduleBase != 0 && addr >= g_moduleBase &&
            (addr - g_moduleBase) < (uintptr_t)0x40000000) {
            w = AppendText(w, end, g_moduleName);
            if (w < end) *w++ = '+';
            w = AppendHex(w, end, addr - g_moduleBase);
        } else {
            w = AppendHex(w, end, addr);
        }
        *w = '\0';
    }

    inline void DescribeFault(char *out, size_t outSize, int sig,
                              uintptr_t faultyAddr, uintptr_t pc, uintptr_t lr,
                              const char *phase = "?")
    {
        if (out == nullptr || outSize < 2) return;

        char *w = out;
        char *end = out + outSize - 1;
        char addr[64];

        w = AppendText(w, end, "signal=");
        w = AppendDec(w, end, sig);
        w = AppendText(w, end, " fault=");
        w = AppendHex(w, end, faultyAddr);

        FormatAddress(addr, sizeof(addr), pc);
        w = AppendText(w, end, " pc=");
        w = AppendText(w, end, addr);

        FormatAddress(addr, sizeof(addr), lr);
        w = AppendText(w, end, " lr=");
        w = AppendText(w, end, addr);

        w = AppendText(w, end, " phase=");
        w = AppendText(w, end, phase != nullptr ? phase : "?");

        *w = '\0';
    }

    // ------------------------------------------------------------------
    // Ownership of the signal handler.
    //
    // The guard only does anything while its handler is the one the kernel
    // calls, and this process has other owners: xhook installs its own SIGSEGV
    // handler around xhook_refresh(), and the game and its anti-cheat install
    // theirs whenever they like. Whoever installs last wins silently, and the
    // cost is that Run() keeps setting g_armed while the fault goes straight to
    // a handler that has never heard of it - the guard looks correct, is
    // tested, and then every fault kills the process anyway. That is the shape
    // of "the triggerbot is guarded and it still crashes on an enemy".
    //
    // So ownership is checked again before every guarded section (throttled),
    // taken back when it was lost, and the handler that had taken it is chained
    // to on an unguarded fault instead of being thrown away.
    // ------------------------------------------------------------------
    // SIGABRT is in the set because a fault is not the only way this process
    // dies. An unhandled managed exception inside il2cpp calls abort(), and a
    // std::terminate, a failed assertion or an allocation failure does the
    // same; all of them arrive here as SIGABRT and none of them were covered
    // before, so a whole class of startup death produced no record at all.
    inline const int kSigs[4] = { SIGSEGV, SIGBUS, SIGILL, SIGABRT };
    inline const char *kSigName[4] = { "SIGSEGV", "SIGBUS", "SIGILL", "SIGABRT" };
    constexpr int kSigCount = 4;

    inline struct sigaction g_prev[4] = {};
    inline bool g_prevSet[4] = {false, false, false, false};
    inline bool g_oursSet[4] = {false, false, false, false};
    inline int g_takeBacks = 0;
    inline uint64_t g_lastCheckMs = 0;

    inline void OnSignal(int sig, siginfo_t *info, void *ctx);

    inline bool IsOurs(const struct sigaction &a)
    {
        if (!(a.sa_flags & SA_SIGINFO)) return false;
        return a.sa_sigaction == reinterpret_cast<HandlerFn>(&OnSignal);
    }

    inline int SigIndex(int sig)
    {
        if (sig == SIGSEGV) return 0;
        if (sig == SIGBUS)  return 1;
        if (sig == SIGILL)  return 2;
        if (sig == SIGABRT) return 3;
        return 3;
    }

    inline void ReportCrash(int sig, void *infoPtr, void *ctx);

    inline void OnSignal(int sig, siginfo_t *info, void *ctx)
    {
        if (g_armed)
        {
            g_armed = 0;
            siglongjmp(g_jmp, 1);
        }

        ReportCrash(sig, info, ctx);

        // A real crash, outside any guarded section. Re-raise it as the default
        // action so Android writes its tombstone, the same way it would have
        // without us. Deliberately NOT delegating to g_prev: a handler that was
        // displaced only to restore ours and return would spin the two handlers
        // off each other instead of killing the process.
        ::signal(sig, SIG_DFL);
        ::raise(sig);
    }

    inline bool InstallOne(int idx)
    {
        struct sigaction sa {};
        sa.sa_sigaction = &OnSignal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NODEFER | SA_SIGINFO;

        struct sigaction prev {};
        if (sigaction(kSigs[idx], &sa, &prev) != 0)
            return false;

        if (IsOurs(prev))
        {
            g_oursSet[idx] = true;
            return true;
        }

        // Something real was there before us - remembered so a take-back can
        // name who had it, and so an audit of the handler state is possible.
        // It is never invoked: see OnSignal.
        const bool realHandler = (prev.sa_flags & SA_SIGINFO)
            ? (prev.sa_sigaction != nullptr &&
               prev.sa_sigaction != reinterpret_cast<HandlerFn>(&OnSignal))
            : (prev.sa_handler != nullptr && prev.sa_handler != SIG_DFL &&
               prev.sa_handler != SIG_IGN);
        if (realHandler)
        {
            g_prev[idx] = prev;
            g_prevSet[idx] = true;
        }
        g_oursSet[idx] = true;
        return true;
    }

    inline bool Install()
    {
        bool ok = true;
        for (int i = 0; i < kSigCount; i++)
            if (!InstallOne(i)) ok = false;
        g_installed = true;
        return ok;
    }

    inline uint64_t NowMs()
    {
        struct timespec ts {};
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
    }

    // Called before a guarded section: cheap enough to run often, throttled to
    // every 100 ms because each check is a syscall.
    inline void EnsureInstalled()
    {
        if (!g_installed) { Install(); return; }

        const uint64_t now = NowMs();
        if (now - g_lastCheckMs < 100) return;
        g_lastCheckMs = now;

        for (int i = 0; i < kSigCount; i++)
        {
            struct sigaction cur {};
            if (sigaction(kSigs[i], nullptr, &cur) != 0) continue;
            if (IsOurs(cur)) continue;

            if (InstallOne(i))
            {
                g_takeBacks++;
                char line[176];
                char *w = line;
                char *end = line + sizeof(line) - 1;
                w = AppendText(w, end, "fault_guard: handler for ");
                w = AppendText(w, end, kSigName[i]);
                w = AppendText(w, end, " was replaced - took it back from ");
                w = AppendHex(w, end,
                    (uintptr_t)(g_prevSet[i]
                        ? ((g_prev[i].sa_flags & SA_SIGINFO)
                             ? (uintptr_t)(void *)g_prev[i].sa_sigaction
                             : (uintptr_t)(void *)g_prev[i].sa_handler)
                        : 0));
                w = AppendText(w, end, " - takebacks=");
                w = AppendDec(w, end, g_takeBacks);
                *w = '\0';
                GuardLog(line);
            }
        }
    }

    inline bool IsInstalled()
    {
        struct sigaction cur {};
        if (sigaction(SIGSEGV, nullptr, &cur) != 0) return false;
        return IsOurs(cur);
    }

    // Only the pieces that are safe inside a signal handler: fixed buffers, no
    // allocation, no libc formatting.
    inline void ReportCrash(int sig, void *infoPtr, void *ctx)
    {
        uintptr_t faulty = 0;
        uintptr_t pc = 0;
        uintptr_t lr = 0;

#if defined(__ANDROID__)
        if (infoPtr != nullptr)
            faulty = (uintptr_t)((const siginfo_t *)infoPtr)->si_addr;
#endif

#if defined(__ANDROID__) && defined(__aarch64__)
        if (ctx != nullptr) {
            const ucontext_t *uc = (const ucontext_t *)ctx;
            pc = (uintptr_t)uc->uc_mcontext.pc;
            lr = (uintptr_t)uc->uc_mcontext.regs[30];
        }
#elif defined(__ANDROID__) && defined(__arm__)
        if (ctx != nullptr) {
            const ucontext_t *uc = (const ucontext_t *)ctx;
            pc = (uintptr_t)uc->uc_mcontext.arm_pc;
            lr = (uintptr_t)uc->uc_mcontext.arm_lr;
        }
#endif

        char line[192];
        DescribeFault(line, sizeof(line), sig, faulty, pc, lr, g_phase);
        GuardLog(line);
        // The same line goes to the file, because this is the only record that
        // survives when logcat cannot be reached. The last breadcrumb above it
        // names the stage that did not complete.
        TraceWrite(line);
    }

    template <typename Fn>
    inline bool Run(Fn&& fn)
    {
        // A guard nested inside another one inherits the outer section instead
        // of arming a second jump buffer.
        //
        // Re-entering used to be harmless-looking and was not: the inner call
        // overwrote g_jmp with its own buffer and cleared g_armed on the way
        // out, so every line of the outer section AFTER the first inner call ran
        // with the handler installed but disarmed - a fault there went straight
        // to the tombstone, exactly like no guard at all. The triggerbot is full
        // of such nests (the LOS ray calls the muzzle read, which is guarded,
        // and the collider walk that follows is not), which is how a "guarded"
        // feature can still kill the process the moment it looks at an enemy.
        if (g_armed)
        {
            fn();
            return true;
        }

        if (!Install())
            return false;

        EnsureInstalled();

        if (sigsetjmp(g_jmp, 1) != 0)
        {
            g_armed = 0;
            return false;
        }

        g_armed = 1;
        fn();
        g_armed = 0;
        return true;
    }
}
