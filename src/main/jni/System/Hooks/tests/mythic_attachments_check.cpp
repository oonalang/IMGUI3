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

static_assert(std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*, int>::value,
              "IsWeaponAttachmentUnlock takes the instance and attachment ID");
static_assert(!std::is_invocable_r<bool, decltype(&IsWeaponAttachmentUnlock), void*>::value,
              "IsWeaponAttachmentUnlock requires an attachment ID");

static int  g_calls = 0;
static void *g_lastInstance = nullptr;
static int g_lastAttachment = -1;
static bool g_origAnswer = false;

static bool fake_origAttachmentUnlock(void *instance, int attachment)
{
    g_calls++;
    g_lastInstance = instance;
    g_lastAttachment = attachment;
    return g_origAnswer;
}

static bool (*installFake())(void *, int)
{
    orig_IsWeaponAttachmentUnlock = fake_origAttachmentUnlock;
    g_calls = 0;
    g_lastInstance = nullptr;
    g_lastAttachment = -1;
    g_origAnswer = false;
    return fake_origAttachmentUnlock;
}

int main()
{
    g_premiumFeaturesEnabled = true;

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
        bool (*typed)(void *, int) = installFake();
        CHECK(typed == orig_IsWeaponAttachmentUnlock,
              "A3 original receives the instance and attachment ID");
    }

    static int g_instance = 0;

    {
        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(IsWeaponAttachmentUnlock(&g_instance, 607011),
              "A4 ON: the attachment counts as owned");
        CHECK(g_calls == 0,
              "A4 ON: nothing native runs to produce that answer");
    }

    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        CHECK(!IsWeaponAttachmentUnlock(&g_instance, 607011),
              "A5 OFF: the game's own verdict is returned");
        CHECK(g_calls == 1 && g_lastInstance == &g_instance && g_lastAttachment == 607011,
              "A5 OFF: forwards both the receiver and attachment ID");
    }

    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        IsWeaponAttachmentUnlock(&g_instance, 607012);
        CHECK(g_lastAttachment == 607012,
              "A5 OFF: different attachment IDs arrive unmodified");
    }

    {
        Config.ExtraMenu.Attachment = false;
        installFake();
        g_origAnswer = true;
        CHECK(IsWeaponAttachmentUnlock(&g_instance, 607011),
              "A6 OFF: a genuine unlock still comes through");
        CHECK(g_calls == 1,
              "A6 OFF: ... and it was the game that said so");
    }

    {

        Config.ExtraMenu.Attachment = true;
        installFake();
        CHECK(IsWeaponAttachmentUnlock(nullptr, 607011),
              "A7 a static call (null receiver) is granted while the switch is on");
        CHECK(g_calls == 0,
              "A7 ... without asking the game first");
    }

    {
        Config.ExtraMenu.Attachment = true;
        orig_IsWeaponAttachmentUnlock = nullptr;
        CHECK(IsWeaponAttachmentUnlock(&g_instance, 607011),
              "A8 with no original installed the grant still answers yes");
        CHECK(IsWeaponAttachmentUnlock(nullptr, 607011),
              "A8 ... the static call included");
    }

    {
        Config.ExtraMenu.Attachment = false;
        Config.ExtraMenu.MythicAttachments = true;
        installFake();
        CHECK(IsWeaponAttachmentUnlock(nullptr, 607011),
              "A9 the blueprint switch also unlocks attachments");
        CHECK(g_calls == 0, "A9 ... without a native call");
        Config.ExtraMenu.MythicAttachments = false;
    }

    {
        Config.ExtraMenu.Attachment = true;
        CHECK(AttachmentsUnlocked(), "A10 AttachmentsUnlocked() follows the switch");
        Config.ExtraMenu.Attachment = false;
        CHECK(!AttachmentsUnlocked(), "A10 ... in both directions");
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

