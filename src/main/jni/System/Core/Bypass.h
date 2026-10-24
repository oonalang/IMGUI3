#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unistd.h>

#include "Includes/Logger.h"

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";

#endif

inline void InitializeProtection() {
#if defined(__aarch64__)
    static std::once_flag once;
    std::call_once(once, []() {
        for (int i = 0; i < 200; ++i) {
            if (Tools::IsPtrValid((void *)Tools::GetBaseAddress(OBFUSCATE("libanogs.so")))) break;
            usleep(100 * 1000);
        }
        if (!Tools::IsPtrValid((void *)Tools::GetBaseAddress(OBFUSCATE("libanogs.so")))) {
            LOGE("[Bypass] libanogs.so never mapped; the bypass set was not applied");
            return;
        }

        const uintptr_t base = (uintptr_t)Tools::GetBaseAddress(OBFUSCATE("libanogs.so"));

        MemoryPatch::createWithHex("libanogs.so", 0x204218, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x2B9D8C, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x2E4ED8, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x325CC8, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x4495A4, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x470A04, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x4F9DDC, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x3893D8, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x455A80, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x497244, "00 00 80 D2 C0 03 5F D6").Modify();
        MemoryPatch::createWithHex("libanogs.so", 0x4AFC1C, "00 00 80 D2 C0 03 5F D6").Modify();

        Tools::Hook((void *)(base + 0x204218), (void *)hook_bypass, (void **)&orig_bypass);
        Tools::Hook((void *)(base + 0x455A80), (void *)hook_bypass, (void **)&orig_bypass);
        Tools::Hook((void *)(base + 0x497244), (void *)hook_bypass, (void **)&orig_bypass);

        LOGI("[Bypass] anogs set applied");
    });
#endif
}
