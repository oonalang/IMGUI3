// Host check for the anti-login reclaim logic: the gates forward, the relogin
// wait is zeroed, the auto-login cycle is only kept alive inside a bounded
// window opened by the game's own logout signals, and the toggle is the only
// thing that decides between forcing and plain forwarding.
//
//   g++ -std=c++17 -Isrc/main/jni -Isrc/main/jni/Skin/tests/stubs \
//       src/main/jni/System/Hooks/tests/anti_login_check.cpp -o /tmp/anti_login_check

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
#include "Hacks/Feature/Init_Hacks.h"
#include "System/Hooks/AntiLogin.h"

bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }
extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }
void MSHookFunction(void *, void *, void **) {}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int  g_reloginCoroutines = 0;
static int  g_checkStates = 0;
static int  g_logouts = 0;
static int  g_lobbyLogouts = 0;
static int  g_dsLogouts = 0;
static int  g_retryAsked = 0;
static int  g_startAsked = 0;
static int  g_maxAsked = 0;

static void *fakeExecuteRelogin(void *) { g_reloginCoroutines++; return (void *)0x99; }
static void fakeCheckReloginState(void *, int) { g_checkStates++; }
static void fakeLogout(void *, bool) { g_logouts++; }
static void fakeLobbyLogout(void *) { g_lobbyLogouts++; }
static void fakeDsLogout(void *) { g_dsLogouts++; }
static bool fakeCanRetry() { g_retryAsked++; return false; }
static bool fakeCanStart(int, bool) { g_startAsked++; return false; }
static int  fakeMaxNum() { g_maxAsked++; return 3; }

static void armOriginals()
{
    anti_login::orig_ExecuteReloginByRandomDelay = fakeExecuteRelogin;
    anti_login::orig_CheckReloginState          = fakeCheckReloginState;
    anti_login::orig_Logout                     = fakeLogout;
    anti_login::orig_OnReceiveLobbyLogout       = fakeLobbyLogout;
    anti_login::orig_OnReceiveDSLogout          = fakeDsLogout;
    anti_login::orig_CanRetryAutoLogin          = fakeCanRetry;
    anti_login::orig_CanStartAutoLogin          = fakeCanStart;
    anti_login::orig_GetMaxAutoLoginNum         = fakeMaxNum;
}

static void resetCounters()
{
    g_reloginCoroutines = g_checkStates = g_logouts = 0;
    g_lobbyLogouts = g_dsLogouts = 0;
    g_retryAsked = g_startAsked = g_maxAsked = 0;
    anti_login::g_windowUntil  = -1000.0;
    anti_login::g_windowCount  = 0;
    anti_login::g_forcedStarts = 0;
}

static void CloseWindowNow()
{
    anti_login::g_windowUntil = anti_login::Now() - 1.0;
}

