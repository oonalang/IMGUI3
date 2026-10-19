#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

// Report blocking: player-report pull + ACE evidence scrub.
// Both gated by the Account & Data switches.

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
        GateAceGetData,         // AnoSDK.GetReportData()
        GateAceGetData2,        // AnoSDK.GetReportData2()
        GateAceGetData3,        // AnoSDK.GetReportData3()
        GateAceGetData4,        // AnoSDK.GetReportData4(uint token)
        GateCount
    };

    inline bool g_landed[GateCount] = {};

    // Live counters for the menu: pulls seen, payload fills dropped, sends
    // dropped. Game thread writes, render thread reads - plain ints, racy but
    // harmless (same convention as the triggerbot diagnostics).
    inline int g_pullsSeen = 0;
    inline int g_payloadsDropped = 0;
    inline int g_sendsDropped = 0;

    // ACE side, same convention: blobs the anti-cheat handed the game while
    // the switch was on, and how many of their bytes were emptied.
    inline int g_aceBlobsScrubbed = 0;
    inline int g_aceBytesScrubbed = 0;

    // A byte[] handed back by the managed wrapper. The IL2CPP object header is
    // klass 0x00 / monitor 0x08 / bounds 0x10 / max_length 0x18, and the first
    // element always sits at 0x20 (kIl2CppSizeOfArray, 8-byte aligned) even for
    // a byte[] - hence the explicit pad instead of a flexible member, which the
    // compiler would place at 0x1C.
    struct AceByteArray {
        void *klass;
        void *monitor;
        void *bounds;
        int max_length;
        int pad;
        uint8_t data[];
    };

    // Never memset more than this from a length the game hands us.
    constexpr int MaxAceBlobBytes = 1 << 20; // 1 MiB

    // Zero the CONTENTS of the blob, keeping the object and its length intact.
    // A null / unreadable result is returned untouched: the gate must never
    // invent an array, and it must never turn a working call into a crash.
    inline void *ScrubAceBlob(void *arrayObj)
    {
        if (!Config.ExtraMenu.BlockAceReport)
            return arrayObj;
        if (arrayObj == nullptr || !Tools::IsPtrValid(arrayObj))
            return arrayObj;

        g_aceBlobsScrubbed++;

        AceByteArray *blob = (AceByteArray *)arrayObj;
        int len = blob->max_length;
        if (len <= 0)
            return arrayObj;
        if (len > MaxAceBlobBytes)
            len = MaxAceBlobBytes;

        memset(blob->data, 0, (size_t)len);
        g_aceBytesScrubbed += len;
        return arrayObj;
    }

    // The four managed getters. One shape each: ask the original, then empty
    // what it produced. Static methods - no this pointer.
    inline void *(*orig_AceGetReportData)(void) = nullptr;
    inline void *hook_AceGetReportData(void)
    {
        void *blob = orig_AceGetReportData ? orig_AceGetReportData() : nullptr;
        return ScrubAceBlob(blob);
    }

    inline void *(*orig_AceGetReportData2)(void) = nullptr;
    inline void *hook_AceGetReportData2(void)
    {
        void *blob = orig_AceGetReportData2 ? orig_AceGetReportData2() : nullptr;
        return ScrubAceBlob(blob);
    }

    inline void *(*orig_AceGetReportData3)(void) = nullptr;
    inline void *hook_AceGetReportData3(void)
    {
        void *blob = orig_AceGetReportData3 ? orig_AceGetReportData3() : nullptr;
        return ScrubAceBlob(blob);
    }

    // GetReportData4(uint token): the token is forwarded untouched, it selects
    // which report ACE is being asked for.
    inline void *(*orig_AceGetReportData4)(uint32_t token) = nullptr;
    inline void *hook_AceGetReportData4(uint32_t token)
    {
        void *blob = orig_AceGetReportData4 ? orig_AceGetReportData4(token) : nullptr;
        return ScrubAceBlob(blob);
    }

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

        // The anti-cheat's own evidence, at the four managed wrappers the game
        // obtains it through (dump.cs AnoSDK, see the file head). Independent
        // of Config.ExtraMenu.BlockReportEvidence - either switch works alone.
        HOOK_LIB("libunity.so", "0x7F5D8B8", hook_AceGetReportData,  orig_AceGetReportData);
        HOOK_LIB("libunity.so", "0x7F5DD90", hook_AceGetReportData2, orig_AceGetReportData2);
        HOOK_LIB("libunity.so", "0x7F5DF98", hook_AceGetReportData3, orig_AceGetReportData3);
        HOOK_LIB("libunity.so", "0x7F5E45C", hook_AceGetReportData4, orig_AceGetReportData4);
    }
}
