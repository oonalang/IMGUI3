#pragma once

#include <cstdint>
#include <mutex>
#include <unistd.h>

#include "Includes/Logger.h"

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

// mov x0, #0 ; ret
inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";

#endif

// ---------------------------------------------------------------------------
// RAVEN set (tested, current libanogs build).
//
// Two kinds of entry, and they are not interchangeable:
//
//  * patched: the routine is replaced with "return 0" outright. These are the
//    report/check paths nothing else calls into, so nothing needs the original.
//  * hooked: the same "return 0" through a trampoline instead of a byte patch.
//    Kept for the three entry points that are also in the patch list below, so
//    the set matches what was tested; the two must never be applied to the same
//    address - a hook's jump and a hex patch overwrite each other and whichever
//    lands second leaves a broken prologue.
// ---------------------------------------------------------------------------
namespace bypass_raven
{
    // Patched with "mov x0, #0; ret".
    constexpr uintptr_t kPatched[] = {
        0x2B9D8C,
        0x2E4ED8,
        0x325CC8,
        0x3893D8,
        0x4495A4,
        0x470A04,
        0x4AFC1C,
        0x4F9DDC,
        0x1,
        0x10B,
        0x1F3078,
        0x2BE47C,
        0x2D7B5C,
        0x2E8964,
        0x404444,
        0x448E68,
        0x4987A8,
        0x4A9A00,
        0x4EDAD8,
    };

    // Redirected to hook_bypass().
    constexpr uintptr_t kHooked[] = {
        0x204218,
        0x455A80,
        0x497244,
    };

    constexpr int kPatchCount = (int)(sizeof kPatched / sizeof kPatched[0]);
    constexpr int kHookCount  = (int)(sizeof kHooked  / sizeof kHooked[0]);

    inline bool LibraryReady()
    {
        return Tools::IsPtrValid((void *)Tools::GetBaseAddress(OBFUSCATE("libanogs.so")));
    }
}

inline void InitializeProtection() {
#if defined(__aarch64__)
    // Called from two init threads, and libanogs is mapped by the game's own
    // startup - which can be after us. Resolving an address before the library
    // is there patches address 0, and hooking the same offset twice corrupts
    // the trampoline, so: wait for the library, and run the set exactly once.
    static std::once_flag once;
    std::call_once(once, []() {
        for (int i = 0; i < 200; ++i) {
            if (bypass_raven::LibraryReady()) break;
            usleep(100 * 1000);
        }
        if (!bypass_raven::LibraryReady()) {
            LOGE("[Bypass] libanogs.so never mapped; the bypass set was not applied");
            return;
        }

        int patched = 0;
        for (int i = 0; i < bypass_raven::kPatchCount; ++i) {
            const uintptr_t off = bypass_raven::kPatched[i];
            MemoryPatch patch = MemoryPatch::createWithHex(
                OBFUSCATE("libanogs.so"), off, armFalse);
            if (!patch.isValid()) {
                LOGE("[Bypass] patch 0x%lX did not resolve",
                     (unsigned long)off);
                continue;
            }
            if (patch.Modify()) patched++;
            else LOGE("[Bypass] patch 0x%lX could not be written",
                      (unsigned long)off);
        }

        int hooked = 0;
        for (int i = 0; i < bypass_raven::kHookCount; ++i) {
            const uintptr_t off = bypass_raven::kHooked[i];
            void *addr = reinterpret_cast<void *>(KittyMemory::getAbsoluteAddress(OBFUSCATE("libanogs.so"), off));
            if (!Tools::IsPtrValid(addr)) {
                LOGE("[Bypass] hook 0x%lX did not resolve", (unsigned long)off);
                continue;
            }
            Tools::Hook(addr, (void *)hook_bypass, (void **)&orig_bypass);
            hooked++;
        }

        LOGI("[Bypass] raven set applied: %d/%d patched, %d/%d hooked",
             patched, bypass_raven::kPatchCount, hooked, bypass_raven::kHookCount);
    });
#endif
}
