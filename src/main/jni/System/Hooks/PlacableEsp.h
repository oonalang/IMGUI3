#pragma once

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <unordered_map>

// Placed Items ESP: tripmines, C4, trophy systems and trip sensors.
// Tracked from the projectile ticks and the game's live projectile list.

namespace PlacableCfg {
    // libunity.so RVAs, dump.cs GR .57
    constexpr uintptr_t StickTickRva    = 0x956D4CC; // WeaponProjectile_Stick.Tick(float)
    constexpr uintptr_t TripMineTickRva = 0x9B5B850; // WeaponProjectile_TripMine.Tick(float)
    constexpr uintptr_t TrapChipTickRva = 0x9B58A10; // WeaponProjectile_TrapChipMine.Tick(float)
    constexpr uintptr_t TripWireTickRva = 0xB201650; // WeaponProjectile_ElectricTripWire.Tick(float)

    // The game's live projectile table (route 3, no Tick involved).
    constexpr uintptr_t ProjectileListRva   = 0x9CB0490; // BaseGame.get_WeaponProjectileList()
    constexpr uintptr_t PushProjectileRva   = 0x9CBEE0C; // BaseGame.PushProjectile(WeaponProjectile)
    constexpr uintptr_t RemoveProjectileRva = 0x9CBDAC4; // BaseGame.RemoveWeaponProjectile(WeaponProjectile)
    constexpr int       MaxListScan         = 128;       // hard cap per frame

    // Position fallback through the device actor (see the file head).
    constexpr uintptr_t GrenadeActorOff          = 0x1C8; // WeaponProjectile.m_GrenadeActor
    constexpr uintptr_t HitColliderOff           = 0x110; // GrenadeActor.HitCollider
    constexpr uintptr_t ComponentGetTransformRva = 0x50291B0;
    constexpr uintptr_t TransformGetPositionRva  = 0x507C19C;

    // WeaponProjectile field offsets, dump.cs GR .57
    constexpr uintptr_t ExplodedOff  = 0xA1;  // bHasExplode
    constexpr uintptr_t DestroyedOff = 0x18D; // mIsDestroyed
    constexpr uintptr_t OwnerCampOff = 0xB0;  // m_SpawnOwnerCamp
    constexpr float MaxDist  = 100.0f;        // draw distance cap (m)
    constexpr float WarnDist = 12.0f;         // hostile pulse / warning distance (m)
    constexpr uint64_t ExpireFrames = 240;    // rendered frames without a refresh
}

// Which of the four Tick hooks this install landed. Written once by Feats.h
// right after the DobbyHook call (the hooker fills the original pointer only on
// success) and read by the ESP page.
enum PlacableHook
{
    PlcHookStick = 0,   // WeaponProjectile_Stick.Tick
    PlcHookTripMine,    // WeaponProjectile_TripMine.Tick
    PlcHookTrapChip,    // WeaponProjectile_TrapChipMine.Tick
    PlcHookTripWire,    // WeaponProjectile_ElectricTripWire.Tick
    PlcHookCount
};

inline bool g_placableHookLanded[PlcHookCount] = {false, false, false, false};

// Route 3's install verdict, same convention (set by Feats.h).
inline bool g_placablePushLanded = false;

inline void NotePlacableHookLanded(PlacableHook hook, bool landed)
{
    if (landed && hook >= 0 && hook < PlcHookCount) g_placableHookLanded[hook] = true;
}

inline void NotePlacablePushLanded(bool landed)
{
    if (landed) g_placablePushLanded = true;
}

// Which route delivered a tracking call. Only used for the diagnostics line.
enum PlacableRoute
{
    PlcRouteTick = 0,   // one of the Tick hooks / the base Tick sink
    PlcRoutePush,       // BaseGame.PushProjectile
    PlcRouteList,       // the per-frame walk of get_WeaponProjectileList()
    PlcRouteCount
};

// Game thread writes, render thread reads - plain counters, racy but harmless
// (same convention as the triggerbot's TriggerbotDiag).
struct PlacableDiag
{
    int  tickCalls;      // Tick routes delivered
    int  pushed;         // BaseGame.PushProjectile registrations
    int  scanned;        // live-list walk registrations
    int  classified;     // ... whose class name is a known device
    int  tracked;        // lifetime writes into the map
    int  drawn;          // markers emitted by the LAST draw pass
    int  nameFailed;     // class name unreadable (stale/moved object)
    int  posFailed;      // position unreadable by BOTH sources (LAST draw pass)
    char lastClass[48];  // last class name that classified ("" = none yet)
};

