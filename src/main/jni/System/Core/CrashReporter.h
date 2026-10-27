#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#if defined(__aarch64__) || defined(__x86_64__)
#include <ucontext.h>
#endif

namespace crash_report
{
    constexpr int kMaxModules = 8;
    constexpr int kModuleName = 64;
    constexpr int kNoteSlots  = 32;
    constexpr int kNoteLen    = 80;
    constexpr int kSignals    = 4;

    constexpr const char *kLogTag = "ETHNIR_CRASH";
    constexpr const char *kLogEnv = "ETHNIR_CRASH_LOG";

    struct Module
    {
        uintptr_t begin;
        uintptr_t end;
        char      name[kModuleName];
    };

    inline Module g_modules[kMaxModules];
    inline int    g_moduleCount = 0;

    inline char             g_notes[kNoteSlots][kNoteLen];
    inline volatile int     g_noteSeq = 0;
    inline volatile sig_atomic_t g_inHandler = 0;

    inline int  g_fd = -1;
    inline bool g_installed = false;

    inline int  g_sigNum[kSignals] = { SIGSEGV, SIGBUS, SIGILL, SIGABRT };
    inline struct sigaction g_prev[kSignals];
    inline bool g_sigSaved[kSignals] = { false, false, false, false };

    inline char *PutChar(char *p, char *end, char c)
    {
        if (p < end) *p++ = c;
        return p;
    }

    inline char *PutStr(char *p, char *end, const char *s)
    {
        while (s != nullptr && *s != '\0' && p < end) *p++ = *s++;
        return p;
    }

    inline char *PutHex(char *p, char *end, uintptr_t v)
    {
        char tmp[2 * sizeof(uintptr_t) + 1];
        int n = 0;
        if (v == 0) tmp[n++] = '0';
        while (v != 0) {
            const unsigned digit = (unsigned)(v & 0xF);
            tmp[n++] = (char)(digit < 10 ? '0' + digit : 'a' + (digit - 10));
            v >>= 4;
        }
        while (n > 0 && p < end) *p++ = tmp[--n];
        return p;
    }

    inline char *PutDec(char *p, char *end, long v)
    {
        char tmp[24];
        int n = 0;
        const bool neg = v < 0;
        unsigned long u = neg ? (unsigned long)(-(v + 1)) + 1UL : (unsigned long)v;
        if (u == 0) tmp[n++] = '0';
        while (u != 0) { tmp[n++] = (char)('0' + (u % 10)); u /= 10; }
        if (neg) p = PutChar(p, end, '-');
        while (n > 0 && p < end) *p++ = tmp[--n];
        return p;
    }

    // Called from the signal handler: logcat and the log file only, no stdio,
    // no allocation, nothing that can take a lock.
    inline void WriteLine(const char *text, int len)
    {
        if (len <= 0) return;
        if (g_fd >= 0) {
            const ssize_t ignored = ::write(g_fd, text, (size_t)len);
            (void)ignored;
        }
#if defined(__ANDROID__)
        __android_log_write(ANDROID_LOG_ERROR, kLogTag, text);
#else
        const ssize_t ignored = ::write(STDERR_FILENO, text, (size_t)len);
        (void)ignored;
#endif
    }

    inline const Module *ModuleFor(uintptr_t addr)
    {
        for (int i = 0; i < g_moduleCount; ++i) {
            if (addr >= g_modules[i].begin && addr < g_modules[i].end)
                return &g_modules[i];
        }
        return nullptr;
    }

    inline void LocationLine(const char *label, uintptr_t addr)
    {
        char line[224];
        char *p = line;
        char *end = line + sizeof(line) - 1;

        p = PutStr(p, end, "CRASH ");
        p = PutStr(p, end, label);
        p = PutStr(p, end, "=0x");
        p = PutHex(p, end, addr);

        const Module *m = ModuleFor(addr);
        if (m != nullptr) {
            p = PutChar(p, end, ' ');
            p = PutStr(p, end, m->name);
            p = PutStr(p, end, "+0x");
            p = PutHex(p, end, addr - m->begin);
        } else {
            p = PutStr(p, end, " (no captured module)");
        }
        p = PutChar(p, end, '\n');

        WriteLine(line, (int)(p - line));
    }

