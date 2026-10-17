// Host test for the ADS answer that replaced Pawn.IsAiming().
//
// ``Pawn.IsAiming() RVA 0x519BB10`` is a native method that walks the pawn's own
// weapon / fire-component chain, and calling it is what crashed the "ADS only"
// gates (the triggerbot's ADS mode and the aimbot's "Trigger: Scoping" mode both
// used to call it). Those three call sites now ask the pawn's own held weapon
// instead - StructSDK.h's ``Weapon::m_IsAiming()``, two FIELD reads at the dump's
// own offsets (m_IsAiming @0xC21 bool, m_AimingPercentage @0x460 float) reached
// through ``Pawn::get_IsAimingSafe()``.
//
// What is pinned here:
//   * the field-read answer: flag OR a transition past half way, with a
//     percentage outside 0..2 ignored (not read as "always scoped");
//   * a pawn with no weapon (dead / respawning / lobby) is simply not scoping;
//   * the two offsets are the dump's, checked against a fake weapon laid out at
//     exactly those offsets;
//   * Pawn.IsAiming() 0x519BB10 is NEVER called, whatever the state - the
//     counter on the fake stays at 0 for every case below.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests and Tools::IsPtrValid is stubbed because Tools.cpp is
// not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes \
//       src/main/jni/Hacks/Feature/tests/ads_safe_check.cpp \
//       -o /tmp/ads_safe_check && /tmp/ads_safe_check
// Exits 0 and prints "OK: all safe-ADS checks passed" on success.
// Android.mk does not wildcard Hacks/Feature/tests, so it never reaches the APK.
#include <cstdint>
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

// Tools::IsPtrValid (Tools.cpp is intentionally not linked).
bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

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

// ---------------------------------------------------------------------------
// The fakes. `m_unity + api22` is Pawn.get_CurrentWeapon, so pointing m_unity at
// a buffer that holds our fake function makes the REAL StructSDK helper call it.
// Pawn.IsAiming() (api15) points at a counter - nothing may ever reach it.
// ---------------------------------------------------------------------------
static int g_getCurrentWeaponCalls = 0;
static int g_pawnIsAimingCalls     = 0;

static void *g_weapon = nullptr;

static Weapon *fake_get_CurrentWeapon(Pawn *)
{
    g_getCurrentWeaponCalls++;
    return (Weapon *)g_weapon;
}

static bool fake_pawnIsAiming(void *)
{
    g_pawnIsAimingCalls++;
    return true;                        // the crashy call would say "aiming"
}

// A fake weapon with room past both offsets the dump gives.
static char *make_weapon(bool flag, float pct)
{
    char *w = (char *)calloc(1, 0x1000);
    *(bool  *)(w + Class_Weapon_m_IsAiming)         = flag;
    *(float *)(w + Class_Weapon_m_AimingPercentage) = pct;
    return w;
}

int main()
{
    printf("== safe ADS answer (Pawn.IsAiming() replacement) ==\n");

    // ---- the offsets are the dump's ---------------------------------------
    CHECK(Class_Weapon_m_IsAiming == 0xC21,
          "A1 m_IsAiming is the dump's 0xC21 (dump.cs 738043)");
    CHECK(Class_Weapon_m_AimingPercentage == 0x460,
          "A1 m_AimingPercentage is the dump's 0x460 (dump.cs 737588)");

    // ---- wire the two accessors -------------------------------------------
    {
        // Class_Pawn_get_CurrentWeapon is the macro (m_unity + api22), so
        // solving m_unity from it makes the real StructSDK helper call the fake.
        // (api23 / Weapon.get_WeaponID is never called by this test.)
        m_unity = (uintptr_t)&fake_get_CurrentWeapon - api22;
        api15   = (uintptr_t)&fake_pawnIsAiming; // Pawn.IsAiming - must stay 0
    }

    Pawn *pawn = (Pawn *)0x1000;                // never dereferenced here

    // ---- A2: no weapon -> not scoping, and no native call ------------------
    {
        void *saved = g_weapon;
        g_weapon = nullptr;
        g_getCurrentWeaponCalls = 0;
        const bool aiming = pawn->get_IsAimingSafe();
        CHECK(!aiming,
              "A2 a pawn with no held weapon (dead / lobby) is not scoping");
        CHECK(g_getCurrentWeaponCalls == 1,
              "A2 ... the pawn's own getter is what answered");
        g_weapon = saved;
    }

    // ---- A3: the flag alone is enough -------------------------------------
    {
        char *w = make_weapon(/*flag=*/true, /*pct=*/0.0f);
        g_weapon = w;
        CHECK(pawn->get_IsAimingSafe(),
              "A3 m_IsAiming @0xC21 alone answers \"scoping\"");
        CHECK(((Weapon *)w)->m_IsAiming() == true,
              "A3 ... and the weapon itself agrees");
        free(w);
    }

    // ---- A4: a transition past half way counts, a partial one does not ----
    {
        char *partial = make_weapon(false, 0.2f);
        g_weapon = partial;
        CHECK(!pawn->get_IsAimingSafe(),
              "A4 a 20% ADS transition is not scoping yet");
        free(partial);

        char *rising = make_weapon(false, 0.75f);
        g_weapon = rising;
        CHECK(pawn->get_IsAimingSafe(),
              "A4 a 75% ADS transition already counts (the scope is coming up)");
        free(rising);
    }

    // ---- A5: garbage percentage is ignored, not read as "always scoped" ---
    {
        char *bad  = make_weapon(false, 5.0f);
        g_weapon = bad;
        CHECK(!pawn->get_IsAimingSafe(),
              "A5 a percentage outside 0..2 is not ADS progress");
        free(bad);

        char *neg = make_weapon(false, -1.0f);
        g_weapon = neg;
        CHECK(!pawn->get_IsAimingSafe(),
              "A5 ... and neither is a negative one");
        free(neg);
    }

    // ---- A6: the crashy native call is never made --------------------------
    CHECK(g_pawnIsAimingCalls == 0,
          "A6 Pawn.IsAiming() 0x519BB10 was NEVER called by any case above");

    // ---- A7: the answer is live, not latched ------------------------------
    {
        char *w = make_weapon(false, 0.0f);
        g_weapon = w;
        const bool off = pawn->get_IsAimingSafe();
        *(bool *)(w + Class_Weapon_m_IsAiming) = true;
        const bool on = pawn->get_IsAimingSafe();
        CHECK(!off && on,
              "A7 the same weapon reads not-scoping then scoping as the flag flips");
        free(w);
    }

    CHECK(g_pawnIsAimingCalls == 0,
          "A7 ... and still no Pawn.IsAiming() call");

    if (g_failures == 0)
        printf("OK: all safe-ADS checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
