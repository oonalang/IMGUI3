#pragma once

// ---------------------------------------------------------------------------
// Anti-Login.
//
// Someone else logging into your account (a cousin with the password, a
// shared account) makes the SERVER retire your session: your client's
// heartbeat starts getting "relogin" answers and the client complies by
// showing CONNECT / "Account accessed from another location. Please secure
// your account. (0E100004)" with a QUIT GAME button. Anti-Login refuses to
// comply: every client-side step of that forced relogin is swallowed, so
// the popup never appears and the session is never handed over by this
// device.
//
// The path, RVA-verified against the current dump:
//   BrokerSocketManager.ExecuteReloginByRandomDelay()   0x41CDF5C
//     the socket layer's relogin executor (heartbeat learned the session
//     must relogin - IsReloginFromHeartbeat - and this runs the relogin
//     after a random delay). Blocked: the relogin never starts.
//   Pandora.CheckReloginState(int state)                0x850D610
//     the account layer's verdict on the relogin state the server reports.
//     Blocked: the state is never acted on (the raw state is kept in
//     g_lastState for internal inspection).
//   Pandora.ReloginCallback()                           0x850E440
//     the callback that raises the CONNECT popup / logout flow. Blocked.
//   Pandora.Logout(bool isRelogin)                      0x851139C
//     blocked ONLY for the forced form (isRelogin = true); a manual logout
//     is passed through untouched even with Anti-Login on, so the account
//     menu keeps working.
//   LoadingFlowCheckPayTokenController.
//     OnNotifyNeedReloginCallback()                     0x768E850
//     the same verdict arriving through the loading flow's pay-token check
//     (its NotifyNeedRelogin @0x768E6E8 is deliberately NOT hooked: it also
//     drives the access-token refresh the session needs - the callback is
//     the part that acts on the verdict).
//
// WHAT THIS CANNOT DO: it cannot put the popup on the OTHER device, and it
// cannot stop the server from invalidating the session. Which of the two
// sessions the server keeps is server policy (usually newest login wins);
// if the server closes this client's socket anyway, the game will show a
// generic disconnect no client hook can prevent. What Anti-Login removes
// is the client-side cooperation: no popup, no self-logout, no relogin.
// The actual fix for a shared password is still changing it / logging out
// other sessions in the account settings - this feature is the shield
// while that is sorted out.
//
// Gated by Config.ExtraMenu.AntiLogin at the TOP of each body (same shape
// as ReportBlock.h): toggle off = one boolean read and the exact original
// call, so stock behaviour is restored byte for byte. The counters below
// are code-level only; nothing is printed in the menu.
// ---------------------------------------------------------------------------

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace anti_login
{
    enum Gate
    {
        GateHeartbeatExec = 0,  // BrokerSocketManager.ExecuteReloginByRandomDelay
        GateCheckState,         // Pandora.CheckReloginState(int)
        GateReloginCallback,    // Pandora.ReloginCallback()
        GateLogout,             // Pandora.Logout(isRelogin = true) only
        GateLoadCallback,       // LoadingFlow...OnNotifyNeedReloginCallback()
        GateCount
    };

    inline bool Enabled()
    {
        return Config.ExtraMenu.AntiLogin;
    }

    // Install-time truth + blocked-kick count, one slot per gate (same shape
    // as ReportBlock.h's landed bits; Feats.h fills the landed flags right
    // after install). g_lastState keeps the most recent relogin state the
    // server reported, in case the state machine ever needs a closer look.
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

    // -- the socket layer's relogin executor --------------------------------
    // dump.cs, BrokerSocketManager @368649: private ref object
    // ExecuteReloginByRandomDelay() - a coroutine iterator, so the native
    // shape is an object pointer out. Returning null means no iterator, no
    // delayed relogin, nothing for the heartbeat verdict to act through.
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

    // -- the account layer's relogin-state verdict ---------------------------
    // dump.cs, Pandora @648015: private void CheckReloginState(int state).
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

    // -- the popup / logout callback ------------------------------------------
    // dump.cs, Pandora @648015: private void ReloginCallback().
    inline void (*orig_ReloginCallback)(void *instance) = nullptr;
    inline void hook_ReloginCallback(void *instance)
    {
        if (Enabled()) {
            NoteBlocked(GateReloginCallback);
            return;
        }
        if (orig_ReloginCallback != nullptr) orig_ReloginCallback(instance);
    }

    // -- the forced logout ------------------------------------------------------
    // dump.cs, Pandora @648015: public void Logout(bool isRelogin = false).
    // Only the forced form is swallowed; isRelogin = false is the player's
    // own logout from the account menu and must always reach the original.
    inline void (*orig_Logout)(void *instance, bool isRelogin) = nullptr;
    inline void hook_Logout(void *instance, bool isRelogin)
    {
        if (Enabled() && isRelogin) {
            NoteBlocked(GateLogout);
            return;
        }
        if (orig_Logout != nullptr) orig_Logout(instance, isRelogin);
    }

    // -- the loading flow's relogin callback -------------------------------------
    // dump.cs, LoadingFlowCheckPayTokenController @1397882: private void
    // OnNotifyNeedReloginCallback().
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