inline PlacableDiag g_placableDiag{};

struct PlacableInfo {
    const char *label;
    int camp;
    uint64_t seen;     // value of g_PlacableSeen at the last tracking write
    uint64_t frame;    // value of g_PlacableFrame at the last tracking write
};

// What the draw pass copies out (under the lock) and draws from (outside it).
struct PlacableDrawItem {
    uintptr_t key;     // the projectile instance
    const char *label;
    int camp;
};

inline std::unordered_map<uintptr_t, PlacableInfo> g_Placables;
inline std::mutex g_PlacableMutex;
inline uint64_t g_PlacableSeen = 0;
inline uint64_t g_PlacableFrame = 0;

inline void (*orig_StickTick)(void *, float) = nullptr;
inline void (*orig_TripMineTick)(void *, float) = nullptr;
inline void (*orig_TrapChipTick)(void *, float) = nullptr;
inline void (*orig_TripWireTick)(void *, float) = nullptr;

// Route 3 hooks (installed by Feats.h).
inline void (*orig_PushProjectile)(void *, void *) = nullptr;
inline bool (*orig_RemoveWeaponProjectile)(void *, void *) = nullptr;

// Test seam / lazy resolver for BaseGame.get_WeaponProjectileList(). Feats.h
// leaves it null and the first draw resolves it from the dump RVA; a host test
// points it at a fake table (there is no libunity.so on the host, so the real
// resolver answers 0).
inline void *(*g_placableListGetter)(void *game) = nullptr;

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

// Lower-case, drop every byte that is not a letter or a digit:
//   "WeaponProjectile_TripMine" -> "weaponprojectiletripmine"
// so the table below can be matched against it whatever the separator style,
// the namespace or the letter case a build happens to use.
inline void PlacableNormalise(const char *className, char *out, size_t outSize)
{
    size_t w = 0;
    if (outSize == 0) return;
    for (size_t i = 0; className[i] != '\0' && w + 1 < outSize; ++i) {
        const char c = className[i];
        if (c >= 'A' && c <= 'Z')      out[w++] = (char)(c - 'A' + 'a');
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out[w++] = c;
    }
    out[w] = '\0';
}

// Classification table. Every key is a NORMALISED substring; the exact dumped
// names come first so they always win, and the shortened forms after them let a
// class that dropped the "WeaponProjectile" prefix (or gained a namespace) still
// classify. Order inside each group is specific -> general.
inline const char *PlacableLabelFor(const char *className)
{
    if (className == nullptr || className[0] == '\0')
        return nullptr;

    struct Entry { const char *key; const char *label; };
    static const Entry kEntries[] = {
        // the names dumps/dump.cs lists for this build
        {"weaponprojectiletactripwire",      "TRIP SENSOR"},
        {"weaponprojectileelectrictripwire", "TRIP WIRE"},
        {"weaponprojectiletrapchipmine",     "TRAP CHIP MINE"},
        {"weaponprojectiletripmine",         "TRIP MINE"},
        {"weaponprojectiletrophysystem",     "TROPHY SYSTEM"},
        {"weaponprojectiletankbattlec4",     "C4"},
        {"weaponprojectiletankmine",         "TANK MINE"},
        {"weaponprojectilec4bomb",           "C4"},
        // the same devices under a shorter (renamed / re-namespaced) class name
        {"tactripwire",      "TRIP SENSOR"},
        {"electrictripwire", "TRIP WIRE"},
        {"trapchipmine",     "TRAP CHIP MINE"},
        {"tripmine",         "TRIP MINE"},
        {"trophysystem",     "TROPHY SYSTEM"},
        {"tankbattlec4",     "C4"},
        {"tankmine",         "TANK MINE"},
        {"c4bomb",           "C4"},
    };

    char norm[64];
    PlacableNormalise(className, norm, sizeof(norm));
    if (norm[0] == '\0')
        return nullptr;

    for (const Entry &e : kEntries)
        if (strstr(norm, e.key) != nullptr)
            return e.label;
    return nullptr;
}

inline void PlacableRememberClass(const char *className)
{
    size_t i = 0;
    for (; i + 1 < sizeof(g_placableDiag.lastClass) && className[i] != '\0'; ++i)
        g_placableDiag.lastClass[i] = className[i];
    g_placableDiag.lastClass[i] = '\0';
}

