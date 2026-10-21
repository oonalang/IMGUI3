#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "Hacks/Feature/NewFeatures.h"

bool Tools::IsPtrValid(void *addr) { return addr != nullptr; }

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

using namespace NewFeatures;

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

static const size_t OBJ_SIZE = 2048;
struct FakeObj { alignas(8) unsigned char b[OBJ_SIZE]; };

static FakeObj g_localRoot, g_targetRoot, g_wallRoot, g_trigRoot, g_deadRoot;
static FakeObj g_localPawn, g_targetPawn, g_otherPawn;
static FakeObj g_localHead, g_localMesh, g_targetHead, g_targetMesh;
static FakeObj g_selfCol, g_targetCol, g_wallCol, g_trigCol, g_deadCol;
static FakeObj g_camObj, g_camTransform, g_gameObj, g_weaponObj;

static unsigned char g_fakeKlass[0x20];
static const char g_instantName[] = "WeaponFireComponent_Instant";
static FakeObj        g_fireCompObj;

static void objInit(FakeObj &o, FakeObj *identity = nullptr)
{
    std::memset(o.b, 0, sizeof o.b);
    *(void **)(o.b + 0x10) = o.b;
    *(void **)(o.b + 0x38) = identity ? identity->b : o.b;
}
static void objParent(FakeObj &t, FakeObj *p)  { *(void **)(t.b + 0x48) = p ? p->b : nullptr; }
static void objPos(FakeObj &o, float x, float y, float z)
{
    Vector3 v{x, y, z};
    std::memcpy(o.b + 0x100, &v, sizeof v);
}
static Vector3 objGetPos(const FakeObj &o)
{
    Vector3 v;
    std::memcpy(&v, o.b + 0x100, sizeof v);
    return v;
}

static void objTargetUnderDot(float dist = 3.3f)
{
    objPos(g_targetHead, 0.0f, 1.7f, dist);
    objPos(g_targetMesh, 0.0f, 0.0f, dist);
    objPos(g_targetRoot, 0.0f, 0.0f, dist);
}

static void objSetAlive(FakeObj &pawn, bool alive)
{
    pawn.b[Class_Pawn_m_IsAlive] = alive ? 1 : 0;
}
static void objSetBone(FakeObj &pawn, size_t offset, FakeObj *bone)
{
    *(void **)(pawn.b + offset) = bone ? bone->b : nullptr;
}

static Transform *fake_get_transform(Component *c)
{
    return (Transform *)(*(void **)((char *)c + 0x38));
}
static Vector3 fake_get_position(Transform *t)
{
    Vector3 v;
    std::memcpy(&v, (char *)t + 0x100, sizeof v);
    return v;
}
static void *fake_get_parent(void *t)
{
    void *p = *(void **)((char *)t + 0x48);
    return p;
}
static Vector3 fake_get_forward(void *) { return Vector3{0.0f, 0.0f, 1.0f}; }

static Vector3 fake_get_right(void *)   { return Vector3{1.0f, 0.0f, 0.0f}; }
static Vector3 fake_get_up(void *)      { return Vector3{0.0f, 1.0f, 0.0f}; }

static Vector3 fake_headPosition(Pawn *p)
{
    void *bone = *(void **)((char *)p + Class_Pawn_m_HeadBone);
    if (bone == nullptr) return Vector3{0.0f, 0.0f, 0.0f};
    return fake_get_position((Transform *)bone);
}

static Camera *fake_camera_main() { return (Camera *)g_camObj.b; }
static int fake_screen_w() { return 1600; }
static int fake_screen_h() { return 900; }

static Vector3 (*g_w2sImpl)(const Vector3 &) = nullptr;
static Vector3 fake_w2s(void *, Vector3 w)
{
    return g_w2sImpl ? g_w2sImpl(w) : Vector3{0.0f, 0.0f, 0.0f};
}

static Vector3 fake_screen_to_world(void *, Vector3 v)
{
    const float f = (900.0f * 0.5f) /
                    tanf(NewFeatures::A_FireTune::kFallbackFov * 0.5f *
                         NewFeatures::A_FireTune::kDegToRad);
    const float dx = v.x - 800.0f;
    const float dy = v.y - 450.0f;
    Vector3 dir = Vector3{0.0f, 0.0f, 1.0f}
                + Vector3{1.0f, 0.0f, 0.0f} * (dx / f)
                - Vector3{0.0f, 1.0f, 0.0f} * (dy / f);
    dir = Vector3::Normalized(dir);
    return Vector3{0.0f + dir.x * v.z, 1.6f + dir.y * v.z, 0.0f + dir.z * v.z};
}

static std::vector<std::pair<float, FakeObj *>> g_centreHits;
static std::vector<std::pair<float, FakeObj *>> g_losHits;
static int   g_segmentHit = 0;
static int   g_rayAllCalls = 0, g_singleRayCalls = 0, g_segmentCalls = 0;
static float g_lastRayAllMax = -1.0f;

static unsigned char g_hitArrayBuf[0x20 + 64 * 0x30 + 64];

static const std::vector<std::pair<float, FakeObj *>> &hitsForMaxDist(float maxDist)
{
    return (maxDist > 100.0f) ? g_centreHits : g_losHits;
}

static void *fake_raycastAll(Vector3, Vector3, float maxDist)
{
    g_rayAllCalls++;
    g_lastRayAllMax = maxDist;
    const std::vector<std::pair<float, FakeObj *>> &hits = hitsForMaxDist(maxDist);
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = hits.size();
    for (size_t i = 0; i < hits.size(); i++) {
        unsigned char *e = g_hitArrayBuf + AF_ARRAY_DATA_OFF + i * AF_RAYHIT_SIZE;
        *(float *)(e + AF_RAYHIT_DISTOFF) = hits[i].first;
        *(void **)(e + AF_RAYHIT_COLLOFF) = hits[i].second->b;
    }
    return g_hitArrayBuf;
}

static bool fake_raycastHitOut(Vector3, Vector3, void *hitBuf, float maxDist)
{
    g_singleRayCalls++;
    const std::vector<std::pair<float, FakeObj *>> &hits = hitsForMaxDist(maxDist);
    if (hits.empty()) return false;
    std::memset(hitBuf, 0, AF_RAYHIT_SIZE);
    *(float *)((unsigned char *)hitBuf + AF_RAYHIT_DISTOFF) = hits[0].first;
    *(void **)((unsigned char *)hitBuf + AF_RAYHIT_COLLOFF) = hits[0].second->b;
    return true;
}

static bool fake_raySegment(Vector3, Vector3, float)
{
    g_segmentCalls++;
    return g_segmentHit != 0;
}

static int g_isTriggerFlag = 0;
static bool fake_isTrigger(void *) { return g_isTriggerFlag != 0; }

static int g_lastOccVisible = 1;
static bool fake_lastOccVisible(void *) { return g_lastOccVisible != 0; }
static int g_recentlyRendered = 1;
static bool fake_recentlyRendered(void *) { return g_recentlyRendered != 0; }

