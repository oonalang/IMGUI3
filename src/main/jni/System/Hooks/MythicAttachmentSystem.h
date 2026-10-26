#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Hacks/Feature/Init_Hacks.h"

#include "Skin/Fields.h"

namespace mythic_attachment_system
{
inline bool MythicAttachmentsEnabled()
{
    return g_premiumFeaturesEnabled &&
           (Config.ExtraMenu.Attachment || Config.ExtraMenu.MythicAttachments);
}

std::unordered_map<int, int> activeWeaponSkins;
std::unordered_map<int, std::array<int, 31>> activeMythicAttachments;
std::array<int, 31> activeMythicAttachFallback{};
std::unordered_map<uint, uint> runtimeAttachMythicById;
std::unordered_map<uint64_t, int> runtimeAttachResolveCache;
std::unordered_map<uint64_t, bool> loadoutPreviewTiles;
std::unordered_map<uint, int> attachmentTypeCache;
std::unordered_map<uint64_t, void*> weaponPartConfCache;
size_t attachTypeIndexed = 0;
size_t partConfIndexed = 0;
uint runtimeAttachWeaponId = 0;
uint runtimeAttachSkinId = 0;

std::vector<void*> weaponPartAssetInstance;
std::vector<void*> weaponSkinConfigInstance;
std::vector<void*> attachmentConfInstance;
inline std::unordered_set<void*> weaponPartAssetSeen;
inline std::unordered_set<void*> attachmentConfSeen;
inline std::mutex mythicCollectorMutex;

inline void CollectWeaponSkinConfig(void* instance) {
    if (!instance) return;
    std::lock_guard<std::mutex> lock(mythicCollectorMutex);
    if (std::find(weaponSkinConfigInstance.begin(), weaponSkinConfigInstance.end(), instance) == weaponSkinConfigInstance.end())
        weaponSkinConfigInstance.push_back(instance);
}

inline std::vector<void*> SnapshotWeaponSkinConfigs() {
    std::lock_guard<std::mutex> lock(mythicCollectorMutex);
    return weaponSkinConfigInstance;
}

inline std::vector<void*> SnapshotWeaponPartAssets() {
    std::lock_guard<std::mutex> lock(mythicCollectorMutex);
    return weaponPartAssetInstance;
}

uintptr_t WeaponPartAssetAddress = 0xAF85628;
uintptr_t WeaponSkinAddress = 0xAF8B4D4;
uintptr_t AttachmentConfAddress = 0x6FE9168;
uintptr_t AttachmentGroupNewCtorAddress = 0x8719E4C;
uintptr_t AttachmentGroupNewItemCtorAddress = 0x871A270;
uintptr_t AttachmentGroupNewGetColorIdAddress = 0x871A5D8;
uintptr_t BRWeaponAttachItemDataCtorAddress = 0x99811C0;
uintptr_t AttachItemCtorAddress = 0x8449500;
uintptr_t AttachItemSetDataAddress = 0x8449544;
uintptr_t AttachItemSetMythicSkinIdAddress = 0x844988C;
uintptr_t AttachItemCopyEquippedAddress = 0x84498F0;
uintptr_t AttachItemGetModelAttachIdAddress = 0x844A18C;
uintptr_t AttachSkinDataCtorAddress = 0x75D7DE4;
uintptr_t LoadoutAttachItemSetDataAddress = 0xB83F1C0;
uintptr_t WeaponInitPartAddress = 0x517C4D0;

inline void (*orig_AttachmentGroupNew_Ctor)(
    void* instance,
    uint attachId,
    int attachSkinId,
    uint mythicSkinId,
    uint weaponId,
    int ownState,
    bool enableReddot,
    bool isAttachTypeTrans,
    bool isPreConditionSatisfied) = nullptr;

inline void (*orig_AttachmentGroupNew_ItemCtor)(
    void* instance,
    void* attachItem,
    uint weaponId,
    int ownState,
    bool enableReddot,
    bool isAttachTypeTrans,
    bool isPreConditionSatisfied) = nullptr;

inline int (*orig_AttachmentGroupNew_GetColorId)(void* instance) = nullptr;

inline void (*orig_BRWeaponAttachItemData_Ctor)(
    void* instance,
    uint weaponActorId,
    uint attachId,
    int index,
    uint mythicId,
    int attachSkinId,
    bool disable,
    uint dmzItemId) = nullptr;

inline void (*orig_AttachItem_Ctor)(
    void* instance,
    uint itemId,
    int skinId,
    uint mythicSkinId) = nullptr;

inline void (*orig_AttachItem_SetData)(
    void* instance,
    uint itemId,
    int skinId,
    uint mythicSkinId) = nullptr;

inline void (*orig_AttachItem_SetMythicSkinID)(
    void* instance,
    int skinId,
    uint mythicSkinId) = nullptr;

inline void (*orig_AttachItem_CopyEquipped)(
    void* instance,
    void* item) = nullptr;

inline uint (*orig_AttachItem_GetModelAttachId)(void* instance) = nullptr;

inline void (*orig_AttachSkinData_Ctor)(
    void* instance,
    uint blueprintWeaponId,
    int skinId,
    uint mythicSkinId) = nullptr;

inline void (*orig_LoadoutAttachItem_SetData)(
    void* instance,
    void* attachInfo,
    void* panel) = nullptr;

inline void* (*orig_Weapon_InitPart)(
    void* instance,
    void* partInfo,
    bool autoActivate,
    bool independent) = nullptr;

inline bool AttachPtr(const void* ptr)
{
    return (uintptr_t)ptr >= 0x10000;
}

inline void IndexAttachmentTypes()
{
    std::lock_guard<std::mutex> lock(mythicCollectorMutex);
    if (attachmentConfInstance.size() < attachTypeIndexed) {
        attachmentTypeCache.clear();
        attachTypeIndexed = 0;
    }

    for (; attachTypeIndexed < attachmentConfInstance.size(); ++attachTypeIndexed) {
        void* conf = attachmentConfInstance[attachTypeIndexed];
        if (!AttachPtr(conf))
            continue;

        const uint id = *(uint*)((uintptr_t)conf + 0x68);
        if (id > 0)
            attachmentTypeCache[id] = *(uint8_t*)((uintptr_t)conf + 0x40);
    }
}

inline int GetAttachmentType(uint attachId)
{
    IndexAttachmentTypes();

    auto found = attachmentTypeCache.find(attachId);
    return found != attachmentTypeCache.end() ? found->second : 0;
}

inline const std::array<int, 9>& MythicAttachTypes()
{
    static const std::array<int, 9> types = {1, 2, 3, 4, 5, 6, 7, 8, 10};
    return types;
}

inline int AttachSlotForType(int attachType)
{
    const auto& types = MythicAttachTypes();
    for (int i = 0; i < (int)types.size(); ++i) {
        if (types[i] == attachType)
            return i;
    }

    return -1;
}

inline bool IsRealMythicAttachSkin(uint mythicId)
{
    return mythicId >= 600000000 && mythicId < 700000000;
}

inline int RealMythicFromArray(Array<int>* ids, int slot)
{
    const int len = AttachPtr(ids) ? ids->getLength() : 0;
    if (slot < 0 || len <= slot || len > 64)
        return 0;

    const int mythicId = ids->m_Items[slot + 1];
    return IsRealMythicAttachSkin((uint)mythicId) ? mythicId : 0;
}

inline int RealMythicForType(Array<int>* ids, int attachType)
{
    if (!AttachPtr(ids) || attachType <= 0)
        return 0;

    return RealMythicFromArray(ids, AttachSlotForType(attachType));
}

inline int RealMythicForType(WeaponSkinFields* fields, int attachType)
{
    if (!AttachPtr(fields))
        return 0;

    const int skin1P = RealMythicForType(fields->SkinID1P, attachType);
    if (skin1P > 0)
        return skin1P;

    return RealMythicForType(fields->SkinID3P, attachType);
}

inline int ResolveRealMythicAttach(uint weaponId, int attachType)
{
    if (!MythicAttachmentsEnabled())
        return 0;
    if (weaponId == 0 || attachType <= 0)
        return 0;

    for (void* weaponSkin : SnapshotWeaponSkinConfigs()) {
        if (!AttachPtr(weaponSkin))
            continue;

        auto* fields = (WeaponSkinFields*)((uintptr_t)weaponSkin + 0x10);
        if (!AttachPtr(fields))
            continue;

        const bool match =
            fields->ID == weaponId ||
            fields->WeaponID == weaponId ||
            fields->BRWeaponID == weaponId ||
            (uint)fields->BrSkinWeaponID == weaponId;
        if (!match)
            continue;

        const int mythicId = RealMythicForType(fields, attachType);
        if (mythicId > 0)
            return mythicId;
    }

    const uint weaponIds[] = {runtimeAttachSkinId, weaponId};
    for (uint id : weaponIds) {
        if (id == 0)
            continue;

        for (void* part : SnapshotWeaponPartAssets()) {
            if (!AttachPtr(part))
                continue;

            const int partType = *(int*)((uintptr_t)part + 0x14);
            const uint partItemId = *(uint*)((uintptr_t)part + 0x58);
            const uint partWeaponId = *(uint*)((uintptr_t)part + 0x7C);
            if (partWeaponId == id &&
                partType == attachType &&
                IsRealMythicAttachSkin(partItemId)) {
                return (int)partItemId;
            }
        }
    }

    return 0;
}

inline int ActiveMythicAttachFor(uint weaponId, int attachType)
{
    if (!MythicAttachmentsEnabled())
        return 0;
    if (attachType <= 0 || attachType >= 31)
        return 0;

    auto active = activeMythicAttachments.find((int)weaponId);
    if (active != activeMythicAttachments.end() &&
        active->second[attachType] > 0) {
        const int selected = active->second[attachType];
        if (IsRealMythicAttachSkin((uint)selected))
            return selected;

        const int resolved = ResolveRealMythicAttach(weaponId, attachType);
        if (resolved > 0)
            return resolved;

        return selected;
    }

    const uint64_t key =
        ((uint64_t)weaponId << 8) | (uint64_t)attachType;
    auto cached = runtimeAttachResolveCache.find(key);
    if (cached != runtimeAttachResolveCache.end())
        return cached->second;

    const int resolved = ResolveRealMythicAttach(weaponId, attachType);
    if (resolved > 0) {
        runtimeAttachResolveCache[key] = resolved;
        return resolved;
    }

    const int fallback = activeMythicAttachFallback[attachType];
    runtimeAttachResolveCache[key] = fallback;
    return fallback;
}

inline bool IsWeaponItemId(uint itemId)
{
    return itemId >= 10000000 && itemId < 20000000;
}

inline int ActiveAttachForContext(uint weaponId, int attachType)
{
    const int selected = ActiveMythicAttachFor(weaponId, attachType);
    if (selected > 0 ||
        runtimeAttachWeaponId == 0 ||
        runtimeAttachWeaponId == weaponId) {
        return selected;
    }

    return ActiveMythicAttachFor(runtimeAttachWeaponId, attachType);
}

inline uint PatchAttachMythic(void* attachItem, uint weaponId)
{
    if (!AttachPtr(attachItem))
        return 0;

    const uint attachId = *(uint*)((uintptr_t)attachItem + 0x10);
    const int attachType = GetAttachmentType(attachId);
    const int selected = ActiveAttachForContext(weaponId, attachType);
    const uint current = *(uint*)((uintptr_t)attachItem + 0x18);
    const uint next = selected > 0 ? (uint)selected : current;

    if (selected > 0 && current != next)
        *(uint*)((uintptr_t)attachItem + 0x18) = next;
    if (next > 0)
        runtimeAttachMythicById[attachId] = next;

    return next;
}

inline uint64_t LoadoutTileKey(uint weaponId, uint attachId)
{
    return ((uint64_t)weaponId << 32) | attachId;
}

inline uint ActiveWeaponSkinId(int weaponId)
{
    auto found = activeWeaponSkins.find(weaponId);
    return found != activeWeaponSkins.end() && found->second > 0
        ? (uint)found->second
        : 0;
}

inline void IndexWeaponParts()
{
    std::lock_guard<std::mutex> lock(mythicCollectorMutex);
    if (weaponPartAssetInstance.size() < partConfIndexed) {
        weaponPartConfCache.clear();
        partConfIndexed = 0;
    }

    for (; partConfIndexed < weaponPartAssetInstance.size(); ++partConfIndexed) {
        void* conf = weaponPartAssetInstance[partConfIndexed];
        if (!AttachPtr(conf))
            continue;

        const int partId = *(int*)((uintptr_t)conf + 0x58);
        const uint weaponId = *(uint*)((uintptr_t)conf + 0x7C);
        if (weaponId > 0 && partId > 0) {
            const uint64_t key =
                ((uint64_t)weaponId << 32) | (uint32_t)partId;
            weaponPartConfCache[key] = conf;
        }
    }
}

inline void* FindWeaponPartConf(uint weaponId, int partId)
{
    if (weaponId == 0 || partId <= 0)
        return nullptr;

    IndexWeaponParts();

    const uint64_t key =
        ((uint64_t)weaponId << 32) | (uint32_t)partId;
    auto found = weaponPartConfCache.find(key);
    return found != weaponPartConfCache.end() ? found->second : nullptr;
}

inline uint ActiveWeaponPartSkinId(int weaponId, int partId)
{
    if (partId <= 0)
        return 0;

    uint skinId = ActiveWeaponSkinId(weaponId);
    if (skinId > 0 && FindWeaponPartConf(skinId, partId) != nullptr)
        return skinId;

    if (runtimeAttachSkinId > 0 &&
        FindWeaponPartConf(runtimeAttachSkinId, partId) != nullptr) {
        return runtimeAttachSkinId;
    }

    return 0;
}

inline void* hook_Weapon_InitPart(
    void* instance,
    void* partInfo,
    bool autoActivate,
    bool independent)
{
    const int rawWeaponId = AttachPtr(instance)
        ? *(int*)((uintptr_t)instance + 0x1030)
        : 0;

    int getterWeaponId = 0;
    if (AttachPtr(instance) && Weapon_GetWeaponId) {
        using GetWeaponIdFn = int (*)(void*);
        getterWeaponId = ((GetWeaponIdFn)Weapon_GetWeaponId)(instance);
    }

    uint oldSkin = 0;
    int partId = 0;
    uint selectedSkin = 0;
    if (AttachPtr(partInfo)) {
        partId = *(int*)((uintptr_t)partInfo + 0x20);
        oldSkin = *(uint*)((uintptr_t)partInfo + 0x28);

        selectedSkin = ActiveWeaponPartSkinId(rawWeaponId, partId);
        if (selectedSkin == 0 && getterWeaponId != rawWeaponId)
            selectedSkin = ActiveWeaponPartSkinId(getterWeaponId, partId);

        if (selectedSkin > 0)
            *(uint*)((uintptr_t)partInfo + 0x28) = selectedSkin;
    }

    void* part = orig_Weapon_InitPart != nullptr
        ? orig_Weapon_InitPart(
            instance,
            partInfo,
            autoActivate,
            independent)
        : nullptr;

    if (selectedSkin > 0 && AttachPtr(partInfo))
        *(uint*)((uintptr_t)partInfo + 0x28) = oldSkin;

    if (selectedSkin > 0 && AttachPtr(part)) {
        const uintptr_t addr = (uintptr_t)part;
        if (*(uint*)(addr + 0x44) != selectedSkin)
            *(uint*)(addr + 0x44) = selectedSkin;
    }

    return part;
}

inline void hook_AttachmentGroupNew_Ctor(
    void* instance,
    uint attachId,
    int attachSkinId,
    uint mythicSkinId,
    uint weaponId,
    int ownState,
    bool enableReddot,
    bool isAttachTypeTrans,
    bool isPreConditionSatisfied)
{
    const int attachType = GetAttachmentType(attachId);
    if (IsWeaponItemId(weaponId))
        runtimeAttachWeaponId = weaponId;

    const int selected = ActiveAttachForContext(weaponId, attachType);
    const uint next = selected > 0 ? (uint)selected : mythicSkinId;

    if (orig_AttachmentGroupNew_Ctor != nullptr) {
        orig_AttachmentGroupNew_Ctor(
            instance,
            attachId,
            attachSkinId,
            next,
            weaponId,
            ownState,
            enableReddot,
            isAttachTypeTrans,
            isPreConditionSatisfied);
    }

    if (selected > 0 && AttachPtr(instance)) {
        void* item = *(void**)((uintptr_t)instance + 0x10);
        if (AttachPtr(item) &&
            *(uint*)((uintptr_t)item + 0x18) != (uint)selected) {
            *(uint*)((uintptr_t)item + 0x18) = (uint)selected;
        }
    }
}

inline void hook_AttachmentGroupNew_ItemCtor(
    void* instance,
    void* attachItem,
    uint weaponId,
    int ownState,
    bool enableReddot,
    bool isAttachTypeTrans,
    bool isPreConditionSatisfied)
{
    if (IsWeaponItemId(weaponId))
        runtimeAttachWeaponId = weaponId;

    PatchAttachMythic(attachItem, weaponId);

    if (orig_AttachmentGroupNew_ItemCtor != nullptr) {
        orig_AttachmentGroupNew_ItemCtor(
            instance,
            attachItem,
            weaponId,
            ownState,
            enableReddot,
            isAttachTypeTrans,
            isPreConditionSatisfied);
    }

    if (AttachPtr(instance)) {
        void* item = *(void**)((uintptr_t)instance + 0x10);
        PatchAttachMythic(item, weaponId);
    }
}

inline int hook_AttachmentGroupNew_GetColorId(void* instance)
{
    const int colorId = orig_AttachmentGroupNew_GetColorId != nullptr
        ? orig_AttachmentGroupNew_GetColorId(instance)
        : 0;
    if (!AttachPtr(instance))
        return colorId;

    void* item = *(void**)((uintptr_t)instance + 0x10);
    if (!AttachPtr(item))
        return colorId;

    const uint attachId = *(uint*)((uintptr_t)item + 0x10);
    const uint weaponId = *(uint*)((uintptr_t)instance + 0x1C);
    auto preview = loadoutPreviewTiles.find(
        LoadoutTileKey(weaponId, attachId));
    const bool hasPreview =
        preview != loadoutPreviewTiles.end() && preview->second;

    if (hasPreview)
        return 5;

    if (colorId == 5) {
        void* conf = *(void**)((uintptr_t)instance + 0x30);
        return AttachPtr(conf)
            ? *(uint8_t*)((uintptr_t)conf + 0x41)
            : 0;
    }

    return colorId;
}

inline void hook_LoadoutAttachItem_SetData(
    void* instance,
    void* attachInfo,
    void* panel)
{
    uint weaponId = 0;
    uint attachId = 0;
    uint uiMythic = 0;
    uint previewSkin = 0;
    int attachType = 0;
    void* item = nullptr;
    void* previewConf = nullptr;

    if (AttachPtr(attachInfo)) {
        weaponId = *(uint*)((uintptr_t)attachInfo + 0x1C);
        item = *(void**)((uintptr_t)attachInfo + 0x10);
        if (AttachPtr(item)) {
            attachId = *(uint*)((uintptr_t)item + 0x10);
            attachType = GetAttachmentType(attachId);
        }

        const uint activeSkin = ActiveWeaponSkinId((int)weaponId);
        previewConf = FindWeaponPartConf(activeSkin, (int)attachId);
        previewSkin = AttachPtr(previewConf) ? activeSkin : 0;
        if (previewSkin > 0) {
            const int selected =
                ActiveAttachForContext(weaponId, attachType);
            uiMythic = IsRealMythicAttachSkin((uint)selected)
                ? (uint)selected
                : previewSkin;
            loadoutPreviewTiles.emplace(
                LoadoutTileKey(weaponId, attachId),
                true);
        } else {
            loadoutPreviewTiles.erase(
                LoadoutTileKey(weaponId, attachId));
        }

        if (AttachPtr(item) &&
            *(uint*)((uintptr_t)item + 0x18) != uiMythic) {
            *(uint*)((uintptr_t)item + 0x18) = uiMythic;
        }

        if (AttachPtr(previewConf) &&
            *(void**)((uintptr_t)attachInfo + 0x28) != previewConf) {
            *(void**)((uintptr_t)attachInfo + 0x28) = previewConf;
        }
    }

    if (orig_LoadoutAttachItem_SetData != nullptr)
        orig_LoadoutAttachItem_SetData(instance, attachInfo, panel);

    if (AttachPtr(attachInfo) &&
        AttachPtr(previewConf) &&
        *(void**)((uintptr_t)attachInfo + 0x28) != previewConf) {
        *(void**)((uintptr_t)attachInfo + 0x28) = previewConf;
    }

    if (AttachPtr(instance) &&
        *(uint*)((uintptr_t)instance + 0xA0) != uiMythic) {
        *(uint*)((uintptr_t)instance + 0xA0) = uiMythic;
    }

    if (!AttachPtr(attachInfo) || uiMythic == 0)
        return;

    void* part = *(void**)((uintptr_t)attachInfo + 0x28);
    if (!AttachPtr(item) || !AttachPtr(part))
        return;

    const uint partItemId = *(uint*)((uintptr_t)part + 0x58);
    if (attachType <= 0 ||
        attachType >= 31 ||
        !IsRealMythicAttachSkin(partItemId)) {
        return;
    }

    activeMythicAttachments[(int)weaponId][attachType] =
        (int)partItemId;
    runtimeAttachMythicById[attachId] = partItemId;

    if (*(uint*)((uintptr_t)item + 0x18) != partItemId)
        *(uint*)((uintptr_t)item + 0x18) = partItemId;

    if (AttachPtr(instance) &&
        *(uint*)((uintptr_t)instance + 0xA0) != partItemId) {
        *(uint*)((uintptr_t)instance + 0xA0) = partItemId;
    }
}

inline void hook_BRWeaponAttachItemData_Ctor(
    void* instance,
    uint weaponActorId,
    uint attachId,
    int index,
    uint mythicId,
    int attachSkinId,
    bool disable,
    uint dmzItemId)
{
    const int attachType = GetAttachmentType(attachId);
    if (IsWeaponItemId(weaponActorId))
        runtimeAttachWeaponId = weaponActorId;

    const int selected =
        ActiveAttachForContext(weaponActorId, attachType);
    const uint next = selected > 0 ? (uint)selected : mythicId;

    if (orig_BRWeaponAttachItemData_Ctor != nullptr) {
        orig_BRWeaponAttachItemData_Ctor(
            instance,
            weaponActorId,
            attachId,
            index,
            next,
            attachSkinId,
            disable,
            dmzItemId);
    }

    if (selected > 0 && AttachPtr(instance))
        *(uint*)((uintptr_t)instance + 0x14) = (uint)selected;
}

inline void hook_AttachItem_Ctor(
    void* instance,
    uint itemId,
    int skinId,
    uint mythicSkinId)
{
    const int attachType = GetAttachmentType(itemId);
    if (attachType == 0 && IsWeaponItemId(itemId))
        runtimeAttachWeaponId = itemId;

    const int selected =
        ActiveAttachForContext(runtimeAttachWeaponId, attachType);
    auto cached = runtimeAttachMythicById.find(itemId);
    const uint next = selected > 0
        ? (uint)selected
        : (mythicSkinId > 0
            ? mythicSkinId
            : (cached != runtimeAttachMythicById.end()
                ? cached->second
                : 0));

    if (orig_AttachItem_Ctor != nullptr)
        orig_AttachItem_Ctor(instance, itemId, skinId, next);

    if (next > 0) {
        runtimeAttachMythicById[itemId] = next;
        if (AttachPtr(instance) && *(uint*)((uintptr_t)instance + 0x18) != next)
            *(uint*)((uintptr_t)instance + 0x18) = next;
    }
}

inline void hook_AttachItem_SetData(
    void* instance,
    uint itemId,
    int skinId,
    uint mythicSkinId)
{
    const int attachType = GetAttachmentType(itemId);
    if (attachType == 0 && IsWeaponItemId(itemId))
        runtimeAttachWeaponId = itemId;

    const int selected =
        ActiveAttachForContext(runtimeAttachWeaponId, attachType);
    auto cached = runtimeAttachMythicById.find(itemId);
    const uint next = selected > 0
        ? (uint)selected
        : (mythicSkinId > 0
            ? mythicSkinId
            : (cached != runtimeAttachMythicById.end()
                ? cached->second
                : 0));

    if (orig_AttachItem_SetData != nullptr)
        orig_AttachItem_SetData(instance, itemId, skinId, next);

    if (next > 0) {
        runtimeAttachMythicById[itemId] = next;
        if (AttachPtr(instance) && *(uint*)((uintptr_t)instance + 0x18) != next)
            *(uint*)((uintptr_t)instance + 0x18) = next;
    }
}

inline void hook_AttachItem_SetMythicSkinID(
    void* instance,
    int skinId,
    uint mythicSkinId)
{
    uint itemId = 0;
    if (AttachPtr(instance))
        itemId = *(uint*)((uintptr_t)instance + 0x10);

    const int attachType = GetAttachmentType(itemId);
    if (attachType == 0 && IsWeaponItemId(itemId))
        runtimeAttachWeaponId = itemId;

    const int selected =
        ActiveAttachForContext(runtimeAttachWeaponId, attachType);
    auto cached = runtimeAttachMythicById.find(itemId);
    const uint next = selected > 0
        ? (uint)selected
        : (mythicSkinId > 0
            ? mythicSkinId
            : (cached != runtimeAttachMythicById.end()
                ? cached->second
                : 0));

    if (orig_AttachItem_SetMythicSkinID != nullptr)
        orig_AttachItem_SetMythicSkinID(instance, skinId, next);

    if (next > 0) {
        runtimeAttachMythicById[itemId] = next;
        if (AttachPtr(instance) && *(uint*)((uintptr_t)instance + 0x18) != next)
            *(uint*)((uintptr_t)instance + 0x18) = next;
    }
}

inline void hook_AttachItem_CopyEquipped(void* instance, void* item)
{
    uint attachId = 0;
    if (AttachPtr(item))
        attachId = *(uint*)((uintptr_t)item + 0x3C);

    const int attachType = GetAttachmentType(attachId);
    const int selected =
        ActiveAttachForContext(runtimeAttachWeaponId, attachType);

    if (orig_AttachItem_CopyEquipped != nullptr)
        orig_AttachItem_CopyEquipped(instance, item);

    if (AttachPtr(instance) &&
        IsRealMythicAttachSkin((uint)selected)) {
        *(uint*)((uintptr_t)instance + 0x18) = (uint)selected;
    }
}

inline uint hook_AttachItem_GetModelAttachId(void* instance)
{
    if (AttachPtr(instance)) {
        const uint attachId =
            *(uint*)((uintptr_t)instance + 0x10);
        const int attachType = GetAttachmentType(attachId);
        const int selected =
            ActiveAttachForContext(runtimeAttachWeaponId, attachType);
        if (IsRealMythicAttachSkin((uint)selected))
            return (uint)selected;
    }

    return orig_AttachItem_GetModelAttachId != nullptr
        ? orig_AttachItem_GetModelAttachId(instance)
        : 0;
}

inline void hook_AttachSkinData_Ctor(
    void* instance,
    uint blueprintWeaponId,
    int skinId,
    uint mythicSkinId)
{
    const int attachType = GetAttachmentType((uint)skinId);
    if (IsWeaponItemId(blueprintWeaponId))
        runtimeAttachWeaponId = blueprintWeaponId;

    const int selected =
        ActiveAttachForContext(blueprintWeaponId, attachType);
    const uint next = selected > 0
        ? (uint)selected
        : mythicSkinId;

    if (orig_AttachSkinData_Ctor != nullptr) {
        orig_AttachSkinData_Ctor(
            instance,
            blueprintWeaponId,
            skinId,
            next);
    }
}

inline std::array<int, 31> BuildMythicAttach(
    int confId,
    int baseId,
    bool isMythic)
{
    std::array<int, 31> result{};

    for (void* weaponSkin : SnapshotWeaponSkinConfigs()) {
        if (!AttachPtr(weaponSkin))
            continue;

        auto* fields =
            (WeaponSkinFields*)((uintptr_t)weaponSkin + 0x10);
        if (!AttachPtr(fields))
            continue;

        const bool match =
            (int)fields->ID == confId ||
            (int)fields->WeaponID == baseId ||
            (int)fields->BRWeaponID == baseId ||
            fields->BrSkinWeaponID == baseId;
        if (!match)
            continue;

        for (int attachType : MythicAttachTypes()) {
            if (attachType <= 0 ||
                attachType >= (int)result.size()) {
                continue;
            }

            const int mythicId =
                RealMythicForType(fields, attachType);
            if (mythicId > 0)
                result[attachType] = mythicId;
        }
    }

    if (isMythic && confId > 0) {
        static const std::array<int, 8> previewTypes = {
            1, 4, 5, 6, 8, 9, 10, 12
        };

        for (int attachType : previewTypes) {
            if (attachType > 0 &&
                attachType < (int)result.size() &&
                result[attachType] <= 0) {
                result[attachType] = confId;
            }
        }
    }

    return result;
}

struct AttachWeaponIds {
    int baseId;
    int extraId;
    int skinId;
    int itemId;
    int lootId;
};

// Defined with the installers at the bottom of this header; declared here
// because applying a loadout is what triggers the install.
inline bool EnsureRuntimeInstalled();

inline void ApplyMythicAttachments(
    const AttachWeaponIds& ids,
    const std::array<int, 31>& mythicAttach)
{
    if (!MythicAttachmentsEnabled()) {
        activeWeaponSkins.clear();
        activeMythicAttachments.clear();
        runtimeAttachMythicById.clear();
        runtimeAttachResolveCache.clear();
        loadoutPreviewTiles.clear();
        runtimeAttachWeaponId = 0;
        return;
    }

    EnsureRuntimeInstalled();

    activeWeaponSkins.clear();
    activeMythicAttachments.clear();
    runtimeAttachMythicById.clear();
    runtimeAttachResolveCache.clear();
    loadoutPreviewTiles.clear();
    runtimeAttachWeaponId = 0;
    runtimeAttachSkinId = ids.skinId > 0 ? (uint)ids.skinId : 0;

    const std::array<int, 5> keys = {
        ids.baseId,
        ids.extraId,
        ids.skinId,
        ids.itemId,
        ids.lootId
    };

    for (int key : keys) {
        if (key <= 0)
            continue;

        activeMythicAttachments[key] = mythicAttach;
        if (ids.skinId > 0)
            activeWeaponSkins[key] = ids.skinId;
    }

    for (void* weaponSkin : SnapshotWeaponSkinConfigs()) {
        if (!AttachPtr(weaponSkin)) continue;
        auto* fields = (WeaponSkinFields*)((uintptr_t)weaponSkin + 0x10);
        if (!AttachPtr(fields)) continue;
        const bool selected =
            (ids.skinId > 0 && (int)fields->ID == ids.skinId) ||
            (ids.baseId > 0 && ((int)fields->WeaponID == ids.baseId ||
                                (int)fields->BRWeaponID == ids.baseId ||
                                fields->BrSkinWeaponID == ids.baseId));
        if (!selected) continue;
        const int aliases[] = {(int)fields->ID, (int)fields->WeaponID,
                               (int)fields->BRWeaponID, fields->BrSkinWeaponID};
        for (int alias : aliases) {
            if (alias <= 0) continue;
            activeMythicAttachments[alias] = mythicAttach;
            if (ids.skinId > 0) activeWeaponSkins[alias] = ids.skinId;
        }
    }

    activeMythicAttachFallback = mythicAttach;
}

inline void ClearMythicAttachments(const AttachWeaponIds& ids)
{
    const std::array<int, 5> keys = {
        ids.baseId,
        ids.extraId,
        ids.skinId,
        ids.itemId,
        ids.lootId
    };

    for (int key : keys) {
        if (key <= 0)
            continue;

        activeMythicAttachments.erase(key);
        activeWeaponSkins.erase(key);
    }

    runtimeAttachResolveCache.clear();
    loadoutPreviewTiles.clear();
    runtimeAttachMythicById.clear();
    activeMythicAttachFallback.fill(0);
    runtimeAttachWeaponId = 0;
    runtimeAttachSkinId = 0;
}

inline void (*orig_WeaponPartAsset_Ctor)(void*) = nullptr;
inline void (*orig_AttachmentConf_Ctor)(void*) = nullptr;

// These run on the game's own config-construction path, which starts in the
// first second of the process and builds objects by the thousand. Three things
// keep that path from being a launch hazard: they are inert while the switch is
// off, the retained set is capped, and membership is O(1) instead of a linear
// scan under a global mutex.
constexpr size_t kMaxCollectedConfigs = 4096;

inline void RetainOnce(std::vector<void*>& vec, std::unordered_set<void*>& seen, void* instance)
{
    if (vec.size() >= kMaxCollectedConfigs) return;
    if (!seen.insert(instance).second) return;
    vec.push_back(instance);
}

inline void hook_WeaponPartAsset_Ctor(void* instance) {
    if (orig_WeaponPartAsset_Ctor) orig_WeaponPartAsset_Ctor(instance);
    if (!MythicAttachmentsEnabled()) return;
    if (AttachPtr(instance)) {
        std::lock_guard<std::mutex> lock(mythicCollectorMutex);
        RetainOnce(weaponPartAssetInstance, weaponPartAssetSeen, instance);
    }
}

inline void hook_AttachmentConf_Ctor(void* instance) {
    if (orig_AttachmentConf_Ctor) orig_AttachmentConf_Ctor(instance);
    if (!MythicAttachmentsEnabled()) return;
    if (AttachPtr(instance)) {
        std::lock_guard<std::mutex> lock(mythicCollectorMutex);
        RetainOnce(attachmentConfInstance, attachmentConfSeen, instance);
    }
}

inline std::atomic<bool> mythicCollectorsInstalled{false};
inline std::atomic<bool> mythicRuntimeInstalled{false};

inline bool InstallMythicAttachmentCollectorsFresh(uintptr_t unityBase) {
    if (mythicCollectorsInstalled.load(std::memory_order_acquire)) return true;
    if (!unityBase) return false;

    auto hookRva = [unityBase](uintptr_t rva, void* hook, void** orig) {
        void* target = (void*)(unityBase + rva);
        if (!target || !Tools::IsPtrValid(target)) return false;
        Tools::Hook(target, hook, orig);
        return true;
    };

    hookRva(WeaponPartAssetAddress, (void*)hook_WeaponPartAsset_Ctor,
            (void**)&orig_WeaponPartAsset_Ctor);
    hookRva(AttachmentConfAddress, (void*)hook_AttachmentConf_Ctor,
            (void**)&orig_AttachmentConf_Ctor);

    mythicCollectorsInstalled.store(true, std::memory_order_release);
    return true;
}

inline bool InstallMythicAttachmentRuntimeFresh(uintptr_t unityBase) {
    if (mythicRuntimeInstalled.load(std::memory_order_acquire)) return true;
    if (!unityBase) return false;

    auto hookRva = [unityBase](uintptr_t rva, void* hook, void** orig) {
        void* target = (void*)(unityBase + rva);
        if (!target || !Tools::IsPtrValid(target)) return false;
        Tools::Hook(target, hook, orig);
        return true;
    };

    hookRva(AttachmentGroupNewCtorAddress, (void*)hook_AttachmentGroupNew_Ctor,
            (void**)&orig_AttachmentGroupNew_Ctor);
    hookRva(AttachmentGroupNewItemCtorAddress, (void*)hook_AttachmentGroupNew_ItemCtor,
            (void**)&orig_AttachmentGroupNew_ItemCtor);
    hookRva(AttachmentGroupNewGetColorIdAddress, (void*)hook_AttachmentGroupNew_GetColorId,
            (void**)&orig_AttachmentGroupNew_GetColorId);
    hookRva(BRWeaponAttachItemDataCtorAddress, (void*)hook_BRWeaponAttachItemData_Ctor,
            (void**)&orig_BRWeaponAttachItemData_Ctor);
    hookRva(AttachItemCtorAddress, (void*)hook_AttachItem_Ctor,
            (void**)&orig_AttachItem_Ctor);
    hookRva(AttachItemSetDataAddress, (void*)hook_AttachItem_SetData,
            (void**)&orig_AttachItem_SetData);
    hookRva(AttachItemSetMythicSkinIdAddress, (void*)hook_AttachItem_SetMythicSkinID,
            (void**)&orig_AttachItem_SetMythicSkinID);
    hookRva(AttachItemCopyEquippedAddress, (void*)hook_AttachItem_CopyEquipped,
            (void**)&orig_AttachItem_CopyEquipped);
    hookRva(AttachItemGetModelAttachIdAddress, (void*)hook_AttachItem_GetModelAttachId,
            (void**)&orig_AttachItem_GetModelAttachId);
    hookRva(AttachSkinDataCtorAddress, (void*)hook_AttachSkinData_Ctor,
            (void**)&orig_AttachSkinData_Ctor);
    hookRva(LoadoutAttachItemSetDataAddress, (void*)hook_LoadoutAttachItem_SetData,
            (void**)&orig_LoadoutAttachItem_SetData);
    hookRva(WeaponInitPartAddress, (void*)hook_Weapon_InitPart,
            (void**)&orig_Weapon_InitPart);

    mythicRuntimeInstalled.store(true, std::memory_order_release);
    return true;
}

inline bool InstallMythicAttachmentHooksFresh(uintptr_t unityBase) {
    return InstallMythicAttachmentCollectorsFresh(unityBase) &&
           InstallMythicAttachmentRuntimeFresh(unityBase);
}

// Installed on first use rather than during startup. Sixteen hooks on game
// constructors and loadout methods is a lot of surface to take before the client
// has shown its login screen, and the runtime they serve cannot do anything
// until ApplyMythicAttachments has filled its tables.
inline bool EnsureRuntimeInstalled() {
    if (!MythicAttachmentsEnabled()) return false;
    if (mythicRuntimeInstalled.load(std::memory_order_acquire)) return true;

    const uintptr_t base = (uintptr_t)getAbsoluteAddress("libunity.so", 0);
    if (base == 0) return false;
    return InstallMythicAttachmentHooksFresh(base);
}
}

