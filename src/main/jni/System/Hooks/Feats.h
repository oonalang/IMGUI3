#pragma once

#include <cstdint>
#include <string>

extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern float SlideRange;
extern bool UnlimitedAmmo;
extern bool UnlimitedFly;
extern std::chrono::steady_clock::time_point lastAmmoRefillTime;

namespace SmartReloadCfg
{
    constexpr uintptr_t CarriedAmmoCountRva = 0x50EC6E8;
    constexpr uintptr_t PawnTryChangeClipRva = 0x5262594;
}

inline bool g_SmartReloadPending = false;

inline int SmartReloadCarriedAmmo(void *weapon)
{
    using GetAmmoFn = int (*)(void *);
    static GetAmmoFn getCarriedAmmo = reinterpret_cast<GetAmmoFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::CarriedAmmoCountRva));
    return getCarriedAmmo != nullptr ? getCarriedAmmo(weapon) : 0;
}

inline void SmartReloadRequestChangeClip(Pawn *local)
{
    using TryChangeClipFn = void (*)(Pawn *, bool);
    static TryChangeClipFn tryChangeClip = reinterpret_cast<TryChangeClipFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::PawnTryChangeClipRva));

    if (tryChangeClip != nullptr)
        tryChangeClip(local, true);
}

inline void SmartReloadMarkShotFired()
{
    if (!Config.ExtraMenu.SmartReload)
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive())
        return;

    g_SmartReloadPending = true;
}

inline bool SmartReloadCanRequestForPawn(Pawn *local)
{
    if (!Config.ExtraMenu.SmartReload || !g_SmartReloadPending)
        return false;

    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_SmartReloadPending = false;
        return false;
    }

    Pawn *currentLocal = GamePlay::get_LocalPawn();
    if (currentLocal != local) {
        g_SmartReloadPending = false;
        return false;
    }

    Weapon *weapon = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(weapon)) {
        g_SmartReloadPending = false;
        return false;
    }

    if (SmartReloadCarriedAmmo(weapon) <= 0) {
        g_SmartReloadPending = false;
        return false;
    }

    return true;
}

inline void SmartReloadRequestAfterStopFire(Pawn *local)
{
    if (!SmartReloadCanRequestForPawn(local))
        return;

    g_SmartReloadPending = false;
    SmartReloadRequestChangeClip(local);
}

inline void (*orig_Pawn_StopFire)(Pawn *instance, bool isImmidiately) = nullptr;
inline void hook_Pawn_StopFire(Pawn *instance, bool isImmidiately)
{
    orig_Pawn_StopFire(instance, isImmidiately);
    SmartReloadRequestAfterStopFire(instance);
}

inline void (*orig_SetGrassShowState)(bool showGrass) = nullptr;
inline void hook_SetGrassShowState(bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain)
        showGrass = false;
    return orig_SetGrassShowState(showGrass);
}

inline void (*orig_SetGrassLODBias)(float lodBias, float midLodBias, bool showGrass) = nullptr;
inline void hook_SetGrassLODBias(float lodBias, float midLodBias, bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain) {
        lodBias = 0.0f;
        midLodBias = 0.0f;
        showGrass = false;
    }
    return orig_SetGrassLODBias(lodBias, midLodBias, showGrass);
}

inline void ApplyWorldVisualsRuntime()
{
    static bool lastClearTerrain = false;
    if (Config.ExtraMenu.ClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(false);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(0.0f, 0.0f, false);
    } else if (lastClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(true);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(1.0f, 1.0f, true);
    }
    lastClearTerrain = Config.ExtraMenu.ClearTerrain;
}

inline void ApplyStretchScreen(float displayW, float displayH)
{
    static Camera *lastCamera = nullptr;
    static float   savedAspect = 0.0f;
    static bool    applied = false;

    Camera *cam = Camera::get_main();
    if (cam == nullptr)
        return;

    if (cam != lastCamera) {
        lastCamera = cam;
        const float current = cam->get_aspect();
        savedAspect = (current >= 1.0f && current <= 3.0f)
            ? current
            : ((displayW > 0.0f && displayH > 0.0f) ? displayW / displayH : 1.7778f);
        applied = false;
    }

    if (!Config.ExtraMenu.StretchScreen) {
        if (applied) {
            cam->set_aspect(savedAspect);
            applied = false;
        }
        return;
    }

    float target = Config.ExtraMenu.StretchAspect;
    if (!(target >= 1.0f && target <= 3.0f))
        target = savedAspect;
    cam->set_aspect(target);
    applied = true;
}

