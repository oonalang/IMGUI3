#pragma once

#include "AutoEquipState.h"

// Skin/Fields.h is a fragment without an include guard (it is included exactly
// once per translation unit, in the order Fields.h -> Thread.h -> Camo.h), so it
// is NOT included here. Callers include it first, the same way Camo.h is used.

// ---------------------------------------------------------------------------
// Apply the equipped skin, not the searched one.
//
// Until now a skin only reached the weapon config table when the player found it
// in the SKINS tab and pressed it: every skin hit `DefWeaponSkinID` through the
// search box. The game's own equip path already knows which skin is being put on
// which gun - it just never tells the config table anything the account does not
// own, and the server rejects the request ("Weapon does not exist (11035)").
//
// So the pick is taken from the equip gates instead
// (System/Hooks/BlueprintEquip.h -> AutoEquipSkin_NoteCandidate), and this file
// turns that item id into the local write:
//
//   item id --(its own WeaponConfConfig row, BaseWeaponId @0x34)--> base gun
//           --(every row whose ID is that base gun)--> DefWeaponSkinID = item id
//
// which is exactly the field the mythic-skin path already proves works on
// device. Nothing here touches the network, the account or the server: it is the
// same local render override, only triggered by the equip instead of the search.
//
// `weaponSkinConfigInstance` is the second opinion for skins whose WeaponConfConfig
// row does not name a base weapon: a WeaponSkinConfig row carries the pair
// directly (ID @0x24, WeaponID @0x28).
struct AutoEquipSkinDiag
{
    int candidates;   // equip-gate calls that carried an item id
    int resolved;     // item ids that mapped to a base gun
    int writes;       // conf fields written in total
    int unresolved;   // item ids dropped after the retry window
    int notSkin;      // ids that named a gun rather than a skin
    uint lastID;      // the last id that resolved
    int  lastBase;    // the gun it was put on
};

inline AutoEquipSkinDiag g_autoEquipSkinDiag{};

// Resolve one item id to the base gun it belongs to and put the skin on that gun.
// Returns how many conf fields were written (0 when the gun already carried it),
// and reports the base gun through `outBase` (0 = the id is not a skin).
// Pure over `confs` + `weaponSkinConfigInstance`, so the host test drives the
// identical code (Skin/tests/auto_equip_check.cpp).
inline int AutoEquipSkin_ApplyToBase(uint skinID,
                                     const std::vector<void *> &confs,
                                     uint &outBase)
{
    outBase = 0;
    if (skinID == 0)
        return 0;

    uint baseID = 0;
    bool rowSeen = false;

    for (void *conf : confs)
    {
        if (!conf || !Tools::IsPtrValid(conf))
            continue;
        auto *cf = (WeaponConfFields *)((uintptr_t)conf + 0x20);
        if (!Tools::IsPtrValid(cf) || cf->ID != skinID)
            continue;
        rowSeen = true;
        baseID = cf->BaseWeaponId;   // a row that names itself is a base gun
        break;
    }

    if (!rowSeen || baseID == 0 || baseID == skinID)
    {
        // Same rule as the conf table: this table is filled by the skin-config
        // ctor hooks on the game thread, so it is walked as a locked copy.
        std::vector<void *> skinRows;
        {
            std::lock_guard<std::mutex> lock(g_dataMutex);
            skinRows = weaponSkinConfigInstance;
        }
        for (void *skin : skinRows)
        {
            if (!skin || !Tools::IsPtrValid(skin))
                continue;
            auto *sf = (WeaponSkinFields *)((uintptr_t)skin + 0x10);
            if (!Tools::IsPtrValid(sf) || sf->ID != skinID)
                continue;
            if (sf->WeaponID != 0 && sf->WeaponID != skinID)
                baseID = sf->WeaponID;
            break;
        }
    }

    if (baseID == 0)
        return 0;                    // no row for this id, or no base named
    if (baseID == skinID)
    {
        g_autoEquipSkinDiag.notSkin++;   // the id IS a gun: nothing to put on it
        return 0;
    }

    outBase = baseID;

    int written = 0;
    for (void *conf : confs)
    {
        if (!conf || !Tools::IsPtrValid(conf))
            continue;
        auto *cf = (WeaponConfFields *)((uintptr_t)conf + 0x20);
        if (!Tools::IsPtrValid(cf) || cf->ID != baseID)
            continue;
        if (cf->DefWeaponSkinID == (int)skinID)
            continue;                // already worn - no per-frame writes
        cf->DefWeaponSkinID = (int)skinID;
        written++;
    }
    return written;
}

// Once per rendered frame (Main.cpp, next to KillDecoration_ApplyLocalInventoryInfo).
inline void AutoEquipSkin_Tick(bool enabled)
{
    if (!enabled)
    {
        if (g_autoEquipSkinPendingID != 0)
        {
            g_autoEquipSkinPendingID = 0;
            g_autoEquipSkinPendingFrames = 0;
        }
        return;
    }
    if (g_autoEquipSkinPendingID == 0)
        return;

    const uint id = g_autoEquipSkinPendingID;
    // A LOCKED COPY, never the live table: the WeaponConfConfig ctor hooks
    // append to it on the game thread, and joining a match builds hundreds of
    // confs in a burst - walking the live vector through that burst is a
    // realloc under an in-flight iteration (the crash reported with only
    // Unlock Blueprints on: that is the only way to equip a blueprint, which is
    // the only thing that sets this pending id, so this walk is the one that
    // runs while the match loads).
    std::vector<void *> confs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        confs = weaponConfInstance;
    }
    uint base = 0;
    const int written = AutoEquipSkin_ApplyToBase(id, confs, base);

    if (base != 0)
    {
        g_autoEquipSkinDiag.resolved++;
        g_autoEquipSkinDiag.writes += written;
        g_autoEquipSkinDiag.lastID = id;
        g_autoEquipSkinDiag.lastBase = (int)base;
        g_autoEquipSkinPendingID = 0;
        g_autoEquipSkinPendingFrames = 0;
        return;
    }

    // The config tables are created ~1.5 s into a session, so an equip that lands
    // before them is retried - but only for ~0.2 s at 60 fps. Three seconds of
    // per-frame table walks is the window the match-load burst has to land in;
    // an equip made in the lobby never needs more than a frame or two.
    if (++g_autoEquipSkinPendingFrames > 12)
    {
        g_autoEquipSkinDiag.unresolved++;
        g_autoEquipSkinPendingID = 0;
        g_autoEquipSkinPendingFrames = 0;
    }
}
