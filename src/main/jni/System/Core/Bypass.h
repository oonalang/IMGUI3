#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unistd.h>

#include "Includes/Logger.h"

void *GetAnogs();

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";

#endif

namespace bypass_anogs
{
    constexpr uintptr_t kPatched[] = {
        0x2D2A70, 0x30B87C, 0x438154, 0x44A714,
        0x20A39C, 0x20AE78, 0x25FA98, 0x2AA880, 0x2B63E4,
        0x36F0AC, 0x334310, 0x39AE94, 0x44BC90,
        0x491EDC, 0x497E64, 0x4B68A0, 0x4B9C10,
        0x2B9D8C, 0x2E4ED8, 0x325CC8, 0x3893D8,
        0x4495A4, 0x470A04, 0x4AFC1C, 0x4F9DDC,
    };

    constexpr uintptr_t kHooked[] = {
        0x204218,
        0x455A80,
        0x497244,
    };

    constexpr int kPatchCount = (int)(sizeof kPatched / sizeof kPatched[0]);
    constexpr int kHookCount  = (int)(sizeof kHooked / sizeof kHooked[0]);

    inline uintptr_t ModuleBase()
    {
        return (uintptr_t)Tools::GetBaseAddress(OBFUSCATE("libanogs.so"));
    }

    inline uintptr_t SegmentDelta()
    {
        static uintptr_t cached = 0;
        static bool resolved = false;

        if (resolved)
            return cached;
        resolved = true;

        const uintptr_t module = ModuleBase();
        if (module == 0)
            return cached;

        FILE *map = fopen(OBFUSCATE("/proc/self/maps"), "r");
        if (map == nullptr)
            return cached;

        char line[256];
        while (fgets(line, sizeof(line), map) != nullptr) {
            if (strstr(line, OBFUSCATE("libanogs.so")) == nullptr ||
                strstr(line, OBFUSCATE("r-xp")) == nullptr)
                continue;

            uintptr_t start = 0;
            if (sscanf(line, OBFUSCATE("%lx"), &start) != 1 || start < module)
                continue;

            cached = start - module;
            break;
        }
        fclose(map);
        return cached;
    }

    inline void *Target(uintptr_t rva)
    {
        const uintptr_t privateBase = (uintptr_t)GetAnogs();
        const uintptr_t delta = SegmentDelta();

        if (privateBase != 0)
            return (void *)(privateBase + (rva >= delta ? rva - delta : rva));

        const uintptr_t module = ModuleBase();
        return module != 0 ? (void *)(module + rva) : nullptr;
    }

    inline bool LibraryReady()
    {
        return Tools::IsPtrValid((void *)ModuleBase());
    }
}

inline void InitializeProtection() {
#if defined(__aarch64__)
    static std::once_flag once;
    std::call_once(once, []() {
        for (int i = 0; i < 200; ++i) {
            if (bypass_anogs::LibraryReady() && GetAnogs() != nullptr) break;
            usleep(100 * 1000);
        }
        if (!bypass_anogs::LibraryReady()) {
            LOGE("[Bypass] libanogs.so never mapped; the bypass set was not applied");
            return;
        }

        const std::string patchBytes(armFalse);

        int patched = 0;
        for (int i = 0; i < bypass_anogs::kPatchCount; ++i) {
            const uintptr_t off = bypass_anogs::kPatched[i];
            void *target = bypass_anogs::Target(off);
            if (!Tools::IsPtrValid(target)) {
                LOGE("[Bypass] patch 0x%lX did not resolve", (unsigned long)off);
                continue;
            }
            MemoryPatch patch = MemoryPatch::createWithHex((uintptr_t)target, patchBytes);
            if (!patch.isValid()) {
                LOGE("[Bypass] patch 0x%lX is not writable", (unsigned long)off);
                continue;
            }
            if (patch.Modify()) patched++;
            else LOGE("[Bypass] patch 0x%lX could not be written", (unsigned long)off);
        }

        int hooked = 0;
        for (int i = 0; i < bypass_anogs::kHookCount; ++i) {
            const uintptr_t off = bypass_anogs::kHooked[i];
            void *target = bypass_anogs::Target(off);
            if (!Tools::IsPtrValid(target)) {
                LOGE("[Bypass] hook 0x%lX did not resolve", (unsigned long)off);
                continue;
            }
            Tools::Hook(target, (void *)hook_bypass, (void **)&orig_bypass);
            hooked++;
        }

        LOGI("[Bypass] anogs set applied: %d/%d patched, %d/%d hooked",
             patched, bypass_anogs::kPatchCount, hooked, bypass_anogs::kHookCount);
    });
#endif
}