inline float (*orig_GetAssitAimSpeed)(void *, Vector3, float, float, float, bool, bool) = nullptr;
inline float GetAssitAimSpeed(void * instance, Vector3 assistCentorPos, float assistDis, float dis, float angle, bool isPVE, bool gamepadInput) {
    if (instance != NULL) {
        if (Config.Aim.AimAssistSize > 0.0f) {
            return (float)Config.Aim.AimAssistSize;
        }
    }
    return orig_GetAssitAimSpeed(instance, assistCentorPos, assistDis, dis, angle, isPVE, gamepadInput);
}

inline void (*orig_OnFlashBangExplode)(void *, int, float, float, float) = nullptr;
inline void hook_OnFlashBangExplode(void *instance, int weaponItemID, float whiteTime, float whiteAlphaTime, float initIntensity) {
    if (instance != nullptr && Config.ExtraMenu.Flash) {
        whiteTime = 0.1f;
        whiteAlphaTime = 0.1f;
        initIntensity = 0.1f;
    }
    orig_OnFlashBangExplode(instance, weaponItemID, whiteTime, whiteAlphaTime, initIntensity);
}

inline float (*orig_get_FireBoltTime)(void *) = nullptr;
inline float get_FireBoltTime(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.05f;
        }
    }
    return orig_get_FireBoltTime(instance);
}

inline float (*orig_get_FireInterval)(void *) = nullptr;
inline float get_FireInterval(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.05f;
        }
    }
    return orig_get_FireInterval(instance);
}

inline float (*orig_get_DelaySprintFire)(void *) = nullptr;
inline float get_DelaySprintFire(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire || UnlimitedFly) {
            return 0.00001f;
        }
        if (NewFeatures::NoSprintFireDelayActive(instance)) {
            return 0.00001f;
        }
    }
    return orig_get_DelaySprintFire(instance);
}

typedef void (*SetUltraFrameRateDeviceInfo_t)(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate);
SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 144;
        ultraFrameRateBR = 144;
        ultraFrameRateQualityLimit = 144;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
}

inline float (*orig_GetMaxJumpHeight)(void*) = nullptr;
inline float hook_GetMaxJumpHeight(void* instance) {
    float orig_ = orig_GetMaxJumpHeight(instance);
    return (jumpHeightMultiplier > 1.0f) ? orig_ * jumpHeightMultiplier : orig_;
}

inline bool A_BulletTrackIsLocalShot(void *fireComponent)
{
    if (!Tools::IsPtrValid(fireComponent)) return false;

    void *weapon = *(void **)((uintptr_t)fireComponent + 0x10);
    if (!Tools::IsPtrValid(weapon)) return false;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local)) return false;

    return (void *)local->get_CurrentWeapon() == weapon;
}

inline bool A_BulletTrackTriggerReady()
{
    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local)) return false;

    if (Config.Aim.Trigger == EAimTrigger::Shooting)
        return ((bool (*)(uintptr_t))(Class_Pawn_get_IsFiring))((uintptr_t)local);
    if (Config.Aim.Trigger == EAimTrigger::Scoping)
        return ((bool (*)(uintptr_t))(Class_Pawn_IsAiming))((uintptr_t)local);
    return true;
}

inline bool A_BulletTrackTargetHead(const Vector3 &startPos, const Vector3 &dir, Vector3 &outHead)
{
    auto getMatchGame = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    uintptr_t matchGame = (getMatchGame != nullptr) ? getMatchGame() : 0;
    if (!Tools::IsPtrValid((void *) matchGame)) return false;

    auto EnemyPawns = *(List<uintptr_t> **) (matchGame + Class_BaseGame_EnemyPawns);
    if (!Tools::IsPtrValid((void *) EnemyPawns)) return false;
    auto Items = EnemyPawns->getItems();
    if (!Items) return false;

    const int count = EnemyPawns->getSize();
    if (count <= 0 || count > 1024) return false;

    const bool byFov = (Config.Aim.By == EAim::Crosshair);
    const Vector3 rayAxis = Vector3::Normalized(dir);

    bool found = false;
    float best = 0.0f;
    for (int i = 0; i < count; i++) {
        uintptr_t pawnU = Items[i];
        if (!Tools::IsPtrValid((void *) pawnU)) continue;

        Pawn *pawn = (Pawn *) pawnU;
        if (!pawn->m_IsAlive()) continue;

        const Vector3 head = pawn->get_HeadPosition();
        const Vector3 toHead = head - startPos;

        float metric;
        if (byFov) {
            const float along = Vector3::Dot(toHead, rayAxis);
            if (along <= 0.0f) continue;
            metric = Vector3::Dot(toHead, toHead) - along * along;
        } else {
            metric = Vector3::SqrMagnitude(toHead);
        }

        if (!found || metric < best) {
            best = metric;
            outHead = head;
            found = true;
        }
    }
    return found;
}

