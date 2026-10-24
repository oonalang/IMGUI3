#pragma once

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace mythic_attachments
{
    constexpr uintptr_t Rva_IsWeaponAttachmentUnlock = 0xA61AA00;

    // "Attachment unlock" and the blueprint toggle both mean "this player owns
    // everything". The game asks some of these questions through a static
    // helper, so it passes a null instance; requiring an instance made the
    // honest answer ("locked") win on exactly those calls, which is why the
    // switch did nothing in the loadout.
    inline bool AttachmentsUnlocked()
    {
        return Config.ExtraMenu.Attachment || Config.ExtraMenu.MythicAttachments;
    }
}

inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance, int attachment) = nullptr;
inline bool IsWeaponAttachmentUnlock(void *instance, int attachment)
{
    if (mythic_attachments::AttachmentsUnlocked())
        return true;
    return orig_IsWeaponAttachmentUnlock != nullptr
         ? orig_IsWeaponAttachmentUnlock(instance, attachment)
         : false;
}
