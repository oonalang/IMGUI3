#pragma once

#include <random>
#include <chrono>
#include <atomic>

#include <cstdint>
#include <cstring>

#include "../../IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "../../IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "../../IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "../../IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "../../foxcheats/include/ScanEngine.hpp"
#include "../../Includes/Utils.h"
#include "../../Includes/Macros.h"
#include "../StructGame/StructGame.h"

namespace NewFeatures
{
    namespace Offsets
    {
        constexpr uintptr_t WeaponFire_TargetPawn           = 0x668;
        constexpr uintptr_t WeaponFire_AimAssistDis         = 0x628;
        constexpr uintptr_t WeaponFire_TargetPawnDist       = 0x690;
        constexpr uintptr_t WeaponFire_AimmingBreatheCD     = 0x774;
        constexpr uintptr_t WeaponFire_BreatheTremblingPos  = 0x5A0;
        constexpr uintptr_t WeaponFire_Owner                = 0x28;
        constexpr uintptr_t WeaponFire_ClipAmmoCount        = 0xA8;

        constexpr uintptr_t Pawn_FirstPersonCamOffsetX      = 0x6FC;
        constexpr uintptr_t Pawn_FirstPersonCamDestX        = 0x700;

        constexpr uintptr_t WeaponShake_Owner               = 0x78;

        constexpr uintptr_t Weapon_m_IsAiming               = 0xC21;
        constexpr uintptr_t Weapon_m_AimingPercentage       = 0x460;
    }

    namespace RVA
    {
        constexpr uintptr_t CheckTarget                     = 0xC9C1B50;
        constexpr uintptr_t HasEnemyInPort                  = 0xC9C13F8;
        constexpr uintptr_t GetIsAiming                     = 0xC9AF4E0;

        constexpr uintptr_t AmmoCanFire                     = 0x50EB73C;

        constexpr uintptr_t Weapon_StartFire                = 0x5123C84;
        constexpr uintptr_t Weapon_StopFire                 = 0x5124EAC;

        constexpr uintptr_t Weapon_Tick                     = 0x5114C34;
        constexpr uintptr_t Pawn_Tick                       = 0x51C366C;

        constexpr uintptr_t PlayerPawn_Tick                 = 0x54816D8;
        constexpr uintptr_t MPPawn_Tick                     = 0xB5D0DC8;
        constexpr uintptr_t BRPlayerPawn_Tick               = 0x78B68FC;
        constexpr uintptr_t DMZPlayerPawn_Tick              = 0x7FF06EC;
        constexpr uintptr_t PVEPlayerPawn_Tick              = 0x78378AC;
        constexpr uintptr_t BRGroundWarPlayerPawn_Tick      = 0x6FB04A8;
        constexpr uintptr_t OpenWorldPlayerPawn_Tick        = 0x5A0E994;
        constexpr uintptr_t Pawn_GetCurrentWeapon           = 0x518BEA8;
        constexpr uintptr_t Pawn_IsAiming                   = 0x519BB10;
        constexpr uintptr_t Pawn_get_IsFiring               = 0x51A8A38;
        constexpr uintptr_t Gameplay_get_MatchGame          = 0x8569B10;
        constexpr uintptr_t Gameplay_get_LocalPawn          = 0x856AA08;
        constexpr uintptr_t BaseGame_GetEnemyPawns          = 0x9CAFE38;
        constexpr uintptr_t Pawn_get_HeadPosition           = 0x51B86EC;
        constexpr uintptr_t Pawn_get_LastPawnPos            = 0x51B8D30;
        constexpr uintptr_t Camera_get_main                 = 0x5024010;
        constexpr uintptr_t Camera_WorldToScreenPoint       = 0x5023B54;

        constexpr uintptr_t Camera_ScreenToWorldPoint       = 0x5023D34;
        constexpr uintptr_t Physics_RaycastLOS              = 0x506C1E4;

        constexpr uintptr_t Physics_RaycastHitOut            = 0x506C188;
        constexpr uintptr_t Physics_RaycastSegment           = 0x506BF60;

        constexpr uintptr_t Physics_RaycastAll               = 0x506CBB0;

        constexpr uintptr_t Collider_IsTrigger               = 0x50279D4;

        constexpr uintptr_t Transform_get_forward            = 0x509C650;
        constexpr uintptr_t Transform_get_parent             = 0x507BDEC;

        constexpr uintptr_t Transform_get_right              = 0x509C368;
        constexpr uintptr_t Transform_get_up                 = 0x509C4E4;

        constexpr uintptr_t Pawn_get_RecentlyRendered       = 0x51BA7E8;
        constexpr uintptr_t Pawn_get_LastOccVisible         = 0x51BA800;
        constexpr uintptr_t Screen_get_width                = 0x50504BC;
        constexpr uintptr_t Screen_get_height               = 0x5050520;

        constexpr uintptr_t Pawn_GetTeamId                  = 0x51B89CC;
        constexpr uintptr_t Pawn_IsSameCamp                 = 0x51EF174;
        constexpr uintptr_t Pawn_IsLocalPlayer              = 0x518872C;

        constexpr uintptr_t Weapon_get_CurrentFireComponent = 0x50EB1E4;
        constexpr uintptr_t Weapon_get_IsAutoFire           = 0x513D704;
        constexpr uintptr_t Weapon_get_IsBurstFire          = 0x50EBB28;
        constexpr uintptr_t Weapon_get_ClipAmmoCount        = 0x50EC614;
        constexpr uintptr_t Weapon_get_ShotSpreadSize       = 0x5109B68;
        constexpr uintptr_t WeaponFireComponent_get_BulletInitSpeed = 0xC14A158;

        constexpr uintptr_t Weapon_get_IsSingleShot          = 0x50EBA5C;
        constexpr uintptr_t Weapon_CanFire                   = 0x5126CC4;
        constexpr uintptr_t Weapon_InSwitchWeaponState       = 0x512724C;
        constexpr uintptr_t Weapon_HasSwitchingState         = 0x5124C70;
        constexpr uintptr_t Weapon_RealFireInterval          = 0x50EC250;
        constexpr uintptr_t Weapon_get_FireInterval          = 0x5105C74;
        constexpr uintptr_t Weapon_get_AimingTarget          = 0x510EAAC;
        constexpr uintptr_t Weapon_get_CurrentAimingTargetDis = 0x510EABC;
        constexpr uintptr_t Camera_get_fieldOfView           = 0x5021F7C;
        constexpr uintptr_t BaseGame_GetTeamPawns            = 0x9CAFEB4;
        constexpr uintptr_t BaseGame_get_IsPaused            = 0x9CAFDC4;
        constexpr uintptr_t Pawn_get_IsAlive                 = 0x51B7C40;
        constexpr uintptr_t Pawn_get_LastFireTime            = 0x51B843C;

        constexpr uintptr_t Pawn_get_Velocity               = 0x51B38CC;

        constexpr uintptr_t GamePlay_get_AttackableColliderMap = 0x856A020;
        constexpr uintptr_t AtkColliderMap_GetTargetForCollider = 0x773FA68;

        constexpr uintptr_t Weapon_get_DelaySprintFire      = 0x513CBC4;
        constexpr uintptr_t Weapon_get_UseGetReadyTime      = 0x50E9F60;
        constexpr uintptr_t Weapon_get_SpringFireGetReady   = 0x50E9F28;
        constexpr uintptr_t Weapon_get_GetReadyTime         = 0x514D2C8;
    }

    inline std::mt19937 rng(std::random_device{}());

    inline void* UnityRva(uintptr_t rva)
    {
        return (void *)getAbsoluteAddress("libunity.so", (DWORD)rva);
    }

    inline void* LocalPlayer()
    {
        return (void *)GamePlay::get_LocalPawn();
    }

    inline bool IsReadable(void *p)
    {
        return p != nullptr && Tools::IsPtrValid(p);
    }

    constexpr size_t AF_OBJ_CACHEDPTR = 0x10;

    inline bool A_FireObjectAlive(void *obj)
    {
        if (!Tools::IsPtrValid(obj)) return false;
        void *cached = *(void **)((uintptr_t)obj + AF_OBJ_CACHEDPTR);
        return Tools::IsPtrValid(cached);
    }

    inline bool A_FireClassNameContains(void *obj, const char *needle)
    {
        if (needle == nullptr || needle[0] == '\0') return false;
        if (!Tools::IsPtrValid(obj)) return false;
        void *klass = *(void **)((uintptr_t)obj);
        if (!Tools::IsPtrValid(klass)) return false;
        const char *name = *(const char **)((uintptr_t)klass + 0x10);
        if (!IsReadable((void *)name)) return false;

        const unsigned maxLen = 96;
        unsigned nameLen = 0;
        while (nameLen < maxLen && name[nameLen] != '\0') ++nameLen;
        if (nameLen == 0 || nameLen >= maxLen) return false;

        unsigned needleLen = 0;
        while (needle[needleLen] != '\0') ++needleLen;
        if (needleLen > nameLen) return false;

        for (unsigned i = 0; i + needleLen <= nameLen; ++i) {
            unsigned j = 0;
            while (j < needleLen && name[i + j] == needle[j]) ++j;
            if (j == needleLen) return true;
        }
        return false;
    }

    inline bool A_FireFireComponentIsInstant(void *component)
    {
        return A_FireClassNameContains(component, "Instant");
    }

    inline bool A_FireListObjectAlive(void *obj)
    {
        if (((uintptr_t)obj & (uintptr_t)0x7) != (uintptr_t)0) return false;
        return A_FireObjectAlive(obj);
    }

    inline bool IsPawnAlive(void *pawn)
    {
        if (!IsReadable(pawn)) return false;
        return *(bool *)((uintptr_t)pawn + Class_Pawn_m_IsAlive);
    }

    inline bool   (*fn_PawnIsAiming)(void *)                = nullptr;
    inline void  *(*fn_WeaponGetCurrentFireComponent)(void *) = nullptr;
    inline bool   (*fn_FireComponentGetIsAiming)(void *)     = nullptr;

    inline bool IsLocalFireComponent(void *fireComponent)
    {
        if (!IsReadable(fireComponent)) return false;
        void *owner = *(void **)((uintptr_t)fireComponent + Offsets::WeaponFire_Owner);
        return owner != nullptr && owner == LocalPlayer();
    }

    inline bool IsFireComponentScoping(void *fireComponent, bool *answered = nullptr)
    {
        if (answered != nullptr) *answered = false;
        if (!IsReadable(fireComponent)) return false;
        if (!A_FireFireComponentIsInstant(fireComponent)) return false;
        bool (*getIsAiming)(void *) = fn_FireComponentGetIsAiming;
        if (getIsAiming == nullptr) {
            static auto lazy = (bool (*)(void *))UnityRva(RVA::GetIsAiming);
            getIsAiming = lazy;
        }
        if (getIsAiming == nullptr) return false;
        if (answered != nullptr) *answered = true;
        return getIsAiming(fireComponent);
    }

