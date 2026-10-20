#pragma once

// Attachment unlock - the one answer VARIANT4 ships.
//
// github.com/oonalang/VARIANT4 is the build this feature is taken from, and it
// asks the game exactly two questions: BlueprintWeaponInfo.IsUnlocked() for the
// Blueprints switch and WeaponPromotionInfo.IsWeaponAttachmentUnlock() for this
// one. Nothing else.
//
// This file used to carry ten hooks - the four "is it unlocked" answers, two
// data writes that forced AttachmentGroupNew.OwnState back to EUnlock, and the
// MythicAttachmentSystem module that hooked the loadout's SetData builders. That
// stack is why "Attachment unlock" did nothing useful: two of its trampolines
// sat on the same prologue (a trampoline each, so both broke), and the row
// rewrites put a value on objects the popup had already decided about, which is
// a state the UI reads as locked again. The extra hooks are gone; what is left
// is the answer the game actually asks.
//
// The toggle stays the menu's "Attachment unlock" (Hacks/Feature/Init_Hacks.h,
// Config.ExtraMenu.Attachment) and is independent of "Blueprint"
// (Config.ExtraMenu.MythicAttachments), which is BlueprintWeaponInfo.IsUnlocked
// in Feats.h.
#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace mythic_attachments
{
    // libunity.so RVA of WeaponPromotionInfo.IsWeaponAttachmentUnlock(void*) -
    // the dump's address, i.e. what getAbsoluteAddress() wants. Feats.h installs
    // the hook from here.
    constexpr uintptr_t Rva_IsWeaponAttachmentUnlock = 0xA61AA00;

    // "Treat every attachment as owned."
    inline bool AttachmentsUnlocked()
    {
        return Config.ExtraMenu.Attachment;
    }
}

// WeaponPromotionInfo.IsWeaponAttachmentUnlock - "does this weapon own the
// attachment in question". With the switch on the answer is yes; with it off
// the game's own answer is passed through untouched, so stock behaviour is
// exactly stock behaviour.
inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance) = nullptr;
inline bool IsWeaponAttachmentUnlock(void *instance)
{
    if (instance != nullptr && mythic_attachments::AttachmentsUnlocked())
        return true;
    return orig_IsWeaponAttachmentUnlock != nullptr
         ? orig_IsWeaponAttachmentUnlock(instance)
         : false;
}
