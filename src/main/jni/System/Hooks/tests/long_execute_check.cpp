// Long Execute only widens a finisher that already exists. The check drives the hook
// against a fake ExecutionComponent whose ExecuteParameters block sits where the dump
// puts it (m_FixParamConf at 0x30, m_fixMaxDistance at 0x34, m_fixViewRange at 0x3C)
// and asserts the three rules: raise, never lower, never invent a range from zero.

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "System/Hooks/LongExecute.h"

bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }
extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }
void MSHookFunction(void *, void *, void **) {}

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

struct FakeComponent
{
    unsigned char bytes[0x100];
};

static float *MaxDistance(FakeComponent &c) { return (float *)(c.bytes + 0x34); }
static float *ViewRange(FakeComponent &c)   { return (float *)(c.bytes + 0x3C); }

static int g_origCalls = 0;
static float g_lastOut = -1.0f;

static int fakeCheckTargetIsValid(void *self, void *pawn, float *outDistance)
{
    g_origCalls++;
    (void)self;
    (void)pawn;
    if (outDistance != nullptr) *outDistance = 1.25f;
    return 1;
}

int main()
{
    long_execute::orig_CheckTargetIsValid = fakeCheckTargetIsValid;

    FakeComponent component{};
    *MaxDistance(component) = 2.0f;
    *ViewRange(component)   = 3.0f;

    CHECK(Config.ExtraMenu.LongExecute == false);
    CHECK(long_execute::WantedRange() == 8.0f);

    // Off: the component is untouched and the original still runs.
    g_origCalls = 0;
    g_lastOut = -1.0f;
    long_execute::Hook_CheckTargetIsValid(component.bytes, nullptr, &g_lastOut);
    CHECK(g_origCalls == 1);
    CHECK(g_lastOut == 1.25f);
    CHECK(*MaxDistance(component) == 2.0f);
    CHECK(*ViewRange(component) == 3.0f);

    // On: both values are raised to the wanted range, the out distance still comes back.
    Config.ExtraMenu.LongExecute = true;
    Config.ExtraMenu.LongExecuteRange = 8.0f;
    g_origCalls = 0;
    long_execute::Hook_CheckTargetIsValid(component.bytes, nullptr, &g_lastOut);
    CHECK(g_origCalls == 1);
    CHECK(*MaxDistance(component) == 8.0f);
    CHECK(*ViewRange(component) == 8.0f);

    // Already longer than asked: never lowered.
    *MaxDistance(component) = 19.0f;
    *ViewRange(component)   = 19.0f;
    long_execute::Hook_CheckTargetIsValid(component.bytes, nullptr, nullptr);
    CHECK(*MaxDistance(component) == 19.0f);
    CHECK(*ViewRange(component) == 19.0f);

    // A weapon without a finisher keeps its zero range.
    *MaxDistance(component) = 0.0f;
    *ViewRange(component)   = 0.0f;
    long_execute::Hook_CheckTargetIsValid(component.bytes, nullptr, nullptr);
    CHECK(*MaxDistance(component) == 0.0f);
    CHECK(*ViewRange(component)   == 0.0f);

    // The slider is clamped into a sane band before it ever reaches the component.
    Config.ExtraMenu.LongExecuteRange = 500.0f;
    CHECK(long_execute::WantedRange() == long_execute::kMaxRange);
    Config.ExtraMenu.LongExecuteRange = 0.0f;
    CHECK(long_execute::WantedRange() == long_execute::kMinRange);
    Config.ExtraMenu.LongExecuteRange = 8.0f;

    // A null component is rejected instead of dereferenced.
    long_execute::Widen(nullptr);

    if (fails)
    {
        std::printf("FAIL: %d check(s)\n", fails);
        return 1;
    }

    std::printf("PASS long_execute_check: raise only, never lower, never invent\n");
    return 0;
}
