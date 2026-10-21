#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace report_block
{
    enum Gate
    {
        GateDoReport = 0,
        GateDoReportPlayer,
        GateBeginReport,
        GateFillReportData,
        GateEndReport,
        GateAceGetData,
        GateAceGetData2,
        GateAceGetData3,
        GateAceGetData4,
        GateCount
    };

    inline bool g_landed[GateCount] = {};

    inline int g_pullsSeen = 0;
    inline int g_payloadsDropped = 0;
    inline int g_sendsDropped = 0;

    inline int g_aceBlobsScrubbed = 0;
    inline int g_aceBytesScrubbed = 0;

    struct AceByteArray {
        void *klass;
        void *monitor;
        void *bounds;
        int max_length;
        int pad;
        uint8_t data[];
    };

    constexpr int MaxAceBlobBytes = 1 << 20;

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
    }

    inline int (*orig_DoReportPlayer)(void *instance, void *openids) = nullptr;
    inline int hook_DoReportPlayer(void *instance, void *openids)
    {
        if (!Config.ExtraMenu.BlockReportEvidence)
        {
            if (orig_DoReportPlayer) return orig_DoReportPlayer(instance, openids);
            return 0;
        }
        return 0;
    }

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

    inline void InstallReportBlockHooks()
    {
        HOOK_LIB("libunity.so", "0xB23D358", hook_DoReport, orig_DoReport);
        HOOK_LIB("libunity.so", "0x8FB4B60", hook_DoReportPlayer, orig_DoReportPlayer);
        HOOK_LIB("libunity.so", "0x5620BAC", hook_BeginReportPlayerPull, orig_BeginReportPlayerPull);
        HOOK_LIB("libunity.so", "0x58EA8E8", hook_FillPlayerFullData, orig_FillPlayerFullData);
        HOOK_LIB("libunity.so", "0x5620F9C", hook_EndReportPlayerPull, orig_EndReportPlayerPull);

        HOOK_LIB("libunity.so", "0x7F5D8B8", hook_AceGetReportData,  orig_AceGetReportData);
        HOOK_LIB("libunity.so", "0x7F5DD90", hook_AceGetReportData2, orig_AceGetReportData2);
        HOOK_LIB("libunity.so", "0x7F5DF98", hook_AceGetReportData3, orig_AceGetReportData3);
        HOOK_LIB("libunity.so", "0x7F5E45C", hook_AceGetReportData4, orig_AceGetReportData4);
    }
}