static void resetWorld()
{
    objInit(g_localRoot);   objInit(g_targetRoot);
    objInit(g_wallRoot);    objInit(g_trigRoot);   objInit(g_deadRoot);

    objInit(g_localPawn,  &g_localRoot);
    objInit(g_targetPawn, &g_targetRoot);
    objInit(g_otherPawn);
    objInit(g_localHead);  objInit(g_localMesh);
    objInit(g_targetHead); objInit(g_targetMesh);

    objInit(g_selfCol,  &g_localRoot);
    objInit(g_targetCol, &g_targetRoot);
    objInit(g_wallCol,  &g_wallRoot);
    objInit(g_trigCol,  &g_trigRoot);
    objInit(g_deadCol,  &g_deadRoot);
    *(void **)(g_deadCol.b + 0x10) = nullptr;

    objInit(g_camObj, &g_camTransform);
    objInit(g_camTransform);
    objPos(g_camTransform, 0.0f, 1.6f, 0.0f);
    objInit(g_gameObj);
    objInit(g_weaponObj);

    NewFeatures::g_afireAdsLastState   = -1;
    NewFeatures::g_afireAdsSettleUntil = -1000.0;
    NewFeatures::g_afireAdsSettleMax   = -1000.0;
    NewFeatures::g_afireAdsLastComp    = nullptr;
    NewFeatures::g_afireAdsCompSeen    = false;
    NewFeatures::g_afireAdsSettling    = false;
    NewFeatures::g_afireReleasePending = false;

    objSetBone(g_localPawn,  Class_Pawn_m_HeadBone, &g_localHead);
    objSetBone(g_localPawn,  Class_Pawn_m_Mesh,     &g_localMesh);
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, &g_targetHead);
    objSetBone(g_targetPawn, Class_Pawn_m_Mesh,     &g_targetMesh);
    objSetAlive(g_localPawn, true);
    objSetAlive(g_targetPawn, true);
    objSetAlive(g_otherPawn, true);

    objPos(g_targetHead, 0.5f, 1.7f, 3.3f);
    objPos(g_targetMesh, 0.5f, 0.0f, 3.3f);

    objPos(g_targetRoot, 0.5f, 0.0f, 3.3f);
    objPos(g_localRoot,  0.0f, 0.0f, 0.0f);
    objParent(g_wallRoot, nullptr);
    objParent(g_trigRoot, nullptr);

    g_centreHits.clear();
    g_losHits.clear();
    g_segmentHit = 0;
    g_isTriggerFlag = 0;
    g_lastOccVisible = 1;
    g_recentlyRendered = 1;
    g_rayAllCalls = g_singleRayCalls = g_segmentCalls = 0;
    g_lastRayAllMax = -1.0f;
    g_triggerDiag = TriggerbotDiag{};
}

static void installFns()
{
    m_unity = 0;
    api7  = (uintptr_t)&fake_camera_main;
    api9  = (uintptr_t)&fake_get_transform;
    api10 = (uintptr_t)&fake_get_position;
    api18 = (uintptr_t)&fake_headPosition;
    api4  = (uintptr_t)&fake_screen_w;
    api5  = (uintptr_t)&fake_screen_h;

    fn_PhysicsRaycastAll      = fake_raycastAll;
    fn_PhysicsRaycastHitOut   = fake_raycastHitOut;
    fn_PhysicsRaycastSegment  = fake_raySegment;
    fn_ColliderIsTrigger      = fake_isTrigger;
    fn_TransformGetParent     = fake_get_parent;
    fn_TransformGetForward    = fake_get_forward;
    fn_TransformGetRight      = fake_get_right;
    fn_TransformGetUp         = fake_get_up;
    fn_PawnLastOccVisible     = fake_lastOccVisible;
    fn_PawnRecentlyRendered   = fake_recentlyRendered;
    fn_WorldToScreen          = fake_w2s;
    fn_ScreenToWorldPoint     = fake_screen_to_world;
    fn_PawnIsAlive            = nullptr;
    fn_CameraGetFov           = nullptr;
    fn_CameraGetMain          = nullptr;
    fn_WeaponGetAimingTarget  = nullptr;
    fn_WeaponGetAimingTargetDis = nullptr;
    fn_BaseGameGetTeamPawns   = nullptr;

    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    fn_WeaponCanFire                 = nullptr;
    fn_WeaponRealFireInterval        = nullptr;
    fn_WeaponIsAutoFire              = nullptr;

    Config.Crosshair.Enabled = true;
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
    Config.Crosshair.Size    = 80.0f;
}

