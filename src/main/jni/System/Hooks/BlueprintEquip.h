#pragma once

#include "Hacks/Feature/Init_Hacks.h"

// Apply-skin-on-equip: this header only records the item id the equip gates are
// asked about; the write happens per frame in Skin/AutoEquipSkin.h.
#include "Skin/AutoEquipState.h"

#include <cstdint>

namespace blueprint_equip
{
    namespace Rva
    {
        constexpr uintptr_t LoadoutHelper_IsThisItemCanBeEquipedInMP = 0x8D33914;
        constexpr uintptr_t LoadoutHelper_IsBlueprintWeaponID        = 0x8D49D00;
        constexpr uintptr_t Loadout_IsSelectBlueprintWeaponUnlock    = 0x5D30134;
        constexpr uintptr_t Loadout_CheckIsEquip                     = 0x5D31A4C;
        constexpr uintptr_t SafeHouse_IsBlueprintWeaponValid         = 0x6741CE4;
    }

    namespace Dump
    {
        constexpr int LoadoutHelper_IsThisItemCanBeEquipedInMP = 2069980;
        constexpr int LoadoutHelper_IsBlueprintWeaponID        = 2070514;
        constexpr int Loadout_IsSelectBlueprintWeaponUnlock    = 2045536;
        constexpr int Loadout_CheckIsEquip                     = 2045575;
        constexpr int SafeHouse_IsBlueprintWeaponValid         = 2068679;
    }

    inline bool UnlockBlueprints()
    {
        return Config.ExtraMenu.Blueprints;
    }

    // BagIdentifier is a 12-byte value type passed by value (dump.cs 2285560).
    struct BagIdentifierRaw
    {
        int32_t bag_type;
        int32_t mode_type;
        int32_t index;
    };
}

inline bool (*orig_IsBlueprintWeaponID)(uint32_t weaponID) = nullptr;
inline bool IsBlueprintWeaponID(uint32_t weaponID)
{
    return orig_IsBlueprintWeaponID != nullptr && orig_IsBlueprintWeaponID(weaponID);
}

inline bool (*orig_IsThisItemCanBeEquipedInMP)(void *instance, uint32_t itemID,
                                              blueprint_equip::BagIdentifierRaw bag) = nullptr;
inline bool IsThisItemCanBeEquipedInMP(void *instance, uint32_t itemID,
                                       blueprint_equip::BagIdentifierRaw bag)
{
    // The item being put in a bag is named here and nowhere else at equip time,
    // so this is where the "apply the skin I just equipped" pick is taken from.
    // Nothing is written from inside this hook: Skin/AutoEquipSkin.h does that
    // once per rendered frame.
    if (itemID != 0 && Config.ExtraMenu.AutoSkin)
        AutoEquipSkin_NoteCandidate(itemID);

    if (itemID != 0 && blueprint_equip::UnlockBlueprints() && IsBlueprintWeaponID(itemID))
        return true;
    return orig_IsThisItemCanBeEquipedInMP != nullptr
         ? orig_IsThisItemCanBeEquipedInMP(instance, itemID, bag)
         : false;
}

inline bool (*orig_IsSelectBlueprintWeaponUnlock)(void *instance) = nullptr;
inline bool IsSelectBlueprintWeaponUnlock(void *instance)
{
    if (instance != nullptr && blueprint_equip::UnlockBlueprints())
        return true;
    return orig_IsSelectBlueprintWeaponUnlock != nullptr
         ? orig_IsSelectBlueprintWeaponUnlock(instance)
         : false;
}

inline bool (*orig_CheckIsEquip)(void *instance) = nullptr;
inline bool CheckIsEquip(void *instance)
{
    if (instance != nullptr && blueprint_equip::UnlockBlueprints())
        return true;
    return orig_CheckIsEquip != nullptr ? orig_CheckIsEquip(instance) : false;
}

inline bool (*orig_IsBlueprintWeaponValid)(void *blueprintWeaponData) = nullptr;
inline bool IsBlueprintWeaponValid(void *blueprintWeaponData)
{
    if (blueprintWeaponData != nullptr && blueprint_equip::UnlockBlueprints())
        return true;
    return orig_IsBlueprintWeaponValid != nullptr
         ? orig_IsBlueprintWeaponValid(blueprintWeaponData)
         : false;
}
