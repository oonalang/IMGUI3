#pragma once

#include "Hacks/Feature/Init_Hacks.h"

#include <cstdint>

namespace mythic_attachments
{
    constexpr uintptr_t Rva_IsWeaponAttachmentUnlock = 0xA61AA00;

    inline bool AttachmentsUnlocked()
    {
        return Config.ExtraMenu.Attachment;
    }
}

inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance, int attachment) = nullptr;
inline bool IsWeaponAttachmentUnlock(void *instance, int attachment)
{
    if (instance != nullptr && mythic_attachments::AttachmentsUnlocked())
        return true;
    return orig_IsWeaponAttachmentUnlock != nullptr
         ? orig_IsWeaponAttachmentUnlock(instance, attachment)
         : false;
}

