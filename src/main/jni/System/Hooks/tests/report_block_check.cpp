// Host test for System/Hooks/ReportBlock.h - "Block Report Evidence": the
// client's own report / evidence senders are swallowed while the toggle is ON,
// and restored exactly while it is OFF.
//
// It drives the REAL hook bodies with fake originals that count every call and
// record every argument, which is what pins the two things that matter:
//
//   * the default: sConfig's own aggregate init leaves BlockReport ON (and the
//     row is the one the SETTINGS tab draws);
//   * ON  -> none of the four originals runs: DoReport does not fire,
//            DoReportPlayer answers 0, BeginReportPlayerPull answers nullptr,
//            EndReportPlayerPull sends nothing;
//   * OFF -> each original runs EXACTLY once with the caller's arguments
//            unchanged (all seven BeginReportPlayerPull arguments included) and
//            its answer is passed straight back;
//   * a null original (hook install failed) is never dereferenced - the hooks
//            fail open with the "nothing scheduled" answer;
//   * the counters: seen increments on every call, blocked only while ON;
//   * the landed flags Feats.h records.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests and Tools::IsPtrValid is stubbed because Tools.cpp is
// not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes \
//       src/main/jni/System/Hooks/tests/report_block_check.cpp \
//       -o /tmp/report_block_check && /tmp/report_block_check
// Exits 0 and prints "OK: all report-block checks passed" on success.
// Android.mk compiles only ImGui / IL2CppSDKGenerator/IL2Cpp /
// IL2CppSDKGenerator/KittyMemory / System/Texture and the jni root, so nothing
// under System/Hooks is ever compiled into the APK.
#include <cstdint>
#include <cstdio>
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
#include "Hacks/Feature/Init_Hacks.h"      // sConfig Config - used by the header
#include "System/Hooks/ReportBlock.h"

// Tools::IsPtrValid (Tools.cpp is intentionally not linked).
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

// __android_log_print is what LOGI() expands to; nothing here logs, but the
// symbol has to exist for the TU to link.
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

// Substrate's hook entry (pulled in by Includes/Macros.h's non-inline hook()
// helper) - this test installs no hooks, but the symbol has to exist to link.
void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

using namespace report_block;

// ---------------------------------------------------------------------------
// Fake originals: every call is counted, and every argument is recorded so the
// pass-through can be checked argument-for-argument.
// ---------------------------------------------------------------------------
struct Calls
{
    int doReport       = 0;
    int doReportPlayer = 0;
    int beginPull      = 0;
    int endPull        = 0;

    void *lastInstance = nullptr;
    void *lastOpenids  = nullptr;
    void *lastRequest  = nullptr;

    int     lastPullType    = -1;
    uint8_t lastMoveState   = 0;
    void   *lastClientPos   = nullptr;
    void   *lastServerPos   = nullptr;
    int     lastHitCount    = -1;
    uint8_t lastLineHit     = 0;
    void   *lastHitPosition = nullptr;

    void reset()
    {
        doReport = doReportPlayer = beginPull = endPull = 0;
        lastInstance = lastOpenids = lastRequest = nullptr;
        lastPullType = -1; lastMoveState = 0;
        lastClientPos = lastServerPos = nullptr;
        lastHitCount = -1; lastLineHit = 0; lastHitPosition = nullptr;
    }
};
static Calls g_calls;

static int g_reportWindow = 0;
static int g_voiceSystem  = 0;
static int g_openIds[2]   = {0, 0};
static int g_clientPos = 0, g_serverPos = 0, g_hitPos = 0, g_request = 0;

