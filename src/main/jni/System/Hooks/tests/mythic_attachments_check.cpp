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
#include "Hacks/Feature/Init_Hacks.h"
#include "System/Hooks/MythicAttachments.h"

bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

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

static_assert(std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*>::value,
              "IsWeaponAttachmentUnlock takes (void* instance)");
static_assert(!std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*, int>::value,
              "IsWeaponAttachmentUnlock takes no attachment argument");

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
    {
        sConfig fresh{0};
        CHECK(fresh.ExtraMenu.Attachment,
              "A1 Attachment unlock is ON out of the box");
        CHECK(!fresh.ExtraMenu.MythicAttachments,
              "A1 Blueprint (MythicAttachments) is OFF out of the box");
    }

    CHECK(Rva_IsWeaponAttachmentUnlock == 0xA61AA00u,
          "A2 the hook address is VARIANT4's 0xA61AA00");

    {
        bool (*typed)(void *) = installFake();
        CHECK(typed == orig_IsWeaponAttachmentUnlock,
              "A3 the original is a bool(void*) - the game's own signature");
    }

    static int g_instance = 0;

    {
        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A4 ON: the attachment counts as owned");
        CHECK(g_calls == 0,
              "A4 ON: nothing native runs to produce that answer");
    }

    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        CHECK(!IsWeaponAttachmentUnlock(&g_instance),
              "A5 OFF: the game's own verdict is returned");
        CHECK(g_calls == 1 && g_lastInstance == &g_instance,
              "A5 OFF: called exactly once, with the receiver intact");
    }

    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        g_origAnswer = true;
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A6 OFF: a genuine unlock still comes through");
        CHECK(g_calls == 1,
              "A6 OFF: ... and it was the game that said so");
    }

    {
        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(!IsWeaponAttachmentUnlock(nullptr),
              "A7 a null object gets the game's answer, never a fabricated yes");
        CHECK(g_calls == 1,
              "A7 ... and the game's own function did the deciding");
    }

    {
        Config.ExtraMenu.Attachment = true;
        orig_IsWeaponAttachmentUnlock = nullptr;
        CHECK(IsWeaponAttachmentUnlock(&g_instance),
              "A8 with no original installed the grant still answers yes");
        CHECK(!IsWeaponAttachmentUnlock(nullptr),
              "A8 ... and a null receiver answers false, not a crash");
    }

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

