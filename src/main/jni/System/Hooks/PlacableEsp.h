#pragma once

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "../../Includes/Logger.h"

namespace PlacableCfg {
    constexpr uintptr_t StickTickRva    = 0x956D4CC;
    constexpr uintptr_t TripMineTickRva = 0x9B5B850;
    constexpr uintptr_t TrapChipTickRva = 0x9B58A10;
    constexpr uintptr_t TripWireTickRva = 0xB201650;

    constexpr uintptr_t ProjectileListRva   = 0x9CB0490;
    constexpr uintptr_t PushProjectileRva   = 0x9CBEE0C;
    constexpr uintptr_t RemoveProjectileRva = 0x9CBDAC4;
    constexpr int       MaxListScan         = 128;

    constexpr uintptr_t GrenadeActorOff          = 0x1C8;
    constexpr uintptr_t HitColliderOff           = 0x110;
    constexpr uintptr_t ComponentGetTransformRva = 0x50291B0;
    constexpr uintptr_t TransformGetPositionRva  = 0x507C19C;

    constexpr uintptr_t DestroyedOff = 0x18D;
    constexpr uintptr_t OwnerCampOff = 0xB0;
    constexpr float MaxDist  = 100.0f;
    constexpr float WarnDist = 12.0f;
    constexpr uint64_t ExpireFrames = 240;
}

enum PlacableHook
{
    PlcHookStick = 0,
    PlcHookTripMine,
    PlcHookTrapChip,
    PlcHookTripWire,
    PlcHookCount
};

inline bool g_placableHookLanded[PlcHookCount] = {false, false, false, false};

inline bool g_placablePushLanded = false;

inline void NotePlacableHookLanded(PlacableHook hook, bool landed)
{
    if (landed && hook >= 0 && hook < PlcHookCount) g_placableHookLanded[hook] = true;
}

inline void NotePlacablePushLanded(bool landed)
{
    if (landed) g_placablePushLanded = true;
}

enum PlacableRoute
{
    PlcRouteTick = 0,
    PlcRoutePush,
    PlcRouteList,
    PlcRouteCount
};

struct PlacableDiag
{
    int  tickCalls;
    int  pushed;
    int  scanned;
    int  classified;
    int  tracked;
    int  drawn;
    int  nameFailed;
    int  posFailed;
    char lastClass[48];
};

inline PlacableDiag g_placableDiag{};
inline double g_placableDiagStamp = -1000.0;

struct PlacableInfo {
    const char *label;
    int camp;
    uint64_t seen;
    uint64_t frame;
};

