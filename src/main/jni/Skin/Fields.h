#pragma once

#include <cstdint>
#include <cctype>
#include <map>
#include <string>
#include <array>
#include <unordered_map>
#include <vector>

extern uintptr_t Item2InventoryAddress;
extern uintptr_t WeaponConfAddress;
extern uintptr_t WeaponConfExtraAddress;
extern uintptr_t WeaponFireEffectAddress;
extern uintptr_t CharacterModelAddress;
extern uintptr_t BRDeadboxSkinAddress;
extern uintptr_t BRDropPlaneSkinAddress;
extern uintptr_t WeaponAssetGroupAddress;
extern uintptr_t MythicArmorConfigAddress;
extern uintptr_t MythicSightConfigAddress;
extern uintptr_t KillEffectItemConfConfigAddress;
extern uintptr_t WeaponSkinAddress;
extern uintptr_t WeaponCamosAddress;
extern uintptr_t ItemResourceAddress;
extern uintptr_t RoleConfAddress;
extern uintptr_t RoleSkinAddress;
extern uintptr_t RolePackConfAddress;
extern uintptr_t WeaponConfName;
extern uintptr_t GetDropPlaneName;
extern uintptr_t GetVehicleSkinName;
extern uintptr_t GetNameRoleSkin;
extern uintptr_t GetRoleConfName;
extern uintptr_t GetDeadBoxIDAddress;
extern uintptr_t UnlockedCamo;
extern uintptr_t VehicleSkinConfAddress;
extern uintptr_t VehicleItemConfAddress;
extern uintptr_t VehicleSkinHelperChangeVehicleSkinIdAddress;
extern uintptr_t VehicleSkinHelperChangeVehicleSkinConfAddress;
extern uintptr_t VehicleSkisSetupSkinAddress;
extern uintptr_t VehicleSkisChangeSkinIdAddress;
extern uintptr_t VehicleSkisChangeSkinConfAddress;
extern uintptr_t VehicleSkisBuildVehicleModelAddress;
extern uintptr_t VehicleSkisLoadModelCompleteAddress;
extern uintptr_t VehicleSkisApplySkinAddress;
extern std::vector<void *> itemInventoryInstance;
extern std::vector<void *> weaponExtraInstance;
extern std::vector<void *> weaponFireEffectInstance;
extern std::vector<void *> weaponConfInstance;
extern std::vector<void *> weaponAssetGroupInstance;
extern std::vector<void *> mythicArmorInstance;
extern std::vector<void *> mythicSightInstance;
extern std::vector<void *> killEffectItemInstance;
extern std::vector<void *> weaponSkinConfigInstance;
extern std::vector<void *> weaponCamosConfigInstance;
extern std::vector<void *> itemResourceConfigInstance;
extern std::vector<void *> CharacterModelConfigInstance;
extern std::vector<void *> RoleConfConfigInstance;
extern std::vector<void *> RoleSkinConfigInstance;
extern std::vector<void *> RolePackConfConfigInstance;
extern std::vector<void *> BRDeadboxSkinConfigInstance;
extern std::vector<void *> BRDropPlaneSkinConfigInstance;
extern std::vector<void *> vehicleSkinConfigInstance;
extern std::vector<void *> vehicleItemConfigInstance;
extern std::unordered_map<int, int> activeVehicleSkins;
extern std::unordered_map<int, int> activeVehicleSkinsById;
extern std::unordered_map<int, void*> activeVehicleSkinConfs;
extern std::unordered_map<std::string, bool> sBool;
extern std::unordered_map<int, int> activeKillEffects;
extern std::unordered_map<int, int> activeBulletTrackEffects;
extern std::unordered_map<int, int> activeWeaponFireEffects;
extern std::unordered_map<int, int> activeWeaponBrocast;
extern int g_selectedTargetCharIndex;

struct TargetChar {
    std::string name;
    int traitor1p;
    int traitor3p;
    int itemID;
    int roleID;
    int rolepackID;
};

extern std::vector<TargetChar> g_targetCharacters;

struct itemInfo
{
    std::string itemName;
    std::array<int, 4> Item2Inventory;
    std::array<int, 5> WeaponConf;
    std::array<int, 5> WeaponExtra;
    std::array<int, 3> WeaponAsset;
    std::array<int, 3> ItemResInt;
    std::array<uint16_t, 4> ItemRes;
};
extern std::vector<itemInfo> itemData;