inline void A_BulletTrackAimRay(void *fireComponent, const Vector3 &startPos, Vector3 &dir)
{
    if (!Config.Aim.AimSilent && !Config.Aim.Aimbot360) return;
    if (!A_BulletTrackTriggerReady()) return;
    if (!A_BulletTrackIsLocalShot(fireComponent)) return;

    Vector3 head;
    if (!A_BulletTrackTargetHead(startPos, dir, head)) return;

    const Vector3 toTarget = head - startPos;
    if (!(Vector3::SqrMagnitude(toTarget) > 0.0001f)) return;

    dir = toTarget;
}

inline bool (*orig_SingleLineCheckPhysics_Instant)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;
inline bool SingleLineCheckPhysics_Instant_Hook(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    A_BulletTrackAimRay(instance, startPos, dir);
    if (orig_SingleLineCheckPhysics_Instant != nullptr)
        return orig_SingleLineCheckPhysics_Instant(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
    return false;
}

inline bool (*orig_SingleLineCheckPhysics)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;
inline bool SingleLineCheckPhysics(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Hit) {
            return true;
        }
    }
    return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

inline float (*o_get_SlideTackleAcclerationSpeed)(void*) = nullptr;
inline float h_get_SlideTackleAcclerationSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 1;
    }
    return o_get_SlideTackleAcclerationSpeed(ins);
}

inline float (*o_PawnGetMaxSpeed)(void*) = nullptr;
inline float h_PawnGetMaxSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_PawnGetMaxSpeed(ins);
}

inline float (*o_get_SlideTackleSpeed)(void*) = nullptr;
inline float h_get_SlideTackleSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange;
    }
    return o_get_SlideTackleSpeed(ins);
}

inline float (*o_GetSuperSlideRate)(void*) = nullptr;
inline float h_GetSuperSlideRate(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_GetSuperSlideRate(ins);
}

inline void (*o_TickLocalPlayer)(void*, float) = nullptr;
inline void h_TickLocalPlayer(void* ins, float deltaTime) {
    if (SlideRange <= 0.0f) {
        o_TickLocalPlayer(ins, deltaTime);
    }
}

inline float (*orig_get_AddHotTime)(void* instance) = nullptr;
inline float get_AddHotTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Rpd) {
            return 0.00001f;
        }
    }
    return orig_get_AddHotTime(instance);
}

inline void (*orig_OpenParachute)(void* instance, bool isAuto) = nullptr;
inline void OpenParachute(void* instance, bool isAuto) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Parachute) {
            return;
        }
    }
    return orig_OpenParachute(instance, isAuto);
}

inline float (*orig_get_ChangeClipTime)(void* instance) = nullptr;
inline float get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipTime(instance);
}

inline float (*orig_get_ChangeClipLoopTime)(void* instance) = nullptr;
inline float get_ChangeClipLoopTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipLoopTime(instance);
}

inline float (*orig_get_AimingTime)(void* instance) = nullptr;
inline float get_AimingTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Scope) {
            return 0.0001f;
        }
    }
    return orig_get_AimingTime(instance);
}

inline float (*orig_get_EquipTime)(void* instance) = nullptr;
inline float get_EquipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_EquipTime(instance);
}

inline float (*orig_get_UnequipTime)(void* instance) = nullptr;
inline float get_UnequipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_UnequipTime(instance);
}

float (*orig_GetAccDistance)(void *instance);
float GetAccDistance(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return 50.0f;
        }
    }
    return orig_GetAccDistance(instance);
}

inline bool IsTutorialEnabled() {
    return false;
}

inline float (*orig_get_AccelerationForwardSpeedUp)(void* instance) = nullptr;
inline float get_AccelerationForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_AccelerationForwardSpeedUp(instance);
}

inline float (*orig_get_MaxVelocityForwardSpeedUp)(void* instance) = nullptr;
inline float get_MaxVelocityForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_MaxVelocityForwardSpeedUp(instance);
}

