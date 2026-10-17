// Host test for System/Hooks/BlueprintEquip.h: OFF forwards to the game, ON
// grants blueprints, null receivers and missing originals fail closed, and the
// 12-byte BagIdentifier argument survives the hop.
// Run it with src/main/jni/host_checks.sh.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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
#include "System/Hooks/BlueprintEquip.h"

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

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

static const uint32_t kBlueprintId = 0x7A1201u;
static const uint32_t kPlainId     = 0x1F4u;

struct Calls
{
    int classifier = 0, itemGate = 0, selectGate = 0, equipGate = 0, validGate = 0;
    uint32_t lastWeaponID = 0;
    uint32_t lastItemID = 0;
    int lastBag[3] = { 0, 0, 0 };
    void reset()
    {
        classifier = itemGate = selectGate = equipGate = validGate = 0;
        lastWeaponID = 0; lastItemID = 0;
        lastBag[0] = lastBag[1] = lastBag[2] = 0;
    }
};
static Calls g_calls;
static int g_instance = 0;
static int g_blueprintData = 0;

static blueprint_equip::BagIdentifierRaw makeBag(int a, int b, int c)
{
    blueprint_equip::BagIdentifierRaw bag;
    bag.bag_type = a; bag.mode_type = b; bag.index = c;
    return bag;
}

static bool fake_origClassifier(uint32_t weaponID)
{
    g_calls.classifier++;
    g_calls.lastWeaponID = weaponID;
    return weaponID == kBlueprintId;
}
static bool fake_origItemGate(void *, uint32_t itemID, blueprint_equip::BagIdentifierRaw bag)
{
    g_calls.itemGate++;
    g_calls.lastItemID = itemID;
    g_calls.lastBag[0] = bag.bag_type;
    g_calls.lastBag[1] = bag.mode_type;
    g_calls.lastBag[2] = bag.index;
    return false;
}
static bool fake_origSelectGate(void *)
{
    g_calls.selectGate++;
    return false;
}
static bool fake_origEquipGate(void *)
{
    g_calls.equipGate++;
    return false;
}
static bool fake_origValidGate(void *)
{
    g_calls.validGate++;
    return false;
}

static void installFakes()
{
    orig_IsBlueprintWeaponID          = fake_origClassifier;
    orig_IsThisItemCanBeEquipedInMP   = fake_origItemGate;
    orig_IsSelectBlueprintWeaponUnlock = fake_origSelectGate;
    orig_CheckIsEquip                 = fake_origEquipGate;
    orig_IsBlueprintWeaponValid       = fake_origValidGate;
    g_calls.reset();
}

static void setUnlock(bool on)
{
    Config.ExtraMenu.Blueprints = on;
    g_calls.reset();
}

