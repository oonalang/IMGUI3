#pragma once

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>
#include <chrono>

// Hold the account against a second sign-in.
//
// The previous version blocked the client's own relogin gates. That is the
// wrong half of the fight: with the relogin suppressed the client keeps a
// session the server has already invalidated, never authenticates again, and the
// other device keeps the account while this one sits in a dead match.
//
// The newest sign-in owns the session, so holding the account means being the
// one that keeps signing in. Every gate now forwards, and the relogin is made
// fast and repeatable instead:
//
//   * StageManager.OnReceiveLobbyLogout / OnReceiveDSLogout are the game's own
//     "this session is over" signals. They open a bounded reclaim window.
//   * Inside that window the auto-login cycle is kept alive (CanRetryAutoLogin,
//     CanStartAutoLogin, GetMaxAutoLoginNum) instead of stopping after its few
//     built-in retries. That retry budget running out is why a single blocked
//     relogin leaves the other side in the game for good.
//   * BrokerSocketManager's relogin wait is zeroed so the retry starts on the
//     tick the game asks for it instead of after its random delay.
//
// The window is bounded (kWindowSeconds, kMaxForcedStarts) so a server that
// keeps refusing cannot turn the client into an unattended login loop.

namespace anti_login
{
    enum Gate
    {
        GateHeartbeatExec = 0,
        GateCheckState,
        GateReloginCallback,
        GateLogout,
        GateLoadCallback,
        GateLobbyLogout,
        GateDsLogout,
        GateAutoRetry,
        GateAutoStart,
        GateAutoMax,
        GateCount
    };

    inline bool g_landed[GateCount] = {};

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline bool Enabled()
    {
        return Config.ExtraMenu.AntiLogin;
    }

    constexpr double kWindowSeconds    = 30.0;
    constexpr int    kMaxForcedStarts  = 12;
    constexpr int    kMaxAutoLoginNum  = 999;
    constexpr int    kMaxPlausibleWait = 600;

    // BrokerSocketManager._reloginDelayTime, checked against the 1.6.57 dump.
    constexpr uintptr_t kReloginDelayField = 0x28;

    inline double g_windowUntil     = -1000.0;
    inline int    g_windowCount     = 0;
    inline int    g_forcedStarts    = 0;
    inline int    g_delaySkips      = 0;
    inline int    g_reloginCalls    = 0;
    inline int    g_lobbyLogouts    = 0;
    inline int    g_dsLogouts       = 0;
    inline int    g_lastReloginState = -1;

    inline double Now()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline bool WindowOpen()
    {
        return Now() < g_windowUntil;
    }

    inline void OpenWindow(const char *why)
    {
        const double now = Now();
        if (now >= g_windowUntil) {
            g_windowCount++;
            g_forcedStarts = 0;
            LOGI("[AntiLogin] reclaim window opened (%s) #%d", why, g_windowCount);
        }
        g_windowUntil = now + kWindowSeconds;
    }

    inline void CloseWindow(const char *why)
    {
        if (g_windowUntil <= -1000.0) return;
        g_windowUntil = -1000.0;
        LOGI("[AntiLogin] reclaim window closed (%s) forced=%d", why, g_forcedStarts);
    }

    inline bool ForcingAllowed()
    {
        return Enabled() && WindowOpen() && g_forcedStarts < kMaxForcedStarts;
    }

    inline void *(*orig_ExecuteReloginByRandomDelay)(void *instance) = nullptr;
    inline void *hook_ExecuteReloginByRandomDelay(void *instance)
    {
        if (Enabled() && instance != nullptr) {
            int *wait = (int *)((uintptr_t)instance + kReloginDelayField);
            const int current = *wait;
            if (current > 0 && current <= kMaxPlausibleWait) {
                *wait = 0;
                g_delaySkips++;
            }
            OpenWindow("socket relogin");
        }
        return orig_ExecuteReloginByRandomDelay != nullptr
             ? orig_ExecuteReloginByRandomDelay(instance)
             : nullptr;
    }

    inline void (*orig_CheckReloginState)(void *instance, int state) = nullptr;
    inline void hook_CheckReloginState(void *instance, int state)
    {
        g_lastReloginState = state;
        if (Enabled()) {
            // 0 reloginStart, 1 reloginFailed, 2 reloginSucceed.
            if (state == 2) CloseWindow("relogin succeeded");
            else OpenWindow(state == 1 ? "relogin failed" : "relogin started");
        }
        if (orig_CheckReloginState != nullptr) orig_CheckReloginState(instance, state);
    }

