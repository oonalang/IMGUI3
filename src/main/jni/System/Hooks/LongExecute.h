#pragma once

#include <cstdint>

#include "Hacks/Feature/Init_Hacks.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"

// Long Execute widens the finisher the game already allows, it does not skip it.
//
// Evidence, dump 1.6.57 (the build this mod's RVAs resolve against):
//
//   ExecutionComponent (TypeDefIndex 20307)
//     public int m_CacheExecuteId;          // 0x2C
//     public ExecuteParameters m_FixParamConf; // 0x30
//     public float FixBodyRadis;            // 0x5C
//     public int CheckTargetIsValid(Pawn tempPawn, out float distance); // RVA 0x5947460
//
//   ExecuteParameters (TypeDefIndex 20315), embedded at 0x30 and 0x2C bytes long,
//   which is why FixBodyRadis follows at 0x5C:
//     public float m_fixMinDistance;        // 0x00
//     public float m_fixMaxDistance;        // 0x04
//     public float m_fixHeightDiff;         // 0x08
//     public float m_fixViewRange;          // 0x0C
//
// CheckTargetIsValid is the per-candidate distance gate and it hands the measured
// distance back through the out parameter, so the component the game checks is the
// one this hook widens, right before the check runs. The value is raised, never
// lowered, and only when it is already positive, so a weapon without a finisher
// stays without one. RefreshConfigData (RVA 0x5946A40) refills the struct from
// ExecutionConfConfig, which puts the game's own range back once the toggle is off.

namespace long_execute
{
    namespace Offsets
    {
        constexpr uintptr_t kFixParamConf   = 0x30;
        constexpr uintptr_t kMinDistance    = kFixParamConf + 0x00;
        constexpr uintptr_t kMaxDistance    = kFixParamConf + 0x04;
        constexpr uintptr_t kHeightDiff     = kFixParamConf + 0x08;
        constexpr uintptr_t kViewRange      = kFixParamConf + 0x0C;
    }

    namespace RVA
    {
        constexpr uintptr_t CheckTargetIsValid = 0x5947460;
    }

    constexpr float kMinRange = 2.0f;
    constexpr float kMaxRange = 25.0f;

    inline int (*orig_CheckTargetIsValid)(void *self, void *pawn, float *outDistance) = nullptr;

    inline bool Enabled()
    {
        return Config.ExtraMenu.LongExecute;
    }

    inline float WantedRange()
    {
        float want = Config.ExtraMenu.LongExecuteRange;
        if (!(want > kMinRange)) want = kMinRange;
        if (want > kMaxRange) want = kMaxRange;
        return want;
    }

    inline void Widen(void *self)
    {
        if (!Tools::IsPtrValid(self)) return;

        const float want = WantedRange();
        auto *maxDistance = (float *)((uintptr_t)self + Offsets::kMaxDistance);
        auto *viewRange   = (float *)((uintptr_t)self + Offsets::kViewRange);

        if (*maxDistance > 0.0f && *maxDistance < want) *maxDistance = want;
        if (*viewRange > 0.0f && *viewRange < want) *viewRange = want;
    }

    inline int Hook_CheckTargetIsValid(void *self, void *pawn, float *outDistance)
    {
        if (Enabled()) Widen(self);
        return orig_CheckTargetIsValid != nullptr
             ? orig_CheckTargetIsValid(self, pawn, outDistance)
             : 0;
    }
}