static void testGate()
{
    const Vector3 from(0.0f, 1.6f, 0.0f);
    const Vector3 to(0.5f, 1.7f, 3.3f);
    const float bodyRadius = A_FireTune::kBodyRadius;

    resetWorld();
    {
        bool ident = true; float hd = 5.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear && !ident && hd < 0.0f,
              "T1 empty line is clear, nothing identified");
        CHECK(g_rayAllCalls == 1, "T1 used RaycastAll (the walk, not the single ray)");
    }

    resetWorld();
    g_losHits = { {3.3f, &g_targetCol} };
    {
        bool ident = false; float hd = -1.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear && ident, "T2 nearest hit IS the target -> clear + identified");
        CHECK(std::fabs(hd - 3.3f) < 0.001f, "T2 hit distance reported for the panel");
    }

    resetWorld();
    g_losHits = { {0.0f, &g_selfCol} };
    {
        bool ident = true; float hd = 99.0f;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, &hd);
        CHECK(r == AF_Ray::Clear, "T3 own body at the camera no longer blocks the shot");
        CHECK(!ident, "T3 not 'identified' - the ray never reached the enemy");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T3 self hit skipped + counted");
        CHECK(g_triggerDiag.occOverrides == 0, "T3 no occlusion override needed (line is simply empty)");
        CHECK(g_singleRayCalls == 0, "T3 answered from RaycastAll alone");
    }

    resetWorld();
    g_losHits = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident,
              "T4 walk steps over our body and names the enemy behind it");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T4 exactly one self hit skipped");
    }

    resetWorld();
    g_losHits = { {0.0f, &g_selfCol}, {2.0f, &g_wallCol} };
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T5 wall at range still blocks even with the self hit skipped");
        CHECK(g_triggerDiag.occOverrides == 0, "T5 occlusion does NOT rescue a blocker at range");
    }

    resetWorld();
    g_losHits = { {0.05f, &g_trigCol} };
    g_isTriggerFlag = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T6 trigger volume cannot be 'the wall'");
        CHECK(g_triggerDiag.triggerHitsSkipped == 1, "T6 trigger hit skipped + counted");
    }

    resetWorld();
    g_losHits = { {0.05f, &g_trigCol}, {3.3f, &g_targetCol} };
    g_isTriggerFlag = 1;
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident, "T7 trigger skipped, enemy behind it still named");
    }

    resetWorld();
    g_losHits = { {0.10f, &g_wallCol} };
    g_lastOccVisible = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T8 near-origin blocker + LastOccVisible -> clear");
        CHECK(g_triggerDiag.occOverrides == 1, "T8 override counted for the panel");
    }

    resetWorld();
    g_losHits = { {0.10f, &g_wallCol} };
    g_lastOccVisible = 0;
    g_recentlyRendered = 0;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T8b near-origin blocker + occluded -> blocked");
        CHECK(g_triggerDiag.occOverrides == 0, "T8b no override when the game agrees he is hidden");
    }

    resetWorld();
    g_losHits = { {2.0f, &g_wallCol} };
    g_lastOccVisible = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T9 occlusion never overrides a normal-range wall");
    }

    resetWorld();
    g_losHits = { {3.0f, &g_deadCol} };
    g_segmentHit = 1;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T10 unnamed hit + something inside the stem range -> blocked");
    }
    resetWorld();
    g_losHits = { {3.0f, &g_deadCol} };
    g_segmentHit = 0;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Clear, "T10b unnamed hit but the stem range is empty -> clear");
    }

    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {0.0f, &g_selfCol} };
    g_lastOccVisible = 1;
    {
        bool ident = true;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && !ident,
              "T11 fallback: single ray self-hit + visible -> clear (occlusion escape)");
        CHECK(g_singleRayCalls == 1, "T11 fallback really used the single-ray symbol");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "T11 self hit counted on the fallback too");
    }
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {2.0f, &g_wallCol} };
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Blocked, "T11b fallback: a wall at range still blocks");
    }
    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    g_losHits = { {3.3f, &g_targetCol} };
    {
        bool ident = false;
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, &ident, nullptr);
        CHECK(r == AF_Ray::Clear && ident, "T11c fallback: the enemy himself is still identified");
    }
    fn_PhysicsRaycastAll = fake_raycastAll;

    resetWorld();
    fn_PhysicsRaycastAll = nullptr;
    fn_PhysicsRaycastHitOut = nullptr;
    fn_PhysicsRaycastSegment = nullptr;
    {
        AF_Ray r = A_FireRayToTarget(from, to, (Pawn *)g_targetPawn.b, bodyRadius,
                                     (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(r == AF_Ray::Unknown, "T12 no symbols at all -> Unknown, caller fails closed");
    }
    fn_PhysicsRaycastAll = fake_raycastAll;
    fn_PhysicsRaycastHitOut = fake_raycastHitOut;
    fn_PhysicsRaycastSegment = fake_raySegment;
}

static void testArrayGuards()
{
    AF_NamedHit h{};

    resetWorld();
    CHECK(!A_FireRaycastAllNearest(nullptr, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G1 null RaycastAll result is refused, not read as 'no hits'");

    resetWorld();
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = 100000;
    CHECK(!A_FireRaycastAllNearest(g_hitArrayBuf, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G2 out-of-range length -> refused (degrades to the single-ray path)");

    resetWorld();
    std::memset(g_hitArrayBuf, 0, sizeof g_hitArrayBuf);
    *(size_t *)(g_hitArrayBuf + AF_ARRAY_LEN_OFF) = 1;
    {
        unsigned char *e = g_hitArrayBuf + AF_ARRAY_DATA_OFF;
        *(float *)(e + AF_RAYHIT_DISTOFF) = 999999.0f;
        *(void **)(e + AF_RAYHIT_COLLOFF) = g_wallCol.b;
    }
    CHECK(!A_FireRaycastAllNearest(g_hitArrayBuf, 10.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G3 impossible hit distance -> refused");

    resetWorld();
    g_losHits = { {4.0f, &g_wallCol}, {1.5f, &g_wallCol}, {3.0f, &g_targetCol} };
    void *arr = fake_raycastAll(Vector3{}, Vector3{}, 5.0f);
    CHECK(A_FireRaycastAllNearest(arr, 5.0f, (Pawn *)g_targetPawn.b, (Pawn *)g_localPawn.b, h),
          "G4 well-formed array accepted");
    CHECK(!h.isTarget && h.distance > 1.4f && h.distance < 1.6f,
          "G4 unsorted records are walked nearest-first (1.5 m wall, not 4 m)");
}

static void testCentreRay()
{
    Pawn *items[2] = { (Pawn *)g_targetPawn.b, (Pawn *)g_otherPawn.b };
    const Vector3 camPos(0.0f, 1.6f, 0.0f);
    const Vector3 forward(0.0f, 0.0f, 1.0f);

    resetWorld();
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    {
        float hd = -1.0f; bool missing = false;
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, &hd, &missing);
        CHECK(p == (Pawn *)g_targetPawn.b, "C1 centre ray steps over our body and names the enemy");
        CHECK(hd > 4.9f && hd < 5.1f, "C1 centre hit distance is the enemy's");
        CHECK(g_triggerDiag.selfHitsSkipped == 1, "C1 self hit counted");
    }

    resetWorld();
    g_centreHits = { {0.02f, &g_selfCol} };
    {
        float hd = 7.0f; bool missing = false;
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, &hd, &missing);
        CHECK(p == nullptr && hd < 0.0f,
              "C2 centre ray on empty space (self skipped) -> no centred pawn");
    }

    resetWorld();
    g_centreHits = { {2.0f, &g_wallCol}, {5.0f, &g_targetCol} };
    {
        Pawn *p = A_FireCrosshairRayPawn(camPos, forward, items, 2,
                                         (Pawn *)g_localPawn.b, nullptr, nullptr);
        CHECK(p == nullptr, "C3 a wall first on the centre line -> no centred pawn");
    }
}

static Il2CppArray<Pawn *> g_enemyArr;
static Il2CppList<Pawn *>  g_enemyList;

static void killHonestDotRay()
{
    Config.Crosshair.OffsetX = 30.0f;
    Config.Crosshair.OffsetY = 0.0f;
    fn_ScreenToWorldPoint    = nullptr;
    fn_TransformGetRight     = nullptr;
    fn_TransformGetUp        = nullptr;
}
static void restoreHonestDotRay()
{
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
    fn_ScreenToWorldPoint    = fake_screen_to_world;
    fn_TransformGetRight     = fake_get_right;
    fn_TransformGetUp        = fake_get_up;
}

static void setEnemyList(Pawn **pawns, int n)
{
    std::memset(&g_enemyArr, 0, 16 + 8 + 4);
    g_enemyArr.max_length = n;
    g_enemyArr.bounds = nullptr;
    for (int i = 0; i < n; i++) g_enemyArr.m_Items[i] = pawns[i];
    g_enemyList.klass = nullptr;
    g_enemyList.unk1   = nullptr;
    g_enemyList.items  = &g_enemyArr;
    g_enemyList.size   = n;
    *(void **)(g_gameObj.b + Class_BaseGame_EnemyPawns) = &g_enemyList;
}

static Vector3 w2sOnCrosshair(const Vector3 &) { return Vector3{820.0f, 450.0f, 1.0f}; }
static Vector3 w2sAwayFromCrosshair(const Vector3 &) { return Vector3{1400.0f, 450.0f, 1.0f}; }

static Vector3 w2sTrueProjection(const Vector3 &w)
{
    const float f = (900.0f * 0.5f) /
                    tanf(NewFeatures::A_FireTune::kFallbackFov * 0.5f *
                         NewFeatures::A_FireTune::kDegToRad);
    const float dx = w.x - 0.0f;
    const float dy = w.y - 1.6f;
    const float dz = w.z - 0.0f;
    return Vector3{800.0f + (dx / dz) * f, 450.0f - (dy / dz) * f, dz};
}

static void testScan()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "S1 enemy in the open -> FIRED (the old build said 'wall in the way')");
        CHECK(out.pawn == (Pawn *)g_targetPawn.b, "S1 the right pawn was chosen");
        CHECK(out.path == 1, "S1 went through the crosshair-zone path");
        CHECK(out.visible && out.visibilityIdentified,
              "S1 visibility came from the ray naming him, not from a guess");
        CHECK(g_triggerDiag.selfHitsSkipped >= 1, "S1 the self hit was skipped on the way");
        CHECK(g_triggerDiag.losBlocked == 0, "S1 nothing counted as blocked");
        CHECK(g_triggerDiag.lastBlock != (int)FireBlock::NoLineOfSight,
              "S1 'wall in the way' was NOT the verdict");
    }
    restoreHonestDotRay();

    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {2.0f, &g_wallCol}, {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoLineOfSight, "S2 a genuine wall between us and him still refuses");
        CHECK(!out.visible && out.pawn == nullptr, "S2 no target escapes with the refusal");
        CHECK(g_triggerDiag.losBlockedScan == 1, "S2 the block is reported in the panel counters");
    }
    restoreHonestDotRay();

    resetWorld();
    objTargetUnderDot();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "S3 centre ray names the enemy -> fires");
        CHECK(out.path == 0 && out.pawn == (Pawn *)g_targetPawn.b,
              "S3 reported as the centre-ray path");
    }

    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sAwayFromCrosshair;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "S4 nobody in the zone -> no enemy, no rays");
        CHECK(g_triggerDiag.candidates == 0, "S4 candidate counter agrees");
    }

    resetWorld();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    objSetAlive(g_targetPawn, false);
    g_losHits = { {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone, "S5 a dead pawn is not a candidate");
        CHECK(g_rayAllCalls == 1, "S5 only the centre ray ran (no LOS ray for a non-candidate)");
    }

    g_w2sImpl = nullptr;
}

