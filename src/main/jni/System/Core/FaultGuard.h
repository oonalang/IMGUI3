#pragma once

#include <cstdint>

#include <setjmp.h>
#include <signal.h>

namespace fault_guard
{
    inline thread_local sigjmp_buf g_jmp;
    inline thread_local volatile sig_atomic_t g_armed = 0;
    inline bool g_installed = false;

    inline void OnSignal(int sig)
    {
        if (g_armed)
        {
            g_armed = 0;
            siglongjmp(g_jmp, 1);
        }

        ::signal(sig, SIG_DFL);
        ::raise(sig);
    }

    inline bool Install()
    {
        if (g_installed)
            return true;

        struct sigaction sa {};
        sa.sa_handler = &OnSignal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_NODEFER;

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
