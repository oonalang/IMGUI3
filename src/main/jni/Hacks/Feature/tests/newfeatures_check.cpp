// Host check for the triggerbot's fault guard. It compiles the real
// NewFeatures.h and AimLock.h against the game SDK headers and exercises the
// guard semantics: a fault inside a guarded section is caught and counted, the
// same key is skipped while it cools down, and other keys are unaffected.
//
//   g++ -std=c++17 -D__fastcall= -Isrc/main/jni -Isrc/main/jni/Skin/tests/stubs
//       src/main/jni/Hacks/Feature/tests/newfeatures_check.cpp -o /tmp/nf_check

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <chrono>
#include <atomic>
#include <random>
#include <string>
#include <vector>

extern "C" void MSHookFunction(void *, void *, void **) {}
extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }

#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/NewFeatures.h"

namespace Tools {
bool IsPtrValid(void *a) { return a != nullptr; }
}

static int g_failures = 0;

static void Check(bool ok, const char *what)
{
    printf("%s: %s\n", ok ? "OK  " : "FAIL", what);
    if (!ok) g_failures++;
}

int main()
{
    int ran = 0;
    Check(NewFeatures::A_FireGuardedRun("clean", 1, [&] { ran++; }),
          "a clean guarded call runs");
    Check(ran == 1, "the guarded body executed exactly once");

    Check(!NewFeatures::A_FireGuardedRun("fault", 2, [&] {
              (void)*(volatile int *)0;
              ran++;
          }),
          "a fault inside a guarded call is caught, not fatal");
    Check(ran == 1, "the faulting body stopped at the fault");

    int retried = 0;
    Check(!NewFeatures::A_FireGuardedRun("fault", 2, [&] { retried++; }),
          "the faulting key is skipped while it cools down");
    Check(retried == 0, "the cooling-down key does not re-enter the body");

    Check(NewFeatures::g_afireFaultCount.load() == 1, "the fault was counted");
    Check(NewFeatures::A_FireGuardedRun("clean", 3, [&] { ran++; }),
          "other keys are unaffected by the cooldown");

    Check(aim_lock::LockGuardedRun("lock clean", 10, [&] { ran++; }),
          "aim lock guard runs clean bodies");
    Check(!aim_lock::LockGuardedRun("lock fault", 11, [&] {
              (void)*(volatile int *)0;
          }),
          "aim lock guard catches faults");
    int lockRetried = 0;
    Check(!aim_lock::LockGuardedRun("lock fault", 11, [&] { lockRetried++; }) &&
              lockRetried == 0,
          "aim lock cooldown holds for the faulting key");

    Vector3 point;
    float dist = 0.0f;
    Check(!aim_lock::CandidateOk(nullptr, nullptr, Vector3::zero(), Vector3::zero(),
                                nullptr, point, dist),
          "aim lock rejects a null candidate without touching the game");

    if (g_failures == 0) printf("ALL CHECKS PASS\n");
    else printf("%d CHECKS FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
