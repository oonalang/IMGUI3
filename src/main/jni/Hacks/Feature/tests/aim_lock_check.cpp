#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Quaternion.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/StructGame/StructGame.h"
#include "Hacks/Feature/Init_Hacks.h"
#include "Hacks/Feature/AimLock.h"

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

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

// --- fake Unity objects -----------------------------------------------------
static const size_t OBJ_SIZE = 512;
struct FakeObj { alignas(8) unsigned char b[OBJ_SIZE]; };

static FakeObj g_game, g_localPawn, g_enemyPawn, g_deadPawn;
static FakeObj g_localHead, g_enemyHead, g_deadHead;
static FakeObj g_camObj, g_camTransform;

static void objInit(FakeObj &o)
{
    std::memset(o.b, 0, sizeof o.b);
    *(void **)(o.b + 0x10) = o.b;   // m_CachedPtr: "alive"
    *(void **)(o.b + 0x38) = o.b;   // transform identity
}
static void objPos(FakeObj &o, float x, float y, float z)
{
    Vector3 v{x, y, z};
    std::memcpy(o.b + 0x100, &v, sizeof v);
}
static void objSetBone(FakeObj &pawn, size_t offset, FakeObj *bone)
{
    *(void **)(pawn.b + offset) = bone ? bone->b : nullptr;
}
static void objSetAlive(FakeObj &pawn, bool alive)
{
    pawn.b[Class_Pawn_m_IsAlive] = alive ? 1 : 0;
}

static Camera *fake_camera_main() { return (Camera *)g_camObj.b; }
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
static void *fake_match_game() { return g_game.b; }

static Vector3 g_forward{0.0f, 0.0f, 1.0f};
static Vector3 fake_forward(void *) { return g_forward; }

static uint32_t g_localTeam = 1, g_enemyTeam = 2;
static uint32_t fake_team(void *p)
{
    return (p == (void *)g_localPawn.b || p == (void *)g_deadPawn.b)
         ? g_localTeam : g_enemyTeam;
}

static int        g_setAimCalls = 0;
static Quaternion g_lastAim{};
static uintptr_t  g_lastAimPawn = 0;
static void fake_setAim(uintptr_t pawn, Quaternion q)
{
    g_setAimCalls++;
    g_lastAim = q;
    // The real camera follows the aim the pawn was given, so the next tick
    // starts from where this one left off - without that, repeated ticks would
    // all step from the same stale forward.
    g_forward = Quaternion::Forward(q);
    g_lastAimPawn = pawn;
}

// --- fake enemy list --------------------------------------------------------
static Il2CppArray<Pawn *> g_enemyArr;
static Il2CppList<Pawn *>  g_enemyList;

static void setEnemies(Pawn **pawns, int n)
{
    std::memset(&g_enemyArr, 0, 16 + 8 + 4);
    g_enemyArr.max_length = n;
    g_enemyArr.bounds = nullptr;
    for (int i = 0; i < n; i++) g_enemyArr.m_Items[i] = pawns[i];
    g_enemyList.klass = nullptr;
    g_enemyList.unk1  = nullptr;
    g_enemyList.items = &g_enemyArr;
    g_enemyList.size  = n;
    *(void **)(g_game.b + Class_BaseGame_EnemyPawns) = &g_enemyList;
}

static void resetWorld()
{
    objInit(g_game);
    objInit(g_localPawn);
    objInit(g_enemyPawn);
    objInit(g_deadPawn);
    objInit(g_localHead);
    objInit(g_enemyHead);
    objInit(g_deadHead);
    objInit(g_camObj);
    objInit(g_camTransform);

    objSetBone(g_localPawn, Class_Pawn_m_HeadBone, &g_localHead);
    objSetBone(g_enemyPawn, Class_Pawn_m_HeadBone, &g_enemyHead);
    objSetBone(g_deadPawn,  Class_Pawn_m_HeadBone, &g_deadHead);
    objSetAlive(g_localPawn, true);
    objSetAlive(g_enemyPawn, true);
    objSetAlive(g_deadPawn,  false);

    // The camera component hands out its transform: without this the eye would
    // be the camera object itself, which sits at the origin.
    *(void **)(g_camObj.b + 0x38) = g_camTransform.b;
    objPos(g_camTransform, 0.0f, 1.6f, 0.0f);
    objPos(g_localHead,    0.0f, 1.7f, 0.0f);
    // 33.7 degrees off the camera's forward: a real turn, not a nudge.
    objPos(g_enemyHead,    2.0f, 1.7f, 3.0f);
    objPos(g_deadHead,     2.0f, 1.7f, 3.0f);

    g_forward = Vector3{0.0f, 0.0f, 1.0f};
    g_localTeam = 1;
    g_enemyTeam = 2;

    Pawn *enemies[1] = { (Pawn *)g_enemyPawn.b };
    setEnemies(enemies, 1);

    Config.Aim.AimLock = 0.0f;
    Config.Aim.Target  = EAimTarget::Heads;
    Config.Aim.By      = EAim::Distance;
    Config.Aim.size    = 0.0f;
    Config.Aim.Cross   = 45.0f;

    aim_lock::g_cameraForward = fake_forward;
    aim_lock::g_getTeamId     = fake_team;
    aim_lock::g_lastTick      = -1000.0;
    aim_lock::g_turns         = 0;
    aim_lock::g_lastTarget    = nullptr;

    g_setAimCalls = 0;
    g_lastAim = Quaternion::Identity();
    g_lastAimPawn = 0;
}

