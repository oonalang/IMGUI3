#pragma once

#include <cstdint>

#include <setjmp.h>
#include <signal.h>

#include "CrashReporter.h"

namespace fault_guard
{
    inline thread_local sigjmp_buf g_jmp;
    inline thread_local volatile sig_atomic_t g_armed = 0;
    inline bool g_installed = false;

    inline void OnSignal(int sig, siginfo_t *info, void *ctx)
    {
        if (g_armed)
        {
            g_armed = 0;
            siglongjmp(g_jmp, 1);
        }

        // A fault outside a guarded section is a real crash, and this handler
        // owns the signal once the first guarded call has run. Name it before
        // handing the process back to the default action, so the report and the
        // tombstone describe the same fault.
        crash_report::Handle(sig, info, ctx);

        ::signal(sig, SIG_DFL);
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        sigprocmask(SIG_UNBLOCK, &set, nullptr);
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
