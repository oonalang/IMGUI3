#pragma once

#include <string>
#include <unordered_map>

struct CamoConfigSnapshot {
    uint8_t CamoType;
    uint8_t ColorID;
    int TextureAssetID;
    int TextureSpecialAssetID;
};

struct CamoSkinSnapshot {
    int CamoID;
    int ColorSubID;
};

static std::unordered_map<uint, CamoConfigSnapshot> g_CamoOriginals;
static std::unordered_map<uint, CamoSkinSnapshot> g_CamoSkinOriginals;
static bool g_CamoApplied = false;

// The CamoTest "(test)" mode that used to live here is removed; the real
// camo browser in SkinTab.h (ApplyCamoToEveryGun / ApplyCamoToEveryWeaponSkin)
// is the camo feature. RestoreCamo stays as the per-frame safety below.

static void RestoreCamo()
{
    if (!g_CamoApplied) {
        return;
    }

    for (void *camo : weaponCamosConfigInstance) {
        if (!camo || !Tools::IsPtrValid(camo)) continue;
        auto *fields = reinterpret_cast<WeaponCamosFields *>((uintptr_t)camo + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;

        auto original = g_CamoOriginals.find(fields->CamoItemID);
        if (original == g_CamoOriginals.end()) continue;

        fields->CamoType = original->second.CamoType;
        fields->ColorID = original->second.ColorID;
        fields->TextureAssetID = original->second.TextureAssetID;
        fields->TextureSpecialAssetID = original->second.TextureSpecialAssetID;
    }

    for (void *skin : weaponSkinConfigInstance) {
        if (!skin || !Tools::IsPtrValid(skin)) continue;
        auto *fields = reinterpret_cast<WeaponSkinFields *>((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(fields)) continue;

        auto original = g_CamoSkinOriginals.find(fields->ID);
        if (original == g_CamoSkinOriginals.end()) continue;

        fields->CamoID = original->second.CamoID;
        fields->ColorSubID = original->second.ColorSubID;
    }

    g_CamoOriginals.clear();
    g_CamoSkinOriginals.clear();
    g_CamoApplied = false;
}

static void UpdateCamoOverride()
{
    RestoreCamo();
}