struct charInfo
{
    std::string charName;
    std::array<int, 12> charModel;
    std::array<int, 3> charRes;
    std::array<uint16_t, 4> ItemRes2;
    std::array<int, 8> charRole;
    std::array<void *, 1> charRole2;
    std::array<int, 6> charPack;
};
extern std::vector<charInfo> charData;

struct watcher
{
    std::string watchname;
    int watchvalue;
};
extern std::vector<watcher> watch;

struct deadbox
{
    std::string deadname;
    std::array<int, 6> dead;
};
extern std::vector<deadbox> deadboxF;

struct planeID
{
    std::string planename;
    std::array<int, 6> plane;
};
extern std::vector<planeID> dropplane;

struct snowboardInfo
{
    std::string skinName;
    int skinId;
};
extern std::vector<snowboardInfo> snowboardData;

struct camoInfo
{
    std::string camoName;
    int camoItemID;
    int applyID;
    int textureAssetID;
    int belongWeaponID;
};
extern std::vector<camoInfo> camoData;

inline std::string CamoNameLower(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char ch : s)
        out.push_back((char)::tolower((unsigned char)ch));
    return out;
}

inline bool IsPlaceholderCamoName(const std::string &name)
{
    return name.rfind("Camo ", 0) == 0;
}

inline int KnownCamoIdForName(const std::string &name)
{
    const std::string lower = CamoNameLower(name);

    auto is = [&](const char *want) {
        const std::string w = want;
        for (size_t pos = lower.find(w); pos != std::string::npos;
             pos = lower.find(w, pos + 1)) {
            const size_t end = pos + w.size();
            const bool startsWord = (pos == 0) ||
                !std::isalnum((unsigned char)lower[pos - 1]);
            const bool endsWord = (end == lower.size()) ||
                !std::isalnum((unsigned char)lower[end]);
            if (startsWord && endsWord)
                return true;
        }
        return false;
    };
    if (is("gold"))       return 0x1D37F755;
    if (is("platinum"))   return 0x1D37F756;
    if (is("damascus"))   return 0x1D37F757;
    if (is("diamond"))    return 0x1D37F758;
    if (is("aether"))     return 0x1D37F77D;
    if (is("red sprite")) return 0x1D37F77E;
    if (is("glacial"))    return 0x1D37F77F;
    return 0;
}

inline std::string KnownCamoNameForId(int id)
{
    switch ((unsigned)id) {
        case 0x1D37F755: return "Gold";
        case 0x1D37F756: return "Platinum";
        case 0x1D37F757: return "Damascus";
        case 0x1D37F758: return "Diamond";
        case 0x1D37F77D: return "Aether Crystal";
        case 0x1D37F77E: return "Red Sprite";
        case 0x1D37F77F: return "Glacial Ripple";
        default:         return std::string();
    }
}

inline void InsertCamo(int camoItemID, const std::string &name,
                       int textureAssetID, int belongWeaponID)
{
    if (camoItemID <= 0 || name.empty())
        return;

    const int known = KnownCamoIdForName(name);
    const int applyID = known != 0 ? known : camoItemID;

    const std::string displayName = known != 0 ? KnownCamoNameForId(known) : name;
    const std::string key = CamoNameLower(displayName);

    for (auto &existing : camoData) {
        const bool sameRow = existing.camoItemID == camoItemID;
        const bool sameThing = existing.applyID == applyID &&
                               CamoNameLower(existing.camoName) == key;
        if (!sameRow && !sameThing)
            continue;
        if (existing.belongWeaponID != 0 && belongWeaponID == 0)
            existing.belongWeaponID = 0;
        if (existing.textureAssetID == 0 && textureAssetID != 0)
            existing.textureAssetID = textureAssetID;
        if (IsPlaceholderCamoName(existing.camoName) && !IsPlaceholderCamoName(displayName)) {
            existing.camoName = displayName;
            existing.applyID = applyID;
        }
        return;
    }

    camoInfo entry;
    entry.camoName = displayName;
    entry.camoItemID = camoItemID;
    entry.textureAssetID = textureAssetID;
    entry.belongWeaponID = belongWeaponID;
    entry.applyID = applyID;

    auto at = camoData.begin();
    while (at != camoData.end() && CamoNameLower(at->camoName) <= key)
        ++at;
    camoData.insert(at, entry);
}