// Runs one tick with a known delta so the rate limit is testable.
static void tickWithDt(float seconds)
{
    aim_lock::g_lastTick = aim_lock::Now() - (double)seconds;
    aim_lock::Tick((Pawn *)g_localPawn.b);
}

static float AngleDeg(const Vector3 &a, const Vector3 &b)
{
    const float la = Vector3::Magnitude(a);
    const float lb = Vector3::Magnitude(b);
    if (!(la > 1e-4f) || !(lb > 1e-4f)) return 180.0f;
    float c = (a.x * b.x + a.y * b.y + a.z * b.z) / (la * lb);
    if (c > 1.0f) c = 1.0f;
    if (c < -1.0f) c = -1.0f;
    return acosf(c) * 57.29578f;
}

int main()
{
    m_unity = 0;
    api7  = (uintptr_t)&fake_camera_main;
    api9  = (uintptr_t)&fake_get_transform;
    api10 = (uintptr_t)&fake_get_position;
    api11 = (uintptr_t)&fake_match_game;
    api14 = (uintptr_t)&fake_setAim;

    const Vector3 want(2.0f, 0.1f, 3.0f);   // eye -> enemy head
    const float wantAngle = AngleDeg(Vector3{0.0f, 0.0f, 1.0f}, want);

    // A1: zero on the slider means the feature is off, not "slow".
    resetWorld();
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == 0, "L1 Aim lock 0 never touches the aim");

    // A2: it turns toward the target, and only by what the rate allows.
    resetWorld();
    Config.Aim.AimLock = 10.0f;              // 6 + 10 * 5.94 = 65.4 deg/s
    tickWithDt(0.1f);                        // -> 6.54 deg this tick
    CHECK(g_setAimCalls == 1 && g_lastAimPawn == (uintptr_t)g_localPawn.b,
          "L2 one tick turns the local pawn's aim");
    const Vector3 dir2 = Quaternion::Forward(g_lastAim);
    const float moved2 = AngleDeg(Vector3{0.0f, 0.0f, 1.0f}, dir2);
    const float left2  = AngleDeg(dir2, want);
    CHECK(moved2 > 1.0f && moved2 < 8.0f,
          "L2 the turn is rate limited (about 6.5 deg for 0.1 s at strength 10)");
    CHECK(left2 < wantAngle && left2 > 20.0f,
          "L2 ... toward the target, without snapping onto it");
    CHECK(aim_lock::g_lastTarget == (void *)g_enemyPawn.b,
          "L2 the tracked target is reported");

    // A3: full strength converges onto the target and stays there.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    for (int i = 0; i < 40; i++) tickWithDt(0.1f);
    const Vector3 dir3 = Quaternion::Forward(g_lastAim);
    CHECK(AngleDeg(dir3, want) < 0.5f,
          "L3 repeated ticks at strength 100 settle on the target");
    const int callsAtConvergence = g_setAimCalls;
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == callsAtConvergence + 1, "L3 ... and it keeps the aim there");

    // A4: a teammate is not a target.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    g_enemyTeam = g_localTeam;
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == 0, "L4 a teammate is never locked");

    // A5: the range slider bounds it.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    Config.Aim.size = 1.0f;                  // the enemy is 3.6 m away
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == 0, "L5 a target beyond Aim range is ignored");

    // A6: dead pawns are not targets.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    {
        Pawn *enemies[1] = { (Pawn *)g_deadPawn.b };
        setEnemies(enemies, 1);
    }
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == 0, "L6 a dead pawn is never locked");

    // A7: the Hit part setting moves the aim point down the body.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    Config.Aim.Target = EAimTarget::Chests;
    for (int i = 0; i < 40; i++) tickWithDt(0.1f);
    {
        const Vector3 chest(2.0f, 1.5f - 1.6f, 3.0f);   // eye -> chest, 20 cm down
        CHECK(AngleDeg(Quaternion::Forward(g_lastAim), chest) < 0.5f,
              "L7 Aim lock follows the Hit part setting (chest, not head)");
    }

    // A8: without a camera forward there is nothing to turn from, and a blind
    // snap would be worse than doing nothing.
    resetWorld();
    Config.Aim.AimLock = 100.0f;
    g_forward = Vector3{0.0f, 0.0f, 0.0f};
    tickWithDt(0.1f);
    CHECK(g_setAimCalls == 0, "L8 no camera forward -> no turn, never a blind snap");

    // A9: the same strength behaves the same at different frame rates.
    resetWorld();
    Config.Aim.AimLock = 50.0f;              // 6 + 50 * 5.94 = 303 deg/s
    tickWithDt(0.02f);
    const float movedFast = AngleDeg(Vector3{0.0f, 0.0f, 1.0f}, Quaternion::Forward(g_lastAim));
    resetWorld();
    Config.Aim.AimLock = 50.0f;
    for (int i = 0; i < 5; i++) tickWithDt(0.004f);   // same 20 ms, five ticks
    const float movedSlow = AngleDeg(Vector3{0.0f, 0.0f, 1.0f}, Quaternion::Forward(g_lastAim));
    CHECK(std::fabs(movedFast - movedSlow) < 1.5f,
          "L9 the turn is per second, not per tick (20 ms in one tick ~= 20 ms in five)");

    if (g_failures == 0) {
        printf("\nOK: all aim lock checks passed\n");
        return 0;
    }
    printf("\n%d check(s) FAILED\n", g_failures);
    return 1;
}
