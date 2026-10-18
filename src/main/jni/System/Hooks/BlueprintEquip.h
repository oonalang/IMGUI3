#pragma once

// ============================================================================
//  BlueprintEquip.h  -  "Equippable Blueprints": picking ANY blueprint in the
//  blueprint selection applies it, not just the ones the account owns.
//
//  What shipped before this header (and what other menus sell) is a single
//  hook: BlueprintWeaponInfo.IsUnlocked() @0x901F988 answers true, so the
//  selection list draws every blueprint without a lock. Tapping Equip then
//  fails with "Weapon does not exist. (11305)". That message's code appears
//  NOWHERE in the client dump - it is not a client-side constant - so the
//  refusal is either a server verdict on the loadout save, or a client gate
//  whose verdict the UI maps to that tip through a data table. This header
//  covers the client-side verdicts the equip path can ask, and - at least as
//  important - it MEASURES the path, because (see MythicAttachments.h) the
//  small getters of this build tend to be inlined into their callers and a
//  hook nobody calls is indistinguishable from a hook that grants nothing.
//
//  OVERRIDE gates (answer changed while Config.ExtraMenu.Blueprints is on;
//  the original is always the fallback while it is off):
//
//   1. WeaponDataStore.IsSpecialForbid(uint blueprintWeaponId)
//        RVA 0x6DCE594  (dump.cs, WeaponDataStore @2049736)
//        "is this blueprint specially forbidden" - a hard block list the
//        equip flow consults. Answered false: nothing is forbidden.
//   2. LoadoutWeaponsUIHelper.IsWeaponCanEquipItem(uint weaponID,
//        uint itemID, EGameType, int loadoutIndex, int blueprintIndex,
//        int weaponType)
//        RVA 0x5D34E50  (dump.cs, LoadoutWeaponsUIHelper @2072394)
//        THE "can this be equipped" validator of the loadout UI - the same
//        shape as IsThisAttachmentUnlock, which the mythic system already
//        answers true. Answered true.
//        ABI note: this one is STATIC, so it is declared with exactly the
//        dump's parameters. (The mythic header declares its static gate
//        with a leading instance slot; passing every argument straight
//        through makes that transparent, but this feature READS itemID, so
//        the declaration here keeps the game's own argument order.)
//
//  OBSERVER hooks (never change an answer - they only count, so one device
//  session shows exactly where an equip dies):
//
//   3. WeaponDataStore.GetOwnedMostSimilarWeaponID(uint bluePrintWeaponId)
//        RVA 0x6DCCFD0 - resolves a blueprint to the OWNED weapon the equip
//        can actually use; a 0 here for a tapped blueprint is a prime
//        suspect for "Weapon does not exist". Records last in/out.
//   4. WeaponDataStore.GetSiWeaponByBluePrintWeaponId(uint bluePrintWeaponId)
//        RVA 0x6DCD3FC - the owned inventory weapon behind a blueprint;
//        counts how often it comes back null.
//   5. LoadoutWeaponsQuicklySelectPopupController.
//        OnEasyListItemClick_Weapon(list, controller, index)
//        RVA 0x5D2D420  (dump.cs @1168996) - the tap itself, on the blueprint
//        selection popup. The tapped BlueprintWeaponInfo is read out of the
//        controller's own m_BlueprintWeaponInfoList @0x130 (dump.cs
//        1169014; fields m_BlueprintWeaponId @0x24, m_SiWeapon @0x30 - an
//        entry built from server data has one, a config-only entry does
//        not) BEFORE the original runs, so even a tap the game refuses is
//        on record: last tapped blueprint id + whether it was owned.
//
//  Gate 0 of the diagnostics is the pre-existing IsUnlocked hook in Feats.h,
//  which counts into the same table.
//
//  WHAT THIS IS NOT: like the mythic system, these are CLIENT answers, not
//  server grants. If the counters show canEquip asked AND granted and the
//  game still answers 11305, the verdict is the server's, and the next layer
//  is a local record write (the equipped skin id lives in the client's own
//  WeaponAttrConf record: attr_conf @0x38 -> LoadoutWeaponAttr.skin @0x80 ->
//  EquippedItem.id @0x3C) driven by the click observer above. The counters
//  on the SKINS tab say which world we are in; nothing here guesses.
//
//  Config: Config.ExtraMenu.Blueprints - the same SKINS-tab switch that
//  already drives IsUnlocked, so one toggle is the whole feature, and off
//  restores stock behaviour exactly (every body falls back to its original).
//
//  Include contract: same as Feats.h / MythicAttachments.h - this header
//  uses sConfig Config, so it is included after the game structs; it
//  includes Init_Hacks.h so a host test can pull just this file.
// ============================================================================

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace blueprint_equip
{
    // The addresses, straight from the dump (RVA / Offset pairs as printed by
    // dump.cs, i.e. what getAbsoluteAddress() wants).
    namespace Rva
    {
        constexpr uintptr_t IsUnlocked              = 0x901F988;  // gate 0, Feats.h
        constexpr uintptr_t IsSpecialForbid         = 0x6DCE594;
        constexpr uintptr_t IsWeaponCanEquipItem    = 0x5D34E50;
        constexpr uintptr_t GetOwnedMostSimilar     = 0x6DCCFD0;
        constexpr uintptr_t GetSiWeaponByBlueprint  = 0x6DCD3FC;
        constexpr uintptr_t OnBlueprintItemClick    = 0x5D2D420;
    }

    // The questions / observation points, as an index. Used by the counters
    // below and by the SKINS tab's diagnostic lines.
    enum Gate
    {
        GateUnlock = 0,  // BlueprintWeaponInfo.IsUnlocked (hooked in Feats.h)
        GateForbid,      // WeaponDataStore.IsSpecialForbid          -> override
        GateCanEquip,    // LoadoutWeaponsUIHelper.IsWeaponCanEquipItem -> override
        GateSimilar,     // WeaponDataStore.GetOwnedMostSimilarWeaponID -> observe
        GateSiWeapon,    // WeaponDataStore.GetSiWeaponByBluePrintWeaponId -> observe
        GateClick,       // QuicklySelectPopup.OnEasyListItemClick_Weapon -> observe
        GateCount
    };

    // On-device instrument. The loadout asks these questions by itself when it
    // draws and when a blueprint is tapped, so the SKINS tab prints how often
    // each point was asked and how often this feature granted it. For the
    // observers "granted" means the game got a usable answer on its own
    // (similar resolved non-zero / SI weapon found / an UNOWNED blueprint was
    // tapped) - it marks the events worth looking at, not an override.
    struct DiagStats
    {
        int asked[GateCount];
        int granted[GateCount];
        uint32_t lastClickBlueprintId;  // GateClick: blueprint id of the last tap
        int      lastClickOwned;        // GateClick: 1 if that entry had an SIWeapon
        uint32_t canEquipLastItemId;    // GateCanEquip: itemID of the last question
        uint32_t similarLastIn;         // GateSimilar: blueprint id asked about
        uint32_t similarLastOut;        // GateSimilar: owned weapon id it resolved to
        int      siNullResults;         // GateSiWeapon: lookups that found nothing
    };
    inline DiagStats g_diag{};

    // Install-time truth, one flag per gate: Feats.h sets it right after the
    // hook call from whether the hooker filled in the original pointer. A 0
    // here is the reason the counters above stay at 0/0 - the address never
    // took the hook (a dump that no longer matches the installed game, or a
    // failed install), which is the first thing to check after a game update.
    inline bool g_landed[GateCount];
    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void NoteAsk(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.asked[gate]++;
    }

    inline void NoteGrant(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) g_diag.granted[gate]++;
    }

    // The one switch for the whole feature - the SKINS tab's existing
    // "Unlock Blueprints" toggle.
    inline bool Enabled()
    {
        return Config.ExtraMenu.Blueprints;
    }

    // ---- field layout of the client's own objects --------------------------
    // IL2CPP objects carry a 16-byte header, so every dump offset below is
    // the ABSOLUTE offset from the object pointer - the same convention (and
    // the same List<T> / array layout) MythicAttachments.h walks.
    constexpr uintptr_t kPopupBlueprintList = 0x130;  // m_BlueprintWeaponInfoList
    constexpr uintptr_t kInfoBlueprintId    = 0x24;   // BlueprintWeaponInfo.m_BlueprintWeaponId
    constexpr uintptr_t kInfoSiWeapon       = 0x30;   // BlueprintWeaponInfo.m_SiWeapon
    constexpr uintptr_t kListItems          = 0x10;   // List<T>._items
    constexpr uintptr_t kListSize           = 0x18;   // List<T>._size
    constexpr uintptr_t kArrayMaxLen        = 0x18;   // Il2CppArray.max_length
    constexpr uintptr_t kArrayData          = 0x20;   // Il2CppArray[0]

    // Read entry `index` of a List<BlueprintWeaponInfo>: its blueprint id and
    // whether it is an owned entry (server-built entries carry their SIWeapon;
    // config-only entries do not). Every size is validated against the backing
    // array's OWN length (and the same hard 4096 ceiling the mythic list walk
    // uses), so a list that is not what the dump says it is stops the read
    // instead of walking off into the heap. Pure pointer arithmetic over
    // caller-owned memory - this is the part the host test drives.
    inline bool ReadInfoAt(void *list, int index, uint32_t *outId, bool *outOwned)
    {
        if (list == nullptr || index < 0) return false;
        void *items = *(void **)((char *)list + kListItems);
        if (items == nullptr) return false;

        const int64_t size = *(int *)((char *)list + kListSize);
        const uint64_t maxLen = *(uint64_t *)((char *)items + kArrayMaxLen);
        if (size <= 0 || maxLen == 0 || maxLen > 4096) return false;
        if (index >= size || (uint64_t)index >= maxLen) return false;

        void *info = ((void **)((char *)items + kArrayData))[index];
        if (info == nullptr) return false;
        if (outId != nullptr)    *outId    = *(uint32_t *)((char *)info + kInfoBlueprintId);
        if (outOwned != nullptr) *outOwned = *(void **)((char *)info + kInfoSiWeapon) != nullptr;
        return true;
    }

    // The same read, starting from the selection popup controller itself.
    inline bool ReadClickedBlueprint(void *popupController, int index,
                                     uint32_t *outId, bool *outOwned)
    {
        if (popupController == nullptr) return false;
        void *list = *(void **)((char *)popupController + kPopupBlueprintList);
        return ReadInfoAt(list, index, outId, outOwned);
    }
}

