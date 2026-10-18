#pragma once

// ============================================================================
//  BlockReportEvidence.h  -  "Block Report Evidence": keep YOUR client from
//  sending evidence when someone reports you.
//
//  Four hook points, straight from the dump:
//
//   1. ReportWindowController.DoReport()                       RVA 0xB23D358
//   2. PlayerInfoReportDataStore.DoReportPlayer(string[])       RVA 0x8FB4B60
//   3. C2S_REQ_REPORT_PLAYER_PULL.BeginReportPlayerPull(...)    RVA 0x5620BAC
//   4. C2S_REQ_REPORT_PLAYER_PULL.EndReportPlayerPull(...)      RVA 0x5620F9C
//
//  Config: Config.ExtraMenu.BlockReportEvidence (default ON). Off restores
//  stock behaviour exactly (every body falls back to its original).
// ============================================================================

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace block_report_evidence
{
    namespace Rva
    {
        constexpr uintptr_t DoReport         = 0xB23D358;
        constexpr uintptr_t DoReportPlayer   = 0x8FB4B60;
        constexpr uintptr_t BeginReport       = 0x5620BAC;
        constexpr uintptr_t EndReport         = 0x5620F9C;

        // Legacy aliases (old ReportBlock.h naming).
        constexpr uintptr_t ReportWindow_DoReport       = DoReport;
        constexpr uintptr_t GVoiceSystem_DoReportPlayer = DoReportPlayer;
        constexpr uintptr_t Net_BeginReportPlayerPull   = BeginReport;
        constexpr uintptr_t Net_EndReportPlayerPull     = EndReport;
    }

    inline bool Enabled()
    {
        // Both the legacy (BlockReport) and the new (BlockReportEvidence)
        // switches must be on: a config written by either generation keeps
        // working, and turning either off restores stock behaviour.
        return Config.ExtraMenu.BlockReportEvidence && Config.ExtraMenu.BlockReport;
    }

    enum Gate
    {
        GateDoReport = 0,
        GateDoReportPlayer,
        GateBeginReport,
        GateEndReport,
        GateCount
    };

    // Legacy gate aliases.
    constexpr Gate GateVoice  = GateDoReportPlayer;
    constexpr Gate GateBegin  = GateBeginReport;
    constexpr Gate GateEnd    = GateEndReport;

    struct DiagStats
    {
        int asked[GateCount];
        int granted[GateCount];
        int seen[GateCount];
        int blocked[GateCount];
    };
    inline DiagStats g_diag{};

    inline bool g_landed[GateCount];
    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void NoteAsk(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) {
            g_diag.asked[gate]++;
            g_diag.seen[gate]++;
        }
    }

    inline void NoteGrant(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) {
            g_diag.granted[gate]++;
            g_diag.blocked[gate]++;
        }
    }

    // Legacy helper: record a call and say whether it must be swallowed.
    inline bool Block(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) {
            g_diag.seen[gate]++;
            g_diag.asked[gate]++;
            if (Enabled()) {
                g_diag.blocked[gate]++;
                g_diag.granted[gate]++;
                return true;
            }
        }
        return false;
    }
}

namespace report_evidence = block_report_evidence;
namespace block_report = block_report_evidence;
namespace report_block = block_report_evidence;

// ---- original pointers ----------------------------------------------------
// Canonical storage for the begin/end pointers uses the legacy names so that
// both old and new callers share one slot.
inline void (*orig_DoReport)(void *ins) = nullptr;
inline int (*orig_DoReportPlayer)(void *ins, void *openids) = nullptr;
inline void *(*orig_BeginReportPlayerPull)(int pullType, uint8_t moveState, void *clientPos,
                                          void *serverPos, int hitCount, uint8_t lineHitCount,
                                          void *hitPosition) = nullptr;
inline void (*orig_EndReportPlayerPull)(void *request) = nullptr;

// New-style aliases that refer to the same storage.
inline void *(*&orig_BeginReport)(int pullType, uint8_t moveState, void *clientPos,
                                  void *serverPos, int hitCount, uint8_t lineHitCount,
                                  void *hitPosition) = orig_BeginReportPlayerPull;
inline void (*&orig_EndReport)(void *request) = orig_EndReportPlayerPull;

// ---- 1. ReportWindowController.DoReport ------------------------------------
inline void hook_DoReport(void *ins)
{
    block_report_evidence::NoteAsk(block_report_evidence::GateDoReport);
    if (ins != nullptr && block_report_evidence::Enabled()) {
        block_report_evidence::NoteGrant(block_report_evidence::GateDoReport);
        return;
    }
    if (orig_DoReport != nullptr)
        orig_DoReport(ins);
}

// ---- 2. PlayerInfoReportDataStore.DoReportPlayer ----------------------------
inline int hook_DoReportPlayer(void *ins, void *openids)
{
    block_report_evidence::NoteAsk(block_report_evidence::GateDoReportPlayer);
    if (ins != nullptr && block_report_evidence::Enabled()) {
        block_report_evidence::NoteGrant(block_report_evidence::GateDoReportPlayer);
        return 0;
    }
    if (orig_DoReportPlayer != nullptr)
        return orig_DoReportPlayer(ins, openids);
    return 0;
}

// ---- 3. C2S_REQ_REPORT_PLAYER_PULL.BeginReportPlayerPull --------------------
inline void *hook_BeginReport(int pullType, uint8_t moveState, void *clientPos,
                             void *serverPos, int hitCount, uint8_t lineHitCount,
                             void *hitPosition)
{
    block_report_evidence::NoteAsk(block_report_evidence::GateBeginReport);
    if (block_report_evidence::Enabled()) {
        block_report_evidence::NoteGrant(block_report_evidence::GateBeginReport);
        return nullptr;
    }
    if (orig_BeginReport != nullptr)
        return orig_BeginReport(pullType, moveState, clientPos, serverPos, hitCount,
                                lineHitCount, hitPosition);
    return nullptr;
}

// ---- 4. C2S_REQ_REPORT_PLAYER_PULL.EndReportPlayerPull ----------------------
inline void hook_EndReport(void *request)
{
    block_report_evidence::NoteAsk(block_report_evidence::GateEndReport);
    if (request != nullptr && block_report_evidence::Enabled()) {
        block_report_evidence::NoteGrant(block_report_evidence::GateEndReport);
        return;
    }
    if (orig_EndReport != nullptr)
        orig_EndReport(request);
}

// ---- legacy (old ReportBlock.h) function names ----------------------------
inline void DoReport(void *ins) { hook_DoReport(ins); }
inline int DoReportPlayer(void *ins, void *openids) { return hook_DoReportPlayer(ins, openids); }
inline void *BeginReportPlayerPull(int pullType, uint8_t moveState, void *clientPos,
                                  void *serverPos, int hitCount, uint8_t lineHitCount,
                                  void *hitPosition)
{
    return hook_BeginReport(pullType, moveState, clientPos, serverPos, hitCount,
                            lineHitCount, hitPosition);
}
inline void EndReportPlayerPull(void *request) { hook_EndReport(request); }
