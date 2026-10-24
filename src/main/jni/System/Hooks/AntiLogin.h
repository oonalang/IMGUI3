#pragma once

#include "Hacks/Feature/Init_Hacks.h"
#include "System/Core/FaultGuard.h"

#include <atomic>
#include <cstdint>
#include <ctime>

namespace anti_login
{
    enum Gate
    {
        GateKickDispatch = 0,
        GateKickPopup,
        GateCount
    };

    constexpr uintptr_t KickDispatchRva = 0x53F47E8;
    constexpr uintptr_t KickPopupRva    = 0x6F52BB0;
    constexpr uintptr_t ReloginRva      = 0x9812F88;

    constexpr int ReclaimAttempts = 15;
    constexpr int64_t ReclaimStepMs = 1200;

    inline bool g_landed[GateCount] = {};
    inline std::atomic<int> g_kickEvents{0};
    inline std::atomic<int> g_kickPopupsBlocked{0};
    inline std::atomic<int> g_reclaims{0};
    inline std::atomic<int> g_reclaimFaults{0};
    inline std::atomic<int> g_attemptsLeft{0};
    inline std::atomic<int64_t> g_nextAttemptMs{0};

    inline bool Enabled()
    {
        return Config.ExtraMenu.AntiLogin;
    }

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount)
            g_landed[gate] = true;
    }

    inline int64_t NowMs()
    {
        timespec ts{};
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
            return 0;
        return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    inline void ReloginNow()
    {
        using ReloginFn = void (*)();

        static ReloginFn relogin = nullptr;
        static bool resolved = false;

        if (!resolved) {
            resolved = true;
            relogin = (ReloginFn)getAbsoluteAddress("libunity.so", ReloginRva);
        }
        if (relogin == nullptr) {
            g_reclaimFaults++;
            return;
        }

        const ReloginFn call = relogin;
        if (fault_guard::Run([call] { call(); }))
            g_reclaims++;
        else {
            g_reclaimFaults++;
            LOGE("[anti-login] relogin call faulted, retries left %d",
                 g_attemptsLeft.load(std::memory_order_relaxed));
        }
    }

    inline void RequestReclaim()
    {
        if (!Enabled())
            return;

        const int kicks = ++g_kickEvents;
        g_attemptsLeft.store(ReclaimAttempts, std::memory_order_relaxed);
        g_nextAttemptMs.store(NowMs(), std::memory_order_relaxed);
        LOGI("[anti-login] kick #%d, reclaiming the session", kicks);
    }

    inline void Pump()
    {
        if (!Enabled()) {
            g_attemptsLeft.store(0, std::memory_order_relaxed);
            return;
        }
        if (g_attemptsLeft.load(std::memory_order_relaxed) <= 0)
            return;

        const int64_t now = NowMs();
        if (now < g_nextAttemptMs.load(std::memory_order_relaxed))
            return;

        g_nextAttemptMs.store(now + ReclaimStepMs, std::memory_order_relaxed);
        g_attemptsLeft.fetch_sub(1, std::memory_order_relaxed);
        ReloginNow();
    }

    inline void (*orig_OnKickDispatch)(void *instance, void *msg) = nullptr;
    inline void hook_OnKickDispatch(void *instance, void *msg)
    {
        if (Enabled())
            RequestReclaim();

        if (orig_OnKickDispatch != nullptr)
            orig_OnKickDispatch(instance, msg);
    }

    inline void (*orig_OnKickPopup)(void *instance, void *msg) = nullptr;
    inline void hook_OnKickPopup(void *instance, void *msg)
    {
        if (Enabled()) {
            g_kickPopupsBlocked++;
            RequestReclaim();
            return;
        }

        if (orig_OnKickPopup != nullptr)
            orig_OnKickPopup(instance, msg);
    }

    inline void InstallAntiLoginHooks()
    {
        HOOK_LIB("libunity.so", "0x53F47E8", hook_OnKickDispatch, orig_OnKickDispatch);
        HOOK_LIB("libunity.so", "0x6F52BB0", hook_OnKickPopup, orig_OnKickPopup);

        NoteLanded(GateKickDispatch, orig_OnKickDispatch != nullptr);
        NoteLanded(GateKickPopup, orig_OnKickPopup != nullptr);

        LOGI("[anti-login] kick dispatch=%p popup=%p relogin=%p",
             (void *)orig_OnKickDispatch, (void *)orig_OnKickPopup,
             (void *)getAbsoluteAddress("libunity.so", ReloginRva));
    }
}