extern char searchQuery[256];
extern std::string ToLower(std::string str);
extern uintptr_t getRealOffset(uintptr_t offset);

struct RoleSkinFields
{
    void *m_ownerRoleIDs;
    uint8_t ClothType;
    uint8_t ColorID;
    uint8_t GoGetPos;
    uint8_t HatSubMode;
    bool HideInBag;
    uint8_t IsCanBeUsedForAllRole;
    bool IsCloseScreenSaver;
    uint8_t IsDisassemble;
    uint8_t SecondTab;
    bool ShowInBag;
    bool ShowRare;
    char pad_23[1];
    int BRBagAssetID;
    int BRClothAssetID;
    int BRHatAssetID;
    int BRLeftArmAssetID;
    int BRSuitAssetID;
    int BRWingAssetID;
    int BagAssetID;
    int CamoID;
    int ClothAssetID;
    int FxAssetIDHandGestureUI;
    int FxAssetIDShow;
    int FxAssetID_1P;
    int FxAssetID_3P;
    int FxAssetID_UI;
    int HatAssetID;
    uint ID;
    int LeftArm1PAssetID;
    int LeftArmAssetID;
    int ShowBagAssetID;
    int ShowClothAssetID;
    int ShowHatAssetID;
    int SubTypeForCommonAttr;
    int SuitAssetID;
    int WingAssetID;
    char pad_84[4];
    void *ExtraAttriAssetIDs_1P;
    void *ExtraAttriAssetIDs_3P;
    void *ExtraAttriAssetIDs_UI;
    void *GoGetUrl;
    char pad_98[24];
    void *MaterialIDs;
};
extern RoleSkinFields *roleskinFields;

struct CharacterModelFields
{
    bool Female;
    uint AvatarAnimationType;
    int BRBagModel;
    int BRHeadModel;
    int BRLobby;
    int BRModel;
    int BindEffect1P;
    int ChangeClipEffect1P;
    int DefaultModelID;
    int Guarder1P;
    int Guarder3P;
    int GuarderBagModel;
    int GuarderHeadModel;
    int GuarderLobby;
    int HeadIgnoreGUPSkin;
    int SpReceiveAnimType;
    int Traitor1P;
    int Traitor3P;
    float *BagOffset;
    string *Desc;
    float *VehicleOffset;
    int *VestOtherModelID;
    uintptr_t ItemID;
    uintptr_t LitePackDefault;
};
extern CharacterModelFields *characterfields;

struct RolePackFields
{
    int mMatchRoomEasterEggType;
    uint mMatchRoomEasterEggCharacterID;
    String *mMatchRoomEasterEggActionName;
    bool DisableCustomize;
    bool EntryDefaultAppearance;
    bool SkipWinnerCircleAnimation;
    bool UseDynamicLobbyScene;
    bool UseVFWFeature;
    int AirItem1;
    int AnimationWhenSell;
    int CabinIndivID;
    int DeadBoxID;
    int DynamicRoomAssetID;
    int DynamicRoomUnlockItemID;
    int EntryAnimID;
    int EntryUIEffectAssetID;
    int EntryVideoAnimID;
    float EntryVideoPlayTime;
    int EntryVideoTagAnimID;
    int GestureId;
    int HandEffectUI;
    int HomeEasterEggAssetID;
    float HomeEasterEggCoolingTime;
    int HomeEasterEggType;
    int KillStreakSkinID;
    int LoadingFrame;
    int LobbySceneType;
    int MVPIndivID;
    int MatchRoomSort;
    int OpenSceneIndivID;
    int PendantShowID;
    int PetID;
    int PetSwitchEffectAssetID;
    int RoleFeatureType;
    uint RolePackID;
    int SkillSkinID;
    int SkillSkinUI;
    int SpecialSceneFrameGroupAssetID;
    uint TODSceneID;
    int VTOLShowAssetID;
    String *AirPoseBR;
    String *EntryAnim;
    String *EntryIdleAnim;
    void *EntryIdleItems;
    void *EntryItems;
    void *EntryItemsAnim;
    void *EntryItemsIdleAnim;
    String *EntryUIAnimationStateName;
};
extern RolePackFields *packfields;

