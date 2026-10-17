// Host test for the "crashes when I join a game" report, with only Unlock
// Blueprints enabled.
//
// The chain: unlocking blueprints is the only way to equip one, equipping is the
// only thing that sets the apply-on-equip pending id, and the pending id is what
// makes Skin/AutoEquipSkin.h walk `weaponConfInstance`. Walking the LIVE vector
// races the WeaponConfConfig ctor hooks, which append to it on the game thread -
// and joining a match builds hundreds of confs in a burst, so the realloc lands
// under an in-flight iteration. That is a SIGSEGV that only shows up at load.
//
// This test is the measurement: one thread behaves like the ctor hook (append
// under g_dataMutex), the other calls the real AutoEquipSkin_Tick(), which must
// take the same lock before it touches the table. Run under ThreadSanitizer:
// pre-fix it reports a data race on the vector; post-fix it is clean.
//
//   g++ -std=c++17 -fsanitize=thread -D__fastcall= \
//       -I Skin/tests/stubs -I . -I ImGui -I Includes \
//       -o /tmp/auto_equip_race Skin/tests/auto_equip_race_check.cpp \
//       IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "Skin/Fields.h"
#include "Skin/AutoEquipSkin.h"

// Skin/Thread.h owns these in the app; this test is the only other place they live.
std::vector<void *> weaponConfInstance;
std::vector<void *> weaponSkinConfigInstance;
std::mutex g_dataMutex;

// Tools.cpp references the inline-hook engine and MD5; neither is reached here.
#include <openssl/md5.h>
extern "C" int DobbyHook(void *address, void *replace_call, void **origin_call)
{
    (void)address; (void)replace_call; (void)origin_call;
    return 0;
}
extern "C" int MD5_Init(MD5_CTX *) { return 0; }
extern "C" int MD5_Update(MD5_CTX *, const void *, size_t) { return 0; }
extern "C" int MD5_Final(unsigned char *, MD5_CTX *) { return 0; }

static const size_t OBJ_SIZE = 0x80;

// One fake WeaponConfConfig: class word, m_CachedPtr @0x10, then the +0x20 cast
// base with DefWeaponSkinID @0x18 and ID @0x20 (real 0x38 / 0x40).
static void *MakeConf(uint id, uint baseWeaponId)
{
    auto *raw = (unsigned char *)std::calloc(1, OBJ_SIZE);
    *(void **)(raw + 0x00) = raw;          // Il2CppClass* stand-in
    *(void **)(raw + 0x10) = raw;          // m_CachedPtr
    *(uint *)(raw + 0x20 + 0x14) = baseWeaponId;
    *(uint *)(raw + 0x20 + 0x20) = id;
    return raw;
}

int main()
{
    // The gun and the skin that goes on it exist from the start.
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponConfInstance.push_back(MakeConf(800, 800));
        weaponConfInstance.push_back(MakeConf(900, 800));
    }

    std::atomic<bool> stop{false};
    std::atomic<int> appended{0};

    // The ctor hook: append new confs under the lock, exactly like Skin/Thread.h.
    // Bounded, with a small pause, so the vector keeps REALLOCATING across the
    // reader's walks without the test itself becoming a memory hog.
    std::thread writer([&]() {
        for (int i = 0; i < 2000; ++i)
        {
            {
                std::lock_guard<std::mutex> lock(g_dataMutex);
                weaponConfInstance.push_back(MakeConf((uint)(1000 + i), 800));
            }
            appended.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        stop.store(true, std::memory_order_relaxed);
    });

    // The render thread: an equip is pending on every frame, so the table is
    // walked by the real AutoEquipSkin_Tick() while the writer is appending.
    int ticks = 0;
    while (!stop.load(std::memory_order_relaxed))
    {
        AutoEquipSkin_NoteCandidate(900);
        AutoEquipSkin_Tick(true);
        ++ticks;
    }
    writer.join();

    const int n = appended.load();
    printf("%d confs appended while %d ticks walked the table (%d equips resolved)\n",
           n, ticks, g_autoEquipSkinDiag.resolved);
    printf("%s\n", n > 0 ? "OK: no crash and no unsynchronised table walk" : "FAILED: writer never ran");
    return n > 0 ? 0 : 1;
}