static void fake_origDoReport(void *instance)
{
    g_calls.doReport++;
    g_calls.lastInstance = instance;
}
static int fake_origDoReportPlayer(void *instance, void *openids)
{
    g_calls.doReportPlayer++;
    g_calls.lastInstance = instance;
    g_calls.lastOpenids  = openids;
    return 42;                          // a status the block must NOT return
}
static void *fake_origBeginReportPlayerPull(int pullType, uint8_t moveState,
                                            void *clientPos, void *serverPos,
                                            int hitCount, uint8_t lineHitCount,
                                            void *hitPosition)
{
    g_calls.beginPull++;
    g_calls.lastPullType    = pullType;
    g_calls.lastMoveState   = moveState;
    g_calls.lastClientPos   = clientPos;
    g_calls.lastServerPos   = serverPos;
    g_calls.lastHitCount    = hitCount;
    g_calls.lastLineHit     = lineHitCount;
    g_calls.lastHitPosition = hitPosition;
    return &g_request;
}
static void fake_origEndReportPlayerPull(void *request)
{
    g_calls.endPull++;
    g_calls.lastRequest = request;
}

static void installFakes()
{
    orig_DoReport              = fake_origDoReport;
    orig_DoReportPlayer        = fake_origDoReportPlayer;
    orig_BeginReportPlayerPull = fake_origBeginReportPlayerPull;
    orig_EndReportPlayerPull   = fake_origEndReportPlayerPull;
    g_calls.reset();
}

static void setBlocked(bool on) { Config.ExtraMenu.BlockReport = on; }

// One call of each of the four gates, so a single ON/OFF pass checks all four.
static void callAllGates(void **beginOut, int *voiceOut)
{
    DoReport(&g_reportWindow);
    *voiceOut = DoReportPlayer(&g_voiceSystem, g_openIds);
    *beginOut = BeginReportPlayerPull(/*pullType=*/3, /*moveState=*/7,
                                      &g_clientPos, &g_serverPos,
                                      /*hitCount=*/5, /*lineHitCount=*/2,
                                      &g_hitPos);
    EndReportPlayerPull(&g_request);
}

