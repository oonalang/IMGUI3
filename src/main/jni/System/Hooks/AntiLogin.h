#pragma once

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace anti_login
{
    enum Gate
    {
        GateHeartbeatExec = 0,
        GateCheckState,
        GateReloginCallback,
        GateLogout,
        GateLoadCallback,
        GateCount
    };

    inline bool Enabled()
    {
        return Config.ExtraMenu.AntiLogin;
    }

    inline bool g_landed[GateCount] = {};
    inline int  g_blocked[GateCount] = {};
    inline int  g_lastState = 0;

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void NoteBlocked(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_blocked[gate]++;
    }

    inline void *(*orig_ExecuteReloginByRandomDelay)(void *instance) = nullptr;
    inline void *hook_ExecuteReloginByRandomDelay(void *instance)
    {
        if (Enabled()) {
            NoteBlocked(GateHeartbeatExec);
            return nullptr;
        }
        return orig_ExecuteReloginByRandomDelay != nullptr
             ? orig_ExecuteReloginByRandomDelay(instance)
             : nullptr;
    }

    inline void (*orig_CheckReloginState)(void *instance, int state) = nullptr;
    inline void hook_CheckReloginState(void *instance, int state)
    {
        g_lastState = state;
        if (Enabled()) {
            NoteBlocked(GateCheckState);
            return;
        }
        if (orig_CheckReloginState != nullptr) orig_CheckReloginState(instance, state);
    }

    inline void (*orig_ReloginCallback)(void *instance) = nullptr;
    inline void hook_ReloginCallback(void *instance)
    {
        if (Enabled()) {
            NoteBlocked(GateReloginCallback);
            return;
        }
        if (orig_ReloginCallback != nullptr) orig_ReloginCallback(instance);
    }

    inline void (*orig_Logout)(void *instance, bool isRelogin) = nullptr;
    inline void hook_Logout(void *instance, bool isRelogin)
    {
        if (Enabled() && isRelogin) {
            NoteBlocked(GateLogout);
            return;
        }
        if (orig_Logout != nullptr) orig_Logout(instance, isRelogin);
    }

    inline void (*orig_OnNotifyNeedReloginCallback)(void *instance) = nullptr;
    inline void hook_OnNotifyNeedReloginCallback(void *instance)
    {
        if (Enabled()) {
            NoteBlocked(GateLoadCallback);
            return;
        }
        if (orig_OnNotifyNeedReloginCallback != nullptr)
            orig_OnNotifyNeedReloginCallback(instance);
    }

    inline void InstallAntiLoginHooks()
    {
        HOOK_LIB("libunity.so", "0x41CDF5C", hook_ExecuteReloginByRandomDelay, orig_ExecuteReloginByRandomDelay);
        HOOK_LIB("libunity.so", "0x850D610", hook_CheckReloginState, orig_CheckReloginState);
        HOOK_LIB("libunity.so", "0x850E440", hook_ReloginCallback, orig_ReloginCallback);
        HOOK_LIB("libunity.so", "0x851139C", hook_Logout, orig_Logout);
        HOOK_LIB("libunity.so", "0x768E850", hook_OnNotifyNeedReloginCallback, orig_OnNotifyNeedReloginCallback);
    }
}