// ---- 1. the special-forbid block list --------------------------------------
// dump.cs, WeaponDataStore @2049736: bool IsSpecialForbid(uint
// blueprintWeaponId). An instance method, so the replacement takes (this,
// blueprintWeaponId) and hands both back to the original.
inline bool (*orig_IsSpecialForbid)(void *instance, uint32_t blueprintWeaponId) = nullptr;
inline bool IsSpecialForbid(void *instance, uint32_t blueprintWeaponId)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateForbid);
    if (instance != nullptr && blueprint_equip::Enabled()) {
        blueprint_equip::NoteGrant(blueprint_equip::GateForbid);
        return false;
    }
    return orig_IsSpecialForbid != nullptr
         ? orig_IsSpecialForbid(instance, blueprintWeaponId)
         : false;
}

// ---- 2. the loadout's equip validator ---------------------------------------
// dump.cs, LoadoutWeaponsUIHelper @2072394: static bool IsWeaponCanEquipItem(
// uint weaponID, uint itemID, EGameType gameType = 0, int loadoutIndex = -1,
// int blueprintIndex = -1, int weaponType = 0). STATIC: declared with the
// dump's own parameter list (no instance slot) because this hook reads
// itemID - the mythic header's leading-instance convention is transparent
// only for arguments that are passed through unread.
inline bool (*orig_IsWeaponCanEquipItem)(uint32_t weaponID, uint32_t itemID,
                                         int gameType, int loadoutIndex,
                                         int blueprintIndex, int weaponType) = nullptr;
