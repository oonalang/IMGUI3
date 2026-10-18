#pragma once

// ---------------------------------------------------------------------------
// Block Report Evidence.
//
// YOUR client is blocked from BUILDING and SENDING report evidence when another
// player reports you through the in-game report screen. Four points on the
// client's report path, all RVA-verified against the current dump:
//
//   0xB23D358  ReportPanel.DoReport()                 - the screen's confirm
//   0x8FB4B60  DoReportPlayer(string[] openids)      - the gameplay-layer send
//   0x5620BAC  BeginReportPlayerPull(...)            - evidence collection begins
//   0x5620F9C  EndReportPlayerPull(request)          - evidence is committed
//
// Each hook simply swallows the call instead of answering the stock way: an
// evidence pull that never builds, a report that never sends. The hooks are
// gated by Config.ExtraMenu.BlockReportEvidence at the TOP of each body, so
// the work done when the toggle is off is one boolean read.
// ---------------------------------------------------------------------------

namespace report_block
{
    // Install-time/ask diagnostics, same shape as MythicAttachments.h: the HOME
    // tab's diagnostics line reads g_landed to show which RVAs actually took.
    enum Gate
    {
        GateDoReport = 0,       // ReportPanel.DoReport
        GateDoReportPlayer,     // DoReportPlayer(string[] openids)
        GateBeginReport,        // BeginReportPlayerPull(...)
        GateEndReport,          // EndReportPlayerPull(request)
        GateCount
    };

    inline bool g_landed[GateCount] = {};
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

    inline void *(*orig_BeginReportPlayerPull)(int pullType, uint8_t moveState,
        void *clientPos, void *serverPos, int hitCount, uint8_t lineHitCount,
        void *hitPosition) = nullptr;
    inline void *hook_BeginReportPlayerPull(int pullType, uint8_t moveState,
        void *clientPos, void *serverPos, int hitCount, uint8_t lineHitCount,
        void *hitPosition)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_BeginReportPlayerPull)
                return orig_BeginReportPlayerPull(pullType, moveState, clientPos,
                    serverPos, hitCount, lineHitCount, hitPosition);
            return nullptr;
        }
        return nullptr; // no evidence object -> nothing for EndReport to commit
    }

    inline void (*orig_EndReportPlayerPull)(void *request) = nullptr;
    inline void hook_EndReportPlayerPull(void *request)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_EndReportPlayerPull) orig_EndReportPlayerPull(request);
            return;
        }
        // With BeginReport swallowed there is never a request here anyway.
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
        HOOK_LIB("libunity.so", "0x5620F9C", hook_EndReportPlayerPull, orig_EndReportPlayerPull);
    }
}