    inline void (*orig_ReloginCallback)(void *instance) = nullptr;
    inline void hook_ReloginCallback(void *instance)
    {
        g_reloginCalls++;
        if (Enabled()) OpenWindow("relogin callback");
        if (orig_ReloginCallback != nullptr) orig_ReloginCallback(instance);
    }

    inline void (*orig_Logout)(void *instance, bool isRelogin) = nullptr;
    inline void hook_Logout(void *instance, bool isRelogin)
    {
        if (Enabled() && isRelogin) OpenWindow("relogin logout");
        if (orig_Logout != nullptr) orig_Logout(instance, isRelogin);
    }

    inline void (*orig_OnNotifyNeedReloginCallback)(void *instance) = nullptr;
    inline void hook_OnNotifyNeedReloginCallback(void *instance)
    {
        if (Enabled()) OpenWindow("need relogin notice");
        if (orig_OnNotifyNeedReloginCallback != nullptr)
            orig_OnNotifyNeedReloginCallback(instance);
    }

    // The game's own end-of-session signals. A second sign-in lands here as a
    // lobby logout; the battle server dropping the player mid-match lands as a
    // data-server logout.
    inline void (*orig_OnReceiveLobbyLogout)(void *instance) = nullptr;
    inline void hook_OnReceiveLobbyLogout(void *instance)
    {
        g_lobbyLogouts++;
        if (Enabled()) OpenWindow("lobby logout");
        if (orig_OnReceiveLobbyLogout != nullptr) orig_OnReceiveLobbyLogout(instance);
    }

    inline void (*orig_OnReceiveDSLogout)(void *instance) = nullptr;
    inline void hook_OnReceiveDSLogout(void *instance)
    {
        g_dsLogouts++;
        if (Enabled()) OpenWindow("data server logout");
        if (orig_OnReceiveDSLogout != nullptr) orig_OnReceiveDSLogout(instance);
    }

    inline bool (*orig_CanRetryAutoLogin)() = nullptr;
    inline bool hook_CanRetryAutoLogin()
    {
        if (ForcingAllowed()) return true;
        return orig_CanRetryAutoLogin != nullptr ? orig_CanRetryAutoLogin() : false;
    }

    inline bool (*orig_CanStartAutoLogin)(int loginStatus, bool acceptAccessTokenExpired) = nullptr;
    inline bool hook_CanStartAutoLogin(int loginStatus, bool acceptAccessTokenExpired)
    {
        const bool orig = orig_CanStartAutoLogin != nullptr
                        ? orig_CanStartAutoLogin(loginStatus, acceptAccessTokenExpired)
                        : false;
        if (!ForcingAllowed()) return orig;

        g_forcedStarts++;
        LOGI("[AntiLogin] auto-login forced %d/%d (status=%d expired=%d)",
             g_forcedStarts, kMaxForcedStarts, loginStatus,
             (int)acceptAccessTokenExpired);
        return true;
    }

    inline int (*orig_GetMaxAutoLoginNum)() = nullptr;
    inline int hook_GetMaxAutoLoginNum()
    {
        const int orig = orig_GetMaxAutoLoginNum != nullptr
                       ? orig_GetMaxAutoLoginNum() : 0;
        if (Enabled() && WindowOpen() && orig < kMaxAutoLoginNum)
            return kMaxAutoLoginNum;
        return orig;
    }

    inline void InstallAntiLoginHooks()
    {
        HOOK_LIB("libunity.so", "0x41CDF5C", hook_ExecuteReloginByRandomDelay, orig_ExecuteReloginByRandomDelay);
        HOOK_LIB("libunity.so", "0x850D610", hook_CheckReloginState, orig_CheckReloginState);
        HOOK_LIB("libunity.so", "0x850E440", hook_ReloginCallback, orig_ReloginCallback);
        HOOK_LIB("libunity.so", "0x851139C", hook_Logout, orig_Logout);
        HOOK_LIB("libunity.so", "0x768E850", hook_OnNotifyNeedReloginCallback, orig_OnNotifyNeedReloginCallback);
        HOOK_LIB("libunity.so", "0x9A09504", hook_OnReceiveLobbyLogout, orig_OnReceiveLobbyLogout);
        HOOK_LIB("libunity.so", "0x9A07DCC", hook_OnReceiveDSLogout, orig_OnReceiveDSLogout);
        HOOK_LIB("libunity.so", "0x778F2A8", hook_CanRetryAutoLogin, orig_CanRetryAutoLogin);
        HOOK_LIB("libunity.so", "0x778F37C", hook_CanStartAutoLogin, orig_CanStartAutoLogin);
        HOOK_LIB("libunity.so", "0x778F114", hook_GetMaxAutoLoginNum, orig_GetMaxAutoLoginNum);
    }
}
