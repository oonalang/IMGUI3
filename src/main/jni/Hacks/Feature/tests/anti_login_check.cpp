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

static void *g_self = (void *)&g_iteratorSentinel;

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

    CHECK(Config.ExtraMenu.AntiLogin == false);
    CHECK(!Enabled());

    ResetAll();
    Config.ExtraMenu.AntiLogin = false;
    CHECK(hook_ExecuteReloginByRandomDelay(g_self) == (void *)&g_iteratorSentinel);
    hook_CheckReloginState(g_self, 7);
    hook_ReloginCallback(g_self);
    hook_Logout(g_self, true);
    hook_Logout(g_self, false);
    hook_OnNotifyNeedReloginCallback(g_self);
    CHECK(g_execCalls == 1 && g_checkCalls == 1 && g_lastOrigState == 7);
    CHECK(g_callbackCalls == 1 && g_loadCallbackCalls == 1);
    CHECK(g_logoutCalls == 2 && g_lastLogoutRelogin == 0);
    for (int i = 0; i < GateCount; i++) CHECK(g_blocked[i] == 0);

    ResetAll();
    Config.ExtraMenu.AntiLogin = true;
    CHECK(Enabled());
    CHECK(hook_ExecuteReloginByRandomDelay(g_self) == nullptr);
    hook_CheckReloginState(g_self, 3);
    hook_ReloginCallback(g_self);
    hook_Logout(g_self, true);
    hook_OnNotifyNeedReloginCallback(g_self);
    CHECK(g_execCalls == 0 && g_checkCalls == 0);
    CHECK(g_callbackCalls == 0 && g_logoutCalls == 0 && g_loadCallbackCalls == 0);
    CHECK(g_lastState == 3);
    for (int i = 0; i < GateCount; i++) CHECK(g_blocked[i] == 1);

    hook_Logout(g_self, false);
    CHECK(g_logoutCalls == 1 && g_lastLogoutRelogin == 0);
    CHECK(g_blocked[GateLogout] == 1);

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