struct RoleConfFields
{
    uint BaseCharacterID;
    uint roleLeftArmID;
    uint roleFinalSuitID;
    uint roleBasicHologramID;
    uint8_t ArchiveSwitch;
    uint8_t ArmorType;
    uint8_t ColorID;
    uint8_t ColorSubID;
    uint8_t DefaultPerk_1;
    uint8_t DefaultPerk_2;
    uint8_t DefaultPerk_3;
    bool DefaultShow;
    uint8_t DefaultSkill_1;
    bool ExternalUnVisible;
    bool IsVFW;
    uint8_t Loadout_Magnifier_Flag;
    uint8_t RoleLvGroupID;
    bool ShowInBag;
    uint8_t ShowOrder;
    bool ShowRare;
    int BlockAppearance;
    uint ID;
    int JetPackInfoID;
    int KeepInCatalog;
    float RTPCMaxValue;
    int RolePackID;
    void *AudioSwitch;
    void *BRBankNameEX;
    void *BasicAudioBank;
    void *BasicAudioBank2;
    void *BasicAudioBank3;
    void *DefaultAudioBank;
    void *DefaultAudioBank2;
    void *DefaultAudioEventEx;
    void *FoleyAudioBank;
    void *FoleyAudioNameEx;
    void *LOCID_Desc;
    void *LOCID_Name;
    void *LitePackageKeyName;
    void *MPBankNameEx;
    void *PVEBankNameEX;
    void *RTPCName;
    void *RoleSkinBox;
    void *SpecialAudioBank;
    void *SpecialAudioBank2;
    void *SpecialAudioEventEx;
    void *SpecialTips;
    void *SupportVoiceTypes;
    uint64_t WinnerCircle;
};
extern RoleConfFields *roleFields;

struct ItemResourceFields
{
    uint16_t UIMiniSpriteNameIndex;
    uint16_t UISmallSpriteNameIndex;
    uint16_t UISpriteNameIndex;
    uint16_t UISquareSpriteNameIndex;
    int32_t  FxAssetID;
    int32_t  ID;
    int32_t  InventoryModelID;
    int32_t  ModelAssetIDRaw;
    uint64_t AvatarModelID;
};
extern thread_local ItemResourceFields *itemFields;

inline void ResolveTargetIds(const TargetChar &t,
                             int &traitor1p, int &traitor3p,
                             int &itemId, int &roleId, int &packId)
{
    traitor1p = (t.name == "Charly") ? 710001101 : t.traitor1p;
    traitor3p = (t.name == "Charly") ? 710001102 : t.traitor3p;
    itemId    = (t.name == "Charly") ? 100301208 : t.itemID;
    roleId    = (t.name == "Charly") ? 100301208 : t.roleID;
    packId    = t.rolepackID;
}

inline bool CharacterApplyReady(const std::vector<void *> &charModels,
                                const std::vector<void *> &itemResources,
                                const std::vector<void *> &roleConfs)
{
    return !charModels.empty() && !itemResources.empty() && !roleConfs.empty();
}

