#pragma once

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Placed Items ESP: tripmines, C4, trophy systems, trip sensors and every
// other place-and-stay tactical/lethal, highlighted through walls the same
// way players are (overlay marker + label + distance).
//
// Every one of these is a WeaponProjectile subclass (dump.cs, GR .57 -
// github.com/eliwoahzja/dump):
//
//   WeaponProjectile_TripMine         tripmine          (own Tick)
//   WeaponProjectile_C4bomb           C4                (Stick Tick)
//   WeaponProjectile_TankBattleC4     C4 (tank battle)  (Stick Tick)
//   WeaponProjectile_TankMine         tank mine         (Stick Tick)
//   WeaponProjectile_TrophySystem     trophy system     (Stick Tick)
//   WeaponProjectile_TacTripWire      trip sensor       (Stick Tick)
//   WeaponProjectile_TrapChipMine     trap chip mine    (own Tick)
//   WeaponProjectile_ElectricTripWire electric wire     (own Tick)
//
// Everything else Stick-based rides WeaponProjectile_Stick.Tick, so hooking
// the four Tick entry points below sees every placed item on every frame.
// The runtime class name (Il2CppClass::name, klass + 0x10 - same layout as
// Il2CppClass_1 in IL2Cpp/Il2Cpp.h) tells us which device an instance is,
// so a thrown NA45 / thermite / grapple passing through the Stick hook is
// simply not in the table and gets ignored.
//
// Tracking mirrors ThrowableAlert: hooks register live instances in a map
// keyed by object pointer, entries expire after 360 unseen ticks, and the
// draw pass only reads what the track pass stored (plus the local camp).
// ---------------------------------------------------------------------------

namespace PlacableCfg {
    // libunity.so RVAs, dump.cs GR .57
    constexpr uintptr_t StickTickRva    = 0x956D4CC; // WeaponProjectile_Stick.Tick(float)
    constexpr uintptr_t TripMineTickRva = 0x9B5B850; // WeaponProjectile_TripMine.Tick(float)
    constexpr uintptr_t TrapChipTickRva = 0x9B58A10; // WeaponProjectile_TrapChipMine.Tick(float)
    constexpr uintptr_t TripWireTickRva = 0xB201650; // WeaponProjectile_ElectricTripWire.Tick(float)
    // WeaponProjectile field offsets, dump.cs GR .57
    constexpr uintptr_t ExplodedOff  = 0xA1;  // bHasExplode
    constexpr uintptr_t DestroyedOff = 0x18D; // mIsDestroyed
    constexpr uintptr_t OwnerCampOff = 0xB0;  // m_SpawnOwnerCamp
    constexpr float MaxDist  = 100.0f;        // draw distance cap (m)
    constexpr float WarnDist = 12.0f;         // hostile pulse distance (m)
}

struct PlacableInfo {
    Vector3 pos;
    const char *label;
    int camp;
    uint64_t seen;
};

inline std::unordered_map<uintptr_t, PlacableInfo> g_Placables;
inline uint64_t g_PlacableSeen = 0;

inline void (*orig_StickTick)(void *, float) = nullptr;
inline void (*orig_TripMineTick)(void *, float) = nullptr;
inline void (*orig_TrapChipTick)(void *, float) = nullptr;
inline void (*orig_TripWireTick)(void *, float) = nullptr;

inline bool PlacableEspOn()
{
    return Config.ESPMenu.Placables;
}

// Bounded, sanity-checked copy of the instance's Il2Cpp class name. Any
// unreadable / corrupt pointer just yields "not a placable" instead of a
// crash - the whole point of this feature set is that ESP code must never
// take the game down with it.
inline bool PlacableClassName(void *proj, char *out, size_t outSize)
{
    if (!Tools::IsPtrValid(proj) || outSize < 2)
        return false;

    uintptr_t klass = *(uintptr_t *)proj;
    if (!Tools::IsPtrValid((void *)klass))
        return false;

    const char *name = *(const char **)(klass + 0x10); // Il2CppClass::name
    if (!Tools::IsPtrValid((void *)name))
        return false;

    size_t i = 0;
    for (; i + 1 < outSize; ++i) {
        char c = name[i];
        if (c == '\0')
            break;
        if (c < 0x20 || c > 0x7E)
            return false; // not a plausible class name: stale object, bail
        out[i] = c;
    }
    out[i] = '\0';
    return i > 0;
}

inline const char *PlacableLabelFor(const char *className)
{
    struct Entry { const char *cls; const char *label; };
    static const Entry kEntries[] = {
        {"WeaponProjectile_TripMine",         "TRIP MINE"},
        {"WeaponProjectile_C4bomb",           "C4"},
        {"WeaponProjectile_TankBattleC4",     "C4"},
        {"WeaponProjectile_TankMine",         "TANK MINE"},
        {"WeaponProjectile_TrophySystem",     "TROPHY SYSTEM"},
        {"WeaponProjectile_TacTripWire",      "TRIP SENSOR"},
        {"WeaponProjectile_TrapChipMine",     "TRAP CHIP MINE"},
        {"WeaponProjectile_ElectricTripWire", "TRIP WIRE"},
    };
    for (const Entry &e : kEntries) {
        if (strcmp(className, e.cls) == 0)
            return e.label;
    }
    return nullptr;
}