static void testNoTargetFilter()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };

    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_lastOccVisible = 0; g_recentlyRendered = 0;
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {3.3f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired,
              "F1 no filter: an occluded-by-the-game pawn is still fired");
        CHECK(g_rayAllCalls >= 1,
              "F1 ... decided by a real visibility ray, not by a getter veto");
    }
    restoreHonestDotRay();

    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {38.0f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired, "F2 no filter: a 40 m pawn under the dot is fired");
        CHECK(g_triggerDiag.losBlocked == 0,
              "F2 ... and it is not reported as a wall either");
    }
    restoreHonestDotRay();

    resetWorld();
    setEnemyList(enemies, 1);
    killHonestDotRay();
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sOnCrosshair;
    g_lastOccVisible = 0; g_recentlyRendered = 0;
    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {38.0f, &g_targetCol} };
    {
        TriggerTarget out{};
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired,
              "F3 occluded AND 40 m away: still fired - neither removed rule survives");
        CHECK(g_triggerDiag.losBlocked == 0, "F3 ... and never reported as a wall");
    }
    restoreHonestDotRay();

    g_w2sImpl = nullptr;
}

static int g_localPawnLookups = 0;
static Pawn *fake_localPawnCounted()
{
    g_localPawnLookups++;
    return (Pawn *)g_localPawn.b;
}

static void setCrosshair(float offX, float offY, float sizePx, bool drawn = true)
{
    Config.Crosshair.Enabled = drawn;
    Config.Crosshair.OffsetX = offX;
    Config.Crosshair.OffsetY = offY;
    Config.Crosshair.Size    = sizePx;
}

static FireBlock runScanOnCrosshairSetup(TriggerTarget &out, bool dotSeesPawn = false)
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
    objTargetUnderDot();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = dotSeesPawn ? w2sTrueProjection : w2sOnCrosshair;
    if (dotSeesPawn)
        g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    else
        g_centreHits = { {0.02f, &g_selfCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    return A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
}

static void testCrosshairAim()
{
    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out, true);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X1 the enemy under the dot's own ray is fired (path A)");
        CHECK(g_triggerDiag.crosshairCx == 800.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X1 the scan reports the dot's pixel as its crosshair (800,450)");
        CHECK(g_triggerDiag.crosshairRayViaGame == 1,
              "X1 the shot ray came from the game's own projection of that pixel");
        CHECK(g_triggerDiag.radiusPx == 40.0f,
              "X1 the zone is the dot: an 80 px dot is a 40 px radius");
        CHECK(g_triggerDiag.projKeepFrac >= 0.0f && g_triggerDiag.projKeepFrac <= 1.0f,
              "X1 ... and the accepted shot is INSIDE his projected body (keep <= 1)");
    }

    resetWorld();
    setCrosshair(0.0f, 0.0f, 10.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.radiusPx == 5.0f, "X2 a 10 px dot is a 5 px zone");
        CHECK(b == FireBlock::NoEnemyInZone,
              "X2 the same enemy 20 px off the dot is now OUTSIDE it");
        CHECK(b != FireBlock::Fired,
              "X2 ... so shrinking the dot really does tighten the trigger (the old build fired anyway)");
        CHECK(g_triggerDiag.dotRayDecided,
              "X2 ... and the refusal came from the dot's own ray, not from the zone");
    }

    resetWorld();
    setCrosshair(40.0f, -25.0f, 100.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 840.0f && g_triggerDiag.crosshairCy == 425.0f,
              "X3 the scan aims through the dot (800+40, 450-25)");
        CHECK(g_triggerDiag.crosshairRayViaGame == 1,
              "X3 ... and the ray is the ray through that pixel");

        CHECK(b == FireBlock::NoEnemyInZone && g_triggerDiag.dotRayDecided,
              "X3 near the dot but NOT under its ray -> NO fire (the over-fire fix)");
    }
    resetWorld();
    setCrosshair(40.0f, -25.0f, 100.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out, true);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X3 the same dot ON him still fires (the decision follows the dot)");
    }

    resetWorld();
    setCrosshair(200.0f, 0.0f, 80.0f);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X4 with the dot 200 px right the enemy at +20 px is outside it");
        CHECK(b != FireBlock::Fired, "X4 ... so the trigger does NOT fire him");
    }

    resetWorld();
    setCrosshair(200.0f, 0.0f, 80.0f, false);
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairCx == 1000.0f && g_triggerDiag.crosshairCy == 450.0f,
              "X5 a hidden dot still aims the scan (1000,450) - there is no switch to undo it");
        CHECK(b == FireBlock::NoEnemyInZone,
              "X5 ... and the enemy at +20 px stays unfired");
    }

    resetWorld();
    setCrosshair(40.0f, -25.0f, 80.0f);
    fn_TransformGetRight  = nullptr;
    fn_TransformGetUp     = nullptr;
    fn_ScreenToWorldPoint = nullptr;
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairRayViaGame == 0 &&
              g_triggerDiag.crosshairRayFellBack == 0 &&
              g_triggerDiag.crosshairRayPlain == 0,
              "X6 nothing can apply the offset -> NO honest dot ray, not a fake plain one");
        CHECK(b == FireBlock::Fired,
              "X6 the dot's zone still covers the enemy, so the trigger keeps working");
    }

    resetWorld();
    setCrosshair(40.0f, -25.0f, 80.0f);
    fn_TransformGetRight  = nullptr;
    fn_TransformGetUp     = nullptr;
    fn_ScreenToWorldPoint = fake_screen_to_world;
    {
        TriggerTarget out{};
        FireBlock b = runScanOnCrosshairSetup(out);
        CHECK(g_triggerDiag.crosshairRayViaGame == 1 && g_triggerDiag.crosshairRayPlain == 0,
              "X7 the game's projection alone carries the dot's offset");
        CHECK(b == FireBlock::NoEnemyInZone && g_triggerDiag.dotRayDecided,
              "X7 ... and the ray's verdict rules: NOT under the dot's ray -> no fire");
    }
    fn_TransformGetRight  = fake_get_right;
    fn_TransformGetUp     = fake_get_up;
    fn_ScreenToWorldPoint = fake_screen_to_world;

    resetWorld();

    objPos(g_targetHead, 0.5f, 1.7f, 40.0f);
    objPos(g_targetMesh, 0.5f, 0.0f, 40.0f);
    objPos(g_targetRoot, 0.5f, 0.0f, 40.0f);
    setCrosshair(200.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X8 aimed BESIDE the enemy's arm (200 px off his body) -> no fire");
        CHECK(g_triggerDiag.dotRayDeclined >= 1,
              "X8 ... and the panel counts it as a projected-body refusal");
        CHECK(out.pawn == nullptr, "X8 no target escapes the bound");
    }

    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone,
              "X9 close range, dot 118 px beside his body: NO fire (the report)");
        CHECK(g_triggerDiag.dotRayDeclined >= 1,
              "X9 ... counted as a projected-body refusal");
        CHECK(out.pawn == nullptr, "X9 no target escapes");
    }

    resetWorld();
    objTargetUnderDot(3.3f);
    setCrosshair(40.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X9b 40 px off a body 3 m away is still ON him -> fires");
    }
    resetWorld();
    objTargetUnderDot(40.0f);
    setCrosshair(40.0f, 0.0f, 80.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone && out.pawn == nullptr,
              "X9b the same 40 px at 40 m is 1.4 m to his side -> NO fire");
    }

    resetWorld();
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, nullptr);
    setCrosshair(0.0f, 0.0f, 10.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::NoEnemyInZone && out.pawn == nullptr,
              "X10 body unmeasurable AND the dot 88 px off his aim point -> NO fire");
        CHECK(g_triggerDiag.dotRayDeclined >= 1, "X10 ... counted, not silently fired");
    }

    resetWorld();
    objSetBone(g_targetPawn, Class_Pawn_m_HeadBone, nullptr);
    objTargetUnderDot();
    setCrosshair(0.0f, 130.0f, 10.0f);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = {};
    {
        TriggerTarget out{};
        Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
        setEnemyList(enemies, 1);
        FireBlock b = A_FireScan((Pawn *)g_localPawn.b, (Weapon *)g_weaponObj.b, out);
        CHECK(b == FireBlock::Fired && out.path == 0,
              "X10 ... while a small dot ON his aim point still fires");
    }

    restoreHonestDotRay();
    g_w2sImpl = nullptr;

    resetWorld();
    setCrosshair(0.0f, 0.0f, 80.0f);
    Config.Crosshair.Enabled = true;
}

