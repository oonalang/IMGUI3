// ============================================================================
//  nl_bindings  -  the bridge between the Neverlose menu's controls and this
//  project's feature flags (sConfig + the globals Main.cpp owned before the UI
//  swap). The menu file calls these; every feature behaves exactly as before.
// ============================================================================
#pragma once

#include <cstdint>

namespace nl_bind
{
// Control slots. The menu hands one per row; -1 keeps the row in the menu's
// own demo state (pure Neverleave visuals).
enum Slot : int
{
    SlotNone = -1,

    // AIM (page Rage)
    Aim360, AimSilent, AimAssistSize, AimTarget, AimTrigger, AimBy, AimFov,
    TriggerEnable, TriggerMode,

    // EXTRA (page Legit)
    ExtraHit, ExtraRecoil, ExtraSpread, ExtraShake, ExtraRpd, ExtraParachute,
    ExtraFlash, ExtraFire, ExtraUnlimitedFly, ExtraDiving, ExtraReload,
    ExtraScope, ExtraSwitch, ExtraKinetic, ExtraMythic, ExtraNoSlideFatigue,
    ExtraNoCrouch, ExtraNoHitFlinch, ExtraNoDrift, ExtraNoSprintFireDelay,
    ExtraNoHipfire, ExtraSpectate, ExtraWalkUnderWater, ExtraNoSmoke,
    ExtraNoGravity, ExtraSmartReload,
    SliderSnowboard, SliderSlide, SliderSpeed, SliderJump,

    // ESP (page Players)
    EspLine, EspBox, EspSkeleton, EspHealth, EspName, EspDistance, EspCount,
    EspAlert, EspAimline, EspYellowWallhack, EspRedWallhack,
    SelectBoxType, SelectLinePos, SelectHealthPos, SelectEspStyle,

    // WORLD (page World)
    DotEnabled, DotOpacity, DotSize, DotOffsetX, DotOffsetY,

    // SKINS (page Inventory)
    SkinBlueprints, SkinAutoEquip,

    // MISC (page Miscellaneous)
    MiscHideStream, MiscBlockReport,

    // Actions ( MENU buttons; no state row)
    ActSaveConfig, ActLoadConfig, ActGuestReset,

    SlotCount
};

// Pointer providers; each returns null when the slot is not bound.
bool*  ToggleRef(int slot);
int*   SelectRef(int slot);
float* SliderRef(int slot);
void   SliderRange(int slot, float& lo, float& hi);

// Popup options for a bound select; returns count.
int        SelectOptions(int slot, const char* const** out_options);
const char* SelectFormat(int slot, int value);

// One-off menu actions.
void DoAction(int slot);

// Menu visibility owned by the adapter consumer (Main.cpp decides what runs).
bool MenuWantsVisible();
void SetMenuWantsVisible(bool v);

// Initial slider fraction for a bound slot (reads the live config value).
float SliderInitial(int slot);
} // namespace nl_bind