inline void TrackPlacable(void *proj)
{
    if (!PlacableEspOn())
        return;
    if (!Tools::IsPtrValid(proj))
        return;

    const uintptr_t base = (uintptr_t)proj;
    if (*(bool *)(base + PlacableCfg::ExplodedOff) || *(bool *)(base + PlacableCfg::DestroyedOff)) {
        g_Placables.erase(base);
        return;
    }

    char className[64];
    if (!PlacableClassName(proj, className, sizeof(className)))
        return;

    const char *label = PlacableLabelFor(className);
    if (!label)
        return; // bullet / NA45 / thermite / grapple... not placed equipment

    Vector3 pos = GetThrowPos(proj); // WeaponProjectile.get_Position (ThrowableAlert)
    if (pos == Vector3::zero())
        return;

    PlacableInfo &info = g_Placables[base];
    info.pos = pos;
    info.label = label;
    info.camp = *(int *)(base + PlacableCfg::OwnerCampOff);
    info.seen = ++g_PlacableSeen;
}

inline int PlacableLocalCamp()
{
    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local))
        return -1;

    // Same validated read as the DrawESP health fix: never dereference an
    // AttackableTargetInfo that has not passed IsPtrValid first.
    auto info = *(uintptr_t *)((uintptr_t)local + Class_AttackableTarget_m_AttackableInfo);
    if (!Tools::IsPtrValid((void *)info))
        return -1;

    return *(int *)(info + 0x58); // AttackableTargetInfo.m_Camp
}

inline void DrawPlacableESP(ImDrawList *draw, int width, int height, const Vector3 &localPos)
{
    if (!PlacableEspOn()) {
        if (!g_Placables.empty())
            g_Placables.clear();
        return;
    }

    Camera *cam = Camera::get_main();
    if (!cam)
        return;

    const int localCamp = PlacableLocalCamp();
    (void)width;

    for (auto it = g_Placables.begin(); it != g_Placables.end();) {
        PlacableInfo &info = it->second;
        if (g_PlacableSeen > info.seen + 360) {
            it = g_Placables.erase(it);
            continue;
        }

        float dist = Vector3::Distance(localPos, info.pos);
        if (dist > PlacableCfg::MaxDist) {
            ++it;
            continue;
        }

        ImVec2 pos;
        if (!ThrowToScreen(cam, info.pos, height, pos)) {
            ++it;
            continue;
        }

        const bool friendly = (localCamp > 0 && info.camp == localCamp);
        const ImU32 color = friendly ? IM_COL32(90, 255, 170, 255) : IM_COL32(255, 96, 48, 255);
        const ImU32 fill  = friendly ? IM_COL32(90, 255, 170, 190) : IM_COL32(255, 96, 48, 190);

        // Diamond marker: the through-wall highlight for the device.
        const float r = 7.0f;
        const ImVec2 top(pos.x, pos.y - r), right(pos.x + r, pos.y);
        const ImVec2 bottom(pos.x, pos.y + r), left(pos.x - r, pos.y);
        draw->AddQuadFilled(top, right, bottom, left, fill);
        draw->AddQuad(top, right, bottom, left, IM_COL32(0, 0, 0, 255), 1.5f);
        draw->AddCircleFilled(pos, 2.0f, IM_COL32(255, 255, 255, 255));

        // Nearby hostile devices pulse so they register in peripheral vision.
        if (!friendly && dist <= PlacableCfg::WarnDist) {
            float pulse = 11.0f + 3.5f * sinf((float)ImGui::GetTime() * 6.0f);
            draw->AddCircle(pos, pulse, color, 28, 1.6f);
        }

        char text[80];
        snprintf(text, sizeof(text), "%s | %dm", info.label, (int)dist);
        const float fontSize = 16.0f;
        ImVec2 textSize = ImGui::CalcTextSize(text);
        textSize.x *= (fontSize / ImGui::GetFontSize());
        DrawTextWithBorder1(draw, text, {pos.x - textSize.x * 0.5f, pos.y - 28.0f},
                            color, IM_COL32(0, 0, 0, 255), fontSize);

        ++it;
    }
}

inline void hook_StickTick(void *instance, float deltaTime)
{
    if (orig_StickTick)
        orig_StickTick(instance, deltaTime);
    TrackPlacable(instance);
}

inline void hook_TripMineTick(void *instance, float deltaTime)
{
    if (orig_TripMineTick)
        orig_TripMineTick(instance, deltaTime);
    TrackPlacable(instance);
}

inline void hook_TrapChipTick(void *instance, float deltaTime)
{
    if (orig_TrapChipTick)
        orig_TrapChipTick(instance, deltaTime);
    TrackPlacable(instance);
}

inline void hook_TripWireTick(void *instance, float deltaTime)
{
    if (orig_TripWireTick)
        orig_TripWireTick(instance, deltaTime);
    TrackPlacable(instance);
}