inline void ApplyCharacterSkin(const charInfo &getchar,
                               int selTraitor1P, int selTraitor3P,
                               int selItemID, int selRoleID, int selPackID,
                               const std::vector<void *> &charModels,
                               const std::vector<void *> &itemResources,
                               const std::vector<void *> &roleConfs,
                               const std::vector<void *> &rolePacks)
{
    // The first-person and third-person bodies are two different
    // CharacterModel configs. Writing whichever one matched either id left the
    // other side on its old ids, and a body whose model ids disagree with the
    // role it belongs to renders as nothing at all. Each side is resolved on
    // its own here, with the stock default config as the fallback.
    constexpr int kStockTraitor1P = 710001101;
    constexpr int kStockTraitor3P = 710001102;
    constexpr int kStockItemId    = 100301208;

    CharacterModelFields *body1P = nullptr;
    CharacterModelFields *body3P = nullptr;
    CharacterModelFields *stock1P = nullptr;
    CharacterModelFields *stock3P = nullptr;

    for (auto charModel : charModels) {
        if (!charModel) continue;
        auto *cf = (CharacterModelFields *)((uintptr_t)charModel + 0x10);
        if (cf == nullptr) continue;

        if (cf->Traitor1P == kStockTraitor1P) stock1P = cf;
        if (cf->Traitor3P == kStockTraitor3P) stock3P = cf;
        if (cf->Traitor1P == selTraitor1P) body1P = cf;
        if (cf->Traitor3P == selTraitor3P) body3P = cf;
    }
    if (body1P == nullptr) body1P = stock1P;
    if (body3P == nullptr) body3P = stock3P;

    const auto writeModel = [&getchar](CharacterModelFields *cf) {
        if (cf == nullptr) return;
        cf->BRBagModel         = getchar.charModel[0];
        cf->BRHeadModel        = getchar.charModel[1];
        cf->BRLobby            = getchar.charModel[2];
        cf->BRModel            = getchar.charModel[3];
        cf->BindEffect1P       = getchar.charModel[4];
        cf->ChangeClipEffect1P = getchar.charModel[5];
        cf->DefaultModelID     = getchar.charModel[6];
        cf->Guarder1P          = getchar.charModel[7];
        cf->Guarder3P          = getchar.charModel[8];
        cf->GuarderBagModel    = getchar.charModel[9];
        cf->GuarderHeadModel   = getchar.charModel[10];
        cf->GuarderLobby       = getchar.charModel[11];
    };

    writeModel(body1P);
    writeModel(body3P);

    ItemResourceFields *item = nullptr;
    ItemResourceFields *stockItem = nullptr;

    for (auto itemRes : itemResources) {
        if (!itemRes) continue;
        auto *itf = (ItemResourceFields *)((uintptr_t)itemRes + 0x10);
        if (itf == nullptr) continue;

        if (itf->ID == kStockItemId) stockItem = itf;
        if (itf->ID != selItemID) continue;
        item = itf;
        break;
    }
    if (item == nullptr) item = stockItem;

    if (item != nullptr) {
        auto *itf = item;

        if (getchar.charRes[0] != 0) itf->FxAssetID        = getchar.charRes[0];
        if (getchar.charRes[1] != 0) itf->InventoryModelID = getchar.charRes[1];
        if (getchar.charRes[2] != 0) itf->ModelAssetIDRaw  = getchar.charRes[2];

        if (getchar.ItemRes2[0] != 0) itf->UIMiniSpriteNameIndex   = getchar.ItemRes2[0];
        if (getchar.ItemRes2[1] != 0) itf->UISmallSpriteNameIndex  = getchar.ItemRes2[1];
        if (getchar.ItemRes2[2] != 0) itf->UISpriteNameIndex       = getchar.ItemRes2[2];
        if (getchar.ItemRes2[3] != 0) itf->UISquareSpriteNameIndex = getchar.ItemRes2[3];
    }

    RoleConfFields *role = nullptr;
    RoleConfFields *stockRole = nullptr;

    for (auto roles : roleConfs) {
        if (!roles) continue;
        auto *rf = (RoleConfFields *)((uintptr_t)roles + 0x10);
        if (rf == nullptr) continue;

        if (rf->ID == kStockItemId) stockRole = rf;
        if (rf->ID != selRoleID) continue;
        role = rf;
        break;
    }
    if (role == nullptr) role = stockRole;

    if (role != nullptr) {
        auto *rf = role;
        rf->roleLeftArmID       = getchar.charRole[0];
        rf->roleFinalSuitID     = getchar.charRole[1];
        rf->roleBasicHologramID = getchar.charRole[2];
        rf->ColorID             = getchar.charRole[3];
        rf->ColorSubID          = getchar.charRole[4];
        rf->ShowRare            = getchar.charRole[5];
        rf->RoleLvGroupID       = getchar.charRole[6];
        rf->RolePackID          = getchar.charRole[7];
        rf->LOCID_Name          = getchar.charRole2[0];
    }

    for (auto pack : rolePacks) {
        if (!pack) continue;
        auto *pf = (RolePackFields *)((uintptr_t)pack + 0x10);
        if (pf == nullptr || pf->RolePackID != selPackID) continue;

        pf->EntryAnimID      = getchar.charPack[1];
        pf->GestureId        = getchar.charPack[2];
        pf->HandEffectUI     = getchar.charPack[3];
        pf->LoadingFrame     = getchar.charPack[4];
        pf->KillStreakSkinID = getchar.charPack[5];
    }
}