static void resetLocalPawnCache()
{
    g_afireLocalPawnCache = nullptr;
    g_afireLocalPawnCacheTime = -1000.0;
    g_afireHeldWeaponCache = nullptr;
    g_afireHeldWeaponStamp = -1000.0;
    g_afireOffDiagStamp = -1000.0;
    g_afireLastEvalTime = -1000.0;
    g_afireHoldingFire = false;
    g_afireFiringOn = nullptr;
    g_afireTargetPawn = nullptr;
}

static void testTickHookCost()
{
    api12 = (uintptr_t)&fake_localPawnCounted;

    resetLocalPawnCache();
    g_localPawnLookups = 0;
    for (int i = 0; i < 200; i++) { Pawn *p = GamePlay::get_LocalPawn(); (void)p; }
    CHECK(g_localPawnLookups == 200,
          "H0 control: 200 raw get_LocalPawn() calls are 200 native calls");

    resetLocalPawnCache();
    g_localPawnLookups = 0;
    for (int i = 0; i < 200; i++) { Pawn *p = A_FireLocalPawnFast(); (void)p; }
    CHECK(g_localPawnLookups <= 2,
          "H1 200 cached local-pawn reads cost <= 2 native GamePlay::get_LocalPawn calls");

    resetLocalPawnCache();
    g_localPawnLookups = 0;
    g_triggerDiag = TriggerbotDiag{};
    for (int i = 0; i < 200; i++) A_FirePawnTicked(&g_otherPawn);
    CHECK(g_localPawnLookups <= 2,
          "H2 200 foreign pawn ticks cost <= 2 native lookups (was one each)");
    CHECK(g_triggerDiag.pawnLocalTicks == 0, "H2 no foreign tick was mistaken for ours");

    Config.NewFeaturesMenu.Triggerbot = false;
    resetLocalPawnCache();
    g_localPawnLookups = 0;
    g_triggerDiag = TriggerbotDiag{};
    for (int i = 0; i < 200; i++) A_FireTick(nullptr);
    CHECK(g_localPawnLookups <= 1,
          "H3 200 hook calls with the triggerbot OFF cost <= 1 native lookup");
    CHECK(g_triggerDiag.blockCount[(int)FireBlock::FeatureOff] >= 1,
          "H3 the panel still receives a live 'triggerbot off' verdict");
    CHECK(g_triggerDiag.lastTickTime > 0.0, "H3 the heartbeat stays alive");
}

static int g_isAimingCalls      = 0;
static int g_currentWeaponCalls = 0;

static bool fake_pawnIsAimingTrap(void *)
{
    g_isAimingCalls++;
    return true;
}

static Weapon *fake_currentWeapon(Pawn *)
{
    g_currentWeaponCalls++;
    return (Weapon *)g_weaponObj.b;
}

static void setWeaponAiming(bool flag, float percent)
{
    *(bool  *)(g_weaponObj.b + Offsets::Weapon_m_IsAiming)         = flag;
    *(float *)(g_weaponObj.b + Offsets::Weapon_m_AimingPercentage) = percent;
}

static void armFiringWorld()
{
    Pawn *enemies[1] = { (Pawn *)g_targetPawn.b };
    objTargetUnderDot();
    setEnemyList(enemies, 1);
    fn_GameGetMatchGame = []() -> void * { return g_gameObj.b; };
    g_w2sImpl = w2sTrueProjection;
    g_centreHits = { {0.02f, &g_selfCol}, {5.0f, &g_targetCol} };
    g_losHits    = { {0.0f, &g_selfCol}, {3.3f, &g_targetCol} };
    setCrosshair(0.0f, 0.0f, 80.0f);
}

