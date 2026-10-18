#pragma once

//  ReportBlock.h  -  "Block Report Evidence": YOUR client never uploads the
//  report / evidence payload when someone reports you.
//
//  The four client-side senders, straight from dump.cs (RVA / signature):
//
//   1. ReportWindowController.DoReport()                       RVA 0xB23D358
//        dump.cs 2145536  `private void DoReport()`
//        the report window's confirm handler - builds and fires the report
//        request for the players the reporter ticked.
//   2. GVoiceSystem.DoReportPlayer(string[] openids)           RVA 0x8FB4B60
//        dump.cs 1213875  `public int DoReportPlayer(string[] openids)`
//        the voice-abuse report path; returns an int status, so the blocked
//        answer is 0 (nothing scheduled).
//   3. NetworkManager.BeginReportPlayerPull(int pullType, byte moveState,
//      in object clientPos, in object serverPos, int hitCount,
//      byte lineHitCount, in object hitPosition)               RVA 0x5620BAC
//        dump.cs 468667  `public static C2S_REQ_REPORT_PLAYER_PULL
//                          BeginReportPlayerPull(...)`
//        the evidence payload itself - kills / hit positions / movement state
//        are serialised into the C2S_REQ_REPORT_PLAYER_PULL message here. It
//        returns the request object, so the blocked answer is nullptr.
//   4. NetworkManager.EndReportPlayerPull(C2S_REQ_REPORT_PLAYER_PULL request)
//                                                               RVA 0x5620F9C
//        dump.cs 468670  `public static void EndReportPlayerPull(request)`
//        the send half; without it the request built above is never flushed.
//
//
//  GATING: Config.ExtraMenu.BlockReport, default ON, one row in the SETTINGS
//  tab. While it is OFF every hook calls the game's original function with the
//  caller's own arguments untouched, so stock reporting is restored exactly.
//  A null original (install failed) makes the hook fail OPEN: the game's own
//  function was never replaced in that case, so nothing here can run.
//
//  Include contract: uses sConfig Config, so it is included after the game
//  structs (Hacks/Hacks.h -> Feature/Feature.h -> Init_Hacks.h), the same
//  contract Feats.h has. It includes Init_Hacks.h so a test TU can pull just
//  this file after the structs.

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace report_block
{
    // The four RVAs, in the dump's own convention (what getAbsoluteAddress()
    // wants).
    namespace Rva
    {
        constexpr uintptr_t ReportWindow_DoReport       = 0xB23D358; // dump.cs 2145537
        constexpr uintptr_t GVoiceSystem_DoReportPlayer = 0x8FB4B60; // dump.cs 1213876
        constexpr uintptr_t Net_BeginReportPlayerPull   = 0x5620BAC; // dump.cs 468667
        constexpr uintptr_t Net_EndReportPlayerPull     = 0x5620F9C; // dump.cs 468670
    }

    // The one switch, and the only thing anything here reads.
    inline bool Enabled()
    {
        return Config.ExtraMenu.BlockReport;
    }

    enum Gate
    {
        GateDoReport = 0,   // ReportWindowController.DoReport
        GateVoice,          // GVoiceSystem.DoReportPlayer
        GateBegin,          // NetworkManager.BeginReportPlayerPull
        GateEnd,            // NetworkManager.EndReportPlayerPull
        GateCount
    };

    // On-device instrument. The SETTINGS tab prints these, which is how the
    // feature is confirmed on a phone: "seen" counts the calls the game made,
    // "blocked" counts the ones this feature swallowed.
    struct DiagStats
    {
        int seen[GateCount];
        int blocked[GateCount];
    };
    inline DiagStats g_diag{};

    // Install-time truth, one flag per gate: Feats.h sets it right after the
    // hook call from whether the hooker filled in the original pointer. A 0
    // here is the reason that gate's counter stays at 0/0 - the address never
    // took the hook (a dump that no longer matches the installed game).
    inline bool g_landed[GateCount];

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    // Record one call, and say whether it must be swallowed. A single helper so
    // the four bodies cannot drift apart in how they answer.
    inline bool Block(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) {
            g_diag.seen[gate]++;
            if (Enabled()) {
                g_diag.blocked[gate]++;
                return true;
            }
        }
        return false;
    }
}

// ---- 1. the report window's confirm handler ------------------------------
// dump.cs 2145537: private void DoReport(). An instance method, so the
// replacement takes (this) and hands it straight back when the toggle is off.
inline void (*orig_DoReport)(void *instance) = nullptr;
inline void DoReport(void *instance)
{
    if (report_block::Block(report_block::GateDoReport))
        return;
    if (orig_DoReport != nullptr)
        orig_DoReport(instance);
}

// ---- 2. the voice report path -------------------------------------------
// dump.cs 1213876: public int DoReportPlayer(string[] openids). Blocked answer
// is 0 - the "nothing was scheduled" status the caller already handles.
inline int (*orig_DoReportPlayer)(void *instance, void *openids) = nullptr;
inline int DoReportPlayer(void *instance, void *openids)
{
    if (report_block::Block(report_block::GateVoice))
        return 0;
    return orig_DoReportPlayer != nullptr
         ? orig_DoReportPlayer(instance, openids)
         : 0;
}

// ---- 3. the evidence payload builder ------------------------------------
// dump.cs 468667: public static C2S_REQ_REPORT_PLAYER_PULL
// BeginReportPlayerPull(int pullType, byte moveState, in object clientPos,
// in object serverPos, int hitCount, byte lineHitCount, in object hitPosition).
// Every argument travels through unchanged when the toggle is off; the blocked
// answer is nullptr, i.e. no request object for EndReportPlayerPull to send.
inline void *(*orig_BeginReportPlayerPull)(int pullType, uint8_t moveState,
                                           void *clientPos, void *serverPos,
                                           int hitCount, uint8_t lineHitCount,
                                           void *hitPosition) = nullptr;
inline void *BeginReportPlayerPull(int pullType, uint8_t moveState,
                                   void *clientPos, void *serverPos,
                                   int hitCount, uint8_t lineHitCount,
                                   void *hitPosition)
{
    if (report_block::Block(report_block::GateBegin))
        return nullptr;
    return orig_BeginReportPlayerPull != nullptr
         ? orig_BeginReportPlayerPull(pullType, moveState, clientPos, serverPos,
                                      hitCount, lineHitCount, hitPosition)
         : nullptr;
}

// ---- 4. the send half ---------------------------------------------------
// dump.cs 468670: public static void EndReportPlayerPull(request).
inline void (*orig_EndReportPlayerPull)(void *request) = nullptr;
inline void EndReportPlayerPull(void *request)
{
    if (report_block::Block(report_block::GateEnd))
        return;
    if (orig_EndReportPlayerPull != nullptr)
        orig_EndReportPlayerPull(request);
}