// Register / refresh one projectile. Deliberately does NOT read the position
// here: the push hook runs inside the game's spawn path (where a half-built
// simulator must not be dereferenced), and the Tick hooks run right after the
// game's own Tick has possibly destroyed the object. The draw pass resolves
// the position from the live object every frame instead (PlacableResolvePos),
// which is both safer and self-healing.
inline void TrackPlacable(void *proj, PlacableRoute route = PlcRouteTick)
{
    if (!PlacableEspOn())
        return;
    if (!Tools::IsPtrValid(proj))
        return;

    switch (route) {
        case PlcRoutePush: g_placableDiag.pushed++; break;
        case PlcRouteList: g_placableDiag.scanned++; break;
        default:           g_placableDiag.tickCalls++; break;
    }

    const uintptr_t base = (uintptr_t)proj;
    if (*(bool *)(base + PlacableCfg::ExplodedOff) || *(bool *)(base + PlacableCfg::DestroyedOff)) {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        g_Placables.erase(base);
        return;
    }

    char className[64];
    if (!PlacableClassName(proj, className, sizeof(className))) {
        g_placableDiag.nameFailed++;
        return;
    }

    const char *label = PlacableLabelFor(className);
    if (!label)
        return; // bullet / NA45 / thermite / grapple... not placed equipment

    g_placableDiag.classified++;
    PlacableRememberClass(className);

    const int camp = *(int *)(base + PlacableCfg::OwnerCampOff);

    std::lock_guard<std::mutex> lock(g_PlacableMutex);
    PlacableInfo &info = g_Placables[base];
    info.label = label;
    info.camp = camp;
    info.seen = ++g_PlacableSeen;
    info.frame = g_PlacableFrame;
    g_placableDiag.tracked++;
}

// The second route into the tracker: the BASE WeaponProjectile.Tick hook in
// ThrowableAlert.h calls this for every projectile that dispatches to the base
// entry point (see the file head). Same on/off gate, same checks.
inline void PlacableOnProjectileTick(void *proj)
{
    TrackPlacable(proj, PlcRouteTick);
}

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------

// A position is usable when it is finite, on the map, and not the all-zero
// value get_Position answers for an object whose simulator is gone.
inline bool PlacablePosUsable(const Vector3 &p)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        return false;
    if (fabsf(p.x) > 20000.0f || fabsf(p.y) > 20000.0f || fabsf(p.z) > 20000.0f)
        return false;
    return !(p.x == 0.0f && p.y == 0.0f && p.z == 0.0f);
}

// Fallback: the device actor's own collider. WeaponProjectile.m_GrenadeActor
// @0x1C8 -> GrenadeActor.HitCollider @0x110 -> Component.get_transform ->
// Transform.get_position. Used only when get_Position answered nothing usable.
inline Vector3 PlacablePosFromCollider(void *proj)
{
    if (!Tools::IsPtrValid(proj))
        return Vector3::zero();

    uintptr_t actor = *(uintptr_t *)((uintptr_t)proj + PlacableCfg::GrenadeActorOff);
    if (!Tools::IsPtrValid((void *)actor))
        return Vector3::zero();

    uintptr_t collider = *(uintptr_t *)(actor + PlacableCfg::HitColliderOff);
    if (!Tools::IsPtrValid((void *)collider))
        return Vector3::zero();

    using GetTransformFn = void *(*)(void *);
    using GetPositionFn  = Vector3 (*)(void *);
    static GetTransformFn getTransform = nullptr;
    static GetPositionFn  getPosition  = nullptr;
    // Resolve lazily and retry until it answers: a static that latched a 0
    // before libunity.so was mapped would disable this fallback for the rest
    // of the match.
    if (getTransform == nullptr)
        getTransform = (GetTransformFn)getAbsoluteAddress("libunity.so", PlacableCfg::ComponentGetTransformRva);
    if (getPosition == nullptr)
        getPosition = (GetPositionFn)getAbsoluteAddress("libunity.so", PlacableCfg::TransformGetPositionRva);
    if (getTransform == nullptr || getPosition == nullptr)
        return Vector3::zero();

    void *transform = getTransform((void *)collider);
    if (!Tools::IsPtrValid(transform))
        return Vector3::zero();

    return getPosition(transform);
}

inline Vector3 PlacableResolvePos(void *proj)
{
    if (!Tools::IsPtrValid(proj))
        return Vector3::zero();

    Vector3 pos = GetThrowPos(proj); // WeaponProjectile.get_Position (ThrowableAlert)
    if (PlacablePosUsable(pos))
        return pos;

    pos = PlacablePosFromCollider(proj);
    return PlacablePosUsable(pos) ? pos : Vector3::zero();
}

