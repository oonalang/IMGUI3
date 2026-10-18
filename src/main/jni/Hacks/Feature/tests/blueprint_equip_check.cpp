// Host test for System/Hooks/BlueprintEquip.h (Equippable Blueprints).
//
// The five hooks themselves only run on-device, but everything they decide
// is host-testable: the bounded List<BlueprintWeaponInfo> walk the click
// observer reads the tapped entry through, the override gates' answers with
// the toggle on and off, the observers' strict pass-through, and the
// diagnostics counters the SKINS tab prints. Fake Il2Cpp objects below are
// laid out exactly like the real ones (List._items @0x10, List._size @0x18,
// Il2CppArray.max_length @0x18, elements @0x20; BlueprintWeaponInfo
// m_BlueprintWeaponId @0x24, m_SiWeapon @0x30; the popup controller's
// m_BlueprintWeaponInfoList @0x130 - all from the game dump, GR .57).
//
// It compiles the REAL Init_Hacks.h, so the Config.ExtraMenu.Blueprints
// switch the gates read is checked against the real config struct.
//
// Host test, no NDK needed (same stub set as placable_esp_check.cpp):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes -I src/main/jni/foxcheats/include \
//       src/main/jni/Hacks/Feature/tests/blueprint_equip_check.cpp \
//       -o /tmp/blueprint_equip_check && /tmp/blueprint_equip_check
// Exits 0 and prints "OK: ..." on success. Android.mk's wildcards do not
// cover Hacks/Feature/tests, so this file never reaches the APK.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "System/Hooks/BlueprintEquip.h"

static int g_failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_failures++;                                               \
        }                                                               \
    } while (0)

template <typename T>
static void Put(void *base, uintptr_t off, T v) { std::memcpy((char *)base + off, &v, sizeof(T)); }

// ---- fake game objects ------------------------------------------------------
alignas(16) static uint8_t g_info[3][0x40];        // BlueprintWeaponInfo x3
alignas(16) static uint8_t g_array[0x20 + 3 * 8];  // Il2CppArray of 3 entries
alignas(16) static uint8_t g_list[0x20];           // List<BlueprintWeaponInfo>
alignas(16) static uint8_t g_popup[0x140];         // popup controller
static int g_ownedSentinel;                        // any non-null address

static void BuildFakes()
{
    std::memset(g_info, 0, sizeof(g_info));
    std::memset(g_array, 0, sizeof(g_array));
    std::memset(g_list, 0, sizeof(g_list));
    std::memset(g_popup, 0, sizeof(g_popup));

    // Entry 0: config-only blueprint (no SIWeapon -> unowned).
    Put<uint32_t>(g_info[0], blueprint_equip::kInfoBlueprintId, 5001u);
    Put<uint32_t>(g_info[0], blueprint_equip::kInfoBaseId, 900u);
    Put<void *>(g_info[0], blueprint_equip::kInfoSiWeapon, nullptr);
    // Entries 1-2: owned blueprints of the SAME base gun.
    Put<uint32_t>(g_info[1], blueprint_equip::kInfoBlueprintId, 5002u);
    Put<uint32_t>(g_info[1], blueprint_equip::kInfoBaseId, 900u);
    Put<void *>(g_info[1], blueprint_equip::kInfoSiWeapon, (void *)&g_ownedSentinel);
    Put<uint32_t>(g_info[2], blueprint_equip::kInfoBlueprintId, 5003u);
    Put<uint32_t>(g_info[2], blueprint_equip::kInfoBaseId, 901u);
    Put<void *>(g_info[2], blueprint_equip::kInfoSiWeapon, (void *)&g_ownedSentinel);

    Put<uint64_t>(g_array, blueprint_equip::kArrayMaxLen, 3u);
    for (int i = 0; i < 3; i++)
        Put<void *>(g_array, blueprint_equip::kArrayData + (uintptr_t)i * 8, (void *)g_info[i]);

    Put<void *>(g_list, blueprint_equip::kListItems, (void *)g_array);
    Put<int>(g_list, blueprint_equip::kListSize, 3);

    Put<void *>(g_popup, blueprint_equip::kPopupBlueprintList, (void *)g_list);
}