inline float (*get_m_PhysSkisMaxSpeed)(void*) = nullptr;
inline float hooked_get_m_PhysSkisMaxSpeed(void* instance) {
    if (SnowBsize > 0.0f) {
        return SnowBsize;
    }
    return get_m_PhysSkisMaxSpeed(instance);
}

inline float (*original_CalcFinalMoveScale)(void*) = nullptr;
inline float hooked_CalcFinalMoveScale(void* instance) {
    if (instance == nullptr) {
        return original_CalcFinalMoveScale(instance);
    }
    if (speedHackMultiplier > 1.0f && speedHackMultiplier <= 100.0f) {
        return speedHackMultiplier;
    }
    return original_CalcFinalMoveScale(instance);
}

inline float (*orig_GetScaleRecoil)(void* instance) = nullptr;
inline float GetScaleRecoil(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Recoil) {
            return 0.00001f;
        }
    }
    return orig_GetScaleRecoil(instance);
}

inline float (*orig_MinInaccuracy)(void *) = nullptr;
inline float MinInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MinInaccuracy(instance);
}

inline float (*orig_MaxInaccuracy)(void *) = nullptr;
inline float MaxInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MaxInaccuracy(instance);
}

inline float (*orig_DisperseBase)(void *) = nullptr;
inline float DisperseBase(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_DisperseBase(instance);
}

inline bool (*orig_IsUnlocked)(void *instance) = nullptr;
inline bool IsUnlocked(void *instance) {
    // The blueprint toggle means "this player owns everything". The game asks
    // some of these questions through a static helper that passes a null
    // receiver; requiring an instance made those calls answer "locked", so the
    // switch did nothing in part of the loadout.
    if (Config.ExtraMenu.MythicAttachments)
        return true;
    return orig_IsUnlocked != nullptr ? orig_IsUnlocked(instance) : false;
}

#include "MythicAttachments.h"

#include "ReportBlock.h"

#include "AntiLogin.h"

inline bool (*o_NeedDelayProcess)(void *instance, int a, int b);
inline bool h_NeedDelayProcess(void *instance, int a, int b) {
    bool orig_val = o_NeedDelayProcess(instance, a, b);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

inline bool (*o_ProcessForDelay)(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex);
inline bool h_ProcessForDelay(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex) {
    bool orig_val = o_ProcessForDelay(instance, packData, PackDataCount, Sequence, timeQueued, aiIndex);

    if (Config.ExtraMenu.Spectatex) {
        return false;
    }

    return orig_val;
}

inline bool (*o_get_IsObserver)(void *);
inline bool h_get_IsObserver(void *instance) {
    bool orig_val = o_get_IsObserver(instance);

    if (Config.ExtraMenu.Spectatex) {
        return true;
    }

    return orig_val;
}

inline float (*o_GetDelayCountDown)(void *);
inline float h_GetDelayCountDown(void *instance) {
    float orig_val = o_GetDelayCountDown(instance);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

int (*orig_get_DeviceCapacityLevel)(void *instance);
int get_DeviceCapacityLevel(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Grap) {
            return 6;
        }
    }
    return orig_get_DeviceCapacityLevel(instance);
}

inline bool (*orig_CheckTargetIsValid_DyingInAzurGameMode)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_DyingInAzurGameMode_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_DyingInAzurGameMode(instance, targetPawn);
}

inline bool (*orig_CheckTargetIsValid_PhysState)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_PhysState_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_PhysState(instance, targetPawn);
}

inline bool (*orig_CheckTargetIsValid_ManualParameter)(void* instance, void* tempPawn, float* distance);
inline bool CheckTargetIsValid_ManualParameter_Hook(void* instance, void* tempPawn, float* distance) {
    if (instance != nullptr && tempPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_ManualParameter(instance, tempPawn, distance);
}

inline bool (*orig_CheckTargetIsValid_Ult)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_Ult_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_Ult(instance, targetPawn);
}

inline bool (*orig_CalculateObstacleSide)(void* instance, float* backDis, float* beforeDis);
inline bool CalculateObstacleSide_Hook(void* instance, float* backDis, float* beforeDis) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleSide(instance, backDis, beforeDis);
}

inline bool (*orig_CalculateObstacleTop)(void* instance, Vector3 pawnPos);
inline bool CalculateObstacleTop_Hook(void* instance, Vector3 pawnPos) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleTop(instance, pawnPos);
}

