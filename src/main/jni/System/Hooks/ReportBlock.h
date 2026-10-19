#pragma once

// ---------------------------------------------------------------------------
// Block Report Evidence.
//
// When another player reports you, the reporter's client sends the report and
// the match server may ask YOUR client for its side of the story: the movement
// record, positions, hit lines. This feature removes this client's cooperation
// with that pull. It cannot withdraw the report itself (that packet belongs to
// the reporter) - it makes sure no evidence leaves this device.
//
// Five points on the client's report path, every RVA re-checked against the
// dumps (dump.cs GR .57):
//
//   0xB23D358  ReportWindowController.DoReport()      - report screen confirm
//   0x8FB4B60  VoiceSystem.DoReportPlayer(string[])   - voice report send
//   0x5620BAC  BeginReportPlayerPull(...)             - pull request built
//   0x58EA8E8  LocalPlayerMoveChecker.FillPlayerFullData(request)
//                                                     - evidence payload filled
//   0x5620F9C  EndReportPlayerPull(request)           - evidence sent
//
// WHAT CHANGED AND WHY
//
// The old version returned nullptr from BeginReportPlayerPull when the switch
// was on. That handed a null request to game code that then fills and sends
// it, i.e. the block itself was a null-dereference waiting for the first
// player who reports you. Begin now always builds the real request (the caller
// owns that pointer), and the evidence is killed at its two useful points
// instead:
//
//   * FillPlayerFullData() records the movement/position/hit evidence into the
//     request - swallowed, so the request stays valid but empty.
//   * EndReportPlayerPull() is the only place the C2S_REQ_REPORT_PLAYER_PULL
//     packet actually leaves the client - swallowed, so nothing is sent.
//     (The pooled request object is left allocated; one small object per
//     report is a cheaper failure mode than a crash or a sent packet.)
//
// Every hook is gated by Config.ExtraMenu.BlockReportEvidence at the TOP of
// its body, so the work done when the toggle is off is one boolean read and
// one original call. The counters under the switch show pulls seen and
// payloads/sends dropped - i.e. proof the block is alive.
// ---------------------------------------------------------------------------

namespace report_block
{
    // Install-time/ask diagnostics, same shape as MythicAttachments.h: the HOME
    // tab's diagnostics line reads g_landed to show which RVAs actually took.
    enum Gate
    {
        GateDoReport = 0,       // ReportWindowController.DoReport
        GateDoReportPlayer,     // VoiceSystem.DoReportPlayer(string[] openids)
        GateBeginReport,        // BeginReportPlayerPull(...)
        GateFillReportData,     // LocalPlayerMoveChecker.FillPlayerFullData(request)
        GateEndReport,          // EndReportPlayerPull(request)
        GateCount
    };

    inline bool g_landed[GateCount] = {};

    // Live counters for the menu: pulls seen, payload fills dropped, sends
    // dropped. Game thread writes, render thread reads - plain ints, racy but
    // harmless (same convention as the triggerbot diagnostics).
    inline int g_pullsSeen = 0;
    inline int g_payloadsDropped = 0;
    inline int g_sendsDropped = 0;

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void (*orig_DoReport)(void *instance) = nullptr;
    inline void hook_DoReport(void *instance)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_DoReport) orig_DoReport(instance);
            return;
        }
        // Reported-you evidence: not built.
    }

    inline int (*orig_DoReportPlayer)(void *instance, void *openids) = nullptr;
    inline int hook_DoReportPlayer(void *instance, void *openids)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_DoReportPlayer) return orig_DoReportPlayer(instance, openids);
            return 0;
        }
        return 0; // "handled, nothing sent"
    }

    // Always build the request through the original: the caller keeps that
    // pointer and passes it to Fill/End, so a nullptr here would crash game
    // code the moment somebody reports you. The pull is only counted.
    inline void *(*orig_BeginReportPlayerPull)(int pullType, uint8_t moveState,
        void *clientPos, void *serverPos, int hitCount, uint8_t lineHitCount,
        void *hitPosition) = nullptr;
    inline void *hook_BeginReportPlayerPull(int pullType, uint8_t moveState,
        void *clientPos, void *serverPos, int hitCount, uint8_t lineHitCount,
        void *hitPosition)
    {
        if (orig_BeginReportPlayerPull)
        {
            void *request = orig_BeginReportPlayerPull(pullType, moveState, clientPos,
                serverPos, hitCount, lineHitCount, hitPosition);
            if (Config.ExtraMenu.BlockReportEvidence)
                g_pullsSeen++;
            return request;
        }
        return nullptr;
    }

    // LocalPlayerMoveChecker.FillPlayerFullData(C2S_REQ_REPORT_PLAYER_PULL):
    // this is where this client's movement record / positions / hit lines are
    // written into the request. Swallowed while the switch is on, so the
    // request stays valid but carries no evidence at all.
    inline void (*orig_FillPlayerFullData)(void *instance, void *request) = nullptr;
    inline void hook_FillPlayerFullData(void *instance, void *request)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_FillPlayerFullData) orig_FillPlayerFullData(instance, request);
            return;
        }
        g_payloadsDropped++;
    }

    // EndReportPlayerPull(C2S_REQ_REPORT_PLAYER_PULL): the send. This is the
    // gate that matters - with it swallowed the packet never reaches the
    // server, whatever the rest of the path did.
    inline void (*orig_EndReportPlayerPull)(void *request) = nullptr;
    inline void hook_EndReportPlayerPull(void *request)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_EndReportPlayerPull) orig_EndReportPlayerPull(request);
            return;
        }
        g_sendsDropped++;
    }

    // InstallFeatsHooks() calls this once with the resolved unity handle in
    // scope. Every gate lands even when it answers nothing, because HookLib
    // fills the original pointer on success - the same convention Feats.h uses
    // for its own landing notes.
    inline void InstallReportBlockHooks()
    {
        HOOK_LIB("libunity.so", "0xB23D358", hook_DoReport, orig_DoReport);
        HOOK_LIB("libunity.so", "0x8FB4B60", hook_DoReportPlayer, orig_DoReportPlayer);
        HOOK_LIB("libunity.so", "0x5620BAC", hook_BeginReportPlayerPull, orig_BeginReportPlayerPull);
        HOOK_LIB("libunity.so", "0x58EA8E8", hook_FillPlayerFullData, orig_FillPlayerFullData);
        HOOK_LIB("libunity.so", "0x5620F9C", hook_EndReportPlayerPull, orig_EndReportPlayerPull);
    }
}