inline bool IsWeaponCanEquipItem(uint32_t weaponID, uint32_t itemID,
                                 int gameType, int loadoutIndex,
                                 int blueprintIndex, int weaponType)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateCanEquip);
    blueprint_equip::g_diag.canEquipLastItemId = itemID;
    if (blueprint_equip::Enabled()) {
        blueprint_equip::NoteGrant(blueprint_equip::GateCanEquip);
        return true;
    }
    return orig_IsWeaponCanEquipItem != nullptr
         ? orig_IsWeaponCanEquipItem(weaponID, itemID, gameType, loadoutIndex, blueprintIndex, weaponType)
         : false;
}

// ---- 3. OBSERVER: blueprint -> owned weapon resolution -----------------------
// dump.cs, WeaponDataStore @2049736: uint GetOwnedMostSimilarWeaponID(uint
// bluePrintWeaponId). Never overridden: the tap is recorded, the game's own
// answer is returned untouched. similarLastOut == 0 after tapping an unowned
// blueprint is the "Weapon does not exist" suspect.
inline uint32_t (*orig_GetOwnedMostSimilarWeaponID)(void *instance, uint32_t bluePrintWeaponId) = nullptr;
inline uint32_t GetOwnedMostSimilarWeaponID(void *instance, uint32_t bluePrintWeaponId)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateSimilar);
    const uint32_t result = orig_GetOwnedMostSimilarWeaponID != nullptr
                          ? orig_GetOwnedMostSimilarWeaponID(instance, bluePrintWeaponId)
                          : 0;
    blueprint_equip::g_diag.similarLastIn  = bluePrintWeaponId;
    blueprint_equip::g_diag.similarLastOut = result;
    if (result != 0) blueprint_equip::NoteGrant(blueprint_equip::GateSimilar);
    return result;
}

