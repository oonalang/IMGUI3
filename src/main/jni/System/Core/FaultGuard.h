#pragma once

#include <cstdint>
#include <cstddef>

#include <setjmp.h>
#include <signal.h>
#include <ucontext.h>

#if defined(__ANDROID__)
#include <android/log.h>
#else
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
    __attribute__((weak));
#endif

namespace fault_guard
{
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

#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, "EthnirCrash", "native crash: %s", line);
#else
        if (__android_log_print != nullptr)
            __android_log_print(6, "EthnirCrash", "native crash: %s", line);
#endif
    }

    inline void OnSignal(int sig, siginfo_t *info, void *ctx)
    {
        if (g_armed)
        {
            g_armed = 0;
            siglongjmp(g_jmp, 1);
        }

        ReportCrash(sig, info, ctx);

        ::signal(sig, SIG_DFL);
        ::raise(sig);
    }

    inline bool Install()
    {
        if (g_installed)
            return true;

        struct sigaction sa {};
        sa.sa_sigaction = &OnSignal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NODEFER | SA_SIGINFO;

        if (sigaction(SIGSEGV, &sa, nullptr) != 0)
            return false;
        sigaction(SIGBUS, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);

        g_installed = true;
        return true;
    }

    template <typename Fn>
    inline bool Run(Fn&& fn)
    {
        if (!Install())
            return false;

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