int main()
{
    printf("== ReportBlock: Block Report Evidence ==\n");

    // ---- R1: the shipped default ------------------------------------------
    {
        sConfig fresh{};
        CHECK(fresh.ExtraMenu.BlockReport == true,
              "R1 a fresh config has Block Report Evidence ON");
        CHECK(fresh.ExtraMenu.MythicAttachments == true,
              "R1 ... and the neighbouring default (MythicAttachments) is untouched");
    }

    installFakes();
    setBlocked(true);

    // ---- R2: ON swallows all four ------------------------------------------
    {
        memset(&g_diag, 0, sizeof(g_diag));
        int voiceStatus = -1;
        void *begin = (void *)&g_clientPos;
        callAllGates(&begin, &voiceStatus);

        CHECK(g_calls.doReport == 0,
              "R2 ON: ReportWindowController.DoReport never reaches the game");
        CHECK(g_calls.doReportPlayer == 0 && voiceStatus == 0,
              "R2 ON: DoReportPlayer answers 0 (nothing scheduled), game not called");
        CHECK(g_calls.beginPull == 0 && begin == nullptr,
              "R2 ON: BeginReportPlayerPull answers nullptr - no evidence request");
        CHECK(g_calls.endPull == 0,
              "R2 ON: EndReportPlayerPull sends nothing");

        CHECK(g_diag.seen[GateDoReport] == 1 && g_diag.blocked[GateDoReport] == 1,
              "R2 ON: the report-window gate counts one call, blocked");
        CHECK(g_diag.seen[GateVoice] == 1 && g_diag.blocked[GateVoice] == 1,
              "R2 ON: the voice gate counts one call, blocked");
        CHECK(g_diag.seen[GateBegin] == 1 && g_diag.blocked[GateBegin] == 1,
              "R2 ON: the evidence-build gate counts one call, blocked");
        CHECK(g_diag.seen[GateEnd] == 1 && g_diag.blocked[GateEnd] == 1,
              "R2 ON: the evidence-send gate counts one call, blocked");
    }

    // ---- R3: OFF passes everything straight through ------------------------
    {
        setBlocked(false);
        g_calls.reset();
        memset(&g_diag, 0, sizeof(g_diag));

        // DoReport first, on its own, so its recorded receiver cannot be
        // overwritten by the gate that runs next.
        DoReport(&g_reportWindow);
        CHECK(g_calls.doReport == 1 && g_calls.lastInstance == &g_reportWindow,
              "R3 OFF: DoReport calls the game once with the caller's instance");

        const int voiceStatus = DoReportPlayer(&g_voiceSystem, g_openIds);
        void *begin = BeginReportPlayerPull(3, 7, &g_clientPos, &g_serverPos, 5, 2, &g_hitPos);
        EndReportPlayerPull(&g_request);

        CHECK(g_calls.doReportPlayer == 1 && voiceStatus == 42,
              "R3 OFF: DoReportPlayer calls the game and returns the game's own status");
        CHECK(g_calls.lastInstance == &g_voiceSystem && g_calls.lastOpenids == g_openIds,
              "R3 OFF: ... with the caller's own receiver and openids array");

        CHECK(g_calls.beginPull == 1 && begin == &g_request,
              "R3 OFF: BeginReportPlayerPull returns the game's own request object");
        CHECK(g_calls.lastPullType == 3 && g_calls.lastMoveState == 7 &&
              g_calls.lastClientPos == &g_clientPos &&
              g_calls.lastServerPos == &g_serverPos &&
              g_calls.lastHitCount == 5 && g_calls.lastLineHit == 2 &&
              g_calls.lastHitPosition == &g_hitPos,
              "R3 OFF: all seven evidence arguments arrive unchanged");

        CHECK(g_calls.endPull == 1 && g_calls.lastRequest == &g_request,
              "R3 OFF: EndReportPlayerPull sends the caller's request");

        CHECK(g_diag.seen[GateDoReport] == 1 && g_diag.blocked[GateDoReport] == 0,
              "R3 OFF: the call is counted as seen, not as blocked");
    }

    // ---- R4: a null original is never dereferenced -------------------------
    {
        setBlocked(false);
        orig_DoReport              = nullptr;
        orig_DoReportPlayer        = nullptr;
        orig_BeginReportPlayerPull = nullptr;
        orig_EndReportPlayerPull   = nullptr;

        int voiceStatus = -1;
        void *begin = (void *)&g_clientPos;
        callAllGates(&begin, &voiceStatus);      // must not crash

        CHECK(voiceStatus == 0 && begin == nullptr,
              "R4 with no original installed the hooks answer the safe value");
        installFakes();
    }

    // ---- R5: the landed flags Feats.h records ------------------------------
    {
        memset(g_landed, 0, sizeof(g_landed));
        NoteLanded(GateBegin, true);
        NoteLanded(GateEnd, false);
        NoteLanded((Gate)99, true);              // out of range is ignored

        bool otherFlagSet = false;
        for (int i = 0; i < GateCount; i++)
            if (i != GateBegin && g_landed[i]) otherFlagSet = true;

        CHECK(g_landed[GateBegin] && !g_landed[GateEnd],
              "R5 a landed hook is reported, a failed one is not");
        CHECK(!otherFlagSet,
              "R5 an out-of-range gate leaves every other flag clear");
    }

    // ---- R6: the toggle is read on every call, not latched -----------------
    {
        setBlocked(true);
        g_calls.reset();
        DoReport(&g_reportWindow);
        const bool blockedNow = (g_calls.doReport == 0);

        setBlocked(false);
        DoReport(&g_reportWindow);
        const bool passedNow = (g_calls.doReport == 1);

        CHECK(blockedNow && passedNow,
              "R6 flipping the toggle takes effect on the very next call");
    }

    // Leave the global config as the rest of the session would find it.
    setBlocked(true);
    installFakes();

    if (g_failures == 0)
        printf("OK: all report-block checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
