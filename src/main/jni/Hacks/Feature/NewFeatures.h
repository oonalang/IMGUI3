#pragma once

#include <random>
#include <chrono>
#include <atomic>

#include <cstdint>
#include <cstring>
#include <cmath>

#include "../../IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "../../IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "../../IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "../../IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "../../foxcheats/include/ScanEngine.hpp"
#include "../../Includes/Utils.h"
#include "../../Includes/Macros.h"
#include "../../System/Core/FaultGuard.h"
#include "../StructGame/StructGame.h"
#include "AimLock.h"

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
        // The muzzle the bullet actually leaves. LOS is checked from here, not
        // from the eye, so peeking over cover (a head glitch) no longer reads
        // your own ledge as blocking the shot.
        constexpr uintptr_t Pawn_get_FirePosition           = 0x51B8900;
        constexpr uintptr_t Pawn_get_LastPawnPos            = 0x51B8D30;
        constexpr uintptr_t Camera_get_main                 = 0x5024010;
        constexpr uintptr_t Camera_WorldToScreenPoint       = 0x5023B54;

        constexpr uintptr_t Camera_ScreenToWorldPoint       = 0x5023D34;
        constexpr uintptr_t Physics_RaycastLOS              = 0x506C1E4;

        constexpr uintptr_t Physics_RaycastHitOut            = 0x506C188;
        constexpr uintptr_t Physics_RaycastSegment           = 0x506BF60;

        constexpr uintptr_t Physics_RaycastAll               = 0x506CBB0;

        constexpr uintptr_t Collider_IsTrigger               = 0x50279D4;
        constexpr uintptr_t Object_InstanceIDToObject        = 0x5068274;

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

        // Weapon categories live on Inventory, the base class of Weapon, so the
        // held weapon can be asked directly. There is no getter for SMG or LMG
        // in this build; both are full auto and share the "auto" behaviour.
        constexpr uintptr_t Inventory_IsGun                 = 0xC364D3C;
        constexpr uintptr_t Inventory_IsAssaultRifle         = 0xC364C74;
        constexpr uintptr_t Inventory_IsSniper               = 0xC364C94;
        constexpr uintptr_t Inventory_IsMarksmanRifle        = 0xC364CD4;
        constexpr uintptr_t Inventory_IsShotGun              = 0xC364CB4;
        constexpr uintptr_t Inventory_IsPistol               = 0xC364C10;
        constexpr uintptr_t Inventory_IsLauncher             = 0xC364C54;
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
    inline void *(*fn_ObjectInstanceIDToObject)(int32_t) = nullptr;

    inline Vector3 (*fn_TransformGetForward)(void *) = nullptr;
    inline void   *(*fn_TransformGetParent)(void *) = nullptr;

    inline Vector3 (*fn_TransformGetRight)(void *) = nullptr;
    inline Vector3 (*fn_TransformGetUp)(void *) = nullptr;
    inline void *(*fn_CameraGetMain)() = nullptr;

    inline bool A_FireVectorFinite(const Vector3 &v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    inline bool A_FireWorldToScreenSafe(void *cam, const Vector3 &world, Vector3 &out)
    {
        if (fn_WorldToScreen == nullptr || !A_FireObjectAlive(cam) ||
            !A_FireVectorFinite(world)) return false;
        out = fn_WorldToScreen(cam, world);
        return A_FireVectorFinite(out);
    }
    inline void *(*fn_GameGetMatchGame)() = nullptr;
    inline bool (*fn_PawnRecentlyRendered)(void *) = nullptr;
    inline bool (*fn_PawnLastOccVisible)(void *) = nullptr;    inline bool   (*fn_PawnGetIsFiring)(void *) = nullptr;
    inline Vector3 (*fn_PawnGetFirePosition)(void *) = nullptr;


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
    inline bool  (*fn_InventoryIsGun)(void *) = nullptr;
    inline bool  (*fn_InventoryIsAssaultRifle)(void *) = nullptr;
    inline bool  (*fn_InventoryIsSniper)(void *) = nullptr;
    inline bool  (*fn_InventoryIsMarksmanRifle)(void *) = nullptr;
    inline bool  (*fn_InventoryIsShotGun)(void *) = nullptr;
    inline bool  (*fn_InventoryIsPistol)(void *) = nullptr;
    inline bool  (*fn_InventoryIsLauncher)(void *) = nullptr;
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

        // The bullet leaves the weapon, not the eye. A ray from the eye point
        // hits the cover you are peeking over at ~0 m whenever the camera sits
        // inside or on that cover - the "head glitch" case - and every shot
        // came back as "wall in the way". Start where the muzzle really is.
        static constexpr float kRayStem         = 0.30f;
        static constexpr float kRayStepPast     = 0.05f;
        static constexpr float kHeadRadius      = 0.14f;
        static constexpr float kBodyRadius      = 0.30f;

        static constexpr float kBodyHalfWidthM  = 0.32f;
        static constexpr float kFriendlyRadius  = 0.35f;
        static constexpr float kSelfExclusion    = 0.15f;

        // No practical range cap: 1.5 km is past every sightline in the map, so
        // a distant target is never refused for being distant.
        static constexpr float kCrosshairRayMax = 1500.0f;
        static constexpr float kMaxTravelTime   = 3.00f;
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

    // The weapon family, as the game itself reports it. Tap vs hold and the log
    // panel key off this instead of guessing from the fire interval.
    enum class WeaponClass : int
    {
        Unknown = 0,
        AssaultRifle,
        Sniper,
        Marksman,
        Shotgun,
        AutoRifle,      // SMG / LMG: no getter in this build, both are full auto
        Pistol,
        Launcher,
        Melee,
        Other
    };

    inline const char *WeaponClassName(WeaponClass c)
    {
        switch (c)
        {
            case WeaponClass::AssaultRifle: return "AR";
            case WeaponClass::Sniper:       return "sniper";
            case WeaponClass::Marksman:     return "marksman";
            case WeaponClass::Shotgun:      return "shotgun";
            case WeaponClass::AutoRifle:    return "SMG/LMG";
            case WeaponClass::Pistol:       return "pistol";
            case WeaponClass::Launcher:     return "launcher";
            case WeaponClass::Melee:        return "melee";
            case WeaponClass::Other:        return "other";
            default:                        return "unknown";
        }
    }

    inline bool A_FireInventoryIs(bool (*fn)(void *), void *weapon)
    {
        return fn != nullptr && weapon != nullptr && fn(weapon);
    }

    inline WeaponClass GetWeaponClass(void *weapon)
    {
        if (!Tools::IsPtrValid(weapon)) return WeaponClass::Unknown;
        if (A_FireInventoryIs(fn_InventoryIsSniper, weapon))        return WeaponClass::Sniper;
        if (A_FireInventoryIs(fn_InventoryIsMarksmanRifle, weapon)) return WeaponClass::Marksman;
        if (A_FireInventoryIs(fn_InventoryIsShotGun, weapon))       return WeaponClass::Shotgun;
        if (A_FireInventoryIs(fn_InventoryIsPistol, weapon))        return WeaponClass::Pistol;
        if (A_FireInventoryIs(fn_InventoryIsLauncher, weapon))      return WeaponClass::Launcher;
        if (A_FireInventoryIs(fn_InventoryIsAssaultRifle, weapon))  return WeaponClass::AssaultRifle;
        if (fn_InventoryIsGun == nullptr) return WeaponClass::Unknown;
        return A_FireInventoryIs(fn_InventoryIsGun, weapon) ? WeaponClass::AutoRifle
                                                           : WeaponClass::Other;
    }

    // A sniper, marksman, shotgun or pistol is one press per shot even when the
    // game reports auto fire: holding those triggers is what turns a semi
    // weapon into a fire-rate glitch the server can see.
    inline bool A_FireClassHolds(WeaponClass cls)
    {
        switch (cls)
        {
            case WeaponClass::Sniper:
            case WeaponClass::Marksman:
            case WeaponClass::Shotgun:
            case WeaponClass::Pistol:
            case WeaponClass::Launcher:
            case WeaponClass::Melee:
            case WeaponClass::Other:
                return false;
            default:
                return true;      // AR / SMG-LMG: follow the weapon's fire mode
        }
    }

    inline bool A_FireShouldHoldTrigger(void *weapon, FireMode mode)
    {
        if (!A_FireClassHolds(GetWeaponClass(weapon))) return false;
        return mode == FireMode::Auto || mode == FireMode::Burst;
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

    inline Vector3 A_FireClosestBodyPoint(const A_FireAimPoint &aim,
                                          const Vector3 &screenHead, const Vector3 &screenFeet,
                                          float cx, float cy)
    {
        if (!(screenHead.z > 0.0f && screenFeet.z > 0.0f)) return aim.pos;
        const float dx = screenFeet.x - screenHead.x;
        const float dy = screenFeet.y - screenHead.y;
        const float lenSq = dx * dx + dy * dy;
        float u = lenSq > 0.0001f
            ? ((cx - screenHead.x) * dx + (cy - screenHead.y) * dy) / lenSq : 0.0f;
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        const float headWeight = (1.0f - u) / screenHead.z;
        const float feetWeight = u / screenFeet.z;
        const float total = headWeight + feetWeight;
        if (!(total > 0.0f) || !std::isfinite(total)) return aim.pos;
        return (aim.head * headWeight + aim.feet * feetWeight) / total;
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
        return fn_WeaponInSwitchWeaponState != nullptr && fn_WeaponInSwitchWeaponState(weapon);
    }

    inline bool A_FireWeaponHasAmmo(void *weapon)
    {
        if (!A_FireObjectAlive(weapon) || fn_WeaponAmmoCanFire == nullptr ||
            !fn_WeaponAmmoCanFire(weapon))
            return false;

        const bool gun = fn_InventoryIsGun == nullptr || fn_InventoryIsGun(weapon);
        return !gun || (fn_WeaponGetClipAmmoCount != nullptr &&
                        fn_WeaponGetClipAmmoCount(weapon) > 0);
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

    // ------------------------------------------------------------------
    // Fault guard for the reads the triggerbot makes into enemy state.
    //
    // A pawn that is spawning or despawning leaves pointers that are still
    // inside mapped pages, so IsPtrValid() answers "fine" and the game's own
    // getter then walks the torn object and faults. That is what killed the
    // process the moment an enemy appeared: the scan touched one bad pawn -
    // or one bad collider on the way to it - and the fault took the game with
    // it. Every read into a target now runs under fault_guard, the pattern the
    // skeleton ESP already uses for the same failure: a fault costs that one
    // call, the pawn or weapon is skipped for 1.5 s, and the stage is logged
    // so a repeat report names the call that faulted instead of "POOF".
    // ------------------------------------------------------------------
    struct A_FireFaultEntry
    {
        uintptr_t key;
        double    until;
    };
    inline A_FireFaultEntry g_afireFaults[16] = {};
    inline int              g_afireFaultNext = 0;
    inline std::atomic<int> g_afireFaultCount{0};
    inline const char *g_afireLastFaultStage = "idle";
    inline double      g_afireLastFaultTime  = -1000.0;

    inline bool A_FireFaultCooldownActive(uintptr_t key)
    {
        if (key == 0) return false;
        const double now = A_FireNow();
        for (int i = 0; i < 16; i++)
            if (g_afireFaults[i].key == key && now < g_afireFaults[i].until)
                return true;
        return false;
    }

    inline void A_FireFaultStartCooldown(uintptr_t key)
    {
        g_afireFaults[g_afireFaultNext].key   = key;
        g_afireFaults[g_afireFaultNext].until = A_FireNow() + 1.5;
        g_afireFaultNext = (g_afireFaultNext + 1) % 16;
    }

    template <typename Fn>
    inline bool A_FireGuardedRun(const char *stage, uintptr_t key, Fn &&fn)
    {
        if (A_FireFaultCooldownActive(key)) return false;
        const bool ok = fault_guard::Run([&] { fn(); });
        if (!ok) {
            A_FireFaultStartCooldown(key);
            g_afireFaultCount.fetch_add(1, std::memory_order_relaxed);
            const double now = A_FireNow();
            if (stage != g_afireLastFaultStage || now - g_afireLastFaultTime >= 5.0) {
                LOGE("[NewFeatures] triggerbot: native fault during '%s' (key %p) - skipped for 1.5s",
                     stage, (void *)key);
                g_afireLastFaultStage = stage;
                g_afireLastFaultTime  = now;
            }
        }
        return ok;
    }

    constexpr double kAdsSettleSeconds    = 0.10;

    // Pawn.IsAiming on a pawn that is mid-respawn walks a torn object the same
    // way the enemy getters do. Every ADS read goes through here so a fault
    // reads as "not aiming" - which refuses the shot instead of killing the
    // process on the look-at-enemy tick.
    inline bool A_FirePawnIsAimingSafe(Pawn *localPawn)
    {
        if (fn_PawnIsAiming == nullptr || !A_FirePawnCallable(localPawn))
            return false;
        bool aiming = false;
        A_FireGuardedRun("ads state", (uintptr_t)localPawn, [&] {
            aiming = fn_PawnIsAiming((void *)localPawn);
        });
        return aiming;
    }
    constexpr double kAdsSettleMaxSeconds = 0.35;
    inline double    g_afireAdsSettleUntil = -1000.0;
    inline double    g_afireAdsSettleMax   = -1000.0;
    inline int       g_afireAdsLastState   = -1;
    inline void     *g_afireAdsLastWeapon   = nullptr;
    inline bool      g_afireAdsWeaponSeen   = false;
    inline bool      g_afireAdsSettling     = false;
    inline bool      g_afireAdsTracePending = false;
    inline bool      g_afireAdsFireTracePending = false;
    inline thread_local bool g_afireAdsTraceActive = false;

    inline void A_FireAdsTrace(const char *stage)
    {
        if (g_afireAdsTraceActive) LOGI("[NewFeatures][ADS] %s", stage);
    }

    inline bool A_FirePressTrigger(void *weapon)
    {
        if (fn_Weapon_StartFire == nullptr || !A_FireObjectAlive(weapon) ||
            A_FireWeaponSwitching(weapon) || !A_FireWeaponHasAmmo(weapon))
            return false;

        const bool trace = g_afireAdsTraceActive ||
            (Config.NewFeaturesMenu.TriggerbotMode == 1 && g_afireAdsFireTracePending);
        if (trace) LOGI("[NewFeatures][ADS] start trigger weapon=%p", weapon);
        const bool started = A_FireGuardedRun("start fire", (uintptr_t)weapon, [&] {
            fn_Weapon_StartFire(weapon);
        });
        if (!started) return false;
        if (trace) {
            LOGI("[NewFeatures][ADS] start complete");
            g_afireAdsFireTracePending = false;
        }
        return true;
    }

    struct ScopeSettle
    {
        bool settling;
        bool asked;
        bool aiming;
        void* comp;
    };

    inline bool A_FireAdsSwapActive()
    {
        return g_afireAdsSettling && A_FireNow() < g_afireAdsSettleMax;
    }

    inline ScopeSettle A_FireScopingTransition(void *weapon, Pawn *localPawn)
    {
        ScopeSettle s{false, false, false, nullptr};
        const double now = A_FireNow();
        bool moved = false;

        if (fn_PawnIsAiming != nullptr && A_FirePawnCallable(localPawn)) {
            s.asked  = true;
            s.aiming = A_FirePawnIsAimingSafe(localPawn);
            const int state = s.aiming ? 1 : 0;
            if (state != g_afireAdsLastState) {
                g_afireAdsTracePending = s.aiming;
                g_afireAdsFireTracePending = s.aiming;
            }
            if (g_afireAdsLastState != -1 && state != g_afireAdsLastState) moved = true;
            g_afireAdsLastState = state;
        } else {
            g_afireAdsLastState = -1;
        }

        if (g_afireAdsWeaponSeen && weapon != g_afireAdsLastWeapon) moved = true;
        g_afireAdsLastWeapon = weapon;
        g_afireAdsWeaponSeen = true;

        if (moved && !g_afireAdsSettling) {
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

        if (!A_FireWeaponCanFire(weapon)) {
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
            r.pawnAim = A_FirePawnIsAimingSafe(localPawn);
        }

        r.unknown = !r.pawnOk;
        r.aiming = r.pawnOk && r.pawnAim;
        return r;
    }

    constexpr double kAdsGateRefreshSeconds = 0.012;
    inline AdsRead   g_afireAdsNegCached{};
    inline double    g_afireAdsNegTime     = -1000.0;
    inline void     *g_afireAdsNegWeapon   = nullptr;
    inline void     *g_afireAdsNegComp     = nullptr;
    inline bool      g_afireAdsNegValid    = false;

    inline AdsRead A_FireAdsGateTick(void *weapon, Pawn *localPawn, const ScopeSettle &settle)
    {
        if (settle.asked && settle.aiming)
            return A_FireAdsGate(weapon, localPawn, settle);

        const double now = A_FireNow();
        if (g_afireAdsNegValid && g_afireAdsNegWeapon == weapon &&
            g_afireAdsNegComp == settle.comp &&
            (now - g_afireAdsNegTime) < kAdsGateRefreshSeconds)
            return g_afireAdsNegCached;

        const AdsRead r = A_FireAdsGate(weapon, localPawn, settle);
        if (r.aiming) {
            g_afireAdsNegValid = false;
        } else {
            g_afireAdsNegCached = r;
            g_afireAdsNegTime   = now;
            g_afireAdsNegWeapon = weapon;
            g_afireAdsNegComp   = settle.comp;
            g_afireAdsNegValid  = true;
        }
        return r;
    }

    inline float A_FireWeaponInterval(void *weapon)
    {
        float interval = 0.0f;
        if (fn_WeaponRealFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
            A_FireGuardedRun("fire interval", (uintptr_t)weapon, [&] {
                const float v = fn_WeaponRealFireInterval(weapon);
                if (v > 0.0f && v < 10.0f) interval = v;
            });
        }
        if (interval <= 0.0f) {
            static auto getFireInterval = (float (*)(void *))UnityRva(RVA::Weapon_get_FireInterval);
            if (getFireInterval != nullptr && Tools::IsPtrValid(weapon)) {
                A_FireGuardedRun("fire interval", (uintptr_t)weapon, [&] {
                    const float v = getFireInterval(weapon);
                    if (v > 0.0f && v < 10.0f) interval = v;
                });
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
                if (!(along > 0.0f && along < targetDist)) continue;
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
        int    faults;
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

        float  zonePx;
        int    weaponClass;
        bool   holdTrigger;
        bool   reactionWaiting;
        float  reactionSeconds;
    };
    inline TriggerbotDiag g_triggerDiag{};

    // Reaction delay. A press on the very tick the target entered the zone is
    // the one timing a human cannot produce, and the simplest thing a server
    // can flag. Each engagement rolls its own delay; 0 ms on the slider keeps
    // the old instant trigger.
    inline void   *g_afireSeenTarget   = nullptr;
    inline double  g_afireSeenStamp    = -1000.0;
    inline double  g_afireReactionRoll = 0.0;
    constexpr double kFireSeenGraceSeconds = 0.25;

    inline double A_FireRollReaction()
    {
        int ms = Config.NewFeaturesMenu.TriggerbotDelay;
        if (ms <= 0) return 0.0;
        if (ms > 500) ms = 500;

        // +-12% around the slider, plus 8..28 ms of scatter, so two engagements
        // never press on the same millisecond.
        const double base    = (double)ms * 0.001;
        const double scale   = 0.88 + (double)(rng() % 250) / 1000.0;
        const double scatter = 0.008 + (double)(rng() % 2000) / 100000.0;
        return base * scale + scatter;
    }

    struct WeaponTraits
    {
        WeaponClass cls;
        FireMode    mode;
        bool        holds;
    };

    // Classifying costs up to seven managed calls, and a weapon does not turn
    // into a different weapon, so the answer is remembered per held weapon.
    inline WeaponClass g_afireClassCache  = WeaponClass::Unknown;
    inline void       *g_afireTraitWeapon = nullptr;
    inline double      g_afireTraitStamp  = -1000.0;
    constexpr double   kWeaponTraitSeconds = 0.50;

    inline WeaponTraits A_FireWeaponTraits(void *weapon)
    {
        WeaponTraits t{ WeaponClass::Unknown, FireMode::Unknown, false };
        if (!Tools::IsPtrValid(weapon)) return t;

        const double now = A_FireNow();
        if (g_afireTraitWeapon != weapon || (now - g_afireTraitStamp) > kWeaponTraitSeconds) {
            g_afireClassCache  = GetWeaponClass(weapon);
            g_afireTraitWeapon = weapon;
            g_afireTraitStamp  = now;
        }

        // The fire mode can change under the same weapon (fire selector,
        // attachment), so it is asked every scan - one managed call.
        t.cls   = g_afireClassCache;
        t.mode  = GetWeaponFireMode(weapon);
        t.holds = A_FireShouldHoldTrigger(weapon, t.mode);
        return t;
    }

    inline void *(*fn_GameGetAttackableColliderMap)() = nullptr;
    inline void *(*fn_AtkMapGetTargetForCollider)(void *, void *) = nullptr;

    inline std::atomic<int> s_dbgTriggerState = 0;

    inline Pawn  *g_afireLocalPawnCache     = nullptr;
    inline double g_afireLocalPawnCacheTime = -1000.0;
    inline void  *g_afireLastLocalPawn      = nullptr;
    inline void  *g_afireLastMatchGame      = nullptr;
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

    // LOS is cast from the muzzle, because that is what the bullet uses. When
    // you peek over cover (a head glitch) your eye is still inside the ledge but
    // the muzzle is past it, so an eye-origin ray reads your own cover as
    // blocking and refuses every shot. Falls back to a short stem along the aim
    // from the eye when the muzzle is unavailable or implausible (stale read).
    inline Vector3 A_FireRayOrigin(Pawn *localPawn, const Vector3 &eye,
                                   const Vector3 &aimDir, float fallbackStem)
    {
        if (fn_PawnGetFirePosition != nullptr && A_FireObjectAlive((void *)localPawn)) {
            Vector3 muzzle = Vector3::zero();
            const bool muzzleSafe = A_FireGuardedRun("muzzle position", (uintptr_t)localPawn, [&] {
                muzzle = fn_PawnGetFirePosition(localPawn);
            });
            if (muzzleSafe && (muzzle.x != 0.0f || muzzle.y != 0.0f || muzzle.z != 0.0f)) {
                const float dx = muzzle.x - eye.x;
                const float dy = muzzle.y - eye.y;
                const float dz = muzzle.z - eye.z;
                if (dx * dx + dy * dy + dz * dz <= 4.0f)   // within 2 m of the eye
                    return muzzle;
            }
        }
        const float alen = Vector3::Magnitude(aimDir);
        if (alen > 1.0e-4f) {
            const float s = fallbackStem / alen;
            return Vector3(eye.x + aimDir.x * s,
                           eye.y + aimDir.y * s,
                           eye.z + aimDir.z * s);
        }
        return eye;
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
        if (fn_TransformGetForward == nullptr || camTransform == nullptr) {
            out = Vector3::zero();
            plainCentre = true;
            return;
        }
        if (!A_FireGuardedRun("camera forward", (uintptr_t)camTransform, [&] {
                out = fn_TransformGetForward(camTransform);
            })) {
            out = Vector3::zero();
            plainCentre = true;
            return;
        }

        float cx = 0.0f, cy = 0.0f;
        CrosshairCentrePx(cx, cy);

        if (fn_ScreenToWorldPoint != nullptr && A_FireObjectAlive(cam)) {
            Vector3 world = Vector3::zero();
            if (!A_FireGuardedRun("crosshair projection", (uintptr_t)cam, [&] {
                    world = fn_ScreenToWorldPoint(cam, Vector3(cx, cy, A_FireTune::kCrosshairRayMax));
                })) {
                plainCentre = true;
                return;
            }
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
                Vector3 right = Vector3::zero(), up = Vector3::zero();
                if (!A_FireGuardedRun("camera axes", (uintptr_t)camTransform, [&] {
                        right = fn_TransformGetRight(camTransform);
                        up    = fn_TransformGetUp(camTransform);
                    }))
                    return;
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

    // How far off the aim point, in screen pixels, the crosshair may be and the
    // triggerbot still fires. The crosshair dot used to be the whole zone, so a
    // small dot refused shots that were a few pixels off the body. The trigger
    // size setting (px at 1080p, scaled to the frame) is a floor now, and each
    // candidate is additionally allowed its own projected body width.
    inline float A_FireZoneRadiusPx()
    {
        float radius = A_FireCrosshairRadiusPx();

        const int h = get_height();
        if (h > 0) {
            float configured = (float)Config.NewFeaturesMenu.TriggerbotRadius * (float)h / 1080.0f;
            if (configured > radius) radius = configured;
        }

        const float w = (float)get_width();
        if (w > 0.0f && radius > w * 0.5f) radius = w * 0.5f;
        if (!(radius > 1.0f)) radius = 1.0f;
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
        static int lastLogged = -1;
        static double lastLogTime = -1000.0;
        const double now = A_FireNow();
        if (Config.NewFeaturesMenu.Triggerbot &&
            (idx != lastLogged || now - lastLogTime >= 2.0)) {
            LOGI("[NewFeatures] triggerbot: block=%s mode=%d dot=%d ammo=%d pawn=%p weapon=%p "
                 "localTicks=%d heldTicks=%d scans=%d targets=%d fires=%d",
                 A_FireBlockText(b), Config.NewFeaturesMenu.TriggerbotMode,
                 (int)Config.NewFeaturesMenu.TriggerbotDotRay, (int)d.ammoOk,
                 d.localPawn, d.heldWeapon, d.pawnLocalTicks, d.weaponHeldTicks,
                 d.scans, d.targets, d.fires);
            lastLogged = idx;
            lastLogTime = now;
        }
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
        uintptr_t raw = 0;
        std::memcpy(&raw, hitBuf + AF_RAYHIT_COLLOFF, sizeof raw);
        if (raw > UINT32_MAX) {
            if ((raw & (alignof(void *) - 1)) != 0) return nullptr;
            void *collider = (void *)raw;
            return A_FireObjectAlive(collider) ? collider : nullptr;
        }
        int32_t instanceId = 0;
        std::memcpy(&instanceId, hitBuf + AF_RAYHIT_COLLOFF, sizeof instanceId);
        if (instanceId == 0 || fn_ObjectInstanceIDToObject == nullptr) return nullptr;
        void *collider = fn_ObjectInstanceIDToObject(instanceId);
        return A_FireObjectAlive(collider) ? collider : nullptr;
    }

    inline void *A_FireColliderTransform(void *collider)
    {
        if (!A_FireObjectAlive(collider)) return nullptr;
        return (void *)((Component *)collider)->get_transform();
    }

    inline void *A_FireTargetForCollider(void *collider)
    {
        if (fn_AtkMapGetTargetForCollider == nullptr || !A_FireObjectAlive(collider))
            return nullptr;
        void *map = fn_GameGetAttackableColliderMap ? fn_GameGetAttackableColliderMap() : nullptr;
        if (!A_FireObjectAlive(map)) return nullptr;
        A_FireAdsTrace("collider owner lookup");
        void *target = fn_AtkMapGetTargetForCollider(map, collider);
        return A_FireObjectAlive(target) ? target : nullptr;
    }

    inline bool A_FireHitIsTarget(void *collider, void *hitTransform, Pawn *pawn,
                                 int *depthOut = nullptr)
    {
        if (depthOut != nullptr) *depthOut = -1;
        if (!A_FireObjectAlive(hitTransform) || !A_FireObjectAlive((void *)pawn)) return false;

        void *target = A_FireTargetForCollider(collider);
        if (target != nullptr) {
            if (target == (void *)pawn && depthOut != nullptr) *depthOut = 0;
            return target == (void *)pawn;
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

        return false;
    }

    inline bool A_FireGameSaysVisible(Pawn *pawn)
    {
        if (!A_FireObjectAlive((void *)pawn)) return false;
        if (fn_PawnLastOccVisible != nullptr && fn_PawnLastOccVisible(pawn)) return true;
        if (fn_PawnRecentlyRendered != nullptr && fn_PawnRecentlyRendered(pawn)) return true;
        return false;
    }

    // How many self/trigger colliders a single LOS ray may step past before it
    // gives up. 64 was pure worst-case waste - a real line rarely clips more
    // than a handful - and every extra step is another managed raycast on the
    // tick. Capped low to keep the per-tick cost down.
    constexpr int AF_MAX_RAY_HITS = 12;

    inline AF_Ray A_FireRayToTargetSingleHit(const Vector3 &start, const Vector3 &dir,
                                             float reach, Pawn *target, Pawn *localPawn,
                                             bool *identified, float *hitDistance)
    {
        if (fn_PhysicsRaycastHitOut == nullptr) return AF_Ray::Unknown;

        Vector3 origin = start;
        float travelled = 0.0f;
        float remaining = reach;
        for (int attempt = 0; attempt < AF_MAX_RAY_HITS && remaining > 0.0f; attempt++) {
            alignas(void *) unsigned char hitBuf[AF_RAYHIT_SIZE] = {};
            if (!fn_PhysicsRaycastHitOut(origin, dir, hitBuf, remaining))
                return AF_Ray::Clear;

            const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
            if (!(hd >= 0.0f && hd <= remaining)) return AF_Ray::Unknown;
            const float totalDistance = travelled + hd;
            if (hitDistance != nullptr) *hitDistance = totalDistance;

            void *collider = A_FireHitCollider(hitBuf);
            void *hitTransform = A_FireColliderTransform(collider);
            if (hitTransform != nullptr && A_FireHitIsTarget(collider, hitTransform, target)) {
                if (identified != nullptr) *identified = true;
                return AF_Ray::Clear;
            }

            const bool self = hitTransform != nullptr && localPawn != nullptr &&
                              A_FireHitIsTarget(collider, hitTransform, localPawn);
            const bool trigger = !self && collider != nullptr &&
                                 fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider);
            if (self || trigger) {
                if (self) g_triggerDiag.selfHitsSkipped++;
                else g_triggerDiag.triggerHitsSkipped++;
                const float step = hd + A_FireTune::kRayStepPast;
                travelled += step;
                remaining -= step;
                origin = Vector3(start.x + dir.x * travelled,
                                 start.y + dir.y * travelled,
                                 start.z + dir.z * travelled);
                if (hitDistance != nullptr) *hitDistance = -1.0f;
                continue;
            }

            if (hitTransform == nullptr) return AF_Ray::Unknown;
            return AF_Ray::Blocked;
        }
        return remaining <= 0.0f ? AF_Ray::Clear : AF_Ray::Unknown;
    }

    inline AF_Ray A_FireRayToTarget(const Vector3 &from, const Vector3 &to, Pawn *target,
                                    float probeRadius, Pawn *localPawn,
                                    bool *identified = nullptr, float *hitDistance = nullptr)
    {
        if (identified != nullptr) *identified = false;
        if (hitDistance != nullptr) *hitDistance = -1.0f;

        if (!A_FireVectorFinite(from) || !A_FireVectorFinite(to))
            return AF_Ray::Unknown;
        const Vector3 from2 = A_FireRayOrigin(localPawn, from,
            Vector3((to.x - from.x), (to.y - from.y), (to.z - from.z)),
            0.0f);

        Vector3 d(to.x - from2.x, to.y - from2.y, to.z - from2.z);
        const float dist = Vector3::Magnitude(d);
        if (!std::isfinite(dist)) return AF_Ray::Unknown;
        if (!(dist >= 0.01f)) return AF_Ray::Clear;
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);

        const AF_Ray singleHit = A_FireRayToTargetSingleHit(from2, dir,
            dist, target, localPawn, identified, hitDistance);
        if (singleHit != AF_Ray::Unknown) return singleHit;
        if (fn_PhysicsRaycastHitOut != nullptr) return AF_Ray::Unknown;

        if (fn_PhysicsRaycastSegment != nullptr) {
            return fn_PhysicsRaycastSegment(from2, dir, dist) ? AF_Ray::Blocked : AF_Ray::Clear;
        }

        return AF_Ray::Unknown;
    }

    inline AF_Ray A_FireRaySegmentClear(const Vector3 &from, const Vector3 &to)
    {
        if (!A_FireVectorFinite(from) || !A_FireVectorFinite(to)) return AF_Ray::Unknown;
        Vector3 d(to.x - from.x, to.y - from.y, to.z - from.z);
        const float dist = Vector3::Magnitude(d);
        if (!std::isfinite(dist)) return AF_Ray::Unknown;
        if (!(dist >= 0.01f)) return AF_Ray::Clear;
        const Vector3 dir(d.x / dist, d.y / dist, d.z / dist);
        if (fn_PhysicsRaycastSegment != nullptr)
            return fn_PhysicsRaycastSegment(from, dir, dist) ? AF_Ray::Blocked : AF_Ray::Clear;
        return AF_Ray::Unknown;
    }

    inline int A_FireRegistryOwnerIs(void *collider, Pawn *pawn)
    {
        void *target = A_FireTargetForCollider(collider);
        if (target == nullptr) return 0;
        return target == (void *)pawn ? 1 : -1;
    }

    inline Pawn *A_FirePawnFromHitTransform(void *collider, void *hitTransform, Pawn **items, int count,
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

        void *target = A_FireTargetForCollider(collider);
        if (target != nullptr) {
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
                                        float *hitDistOut, bool *symbolMissing,
                                        Vector3 *hitPointOut = nullptr)
    {
        if (hitDistOut != nullptr) *hitDistOut = -1.0f;
        if (symbolMissing != nullptr) *symbolMissing = false;
        if (hitPointOut != nullptr) *hitPointOut = Vector3::zero();
        if (!A_FireVectorFinite(camPos)) return nullptr;
        const float len = Vector3::Magnitude(rayDir);
        if (!(len > 0.0001f) || !std::isfinite(len)) return nullptr;
        const Vector3 dir(rayDir.x / len, rayDir.y / len, rayDir.z / len);

        const Vector3 start = camPos;
        const float reach = A_FireTune::kCrosshairRayMax;

        if (fn_PhysicsRaycastHitOut == nullptr) {
            if (symbolMissing != nullptr) *symbolMissing = true;
            return nullptr;
        }

        // Single-hit raycast: a hit on your own body used to end the probe. At
        // point blank (and in a head glitch) your capsule is the first thing
        // under the dot, so step past it and keep looking instead.
        Vector3 origin = start;
        float travelled = 0.0f;   // offset from the muzzle start, like the walk
        float remaining = reach;
        for (int attempt = 0; attempt < AF_MAX_RAY_HITS && remaining > 0.01f; attempt++) {
            alignas(void *) unsigned char hitBuf[AF_RAYHIT_SIZE];
            memset(hitBuf, 0, sizeof hitBuf);
            if (!fn_PhysicsRaycastHitOut(origin, dir, hitBuf, remaining))
                return nullptr;

            const float hd = *(const float *)(hitBuf + AF_RAYHIT_DISTOFF);
            if (!(hd >= 0.0f && hd <= remaining)) return nullptr;
            if (hitDistOut != nullptr) *hitDistOut = travelled + hd;

            void *collider = A_FireHitCollider(hitBuf);
            void *hitTransform = A_FireColliderTransform(collider);
            const bool self = A_FireObjectAlive(hitTransform) && localPawn != nullptr &&
                A_FireHitIsTarget(collider, hitTransform, localPawn);
            const bool trigger = !self && A_FireObjectAlive(collider) &&
                fn_ColliderIsTrigger != nullptr && fn_ColliderIsTrigger(collider);
            if (self || trigger) {
                if (self) g_triggerDiag.selfHitsSkipped++;
                else g_triggerDiag.triggerHitsSkipped++;
                const float step = hd + A_FireTune::kRayStepPast;
                origin = Vector3(origin.x + dir.x * step,
                                 origin.y + dir.y * step,
                                 origin.z + dir.z * step);
                travelled += step;
                remaining -= step;
                continue;
            }
            Pawn *pawn = A_FirePawnFromHitTransform(collider, hitTransform, items, count);
            if (pawn != nullptr && hitPointOut != nullptr)
                *hitPointOut = origin + dir * hd;
            return pawn;
        }
        return nullptr;
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

        A_FireAdsTrace("scan match");
        BaseGame *game = (BaseGame *)fn_GameGetMatchGame();
        if (!Tools::IsPtrValid((void *)game)) return FireBlock::NoMatch;

        A_FireAdsTrace("scan camera");
        Camera *cam = Camera::get_main();
        if (!A_FireObjectAlive((void *)cam)) return FireBlock::NoMatch;
        Transform *camTransform = nullptr;
        Vector3 camPos = Vector3::zero();
        if (!A_FireGuardedRun("camera transform", (uintptr_t)cam, [&] {
                camTransform = cam->get_transform();
                if (A_FireObjectAlive((void *)camTransform))
                    camPos = camTransform->get_position();
            }))
            return FireBlock::NoMatch;
        if (!A_FireObjectAlive((void *)camTransform)) return FireBlock::NoMatch;

        auto w2s = [&](const Vector3 &world, Vector3 &sc) -> bool {
            return A_FireWorldToScreenSafe(cam, world, sc);
        };

        A_FireAdsTrace("scan weapon aim target");
        void *aimTarget = nullptr;
        float aimTargetDis = 0.0f;
        if (fn_WeaponGetAimingTarget != nullptr && Tools::IsPtrValid(weapon)) {
            A_FireGuardedRun("aiming target", (uintptr_t)weapon, [&] {
                aimTarget = fn_WeaponGetAimingTarget(weapon);
                if (fn_WeaponGetAimingTargetDis != nullptr && aimTarget != nullptr)
                    aimTargetDis = fn_WeaponGetAimingTargetDis(weapon);
            });
            d.aimingTarget = (unsigned)(uintptr_t)aimTarget;
            d.aimingTargetDis = aimTargetDis;
        } else {
            d.aimingTarget = 0;
            d.aimingTargetDis = 0.0f;
        }

        A_FireAdsTrace("scan enemy list");
        List<Pawn *> *enemyPawns = nullptr;
        Pawn **items = nullptr;
        int count = 0;
        const bool listSafe = A_FireGuardedRun("enemy list", (uintptr_t)game, [&] {
            enemyPawns = game->EnemyPawns();
            if (Tools::IsPtrValid((void *)enemyPawns)) {
                items = (Pawn **)enemyPawns->getItems();
                count = enemyPawns->getSize();
            }
        });
        d.enemyCount = count;
        if (!listSafe || !Tools::IsPtrValid((void *)items) || count <= 0 || count > 1024)
            return FireBlock::NoEnemyInZone;

        float cx = 0.0f, cy = 0.0f;
        CrosshairCentrePx(cx, cy);
        d.crosshairCx = cx;
        d.crosshairCy = cy;
        const float radius = A_FireZoneRadiusPx();
        d.radiusPx = radius;
        d.zonePx   = radius;

        A_FireAdsTrace("scan camera projection");
        const float focalPx = A_FireFocalPx();
        d.focalPx = focalPx;

        bool honestDotRay = false;
        Vector3 dotRayDir = Vector3::zero();
        if (fn_TransformGetForward != nullptr) {
            A_FireAdsTrace("scan crosshair direction");
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
        const bool dotRayGate = Config.NewFeaturesMenu.TriggerbotDotRay;
        d.dotRayDecided = dotRayGate;
        FireBlock dotBlock = FireBlock::NoEnemyInZone;
        if (fn_TransformGetForward != nullptr && honestDotRay) {
            A_FireAdsTrace("scan crosshair ray");
            bool symbolMissing = false;
            float centreHitDist = -1.0f;
            Vector3 centreHitPoint = Vector3::zero();
            Pawn *centred = nullptr;
            const bool raySafe = A_FireGuardedRun("crosshair ray", (uintptr_t)cam, [&] {
                centred = A_FireCrosshairRayPawn(camPos, dotRayDir, items, count, localPawn,
                    &centreHitDist, &symbolMissing, &centreHitPoint);
            });
            if (!raySafe) {
                d.rayUnknown = true;
                d.dotRayDeclined++;
            }
            d.crosshairHitDist = centreHitDist;
            d.crosshairPawn = (unsigned)(uintptr_t)centred;
            if (symbolMissing) d.rayUnknown = true;
            if (centred != nullptr && centred != localPawn) {
                bool confirmed = false;
                const bool targetSafe = A_FireGuardedRun("crosshair target",
                    (uintptr_t)centred, [&] {
                    if (IsSameTeamAcceptBots(localPawn, centred) || !A_FirePawnAlive(centred))
                        return;
                    bool identified = false;
                    float muzzleHitDist = -1.0f;
                    const AF_Ray los = A_FireRayToTarget(camPos, centreHitPoint, centred,
                        0.0f, localPawn, &identified, &muzzleHitDist);
                    d.lastLosHitDist = muzzleHitDist;
                    if (los == AF_Ray::Clear) {
                        const A_FireAimPoint ap = A_FireAimPointFor(centred);
                        out.pawn = centred;
                        out.aim = ap;
                        out.aimPoint = centreHitPoint;
                        out.screenPos = Vector3::zero();
                        w2s(centreHitPoint, out.screenPos);
                        out.pxOffset = 0.0f;
                        out.worldDist = centreHitDist;
                        out.predicted = centreHitPoint;
                        out.path = 0;
                        out.visible = true;
                        out.visibilityIdentified = true;
                        d.lastAimPx = 0.0f;
                        d.lastWorldDist = centreHitDist;
                        d.aimPath = 0;
                        d.projKeepFrac = 0.0f;
                        d.raysIdentified++;
                        g_afireLosOk = true;
                        d.targets++;
                        confirmed = true;
                        return;
                    }
                    dotBlock = FireBlock::NoLineOfSight;
                    d.dotRayDeclined++;
                    d.rayUnknown = los == AF_Ray::Unknown;
                });
                if (!targetSafe) {
                    dotBlock = FireBlock::NoLineOfSight;
                    d.rayUnknown = true;
                } else if (confirmed) {
                    return FireBlock::Fired;
                }
            }
        }
        // The centre ray is a fast path, not a veto. It used to return here,
        // so a ray that missed the collider - or that could not be cast at all
        // - meant the triggerbot never fired even with an enemy filling the
        // crosshair. The projected candidates below still have to pass their
        // own zone and line-of-sight checks, so falling through only adds the
        // shots the ray could not prove.
        if (dotRayGate && !honestDotRay) d.rayUnknown = true;

        // A centre-ray miss is not proof that the visible body is outside the
        // crosshair. Fall back to projected candidates and LOS validation.
        // This also covers missing raycast symbols and close-range colliders.

        A_FireAdsTrace("scan projected candidates");
        static auto getTeamIdFn = (uint32_t (*)(void *))UnityRva(RVA::Pawn_GetTeamId);
        uint32_t localTeam = 0u;
        if (A_FireObjectAlive((void *)localPawn) && getTeamIdFn != nullptr) {
            // GetTeamId is a managed call like every other read into a pawn;
            // it was the one left outside the guard.
            A_FireGuardedRun("local team", (uintptr_t)localPawn, [&] {
                localTeam = getTeamIdFn(localPawn);
            });
        }
        auto sameTeamGate = [&](Pawn *p) -> bool {
            if (!A_FireObjectAlive((void *)p)) return false;
            if (p == localPawn) return true;
            if (localTeam == 0u || getTeamIdFn == nullptr) return false;
            return getTeamIdFn(p) == localTeam;
        };

        constexpr int MAX_CANDIDATES = 8;
        A_FireCandidate cands[MAX_CANDIDATES];
        int n = 0;

        for (int i = 0; i < count; i++) {
            Pawn *p = items[i];
            if (p == nullptr) continue;

            A_FireGuardedRun("enemy candidate", (uintptr_t)p, [&] {
            if (!A_FireListObjectAlive((void *)p)) return;
            if (p == localPawn) return;
            if (sameTeamGate(p)) return;
            if (!A_FirePawnAlive(p)) return;
            const A_FireAimPoint ap = A_FireAimPointFor(p);
            if (!ap.valid) return;

            float d2 = -1.0f;
            Vector3 scChest = Vector3::zero();
            if (w2s(ap.pos, scChest) && scChest.z > 0.0f) {
                const float dx = scChest.x - cx;
                const float dy = scChest.y - cy;
                d2 = dx * dx + dy * dy;
            }

            // Use the target's projected body width instead of testing only
            // the body centre against a fixed pixel-sized crosshair circle.
            // Distance-aware coverage avoids missing small distant targets.
            const float candDist = Vector3::Distance(camPos, ap.pos);
            const float projectedHalfWidth = (focalPx > 0.0f && candDist > 0.05f)
                ? A_FireTune::kBodyHalfWidthM * focalPx / candDist : 0.0f;
            const float bodyAllowance = projectedHalfWidth > 0.0f
                ? projectedHalfWidth : 0.0f;
            const float allowedRadius = radius + bodyAllowance;
            const float allowSq = allowedRadius * allowedRadius;

            if (d2 >= 0.0f && d2 > allowSq && ap.headOk && ap.feetOk) {
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
            // Partial visibility: the chest and feet can sit behind a ledge
            // while the exposed head still intersects the crosshair.
            if (ap.headOk) {
                Vector3 scHead = Vector3::zero();
                if (w2s(ap.head, scHead) && scHead.z > 0.0f) {
                    const float hx = scHead.x - cx;
                    const float hy = scHead.y - cy;
                    const float headSq = hx * hx + hy * hy;
                    if (d2 < 0.0f || headSq < d2) d2 = headSq;
                }
            }
            if (d2 < 0.0f) return;
            if (d.closestPx < 0.0f || d2 < d.closestPx) d.closestPx = d2;
            if (d2 > allowSq) return;
            const float pxOffset = sqrtf(d2);
            int slot = n;
            if (n >= MAX_CANDIDATES) {
                slot = 0;
                for (int k = 1; k < n; k++)
                    if (cands[k].pxOffset > cands[slot].pxOffset) slot = k;
                if (pxOffset >= cands[slot].pxOffset) return;
            } else {
                n++;
            }
            cands[slot].pawn      = p;
            cands[slot].aim       = ap;
            cands[slot].pxOffset  = pxOffset;
            cands[slot].worldDist = candDist;
            cands[slot].velocity  = GetPawnVelocity(p);
            });
        }
        d.candidates = n;
        if (n <= 0) return FireBlock::NoEnemyInZone;

        AF_FriendProbe friendProbes[64];
        int friendCount = 0;
        A_FireGuardedRun("team probe", (uintptr_t)localPawn, [&] {
            friendCount = A_FireCollectFriendProbes(localPawn, friendProbes, 64);
        });

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
        float spreadPx = 0.0f;
        A_FireGuardedRun("weapon spread", (uintptr_t)weapon, [&] {
            spreadPx = A_FireSpreadPx(weapon, focalPx, checkSpread);
        });
        d.lastSpreadPx = spreadPx;

        bool blockedLos = false, blockedFriend = false;

        bool speedKnown = false;
        A_FireAdsTrace("scan bullet speed");
        float bulletSpeed = A_FireTune::kAssumedSpeed;
        A_FireGuardedRun("bullet speed", (uintptr_t)weapon, [&] {
            bulletSpeed = GetWeaponBulletSpeed(weapon, &speedKnown);
        });

        for (int oi = 0; oi < n; oi++) {
            A_FireAdsTrace("scan line of sight");
            const A_FireCandidate &c = cands[order[oi]];

            Vector3 points[5] = {c.aim.pos, c.aim.head, c.aim.pos, c.aim.pos, c.aim.pos};
            float radii[5] = {c.aim.radius, A_FireTune::kHeadRadius,
                             A_FireTune::kHeadRadius, A_FireTune::kBodyRadius,
                             A_FireTune::kBodyRadius};
            int pointCount = c.aim.headOk ? 2 : 1;
            if (c.aim.headOk && c.aim.feetOk) {
                points[2] = A_FireAlongBody(c.aim.head, c.aim.feet, 0.92f);
                points[3] = A_FireAlongBody(c.aim.head, c.aim.feet, 0.78f);
                pointCount = 4;
                Vector3 screenHead = Vector3::zero(), screenFeet = Vector3::zero();
                if (w2s(c.aim.head, screenHead) && w2s(c.aim.feet, screenFeet) &&
                    screenHead.z > 0.0f && screenFeet.z > 0.0f) {
                    points[4] = A_FireClosestBodyPoint(c.aim, screenHead, screenFeet, cx, cy);
                    const Vector3 axis = c.aim.head - c.aim.feet;
                    const float axisSq = Vector3::Dot(axis, axis);
                    if (axisSq > 0.0001f &&
                        Vector3::Dot(points[4] - c.aim.feet, axis) / axisSq >= 0.9f)
                        radii[4] = A_FireTune::kHeadRadius;
                    pointCount = 5;
                }
            }
            float misses[5];
            for (int pi = 0; pi < pointCount; pi++) {
                misses[pi] = -1.0f;
                Vector3 screen = Vector3::zero();
                if (!w2s(points[pi], screen) || !(screen.z > 0.0f)) continue;
                const float dx = screen.x - cx, dy = screen.y - cy;
                const float miss = sqrtf(dx * dx + dy * dy);
                const float distance = Vector3::Distance(camPos, points[pi]);
                const float hitRadius = focalPx > 0.0f && distance > 0.01f
                    ? radii[pi] * focalPx / distance : 0.0f;
                if (std::isfinite(miss) && miss <= radius + hitRadius) misses[pi] = miss;
            }
            AF_Ray los = AF_Ray::Unknown;
            bool losIdentified = false;
            float losHitDist = -1.0f;
            Vector3 probePoint = c.aim.pos;
            float probeRadius = c.aim.radius;
            bool aligned = false;
            for (int attempt = 0; attempt < pointCount; attempt++) {
                int best = -1;
                for (int pi = 0; pi < pointCount; pi++)
                    if (misses[pi] >= 0.0f && (best < 0 || misses[pi] < misses[best])) best = pi;
                if (best < 0) break;
                misses[best] = -1.0f;
                aligned = true;
                d.losChecked++;
                bool identified = false;
                float hitDistance = -1.0f;
                AF_Ray result = AF_Ray::Unknown;
                A_FireGuardedRun("line of sight", (uintptr_t)c.pawn, [&] {
                    result = A_FireRayToTarget(camPos, points[best], c.pawn,
                        radii[best], localPawn, &identified, &hitDistance);
                });
                if (result == AF_Ray::Blocked) los = AF_Ray::Blocked;
                if (result != AF_Ray::Clear) continue;
                los = result;
                losIdentified = identified;
                losHitDist = hitDistance;
                probePoint = points[best];
                probeRadius = radii[best];
                break;
            }
            if (!aligned) continue;
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

                if (lead) {
                    bool leadClear = false;
                    A_FireGuardedRun("lead path", (uintptr_t)c.pawn, [&] {
                        leadClear = A_FireRayToTarget(camPos, predicted, c.pawn,
                                                      probeRadius, localPawn) == AF_Ray::Clear;
                    });
                    if (!leadClear) {
                        d.leadPathBlocked = true;
                        lead = false;
                        predicted = probePoint;
                    }
                }
            }

            // Recalculate against the selected visible point, not the
            // original (possibly occluded) torso projection.
            float predPx = c.pxOffset;
            Vector3 scProbe = Vector3::zero();
            if (w2s(probePoint, scProbe) && scProbe.z > 0.0f) {
                const float px = scProbe.x - cx;
                const float py = scProbe.y - cy;
                predPx = sqrtf(px * px + py * py);
            }
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

            // The muzzle read (Pawn.get_FirePosition) is the call the old
            // triggerbot did not have; it walks the local pawn and was still
            // outside the guard here. A fault falls back to the eye, which is
            // exactly what the old triggerbot used.
            Vector3 shotOrigin = camPos;
            A_FireGuardedRun("shot origin", (uintptr_t)localPawn, [&] {
                shotOrigin = A_FireRayOrigin(localPawn, camPos,
                    probePoint - camPos, 0.0f);
            });
            Vector3 dirToAim = probePoint - shotOrigin;
            const float dirLen = Vector3::Magnitude(dirToAim);
            if (dirLen > 0.01f) {
                dirToAim = Vector3(dirToAim.x / dirLen, dirToAim.y / dirLen, dirToAim.z / dirLen);
                int friends = 0;
                if (A_FireFriendlyInLine(friendProbes, friendCount, shotOrigin, dirToAim, dirLen, &friends)) {
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
            out.pxOffset   = predPx;
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
        if (dotRayGate && dotBlock == FireBlock::NoLineOfSight) return dotBlock;
        return FireBlock::NoEnemyInZone;
    }

    inline FireBlock A_FireShouldFire(TriggerTarget &out)
    {
        TriggerbotDiag &d = g_triggerDiag;
        out = TriggerTarget{};

        Pawn *localPawn = GamePlay::get_LocalPawn();
        d.localPawn = (void *)localPawn;
        if (!A_FireObjectAlive((void *)localPawn)) return FireBlock::NoLocalPawn;

        bool localAlive = false;
        A_FireGuardedRun("local pawn alive", (uintptr_t)localPawn, [&] {
            localAlive = A_FirePawnAlive(localPawn);
        });
        if (!localAlive) return FireBlock::LocalNotAlive;

        if (fn_GameGetMatchGame == nullptr) return FireBlock::NoMatch;
        void *game = fn_GameGetMatchGame();
        d.matchGame = game;
        if (!Tools::IsPtrValid(game)) return FireBlock::NoMatch;
        if (fn_BaseGameIsPaused != nullptr && fn_BaseGameIsPaused(game))
            return FireBlock::MatchPaused;

        Weapon *held = nullptr;
        if (!A_FireGuardedRun("held weapon", (uintptr_t)localPawn, [&] {
                held = localPawn->get_CurrentWeapon();
            }))
            return FireBlock::NoWeapon;
        d.heldWeapon = (void *)held;
        if (!A_FireObjectAlive((void *)held)) return FireBlock::NoWeapon;

        d.switching = A_FireWeaponSwitching(held);
        if (d.switching) {
            if (!A_FireWeaponSwitching(held))
                d.switching = false;
        }

        bool ammoOk = false, canFire = false;
        A_FireGuardedRun("weapon state", (uintptr_t)held, [&] {
            ammoOk  = A_FireWeaponHasAmmo(held);
            canFire = A_FireWeaponCanFire(held);
        });
        d.ammoOk  = ammoOk;
        d.canFire = canFire;

        WeaponTraits traits{ WeaponClass::Unknown, FireMode::Unknown, false };
        A_FireGuardedRun("weapon class", (uintptr_t)held, [&] {
            traits = A_FireWeaponTraits((void *)held);
        });
        d.weaponClass = (int)traits.cls;
        d.holdTrigger = traits.holds;

        // No ammo, no press. StartFire on an empty clip only plays the dry fire
        // on the client - the server sees nothing - and the trigger then kept
        // pressing at the pace floor. Never ask the game to fire a shot it
        // cannot make.
        if (!d.ammoOk) return FireBlock::NoAmmo;

        const ScopeSettle settle = A_FireScopingTransition((void *)held, localPawn);
        if (settle.settling) return FireBlock::AdsTransition;

        if (Config.NewFeaturesMenu.TriggerbotMode == 1) {
            const AdsRead ads = A_FireAdsGateTick((void *)held, localPawn, settle);
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

        g_afireAdsTraceActive = Config.NewFeaturesMenu.TriggerbotMode == 1 &&
                               g_afireAdsTracePending;
        if (g_afireAdsTraceActive) g_afireAdsTracePending = false;
        const FireBlock result = A_FireScan(localPawn, held, out);
        A_FireAdsTrace("scan complete");
        return result;
    }

    inline uintptr_t A_FireFindTarget()
    {
        TriggerTarget target{};
        if (A_FireShouldFire(target) != FireBlock::Fired) return 0;
        return target.pawn ? (uintptr_t)target.pawn : 0;
    }

    inline bool A_FireStillHoldsWeapon(void *weapon)
    {
        Pawn *localPawn = A_FireLocalPawnFast();
        if (!A_FireObjectAlive((void *)localPawn)) return false;
        bool holds = false;
        if (!A_FireGuardedRun("held weapon", (uintptr_t)localPawn, [&] {
                holds = (void *)localPawn->get_CurrentWeapon() == weapon;
            }))
            return false;
        return holds;
    }

    inline void A_FireReleaseTrigger(void *instance)
    {
        if (!g_afireHoldingFire) return;
        if (instance == nullptr || instance == g_afireFiringOn) {
            // A weapon pointer outlives its object between matches and on a
            // respawn: the memory stays mapped, so IsPtrValid still says yes and
            // StopFire was called into a destroyed weapon. Only release the
            // trigger of a weapon the local pawn is holding right now.
            if (fn_Weapon_StopFire != nullptr &&
                A_FireStillHoldsWeapon(g_afireFiringOn) &&
                A_FireObjectAlive(g_afireFiringOn)) {
                A_FireAdsTrace("release trigger");
                A_FireGuardedRun("stop fire", (uintptr_t)g_afireFiringOn, [&] {
                    fn_Weapon_StopFire(g_afireFiringOn, false);
                });
                A_FireAdsTrace("release complete");
            }
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

        if (!A_FirePressTrigger(held)) {
            A_FireReportBlock(A_FireWeaponHasAmmo(held)
                ? FireBlock::WeaponNotReady : FireBlock::NoAmmo);
            d.testFireStatus = 3;
            return;
        }
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

        if (now - g_afireLastEvalTime < (1.0 / 60.0)) return;
        g_afireLastEvalTime = now;
        if (g_afireInTick) return;
        g_afireInTick = true;
        g_afireAdsTraceActive = false;

        if (g_testFireRequest) A_FireRunTestFire();

        d.hookCalls++;
        d.scans++;
        d.faults = g_afireFaultCount.load(std::memory_order_relaxed);

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
            if (block == FireBlock::NoAmmo) {
                g_afireReleasePending = false;
                A_FireReleaseTrigger(nullptr);
            } else {
                A_FireReleaseTriggerDeferred(nullptr);
            }
            // The engagement is over; a fresh one rolls a fresh reaction. The
            // grace keeps a target that flickers in and out of the zone from
            // restarting the timer on every tick.
            if ((now - g_afireSeenStamp) > kFireSeenGraceSeconds) {
                g_afireSeenTarget   = nullptr;
                g_afireReactionRoll = 0.0;
            }
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

        const WeaponTraits traits   = A_FireWeaponTraits((void *)held);
        const float        interval = A_FireWeaponInterval(held);
        d.lastFireInterval = interval;
        d.weaponClass      = (int)traits.cls;
        d.holdTrigger      = traits.holds;

        // Human reaction: a fresh target is never pressed on the tick it
        // entered the zone. The roll is per engagement, and 0 ms on the slider
        // keeps the old instant trigger.
        if (g_afireSeenTarget != (void *)target.pawn) {
            g_afireSeenTarget   = (void *)target.pawn;
            g_afireSeenStamp    = now;
            g_afireReactionRoll = A_FireRollReaction();
        }
        d.reactionSeconds = (float)g_afireReactionRoll;
        d.reactionWaiting = false;
        if (g_afireReactionRoll > 0.0 &&
            (now - g_afireSeenStamp) < g_afireReactionRoll) {
            if (g_afireHoldingFire) A_FireReleaseTriggerDeferred(nullptr);
            d.reactionWaiting = true;
            s_dbgTriggerState = 1;
            g_afireInTick = false;
            return;
        }

        const bool edgePress = !traits.holds;

        if (g_afireHoldingFire && g_afireFiringOn == (void *)held &&
            g_afireTargetPawn == (void *)target.pawn) {
            if (edgePress) {
                A_FireReleaseTrigger(nullptr);
            } else {
                const bool becameReady = d.canFire && !g_afireCanFireLast;
                const bool stale = (now - g_afireLastPressTime) >=
                                   (double)A_FireTune::kReassertPress;
                if (fn_Weapon_StartFire != nullptr && (becameReady || stale)) {
                    if (!A_FirePressTrigger(held)) {
                        A_FireReleaseTrigger(nullptr);
                        A_FireReportBlock(A_FireWeaponHasAmmo(held)
                            ? FireBlock::WeaponNotReady : FireBlock::NoAmmo);
                        s_dbgTriggerState = 0;
                        g_afireInTick = false;
                        return;
                    }
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
            float pace = A_FireRepressPaceSeconds(interval);
            // Human trigger pulls are not a metronome. With a reaction delay set
            // the repress pace is spread a few percent so the press intervals
            // are never a constant.
            if (Config.NewFeaturesMenu.TriggerbotDelay > 0) {
                const float jitter = 0.94f + (float)(rng() % 130) * 0.001f;   // 0.94..1.07
                pace *= jitter;
            }
            const double since   = now - g_afireLastPressTime;
            const bool   tooSoon = since < (double)pace;

            // The pacing floor is a floor. It used to be skipped for as long as
            // Weapon.CanFire() merely said "yes", and CanFire is usually still
            // true right after a press - so a scoped triggerbot pressed and
            // released the trigger on alternating ticks (~125 shots a second),
            // rebuilding the game's fire effects every time. Only the moment
            // CanFire TURNS true (a reload or a bolt finishing) may jump the
            // queue; the pace covers everything else.
            const bool   readyNow    = d.canFire && (fn_WeaponCanFire != nullptr);
            const bool   becameReady = readyNow && !g_afireCanFireLast;
            d.paceSeconds = pace;
            g_afireCanFireLast = d.canFire;
            if (tooSoon && !becameReady) {
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

        if (!A_FirePressTrigger(held)) {
            A_FireReleaseTrigger(nullptr);
            A_FireReportBlock(A_FireWeaponHasAmmo(held)
                ? FireBlock::WeaponNotReady : FireBlock::NoAmmo);
            s_dbgTriggerState = 0;
            g_afireInTick = false;
            return;
        }
        g_afireHoldingFire    = true;
        g_afireFiringOn       = (void *)held;
        g_afireTargetPawn     = (void *)target.pawn;
        g_afireLastPressWeapon = (void *)held;
        g_afireLastPressTime  = now;
        g_afireCanFireLast    = d.canFire;
        d.fires++;
        s_dbgTriggerState = 2;

        LOGI("[NewFeatures] triggerbot: StartFire(held=%p) target=%p class=%s %s "
             "path=%s aim=%.1fpx hit=%.3fm ident=%d dist=%.3f travel=%.4f",
             (void *)held, (void *)target.pawn,
             WeaponClassName(traits.cls), traits.holds ? "hold" : "tap",
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
        g_afireLastMatchGame      = nullptr;

        g_afireAdsSettleUntil  = -1000.0;
        g_afireAdsSettleMax    = -1000.0;
        g_afireAdsLastState    = -1;
        g_afireAdsLastWeapon   = nullptr;
        g_afireAdsWeaponSeen   = false;
        g_afireAdsSettling     = false;
        g_afireAdsTracePending = false;
        g_afireAdsFireTracePending = false;
        g_afireAdsTraceActive  = false;
        g_afireReleasePending  = false;

        g_afireAdsNegCached = AdsRead{};
        g_afireAdsNegTime   = -1000.0;
        g_afireAdsNegWeapon = nullptr;
        g_afireAdsNegComp   = nullptr;
        g_afireAdsNegValid  = false;

        g_afireSeenTarget   = nullptr;
        g_afireSeenStamp    = -1000.0;
        g_afireReactionRoll = 0.0;
        g_afireTraitWeapon  = nullptr;
        g_afireTraitStamp   = -1000.0;
        g_afireClassCache   = WeaponClass::Unknown;
    }

    inline void UpdateNoSprintFireDelayWeapon(void *localPawnInstance);

    inline void A_FirePawnTicked(void *pawnInstance)
    {
        TriggerbotDiag &d = g_triggerDiag;
        d.pawnTicks++;

        if (A_FireLocalPawnFast() == (Pawn *)pawnInstance) {
            // A new local pawn means a new match or a respawn. The triggerbot's
            // caches still describe the old pawn and its weapon, so drop them
            // before anything touches the new scene.
            if (g_afireLastLocalPawn != nullptr && g_afireLastLocalPawn != pawnInstance) {
                LOGI("[NewFeatures] triggerbot: local pawn %p replaced %p, per-match state cleared",
                     pawnInstance, g_afireLastLocalPawn);
                A_FireResetForNewMatch();
            }
            g_afireLastLocalPawn = pawnInstance;

            // A new match game means a new round even when the game reuses the
            // same pawn address across rounds. The weapon and fire-component
            // caches would still point into the torn-down round, and the next
            // managed call would dereference a freed object - that is the crash
            // seen entering the 2nd round. Clear them on the match change too.
            if (fn_GameGetMatchGame != nullptr) {
                void *mg = fn_GameGetMatchGame();
                if (g_afireLastMatchGame != nullptr && g_afireLastMatchGame != mg) {
                    LOGI("[NewFeatures] triggerbot: match game %p replaced %p, per-match state cleared",
                         mg, g_afireLastMatchGame);
                    A_FireResetForNewMatch();
                }
                g_afireLastMatchGame = mg;
            }

            d.localPawn = pawnInstance;
            d.pawnLocalTicks++;

            fault_guard::NotePhase("local pawn tick");
            UpdateNoSprintFireDelayWeapon(pawnInstance);
            A_FireTick(nullptr);
            fault_guard::NotePhase("aim lock");
            aim_lock::Tick((Pawn *)pawnInstance);
        }
    }

    inline void Hook_Weapon_Tick(void *instance, float deltaTime)
    {
        if (orig_Weapon_Tick != nullptr)
            orig_Weapon_Tick(instance, deltaTime);

        if (instance == nullptr) return;

        g_triggerDiag.weaponTicks++;
        fault_guard::NotePhase("weapon tick");
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

        auto instanceIdAddr = UnityRva(RVA::Object_InstanceIDToObject);
        if (Tools::IsPtrValid(instanceIdAddr))
            fn_ObjectInstanceIDToObject = (void *(*)(int32_t))instanceIdAddr;

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
                 " ADS-only firing waits for a valid pawn ADS getter",
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

        auto firePosAddr = UnityRva(RVA::Pawn_get_FirePosition);
        if (Tools::IsPtrValid(firePosAddr))
            fn_PawnGetFirePosition = (Vector3 (*)(void *))firePosAddr;

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

        struct InventoryFn { uintptr_t rva; bool (**slot)(void *); const char *name; };
        const InventoryFn inventoryFns[] = {
            { RVA::Inventory_IsGun,           &fn_InventoryIsGun,           "Inventory::IsGun"           },
            { RVA::Inventory_IsAssaultRifle,  &fn_InventoryIsAssaultRifle,  "Inventory::IsAssaultRifle"  },
            { RVA::Inventory_IsSniper,        &fn_InventoryIsSniper,        "Inventory::IsSniper"        },
            { RVA::Inventory_IsMarksmanRifle, &fn_InventoryIsMarksmanRifle, "Inventory::IsMarksmanRifle" },
            { RVA::Inventory_IsShotGun,       &fn_InventoryIsShotGun,       "Inventory::IsShotGun"       },
            { RVA::Inventory_IsPistol,        &fn_InventoryIsPistol,        "Inventory::IsPistol"        },
            { RVA::Inventory_IsLauncher,      &fn_InventoryIsLauncher,      "Inventory::IsLauncher"      },
        };
        for (const InventoryFn &f : inventoryFns) {
            void *addr = UnityRva(f.rva);
            if (Tools::IsPtrValid(addr))
                *f.slot = (bool (*)(void *))addr;
            else
                LOGE("[NewFeatures] triggerbot: %s 0x%llX did not resolve - weapon"
                     " classification falls back to the fire mode",
                     f.name, (unsigned long long)f.rva);
        }

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
        LOGI("[NewFeatures] triggerbot: AmmoCanFire=%p ClipAmmoCount=%p PawnIsAiming=%p "
             "InstanceIDToObject=%p", (void *)fn_WeaponAmmoCanFire,
             (void *)fn_WeaponGetClipAmmoCount, (void *)fn_PawnIsAiming,
             (void *)fn_ObjectInstanceIDToObject);
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