int main()
{
    const blueprint_equip::BagIdentifierRaw bag = makeBag(11, 22, 33);

    // ---- B1: OFF forwards everything to the game -----------------------------
    setUnlock(false);
    installFakes();
    CHECK(IsBlueprintWeaponID(kBlueprintId) == true,
          "B1 the classifier still answers yes for a blueprint id");
    CHECK(g_calls.classifier == 1, "B1 the classifier runs while the toggle is off");
    g_calls.reset();
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kBlueprintId, bag) == false,
          "B1 the item gate keeps the game's answer while the toggle is off");
    CHECK(g_calls.itemGate == 1 && g_calls.lastItemID == kBlueprintId,
          "B1 the item gate runs once, with the item the game asked about");
    CHECK(IsSelectBlueprintWeaponUnlock(&g_instance) == false,
          "B1 the select gate keeps the game's answer while the toggle is off");
    CHECK(IsBlueprintWeaponValid(&g_blueprintData) == false,
          "B1 the valid gate keeps the game's answer while the toggle is off");
    CHECK(CheckIsEquip(&g_instance) == false,
          "B1 the equip gate keeps the game's answer while the toggle is off");
    CHECK(g_calls.selectGate == 1 && g_calls.equipGate == 1 && g_calls.validGate == 1,
          "B1 every gate ran exactly once while the toggle was off");

    // ---- B2: ON answers a blueprint without running the original -------------
    setUnlock(true);
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kBlueprintId, bag) == true,
          "B2 a blueprint item is equippable in MP");
    CHECK(g_calls.itemGate == 0,
          "B2 the blueprint answer does not run the game's function");

    // ---- B3: ON still defers for non-blueprint items -------------------------
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kPlainId, bag) == false,
          "B3 a non-blueprint item keeps the game's answer");
    CHECK(g_calls.itemGate == 1 && g_calls.lastItemID == kPlainId,
          "B3 the non-blueprint item reached the original");
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, 0, bag) == false,
          "B3 a zero item id is never answered as a blueprint");
    CHECK(g_calls.itemGate == 2, "B3 the zero item id reached the original");
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kBlueprintId, bag) == true &&
          g_calls.itemGate == 2,
          "B3 the gate keeps granting blueprints after a deferral");

    CHECK(IsBlueprintWeaponValid(&g_blueprintData) == true,
          "B3 a valid blueprint info stays valid");
    CHECK(g_calls.validGate == 0, "B3 the valid gate did not run for a blueprint");
    CHECK(IsBlueprintWeaponValid(nullptr) == false,
          "B3 a null blueprint info is never answered as valid");
    CHECK(g_calls.validGate == 1, "B3 the null blueprint info reached the original");

    // ---- B4: the argument reaches the original unchanged ---------------------
    setUnlock(true);
    IsThisItemCanBeEquipedInMP(&g_instance, kPlainId, bag);
    CHECK(g_calls.lastBag[0] == 11 && g_calls.lastBag[1] == 22 && g_calls.lastBag[2] == 33,
          "B4 the 12-byte BagIdentifier arrives unchanged");

    // ---- B5: null receivers are never answered true --------------------------
    setUnlock(true);
    CHECK(IsSelectBlueprintWeaponUnlock(nullptr) == false,
          "B5 a null select receiver is not granted");
    CHECK(g_calls.selectGate == 1, "B5 the null select receiver reached the original");
    CHECK(CheckIsEquip(nullptr) == false,
          "B5 a null equip receiver is not granted");
    CHECK(g_calls.equipGate == 1, "B5 the null equip receiver reached the original");

    // ---- B6: with the originals missing, nothing is dereferenced -------------
    setUnlock(true);
    orig_IsBlueprintWeaponID          = nullptr;
    orig_IsThisItemCanBeEquipedInMP   = nullptr;
    orig_IsSelectBlueprintWeaponUnlock = nullptr;
    orig_CheckIsEquip                 = nullptr;
    orig_IsBlueprintWeaponValid       = nullptr;
    g_calls.reset();
    CHECK(IsBlueprintWeaponID(kBlueprintId) == false,
          "B6 the classifier answers no when its original is not installed");
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kBlueprintId, bag) == false,
          "B6 the item gate fails closed when the classifier is not installed");
    CHECK(IsThisItemCanBeEquipedInMP(&g_instance, kPlainId, bag) == false,
          "B6 a non-blueprint item answers no when the original is not installed");
    CHECK(g_calls.itemGate == 0,
          "B6 nothing in the item gate ran the game's function after the null guard");
    CHECK(IsSelectBlueprintWeaponUnlock(&g_instance) == true,
          "B6 the select gate grants without the original");
    CHECK(CheckIsEquip(&g_instance) == true,
          "B6 the equip gate grants without the original");
    CHECK(IsBlueprintWeaponValid(&g_blueprintData) == true,
          "B6 the valid gate grants a real blueprint info without the original");

    // ---- B7: the toggle default ----------------------------------------------
    sConfig fresh{};
    CHECK(!fresh.ExtraMenu.Blueprints, "B7 the blueprint switch is off by default");

    Config.ExtraMenu.Blueprints = false;
    installFakes();

    if (g_failures == 0)
        printf("OK: all blueprint-equip checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