// ---------------------------------------------------------------------------
// Route 3: the game's own live-projectile table
// ---------------------------------------------------------------------------

// One walk of BaseGame.get_WeaponProjectileList(). Called from the draw pass
// (render thread) once per frame. Every read is validated and the count is
// clamped, so a table mutating under us costs a skipped entry, not a crash.
//
// Returns -1 when the table is not readable at all (getter not resolved, no
// match, no list) and the number of entries walked otherwise. The draw pass
// uses that distinction: only a table that WAS walked tells us an unrefreshed
// entry is really gone.
inline int PlacableScanLiveList()
{
    if (g_placableListGetter == nullptr) {
        using ListGetterFn = void *(*)(void *);
        g_placableListGetter = (ListGetterFn)getAbsoluteAddress("libunity.so", PlacableCfg::ProjectileListRva);
    }
    if (g_placableListGetter == nullptr)
        return -1;

    void *game = (void *)GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(game))
        return -1;

    Il2CppList<void *> *list = (Il2CppList<void *> *)g_placableListGetter(game);
    if (!Tools::IsPtrValid((void *)list))
        return -1;
    if (!Tools::IsPtrValid((void *)list->items))
        return -1;

    int count = list->size;
    if (count <= 0)
        return 0;
    if (count > PlacableCfg::MaxListScan)
        count = PlacableCfg::MaxListScan;

    void **items = list->items->getPointer();
    int seen = 0;
    for (int i = 0; i < count; ++i) {
        void *proj = items[i];
        if (!Tools::IsPtrValid(proj))
            continue;
        TrackPlacable(proj, PlcRouteList);
        ++seen;
    }
    return seen;
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

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

inline void DrawPlacableESP(ImDrawList *draw, int width, int height, const Vector3 &localPos)
{
    g_placableDiag.drawn = 0;
    g_placableDiag.posFailed = 0;

    if (!PlacableEspOn()) {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        if (!g_Placables.empty())
            g_Placables.clear();
        return;
    }

    ++g_PlacableFrame;

    // Route 3: even with every hook dead, the game's own table still lists
    // every live projectile. This is also what keeps label/camp fresh.
    const bool listWalked = PlacableScanLiveList() >= 0;

    // Expire, then copy out what is left: the game thread keeps writing the
    // live map (Tick / push / remove hooks), so iteration and drawing happen
    // on a snapshot instead of on the map itself.
    std::vector<PlacableDrawItem> items;
    {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        for (auto it = g_Placables.begin(); it != g_Placables.end();) {
            PlacableInfo &info = it->second;
            const uintptr_t key = it->first;

            // Expiry: 360 tracking writes (the original rule) or - only when
            // the game's table was actually walked this frame - 4 s of rendered
            // frames without a refresh, which catches a device the game removed
            // without ever reaching the Remove hook. When the table is NOT
            // readable the frame rule is skipped, so a push-only route never
            // loses a live device just because nothing re-observed it.
            if (g_PlacableSeen > info.seen + 360 ||
                (listWalked && g_PlacableFrame > info.frame + PlacableCfg::ExpireFrames)) {
                it = g_Placables.erase(it);
                continue;
            }

            // The device itself says whether it is gone. Cheaper and safer than
            // calling get_Position on an object the game has already exploded.
            if (*(bool *)(key + PlacableCfg::ExplodedOff) || *(bool *)(key + PlacableCfg::DestroyedOff)) {
                it = g_Placables.erase(it);
                continue;
            }

            items.push_back(PlacableDrawItem{key, info.label, info.camp});
            ++it;
        }
    }

    Camera *cam = Camera::get_main();
    if (!cam)
        return;

    const int localCamp = PlacableLocalCamp();

    // Nearest hostile device for the on-screen warning below.
    const char *warnLabel = nullptr;
    float warnDist = 0.0f;

    for (const PlacableDrawItem &item : items) {
        const Vector3 pos = PlacableResolvePos((void *)item.key);
        if (pos == Vector3::zero()) {
            g_placableDiag.posFailed++;
            continue;
        }

        const float dist = Vector3::Distance(localPos, pos);
        if (dist > PlacableCfg::MaxDist)
            continue;

        const bool friendly = (localCamp > 0 && item.camp == localCamp);
        if (!friendly && dist <= PlacableCfg::WarnDist && (warnLabel == nullptr || dist < warnDist)) {
            warnLabel = item.label;
            warnDist = dist;
        }

        ImVec2 screen;
        if (!ThrowToScreen(cam, pos, height, screen)) {
            continue;
        }

        const ImU32 color = friendly ? IM_COL32(90, 255, 170, 255) : IM_COL32(255, 96, 48, 255);
        const ImU32 fill  = friendly ? IM_COL32(90, 255, 170, 190) : IM_COL32(255, 96, 48, 190);

        // Diamond marker: the through-wall highlight for the device.
        const float r = friendly ? 6.0f : 8.0f;
        const ImVec2 top(screen.x, screen.y - r), right(screen.x + r, screen.y);
        const ImVec2 bottom(screen.x, screen.y + r), left(screen.x - r, screen.y);
        draw->AddQuadFilled(top, right, bottom, left, fill);
        draw->AddQuad(top, right, bottom, left, IM_COL32(0, 0, 0, 255), 1.5f);
        draw->AddCircleFilled(screen, 2.0f, IM_COL32(255, 255, 255, 255));

        // Nearby hostile devices pulse so they register in peripheral vision.
        if (!friendly && dist <= PlacableCfg::WarnDist) {
            float pulse = 11.0f + 3.5f * sinf((float)ImGui::GetTime() * 6.0f);
            draw->AddCircle(screen, pulse, color, 28, 1.6f);
        }

        char text[80];
        snprintf(text, sizeof(text), "%s | %dm", item.label, (int)dist);
        const float fontSize = 16.0f;
        ImVec2 textSize = ImGui::CalcTextSize(text);
        textSize.x *= (fontSize / ImGui::GetFontSize());
        DrawTextWithBorder1(draw, text, {screen.x - textSize.x * 0.5f, screen.y - 28.0f},
                            color, IM_COL32(0, 0, 0, 255), fontSize);

        g_placableDiag.drawn++;
    }

    // A device under your feet still warns when it is off-screen or behind
    // you - which is exactly the trip sensor / C4 case that "nothing showed"
    // ended with a respawn. Fixed position under the crosshair, one line.
    if (warnLabel != nullptr) {
        char warn[96];
        snprintf(warn, sizeof(warn), "%s  %.0fm", warnLabel, warnDist);
        const float fontSize = 20.0f;
        ImVec2 textSize = ImGui::CalcTextSize(warn);
        textSize.x *= (fontSize / ImGui::GetFontSize());
        DrawTextWithBorder1(draw, warn,
                            {((float)width - textSize.x) * 0.5f, (float)height * 0.5f + 70.0f},
                            IM_COL32(255, 96, 48, 255), IM_COL32(0, 0, 0, 255), fontSize);
    }
}