struct WeaponConfFields
{
    uint8_t Accuracy;
    bool CanEquipOptic;
    uint8_t ColorID;
    uint8_t ColorSubID;
    uint8_t Control;
    uint8_t Damage;
    bool ExternalUnVisible;
    uint8_t FireRate;
    uint8_t LOCID_HowToGet;
    uint8_t MaxAttach;
    uint8_t MaxSkill;
    uint8_t Mobility;
    uint8_t Range;
    bool ShowRare;
    uint8_t SuperShowGroup;
    uint8_t WeaponBluePrintType;
    uint8_t WinnerCircleAnimID;
    char pad_31[3];
    uint BaseWeaponId;
    int DefWeaponSkinID;
    int DefaultKillBrocast;
    uint ID;
    int IsDouble;
    int WinnerCircleWeaponMountType;
    int pad_4C;
    void *ChangeColorStr;
    void *EnableCamoBeta;
    void *LOCID_Desc;
    void *LOCID_Name;
    void *ModelRotation;
    void *UIProperty;
    void *UIPropertyUnlockLevel;
};
extern thread_local WeaponConfFields *weaponconfFields;

inline int ApplyCamoToEveryGun(int camoItemID,
                               const std::vector<void *> &weaponConfs,
                               std::unordered_map<uint, int> &originals)
{
    int written = 0;
    for (void *conf : weaponConfs) {
        if (!conf || !Tools::IsPtrValid(conf))
            continue;
        auto *cf = (WeaponConfFields *)((uintptr_t)conf + 0x20);
        if (!Tools::IsPtrValid(cf) || cf->ID == 0)
            continue;

        if (camoItemID > 0) {
            originals.emplace(cf->ID, cf->DefWeaponSkinID);
            cf->DefWeaponSkinID = camoItemID;
            written++;
        } else {
            auto it = originals.find(cf->ID);
            if (it == originals.end())
                continue;
            cf->DefWeaponSkinID = it->second;
            written++;
        }
    }
    return written;
}

struct WeaponCamosFields
{
    uint8_t CamoType;
    uint8_t ColorID;
    bool NeedShowInEndGame;
    int BelongWeaponID;
    uint CamoItemID;
    uint ConceptID;
    int DMZseason;
    int KeepInCatalog;
    int TextureAssetID;
    int TextureSpecialAssetID;
    void *LOCID_Describe;
    void *LOCID_Name;
    Array<uint> *Mission_id;
    Array<uint> *Personal_mission_id;
    void *SmallIconSpriteName;
};
extern thread_local WeaponCamosFields *weaponcamosFields;

struct WeaponSkinFields
{
    uint8_t ColorID;
    uint8_t GoGetPos;
    bool IsCleanCamo;
    uint8_t SkinType;
    uint BRWeaponID;
    int BrSkinWeaponID;
    int CamoID;
    int ColorSubID;
    uint ID;
    uint WeaponID;
    void *LOCID_Describe;
    void *LOCID_HowToGet;
    void *LOCID_Name;
    Array<int> *SkinID1P;
    Array<int> *SkinID3P;
};
extern thread_local WeaponSkinFields *weaponskinFields;

struct WeaponSkinCamoOriginal
{
    int camoID;
    int colorSubID;
};

inline int ApplyCamoToEveryWeaponSkin(int camoItemID,
                                      const std::vector<void *> &skinConfs,
                                      std::unordered_map<uint, WeaponSkinCamoOriginal> &originals)
{
    int written = 0;
    for (void *skin : skinConfs) {
        if (!skin || !Tools::IsPtrValid(skin))
            continue;
        auto *sf = (WeaponSkinFields *)((uintptr_t)skin + 0x10);
        if (!Tools::IsPtrValid(sf) || sf->ID == 0)
            continue;

        if (camoItemID > 0) {
            originals.emplace(sf->ID, WeaponSkinCamoOriginal{sf->CamoID, sf->ColorSubID});
            sf->CamoID = camoItemID;
            sf->ColorSubID = 0;
            written++;
        } else {
            auto it = originals.find(sf->ID);
            if (it == originals.end())
                continue;
            sf->CamoID = it->second.camoID;
            sf->ColorSubID = it->second.colorSubID;
            written++;
        }
    }
    return written;
}

