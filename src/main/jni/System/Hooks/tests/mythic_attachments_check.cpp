// Host test for System/Hooks/MythicAttachments.h - "Attachment unlock".
//
// The feature is github.com/oonalang/VARIANT4's, and VARIANT4 asks the game one
// question: WeaponPromotionInfo.IsWeaponAttachmentUnlock(void*) @0xA61AA00. This
// test drives the REAL hook body with a fake original that records every call
// and answers "locked" (false), which is what the game returns for an attachment
// the player has not ground out. What is pinned:
//
//   * the signature: the replacement takes the instance and nothing else, so the
//     original can be driven with exactly the argument the game passes;
//   * ON  -> true, and the game's function is NOT called (nothing native runs to
//     produce the answer);
//   * OFF -> the original's answer, called EXACTLY once, instance intact;
//   * the original answering "unlocked" still comes through when the switch is
//     off - the hook only ever adds a yes, never takes one away;
//   * a null receiver is never given a fabricated answer;
//   * with the original not installed yet (null), the hook answers false instead
//     of dereferencing null.
//
// The seven-hook stack this replaced (four answers, two OwnState row rewrites and
// the MythicAttachmentSystem builders) is gone with its test: the row rewrites
// were what put locked states back on rows the loadout popup had already drawn.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests and Tools::IsPtrValid is stubbed because Tools.cpp is
// not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes \
//       src/main/jni/System/Hooks/tests/mythic_attachments_check.cpp \
//       -o /tmp/mythic_attachments_check && /tmp/mythic_attachments_check
// Exits 0 and prints "OK: all attachment-unlock checks passed" on success.
// Android.mk compiles only ImGui / IL2CppSDKGenerator/IL2Cpp /
// IL2CppSDKGenerator/KittyMemory / System/Texture and the jni root, so nothing
// under System/Hooks is ever compiled into the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

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
#include "System/Hooks/MythicAttachments.h"

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

static int g_checks = 0;
static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        g_checks++;                                                       \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

using namespace mythic_attachments;

// The hook body must be callable with the instance and nothing else: this is the
// VARIANT4 signature (bool(void*)), and the compiler enforces it here.
static_assert(std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*>::value,
              "IsWeaponAttachmentUnlock takes (void* instance)");
static_assert(!std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*, int>::value,
              "IsWeaponAttachmentUnlock takes no attachment argument");

// ---------------------------------------------------------------------------
// Fake original: counts every call, records the receiver, and answers "locked"
// unless the test says otherwise.
// ---------------------------------------------------------------------------
static int  g_calls = 0;
static void *g_lastInstance = nullptr;
static bool g_origAnswer = false;

static bool fake_origAttachmentUnlock(void *instance)
{
    g_calls++;
    g_lastInstance = instance;
    return g_origAnswer;
}

static bool (*installFake())(void *)
{
    orig_IsWeaponAttachmentUnlock = fake_origAttachmentUnlock;
    g_calls = 0;
    g_lastInstance = nullptr;
    g_origAnswer = false;
    return fake_origAttachmentUnlock;
}

int main()
{
    // ---- A1: the defaults this feature ships with --------------------------
    {
        sConfig fresh{0};                       // exactly what Init_Hacks.h declares
        CHECK(fresh.ExtraMenu.Attachment,
              "A1 Attachment unlock is ON out of the box");
        CHECK(!fresh.ExtraMenu.MythicAttachments,
              "A1 Blueprint (MythicAttachments) is OFF out of the box");
    }

    // ---- A2: the address VARIANT4 hooks ------------------------------------
    CHECK(Rva_IsWeaponAttachmentUnlock == 0xA61AA00u,
          "A2 the hook address is VARIANT4's 0xA61AA00");

    // ---- A3: the signature, from the original's side -----------------------
    {
        // Assigning a plain bool(void*) straight to the original pointer is the
        // check: a trampoline declared with an extra argument cannot take it.
        bool (*typed)(void *) = installFake();
        CHECK(typed == orig_IsWeaponAttachmentUnlock,
              "A3 the original is a bool(void*) - the game's own signature");
    }

    static int g_instance = 0;

    // ---- A4: ON -> yes, and the game's function is never called ------------
    {
        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A4 ON: the attachment counts as owned");
        CHECK(g_calls == 0,
              "A4 ON: nothing native runs to produce that answer");
    }

    // ---- A5: OFF -> the game's own answer, pass-through intact -------------
    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        CHECK(!IsWeaponAttachmentUnlock(&g_instance),
              "A5 OFF: the game's own verdict is returned");
        CHECK(g_calls == 1 && g_lastInstance == &g_instance,
              "A5 OFF: called exactly once, with the receiver intact");
    }

    // ---- A6: OFF never takes away a real unlock ---------------------------
    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        g_origAnswer = true;                    // the game says: owned
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A6 OFF: a genuine unlock still comes through");
        CHECK(g_calls == 1,
              "A6 OFF: ... and it was the game that said so");
    }

    // ---- A7: a null receiver is never answered for ------------------------
    {
        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(!IsWeaponAttachmentUnlock(nullptr),
              "A7 a null object gets the game's answer, never a fabricated yes");
        CHECK(g_calls == 1,
              "A7 ... and the game's own function did the deciding");
    }

    // ---- A8: the hook is safe before the original is installed ------------
    // A grant does not need the original (the answer is decided here), so with
    // the switch on a real weapon still gets "owned" before the trampoline is
    // filled in; only the pass-through path can be reached with a null original,
    // and that answers false instead of calling through it.
    {
        Config.ExtraMenu.Attachment = true;
        orig_IsWeaponAttachmentUnlock = nullptr;
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A8 with no original installed the grant still answers yes");
        CHECK(!IsWeaponAttachmentUnlock(nullptr),
              "A8 ... and a null receiver answers false, not a crash");
    }

    // ---- A9: the toggle the body reads is the menu's ----------------------
    {
        Config.ExtraMenu.Attachment = true;
        CHECK(AttachmentsUnlocked(), "A9 AttachmentsUnlocked() follows the switch");
        Config.ExtraMenu.Attachment = false;
        CHECK(!AttachmentsUnlocked(), "A9 ... in both directions");
        Config.ExtraMenu.Attachment = true;
    }

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all attachment-unlock checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