static int  g_pawnAimingCalls = 0;
static bool g_pawnAimingValue = false;
static int  g_fcAimingCalls   = 0;
static bool g_fcAimingValue   = false;
static bool g_isFiringValue   = false;

static bool fake_pawnAimingCounted(void *)
{
    g_pawnAimingCalls++;
    return g_pawnAimingValue;
}
static bool fake_fireComponentAiming(void *)
{
    g_fcAimingCalls++;
    return g_fcAimingValue;
}
static void *fake_currentFireComponent(void *weapon)
{
    return weapon == (void *)g_weaponObj.b ? (void *)g_fireCompObj.b : nullptr;
}
static bool fake_pawnIsFiring(void *) { return g_isFiringValue; }

static void installAdsFakes()
{
    objInit(g_fireCompObj);
    std::memset(g_fakeKlass, 0, sizeof g_fakeKlass);
    *(const char **)(g_fakeKlass + 0x10) = g_instantName;
    *(void **)(g_fireCompObj.b + 0x00)   = g_fakeKlass;

    fn_PawnIsAiming                  = fake_pawnAimingCounted;
    fn_WeaponGetCurrentFireComponent = fake_currentFireComponent;
    fn_FireComponentGetIsAiming      = fake_fireComponentAiming;
}

static FireBlock runTreeOnCrosshairSetup(TriggerTarget &out)
{
    armFiringWorld();
    return A_FireShouldFire(out);
}

static void testCrashFixes()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api15 = (uintptr_t)&fake_pawnIsAimingTrap;
    api22 = (uintptr_t)&fake_currentWeapon;
    Config.NewFeaturesMenu.Triggerbot = false;

    resetWorld();
    *(void **)(g_weaponObj.b + 0x10) = nullptr;
    Config.NewFeaturesMenu.TriggerbotMode = 1;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        const FireBlock b = runTreeOnCrosshairSetup(out);
        CHECK(b == FireBlock::NoWeapon,
              "A0 a destroyed weapon is refused by the liveness check");
        CHECK(g_pawnAimingCalls == 0 && g_fcAimingCalls == 0,
              "A0 ... and neither ADS source is asked about it");
    }
    objInit(g_weaponObj);

    resetWorld();
    api12 = (uintptr_t)&fake_localPawnCounted;
    api22 = (uintptr_t)&fake_currentWeapon;
    Config.NewFeaturesMenu.NoSprintToFireDelay = true;
    g_noSprintFireDelayWeapon.store((void *)g_weaponObj.b, std::memory_order_relaxed);
    {
        g_localPawnLookups = 0;
        g_currentWeaponCalls = 0;
        for (int i = 0; i < 200; i++)
            (void)NoSprintFireDelayActive(g_weaponObj.b);
        CHECK(g_localPawnLookups == 0 && g_currentWeaponCalls == 0,
              "B1 200 hooked-getter bodies make ZERO native calls (the sprint-delay crash)");
        CHECK(NoSprintFireDelayActive(g_weaponObj.b),
              "B1 ... and still recognise the local player's own weapon");
    }

    CHECK(!NoSprintFireDelayActive((void *)g_otherPawn.b),
          "B2 a remote player's weapon is never touched");
    CHECK(!NoSprintFireDelayActive(nullptr), "B2 a null instance is refused");
    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    CHECK(!NoSprintFireDelayActive(g_weaponObj.b),
          "B2 with the option off the getter falls through to the game's value");
    Config.NewFeaturesMenu.NoSprintToFireDelay = true;

    g_noSprintFireDelayWeapon.store(nullptr, std::memory_order_relaxed);
    {
        g_currentWeaponCalls = 0;
        A_FirePawnTicked((void *)g_localPawn.b);
        CHECK(g_currentWeaponCalls == 1,
              "B3 our own pawn tick is one native get_CurrentWeapon call");
        CHECK(g_noSprintFireDelayWeapon.load() == (void *)g_weaponObj.b,
              "B3 ... and it caches the held weapon for the hook to compare against");
    }

    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    A_FirePawnTicked((void *)g_localPawn.b);
    CHECK(g_noSprintFireDelayWeapon.load() == nullptr,
          "B4 with the option off the cache is cleared, so the hook changes nothing");

    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    objInit(g_weaponObj);
    resetWorld();
}

static void testAdsGate()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api15 = (uintptr_t)&fake_pawnIsAimingTrap;
    api22 = (uintptr_t)&fake_currentWeapon;
    Config.NewFeaturesMenu.Triggerbot = false;

    resetWorld();
    installAdsFakes();
    setWeaponAiming(false, 0.0f);
    Config.NewFeaturesMenu.TriggerbotMode = 1;
    g_pawnAimingCalls = g_fcAimingCalls = g_isAimingCalls = 0;
    g_pawnAimingValue = g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::ModeNotAds,
              "B1 ADS Only: neither source is scoping -> the shot is blocked");

        CHECK(g_pawnAimingCalls == 1 && g_fcAimingCalls == 1,
              "B1 both ADS sources were asked once (probe -> the gate reuses the pawn answer)");
        CHECK(g_isAimingCalls == 0,
              "B1 ... through the resolved pointer, never the api15 macro (a trap that says 'scoping')");
        CHECK(g_triggerDiag.adsPawnOk && g_triggerDiag.adsFcOk &&
              !g_triggerDiag.adsPawnAim && !g_triggerDiag.adsFcAim &&
              !g_triggerDiag.adsUnknown,
              "B1 the panel reports who answered and what they said");
    }

    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = true; g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B2 Pawn.IsAiming 0x519BB10 = scoped -> the shot goes out");
        CHECK(g_triggerDiag.adsPawnAim && !g_triggerDiag.adsFcAim,
              "B2 ... and the panel credits the pawn flag");
    }

    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = false; g_fcAimingValue = true;
    g_pawnAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B3 the fire component's get_IsAiming 0xC9AF4E0 = scoped -> fires");
        CHECK(g_pawnAimingCalls == 1, "B3 ... the pawn is still asked (once, by the scope-swap probe)");
        CHECK(g_triggerDiag.adsFcAim && !g_triggerDiag.adsPawnAim,
              "B3 ... and the fire component is credited");
    }

    resetWorld();
    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    g_pawnAimingValue = g_fcAimingValue = false;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B4 ADS Only with no usable ADS signal FAILS OPEN (was: every shot blocked)");
        CHECK(g_triggerDiag.adsUnknown,
              "B4 ... and the panel says the signal is missing");
    }
    installAdsFakes();

    {
        objSetAlive(g_localPawn, false);
        g_pawnAimingCalls = 0; g_fcAimingValue = false;
        ScopeSettle probe{};
        const AdsRead ads = A_FireAdsGate((void *)g_weaponObj.b, (Pawn *)g_localPawn.b, probe);
        CHECK(g_pawnAimingCalls == 0 && !ads.pawnOk,
              "B5 a pawn whose alive flag is clear is NEVER handed to the native getter");
        objSetAlive(g_localPawn, true);
    }

    {
        *(void **)(g_localPawn.b + 0x10) = nullptr;
        g_pawnAimingCalls = 0;
        ScopeSettle probe{};
        const AdsRead ads = A_FireAdsGate((void *)g_weaponObj.b, (Pawn *)g_localPawn.b, probe);
        CHECK(g_pawnAimingCalls == 0 && !ads.pawnOk,
              "B6 a Destroy()ed pawn (m_CachedPtr cleared) is never passed to the game");
        objInit(g_localPawn);
    }

    resetWorld();
    installAdsFakes();
    g_pawnAimingValue = g_fcAimingValue = false;
    setWeaponAiming(true, 7.5f);
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::ModeNotAds,
              "B7 garbage in the unverified field offsets cannot make the mode 'Always'");
        CHECK(g_triggerDiag.adsFlag == 1 && g_triggerDiag.adsPct == 7.5f,
              "B7 ... but the panel still reports them for the record");
    }

    resetWorld();
    installAdsFakes();
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B8 mode Always fires with a hipfiring weapon");

        CHECK(g_pawnAimingCalls == 1 && g_fcAimingCalls == 0,
              "B8 ... and the mode gate asks neither ADS source");
    }

    resetWorld();
    installAdsFakes();
    Config.NewFeaturesMenu.TriggerbotMode = 2;
    fn_PawnGetIsFiring = fake_pawnIsFiring;
    g_isFiringValue = true;
    g_pawnAimingCalls = g_fcAimingCalls = 0;
    {
        TriggerTarget out{};
        CHECK(runTreeOnCrosshairSetup(out) == FireBlock::Fired,
              "B9 mode Firing only uses Pawn.get_IsFiring, not the ADS gate");
        CHECK(g_pawnAimingCalls == 1 && g_fcAimingCalls == 0,
              "B9 ... so the mode gate asks no ADS source (only the scope-swap probe reads the pawn)");
    }
    g_isFiringValue = false;
    fn_PawnGetIsFiring = nullptr;

    objInit(g_weaponObj);
    setWeaponAiming(false, 0.0f);
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    Config.NewFeaturesMenu.Triggerbot = false;
    resetWorld();
}