int (*oCheckTargetIsValid)(void *ins, void* tempPawn, float distance);
int CheckTargetIsValid(void *ins, void* tempPawn, float distance) {
    if ( ins != nullptr ) {
        if (isExecute) {
            return (int)true;
        }
    }
    return oCheckTargetIsValid(ins, tempPawn, distance);
}

inline bool (*oCheckCanExcution_Extra)(void *ins);
inline bool CheckCanExcution_Extra(void *ins) {
    if ( ins != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return oCheckCanExcution_Extra(ins);
}

inline bool (*o_CheckExecution_ObstacleAround)(void *ins);
inline bool CheckExecution_ObstacleAround(void *ins) {
    if (ins!=nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return o_CheckExecution_ObstacleAround(ins);
}

inline bool (*oPawn_GetCurrentDistToWaterSurface)(void *instance);
inline bool Pawn_GetCurrentDistToWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPawn_GetCurrentDistToWaterSurface(instance);
}

inline bool (*oPlayerPawn_IsUnderWaterSurface)(void *instance);
inline bool PlayerPawn_IsUnderWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPlayerPawn_IsUnderWaterSurface(instance);
}

inline bool (*oCheckInWaterComponent_get_CurrentWaterSurfaceHeight)(void *instance);
inline bool CheckInWaterComponent_get_CurrentWaterSurfaceHeight(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return true;
        }
    }
    return oCheckInWaterComponent_get_CurrentWaterSurfaceHeight(instance);
}

float (*oInputSettingConfig_Instant_GetMainCameraFov_3p)(void *instance);
float InputSettingConfig_Instant_GetMainCameraFov_3p(void *instance) {
    if (instance && Config.ExtraMenu.CameraPov) {
        return (float)Config.ExtraMenu.CameraPovSize;
    }
    return oInputSettingConfig_Instant_GetMainCameraFov_3p(instance);
}

inline bool NameHasSmokeToken(const char *name)
{
    if (!name)
        return false;

    std::string s(name);
    for (char &c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }

    return s.find("smoke") != std::string::npos ||
           s.find("smog") != std::string::npos ||
           s.find("smk") != std::string::npos;
}

inline String *(*UnityObject_get_name)(void *instance) = nullptr;
inline void (*orig_GameObjectSetActive)(void *instance, bool value) = nullptr;
inline void hook_GameObjectSetActive(void *instance, bool value)
{
    if (Config.ExtraMenu.NoSmoke && value && instance && UnityObject_get_name) {
        String *name = UnityObject_get_name(instance);
        if (name && NameHasSmokeToken(name->CString()))
            value = false;
    }
    return orig_GameObjectSetActive(instance, value);
}