struct Item2InventoryFields
{
    int ItemID;
    int WeaponAssetGroupID;
    int WeaponIconID;
    void *AssetName;
    void *ConfigFilePathH;
    void *ConfigFilePath_BRH;
    void *ConfigFilePath_DMZH;
    void *ConfigFilePath_PVEH;
    void *WeaponNameH;
};
extern thread_local Item2InventoryFields *item2Fields;

struct WeaponAssetGroupFields
{
    int FireEffectGroupID;
    int Id;
    int MeshAssetID1P;
    int MeshAssetID3P;
    int MeshAssetID3P_L;
    int MeshAssetIDUI;
    int ProjAssetID;
    int ProjExplosionAssetID;
    int ProjFlightAssetID;
    int ProjFlightAssetID3P;
    int SecendBRProjExplosionAssetID;
    int SecendProjAssetID;
    int SecendProjExplosionAssetID;
    int SecendProjFlightAssetID;
    int SecendProjFlightAssetID3P;
    int SecondaryFireEffectGroupID;
    int SpecialUnAimingEffectAssetID1P;
    int UnusePupil;
    int WeaponPickUpAssetID;
    void *LeftLaserRotation;
    void *RightLaserRotation;
    void *ScopeDistancePartIDs;
    void *ScopeDistanceRatios;
};
extern thread_local WeaponAssetGroupFields *weaponAssetFields;

struct WeaponFireEffectFields
{
    int AssetIdFor1P;
    int AssetIdFor3P;
    int AssetIdForBulletSmoke1P;
    int AssetIdForBulletSmoke3P;
    int AssetIdForBulletTrack1P;
    int AssetIdForBulletTrack3P;
    int AssetIdForCharging1P;
    int AssetIdForCrossHair;
    int AssetIdForDecalEffect;
    int AssetIdForExtra1P;
    int AssetIdForExtraBulletSmoke1P;
    int AssetIdForExtraBulletSmoke3P;
    int AssetIdForHip1P;
    int AssetIdForHip3P;
    int AssetIdForHitEnemy;
    int AssetIdForInspectionScreenEffect;
    int AssetIdForSecondBulletSmoke1P;
    int AssetIdForSecondBulletSmoke3P;
    int AssetIdForSecondHip1P;
    int AssetIdForSecondHip3P;
    int AssetIdForShellDrop1P;
    int AssetIdForShellDrop3P;
    int AttachAssetIdFor1P;
    int AttachAssetIdFor3P;
    int AttachAssetIdForHip1P;
    int AttachAssetIdForHip3P;
    int DefaultKillEffectLevelUI;
    float HitEffectDelayDestroyTime;
    int HitEffectType;
    int Id;
    float LevelEffectDelayTimeUI;
    float LevelEffectDelayTimeUI_King;
    void *AssetIdForInspectionEffect;
    void *AssetIdForLevelEffects;
    void *AssetIdForLevelEffectsUI;
    void *AssetIdsForCircleBluntDecal;
    void *AssetIdsForCrustDecal;
    void *AssetIdsForExtraEffects;
    void *AssetIdsForExtraEffects3P;
    void *AssetIdsForExtraEffectsUI;
    void *AssetIdsForMeleeAttack;
    void *AssetIdsForMetalDecal;
    void *AssetIdsForShellDrop1P;
    void *AssetIdsForShellDrop3P;
    void *AssetIdsForStripBluntDecal;
    void *BRKillCountTimePlus;
    void *BRKillCountTimePlus_King;
    void *FireSoundExtend;
    void *HitEffectAssetIds;
    void *KillCountForLevelEffect;
    void *KillCountForLevelEffect_King;
    void *MPKillCountTimePlus;
    void *MPKillCountTimePlus_King;
    void *MeleeEffectDetachParent;
    void *SocketForInspectionEffect;
    void *SocketForLevelEffects;
    void *SocketForLevelEffectsUI;
    void *SocketNameForExtra1P;
    void *SocketsForExtraEffects;
    void *SocketsForExtraEffects3P;
    void *SocketsForExtraEffectsUI;
    void *SocketsForMeleeAttack;
    void *SocketsForShellDrop1P;
    void *SocketsForShellDrop3P;
};
extern thread_local WeaponFireEffectFields *weaponfireFields;

