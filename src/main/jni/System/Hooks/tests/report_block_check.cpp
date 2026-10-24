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

static int g_doReport = 0, g_doReportPlayer = 0, g_begin = 0, g_end = 0, g_fill = 0;
static void fakeDoReport(void *) { g_doReport++; }
static int  fakeDoReportPlayer(void *, void *) { g_doReportPlayer++; return 7; }
static void *fakeBegin(int, uint8_t, void *, void *, int, uint8_t, void *) { g_begin++; return (void *)0x1234; }
static void fakeEnd(void *) { g_end++; }
static void fakeFill(void *, void *) { g_fill++; }

static void resetCounters()
{
    g_doReport = g_doReportPlayer = g_begin = g_end = g_fill = 0;
    report_block::g_pullsSeen = 0;
    report_block::g_payloadsDropped = 0;
    report_block::g_sendsDropped = 0;
}

static void armOriginals()
{
    orig_DoReport            = fakeDoReport;
    orig_DoReportPlayer      = fakeDoReportPlayer;
    orig_BeginReport         = fakeBegin;
    orig_EndReport           = fakeEnd;
    report_block::orig_FillPlayerFullData = fakeFill;
}

int main()
{
    armOriginals();
    resetCounters();

    CHECK(Config.ExtraMenu.BlockReportEvidence == true);

    hook_DoReport((void *)1);
    CHECK(hook_DoReportPlayer((void *)1, nullptr) == 0);
    CHECK(hook_BeginReport(0, 0, nullptr, nullptr, 0, 0, nullptr) == nullptr);
    report_block::hook_FillPlayerFullData((void *)1, (void *)0x1234);
    hook_EndReport((void *)0x1234);

    CHECK(g_doReport == 0);
    CHECK(g_doReportPlayer == 0);
    CHECK(g_begin == 0);
    CHECK(g_end == 0);
    CHECK(g_fill == 0);
    CHECK(report_block::g_pullsSeen == 1);
    CHECK(report_block::g_payloadsDropped == 1);
    CHECK(report_block::g_sendsDropped == 1);

    hook_BeginReport(2, 1, (void *)0x10, (void *)0x20, 9, 3, (void *)0x30);
    hook_DoReport((void *)2);
    CHECK(hook_DoReportPlayer((void *)2, (void *)0x40) == 0);
    report_block::hook_FillPlayerFullData((void *)2, (void *)0x5678);
    hook_EndReport((void *)0x1234);
    CHECK(g_begin == 0 && g_doReport == 0 && g_doReportPlayer == 0 && g_end == 0 && g_fill == 0);
    CHECK(report_block::g_pullsSeen == 2 && report_block::g_sendsDropped == 2);

    orig_DoReport       = nullptr;
    orig_DoReportPlayer = nullptr;
    orig_BeginReport    = nullptr;
    orig_EndReport      = nullptr;
    report_block::orig_FillPlayerFullData = nullptr;
    hook_DoReport((void *)3);
    CHECK(hook_DoReportPlayer((void *)3, nullptr) == 0);
    CHECK(hook_BeginReport(0, 0, nullptr, nullptr, 0, 0, nullptr) == nullptr);
    report_block::hook_FillPlayerFullData((void *)3, nullptr);
    hook_EndReport(nullptr);
    CHECK(g_doReportPlayer == 0 && g_begin == 0 && g_end == 0 && g_fill == 0);

    armOriginals();
    resetCounters();
    Config.ExtraMenu.BlockReportEvidence = false;

    hook_DoReport((void *)4);
    CHECK(hook_DoReportPlayer((void *)4, (void *)0x40) == 7);
    CHECK(hook_BeginReport(1, 0, nullptr, nullptr, 0, 0, nullptr) == (void *)0x1234);
    report_block::hook_FillPlayerFullData((void *)4, (void *)0x9ABC);
    hook_EndReport((void *)0x1234);

    CHECK(g_doReport == 1 && g_doReportPlayer == 1);
    CHECK(g_begin == 1 && g_end == 1 && g_fill == 1);
    CHECK(report_block::g_pullsSeen == 0);
    CHECK(report_block::g_payloadsDropped == 0);
    CHECK(report_block::g_sendsDropped == 0);

    orig_BeginReport = nullptr;
    CHECK(hook_BeginReport(0, 0, nullptr, nullptr, 0, 0, nullptr) == nullptr);
    orig_DoReportPlayer = nullptr;
    CHECK(hook_DoReportPlayer((void *)5, nullptr) == 0);

    Config.ExtraMenu.BlockReportEvidence = true;

    report_block::NoteLanded(report_block::GateDoReport, true);
    report_block::NoteLanded(report_block::GateEndReport, true);
    CHECK(report_block::g_landed[report_block::GateDoReport]);
    CHECK(report_block::g_landed[report_block::GateEndReport]);
    CHECK(!report_block::g_landed[report_block::GateBeginReport]);

    report_block::NoteLanded((report_block::Gate)99, true);
    report_block::NoteLanded((report_block::Gate)-4, true);
    CHECK(!report_block::g_landed[report_block::GateDoReportPlayer]);
    CHECK(!report_block::g_landed[report_block::GateBeginReport]);
    CHECK(report_block::g_landed[report_block::GateEndReport]);
    CHECK(!report_block::g_landed[report_block::GateAceGetData]);

    if (!fails) { std::printf("OK: report_block checks passed\n"); return 0; }
    std::printf("FAILED: %d\n", fails);
    return 1;
}