// ---- 4. OBSERVER: blueprint -> owned inventory weapon -------------------------
// dump.cs, WeaponDataStore @2049736: SIWeapon GetSiWeaponByBluePrintWeaponId(
// uint bluePrintWeaponId). Never overridden; counts the nulls.
inline void *(*orig_GetSiWeaponByBluePrintWeaponId)(void *instance, uint32_t bluePrintWeaponId) = nullptr;
inline void *GetSiWeaponByBluePrintWeaponId(void *instance, uint32_t bluePrintWeaponId)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateSiWeapon);
    void *result = orig_GetSiWeaponByBluePrintWeaponId != nullptr
                 ? orig_GetSiWeaponByBluePrintWeaponId(instance, bluePrintWeaponId)
                 : nullptr;
    if (result == nullptr) blueprint_equip::g_diag.siNullResults++;
    else                   blueprint_equip::NoteGrant(blueprint_equip::GateSiWeapon);
    return result;
}

// ---- 5. OBSERVER: the tap on the blueprint selection popup ---------------------
// dump.cs @1168996: bool OnEasyListItemClick_Weapon([Out] object list, out
// object controller, int index). Never overridden: the tapped entry is read
// out of the controller's own list BEFORE the original runs (a refused tap
// is still recorded), then the game's handler runs exactly as stock.
inline bool (*orig_OnEasyListItemClick_Weapon)(void *instance, void *list, void *controller, int index) = nullptr;
inline bool OnEasyListItemClick_Weapon(void *instance, void *list, void *controller, int index)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateClick);
    uint32_t blueprintId = 0;
    bool owned = false;
    if (blueprint_equip::ReadClickedBlueprint(instance, index, &blueprintId, &owned)) {
        blueprint_equip::g_diag.lastClickBlueprintId = blueprintId;
        blueprint_equip::g_diag.lastClickOwned = owned ? 1 : 0;
        if (!owned) blueprint_equip::NoteGrant(blueprint_equip::GateClick);
    }
    return orig_OnEasyListItemClick_Weapon != nullptr
         ? orig_OnEasyListItemClick_Weapon(instance, list, controller, index)
         : false;
}
