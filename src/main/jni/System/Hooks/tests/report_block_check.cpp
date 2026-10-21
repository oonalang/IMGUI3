#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "System/Hooks/ReportBlock.h"

bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }
extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }
void MSHookFunction(void *, void *, void **) {}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int g_doReport = 0, g_doReportPlayer = 0, g_begin = 0, g_fill = 0, g_end = 0;
static void fakeDoReport(void *) { g_doReport++; }
static int  fakeDoReportPlayer(void *, void *) { g_doReportPlayer++; return 7; }
static void *fakeBegin(int, uint8_t, void *, void *, int, uint8_t, void *) { g_begin++; return (void *)0x1234; }
static void fakeFill(void *, void *) { g_fill++; }
static void fakeEnd(void *) { g_end++; }

int main()
{
    report_block::orig_DoReport             = fakeDoReport;
    report_block::orig_DoReportPlayer       = fakeDoReportPlayer;
    report_block::orig_BeginReportPlayerPull = fakeBegin;
    report_block::orig_FillPlayerFullData   = fakeFill;
    report_block::orig_EndReportPlayerPull  = fakeEnd;

    CHECK(Config.ExtraMenu.BlockReportEvidence == true);

    Config.ExtraMenu.BlockReportEvidence = true;
    report_block::hook_DoReport((void *)1);
    CHECK(report_block::hook_DoReportPlayer((void *)1, nullptr) == 0);
    CHECK(report_block::hook_BeginReportPlayerPull(0, 0, nullptr, nullptr, 0, 0, nullptr) == (void *)0x1234);
    CHECK(g_begin == 1);
    CHECK(report_block::g_pullsSeen == 1);
    report_block::hook_FillPlayerFullData((void *)1, (void *)0x1234);
    report_block::hook_EndReportPlayerPull((void *)0x1234);
    CHECK(g_doReport == 0 && g_doReportPlayer == 0);
    CHECK(g_fill == 0 && g_end == 0);
    CHECK(report_block::g_payloadsDropped == 1 && report_block::g_sendsDropped == 1);

    Config.ExtraMenu.BlockReportEvidence = false;
    report_block::hook_DoReport((void *)1);
    CHECK(report_block::hook_DoReportPlayer((void *)1, nullptr) == 7);
    CHECK(report_block::hook_BeginReportPlayerPull(0, 0, nullptr, nullptr, 0, 0, nullptr) == (void *)0x1234);
    report_block::hook_FillPlayerFullData((void *)1, (void *)0x1234);
    report_block::hook_EndReportPlayerPull((void *)0x1234);
    CHECK(g_doReport == 1 && g_doReportPlayer == 1 && g_begin == 2);
    CHECK(g_fill == 1 && g_end == 1);
    CHECK(report_block::g_pullsSeen == 1);

    report_block::orig_BeginReportPlayerPull = nullptr;
    CHECK(report_block::hook_BeginReportPlayerPull(0, 0, nullptr, nullptr, 0, 0, nullptr) == nullptr);

    report_block::NoteLanded(report_block::GateDoReport, true);
    report_block::NoteLanded(report_block::GateFillReportData, true);
    CHECK(report_block::g_landed[report_block::GateDoReport]);
    CHECK(report_block::g_landed[report_block::GateFillReportData]);
    report_block::NoteLanded((report_block::Gate)99, true);
    CHECK(report_block::g_landed[report_block::GateCount - 1] == false);

    if (!fails) { std::printf("OK: report_block checks passed\n"); return 0; }
    std::printf("FAILED: %d\n", fails);
    return 1;
}