// ---------------------------------------------------------------------------
// Hooks
// ---------------------------------------------------------------------------

inline void hook_StickTick(void *instance, float deltaTime)
{
    if (orig_StickTick)
        orig_StickTick(instance, deltaTime);
    TrackPlacable(instance, PlcRouteTick);
}

inline void hook_TripMineTick(void *instance, float deltaTime)
{
    if (orig_TripMineTick)
        orig_TripMineTick(instance, deltaTime);
    TrackPlacable(instance, PlcRouteTick);
}

inline void hook_TrapChipTick(void *instance, float deltaTime)
{
    if (orig_TrapChipTick)
        orig_TrapChipTick(instance, deltaTime);
    TrackPlacable(instance, PlcRouteTick);
}

inline void hook_TripWireTick(void *instance, float deltaTime)
{
    if (orig_TripWireTick)
        orig_TripWireTick(instance, deltaTime);
    TrackPlacable(instance, PlcRouteTick);
}

// BaseGame.PushProjectile(WeaponProjectile): the game's own registration point
// for the live-projectile table. Runs after the original so the object is
// fully in the table; reads nothing that the spawn path might not have built
// yet (see TrackPlacable).
inline void hook_PushProjectile(void *game, void *projectile)
{
    if (orig_PushProjectile)
        orig_PushProjectile(game, projectile);
    TrackPlacable(projectile, PlcRoutePush);
}

// BaseGame.RemoveWeaponProjectile(WeaponProjectile): drop the marker the
// moment the game drops the projectile, instead of waiting for expiry.
inline bool hook_RemoveWeaponProjectile(void *game, void *projectile)
{
    const bool removed = orig_RemoveWeaponProjectile
                             ? orig_RemoveWeaponProjectile(game, projectile)
                             : false;
    if (removed && projectile != nullptr) {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        g_Placables.erase((uintptr_t)projectile);
    }
    return removed;
}
