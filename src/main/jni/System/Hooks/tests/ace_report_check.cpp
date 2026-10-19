// Host check for the "Scrub ACE Report Data" half of
// System/Hooks/ReportBlock.h.
//
// The four managed wrappers the game pulls the anti-cheat's evidence through
// are hooked on device, which no host build can exercise - but the gate itself
// is pure bookkeeping over IL2CPP object memory: ask the original, then empty
// the byte[] it returned IN PLACE. That is what this test drives, with fake
// byte[] objects laid out exactly like the real ones (klass 0x00,
// monitor 0x08, bounds 0x10, max_length 0x18, elements 0x20 - the element
// offset is 8-byte aligned for every array element type, which is why the gate
// uses an explicit pad rather than a flexible member).
//
// What it pins down:
//   * OFF by default, and OFF really is inert (no writes, no counters);
//   * ON empties the CONTENTS but keeps the object and its length, so every
//     caller still gets a valid array of the size it asked for;
//   * exactly max_length bytes are touched - never the tail of the buffer;
//   * a null original, a null blob and an unreadable pointer are all passed
//     back untouched instead of crashing or inventing an array;
//   * GetReportData4 still forwards its token;
//   * all four wrappers route through the same gate.
//
// Host test, no NDK needed (same stub set as report_block_check.cpp):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes -I src/main/jni/foxcheats/include \
//       src/main/jni/System/Hooks/tests/ace_report_check.cpp \
//       -o /tmp/ace_report_check && /tmp/ace_report_check
// Exits 0 and prints "OK: ..." on success. Android.mk's wildcards do not cover
// System/Hooks/tests, so this file never reaches the APK.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>

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

// The host has no /proc/self/maps libunity.so, so a pointer-range check is the
// faithful stand-in for Tools::IsPtrValid: the fakes live on the stack/heap.
bool Tools::IsPtrValid(void *addr)
{
    return addr != nullptr && (uintptr_t)addr >= 0x10000;
}

extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }
void MSHookFunction(void *, void *, void **) {}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define CHECK_MSG(c, msg) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); fails++; } } while (0)

// A byte[] is handed back to the game by the real wrappers; the gate only ever
// looks at max_length and the element area, so the fake needs exactly that.
struct FakeBlob {
    void *klass;
    void *monitor;
    void *bounds;
    int max_length;
    int pad;
    unsigned char data[64];
};

static_assert(sizeof(FakeBlob) == 0x60, "fake blob must stay a plain POD");
// The whole gate depends on this: elements start at 0x20, not 0x1C.
static_assert(offsetof(FakeBlob, data) == 0x20, "byte[] elements must sit at 0x20");

static void InitBlob(FakeBlob &b, int len, unsigned char value)
{
    b.klass = nullptr;
    b.monitor = nullptr;
    b.bounds = nullptr;
    b.max_length = len;
    b.pad = 0;
    memset(b.data, value, sizeof(b.data));
}

// The "original" managed getters. The token one records what it was handed, so
// the test can prove the gate forwards it.
static FakeBlob *g_nextBlob = nullptr;
static uint32_t g_lastToken = 0;
static int g_calls[4] = {0, 0, 0, 0};

static void *fakeGet1(void) { g_calls[0]++; return g_nextBlob; }
static void *fakeGet2(void) { g_calls[1]++; return g_nextBlob; }
static void *fakeGet3(void) { g_calls[2]++; return g_nextBlob; }
static void *fakeGet4(uint32_t token) { g_calls[3]++; g_lastToken = token; return g_nextBlob; }

static bool AllZero(const FakeBlob &b, int from, int to)
{
    for (int i = from; i < to; ++i)
        if (b.data[i] != 0)
            return false;
    return true;
}

static bool AllValue(const FakeBlob &b, int from, int to, unsigned char value)
{
    for (int i = from; i < to; ++i)
        if (b.data[i] != value)
            return false;
    return true;
}