int main()
{
    armOriginals();
    resetCounters();

    // The toggle ships off.
    CHECK(Config.ExtraMenu.AntiLogin == false);

    // Off: every gate forwards and nothing is forced, even if a window were open.
    anti_login::OpenWindow("test");
    Config.ExtraMenu.AntiLogin = false;
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);
    CHECK(anti_login::hook_CanStartAutoLogin(3, true) == false);
    CHECK(anti_login::hook_GetMaxAutoLoginNum() == 3);
    CHECK(g_retryAsked == 1 && g_startAsked == 1 && g_maxAsked == 1);
    CHECK(anti_login::g_forcedStarts == 0);
    anti_login::hook_OnReceiveLobbyLogout(nullptr);
    CHECK(g_lobbyLogouts == 1);
    anti_login::hook_CheckReloginState(nullptr, 2);
    CHECK(g_checkStates == 1);
    resetCounters();

    // On, but nobody has asked for a relogin: still plain forwarding.
    Config.ExtraMenu.AntiLogin = true;
    CHECK(!anti_login::WindowOpen());
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == false);
    CHECK(anti_login::hook_GetMaxAutoLoginNum() == 3);
    CHECK(anti_login::g_forcedStarts == 0);

    // The game reports the session is over: a window opens and the relogin is
    // held open instead of forwarding straight away.
    anti_login::hook_OnReceiveLobbyLogout((void *)0x10);
    CHECK(g_lobbyLogouts == 1);
    CHECK(anti_login::WindowOpen());
    CHECK(anti_login::g_windowCount == 1);
    CHECK(anti_login::hook_CanRetryAutoLogin() == true);
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == true);
    CHECK(anti_login::hook_GetMaxAutoLoginNum() == anti_login::kMaxAutoLoginNum);
    CHECK(anti_login::g_forcedStarts == 1);

    // The forced starts are capped, so a server that keeps refusing cannot turn
    // this into an endless login loop.
    for (int i = 0; i < anti_login::kMaxForcedStarts + 4; i++)
        anti_login::hook_CanStartAutoLogin(0, false);
    CHECK(anti_login::g_forcedStarts == anti_login::kMaxForcedStarts);
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == false);
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);

    // A successful relogin closes the window and handing control back to the game.
    const int before = g_checkStates;
    anti_login::hook_CheckReloginState(nullptr, 2);
    CHECK(g_checkStates == before + 1);
    CHECK(!anti_login::WindowOpen());
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);

    // A failed relogin keeps it open, and a fresh window resets the budget.
    anti_login::hook_CheckReloginState(nullptr, 1);
    CHECK(anti_login::WindowOpen());
    CHECK(anti_login::g_forcedStarts == 0);
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == true);

    // Expiry: the same state, but out of time.
    CloseWindowNow();
    CHECK(!anti_login::WindowOpen());
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == false);
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);
    CHECK(anti_login::hook_GetMaxAutoLoginNum() == 3);

    // The socket layer's random wait is dropped so the retry starts now.
    // _reloginDelayTime sits at 0x28 on BrokerSocketManager.
    struct RelayLogin { char pad[0x28]; int delay; char rest[0x18]; };
    RelayLogin broker{};
    memset(&broker, 0, sizeof broker);
    broker.delay = 12;
    CHECK(anti_login::hook_ExecuteReloginByRandomDelay(&broker) == (void *)0x99);
    CHECK(broker.delay == 0);
    CHECK(anti_login::g_delaySkips == 1);
    CHECK(g_reloginCoroutines == 1);

    // A field that does not look like a seconds count is left alone - a wrong
    // offset must not overwrite a pointer into the socket manager.
    broker.delay = 100000;
    anti_login::hook_ExecuteReloginByRandomDelay(&broker);
    CHECK(broker.delay == 100000);
    CHECK(anti_login::g_delaySkips == 1);

    // With the toggle off the wait is untouched too.
    Config.ExtraMenu.AntiLogin = false;
    CloseWindowNow();
    broker.delay = 5;
    anti_login::hook_ExecuteReloginByRandomDelay(&broker);
    CHECK(broker.delay == 5);

    // A relogin logout opens the window; a plain logout does not.
    Config.ExtraMenu.AntiLogin = true;
    resetCounters();
    anti_login::hook_Logout((void *)0x20, false);
    CHECK(g_logouts == 1);
    CHECK(!anti_login::WindowOpen());
    anti_login::hook_Logout((void *)0x20, true);
    CHECK(g_logouts == 2);
    CHECK(anti_login::WindowOpen());

    // Missing originals never crash and never invent a value.
    anti_login::orig_CanRetryAutoLogin  = nullptr;
    anti_login::orig_CanStartAutoLogin  = nullptr;
    anti_login::orig_GetMaxAutoLoginNum = nullptr;
    anti_login::orig_ExecuteReloginByRandomDelay = nullptr;
    CloseWindowNow();
    CHECK(anti_login::hook_CanRetryAutoLogin() == false);
    CHECK(anti_login::hook_CanStartAutoLogin(0, false) == false);
    CHECK(anti_login::hook_GetMaxAutoLoginNum() == 0);
    CHECK(anti_login::hook_ExecuteReloginByRandomDelay(nullptr) == nullptr);

    // Landed bookkeeping the installer fills in.
    anti_login::NoteLanded(anti_login::GateAutoStart, true);
    anti_login::NoteLanded(anti_login::GateLobbyLogout, true);
    CHECK(anti_login::g_landed[anti_login::GateAutoStart]);
    CHECK(anti_login::g_landed[anti_login::GateLobbyLogout]);
    anti_login::NoteLanded((anti_login::Gate)99, true);
    anti_login::NoteLanded((anti_login::Gate)-3, true);
    CHECK(!anti_login::g_landed[anti_login::GateAutoMax]);

    if (!fails) { std::printf("OK: anti_login checks passed\n"); return 0; }
    std::printf("FAILED: %d\n", fails);
    return 1;
}