static int   g_startFireCalls = 0;
static int   g_stopFireCalls  = 0;
static bool  g_canFireValue   = true;
static float g_realInterval   = 0.20f;

static void  fake_startFire(void *)       { g_startFireCalls++; }
static void  fake_stopFire(void *, bool)  { g_stopFireCalls++; }
static bool  fake_canFire(void *)         { return g_canFireValue; }
static float fake_realInterval(void *)    { return g_realInterval; }
static bool  fake_autoFire(void *)        { return true; }

static void openTickWindow()
{
    g_afireLastEvalTime = -1000.0;
    g_afireInTick       = false;
}

static void testRepressPacing()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api22 = (uintptr_t)&fake_currentWeapon;
    fn_PawnIsAiming                  = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    fn_FireComponentGetIsAiming      = nullptr;
    fn_PawnGetIsFiring               = nullptr;
    fn_WeaponIsAutoFire              = nullptr;
    fn_WeaponRealFireInterval        = fake_realInterval;
    fn_WeaponCanFire                 = fake_canFire;
    fn_Weapon_StartFire              = fake_startFire;
    fn_Weapon_StopFire               = fake_stopFire;
    Config.NewFeaturesMenu.Triggerbot = true;
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    g_realInterval = 0.20f;
    g_canFireValue = false;

    resetWorld();
    armFiringWorld();
    resetLocalPawnCache();
    g_triggerDiag = TriggerbotDiag{};
    g_startFireCalls = g_stopFireCalls = 0;
    g_afireLastPressTime   = -1000.0;
    g_afireLastPressWeapon = nullptr;

    CHECK(std::fabs(A_FireRepressPaceSeconds(0.20f) - 0.18f) < 1e-5f,
          "P0 pace = 90% of the weapon's own interval");
    CHECK(A_FireRepressPaceSeconds(0.0001f) == A_FireTune::kMinFireInterval,
          "P0 a degenerate interval falls back to the pacing floor");
    CHECK(A_FireRepressPaceSeconds(9.0f) == A_FireTune::kMaxRepressPace,
          "P0 a huge interval is capped, so a repeat press never waits too long");

    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1, "P1 the first press goes out with no delay");
    CHECK(g_triggerDiag.fires == 1, "P1 ... and the panel counts it");

    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_stopFireCalls == 1 && !g_afireHoldingFire,
          "P2 an edge-press weapon is released on the next scan (one press = one shot)");
    CHECK(g_startFireCalls == 1, "P2 ... and nothing was re-pressed yet");

    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1,
          "P3 a repeat press waits (was: one StartFire per scan, ~250/s)");
    CHECK(g_triggerDiag.repressSkipped >= 1 && g_triggerDiag.paceWaiting,
          "P3 ... and the panel shows the wait");
    CHECK(GetTriggerbotDebugState() == 1,
          "P3 ... as an 'aimed in, waiting on the gun' state");

    g_canFireValue = true;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 2,
          "P4 the moment the game says the gun can fire, the press goes out (no added delay)");
    g_canFireValue = false;

    openTickWindow();
    A_FireTick(nullptr);
    g_afireLastPressTime = A_FireNow() - 0.30;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 3,
          "P5 past the pace a press goes out even with CanFire stuck 'not ready'");

    g_afireLastPressTime   = A_FireNow();
    g_afireLastPressWeapon = (void *)g_weaponObj.b;
    g_afireTargetPawn      = (void *)g_otherPawn.b;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 4, "P6 a new target is pressed immediately, never paced");

    A_FireReleaseTrigger(nullptr);
    g_afireTargetPawn      = nullptr;
    g_afireLastPressTime   = -1000.0;
    g_afireLastPressWeapon = nullptr;
    fn_WeaponIsAutoFire    = fake_autoFire;
    openTickWindow();
    A_FireTick(nullptr);
    const int autoPresses = g_startFireCalls;
    const int autoStops   = g_stopFireCalls;
    openTickWindow();
    A_FireTick(nullptr);
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == autoPresses && g_stopFireCalls == autoStops,
          "P7 an auto weapon is held: no release and no re-press while the target stays");
    CHECK(g_afireHoldingFire, "P7 ... the trigger stays down");

    const int beforeReassert = g_startFireCalls;
    g_canFireValue = false;
    openTickWindow();
    A_FireTick(nullptr);
    g_canFireValue = true;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == beforeReassert + 1,
          "P9 a held auto weapon re-presses the moment CanFire turns true (reload recovery)");
    CHECK(g_triggerDiag.reasserts >= 1, "P9 ... and the panel counts the re-assert");

    g_afireLastPressTime = A_FireNow() - 0.30;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == beforeReassert + 2,
          "P10 a held trigger also re-asserts on the periodic backstop");

    fn_WeaponIsAutoFire = nullptr;
    CHECK(GetWeaponFireMode((void *)g_weaponObj.b) == FireMode::Single,
          "P8 an unrecognised weapon behaves as single shot (edge), never as a hold");

    g_afireHoldingFire     = false;
    g_afireFiringOn        = nullptr;
    g_afireTargetPawn      = nullptr;
    g_afireLastPressWeapon = nullptr;
    g_afireLastPressTime   = -1000.0;
    fn_Weapon_StartFire          = nullptr;
    fn_Weapon_StopFire           = nullptr;
    fn_WeaponCanFire             = nullptr;
    fn_WeaponRealFireInterval    = nullptr;
    fn_WeaponIsAutoFire          = nullptr;
    Config.NewFeaturesMenu.Triggerbot = false;
    resetWorld();
}