int main()
{
    // -- Default is OFF, and the gate is inert while it is -------------------
    CHECK(Config.ExtraMenu.BlockAceReport == false);

    report_block::orig_AceGetReportData  = fakeGet1;
    report_block::orig_AceGetReportData2 = fakeGet2;
    report_block::orig_AceGetReportData3 = fakeGet3;
    report_block::orig_AceGetReportData4 = fakeGet4;

    FakeBlob off;
    InitBlob(off, 16, 0xAB);
    g_nextBlob = &off;
    CHECK(report_block::hook_AceGetReportData() == &off);
    CHECK(AllValue(off, 0, 64, 0xAB));
    CHECK(off.max_length == 16);
    CHECK(report_block::g_aceBlobsScrubbed == 0 && report_block::g_aceBytesScrubbed == 0);

    // -- ON: contents emptied, object and length kept ------------------------
    Config.ExtraMenu.BlockAceReport = true;

    FakeBlob on;
    InitBlob(on, 16, 0xCD);
    g_nextBlob = &on;
    CHECK(report_block::hook_AceGetReportData() == &on); // same object handed back
    CHECK(on.max_length == 16);                          // length the caller asked for
    CHECK(AllZero(on, 0, 16));                           // evidence gone
    CHECK(AllValue(on, 16, 64, 0xCD));                   // nothing beyond max_length touched
    CHECK(report_block::g_aceBlobsScrubbed == 1);
    CHECK(report_block::g_aceBytesScrubbed == 16);

    // -- All four wrappers route through the same gate -----------------------
    FakeBlob b2, b3, b4;
    InitBlob(b2, 8, 0x11); g_nextBlob = &b2;
    CHECK(report_block::hook_AceGetReportData2() == &b2 && AllZero(b2, 0, 8));
    InitBlob(b3, 8, 0x22); g_nextBlob = &b3;
    CHECK(report_block::hook_AceGetReportData3() == &b3 && AllZero(b3, 0, 8));
    InitBlob(b4, 8, 0x33); g_nextBlob = &b4;
    CHECK(report_block::hook_AceGetReportData4(0xDEADBEEFu) == &b4 && AllZero(b4, 0, 8));
    CHECK(g_lastToken == 0xDEADBEEFu); // the token survives the gate
    CHECK(g_calls[0] == 2 && g_calls[1] == 1 && g_calls[2] == 1 && g_calls[3] == 1);
    CHECK(report_block::g_aceBlobsScrubbed == 4);

    // -- A blob with no bytes: seen, nothing written -------------------------
    FakeBlob empty;
    InitBlob(empty, 0, 0x44);
    g_nextBlob = &empty;
    const int bytesBeforeEmpty = report_block::g_aceBytesScrubbed;
    CHECK(report_block::hook_AceGetReportData() == &empty);
    CHECK(AllValue(empty, 0, 64, 0x44));
    CHECK(report_block::g_aceBytesScrubbed == bytesBeforeEmpty);

    FakeBlob negative;
    InitBlob(negative, -5, 0x55);
    g_nextBlob = &negative;
    CHECK(report_block::hook_AceGetReportData() == &negative);
    CHECK(AllValue(negative, 0, 64, 0x55)); // a bogus length must not become a memset

    // -- Nothing the gate is handed can turn into a crash --------------------
    const int blobsBeforeNull = report_block::g_aceBlobsScrubbed;

    g_nextBlob = nullptr;
    CHECK(report_block::hook_AceGetReportData() == nullptr);

    report_block::orig_AceGetReportData = nullptr; // hook that never landed
    CHECK(report_block::hook_AceGetReportData() == nullptr);
    report_block::orig_AceGetReportData = fakeGet1;

    CHECK(report_block::ScrubAceBlob(nullptr) == nullptr);

    void *junk = (void *)0x1234; // not a readable object
    CHECK(report_block::ScrubAceBlob(junk) == junk);
    CHECK(report_block::g_aceBlobsScrubbed == blobsBeforeNull);

    // -- Back OFF: the writes stop ------------------------------------------
    Config.ExtraMenu.BlockAceReport = false;
    FakeBlob offAgain;
    InitBlob(offAgain, 16, 0x77);
    g_nextBlob = &offAgain;
    CHECK(report_block::hook_AceGetReportData() == &offAgain);
    CHECK(AllValue(offAgain, 0, 64, 0x77));

    // -- Landing bookkeeping for the four new gates --------------------------
    report_block::NoteLanded(report_block::GateAceGetData, true);
    report_block::NoteLanded(report_block::GateAceGetData4, true);
    CHECK(report_block::g_landed[report_block::GateAceGetData]);
    CHECK(report_block::g_landed[report_block::GateAceGetData4]);
    CHECK(report_block::g_landed[report_block::GateAceGetData2] == false);
    CHECK(report_block::g_landed[report_block::GateFillReportData] == false);

    if (!fails) { std::printf("OK: ace_report checks passed\n"); return 0; }
    std::printf("FAILED: %d\n", fails);
    return 1;
}