struct PlacableDrawItem {
    uintptr_t key;
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

inline void (*orig_PushProjectile)(void *, void *) = nullptr;
inline bool (*orig_RemoveWeaponProjectile)(void *, void *) = nullptr;

inline void *(*g_placableListGetter)(void *game) = nullptr;

inline bool PlacableEspOn()
{
    return Config.ESPMenu.Placables;
}

inline bool PlacableReadBool(uintptr_t base, uintptr_t offset, bool &out)
{
    const uintptr_t address = base + offset;
    if (address < base || !Tools::IsPtrValid((void *)address))
        return false;
    out = *(const bool *)address;
    return true;
}

inline bool PlacableReadInt(uintptr_t base, uintptr_t offset, int &out)
{
    const uintptr_t address = base + offset;
    if (address < base || !Tools::IsPtrValid((void *)address))
        return false;
    out = *(const int *)address;
    return true;
}

inline bool PlacableReadPtr(uintptr_t base, uintptr_t offset, uintptr_t &out)
{
    const uintptr_t address = base + offset;
    if (address < base || !Tools::IsPtrValid((void *)address))
        return false;
    out = *(const uintptr_t *)address;
    return true;
}

inline bool PlacableClassName(void *proj, char *out, size_t outSize)
{
    if (!Tools::IsPtrValid(proj) || outSize < 2)
        return false;

    uintptr_t klass = *(uintptr_t *)proj;
    if (!Tools::IsPtrValid((void *)klass))
        return false;

    const char *name = *(const char **)(klass + 0x10);
    if (!Tools::IsPtrValid((void *)name))
        return false;

    size_t i = 0;
    for (; i + 1 < outSize; ++i) {
        char c = name[i];
        if (c == '\0')
            break;
        if (c < 0x20 || c > 0x7E)
            return false;
        out[i] = c;
    }
    out[i] = '\0';
    return i > 0;
}

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

inline const char *PlacableLabelFor(const char *className)
{
    if (className == nullptr || className[0] == '\0')
        return nullptr;

    struct Entry { const char *key; const char *label; };
    static const Entry kEntries[] = {
        {"weaponprojectiletactripwire",      "TRIP SENSOR"},
        {"weaponprojectileelectrictripwire", "TRIP WIRE"},
        {"weaponprojectiletrapchipmine",     "TRAP CHIP MINE"},
        {"weaponprojectiletripmine",         "TRIP MINE"},
        {"weaponprojectiletrophysystem",     "TROPHY SYSTEM"},
        {"weaponprojectiletankbattlec4",     "C4"},
        {"weaponprojectiletankmine",         "TANK MINE"},
        {"weaponprojectilec4bomb",           "C4"},

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
    bool destroyed = false;
    if (!PlacableReadBool(base, PlacableCfg::DestroyedOff, destroyed))
        return;
    if (WeaponProjectileHasExploded(proj) || destroyed) {
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
        return;

    g_placableDiag.classified++;
    PlacableRememberClass(className);

    int camp = 0;
    if (!PlacableReadInt(base, PlacableCfg::OwnerCampOff, camp))
        return;

    std::lock_guard<std::mutex> lock(g_PlacableMutex);
    PlacableInfo &info = g_Placables[base];
    info.label = label;
    info.camp = camp;
    info.seen = ++g_PlacableSeen;
    info.frame = g_PlacableFrame;
    g_placableDiag.tracked++;
}

inline void PlacableOnProjectileTick(void *proj)
{
    TrackPlacable(proj, PlcRouteTick);
}

inline bool PlacablePosUsable(const Vector3 &p)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        return false;
    if (fabsf(p.x) > 20000.0f || fabsf(p.y) > 20000.0f || fabsf(p.z) > 20000.0f)
        return false;
    return !(p.x == 0.0f && p.y == 0.0f && p.z == 0.0f);
}

inline Vector3 PlacablePosFromCollider(void *proj)
{
    if (!Tools::IsPtrValid(proj))
        return Vector3::zero();

    uintptr_t actor = 0;
    if (!PlacableReadPtr((uintptr_t)proj, PlacableCfg::GrenadeActorOff, actor) ||
        !Tools::IsPtrValid((void *)actor))
        return Vector3::zero();

    uintptr_t collider = 0;
    if (!PlacableReadPtr(actor, PlacableCfg::HitColliderOff, collider) ||
        !Tools::IsPtrValid((void *)collider))
        return Vector3::zero();

    using GetTransformFn = void *(*)(void *);
    using GetPositionFn  = Vector3 (*)(void *);
    static GetTransformFn getTransform = nullptr;
    static GetPositionFn  getPosition  = nullptr;

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

    Vector3 pos = GetThrowPos(proj);
    if (PlacablePosUsable(pos))
        return pos;

    pos = PlacablePosFromCollider(proj);
    return PlacablePosUsable(pos) ? pos : Vector3::zero();
}

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

    uintptr_t info = 0;
    if (!PlacableReadPtr((uintptr_t)local, Class_AttackableTarget_m_AttackableInfo, info) ||
        !Tools::IsPtrValid((void *)info))
        return -1;

    int camp = 0;
    return PlacableReadInt(info, 0x58, camp) ? camp : -1;
}

inline void DrawPlacableESP(ImDrawList *draw, int width, int height, const Vector3 &localPos)
{
    fault_guard::NotePhase("placable draw");
    g_placableDiag.drawn = 0;
    g_placableDiag.posFailed = 0;

    if (!PlacableEspOn()) {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        if (!g_Placables.empty())
            g_Placables.clear();
        return;
    }

    ++g_PlacableFrame;

    const bool listWalked = PlacableScanLiveList() >= 0;

    std::vector<PlacableDrawItem> items;
    {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        for (auto it = g_Placables.begin(); it != g_Placables.end();) {
            PlacableInfo &info = it->second;
            const uintptr_t key = it->first;

            if (g_PlacableSeen > info.seen + 360 ||
                (listWalked && g_PlacableFrame > info.frame + PlacableCfg::ExpireFrames)) {
                it = g_Placables.erase(it);
                continue;
            }

            bool destroyed = false;
            if (!PlacableReadBool(key, PlacableCfg::DestroyedOff, destroyed) || destroyed ||
                WeaponProjectileHasExploded((void *)key)) {
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

        const float r = friendly ? 6.0f : 8.0f;
        const ImVec2 top(screen.x, screen.y - r), right(screen.x + r, screen.y);
        const ImVec2 bottom(screen.x, screen.y + r), left(screen.x - r, screen.y);
        draw->AddQuadFilled(top, right, bottom, left, fill);
        draw->AddQuad(top, right, bottom, left, IM_COL32(0, 0, 0, 255), 1.5f);
        draw->AddCircleFilled(screen, 2.0f, IM_COL32(255, 255, 255, 255));

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

    // One line every five seconds while the feature is on. Placed items are
    // rare, so there is no other way to tell a working esp from a scan that
    // never finds anything: this says which of the two it is.
    const double nowSec = (double)ImGui::GetTime();
    if (nowSec - g_placableDiagStamp >= 5.0) {
        g_placableDiagStamp = nowSec;
        LOGI("[Placable] tracked=%d drawn=%d posFail=%d scanned=%d list=%d ticks=%d class=%s",
             (int)items.size(), g_placableDiag.drawn, g_placableDiag.posFailed,
             g_placableDiag.scanned, listWalked ? 1 : 0, g_placableDiag.tickCalls,
             g_placableDiag.lastClass[0] ? g_placableDiag.lastClass : "-");
    }

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

inline void hook_PushProjectile(void *game, void *projectile)
{
    if (orig_PushProjectile)
        orig_PushProjectile(game, projectile);
    TrackPlacable(projectile, PlcRoutePush);
}

inline bool hook_RemoveWeaponProjectile(void *game, void *projectile)
{
    const bool removed = orig_RemoveWeaponProjectile
                             ? orig_RemoveWeaponProjectile(game, projectile)
                             : false;
    if (removed && projectile != nullptr && Tools::IsPtrValid(projectile)) {
        std::lock_guard<std::mutex> lock(g_PlacableMutex);
        g_Placables.erase((uintptr_t)projectile);
    }
    return removed;
}