    inline void Handle(int sig, siginfo_t *info, void *ctx)
    {
        if (g_inHandler) return;
        g_inHandler = 1;

        char line[192];
        char *p = line;
        char *end = line + sizeof(line) - 1;
        p = PutStr(p, end, "CRASH sig=");
        p = PutDec(p, end, (long)sig);
        if (info != nullptr) {
            p = PutStr(p, end, " addr=0x");
            p = PutHex(p, end, (uintptr_t)info->si_addr);
        }
        p = PutChar(p, end, '\n');
        WriteLine(line, (int)(p - line));

        uintptr_t pc = 0;
        uintptr_t lr = 0;
#if defined(__aarch64__)
        if (ctx != nullptr) {
            auto *uc = reinterpret_cast<ucontext_t *>(ctx);
            pc = (uintptr_t)uc->uc_mcontext.pc;
            lr = (uintptr_t)uc->uc_mcontext.regs[30];
        }
#elif defined(__x86_64__) && !defined(__ANDROID__)
        // The host check runs on x86_64, where the faulting instruction pointer
        // lives in RIP rather than in a pc field.
        if (ctx != nullptr) {
            auto *uc = reinterpret_cast<ucontext_t *>(ctx);
            pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
        }
#else
        (void)ctx;
#endif
        if (pc != 0) LocationLine("pc", pc);
        if (lr != 0) LocationLine("lr", lr);
        if (info != nullptr) LocationLine("fault", (uintptr_t)info->si_addr);

        // Oldest first, so the last line written is the last thing the mod did.
        const int newest = (int)(g_noteSeq - 1);
        for (int step = 0; step < kNoteSlots; ++step) {
            const int slot = (int)(((unsigned)(newest + 1 + step)) % kNoteSlots);
            const char *note = g_notes[slot];
            if (note[0] == '\0') continue;
            char trail[kNoteLen + 16];
            char *t = trail;
            char *tend = trail + sizeof(trail) - 1;
            t = PutStr(t, tend, "CRASH was: ");
            t = PutStr(t, tend, note);
            t = PutChar(t, tend, '\n');
            WriteLine(trail, (int)(t - trail));
        }

        g_inHandler = 0;
    }

    inline void Reraise(int sig)
    {
        for (int i = 0; i < kSignals; ++i) {
            if (g_sigNum[i] != sig) continue;
            if (g_sigSaved[i]) sigaction(sig, &g_prev[i], nullptr);
            else signal(sig, SIG_DFL);
            break;
        }

        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
        raise(sig);
    }

    inline void OnSignal(int sig, siginfo_t *info, void *ctx)
    {
        Handle(sig, info, ctx);
        Reraise(sig);
    }

    // A note is one line of context: what the mod was doing when it died. The
    // ring is fixed and lock free, so a note can be written from the render
    // thread, a game hook or the skin thread without any risk of blocking.
    inline void Note(const char *what)
    {
        if (what == nullptr) return;
        const int seq = g_noteSeq++;
        char *p = g_notes[(unsigned)seq % kNoteSlots];
        char *end = p + kNoteLen - 1;
        p = PutDec(p, end, (long)seq);
        p = PutChar(p, end, ' ');
        p = PutStr(p, end, what);
        *p = '\0';
    }

    inline void AddModule(const char *name, uintptr_t begin, uintptr_t end)
    {
        if (name == nullptr || name[0] == '\0') return;
        if (begin == 0 || end <= begin) return;
        if (g_moduleCount >= kMaxModules) return;

        Module &m = g_modules[g_moduleCount++];
        m.begin = begin;
        m.end = end;
        int i = 0;
        while (name[i] != '\0' && i < kModuleName - 1) { m.name[i] = name[i]; ++i; }
        m.name[i] = '\0';
    }

    inline void OpenLog()
    {
        const char *fromEnv = getenv(kLogEnv);
        const char *paths[4] = {
            fromEnv,
            "/sdcard/Download/ethnir_crash.txt",
            "/sdcard/ethnir_crash.txt",
            "/data/local/tmp/ethnir_crash.txt"
        };

        for (int i = 0; i < 4; ++i) {
            if (paths[i] == nullptr || paths[i][0] == '\0') continue;
            const int fd = ::open(paths[i], O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
            if (fd >= 0) {
                g_fd = fd;
                break;
            }
        }
    }

    inline void Install()
    {
        if (g_installed) return;
        g_installed = true;
        OpenLog();

        struct sigaction sa {};
        sa.sa_sigaction = &OnSignal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;

        for (int i = 0; i < kSignals; ++i) {
            struct sigaction prev {};
            if (sigaction(g_sigNum[i], &sa, &prev) == 0) {
                g_prev[i] = prev;
                g_sigSaved[i] = true;
            }
        }

        Note("crash reporter armed");
    }
}