    inline bool IsSameTeam(void *localPawn, void *targetPawn)
    {
        if (!IsReadable(localPawn) || !IsReadable(targetPawn)) return false;
        if (localPawn == targetPawn) return true;

        static auto isSameCamp = (bool (*)(void *, void *))UnityRva(RVA::Pawn_IsSameCamp);
        if (isSameCamp != nullptr && isSameCamp(localPawn, targetPawn)) return true;

        static auto getTeamId = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);
        if (getTeamId == nullptr) return false;
        const uint32_t localTeam  = getTeamId(localPawn);
        const uint32_t targetTeam = getTeamId(targetPawn);
        return localTeam != 0 && localTeam == targetTeam;
    }

    inline bool IsSameTeamAcceptBots(void *localPawn, void *targetPawn)
    {
        if (!A_FireObjectAlive(localPawn) || !A_FireObjectAlive(targetPawn)) return false;
        if (localPawn == targetPawn) return true;

        static auto getTeamId = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);
        if (getTeamId == nullptr) return false;
        const uint32_t localTeam  = getTeamId(localPawn);
        const uint32_t targetTeam = getTeamId(targetPawn);
        return localTeam != 0 && localTeam == targetTeam;
    }

    inline void (*orig_Weapon_Tick)(void *, float) = nullptr;
    inline void (*orig_Pawn_Tick)(void *, float) = nullptr;
    inline void (*orig_PlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_MPPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_BRPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_DMZPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_PVEPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_OpenWorldPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*orig_BRGroundWarPlayerPawn_Tick)(void *, float) = nullptr;
    inline void (*fn_Weapon_StartFire)(void *) = nullptr;
    inline void (*fn_Weapon_StopFire)(void *, bool) = nullptr;
    inline Vector3 (*fn_WorldToScreen)(void *, Vector3) = nullptr;

    inline Vector3 (*fn_ScreenToWorldPoint)(void *, Vector3) = nullptr;
    inline bool (*fn_PhysicsRaycastLOS)(Vector3, Vector3, float, void *) = nullptr;

    inline bool (*fn_PhysicsRaycastHitOut)(Vector3, Vector3, void *, float) = nullptr;

    inline bool (*fn_PhysicsRaycastSegment)(Vector3, Vector3, float) = nullptr;

    inline void *(*fn_PhysicsRaycastAll)(Vector3, Vector3, float) = nullptr;

    inline bool (*fn_ColliderIsTrigger)(void *) = nullptr;

    inline Vector3 (*fn_TransformGetForward)(void *) = nullptr;
    inline void   *(*fn_TransformGetParent)(void *) = nullptr;

    inline Vector3 (*fn_TransformGetRight)(void *) = nullptr;
    inline Vector3 (*fn_TransformGetUp)(void *) = nullptr;
    inline void *(*fn_CameraGetMain)() = nullptr;

    inline bool A_FireWorldToScreenSafe(void *cam, const Vector3 &world, Vector3 &out)
    {
        if (fn_WorldToScreen == nullptr || !A_FireObjectAlive(cam)) return false;
        out = fn_WorldToScreen(cam, world);
        return true;
    }
    inline void *(*fn_GameGetMatchGame)() = nullptr;
    inline bool (*fn_PawnRecentlyRendered)(void *) = nullptr;
    inline bool (*fn_PawnLastOccVisible)(void *) = nullptr;
    inline bool (*fn_PawnGetIsFiring)(void *) = nullptr;

    inline bool  (*fn_WeaponIsAutoFire)(void *) = nullptr;
    inline bool  (*fn_WeaponIsBurstFire)(void *) = nullptr;
    inline bool  (*fn_WeaponIsSingleShot)(void *) = nullptr;
    inline bool  (*fn_WeaponAmmoCanFire)(void *) = nullptr;
    inline int   (*fn_WeaponGetClipAmmoCount)(void *) = nullptr;
    inline bool  (*fn_WeaponCanFire)(void *) = nullptr;
    inline bool  (*fn_WeaponInSwitchWeaponState)(void *) = nullptr;
    inline bool  (*fn_WeaponHasSwitchingState)(void *) = nullptr;
    inline float (*fn_WeaponRealFireInterval)(void *) = nullptr;
    inline void *(*fn_WeaponGetAimingTarget)(void *) = nullptr;
    inline float (*fn_WeaponGetAimingTargetDis)(void *) = nullptr;
    inline float (*fn_CameraGetFov)(void *) = nullptr;
    inline void *(*fn_BaseGameGetTeamPawns)(void *) = nullptr;
    inline bool  (*fn_BaseGameIsPaused)(void *) = nullptr;
    inline bool  (*fn_PawnIsAlive)(void *) = nullptr;

    enum class FireMode : int
    {
        Single  = 0,
        Burst   = 1,
        Auto    = 2,
        Unknown = 3
    };

    enum class FireBlock : int
    {
        Fired = 0,
        FeatureOff,
        NoLocalPawn,
        LocalNotAlive,
        NoMatch,
        MatchPaused,
        NoWeapon,
        WeaponSwitching,
        NoAmmo,
        WeaponNotReady,
        FireInterval,
        ModeNotAds,
        ModeNotFiring,
        AdsTransition,
        NoEnemyInZone,
        NoLineOfSight,
        LeadNoLineOfSight,
        TravelTooLong,
        ShotWontLand,
        FriendlyInLine,
        Count
    };

    inline const char *A_FireBlockText(FireBlock b)
    {
        switch (b)
        {
            case FireBlock::Fired:             return "firing";
            case FireBlock::FeatureOff:        return "triggerbot off";
            case FireBlock::NoLocalPawn:       return "no local pawn";
            case FireBlock::LocalNotAlive:     return "you are dead";
            case FireBlock::NoMatch:           return "no match";
            case FireBlock::MatchPaused:       return "match paused";
            case FireBlock::NoWeapon:          return "no weapon";
            case FireBlock::WeaponSwitching:   return "switching weapon";
            case FireBlock::NoAmmo:            return "no ammo";
            case FireBlock::WeaponNotReady:    return "weapon not ready";
            case FireBlock::FireInterval:      return "fire interval";
            case FireBlock::ModeNotAds:        return "not scoped (mode)";
            case FireBlock::AdsTransition:     return "scope transition (held)";
            case FireBlock::ModeNotFiring:     return "not firing (mode)";
            case FireBlock::NoEnemyInZone:     return "no enemy in crosshair";
            case FireBlock::NoLineOfSight:     return "wall in the way";
            case FireBlock::LeadNoLineOfSight: return "lead path blocked";
            case FireBlock::TravelTooLong:     return "bullet travel too long";
            case FireBlock::ShotWontLand:      return "spread/lead would miss";
            case FireBlock::FriendlyInLine:    return "teammate in the line";
            default:                           return "?";
        }
    }

    struct A_FireTune
    {
        static constexpr float kRayReach        = 1.00f;
        static constexpr float kSurfaceFrac     = 0.80f;
        static constexpr float kMinStemGap      = 0.05f;
        static constexpr float kHeadRadius      = 0.14f;
        static constexpr float kBodyRadius      = 0.30f;

        static constexpr float kBodyHalfWidthM  = 0.32f;
        static constexpr float kFriendlyRadius  = 0.35f;
        static constexpr float kSelfExclusion    = 0.15f;

        static constexpr float kCrosshairRayMax = 400.0f;
        static constexpr float kMaxTravelTime   = 1.20f;
        static constexpr float kAssumedSpeed    = 800.0f;
        static constexpr float kMinFireInterval = 0.030f;

        static constexpr float kRepressFraction  = 0.90f;
        static constexpr float kMaxRepressPace   = 0.250f;

        static constexpr float kReassertPress    = 0.250f;
        static constexpr float kFallbackFov     = 60.0f;
        static constexpr float kDegToRad        = 0.01745329f;
        static constexpr float kRadToDeg        = 57.29578f;
    };

    struct A_FireAimPoint
    {
        Vector3 pos;
        Vector3 head;
        Vector3 feet;
        float   radius;
        bool    valid;
        bool    headOk;
        bool    feetOk;
    };

    struct A_FireCandidate
    {
        Pawn          *pawn;
        A_FireAimPoint aim;
        float          pxOffset;
        float          worldDist;
        Vector3        velocity;
    };

    inline void *GetCurrentFireComponent(void *weapon)
    {
        if (!A_FireObjectAlive(weapon)) return nullptr;
        void *(*fn)(void *) = fn_WeaponGetCurrentFireComponent;
        if (fn == nullptr) {
            static auto lazy = (void *(*)(void *))UnityRva(RVA::Weapon_get_CurrentFireComponent);
            fn = lazy;
        }
        if (fn == nullptr) return nullptr;
        void *comp = fn(weapon);
        return A_FireObjectAlive(comp) ? comp : nullptr;
    }

    inline float GetWeaponBulletSpeed(void *weapon, bool *known = nullptr)
    {
        if (known != nullptr) *known = false;
        void *fireComp = GetCurrentFireComponent(weapon);
        if (Tools::IsPtrValid(fireComp)) {
            static auto getSpeed = (float (*)(void *))UnityRva(RVA::WeaponFireComponent_get_BulletInitSpeed);
            if (getSpeed != nullptr) {
                const float speed = getSpeed(fireComp);
                if (speed >= 1.0f && speed <= 5000.0f) {
                    if (known != nullptr) *known = true;
                    return speed;
                }
            }
        }

        return A_FireTune::kAssumedSpeed;
    }

    inline FireMode GetWeaponFireMode(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return FireMode::Unknown;
        if (fn_WeaponIsAutoFire != nullptr && fn_WeaponIsAutoFire(weapon)) return FireMode::Auto;
        if (fn_WeaponIsBurstFire != nullptr && fn_WeaponIsBurstFire(weapon)) return FireMode::Burst;
        if (fn_WeaponIsSingleShot != nullptr && fn_WeaponIsSingleShot(weapon)) return FireMode::Single;

        return FireMode::Single;
    }

    inline Vector3 GetPawnVelocity(void *pawn)
    {
        if (!A_FireObjectAlive(pawn)) return Vector3::zero();
        static auto getVelocity = (Vector3 (*)(void *))UnityRva(RVA::Pawn_get_Velocity);
        if (getVelocity != nullptr) return getVelocity(pawn);
        return Vector3::zero();
    }

    inline float A_FireFocalPx()
    {
        float fov = A_FireTune::kFallbackFov;
        if (fn_CameraGetFov != nullptr && fn_CameraGetMain != nullptr) {
            void *cam = fn_CameraGetMain();
            if (Tools::IsPtrValid(cam)) {
                const float v = fn_CameraGetFov(cam);
                if (v > 1.0f && v < 179.0f) fov = v;
            }
        }
        const float tanHalf = tanf(fov * 0.5f * A_FireTune::kDegToRad);
        if (!(tanHalf > 0.0001f)) return 0.0f;
        return ((float)get_height() * 0.5f) / tanHalf;
    }

    inline Vector3 A_FireAlongBody(const Vector3 &head, const Vector3 &feet, float t)
    {
        return Vector3(feet.x + (head.x - feet.x) * t,
                       feet.y + (head.y - feet.y) * t,
                       feet.z + (head.z - feet.z) * t);
    }

    inline float A_FirePointSegDistSq(float px, float py,
                                      float ax, float ay, float bx, float by)
    {
        const float vx = bx - ax, vy = by - ay;
        const float wx = px - ax, wy = py - ay;
        const float vv = vx * vx + vy * vy;
        float t = 0.0f;
        if (vv > 0.0001f) {
            t = (wx * vx + wy * vy) / vv;
            if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
        }
        const float dx = px - (ax + vx * t);
        const float dy = py - (ay + vy * t);
        return dx * dx + dy * dy;
    }

    inline A_FireAimPoint A_FireAimPointFor(Pawn *pawn)
    {
        A_FireAimPoint ap{};
        ap.valid = false;
        ap.radius = A_FireTune::kBodyRadius;
        if (!A_FireObjectAlive((void *)pawn)) return ap;

        const Vector3 head = pawn->get_HeadPosition();
        ap.head = head;
        ap.headOk = (head.x != 0.0f || head.y != 0.0f || head.z != 0.0f);

        Transform *root = pawn->get_transform();
        const bool rootOk = A_FireObjectAlive((void *)root);
        const Vector3 feet = rootOk ? root->get_position() : Vector3::zero();
        ap.feet = feet;
        ap.feetOk = rootOk;

        if (ap.headOk && ap.feetOk) {
            ap.pos = A_FireAlongBody(head, feet, 0.72f);
            ap.radius = A_FireTune::kBodyRadius;
        } else if (ap.headOk) {
            ap.pos = head;
            ap.radius = A_FireTune::kHeadRadius;
        } else if (ap.feetOk) {
            ap.pos = Vector3(feet.x, feet.y + 1.05f, feet.z);
            ap.radius = A_FireTune::kBodyRadius;
        } else {
            return ap;
        }
        ap.valid = true;
        return ap;
    }

    inline bool A_FireWeaponSwitching(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponInSwitchWeaponState != nullptr && fn_WeaponInSwitchWeaponState(weapon)) return true;
        if (fn_WeaponHasSwitchingState  != nullptr && fn_WeaponHasSwitchingState(weapon))  return true;
        return false;
    }

    inline bool A_FireWeaponHasAmmo(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponAmmoCanFire != nullptr && !fn_WeaponAmmoCanFire(weapon)) return false;
        if (fn_WeaponGetClipAmmoCount != nullptr && fn_WeaponGetClipAmmoCount(weapon) <= 0) return false;
        return true;
    }

    inline bool A_FireWeaponCanFire(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return false;
        if (fn_WeaponCanFire == nullptr) return true;
        return fn_WeaponCanFire(weapon);
    }

    struct AdsRead
    {
        bool  aiming;
        bool  unknown;
        bool  pawnOk;
        bool  pawnAim;
        bool  fcOk;
        bool  fcAim;
        bool  fieldFlag;
        float fieldPct;
    };

    inline bool A_FirePawnCallable(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        return ((Pawn *)pawn)->m_IsAlive();
    }

    inline double A_FireNow()
    {
        return std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    constexpr double kAdsSettleSeconds    = 0.10;
    constexpr double kAdsSettleMaxSeconds = 0.35;
    inline double    g_afireAdsSettleUntil = -1000.0;
    inline double    g_afireAdsSettleMax   = -1000.0;
    inline int       g_afireAdsLastState   = -1;
    inline void     *g_afireAdsLastComp     = nullptr;
    inline bool      g_afireAdsCompSeen     = false;
    inline bool      g_afireAdsSettling     = false;

    struct ScopeSettle
    {
        bool settling;
        bool asked;
        bool aiming;
    };

    inline bool A_FireAdsSwapActive()
    {
        return g_afireAdsSettling && A_FireNow() < g_afireAdsSettleMax;
    }

    inline ScopeSettle A_FireScopingTransition(void *weapon, Pawn *localPawn)
    {
        ScopeSettle s{false, false, false};
        const double now = A_FireNow();
        bool moved = false;

        if (fn_PawnIsAiming != nullptr && A_FirePawnCallable(localPawn)) {
            s.asked  = true;
            s.aiming = fn_PawnIsAiming((void *)localPawn);
            const int state = s.aiming ? 1 : 0;
            if (g_afireAdsLastState != -1 && state != g_afireAdsLastState) moved = true;
            g_afireAdsLastState = state;
        } else {
            g_afireAdsLastState = -1;
        }

        void *comp = GetCurrentFireComponent(weapon);
        if (!g_afireAdsCompSeen) {
            g_afireAdsCompSeen = true;
            g_afireAdsLastComp = comp;
        } else if (comp != g_afireAdsLastComp) {
            moved = true;
            g_afireAdsLastComp = comp;
        }

        if (moved) {
            g_afireAdsSettleUntil = now + kAdsSettleSeconds;
            g_afireAdsSettleMax   = now + kAdsSettleMaxSeconds;
        }

        if (now >= g_afireAdsSettleMax) {
            s.settling = false;
            g_afireAdsSettling = false;
            return s;
        }
        if (now < g_afireAdsSettleUntil) {
            s.settling = true;
            g_afireAdsSettling = true;
            return s;
        }

        const bool compStable = (comp != nullptr && !moved);
        if (!compStable || !A_FireWeaponCanFire(weapon)) {
            s.settling = true;
            g_afireAdsSettling = true;
            return s;
        }
        s.settling = false;
        g_afireAdsSettling = false;
        return s;
    }

    inline AdsRead A_FireAdsGate(void *weapon, Pawn *localPawn, const ScopeSettle &settle)
    {
        AdsRead r{};
        r.aiming = r.unknown = r.pawnOk = r.pawnAim = r.fcOk = r.fcAim = false;
        r.fieldFlag = false;
        r.fieldPct  = 0.0f;

        if (settle.asked) {
            r.pawnOk  = true;
            r.pawnAim = settle.aiming;
        } else if (fn_PawnIsAiming != nullptr && A_FirePawnCallable(localPawn)) {
            r.pawnOk  = true;
            r.pawnAim = fn_PawnIsAiming((void *)localPawn);
        }

        void *fireComp = GetCurrentFireComponent(weapon);
        if (fireComp != nullptr)
            r.fcAim = IsFireComponentScoping(fireComp, &r.fcOk);

        if (Tools::IsPtrValid(weapon)) {
            r.fieldFlag = *(bool *)((uintptr_t)weapon + Offsets::Weapon_m_IsAiming);
            r.fieldPct  = *(float *)((uintptr_t)weapon + Offsets::Weapon_m_AimingPercentage);
        }

        if (r.pawnAim || r.fcAim) { r.aiming = true;  return r; }
        if (r.pawnOk  || r.fcOk)  { r.aiming = false; return r; }
        r.unknown = true;
        r.aiming  = true;
        return r;
    }

    inline float A_FireWeaponInterval(void *weapon)
    {
        float interval = 0.0f;
        if (fn_WeaponRealFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
            const float v = fn_WeaponRealFireInterval(weapon);
            if (v > 0.0f && v < 10.0f) interval = v;
        }
        if (interval <= 0.0f) {
            static auto getFireInterval = (float (*)(void *))UnityRva(RVA::Weapon_get_FireInterval);
            if (getFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
                const float v = getFireInterval(weapon);
                if (v > 0.0f && v < 10.0f) interval = v;
            }
        }
        if (!(interval > 0.0f)) interval = A_FireTune::kMinFireInterval;
        if (interval < A_FireTune::kMinFireInterval) interval = A_FireTune::kMinFireInterval;
        return interval;
    }

    inline float A_FireRepressPaceSeconds(float interval)
    {
        float pace = interval * A_FireTune::kRepressFraction;
        if (!(pace > 0.0f) || pace < A_FireTune::kMinFireInterval)
            pace = A_FireTune::kMinFireInterval;
        if (pace > A_FireTune::kMaxRepressPace) pace = A_FireTune::kMaxRepressPace;
        return pace;
    }

    inline bool A_FirePawnAlive(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        if (fn_PawnIsAlive != nullptr && fn_PawnIsAlive(pawn)) return true;
        return ((Pawn *)pawn)->m_IsAlive();
    }

    inline bool A_FireShotWouldLand(float missPx, float zonePx, float hitboxPx, float spreadPx)
    {
        if (!(missPx >= 0.0f)) return true;
        float allowed = hitboxPx > zonePx ? hitboxPx : zonePx;
        if (allowed < 1.0f) allowed = 1.0f;
        if (spreadPx > 0.0f && spreadPx < 40000.0f) allowed += spreadPx;
        return missPx <= allowed;
    }

    inline float A_FireSpreadPx(void *weapon, float focalPx, bool checkSpread)
    {
        if (!checkSpread) return 0.0f;
        if (!A_FireObjectAlive(weapon) || !(focalPx > 0.0f)) return 0.0f;
        static auto getSpread = (float (*)(void *))UnityRva(RVA::Weapon_get_ShotSpreadSize);
        if (getSpread == nullptr) return 0.0f;
        const float spread = getSpread(weapon);
        if (!(spread >= 0.0f) || spread > 45.0f) return 0.0f;
        return tanf(spread * A_FireTune::kDegToRad) * focalPx;
    }

    struct AF_FriendProbe { Vector3 feet; Vector3 chest; };

    inline int A_FireCollectFriendProbes(Pawn *localPawn, AF_FriendProbe *out, int maxOut)
    {
        if (out == nullptr || maxOut <= 0) return 0;
        if (fn_BaseGameGetTeamPawns == nullptr || fn_GameGetMatchGame == nullptr) return 0;

        void *game = fn_GameGetMatchGame();
        if (!Tools::IsPtrValid(game)) return 0;
        List<Pawn *> *team = (List<Pawn *> *)fn_BaseGameGetTeamPawns(game);
        if (!Tools::IsPtrValid((void *)team)) return 0;
        Pawn **items = (Pawn **)team->getItems();
        const int count = team->getSize();
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 128) return 0;

        int n = 0;
        for (int i = 0; i < count && n < maxOut; i++) {
            Pawn *mate = items[i];
            if (!A_FireListObjectAlive((void *)mate)) continue;
            if (mate == localPawn) continue;
            if (!A_FirePawnAlive(mate)) continue;

            Transform *t = mate->get_transform();
            if (!A_FireObjectAlive((void *)t)) continue;
            const Vector3 pos = t->get_position();
            out[n].feet = pos;

            out[n].chest = Vector3(pos.x, pos.y + 1.15f, pos.z);
            n++;
        }
        return n;
    }

    inline bool A_FireFriendlyInLine(const AF_FriendProbe *probes, int probeCount,
                                     const Vector3 &camPos, const Vector3 &dir,
                                     float targetDist, int *friendsSeen = nullptr)
    {
        if (friendsSeen != nullptr) *friendsSeen = 0;
        if (probes == nullptr || probeCount <= 0) return false;

        bool blocked = false;
        for (int i = 0; i < probeCount; i++) {
            const Vector3 probe[2] = { probes[i].feet, probes[i].chest };
            bool mateInLine = false;
            for (int k = 0; k < 2; k++) {
                const Vector3 to(probe[k].x - camPos.x, probe[k].y - camPos.y, probe[k].z - camPos.z);

                const float dist = Vector3::Magnitude(to);
                if (dist < 0.2f || dist >= targetDist) continue;

                const float along = Vector3::Dot(to, dir);
                const Vector3 lat(to.x - dir.x * along, to.y - dir.y * along, to.z - dir.z * along);
                const float lateral = Vector3::Magnitude(lat);

                if (lateral <= A_FireTune::kFriendlyRadius) { mateInLine = true; break; }
            }
            if (mateInLine) {
                if (friendsSeen != nullptr) (*friendsSeen)++;
                blocked = true;
            }
        }
        return blocked;
    }

    inline bool     g_afireHoldingFire       = false;
    inline void    *g_afireFiringOn          = nullptr;
    inline double   g_afireLastPressTime     = -1000.0;

    inline void    *g_afireLastPressWeapon   = nullptr;

    inline bool     g_afireCanFireLast       = false;
    inline double   g_afireLastEvalTime      = -1000.0;
    inline void    *g_afireTargetPawn       = nullptr;

    inline bool     g_afireInTick            = false;
    inline std::atomic<bool> g_afireLosOk             = false;
    inline bool     g_testFireRequest        = false;
    inline double   g_testFireRequestTime    = -1000.0;

    struct TriggerbotDiag
    {
        int    weaponTicks;
        int    weaponHeldTicks;
        int    pawnTicks;
        int    pawnLocalTicks;
        int    hookCalls;
        int    scans;
        int    enemyCount;
        int    targets;
        int    fires;
        int    losBlocked;
        int    losBlockedScan;
        float  radiusPx;
        float  closestPx;
        double lastTickTime;
        void  *localPawn;
        void  *heldWeapon;
        void  *matchGame;
        int    testFireStatus;

        int    blockCount[(int)FireBlock::Count];
        int    lastBlock;
        int    candidates;
        int    losChecked;
        int    friendsInLine;
        bool   ammoOk;
        bool   canFire;
        float  focalPx;
        float  lastAimPx;
        float  lastPredPx;
        float  lastHitboxPx;
        float  lastSpreadPx;
        float  lastTravel;
        float  lastWorldDist;
        float  lastBulletSpeed;

        int    aimPath;
        unsigned crosshairPawn;
        float  crosshairHitDist;

        float  crosshairCx;
        float  crosshairCy;
        int    crosshairRayViaGame;
        int    crosshairRayFellBack;
        int    crosshairRayPlain;
        bool   dotRayHonest;
        bool   dotRayDecided;
        int    dotRayDeclined;
        float  projKeepFrac;
        int    raysIdentified;
        int    raysBlocked;
        bool   rayUnknown;

        int    selfHitsSkipped;
        int    triggerHitsSkipped;
        int    occOverrides;

        float  lastLosHitDist;
        bool   switching;
        bool   leadPathBlocked;
        bool   spreadWarn;
        float  lastFireInterval;
        unsigned aimingTarget;
        float  aimingTargetDis;

        int    adsFlag;
        float  adsPct;
        bool   adsUnknown;
        bool   adsPawnOk;
        bool   adsPawnAim;
        bool   adsFcOk;
        bool   adsFcAim;

        float  paceSeconds;
        int    repressSkipped;
        bool   paceWaiting;

        int    reasserts;
    };
    inline TriggerbotDiag g_triggerDiag{};

    inline void *(*fn_GameGetAttackableColliderMap)() = nullptr;
    inline void *(*fn_AtkMapGetTargetForCollider)(void *, void *) = nullptr;

    inline std::atomic<int> s_dbgTriggerState = 0;

    inline Pawn  *g_afireLocalPawnCache     = nullptr;
    inline double g_afireLocalPawnCacheTime = -1000.0;
    inline void  *g_afireHeldWeaponCache    = nullptr;
    inline double g_afireHeldWeaponStamp    = -1000.0;
    inline double g_afireOffDiagStamp       = -1000.0;

    inline Pawn *A_FireLocalPawnFast()
    {
        const double now = A_FireNow();
        if (g_afireLocalPawnCache != nullptr &&
            (now - g_afireLocalPawnCacheTime) < 0.05)
            return g_afireLocalPawnCache;
        Pawn *p = GamePlay::get_LocalPawn();
        g_afireLocalPawnCache = p;
        g_afireLocalPawnCacheTime = now;
        return p;
    }

    inline void CrosshairCentrePx(float &cx, float &cy)
    {
        cx = (float)get_width()  * 0.5f + Config.Crosshair.OffsetX;
        cy = (float)get_height() * 0.5f + Config.Crosshair.OffsetY;
    }

    inline void CrosshairRayDir(void *cam, void *camTransform, const Vector3 &camPos,
                                Vector3 &out, bool &viaGame, bool &fellBack,
                                bool &plainCentre)
    {
        viaGame = false;
        fellBack = false;
        plainCentre = false;
        out = fn_TransformGetForward(camTransform);

        float cx = 0.0f, cy = 0.0f;
        CrosshairCentrePx(cx, cy);

        if (fn_ScreenToWorldPoint != nullptr && A_FireObjectAlive(cam)) {
            const Vector3 world =
                fn_ScreenToWorldPoint(cam, Vector3(cx, cy, A_FireTune::kCrosshairRayMax));
            const Vector3 dir(world.x - camPos.x,
                              world.y - camPos.y,
                              world.z - camPos.z);
            if (Vector3::Magnitude(dir) > 1.0e-4f) {
                out = Vector3::Normalized(dir);
                viaGame = true;
                return;
            }
        }

        const float dxPx = Config.Crosshair.OffsetX;
        const float dyPx = Config.Crosshair.OffsetY;
        if (dxPx == 0.0f && dyPx == 0.0f) {
            plainCentre = true;
            return;
        }

        if (fn_TransformGetRight != nullptr && fn_TransformGetUp != nullptr) {
            const float focalPx = A_FireFocalPx();
            if (focalPx > 1.0f) {
                const Vector3 right = fn_TransformGetRight(camTransform);
                const Vector3 up    = fn_TransformGetUp(camTransform);
                const Vector3 dir = out + right * (dxPx / focalPx) - up * (dyPx / focalPx);
                if (Vector3::Magnitude(dir) > 1.0e-6f) {
                    out = Vector3::Normalized(dir);
                    fellBack = true;
                    return;
                }
            }
        }
    }

    inline float A_FireCrosshairRadiusPx()
    {
        float radius = Config.Crosshair.Size * 0.5f;
        if (!(radius > 0.0f)) radius = 3.0f;
        const float w = (float)get_width();
        if (w > 0.0f && radius > w * 0.5f) radius = w * 0.5f;
        return radius;
    }

    inline bool A_FireIsLiveLocalPawn(void *localPawn)
    {
        return A_FireObjectAlive(localPawn) &&
               ((Pawn *)localPawn)->m_IsAlive();
    }

    inline void A_FireReportBlock(FireBlock b)
    {
        TriggerbotDiag &d = g_triggerDiag;
        const int idx = (int)b;
        d.lastBlock = idx;
        if (idx >= 0 && idx < (int)FireBlock::Count) d.blockCount[idx]++;
    }

    struct TriggerTarget
    {
        Pawn          *pawn = nullptr;
        A_FireAimPoint aim;
        Vector3        aimPoint;
        Vector3        screenPos;
        float          pxOffset = -1.0f;
        float          worldDist = 0.0f;
        float          travelTime = 0.0f;
        Vector3        predicted;
        float          predPxOffset = -1.0f;
        float          hitboxPx = 0.0f;
        float          spreadPx = 0.0f;
        float          focalPx = 0.0f;
        float          bulletSpeed = 0.0f;
        bool           speedKnown = false;
        bool           leadUsed = false;
        int            path = 1;
        bool           visible = false;
        bool           visibilityIdentified = false;
    };

    constexpr size_t AF_RAYHIT_SIZE      = 0x30;
    constexpr size_t AF_RAYHIT_DISTOFF   = 0x1C;
    constexpr size_t AF_RAYHIT_COLLOFF   = 0x28;

    enum class AF_Ray : int
    {
        Clear   = 0,
        Blocked = 1,
        Unknown = 2
    };

    inline void *A_FireHitCollider(const unsigned char *hitBuf)
    {
        if (hitBuf == nullptr) return nullptr;
        void *collider = *(void *const *)(hitBuf + AF_RAYHIT_COLLOFF);
        return Tools::IsPtrValid(collider) ? collider : nullptr;
    }

    inline void *A_FireColliderTransform(void *collider)
    {
        if (!A_FireObjectAlive(collider)) return nullptr;
        return (void *)((Component *)collider)->get_transform();
    }

    inline bool A_FireHitIsTarget(void *hitTransform, Pawn *pawn, int *depthOut = nullptr)
    {
        if (depthOut != nullptr) *depthOut = -1;
        if (!A_FireObjectAlive(hitTransform) || !A_FireObjectAlive((void *)pawn)) return false;

        if (fn_AtkMapGetTargetForCollider != nullptr) {
            void *map = fn_GameGetAttackableColliderMap ? fn_GameGetAttackableColliderMap() : nullptr;
            if (A_FireObjectAlive(map)) {
                void *target = fn_AtkMapGetTargetForCollider(map, hitTransform);
                if (A_FireObjectAlive(target) && target != (void *)pawn)
                    return false;
            }
        }

        Transform *root = pawn->get_transform();
        if (!Tools::IsPtrValid((void *)root)) return false;

        void *t = hitTransform;
        for (int depth = 0; depth < 24 && A_FireObjectAlive(t); depth++) {
            if (t == (void *)root) {
                if (depthOut != nullptr) *depthOut = depth;
                return true;
            }
            if (fn_TransformGetParent == nullptr) break;
            void *parent = fn_TransformGetParent(t);
            if (!Tools::IsPtrValid(parent) || parent == t) break;
            t = parent;
        }

        const Vector3 head = pawn->get_HeadPosition();
        if (head.x != 0.0f || head.y != 0.0f || head.z != 0.0f) {
            const Vector3 hp = ((Transform *)hitTransform)->get_position();
            const float dx = hp.x - head.x, dy = hp.y - head.y, dz = hp.z - head.z;
            const float r = 0.60f;
            if (dx * dx + dy * dy + dz * dz <= r * r) return true;
        }
        return false;
    }

    inline bool A_FireGameSaysVisible(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        if (fn_PawnLastOccVisible != nullptr && fn_PawnLastOccVisible(pawn)) return true;
        if (fn_PawnRecentlyRendered != nullptr && fn_PawnRecentlyRendered(pawn)) return true;
        return false;
    }

    constexpr size_t AF_ARRAY_LEN_OFF  = 0x18;
    constexpr size_t AF_ARRAY_DATA_OFF = 0x20;
    constexpr int    AF_MAX_RAY_HITS   = 64;

    struct AF_NamedHit
    {
        void *transform = nullptr;
        float distance  = -1.0f;
        bool  isTarget  = false;
    };

    inline bool A_FireRaycastAllNearest(void *hitArray, float maxDist,
                                        Pawn *target, Pawn *self,
                                        AF_NamedHit &out,
                                        int *selfSkipped = nullptr,
                                        int *triggerSkipped = nullptr)
    {
        out = AF_NamedHit{};
        if (!Tools::IsPtrValid(hitArray)) return false;

        const size_t len = *(const size_t *)((const unsigned char *)hitArray + AF_ARRAY_LEN_OFF);
        if (len > (size_t)AF_MAX_RAY_HITS) return false;

        const unsigned char *data = (const unsigned char *)hitArray + AF_ARRAY_DATA_OFF;
        bool taken[AF_MAX_RAY_HITS] = {};
        const float distCeiling = maxDist + A_FireTune::kRayReach + 0.01f;

        for (size_t n = 0; n < len; n++) {
            int best = -1;
            float bestDist = 0.0f;
            for (size_t i = 0; i < len; i++) {
                if (taken[i]) continue;
                const float hd = *(const float *)(data + i * AF_RAYHIT_SIZE + AF_RAYHIT_DISTOFF);
                if (!(hd >= 0.0f) || hd > distCeiling) {
                    return false;
                }
                if (best < 0 || hd < bestDist) { best = (int)i; bestDist = hd; }
            }
            if (best < 0) break;

            taken[best] = true;
            const unsigned char *elem = data + (size_t)best * AF_RAYHIT_SIZE;
            void *collider = A_FireHitCollider(elem);
            void *hitT     = A_FireColliderTransform(collider);

            if (hitT != nullptr && target != nullptr && A_FireHitIsTarget(hitT, target)) {
                out.transform = hitT;
                out.distance  = bestDist;
                out.isTarget  = true;
                return true;
            }
            if (hitT != nullptr && self != nullptr && A_FireHitIsTarget(hitT, self)) {
                if (selfSkipped != nullptr) (*selfSkipped)++;
                out = AF_NamedHit{};
                continue;
            }
            if (collider != nullptr && fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider)) {
                if (triggerSkipped != nullptr) (*triggerSkipped)++;
                out = AF_NamedHit{};
                continue;
            }

            out.transform = hitT;
            out.distance  = bestDist;
            return true;
        }
        out = AF_NamedHit{};
        return true;
    }

    inline AF_Ray A_FireRayToTarget(const Vector3 &from, const Vector3 &to, Pawn *target,
                                    float probeRadius, Pawn *localPawn,
                                    bool *identified = nullptr, float *hitDistance = nullptr)
    {
        if (identified != nullptr) *identified = false;
        if (hitDistance != nullptr) *hitDistance = -1.0f;

        Vector3 d(to.x - from.x, to.y - from.y, to.z - from.z);
        const float dist = Vector3::Magnitude(d);
        if (!(dist >= 0.01f)) return AF_Ray::Clear;
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);

        if (fn_PhysicsRaycastAll != nullptr) {
            const float reach = dist + A_FireTune::kRayReach;
            void *arr = fn_PhysicsRaycastAll(from, dir, reach);
            AF_NamedHit h;
            if (A_FireRaycastAllNearest(arr, reach, target, localPawn, h,
                                        &g_triggerDiag.selfHitsSkipped,
                                        &g_triggerDiag.triggerHitsSkipped)) {
                if (h.distance < 0.0f) return AF_Ray::Clear;
                if (hitDistance != nullptr) *hitDistance = h.distance;
                if (h.isTarget) {
                    if (identified != nullptr) *identified = true;
                    return AF_Ray::Clear;
                }
                if (h.transform != nullptr) {
                    if (h.distance <= A_FireTune::kSelfExclusion) {
                        if (A_FireGameSaysVisible(target)) {
                            g_triggerDiag.occOverrides++;
                            return AF_Ray::Clear;
                        }
                    }
                    return AF_Ray::Blocked;
                }
            }
        }

        if (fn_PhysicsRaycastHitOut != nullptr) {
            unsigned char hitBuf[AF_RAYHIT_SIZE];
            memset(hitBuf, 0, sizeof hitBuf);
            const bool got = fn_PhysicsRaycastHitOut(from, dir, hitBuf, dist + A_FireTune::kRayReach);
            if (!got) return AF_Ray::Clear;

            const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
            if (hitDistance != nullptr && hd >= 0.0f && hd <= 100000.0f) *hitDistance = hd;

            void *collider   = A_FireHitCollider(hitBuf);
            void *hitTransform = A_FireColliderTransform(collider);
            if (Tools::IsPtrValid(hitTransform)) {
                if (A_FireHitIsTarget(hitTransform, target)) {
                    if (identified != nullptr) *identified = true;
                    return AF_Ray::Clear;
                }
                if (localPawn != nullptr && A_FireHitIsTarget(hitTransform, localPawn)) {
                    g_triggerDiag.selfHitsSkipped++;
                    if (A_FireGameSaysVisible(target)) {
                        g_triggerDiag.occOverrides++;
                        return AF_Ray::Clear;
                    }
                    return AF_Ray::Blocked;
                }
                if (collider != nullptr && fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider)) {
                    g_triggerDiag.triggerHitsSkipped++;
                    if (A_FireGameSaysVisible(target)) {
                        g_triggerDiag.occOverrides++;
                        return AF_Ray::Clear;
                    }
                    return AF_Ray::Blocked;
                }
                if (hd >= 0.0f && hd <= A_FireTune::kSelfExclusion &&
                    A_FireGameSaysVisible(target)) {
                    g_triggerDiag.occOverrides++;
                    return AF_Ray::Clear;
                }
                return AF_Ray::Blocked;
            }
        }

        if (fn_PhysicsRaycastSegment != nullptr) {
            float stem = probeRadius * A_FireTune::kSurfaceFrac;
            if (stem < A_FireTune::kMinStemGap) stem = A_FireTune::kMinStemGap;
            const float stop = dist - stem;
            if (!(stop > 0.01f)) return AF_Ray::Clear;
            return fn_PhysicsRaycastSegment(from, dir, stop) ? AF_Ray::Blocked : AF_Ray::Clear;
        }

        return AF_Ray::Unknown;
    }

    inline AF_Ray A_FireRaySegmentClear(const Vector3 &from, const Vector3 &to)
    {
        Vector3 d(to.x - from.x, to.y - from.y, to.z - from.z);
        const float dist = Vector3::Magnitude(d);
        if (!(dist >= 0.01f)) return AF_Ray::Clear;
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);
        if (fn_PhysicsRaycastSegment != nullptr)
            return fn_PhysicsRaycastSegment(from, dir, dist) ? AF_Ray::Blocked : AF_Ray::Clear;
        return AF_Ray::Unknown;
    }

    inline int A_FireRegistryOwnerIs(void *hitTransform, Pawn *pawn)
    {
        if (fn_AtkMapGetTargetForCollider == nullptr) return 0;
        void *map = fn_GameGetAttackableColliderMap ? fn_GameGetAttackableColliderMap() : nullptr;
        if (!A_FireObjectAlive(map)) return 0;
        void *target = fn_AtkMapGetTargetForCollider(map, hitTransform);
        if (!A_FireObjectAlive(target)) return 0;
        return target == (void *)pawn ? 1 : -1;
    }

    inline Pawn *A_FirePawnFromHitTransform(void *hitTransform, Pawn **items, int count,
                                            int *depthOut = nullptr)
    {
        if (depthOut != nullptr) *depthOut = -1;
        if (!A_FireObjectAlive(hitTransform) || !Tools::IsPtrValid((void *)items)) return nullptr;

        constexpr int kMaxCached = 128;
        const int ncache = count < kMaxCached ? count : kMaxCached;
        void *tcache[kMaxCached];
        for (int i = 0; i < ncache; i++) {
            tcache[i] = nullptr;
            Pawn *p = items[i];
            if (A_FireListObjectAlive((void *)p)) tcache[i] = (void *)p->get_transform();
        }

        if (fn_AtkMapGetTargetForCollider != nullptr) {
            void *map = fn_GameGetAttackableColliderMap ? fn_GameGetAttackableColliderMap() : nullptr;
            if (A_FireObjectAlive(map)) {
                void *target = fn_AtkMapGetTargetForCollider(map, hitTransform);
                if (A_FireObjectAlive(target) && A_FireObjectAlive(target)) {
                    for (int i = 0; i < count; i++) {
                        Pawn *p = items[i];
                        if (!A_FireListObjectAlive((void *)p)) continue;
                        if ((void *)p == target) {
                            if (depthOut != nullptr) *depthOut = 0;
                            return p;
                        }
                    }
                    return nullptr;
                }
            }
        }

        void *t = hitTransform;
        for (int depth = 0; depth < 12 && A_FireObjectAlive(t); depth++) {
            for (int i = 0; i < count; i++) {
                void *pt;
                if (i < ncache) {
                    pt = tcache[i];
                    if (pt == nullptr) continue;
                } else {
                    Pawn *p = items[i];
                    if (!A_FireListObjectAlive((void *)p)) continue;
                    pt = (void *)p->get_transform();
                }
                if (pt == t) {
                    if (depthOut != nullptr) *depthOut = depth;
                    return items[i];
                }
            }
            if (fn_TransformGetParent == nullptr) break;
            void *parent = fn_TransformGetParent(t);
            if (!Tools::IsPtrValid(parent) || parent == t) break;
            t = parent;
        }
        return nullptr;
    }

    inline Pawn *A_FireCrosshairRayPawn(const Vector3 &camPos, const Vector3 &rayDir,
                                        Pawn **items, int count, Pawn *localPawn,
                                        float *hitDistOut, bool *symbolMissing)
    {
        if (hitDistOut != nullptr) *hitDistOut = -1.0f;
        if (symbolMissing != nullptr) *symbolMissing = false;
        const float len = Vector3::Magnitude(rayDir);
        if (!(len > 0.0001f)) return nullptr;
        const Vector3 dir(rayDir.x / len, rayDir.y / len, rayDir.z / len);

        if (fn_PhysicsRaycastAll != nullptr) {
            void *arr = fn_PhysicsRaycastAll(camPos, dir, A_FireTune::kCrosshairRayMax);
            AF_NamedHit h;
            if (A_FireRaycastAllNearest(arr, A_FireTune::kCrosshairRayMax,
                                        nullptr, localPawn, h,
                                        &g_triggerDiag.selfHitsSkipped,
                                        &g_triggerDiag.triggerHitsSkipped)) {
                if (h.distance < 0.0f) return nullptr;
                if (hitDistOut != nullptr) *hitDistOut = h.distance;
                if (h.transform == nullptr) return nullptr;
                return A_FirePawnFromHitTransform(h.transform, items, count);
            }
        }

        if (fn_PhysicsRaycastHitOut == nullptr) {
            if (symbolMissing != nullptr) *symbolMissing = true;
            return nullptr;
        }

        unsigned char hitBuf[AF_RAYHIT_SIZE];
        memset(hitBuf, 0, sizeof hitBuf);
        if (!fn_PhysicsRaycastHitOut(camPos, dir, hitBuf, A_FireTune::kCrosshairRayMax))
            return nullptr;

        const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
        if (hitDistOut != nullptr && hd >= 0.0f) *hitDistOut = hd;

        void *hitTransform = A_FireColliderTransform(A_FireHitCollider(hitBuf));
        if (A_FireObjectAlive(hitTransform) && localPawn != nullptr &&
            A_FireHitIsTarget(hitTransform, localPawn)) {
            g_triggerDiag.selfHitsSkipped++;
            return nullptr;
        }
        return A_FirePawnFromHitTransform(hitTransform, items, count);
    }

    inline FireBlock A_FireScan(Pawn *localPawn, Weapon *weapon, TriggerTarget &out)
    {
        TriggerbotDiag &d = g_triggerDiag;
        out = TriggerTarget{};
        g_afireLosOk = false;
        d.losBlockedScan = 0;
        d.enemyCount = 0;
        d.closestPx = -1.0f;
        d.candidates = 0;
        d.losChecked = 0;
        d.friendsInLine = 0;
        d.selfHitsSkipped = 0;
        d.triggerHitsSkipped = 0;
        d.occOverrides = 0;
        d.lastAimPx = -1.0f;
        d.lastPredPx = -1.0f;
        d.lastHitboxPx = 0.0f;
        d.lastSpreadPx = 0.0f;
        d.lastTravel = 0.0f;
        d.lastWorldDist = 0.0f;
        d.lastBulletSpeed = 0.0f;
        d.dotRayHonest = false;
        d.dotRayDecided = false;
        d.projKeepFrac = -1.0f;

        BaseGame *game = (BaseGame *)fn_GameGetMatchGame();
        if (!Tools::IsPtrValid((void *)game)) return FireBlock::NoMatch;

        Camera *cam = Camera::get_main();
        if (!A_FireObjectAlive((void *)cam)) return FireBlock::NoMatch;
        Transform *camTransform = cam->get_transform();
        if (!A_FireObjectAlive((void *)camTransform)) return FireBlock::NoMatch;
        const Vector3 camPos = camTransform->get_position();

        auto w2s = [&](const Vector3 &world, Vector3 &sc) -> bool {
            if (fn_WorldToScreen == nullptr) return false;
            sc = fn_WorldToScreen(cam, world);
            return true;
        };

        void *aimTarget = nullptr;
        if (fn_WeaponGetAimingTarget != nullptr && Tools::IsPtrValid(weapon)) {
            aimTarget = fn_WeaponGetAimingTarget(weapon);
            d.aimingTarget = (unsigned)(uintptr_t)aimTarget;
            d.aimingTargetDis = (fn_WeaponGetAimingTargetDis != nullptr && aimTarget != nullptr)
                              ? fn_WeaponGetAimingTargetDis(weapon) : 0.0f;
        } else {
            d.aimingTarget = 0;
            d.aimingTargetDis = 0.0f;
        }

        List<Pawn *> *enemyPawns = game->EnemyPawns();
        if (!Tools::IsPtrValid((void *)enemyPawns)) return FireBlock::NoEnemyInZone;
        Pawn **items = (Pawn **)enemyPawns->getItems();
        const int count = enemyPawns->getSize();
        d.enemyCount = count;
        if (!Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024)
            return FireBlock::NoEnemyInZone;

        float cx = 0.0f, cy = 0.0f;
        CrosshairCentrePx(cx, cy);
        d.crosshairCx = cx;
        d.crosshairCy = cy;
        const float radius = A_FireCrosshairRadiusPx();
        const float radiusSq = radius * radius;
        d.radiusPx = radius;

        const float focalPx = A_FireFocalPx();
        d.focalPx = focalPx;

        bool honestDotRay = false;
        Vector3 dotRayDir = Vector3::zero();
        if (fn_TransformGetForward != nullptr) {
            bool rayViaGame = false, rayFellBack = false, rayPlain = false;
            Vector3 camForward;
            CrosshairRayDir(cam, camTransform, camPos, camForward,
                            rayViaGame, rayFellBack, rayPlain);
            d.crosshairRayViaGame  = rayViaGame  ? 1 : 0;
            d.crosshairRayFellBack = rayFellBack ? 1 : 0;
            d.crosshairRayPlain    = rayPlain    ? 1 : 0;
            d.dotRayHonest = honestDotRay = rayViaGame || rayFellBack || rayPlain;
            dotRayDir = camForward;
        }
        if (fn_TransformGetForward != nullptr && honestDotRay) {
            bool symbolMissing = false;
            float centreHitDist = -1.0f;
            Pawn *centred = A_FireCrosshairRayPawn(camPos, dotRayDir, items, count, localPawn,
                                                   &centreHitDist, &symbolMissing);
            d.crosshairHitDist = centreHitDist;
            d.crosshairPawn = (unsigned)(uintptr_t)centred;
            if (symbolMissing && fn_PhysicsRaycastSegment == nullptr) {
                d.rayUnknown = true;
            } else if (centred != nullptr && centred != localPawn &&
                       !IsSameTeamAcceptBots(localPawn, centred) && A_FirePawnAlive(centred)) {
                const A_FireAimPoint ap = A_FireAimPointFor(centred);
                float bestSq = -1.0f;
                if (ap.valid) {
                    Vector3 sc = Vector3::zero();
                    if (w2s(ap.pos, sc) && sc.z > 0.0f) {
                        const float dx = sc.x - cx;
                        const float dy = sc.y - cy;
                        bestSq = dx * dx + dy * dy;
                    }
                }

                float limitPx = -1.0f;
                float testSq  = bestSq;
                if (ap.valid && ap.headOk && ap.feetOk && focalPx > 0.0f) {
                    Vector3 scHead = Vector3::zero(), scFeet = Vector3::zero();
                    if (w2s(ap.head, scHead) && w2s(ap.feet, scFeet) &&
                        scHead.z > 0.0f && scFeet.z > 0.0f) {
                        const float axisPx = sqrtf((scHead.x - scFeet.x) * (scHead.x - scFeet.x) +
                                                   (scHead.y - scFeet.y) * (scHead.y - scFeet.y));
                        const float axisM  = Vector3::Distance(ap.head, ap.feet);
                        if (axisPx > 0.5f && axisM > 0.3f) {
                            limitPx = A_FireTune::kBodyHalfWidthM * (axisPx / axisM) + 2.0f;
                            testSq  = A_FirePointSegDistSq(cx, cy, scHead.x, scHead.y,
                                                           scFeet.x, scFeet.y);
                        }
                    }
                }
                if (limitPx < 0.0f) {
                    float r = A_FireCrosshairRadiusPx();
                    if (!(r > 4.0f)) r = 4.0f;
                    limitPx = r + 2.0f;
                    testSq  = bestSq;
                }
                if (testSq < 0.0f || testSq > limitPx * limitPx) {
                    d.dotRayDeclined++;
                    return FireBlock::NoEnemyInZone;
                }
                d.projKeepFrac = sqrtf(testSq) / limitPx;
                out.pawn      = centred;
                out.aim       = ap;
                out.aimPoint  = ap.valid ? ap.pos : camPos;
                out.screenPos = Vector3::zero();
                if (ap.valid) w2s(ap.pos, out.screenPos);
                out.pxOffset  = bestSq >= 0.0f ? sqrtf(bestSq) : -1.0f;
                out.worldDist = ap.valid ? Vector3::Distance(camPos, ap.pos) : centreHitDist;
                out.path      = 0;
                out.visible   = true;
                out.visibilityIdentified = true;
                d.lastAimPx       = out.pxOffset;
                d.lastWorldDist   = out.worldDist;
                d.aimPath         = 0;
                d.raysIdentified++;
                g_afireLosOk      = true;
                d.targets++;
                return FireBlock::Fired;
            }
        }

        if (fn_TransformGetForward != nullptr && honestDotRay) {
            d.dotRayDecided = true;
            return FireBlock::NoEnemyInZone;
        }

        static auto getTeamIdFn = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);
        const uint32_t localTeam = (A_FireObjectAlive((void *)localPawn) && getTeamIdFn != nullptr)
                                 ? getTeamIdFn(localPawn) : 0u;
        auto sameTeamGate = [&](Pawn *p) -> bool {
            if (!A_FireObjectAlive((void *)p)) return false;
            if (p == localPawn) return true;
            if (localTeam == 0u || getTeamIdFn == nullptr) return false;
            return getTeamIdFn(p) == localTeam;
        };

        constexpr int MAX_CANDIDATES = 12;
        A_FireCandidate cands[MAX_CANDIDATES];
        int n = 0;

        for (int i = 0; i < count; i++) {
            Pawn *p = items[i];

            if (!A_FireListObjectAlive((void *)p)) continue;
            if (p == localPawn) continue;
            if (sameTeamGate(p)) continue;
            if (!A_FirePawnAlive(p)) continue;
            const A_FireAimPoint ap = A_FireAimPointFor(p);
            if (!ap.valid) continue;

            float d2 = -1.0f;
            Vector3 scChest = Vector3::zero();
            if (w2s(ap.pos, scChest) && scChest.z > 0.0f) {
                const float dx = scChest.x - cx;
                const float dy = scChest.y - cy;
                d2 = dx * dx + dy * dy;
            }

            if (d2 >= 0.0f && d2 > radiusSq && ap.headOk && ap.feetOk) {
                Vector3 scHead = Vector3::zero();
                Vector3 scFeet = Vector3::zero();
                if (w2s(ap.head, scHead) && w2s(ap.feet, scFeet) &&
                    scHead.z > 0.0f && scFeet.z > 0.0f) {
                    const float body = A_FirePointSegDistSq(cx, cy,
                                                            scHead.x, scHead.y,
                                                            scFeet.x, scFeet.y);
                    if (d2 < 0.0f || body < d2) d2 = body;
                }
            }
            if (d2 < 0.0f) continue;
            if (d.closestPx < 0.0f || d2 < d.closestPx) d.closestPx = d2;
            if (d2 > radiusSq) continue;
            const float pxOffset = sqrtf(d2);
            if (n >= MAX_CANDIDATES) continue;

            const float candDist = Vector3::Distance(camPos, ap.pos);

            cands[n].pawn      = p;
            cands[n].aim       = ap;
            cands[n].pxOffset  = pxOffset;
            cands[n].worldDist = candDist;
            cands[n].velocity  = GetPawnVelocity(p);
            n++;
        }
        d.candidates = n;
        if (n <= 0) return FireBlock::NoEnemyInZone;

        AF_FriendProbe friendProbes[64];
        const int friendCount = A_FireCollectFriendProbes(localPawn, friendProbes, 64);

        int order[MAX_CANDIDATES];
        for (int i = 0; i < n; i++) order[i] = i;
        for (int i = 1; i < n; i++) {
            const int oi = order[i];
            float keyI = cands[oi].pxOffset;
            if (aimTarget != nullptr && (void *)cands[oi].pawn == aimTarget) keyI -= 0.5f;
            int j = i - 1;
            while (j >= 0) {
                const int oj = order[j];
                float keyJ = cands[oj].pxOffset;
                if (aimTarget != nullptr && (void *)cands[oj].pawn == aimTarget) keyJ -= 0.5f;
                if (keyJ <= keyI) break;
                order[j + 1] = order[j];
                j--;
            }
            order[j + 1] = oi;
        }

        constexpr bool checkSpread = true;
        constexpr bool wantLead    = true;
        const float spreadPx   = A_FireSpreadPx(weapon, focalPx, checkSpread);
        d.lastSpreadPx = spreadPx;

        bool blockedLos = false, blockedFriend = false;

        bool speedKnown = false;
        const float bulletSpeed = GetWeaponBulletSpeed(weapon, &speedKnown);

        for (int oi = 0; oi < n; oi++) {
            const A_FireCandidate &c = cands[order[oi]];

            d.losChecked++;
            bool losIdentified = false;
            float losHitDist = -1.0f;
            AF_Ray los = A_FireRayToTarget(camPos, c.aim.pos, c.pawn, c.aim.radius,
                                           localPawn, &losIdentified, &losHitDist);

            Vector3 probePoint  = c.aim.pos;
            float   probeRadius = c.aim.radius;
            if (los != AF_Ray::Clear && c.aim.headOk) {
                bool headIdentified = false;
                float headHitDist = -1.0f;
                const AF_Ray losHead = A_FireRayToTarget(camPos, c.aim.head, c.pawn,
                                                         A_FireTune::kHeadRadius,
                                                         localPawn, &headIdentified, &headHitDist);
                if (losHead == AF_Ray::Clear) {
                    los           = losHead;
                    losIdentified = headIdentified;
                    losHitDist    = headHitDist;
                    probePoint    = c.aim.head;
                    probeRadius   = A_FireTune::kHeadRadius;
                }
            }
            const float probeDist = Vector3::Distance(camPos, probePoint);
            d.lastLosHitDist = losHitDist;
            if (losIdentified) d.raysIdentified++; else d.raysBlocked++;
            if (los == AF_Ray::Unknown) d.rayUnknown = true;
            if (los != AF_Ray::Clear) {
                d.losBlocked++;
                d.losBlockedScan++;
                blockedLos = true;
                continue;
            }

            float travel = 0.0f;
            Vector3 predicted = probePoint;
            bool lead = false;

            if (wantLead && speedKnown && probeDist > 1.0f) {
                travel = probeDist / bulletSpeed;
                d.lastTravel = travel;
                if (travel > 0.002f && travel <= A_FireTune::kMaxTravelTime) {
                    predicted = probePoint + c.velocity * travel;
                    lead = true;
                }

                if (lead && A_FireRayToTarget(camPos, predicted, c.pawn, probeRadius,
                                              localPawn) != AF_Ray::Clear) {
                    d.leadPathBlocked = true;
                    lead = false;
                    predicted = probePoint;
                }
            }

            float predPx = c.pxOffset;
            if (lead) {
                Vector3 scPred = Vector3::zero();
                if (w2s(predicted, scPred) && scPred.z > 0.0f) {
                    const float pdx = scPred.x - cx;
                    const float pdy = scPred.y - cy;
                    predPx = sqrtf(pdx * pdx + pdy * pdy);
                } else {
                    predicted = probePoint;
                    lead = false;
                }
            }
            const float hitboxPx = (focalPx > 0.0f && probeDist > 0.01f)
                                 ? (probeRadius * focalPx / probeDist) : 0.0f;

            if (!A_FireShotWouldLand(predPx, radius, hitboxPx, spreadPx))
                d.spreadWarn = true;

            Vector3 dirToAim(probePoint.x - camPos.x, probePoint.y - camPos.y, probePoint.z - camPos.z);
            const float dirLen = Vector3::Magnitude(dirToAim);
            if (dirLen > 0.01f) {
                dirToAim = Vector3(dirToAim.x / dirLen, dirToAim.y / dirLen, dirToAim.z / dirLen);
                int friends = 0;
                if (A_FireFriendlyInLine(friendProbes, friendCount, camPos, dirToAim, probeDist, &friends)) {
                    d.friendsInLine += friends;
                    blockedFriend = true;
                    continue;
                }
            }

            out.pawn       = c.pawn;
            out.aim        = c.aim;
            out.aimPoint   = probePoint;
            out.screenPos = Vector3::zero();
            w2s(probePoint, out.screenPos);
            out.pxOffset   = c.pxOffset;
            out.worldDist  = probeDist;
            out.travelTime = travel;
            out.predicted  = predicted;
            out.predPxOffset = predPx;
            out.hitboxPx   = hitboxPx;
            out.spreadPx   = spreadPx;
            out.focalPx    = focalPx;
            out.bulletSpeed = bulletSpeed;
            out.speedKnown = speedKnown;
            out.leadUsed   = lead;
            out.path       = 1;
            out.visible    = true;
            out.visibilityIdentified = losIdentified;

            d.lastAimPx  = c.pxOffset;
            d.lastPredPx = predPx;
            d.lastHitboxPx = hitboxPx;
            d.lastTravel = travel;
            d.lastWorldDist = probeDist;
            d.lastBulletSpeed = bulletSpeed;
            g_afireLosOk = true;
            d.targets++;
            return FireBlock::Fired;
        }

        if (blockedFriend)   return FireBlock::FriendlyInLine;
        if (blockedLos)      return FireBlock::NoLineOfSight;
        return FireBlock::NoEnemyInZone;
    }

    inline FireBlock A_FireShouldFire(TriggerTarget &out)
    {
        TriggerbotDiag &d = g_triggerDiag;
        out = TriggerTarget{};

        Pawn *localPawn = GamePlay::get_LocalPawn();
        d.localPawn = (void *)localPawn;
        if (!A_FireObjectAlive((void *)localPawn)) return FireBlock::NoLocalPawn;
        if (!A_FirePawnAlive(localPawn)) return FireBlock::LocalNotAlive;

        if (fn_GameGetMatchGame == nullptr) return FireBlock::NoMatch;
        void *game = fn_GameGetMatchGame();
        d.matchGame = game;
        if (!Tools::IsPtrValid(game)) return FireBlock::NoMatch;
        if (fn_BaseGameIsPaused != nullptr && fn_BaseGameIsPaused(game))
            return FireBlock::MatchPaused;

        Weapon *held = localPawn->get_CurrentWeapon();
        d.heldWeapon = (void *)held;
        if (!A_FireObjectAlive((void *)held)) return FireBlock::NoWeapon;

        d.switching = A_FireWeaponSwitching(held);
        if (d.switching) {
            if (!A_FireWeaponSwitching(held))
                d.switching = false;
        }
        d.ammoOk    = A_FireWeaponHasAmmo(held);
        d.canFire   = A_FireWeaponCanFire(held);

        const ScopeSettle settle = A_FireScopingTransition((void *)held, localPawn);
        if (settle.settling) return FireBlock::AdsTransition;

        if (Config.NewFeaturesMenu.TriggerbotMode == 1) {
            const AdsRead ads = A_FireAdsGate((void *)held, localPawn, settle);
            d.adsFlag    = ads.fieldFlag ? 1 : 0;
            d.adsPct     = ads.fieldPct;
            d.adsUnknown = ads.unknown;
            d.adsPawnOk  = ads.pawnOk;
            d.adsPawnAim = ads.pawnAim;
            d.adsFcOk    = ads.fcOk;
            d.adsFcAim   = ads.fcAim;
            if (!ads.aiming)
                return FireBlock::ModeNotAds;
        } else if (Config.NewFeaturesMenu.TriggerbotMode == 2) {
            if (fn_PawnGetIsFiring == nullptr || !fn_PawnGetIsFiring(localPawn))
                return FireBlock::ModeNotFiring;
        }

        return A_FireScan(localPawn, held, out);
    }

    inline uintptr_t A_FireFindTarget()
    {
        TriggerTarget target{};
        if (A_FireShouldFire(target) != FireBlock::Fired) return 0;
        return target.pawn ? (uintptr_t)target.pawn : 0;
    }

    inline void A_FireReleaseTrigger(void *instance)
    {
        if (!g_afireHoldingFire) return;
        if (instance == nullptr || instance == g_afireFiringOn) {
            if (fn_Weapon_StopFire != nullptr && A_FireObjectAlive(g_afireFiringOn))
                fn_Weapon_StopFire(g_afireFiringOn, false);
            g_afireHoldingFire = false;
            g_afireFiringOn = nullptr;
        }
    }

    inline bool g_afireReleasePending = false;

    inline void A_FireReleaseTriggerDeferred(void *instance)
    {
        if (A_FireAdsSwapActive()) {
            g_afireReleasePending = true;
            return;
        }
        g_afireReleasePending = false;
        A_FireReleaseTrigger(instance);
    }

    inline void A_FireRunTestFire()
    {
        TriggerbotDiag &d = g_triggerDiag;
        g_testFireRequest = false;

        Pawn *local = GamePlay::get_LocalPawn();
        d.localPawn = (void *)local;
        if (!A_FireIsLiveLocalPawn(local)) { d.testFireStatus = 2; return; }

        Weapon *held = local->get_CurrentWeapon();
        d.heldWeapon = (void *)held;
        if (!Tools::IsPtrValid((void *)held)) { d.testFireStatus = 3; return; }

        if (fn_Weapon_StartFire == nullptr) { d.testFireStatus = 4; return; }

        fn_Weapon_StartFire(held);
        d.fires++;
        d.testFireStatus = 1;
    }

    inline void A_FireTick(void *tickInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        const double now = A_FireNow();
        d.lastTickTime = now;

        if (!Config.NewFeaturesMenu.Triggerbot &&
            !g_afireHoldingFire && !g_testFireRequest) {
            if (now - g_afireOffDiagStamp < 0.1) return;
            g_afireOffDiagStamp = now;
        }

        Pawn *localPawn = A_FireLocalPawnFast();
        d.localPawn = (void *)localPawn;

        if (tickInstance != nullptr) {
            if (now - g_afireHeldWeaponStamp < 0.05 &&
                tickInstance != g_afireHeldWeaponCache)
                return;
            Weapon *held = A_FireObjectAlive((void *)localPawn) ? localPawn->get_CurrentWeapon() : nullptr;
            if (!A_FireObjectAlive((void *)held) || tickInstance != (void *)held)
                return;
            g_afireHeldWeaponCache = (void *)held;
            g_afireHeldWeaponStamp = now;
            d.weaponHeldTicks++;
        }

        if (now - g_afireLastEvalTime < 0.004) return;
        g_afireLastEvalTime = now;
        if (g_afireInTick) return;
        g_afireInTick = true;

        if (g_testFireRequest) A_FireRunTestFire();

        d.hookCalls++;
        d.scans++;

        d.aimPath = 1;
        d.crosshairPawn = 0;
        d.crosshairHitDist = -1.0f;
        d.raysIdentified = 0;
        d.raysBlocked = 0;
        d.rayUnknown = false;
        d.lastLosHitDist = -1.0f;
        d.leadPathBlocked = false;
        d.spreadWarn = false;

        if (!Config.NewFeaturesMenu.Triggerbot) {
            A_FireReleaseTrigger(nullptr);
            A_FireReportBlock(FireBlock::FeatureOff);
            s_dbgTriggerState = 0;
            g_afireInTick = false;
            return;
        }

        TriggerTarget target{};
        const FireBlock block = A_FireShouldFire(target);
        A_FireReportBlock(block);

        if (g_afireReleasePending && block != FireBlock::AdsTransition) {
            g_afireReleasePending = false;
            A_FireReleaseTrigger(nullptr);
        }

        if (block != FireBlock::Fired || !Tools::IsPtrValid((void *)target.pawn)) {
            A_FireReleaseTriggerDeferred(nullptr);
            s_dbgTriggerState = 0;
            g_afireInTick = false;
            return;
        }

        s_dbgTriggerState = 1;

        Weapon *held = (Weapon *)d.heldWeapon;
        if (!A_FireObjectAlive((void *)held)) {
            A_FireReleaseTriggerDeferred(nullptr);
            g_afireInTick = false;
            return;
        }

        if (g_afireHoldingFire && g_afireFiringOn != (void *)held)
            A_FireReleaseTriggerDeferred(nullptr);

        const FireMode mode     = GetWeaponFireMode(held);
        const float    interval = A_FireWeaponInterval(held);
        d.lastFireInterval = interval;

        const bool edgePress = (mode != FireMode::Auto && mode != FireMode::Burst);

        if (g_afireHoldingFire && g_afireFiringOn == (void *)held &&
            g_afireTargetPawn == (void *)target.pawn) {
            if (edgePress) {
                A_FireReleaseTrigger(nullptr);
            } else {
                const bool becameReady = d.canFire && !g_afireCanFireLast;
                const bool stale = (now - g_afireLastPressTime) >=
                                   (double)A_FireTune::kReassertPress;
                if (fn_Weapon_StartFire != nullptr && (becameReady || stale)) {
                    fn_Weapon_StartFire(held);
                    g_afireLastPressTime = now;
                    d.reasserts++;
                }
            }
            g_afireCanFireLast = d.canFire;
            s_dbgTriggerState = 2;
            g_afireInTick = false;
            return;
        }

        d.paceWaiting = false;
        if (edgePress) {
            if (g_afireTargetPawn != (void *)target.pawn ||
                g_afireLastPressWeapon != (void *)held)
                g_afireLastPressTime = -1000.0;
            const float  pace    = A_FireRepressPaceSeconds(interval);
            const double since   = now - g_afireLastPressTime;
            const bool   tooSoon = since < (double)pace;

            const bool   ready   = (fn_WeaponCanFire != nullptr) && fn_WeaponCanFire(held);
            d.paceSeconds = pace;
            if (tooSoon && !ready) {
                d.repressSkipped++;
                d.paceWaiting = true;
                s_dbgTriggerState = 1;
                g_afireInTick = false;
                return;
            }
        }

        if (fn_Weapon_StartFire == nullptr) {
            A_FireReleaseTriggerDeferred(nullptr);
            A_FireReportBlock(FireBlock::WeaponNotReady);
            g_afireInTick = false;
            return;
        }

        g_afireHoldingFire    = true;
        g_afireFiringOn       = (void *)held;
        g_afireTargetPawn     = (void *)target.pawn;
        g_afireLastPressWeapon = (void *)held;
        g_afireLastPressTime  = now;
        g_afireCanFireLast    = d.canFire;
        fn_Weapon_StartFire(held);
        d.fires++;
        s_dbgTriggerState = 2;

        LOGI("[NewFeatures] triggerbot: StartFire(held=%p) target=%p path=%s aim=%.1fpx "
             "hit=%.3fm ident=%d dist=%.3f travel=%.4f",
             (void *)held, (void *)target.pawn,
             target.path == 0 ? "centre-ray" : "zone",
             target.pxOffset,
             d.lastLosHitDist, (int)target.visibilityIdentified,
             target.worldDist, target.travelTime);

        g_afireInTick = false;
    }

    inline void A_FireResetForNewMatch()
    {
        g_afireHoldingFire = false;
        g_afireTargetPawn  = nullptr;
        g_afireFiringOn    = nullptr;
        g_afireLastPressTime = -1000.0;
        g_afireLastPressWeapon = nullptr;
        g_afireCanFireLast   = false;
        g_afireLastEvalTime  = -1000.0;
        g_afireInTick        = false;
        g_afireLosOk         = false;

        g_afireLocalPawnCache     = nullptr;
        g_afireLocalPawnCacheTime = -1000.0;
        g_afireHeldWeaponCache    = nullptr;
        g_afireHeldWeaponStamp    = -1000.0;

        g_afireAdsSettleUntil  = -1000.0;
        g_afireAdsSettleMax    = -1000.0;
        g_afireAdsLastState    = -1;
        g_afireAdsLastComp     = nullptr;
        g_afireAdsCompSeen     = false;
        g_afireAdsSettling     = false;
        g_afireReleasePending  = false;
    }

    inline void UpdateNoSprintFireDelayWeapon(void *localPawnInstance);

    inline void A_FirePawnTicked(void *pawnInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        d.pawnTicks++;

        if (A_FireLocalPawnFast() == (Pawn *)pawnInstance) {
            d.localPawn = pawnInstance;
            d.pawnLocalTicks++;

            UpdateNoSprintFireDelayWeapon(pawnInstance);
            A_FireTick(nullptr);
        }
    }

    inline void Hook_Weapon_Tick(void *instance, float deltaTime)
    {
        if (orig_Weapon_Tick != nullptr)
            orig_Weapon_Tick(instance, deltaTime);

        if (instance == nullptr) return;

        g_triggerDiag.weaponTicks++;
        A_FireTick(instance);
    }

    inline void Hook_Pawn_Tick(void *instance, float deltaTime)
    {
        if (orig_Pawn_Tick != nullptr)
            orig_Pawn_Tick(instance, deltaTime);

        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_PlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_PlayerPawn_Tick != nullptr)
            orig_PlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_MPPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_MPPawn_Tick != nullptr)
            orig_MPPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_BRPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_BRPlayerPawn_Tick != nullptr)
            orig_BRPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_DMZPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_DMZPlayerPawn_Tick != nullptr)
            orig_DMZPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_PVEPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_PVEPlayerPawn_Tick != nullptr)
            orig_PVEPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_OpenWorldPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_OpenWorldPlayerPawn_Tick != nullptr)
            orig_OpenWorldPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void Hook_BRGroundWarPlayerPawn_Tick(void *instance, float deltaTime)
    {
        if (orig_BRGroundWarPlayerPawn_Tick != nullptr)
            orig_BRGroundWarPlayerPawn_Tick(instance, deltaTime);
        if (instance == nullptr) return;
        A_FirePawnTicked(instance);
    }

    inline void InstallTriggerbotHooks()
    {
        auto startFireAddr = UnityRva(RVA::Weapon_StartFire);
        if (Tools::IsPtrValid(startFireAddr))
            fn_Weapon_StartFire = (void (*)(void *))startFireAddr;

        auto stopFireAddr = UnityRva(RVA::Weapon_StopFire);
        if (Tools::IsPtrValid(stopFireAddr))
            fn_Weapon_StopFire = (void (*)(void *, bool))stopFireAddr;

        auto w2sAddr = UnityRva(RVA::Camera_WorldToScreenPoint);
        if (Tools::IsPtrValid(w2sAddr))
            fn_WorldToScreen = (Vector3 (*)(void *, Vector3))w2sAddr;

        auto s2wAddr = UnityRva(RVA::Camera_ScreenToWorldPoint);
        if (Tools::IsPtrValid(s2wAddr))
            fn_ScreenToWorldPoint = (Vector3 (*)(void *, Vector3))s2wAddr;

        static_assert(RVA::Physics_RaycastLOS == 0x506C1E4, "Physics_RaycastLOS must stay 0x506C1E4 - the 2-arg out-struct overload");
        auto rayAddr = UnityRva(RVA::Physics_RaycastLOS);
        if (Tools::IsPtrValid(rayAddr))
            fn_PhysicsRaycastLOS =
                (bool (*)(Vector3, Vector3, float, void *))rayAddr;

        auto rayHitOutAddr = UnityRva(RVA::Physics_RaycastHitOut);
        if (Tools::IsPtrValid(rayHitOutAddr))
            fn_PhysicsRaycastHitOut =
                (bool (*)(Vector3, Vector3, void *, float))rayHitOutAddr;

        auto raySegAddr = UnityRva(RVA::Physics_RaycastSegment);
        if (Tools::IsPtrValid(raySegAddr))
            fn_PhysicsRaycastSegment = (bool (*)(Vector3, Vector3, float))raySegAddr;

        auto rayAllAddr = UnityRva(RVA::Physics_RaycastAll);
        if (Tools::IsPtrValid(rayAllAddr))
            fn_PhysicsRaycastAll = (void *(*)(Vector3, Vector3, float))rayAllAddr;

        auto atkMapAddr = UnityRva(RVA::GamePlay_get_AttackableColliderMap);
        if (Tools::IsPtrValid(atkMapAddr))
            fn_GameGetAttackableColliderMap = (void *(*)())atkMapAddr;
        auto atkTargetAddr = UnityRva(RVA::AtkColliderMap_GetTargetForCollider);
        if (Tools::IsPtrValid(atkTargetAddr))
            fn_AtkMapGetTargetForCollider = (void *(*)(void *, void *))atkTargetAddr;
        else
            LOGE("[NewFeatures] triggerbot: RaycastAll RVA 0x%llX did not resolve -"
                 " LOS falls back to the single-ray + occlusion path",
                 (unsigned long long)RVA::Physics_RaycastAll);

        auto trigAddr = UnityRva(RVA::Collider_IsTrigger);
        if (Tools::IsPtrValid(trigAddr))
            fn_ColliderIsTrigger = (bool (*)(void *))trigAddr;

        auto fwdAddr = UnityRva(RVA::Transform_get_forward);
        if (Tools::IsPtrValid(fwdAddr))
            fn_TransformGetForward = (Vector3 (*)(void *))fwdAddr;

        auto parentAddr = UnityRva(RVA::Transform_get_parent);
        if (Tools::IsPtrValid(parentAddr))
            fn_TransformGetParent = (void *(*)(void *))parentAddr;

        auto rightAddr = UnityRva(RVA::Transform_get_right);
        if (Tools::IsPtrValid(rightAddr))
            fn_TransformGetRight = (Vector3 (*)(void *))rightAddr;

        auto upAddr = UnityRva(RVA::Transform_get_up);
        if (Tools::IsPtrValid(upAddr))
            fn_TransformGetUp = (Vector3 (*)(void *))upAddr;

        auto rrAddr = UnityRva(RVA::Pawn_get_RecentlyRendered);
        if (Tools::IsPtrValid(rrAddr))
            fn_PawnRecentlyRendered = (bool (*)(void *))rrAddr;

        auto lvAddr = UnityRva(RVA::Pawn_get_LastOccVisible);
        if (Tools::IsPtrValid(lvAddr))
            fn_PawnLastOccVisible = (bool (*)(void *))lvAddr;

        auto camMainAddr = UnityRva(RVA::Camera_get_main);
        if (Tools::IsPtrValid(camMainAddr))
            fn_CameraGetMain = (void *(*)())camMainAddr;

        auto aimingAddr = UnityRva(RVA::Pawn_IsAiming);
        if (Tools::IsPtrValid(aimingAddr))
            fn_PawnIsAiming = (bool (*)(void *))aimingAddr;
        else
            LOGE("[NewFeatures] triggerbot: Pawn::IsAiming 0x%llX did not resolve -"
                 " \"ADS only\" falls back to the fire component",
                 (unsigned long long)RVA::Pawn_IsAiming);

        auto curFireAddr = UnityRva(RVA::Weapon_get_CurrentFireComponent);
        if (Tools::IsPtrValid(curFireAddr))
            fn_WeaponGetCurrentFireComponent = (void *(*)(void *))curFireAddr;

        auto fcAimAddr = UnityRva(RVA::GetIsAiming);
        if (Tools::IsPtrValid(fcAimAddr))
            fn_FireComponentGetIsAiming = (bool (*)(void *))fcAimAddr;

        auto firingAddr = UnityRva(RVA::Pawn_get_IsFiring);
        if (Tools::IsPtrValid(firingAddr))
            fn_PawnGetIsFiring = (bool (*)(void *))firingAddr;

        auto ggmAddr = UnityRva(RVA::Gameplay_get_MatchGame);
        if (Tools::IsPtrValid(ggmAddr))
            fn_GameGetMatchGame = (void *(*)())ggmAddr;

        auto autoFireAddr = UnityRva(RVA::Weapon_get_IsAutoFire);
        if (Tools::IsPtrValid(autoFireAddr))
            fn_WeaponIsAutoFire = (bool (*)(void *))autoFireAddr;

        auto burstAddr = UnityRva(RVA::Weapon_get_IsBurstFire);
        if (Tools::IsPtrValid(burstAddr))
            fn_WeaponIsBurstFire = (bool (*)(void *))burstAddr;

        auto singleAddr = UnityRva(RVA::Weapon_get_IsSingleShot);
        if (Tools::IsPtrValid(singleAddr))
            fn_WeaponIsSingleShot = (bool (*)(void *))singleAddr;

        auto ammoOkAddr = UnityRva(RVA::AmmoCanFire);
        if (Tools::IsPtrValid(ammoOkAddr))
            fn_WeaponAmmoCanFire = (bool (*)(void *))ammoOkAddr;

        auto clipAddr = UnityRva(RVA::Weapon_get_ClipAmmoCount);
        if (Tools::IsPtrValid(clipAddr))
            fn_WeaponGetClipAmmoCount = (int (*)(void *))clipAddr;

        auto canFireAddr = UnityRva(RVA::Weapon_CanFire);
        if (Tools::IsPtrValid(canFireAddr))
            fn_WeaponCanFire = (bool (*)(void *))canFireAddr;

        auto swallowingAddr = UnityRva(RVA::Weapon_InSwitchWeaponState);
        if (Tools::IsPtrValid(swallowingAddr))
            fn_WeaponInSwitchWeaponState = (bool (*)(void *))swallowingAddr;

        auto switchingAddr = UnityRva(RVA::Weapon_HasSwitchingState);
        if (Tools::IsPtrValid(switchingAddr))
            fn_WeaponHasSwitchingState = (bool (*)(void *))switchingAddr;

        auto realIntervalAddr = UnityRva(RVA::Weapon_RealFireInterval);
        if (Tools::IsPtrValid(realIntervalAddr))
            fn_WeaponRealFireInterval = (float (*)(void *))realIntervalAddr;

        auto aimTargetAddr = UnityRva(RVA::Weapon_get_AimingTarget);
        if (Tools::IsPtrValid(aimTargetAddr))
            fn_WeaponGetAimingTarget = (void *(*)(void *))aimTargetAddr;

        auto aimTargetDisAddr = UnityRva(RVA::Weapon_get_CurrentAimingTargetDis);
        if (Tools::IsPtrValid(aimTargetDisAddr))
            fn_WeaponGetAimingTargetDis = (float (*)(void *))aimTargetDisAddr;

        auto fovAddr = UnityRva(RVA::Camera_get_fieldOfView);
        if (Tools::IsPtrValid(fovAddr))
            fn_CameraGetFov = (float (*)(void *))fovAddr;

        auto teamPawnsAddr = UnityRva(RVA::BaseGame_GetTeamPawns);
        if (Tools::IsPtrValid(teamPawnsAddr))
            fn_BaseGameGetTeamPawns = (void *(*)(void *))teamPawnsAddr;

        auto pausedAddr = UnityRva(RVA::BaseGame_get_IsPaused);
        if (Tools::IsPtrValid(pausedAddr))
            fn_BaseGameIsPaused = (bool (*)(void *))pausedAddr;

        auto isAliveAddr = UnityRva(RVA::Pawn_get_IsAlive);
        if (Tools::IsPtrValid(isAliveAddr))
            fn_PawnIsAlive = (bool (*)(void *))isAliveAddr;

        void *weaponTick = UnityRva(RVA::Weapon_Tick);
        if (Tools::IsPtrValid(weaponTick)) {
            Tools::Hook(weaponTick, (void *)&Hook_Weapon_Tick, (void **)&orig_Weapon_Tick);
            if (orig_Weapon_Tick == nullptr)
                LOGE("[NewFeatures] triggerbot: Weapon::Tick hook FAILED at %p", weaponTick);
            else
                LOGI("[NewFeatures] triggerbot: hooked Weapon::Tick @%p -> orig %p",
                     weaponTick, (void *)orig_Weapon_Tick);
        } else {
            LOGE("[NewFeatures] triggerbot: Weapon::Tick RVA 0x%llX did not resolve -"
                 " triggerbot DISABLED", (unsigned long long)RVA::Weapon_Tick);
        }

        void *pawnTick = UnityRva(RVA::Pawn_Tick);
        if (Tools::IsPtrValid(pawnTick)) {
            Tools::Hook(pawnTick, (void *)&Hook_Pawn_Tick, (void **)&orig_Pawn_Tick);
            LOGI("[NewFeatures] triggerbot: hooked Pawn::Tick @%p -> orig %p",
                 pawnTick, (void *)orig_Pawn_Tick);
        } else {
            LOGE("[NewFeatures] triggerbot: Pawn::Tick RVA 0x%llX did not resolve",
                 (unsigned long long)RVA::Pawn_Tick);
        }

        struct TickTarget { uintptr_t rva; void *hook; void **orig; const char *name; };
        const TickTarget pawnTickTargets[] = {
            { RVA::PlayerPawn_Tick,            (void *)&Hook_PlayerPawn_Tick,            (void **)&orig_PlayerPawn_Tick,            "PlayerPawn::Tick" },
            { RVA::MPPawn_Tick,                (void *)&Hook_MPPawn_Tick,                (void **)&orig_MPPawn_Tick,                "MPPawn::Tick"     },
            { RVA::BRPlayerPawn_Tick,          (void *)&Hook_BRPlayerPawn_Tick,          (void **)&orig_BRPlayerPawn_Tick,          "BRPlayerPawn::Tick" },
            { RVA::DMZPlayerPawn_Tick,         (void *)&Hook_DMZPlayerPawn_Tick,         (void **)&orig_DMZPlayerPawn_Tick,         "DMZPlayerPawn::Tick" },
            { RVA::PVEPlayerPawn_Tick,         (void *)&Hook_PVEPlayerPawn_Tick,         (void **)&orig_PVEPlayerPawn_Tick,         "PVEPlayerPawn::Tick" },
            { RVA::OpenWorldPlayerPawn_Tick,   (void *)&Hook_OpenWorldPlayerPawn_Tick,   (void **)&orig_OpenWorldPlayerPawn_Tick,   "OpenWorldPlayerPawn::Tick" },
            { RVA::BRGroundWarPlayerPawn_Tick, (void *)&Hook_BRGroundWarPlayerPawn_Tick, (void **)&orig_BRGroundWarPlayerPawn_Tick, "BRGroundWarPlayerPawn::Tick" },
        };
        for (const TickTarget &t : pawnTickTargets) {
            void *addr = UnityRva(t.rva);
            if (Tools::IsPtrValid(addr)) {
                Tools::Hook(addr, t.hook, t.orig);
                LOGI("[NewFeatures] triggerbot: hooked %s @%p -> orig %p",
                     t.name, addr, *t.orig);
            } else {
                LOGE("[NewFeatures] triggerbot: %s RVA 0x%llX did not resolve",
                     t.name, (unsigned long long)t.rva);
            }
        }

        LOGI("[NewFeatures] triggerbot: StartFire=%p StopFire=%p W2S=%p S2W=%p RayAll=%p RayHitOut=%p RaySeg=%p IsTrigger=%p Fwd=%p Parent=%p Raycast=%p CamRight=%p CamUp=%p",
             (void *)fn_Weapon_StartFire, (void *)fn_Weapon_StopFire,
             (void *)fn_WorldToScreen, (void *)fn_ScreenToWorldPoint,
             (void *)fn_PhysicsRaycastAll,
             (void *)fn_PhysicsRaycastHitOut, (void *)fn_PhysicsRaycastSegment,
             (void *)fn_ColliderIsTrigger, (void *)fn_TransformGetForward,
             (void *)fn_TransformGetParent, (void *)fn_PhysicsRaycastLOS,
             (void *)fn_TransformGetRight, (void *)fn_TransformGetUp);
    }

    inline bool A_FireHeartbeatAlive()
    {
        const double t = g_triggerDiag.lastTickTime;
        return t > 0.0 && (A_FireNow() - t) < 0.5;
    }

    inline void A_FireRenderTick()
    {
        TriggerbotDiag &d = g_triggerDiag;

        if (g_testFireRequest && A_FireNow() - g_testFireRequestTime > 0.35) {
            g_testFireRequest = false;
            Pawn *local = GamePlay::get_LocalPawn();
            d.localPawn = (void *)local;
            if (!A_FireIsLiveLocalPawn(local)) { d.testFireStatus = 2; return; }
            Weapon *held = local->get_CurrentWeapon();
            d.heldWeapon = (void *)held;
            if (!Tools::IsPtrValid((void *)held)) { d.testFireStatus = 3; return; }
            d.testFireStatus = (fn_Weapon_StartFire == nullptr) ? 4 : 5;
        }
    }

    inline int GetTriggerbotDebugState() { return s_dbgTriggerState; }
    inline TriggerbotDiag &GetTriggerbotDiag() { return g_triggerDiag; }

    namespace FiringTiming
    {
        constexpr uintptr_t Weapon_SpringFireGetReady = 0xDC;
        constexpr uintptr_t Weapon_UseGetReadyTime    = 0xDF;
        constexpr uintptr_t Weapon_DelaySprintFire    = 0x9F8;
        constexpr uintptr_t Weapon_GetReadyTime       = 0xBA0;

        constexpr uintptr_t get_DelaySprintFire    = 0x513CBC4;
        constexpr uintptr_t get_FireInterval       = 0x5105C74;
        constexpr uintptr_t get_FireBoltTime       = 0x51237F4;
        constexpr uintptr_t get_UseGetReadyTime    = 0x50E9F60;
        constexpr uintptr_t get_SpringFireGetReady = 0x50E9F28;
        constexpr uintptr_t get_GetReadyTime       = 0x514D2C8;
        constexpr uintptr_t LocalPawn_GetCurrentWeapon = 0x518BEA8;
    }

    inline std::atomic<void *> g_noSprintFireDelayWeapon{nullptr};

    inline void UpdateNoSprintFireDelayWeapon(void *localPawnInstance)
    {
        void *held = nullptr;
        if (Config.NewFeaturesMenu.NoSprintToFireDelay &&
            A_FireObjectAlive(localPawnInstance)) {
            Weapon *weapon = ((Pawn *)localPawnInstance)->get_CurrentWeapon();
            if (A_FireObjectAlive((void *)weapon))
                held = (void *)weapon;
        }
        g_noSprintFireDelayWeapon.store(held, std::memory_order_relaxed);
    }

    inline bool NoSprintFireDelayActiveFor(void *weaponInstance, void *cachedLocalWeapon)
    {
        return weaponInstance != nullptr && weaponInstance == cachedLocalWeapon;
    }

    inline bool NoSprintFireDelayActive(void *weaponInstance)
    {
        if (!Config.NewFeaturesMenu.NoSprintToFireDelay) return false;
        return NoSprintFireDelayActiveFor(
            weaponInstance, g_noSprintFireDelayWeapon.load(std::memory_order_relaxed));
    }

    inline void InstallHooks()
    {
        InstallTriggerbotHooks();

        LOGI("[NewFeatures] hooks installed: Pawn_Tick=%p Weapon_StartFire=%p Weapon_StopFire=%p "
             "(NoHipfireDelay + NoSlideFatigue are 8-byte patches owned by Main.cpp)",
             (void *)orig_Pawn_Tick,
             (void *)fn_Weapon_StartFire,
             (void *)fn_Weapon_StopFire);
    }
}

