// nl_bindings.cpp - implements every slot in nl_bindings.hpp against the
// project's real feature flags. Deliberately the ONLY file that knows both
// sides: the menu stays independent, the features stay untouched.
#include "nl_bindings.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <string>
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "IL2CppSDKGenerator/BasicStructs/Includes.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools2.h"
#include "IL2CppSDKGenerator/BasicStructs/Quaternion.h"
#include "IL2CppSDKGenerator/BasicStructs/Rect.h"
#include "Includes/Utils.h"
#include "Includes/Jni_Stuff.h"
#include "Includes/OverlayGeometry.h"
#include "Hacks/Hacks.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "System/Core/FileUtils.h"
#include "System/Core/ConfigManager.h"
#include "System/Core/SaveConfig.h"
#include "System/UI/GuestAccount.h"

extern bool  HIDEESP;
extern bool  UnlimitedFly;
extern float SnowBsize;
extern float SlideRange;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;

namespace nl_bind
{
bool MenuWantsVisible() { return true; }
void SetMenuWantsVisible(bool v) { (void)v; }

bool* ToggleRef(int slot)
{
    switch (slot)
    {
    case Aim360:             return &Config.Aim.Aimbot360;
    case AimSilent:          return &Config.Aim.AimSilent;
    case TriggerEnable:      return &Config.NewFeaturesMenu.Triggerbot;
    case ExtraHit:           return &Config.ExtraMenu.Hit;
    case ExtraRecoil:        return &Config.ExtraMenu.Recoil;
    case ExtraSpread:        return &Config.ExtraMenu.Spread;
    case ExtraShake:         return &Config.ExtraMenu.Shake;
    case ExtraRpd:           return &Config.ExtraMenu.Rpd;
    case ExtraParachute:     return &Config.ExtraMenu.Parachute;
    case ExtraFlash:         return &Config.ExtraMenu.Flash;
    case ExtraFire:          return &Config.ExtraMenu.Fire;
    case ExtraUnlimitedFly:  return &UnlimitedFly;
    case ExtraDiving:        return &Config.ExtraMenu.Diving;
    case ExtraReload:        return &Config.ExtraMenu.Reload;
    case ExtraScope:         return &Config.ExtraMenu.Scope;
    case ExtraSwitch:        return &Config.ExtraMenu.Switch;
    case ExtraKinetic:       return &Config.ExtraMenu.Kinetic;
    case ExtraMythic:        return &Config.ExtraMenu.MythicAttachments;
    case ExtraNoSlideFatigue:return &Config.NewFeaturesMenu.NoSlideFatigue;
    case ExtraNoCrouch:      return &Config.NewFeaturesMenu.NoCrouch;
    case ExtraNoHitFlinch:   return &Config.NewFeaturesMenu.NoHitFlinch;
    case ExtraNoDrift:       return &Config.NewFeaturesMenu.NoDrift;
    case ExtraNoSprintFireDelay: return &Config.NewFeaturesMenu.NoSprintToFireDelay;
    case ExtraNoHipfire:     return &Config.NewFeaturesMenu.NoHipfireDelay;
    case ExtraSpectate:      return &Config.ExtraMenu.Spectatex;
    case ExtraWalkUnderWater:return &Config.ExtraMenu.WalkUnderWater;
    case ExtraNoSmoke:       return &Config.ExtraMenu.NoSmoke;
    case ExtraNoGravity:     return &Config.ExtraMenu.NoGravity;
    case ExtraSmartReload:   return &Config.ExtraMenu.SmartReload;
    case EspLine:            return &Config.ESPMenu.isPlayerLine;
    case EspBox:             return &Config.ESPMenu.Box;
    case EspSkeleton:        return &Config.ESPMenu.Skeleton;
    case EspHealth:          return &Config.ESPMenu.Health;
    case EspName:            return &Config.ESPMenu.Name;
    case EspDistance:        return &Config.ESPMenu.Distance;
    case EspCount:           return &Config.ESPMenu.Count;
    case EspAlert:           return &Config.ESPMenu.Alert;
    case EspAimline:         return &Config.ESPMenu.Aimline;
    case EspYellowWallhack:  return &Config.ExtraMenu.WallHack;
    case EspRedWallhack:     return &Config.ExtraMenu.RedWallhack;
    case DotEnabled:         return &Config.Crosshair.Enabled;
    case SkinBlueprints:     return &Config.ExtraMenu.Blueprints;
    case SkinAutoEquip:      return &Config.ExtraMenu.AutoSkin;
    case MiscHideStream:     return &HIDEESP;
    case MiscBlockReport:    return &Config.ExtraMenu.BlockReport;
    default:                 return nullptr;
    }
}

int* SelectRef(int slot)
{
    switch (slot)
    {
    case AimTarget:       return (int*)&Config.Aim.Target;
    case AimTrigger:      return (int*)&Config.Aim.Trigger;
    case AimBy:           return (int*)&Config.Aim.By;
    case TriggerMode:     return &Config.NewFeaturesMenu.TriggerbotMode;
    case SelectBoxType:   return (int*)&Config.ESPMenu.BoxType;
    case SelectLinePos:   return (int*)&Config.ESPMenu.Target;
    case SelectHealthPos: return (int*)&Config.ESPMenu.HealthPosition;
    case SelectEspStyle:  return (int*)&Config.ESPMenu.EspStyle;
    default:              return nullptr;
    }
}

float* SliderRef(int slot)
{
    switch (slot)
    {
    case AimAssistSize:  return &Config.Aim.AimAssistSize;
    case AimFov:         return &Config.Aim.Cross;
    case SliderSnowboard:return &SnowBsize;
    case SliderSlide:    return &SlideRange;
    case SliderSpeed:    return &speedHackMultiplier;
    case SliderJump:     return &jumpHeightMultiplier;
    case DotOpacity:     return &Config.Crosshair.Color[3];
    case DotSize:        return &Config.Crosshair.Size;
    case DotOffsetX:     return &Config.Crosshair.OffsetX;
    case DotOffsetY:     return &Config.Crosshair.OffsetY;
    default:             return nullptr;
    }
}

void SliderRange(int slot, float& lo, float& hi)
{
    switch (slot)
    {
    case AimAssistSize:  lo = 0.0f;   hi = 100.0f; break;
    case AimFov:         lo = 0.0f;   hi = 100.0f; break;
    case SliderSnowboard:lo = 0.0f;   hi = 100.0f; break;
    case SliderSlide:    lo = 0.0f;   hi = 30.0f;  break;
    case SliderSpeed:    lo = 0.5f;   hi = 2.0f;   break;
    case SliderJump:     lo = 0.5f;   hi = 5.0f;   break;
    case DotOpacity:     lo = 0.0f;   hi = 1.0f;   break;
    case DotSize:        lo = 1.0f;   hi = 40.0f;  break;
    case DotOffsetX:     lo = -400.0f;hi = 400.0f; break;
    case DotOffsetY:     lo = -400.0f;hi = 400.0f; break;
    default:             lo = 0.0f;   hi = 1.0f;   break;
    }
}

float SliderInitial(int slot)
{
    float lo = 0.0f, hi = 1.0f;
    SliderRange(slot, lo, hi);
    float* v = SliderRef(slot);
    if (!v) return 0.0f;
    if (hi <= lo) return 0.0f;
    return (*v - lo) / (hi - lo);
}

static const char* const kAimTargetOptions[]   = { "Head", "Chest", "Body" };
static const char* const kAimTriggerOptions[]  = { "None", "Shooting", "Scoping" };
static const char* const kAimByOptions[]       = { "Distance", "FOV" };
static const char* const kTriggerModeOptions[] = { "Always", "ADS Only", "Firing Only" };
static const char* const kBoxTypeOptions[]     = { "Fill", "Outline", "Corner", "3D" };
static const char* const kLinePosOptions[]     = { "Top", "Mid", "Bottom" };
static const char* const kHealthPosOptions[]   = { "Top", "Side" };
static const char* const kEspStyleOptions[]    = { "None", "3D Sphere", "Player Signal" };

int SelectOptions(int slot, const char* const** out_options)
{
    switch (slot)
    {
    case AimTarget:       *out_options = kAimTargetOptions;   return 3;
    case AimTrigger:      *out_options = kAimTriggerOptions;  return 3;
    case AimBy:           *out_options = kAimByOptions;       return 2;
    case TriggerMode:     *out_options = kTriggerModeOptions; return 3;
    case SelectBoxType:   *out_options = kBoxTypeOptions;     return 4;
    case SelectLinePos:   *out_options = kLinePosOptions;     return 3;
    case SelectHealthPos: *out_options = kHealthPosOptions;   return 2;
    case SelectEspStyle:  *out_options = kEspStyleOptions;    return 3;
    default:              *out_options = nullptr;             return 0;
    }
}

const char* SelectFormat(int slot, int value)
{
    const char* const* options = nullptr;
    const int count = SelectOptions(slot, &options);
    if (!options || count <= 0) return "";
    if (value < 0 || value >= count) value = 0;
    return options[value];
}

void DoAction(int slot)
{
    switch (slot)
    {
    case ActSaveConfig:  SaveConfig();  break;
    case ActLoadConfig:  LoadConfig();  break;
    case ActGuestReset:  ResetBannedGuestAccount(); break;
    default: break;
    }
}
} // namespace nl_bind
