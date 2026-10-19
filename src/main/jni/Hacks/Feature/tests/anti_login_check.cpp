// Host test for System/Hooks/AntiLogin.h (Anti-Login).
//
// The five hooks themselves only run on-device (they swallow the game's
// forced-relogin path when the account is accessed from another location),
// but their decision is pure pass-through logic over fake originals: with
// the toggle off every call must reach the original exactly; with it on,
// the relogin executor / state verdict / callbacks must be swallowed,
// Logout must be swallowed ONLY in its forced form (isRelogin = true),
// and a manual logout must still pass through.
//
// It compiles the REAL Init_Hacks.h, so the Config.ExtraMenu.AntiLogin
// switch the hooks read is checked against the real config struct.
//
// Host test, no NDK needed (same stub set as config_defaults_check.cpp):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes -I src/main/jni/foxcheats/include \
//       src/main/jni/Hacks/Feature/tests/anti_login_check.cpp \
//       -o /tmp/anti_login_check && /tmp/anti_login_check
// Exits 0 and prints "OK: ..." on success. Android.mk's wildcards do not
// cover Hacks/Feature/tests, so this file never reaches the APK.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "System/Hooks/AntiLogin.h"

// Macros.h pulls in the Substrate headers; nothing here hooks anything.
extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace; (void)result;
}

static int g_failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_failures++;                                               \
        }                                                               \
    } while (0)

// ---- fake originals ---------------------------------------------------------
static int g_iteratorSentinel;
static int g_execCalls = 0;
static int g_checkCalls = 0;
static int g_lastOrigState = -1;
static int g_callbackCalls = 0;
static int g_logoutCalls = 0;
static int g_lastLogoutRelogin = -1;
static int g_loadCallbackCalls = 0;

static void *FakeExecuteRelogin(void *) { g_execCalls++; return (void *)&g_iteratorSentinel; }
static void FakeCheckReloginState(void *, int state) { g_checkCalls++; g_lastOrigState = state; }
static void FakeReloginCallback(void *) { g_callbackCalls++; }
static void FakeLogout(void *, bool isRelogin) { g_logoutCalls++; g_lastLogoutRelogin = isRelogin ? 1 : 0; }
static void FakeLoadCallback(void *) { g_loadCallbackCalls++; }

static void *g_self = (void *)&g_iteratorSentinel;  // any non-null "instance"

static void ResetAll()
{
    std::memset(anti_login::g_blocked, 0, sizeof(anti_login::g_blocked));
    std::memset(anti_login::g_landed, 0, sizeof(anti_login::g_landed));
    anti_login::g_lastState = 0;
    g_execCalls = 0;
    g_checkCalls = 0;
    g_lastOrigState = -1;
    g_callbackCalls = 0;
    g_logoutCalls = 0;
    g_lastLogoutRelogin = -1;
    g_loadCallbackCalls = 0;
}

int main()
{
    using namespace anti_login;

    orig_ExecuteReloginByRandomDelay = &FakeExecuteRelogin;
    orig_CheckReloginState = &FakeCheckReloginState;
    orig_ReloginCallback = &FakeReloginCallback;
    orig_Logout = &FakeLogout;
    orig_OnNotifyNeedReloginCallback = &FakeLoadCallback;

    // The switch must default OFF, like the other ExtraMenu toggles.
    CHECK(Config.ExtraMenu.AntiLogin == false);
    CHECK(!Enabled());

    // ---- toggle OFF: exact stock pass-through ------------------------------
    ResetAll();
    Config.ExtraMenu.AntiLogin = false;
    CHECK(hook_ExecuteReloginByRandomDelay(g_self) == (void *)&g_iteratorSentinel);
    hook_CheckReloginState(g_self, 7);
    hook_ReloginCallback(g_self);
    hook_Logout(g_self, true);    // even a forced logout reaches the original
    hook_Logout(g_self, false);
    hook_OnNotifyNeedReloginCallback(g_self);
    CHECK(g_execCalls == 1 && g_checkCalls == 1 && g_lastOrigState == 7);
    CHECK(g_callbackCalls == 1 && g_loadCallbackCalls == 1);
    CHECK(g_logoutCalls == 2 && g_lastLogoutRelogin == 0);
    for (int i = 0; i < GateCount; i++) CHECK(g_blocked[i] == 0);

    // ---- toggle ON: the whole forced relogin is refused ---------------------
    ResetAll();
    Config.ExtraMenu.AntiLogin = true;
    CHECK(Enabled());
    CHECK(hook_ExecuteReloginByRandomDelay(g_self) == nullptr);  // no iterator, no relogin
    hook_CheckReloginState(g_self, 3);
    hook_ReloginCallback(g_self);
    hook_Logout(g_self, true);     // forced logout: swallowed
    hook_OnNotifyNeedReloginCallback(g_self);
    CHECK(g_execCalls == 0 && g_checkCalls == 0);
    CHECK(g_callbackCalls == 0 && g_logoutCalls == 0 && g_loadCallbackCalls == 0);
    CHECK(g_lastState == 3);       // the verdict is still recorded internally
    for (int i = 0; i < GateCount; i++) CHECK(g_blocked[i] == 1);

    // ---- manual logout is NOT blocked, even with Anti-Login on ---------------
    hook_Logout(g_self, false);
    CHECK(g_logoutCalls == 1 && g_lastLogoutRelogin == 0);
    CHECK(g_blocked[GateLogout] == 1);  // unchanged by the manual pass-through

    // ---- landed-flag bounds ---------------------------------------------------
    ResetAll();
    NoteLanded(static_cast<Gate>(-1), true);
    NoteLanded(GateCount, true);
    NoteBlocked(static_cast<Gate>(99));
    for (int i = 0; i < GateCount; i++) {
        CHECK(!g_landed[i]);
        CHECK(g_blocked[i] == 0);
    }
    NoteLanded(GateLogout, true);
    CHECK(g_landed[GateLogout]);

    Config.ExtraMenu.AntiLogin = false;
    if (g_failures == 0) {
        std::printf("OK: anti_login_check - stock pass-through off, forced relogin blocked on, manual logout untouched\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