struct WeaponConfExtraFields
{
    uint8_t DefaultMythicArmor;
    uint8_t DefaultMythicSig;
    uint8_t GoGetPos;
    uint8_t SecondTab;
    uint8_t ThirdTab;
    int DefaultDeadReplayEffectId;
    int DefaultKillEffectId;
    int FireEffectGroupID;
    uint ID;
    void *GoGetUrl;
    void *IdleAnimationIndex;
    void *OpenViewSkill;
    void *OpenViewSkillUnlockLevel;
    void *PropertyModifys;
};
extern thread_local WeaponConfExtraFields *weaponextraFields;

struct MythicArmorFields
{
    int AssetID;
    int ColorID;
    int ColorSubID;
    int GoGetPos;
    uint ID;
    int MythicArmorLevel;
    int SecondTab;
    int ThirdTab;
    void *EquippedWeaponIcon;
    void *GoGetUrl;
    void *LOCID_Desc;
    void *LOCID_HowToGet;
    void *LOCID_Name;
    void *WeaponCanEquip;
};
extern thread_local MythicArmorFields *mythicarmorFields;

struct MythicSightFields
{
    bool ShowInBag;
    char pad_11[3];
    int AssetID;
    int ColorID;
    int ColorSubID;
    uint ID;
    int RelateMythicSight;
    void *LOCID_Desc;
    void *LOCID_Name;
    void *WeaponCanEquip;
};
extern thread_local MythicSightFields *mythicsightFields;

struct KillEffectItemFields
{
    void *RealAssetIDs;
    bool IsAutoDemolition;
    bool ShowInBag;
    char pad_1A[2];
    int ColorID;
    int ColorSubID;
    uint CurrencyID;
    int GoGetPos;
    uint ID;
    int SecondTab;
    int SellPrice;
    int SellProtection;
    int ThirdTab;
    void *AssetID;
    void *AssetID_CN;
    void *AssetID_GA;
    void *AssetID_KR;
    void *AssetID_VNG;
    void *GoGetUrl;
    void *KillCount;
    void *LOCID_Desc;
    void *LOCID_HowToGet;
    void *LOCID_Name;
    void *WeaponCanEquip;
};
extern thread_local KillEffectItemFields *killeffectFields;

struct BRDeadboxSkinFields
{
    int ColorID;
    int DeadBoxEffectAsset;
    int Flag;
    int FlagAsset;
    uint ID;
    int ModelAsset3P;
    int ModelAssetUI;
};
extern BRDeadboxSkinFields *deadboxFields;

struct BRDropPlaneSkinFields
{
    int ColorID;
    uint ID;
    int ModelAsset1P;
    int ModelAsset3P;
    int ModelAssetCutScene;
    int ModelAssetUI;
    int Priority;
};
extern BRDropPlaneSkinFields *dropplaneFields;

struct VehicleSkinConfFields
{
    uint8_t ColorID;
    bool DisableInCollection;
    bool ExternalUnVisible;
    uint8_t GoGetPos;
    bool IsAutoDemolition;
    uint8_t LockedShowOrder;
    bool ShowInBag;
    bool ShowRare;
    uint8_t SkinMusicsID;
    uint8_t SkinStyle;
    char pad_1A[2];
    uint CurrencyID;
    uint ID;
    int MusicsID;
    int NewVehicleResId;
    float RoomShowScale;
    int SellPrice;
    int SkinExtraMesh;
    int SkinParam;
    int SkinParam2;
    int SkinParam3;
    int UILowModel;
    int VehicleId;
    char pad_3C[12];
    void *FeatureDescLocid1;
    void *FeatureDescLocid2;
    void *GameType;
    void *GoGetUrl;
    char pad_68[24];
    void *PreloadRes;
    char pad_88[48];
    void *SkinParams;
    void *WeaponSkin;
};
extern VehicleSkinConfFields *vehicleSkinFields;