static FakeObj g_fireCompObj2;
static bool    g_fireCompSwapped = false;

static void *fake_currentFireComponentSwappable(void *weapon)
{
    if (weapon != (void *)g_weaponObj.b)
        return nullptr;
    return g_fireCompSwapped ? (void *)g_fireCompObj2.b : (void *)g_fireCompObj.b;
}

static void endSettleFloor()
{
    g_afireAdsSettleUntil = A_FireNow() - 0.01;
    g_afireAdsSettleMax   = A_FireNow() + 10.0;
}

static void testAdsScopeSwapGuard()
{
    api12 = (uintptr_t)&fake_localPawnCounted;
    api22 = (uintptr_t)&fake_currentWeapon;

    resetWorld();
    installAdsFakes();
    objInit(g_fireCompObj2);
    *(const char **)(g_fakeKlass + 0x10) = g_instantName;
    *(void **)(g_fireCompObj2.b + 0x00)  = g_fakeKlass;
    g_fireCompSwapped = false;
    fn_WeaponGetCurrentFireComponent = fake_currentFireComponentSwappable;

    fn_WeaponIsAutoFire       = nullptr;
    fn_WeaponCanFire          = fake_canFire;
    fn_Weapon_StartFire       = fake_startFire;
    fn_Weapon_StopFire        = fake_stopFire;
    fn_WeaponRealFireInterval = fake_realInterval;
    g_realInterval = 0.20f;
    g_canFireValue = true;

    Config.NewFeaturesMenu.Triggerbot     = true;
    Config.NewFeaturesMenu.TriggerbotMode = 0;
    armFiringWorld();
    resetLocalPawnCache();
    g_startFireCalls = g_stopFireCalls = 0;
    g_afireLastPressTime   = -1000.0;
    g_afireLastPressWeapon = nullptr;
    g_pawnAimingValue      = false;
    g_fcAimingValue        = false;

    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1 && g_afireHoldingFire,
          "G1 no scope motion: the first scan presses fire (the guard adds no delay)");
    CHECK(g_stopFireCalls == 0 && !g_afireReleasePending,
          "G1 ... and nothing is being held back");

    g_pawnAimingValue = true;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == 1,
          "G2 the scan that sees the scope edge does NOT press (this is the crash)");
    CHECK(g_triggerDiag.lastBlock == (int)FireBlock::AdsTransition,
          "G2 ... it reports the scope transition as the reason");
    CHECK(g_stopFireCalls == 0 && g_afireReleasePending,
          "G2 ... and the RELEASE is deferred too: no native fire call inside the swap");

    g_canFireValue = false;
    endSettleFloor();
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_afireAdsSettling && g_startFireCalls == 1,
          "G3 floor elapsed but Weapon.CanFire() says 'not ready' -> still held");
    CHECK(g_stopFireCalls == 0 && g_afireReleasePending,
          "G3 ... and the release is still waiting behind the window");

    g_canFireValue = true;
    endSettleFloor();
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(!g_afireAdsSettling,
          "G4 a stable fire component + CanFire() == true ends the hold (no fixed delay)");
    CHECK(g_stopFireCalls == 1 && !g_afireReleasePending,
          "G4 ... and the deferred release goes out on that first settled scan");

    const int pressesBeforeSwap = g_startFireCalls;
    g_fireCompSwapped = true;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == pressesBeforeSwap &&
          g_triggerDiag.lastBlock == (int)FireBlock::AdsTransition,
          "G5 a changed current-fire-component is a scope edge even with the pawn flag still");

    endSettleFloor();
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(!g_afireAdsSettling && g_triggerDiag.lastBlock != (int)FireBlock::AdsTransition,
          "G5b the component swap edge settles on the floor + a stable component + CanFire");

    g_afireAdsSettleUntil = A_FireNow() + 10.0;
    g_afireAdsSettleMax   = A_FireNow() - 0.01;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(!g_afireAdsSettling,
          "G6 the ceiling is absolute: a window that never settles cannot stall the trigger");

    const int pressesBeforeSwapBack = g_startFireCalls;
    g_fireCompSwapped = false;
    openTickWindow();
    A_FireTick(nullptr);
    CHECK(g_startFireCalls == pressesBeforeSwapBack &&
          g_triggerDiag.lastBlock == (int)FireBlock::AdsTransition,
          "G6b scoping back out (the component swaps back) is an edge too, and it holds");
    g_afireAdsSettleUntil = -1000.0;
    g_afireAdsSettleMax   = -1000.0;

    g_afireHoldingFire = true;
    g_afireFiringOn    = (void *)g_weaponObj.b;
    g_afireReleasePending = false;
    g_afireAdsSettling = false;
    const int stopsBefore = g_stopFireCalls;
    *(void **)(g_weaponObj.b + 0x10) = nullptr;
    A_FireReleaseTrigger(nullptr);
    CHECK(g_stopFireCalls == stopsBefore,
          "G7 a Destroy()ed weapon is NEVER handed to Weapon.StopFire");
    CHECK(!g_afireHoldingFire && g_afireFiringOn == nullptr,
          "G7 ... and the trigger state is dropped anyway (nothing left to release)");
    objInit(g_weaponObj);

    g_afireHoldingFire = true;
    g_afireFiringOn    = (void *)g_weaponObj.b;
    A_FireReleaseTrigger(nullptr);
    CHECK(g_stopFireCalls == stopsBefore + 1,
          "G8 a live weapon is released exactly as before (the check is not a mute)");

    g_afireHoldingFire     = false;
    g_afireFiringOn        = nullptr;
    g_afireTargetPawn      = nullptr;
    g_afireLastPressWeapon = nullptr;
    g_afireAdsSettleUntil  = -1000.0;
    g_afireAdsSettleMax    = -1000.0;
    g_afireAdsSettling     = false;
    g_afireAdsCompSeen     = false;
    g_afireAdsLastComp     = nullptr;
    g_afireReleasePending  = false;
    fn_Weapon_StartFire          = nullptr;
    fn_Weapon_StopFire           = nullptr;
    fn_WeaponCanFire             = nullptr;
    fn_WeaponRealFireInterval    = nullptr;
    fn_WeaponIsAutoFire          = nullptr;
    fn_WeaponGetCurrentFireComponent = nullptr;
    Config.NewFeaturesMenu.Triggerbot = false;
    resetWorld();
}

int main()
{
    installFns();
    resetWorld();

    testGate();
    testArrayGuards();
    testCentreRay();
    testScan();
    testNoTargetFilter();
    testCrosshairAim();
    testTickHookCost();
    testCrashFixes();
    testAdsGate();
    testRepressPacing();
    testAdsScopeSwapGuard();

    if (g_failures == 0) {
        printf("\nOK: all LOS gate checks passed\n");
        return 0;
    }
    printf("\n%d check(s) FAILED\n", g_failures);
    return 1;
}