// ---- fake originals -----------------------------------------------------------
static bool FakeForbidTrue(void *, uint32_t) { return true; }
static bool FakeCanEquipFalse(uint32_t, uint32_t, int, int, int, int) { return false; }
static uint32_t FakeSimilarZero(void *, uint32_t) { return 0; }
static uint32_t FakeSimilarOwned(void *, uint32_t in) { return in + 1000; }
static void *FakeSiNull(void *, uint32_t) { return nullptr; }
static bool FakeClickTrue(void *, void *, void *, int) { return true; }

static void ResetDiag()
{
    blueprint_equip::g_diag = blueprint_equip::DiagStats{};
    std::memset(blueprint_equip::g_landed, 0, sizeof(blueprint_equip::g_landed));
}

int main()
{
    using namespace blueprint_equip;
    BuildFakes();
    uint32_t id = 0;
    bool owned = false;

    // ---- the bounded list walk -------------------------------------------
    CHECK(ReadInfoAt(g_list, 0, &id, &owned) && id == 5001u && !owned);
    CHECK(ReadInfoAt(g_list, 1, &id, &owned) && id == 5002u && owned);
    CHECK(ReadInfoAt(g_list, 2, &id, &owned) && id == 5003u && owned);
    CHECK(!ReadInfoAt(g_list, 3, &id, &owned));    // index == size
    CHECK(!ReadInfoAt(g_list, -1, &id, &owned));   // negative index
    CHECK(!ReadInfoAt(nullptr, 0, &id, &owned));   // no list
    CHECK(ReadInfoAt(g_list, 1, nullptr, nullptr)); // out-params optional

    Put<void *>(g_list, kListItems, nullptr);
    CHECK(!ReadInfoAt(g_list, 0, &id, &owned));    // no backing array
    Put<void *>(g_list, kListItems, (void *)g_array);

    Put<int>(g_list, kListSize, 9);
    CHECK(!ReadInfoAt(g_list, 8, &id, &owned));    // size beyond array length
    Put<int>(g_list, kListSize, 3);

    Put<uint64_t>(g_array, kArrayMaxLen, 5000u);
    CHECK(!ReadInfoAt(g_list, 0, &id, &owned));    // absurd length ceiling
    Put<uint64_t>(g_array, kArrayMaxLen, 3u);

    Put<void *>(g_array, kArrayData + 8, nullptr);
    CHECK(!ReadInfoAt(g_list, 1, &id, &owned));    // null entry
    Put<void *>(g_array, kArrayData + 8, (void *)g_info[1]);

    // ---- the read from the popup controller -------------------------------
    CHECK(ReadClickedBlueprint(g_popup, 0, &id, &owned) && id == 5001u && !owned);
    CHECK(ReadClickedBlueprint(g_popup, 2, &id, &owned) && id == 5003u && owned);
    CHECK(!ReadClickedBlueprint(nullptr, 0, &id, &owned));
    CHECK(!ReadClickedBlueprint(g_popup, 7, &id, &owned));

    // ---- override gates -----------------------------------------------------
    ResetDiag();
    orig_IsSpecialForbid = &FakeForbidTrue;
    orig_IsWeaponCanEquipItem = &FakeCanEquipFalse;

    Config.ExtraMenu.Blueprints = true;
    CHECK(IsSpecialForbid((void *)&g_ownedSentinel, 5001u) == false);       // forbidden -> allowed
    CHECK(IsWeaponCanEquipItem(100u, 5001u, 0, -1, -1, 0) == true);        // refused -> allowed
    CHECK(g_diag.asked[GateForbid] == 1 && g_diag.granted[GateForbid] == 1);
    CHECK(g_diag.asked[GateCanEquip] == 1 && g_diag.granted[GateCanEquip] == 1);
    CHECK(g_diag.canEquipLastItemId == 5001u);

    Config.ExtraMenu.Blueprints = false;
    CHECK(IsSpecialForbid((void *)&g_ownedSentinel, 5001u) == true);        // stock answers back
    CHECK(IsWeaponCanEquipItem(100u, 5001u, 0, -1, -1, 0) == false);
    CHECK(g_diag.asked[GateForbid] == 2 && g_diag.granted[GateForbid] == 1); // asked, not granted
    CHECK(g_diag.asked[GateCanEquip] == 2 && g_diag.granted[GateCanEquip] == 1);

    // ---- observers never change an answer -----------------------------------
    ResetDiag();
    Config.ExtraMenu.Blueprints = true;  // even with the feature ON

    orig_GetOwnedMostSimilarWeaponID = &FakeSimilarZero;
    CHECK(GetOwnedMostSimilarWeaponID((void *)&g_ownedSentinel, 5001u) == 0u);
    CHECK(g_diag.similarLastIn == 5001u && g_diag.similarLastOut == 0u);
    CHECK(g_diag.asked[GateSimilar] == 1 && g_diag.granted[GateSimilar] == 0);

    orig_GetOwnedMostSimilarWeaponID = &FakeSimilarOwned;
    CHECK(GetOwnedMostSimilarWeaponID((void *)&g_ownedSentinel, 5001u) == 6001u);
    CHECK(g_diag.similarLastOut == 6001u && g_diag.granted[GateSimilar] == 1);

    orig_OnEasyListItemClick_Weapon = &FakeClickTrue;
    CHECK(OnEasyListItemClick_Weapon(g_popup, nullptr, nullptr, 0) == true);
    CHECK(g_diag.lastClickBlueprintId == 5001u && g_diag.lastClickOwned == 0);
    CHECK(g_diag.asked[GateClick] == 1 && g_diag.granted[GateClick] == 1);
    CHECK(OnEasyListItemClick_Weapon(g_popup, nullptr, nullptr, 1) == true);
    CHECK(g_diag.lastClickBlueprintId == 5002u && g_diag.lastClickOwned == 1);
    CHECK(g_diag.asked[GateClick] == 2 && g_diag.granted[GateClick] == 1);  // owned taps are not "granted"

    // ---- the OWNED-sIBLING resolver ------------------------------------------
    // Fresh capture: tap the unowned entry 0 through the popup, then ask the
    // store for a blueprint it cannot resolve (original answers 0).
    Config.ExtraMenu.Blueprints = true;
    blueprint_equip::SetOwnerSimilarResolver(&blueprint_equip::ResolveOwnedSibling);
    ResetDiag();
    orig_GetOwnedMostSimilarWeaponID = &FakeSimilarZero;
    CHECK(OnEasyListItemClick_Weapon(g_popup, nullptr, nullptr, 0) == true);
    CHECK(blueprint_equip::g_lastPopupController == g_popup);
    CHECK(blueprint_equip::g_lastClickedBaseId == 900u);   // entry 0's base gun
    CHECK(GetOwnedMostSimilarWeaponID((void *)&g_ownedSentinel, 5001u) == 5002u); // owned sibling of base 900
    CHECK(g_diag.similarLastOut == 5002u && g_diag.granted[GateSimilar] == 1);

    // The original's non-zero answer is still trusted over the resolver.
    orig_GetOwnedMostSimilarWeaponID = &FakeSimilarOwned;
    CHECK(GetOwnedMostSimilarWeaponID((void *)&g_ownedSentinel, 5001u) == 6001u);
    CHECK(g_diag.granted[GateSimilar] == 2);

    // No resolver installed -> stock answer (0), never granted.
    orig_GetOwnedMostSimilarWeaponID = &FakeSimilarZero;
    ResetDiag();
    blueprint_equip::SetOwnerSimilarResolver(nullptr);
    CHECK(GetOwnedMostSimilarWeaponID((void *)&g_ownedSentinel, 5001u) == 0u);
    CHECK(g_diag.granted[GateSimilar] == 0);
    Config.ExtraMenu.Blueprints = false;

    orig_GetSiWeaponByBluePrintWeaponId = &FakeSiNull;
    CHECK(GetSiWeaponByBluePrintWeaponId((void *)&g_ownedSentinel, 5001u) == nullptr);
    CHECK(g_diag.siNullResults == 1 && g_diag.asked[GateSiWeapon] == 1);

    // ---- counter bounds -------------------------------------------------------
    ResetDiag();
    NoteAsk(static_cast<Gate>(-1));
    NoteAsk(GateCount);
    NoteGrant(static_cast<Gate>(99));
    NoteLanded(static_cast<Gate>(-1), true);
    for (int i = 0; i < GateCount; i++) {
        CHECK(g_diag.asked[i] == 0 && g_diag.granted[i] == 0);
        CHECK(!g_landed[i]);
    }
    NoteLanded(GateCanEquip, true);
    CHECK(g_landed[GateCanEquip]);

    Config.ExtraMenu.Blueprints = false;
    if (g_failures == 0) {
        std::printf("OK: blueprint_equip_check - list walk, gates, observers, counters all pass\n");
        return 0;
    }
    std::printf("FAILED: %d check(s)\n", g_failures);
    return 1;
}
