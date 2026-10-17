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

    enum Gate
    {
        // Primary gates (distinct).
        GateClassifier = 0,
        GateItem       = 1,
        GateSelect     = 2,
        GateEquip      = 3,
        GateValid      = 4,
        // Aliases for the classifier gate.
        GateIsBlueprintWeaponID = 0,
        GateBlueprintWeaponID   = 0,
        GateWeaponID            = 0,
        GateBlueprintID         = 0,
        // Aliases for the item gate.
        GateMPItem              = 1,
        GateItemGate            = 1,
        GateIsThisItemCanBeEquipedInMP = 1,
        GateEquipMP             = 1,
        // Aliases for the select gate.
        GateSelectBlueprint     = 2,
        GateIsSelectBlueprintWeaponUnlock = 2,
        GateBlueprintSelection  = 2,
        GateSelection           = 2,
        // Aliases for the equip gate.
        GateCheckIsEquip        = 3,
        GateEquipGate           = 3,
        GateCheckEquip          = 3,
        // Aliases for the valid gate.
        GateBlueprintValid      = 4,
        GateIsBlueprintWeaponValid = 4,
        GateValidGate           = 4,
        GateCount               = 5
    };

    struct DiagStats
    {
        int asked[GateCount];
        int granted[GateCount];
        int seen[GateCount];
        int blocked[GateCount];
    };
    inline DiagStats g_diag{};

    inline bool g_landed[GateCount];

    inline void NoteLanded(Gate gate, bool landed)
    {
        if (landed && gate >= 0 && gate < GateCount) g_landed[gate] = true;
    }

    inline void NoteAsk(Gate gate)
    {
        if (gate >= 0 && gate < GateCount) {
            g_diag.asked[gate]++;
            g_diag.seen[gate]++;
        }
    }

inline void NoteGrant(Gate gate)
{
    if (gate >= 0 && gate < GateCount) {
        g_diag.granted[gate]++;
    }
}
}

inline bool (*orig_IsBlueprintWeaponID)(uint32_t weaponID) = nullptr;
inline bool IsBlueprintWeaponID(uint32_t weaponID)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateClassifier);
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
    // once per rendered frame. Recorded for EVERY id this gate asks about, so a
    // skin that needs no grant still reaches its gun.
    if (itemID != 0 && Config.ExtraMenu.AutoSkin)
        AutoEquipSkin_NoteCandidate(itemID);

    blueprint_equip::NoteAsk(blueprint_equip::GateItem);
    if (instance != nullptr && itemID != 0 && blueprint_equip::UnlockBlueprints() &&
        IsBlueprintWeaponID(itemID)) {
        blueprint_equip::NoteGrant(blueprint_equip::GateItem);
        return true;
    }
    return orig_IsThisItemCanBeEquipedInMP != nullptr
         ? orig_IsThisItemCanBeEquipedInMP(instance, itemID, bag)
         : false;
}

inline bool (*orig_IsSelectBlueprintWeaponUnlock)(void *instance) = nullptr;
inline bool IsSelectBlueprintWeaponUnlock(void *instance)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateSelect);
    if (instance != nullptr && blueprint_equip::UnlockBlueprints()) {
        blueprint_equip::NoteGrant(blueprint_equip::GateSelect);
        return true;
    }
    return orig_IsSelectBlueprintWeaponUnlock != nullptr
         ? orig_IsSelectBlueprintWeaponUnlock(instance)
         : false;
}

inline bool (*orig_CheckIsEquip)(void *instance) = nullptr;
inline bool CheckIsEquip(void *instance)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateEquip);
    if (instance != nullptr && blueprint_equip::UnlockBlueprints()) {
        blueprint_equip::NoteGrant(blueprint_equip::GateEquip);
        return true;
    }
    return orig_CheckIsEquip != nullptr ? orig_CheckIsEquip(instance) : false;
}

inline bool (*orig_IsBlueprintWeaponValid)(void *blueprintWeaponData) = nullptr;
inline bool IsBlueprintWeaponValid(void *blueprintWeaponData)
{
    blueprint_equip::NoteAsk(blueprint_equip::GateValid);
    if (blueprintWeaponData != nullptr && blueprint_equip::UnlockBlueprints()) {
        blueprint_equip::NoteGrant(blueprint_equip::GateValid);
        return true;
    }
    return orig_IsBlueprintWeaponValid != nullptr
         ? orig_IsBlueprintWeaponValid(blueprintWeaponData)
         : false;
}