inline void InitializeAllHooks() {
    HOOK_LIB("libunity.so", "0x5260B78", hook_Pawn_StopFire, orig_Pawn_StopFire);

    HOOK_LIB("libunity.so", "0x666FD90", GetAssitAimSpeed, orig_GetAssitAimSpeed);

    HOOK_LIB("libunity.so", "0x51E977C", hook_OnFlashBangExplode, orig_OnFlashBangExplode);

    HOOK_LIB("libunity.so", "0x51237F4", get_FireBoltTime, orig_get_FireBoltTime);
    HOOK_LIB("libunity.so", "0x5105C74", get_FireInterval, orig_get_FireInterval);
    HOOK_LIB("libunity.so", "0x513CBC4", get_DelaySprintFire, orig_get_DelaySprintFire);

    HOOK_LIB("libunity.so", "0x5221D00", hook_GetMaxJumpHeight, orig_GetMaxJumpHeight);

    HOOK_LIB("libunity.so", "0xC1514C0", SingleLineCheckPhysics, orig_SingleLineCheckPhysics);

    HOOK_LIB("libunity.so", "0xC9B2A9C", SingleLineCheckPhysics_Instant_Hook, orig_SingleLineCheckPhysics_Instant);

    HOOK_LIB("libunity.so", "0x68823FC", OpenParachute, orig_OpenParachute);

    HOOK_LIB("libunity.so", "0x50ECADC", get_ChangeClipTime, orig_get_ChangeClipTime);

    HOOK_LIB("libunity.so", "0x50EB944", get_AimingTime, orig_get_AimingTime);

    HOOK_LIB("libunity.so", "0x50ED8D4", get_EquipTime, orig_get_EquipTime);

    HOOK_LIB("libunity.so", "0xAD1DD78", GetAccDistance, orig_GetAccDistance);

    HOOK_LIB_NO_ORIG("libunity.so", "0x9DE0E58", IsTutorialEnabled);

    HOOK_LIB("libunity.so", "0x5DE981C", get_AccelerationForwardSpeedUp, orig_get_AccelerationForwardSpeedUp);
    HOOK_LIB("libunity.so", "0x5DE9880", get_MaxVelocityForwardSpeedUp, orig_get_MaxVelocityForwardSpeedUp);

    HOOK_LIB("libunity.so", "0x522860C", hooked_get_m_PhysSkisMaxSpeed, get_m_PhysSkisMaxSpeed);

    HOOK_LIB("libunity.so", "0x51D2EB8", hooked_CalcFinalMoveScale, original_CalcFinalMoveScale);

    HOOK_LIB("libunity.so", "0xC9BAFF8", GetScaleRecoil, orig_GetScaleRecoil);

    HOOK_LIB("libunity.so", "0xC9B8F78", MinInaccuracy, orig_MinInaccuracy);
    HOOK_LIB("libunity.so", "0xC159288", MaxInaccuracy, orig_MaxInaccuracy);
    HOOK_LIB("libunity.so", "0xC9C76C4", DisperseBase, orig_DisperseBase);

    HOOK_LIB("libunity.so", "0x519676C", h_ProcessForDelay, o_ProcessForDelay);
    HOOK_LIB("libunity.so", "0x51966D8", h_NeedDelayProcess, o_NeedDelayProcess);

    UnityObject_get_name = reinterpret_cast<String *(*)(void *)>(
          getAbsoluteAddress("libunity.so", 0x5052F10));

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x5036EB8),
          (void *)&hook_GameObjectSetActive,
          (void **)&orig_GameObjectSetActive);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", ThrowAlertCfg::TickRva),
          (void *)&hook_WeaponProjectileTick,
          (void **)&orig_WeaponProjectileTick);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::StickTickRva),
          (void *)&hook_StickTick,
          (void **)&orig_StickTick);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::TripMineTickRva),
          (void *)&hook_TripMineTick,
          (void **)&orig_TripMineTick);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::TrapChipTickRva),
          (void *)&hook_TrapChipTick,
          (void **)&orig_TrapChipTick);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::TripWireTickRva),
          (void *)&hook_TripWireTick,
          (void **)&orig_TripWireTick);

    NotePlacableHookLanded(PlcHookStick,    orig_StickTick    != nullptr);
    NotePlacableHookLanded(PlcHookTripMine, orig_TripMineTick != nullptr);
    NotePlacableHookLanded(PlcHookTrapChip, orig_TrapChipTick != nullptr);
    NotePlacableHookLanded(PlcHookTripWire, orig_TripWireTick != nullptr);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::PushProjectileRva),
          (void *)&hook_PushProjectile,
          (void **)&orig_PushProjectile);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", PlacableCfg::RemoveProjectileRva),
          (void *)&hook_RemoveWeaponProjectile,
          (void **)&orig_RemoveWeaponProjectile);
    NotePlacablePushLanded(orig_PushProjectile != nullptr);

    g_placableProjectileTickSink = &PlacableOnProjectileTick;

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xAE2E1B0),
          (void *)&hook_SetGrassShowState,
          (void **)&orig_SetGrassShowState);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x7B4E290),
          (void *)&hook_SetGrassLODBias,
          (void **)&orig_SetGrassLODBias);

    DobbyHook((void*)getAbsoluteAddress("libunity.so", 0x9FDEE28), (void*)hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x51D31BC), (void *)  &Pawn_GetCurrentDistToWaterSurface, (void **) &oPawn_GetCurrentDistToWaterSurface);
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x54BC450), (void *)  &PlayerPawn_IsUnderWaterSurface, (void **) &oPlayerPawn_IsUnderWaterSurface);
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x757162C), (void *)  &CheckInWaterComponent_get_CurrentWaterSurfaceHeight, (void **) &oCheckInWaterComponent_get_CurrentWaterSurfaceHeight);

  DobbyHook((void *) getAbsoluteAddress("libunity.so", 0xC347950), (void *)  &InputSettingConfig_Instant_GetMainCameraFov_3p, (void **) &oInputSettingConfig_Instant_GetMainCameraFov_3p);

    HOOK_LIB("libunity.so", "0x901F988", IsUnlocked, orig_IsUnlocked);

    HOOK_LIB("libunity.so", "0xA61AA00", IsWeaponAttachmentUnlock, orig_IsWeaponAttachmentUnlock);

    report_block::InstallReportBlockHooks();

    anti_login::InstallAntiLoginHooks();
}