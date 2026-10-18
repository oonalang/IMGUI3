// Host test for the entity walks and the ESP pawn loop, which used to
// dereference whatever the game's pawn list happened to hold. The report is
// "crashes ~10 s after joining a match, with bots in it" - the window in which
// bots die and respawn, i.e. the window in which a list entry can be a
// destroyed object (readable managed wrapper, cleared Object.m_CachedPtr) or a
// live pawn whose AttackableTarget / player name has already been cleared.
// GetClosestTarget(), GetInsideFOVTarget() and DrawESP() then called the
// object's own getters (m_Mesh, m_HeadBone, get_position, get_PlayerName,
// WorldToScreenPoint) on it.
//
// The test drives the REAL functions with the game natives replaced by fakes:
//   * Tools::IsPtrValid answers from an arena list, so a "destroyed" pawn is
//     simply not registered;
//   * Class_Gameplay_get_MatchGame / get_LocalPawn / Class_Camera_get_main /
//     Class_Transform_get_position / Class_Camera_WorldToScreenPoint /
//     Class_Pawn_get_PlayerName are fake entries that count their calls, so
//     "was the freed pawn touched at all?" is answered by the counters, not by
//     surviving the run;
//   * Object.m_CachedPtr (@0x10) is set on live fake objects and cleared on
//     "destroyed" ones - that is what EspObjectAlive() keys on.
//
// Build and run from src/main/jni:
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I Skin/tests/stubs -I . -I ImGui -I Includes \
//       Hacks/Feature/tests/esp_walk_check.cpp ImGui/imgui.cpp ImGui/imgui_draw.cpp \
//       ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp -o /tmp/esp_walk_check \
//       && /tmp/esp_walk_check
// Android.mk does not wildcard Hacks/Feature/tests, so this file never reaches
// the APK.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstddef>
#include <sys/mman.h>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/OverlayGeometry.h"   // glWidth/glHeight, read by DrawESP.h
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "Hacks/StructGame/Defines.h"

// Defines.h maps every native to a `m_unity + off` macro, and StructSDK.h's
// inline bodies bake that expression in when they are parsed - undefining the
// macros afterwards is useless, the code is already written. So Defines.h is
// pulled in first (it is `#pragma once`, StructGame.h cannot re-expand it),
// the five natives this test replaces are un-macro'd, and only then is
// StructSDK.h parsed: its call sites now read the uintptr_t globals Defines.h
// also declares, which the fakes below can point at themselves.
#undef Class_Gameplay_get_MatchGame
#undef Class_Gameplay_get_LocalPawn
#undef Class_Camera_get_main
#undef Class_Camera_WorldToScreenPoint
#undef Class_Transform_get_position
#undef Class_Pawn_get_PlayerName

// Defines.h only #defines these two - it declares no global for them.
uintptr_t Class_Camera_get_main = 0;
uintptr_t Class_Camera_WorldToScreenPoint = 0;

#include "Hacks/StructGame/StructGame.h"

#include "Hacks/Feature/Init_Hacks.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Hacks/Feature/DrawESP.h"

// ---- host stubs for symbols the real game/font libs provide on device -----
ImFont *F50 = nullptr;
namespace font { ImFont *inter_semibold = nullptr; }
float speedHackMultiplier = 1.0f;
void MSHookFunction(void *, void *, void **) {}
void SmartReloadMarkShotFired() {}
const char *g_cStringText = "";
const char *Il2CppString::CString() { return g_cStringText; }

// ---------------------------------------------------------------------------
// arena: everything the fake game owns, so IsPtrValid can tell live from freed.
// ---------------------------------------------------------------------------
static std::vector<uintptr_t> g_live;

static bool ArenaContains(uintptr_t addr)
{
    for (uintptr_t base : g_live)
        if (addr >= base && addr < base + 0x1000)
            return true;
    return false;
}

// get_width()/get_height() are inline in Defines.h and bake `m_unity + api4`
// / `m_unity + api5` into their bodies when it is parsed, so they cannot be
// pointed at a fake the way the other natives can. Instead a fake "libunity"
// is mapped at m_unity and two 6-byte `mov eax, imm32; ret` stubs are planted
// at exactly those offsets - the real inline bodies then run unmodified.
#if defined(__x86_64__) || defined(__aarch64__)
static uint8_t *g_fakeUnity = nullptr;
static int g_screenWidth = 1080;
static int g_screenHeight = 2400;

static void PlantReturnStub(uint8_t *at, int value)
{
#if defined(__x86_64__)
    at[0] = 0xB8;                                   // mov eax, imm32
    std::memcpy(at + 1, &value, sizeof(value));
    at[5] = 0xC3;                                   // ret
#else
    // aarch64: mov w0, #imm ; ret  (imm16 only - the test uses screen sizes
    // that fit in 16 bits for the FOV path)
    const uint32_t movz = 0x52800000u | ((uint32_t)value << 5);
    std::memcpy(at, &movz, sizeof(movz));
    const uint32_t ret = 0xD65F03C0u;
    std::memcpy(at + 4, &ret, sizeof(ret));
#endif
}

static void InstallFakeScreen()
{
    if (g_fakeUnity == nullptr)
    {
        // api11/api12 are past 0x8000000, so the reservation has to cover them
        const size_t span = 0xA000000;
        void *region = mmap(nullptr, span, PROT_NONE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (region == MAP_FAILED)
            return;
        g_fakeUnity = (uint8_t *)region;
    }
    // api4 (0x50504BC) and api5 (0x5050520) share one page
    uint8_t *page = (uint8_t *)(((uintptr_t)g_fakeUnity + api4) & ~(uintptr_t)0xFFF);
    if (mprotect(page, 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        return;
    PlantReturnStub(g_fakeUnity + api4, g_screenWidth);
    PlantReturnStub(g_fakeUnity + api5, g_screenHeight);
    m_unity = (uintptr_t)g_fakeUnity;
}
#endif

static void *ArenaBlock()
{
    uint8_t *block = new uint8_t[0x1000]();
    g_live.push_back((uintptr_t)block);
    return block;
}

bool Tools::IsPtrValid(void *addr)
{
    if (addr == nullptr)
        return false;
    return ArenaContains((uintptr_t)addr);
}

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio; (void)tag; (void)fmt;
    return 0;
}

// ---------------------------------------------------------------------------
// fake natives
// ---------------------------------------------------------------------------
static int g_meshPositionCalls = 0;
static int g_worldToScreenCalls = 0;
static int g_headBoneReads = 0;
static std::vector<uintptr_t> g_positionSelfs;

static uintptr_t g_matchGame = 0;
static uintptr_t g_localPawn = 0;

static int g_failed = 0;

static void Check(bool condition, const char *what)
{
    std::printf("%s  %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition)
        ++g_failed;
}

static Transform *g_camera = nullptr;

// The pawn list is a real Il2CppList<Pawn*>: klass, unk1, items(Il2CppArray*),
// size, version - built at the offsets the game uses.
// Mirrors Il2CppArray<T> exactly: klass, monitor, bounds, int32 max_length (then
// padding) and only then m_Items - getPointer() returns &m_Items, i.e. +32.
struct FakeArray
{
    Il2CppClass *klass;
    void *monitor;
    void *bounds;
    int32_t maxLength;
    uintptr_t items[8];
};

struct FakeList
{
    Il2CppClass *klass;
    void *unk1;
    FakeArray *array;
    int size;
    int version;
};

static FakeArray *g_array = nullptr;
static FakeList *g_list = nullptr;

static uintptr_t FakeMatchGame()
{
    return g_matchGame;
}

static uintptr_t FakeLocalPawn()
{
    return g_localPawn;
}

// StructSDK calls this as `Vector3 (*)(Transform *)`, so the fake has to return
// by value - a (Transform*, Vector3*) fake would read a register as an out
// pointer and be handed back garbage coordinates.
static Vector3 FakeTransformGetPosition(Transform *self)
{
    ++g_meshPositionCalls;
    g_positionSelfs.push_back((uintptr_t)self);
    return Vector3{0.0f, 0.0f, 0.0f};
}

static Camera *FakeCameraMain()
{
    return (Camera *)g_camera;
}

static String *g_playerName = nullptr;
static int g_playerNameCalls = 0;

static String *FakePlayerName(Pawn *self)
{
    (void)self;
    ++g_playerNameCalls;
    return g_playerName;
}

static Vector3 FakeWorldToScreenPoint(Camera *self, Vector3 world)
{
    (void)self;
    ++g_worldToScreenCalls;
    return Vector3{ world.x, world.y, 1.0f };
}

// A pawn block: m_Mesh @0x628 holds a transform, m_IsAlive @0x548 a bool,
// m_HeadBone @0x308 a transform.
static uintptr_t MakePawn(Transform *mesh, bool alive, Transform *headBone)
{
    uint8_t *pawn = (uint8_t *)ArenaBlock();
    // Object.m_CachedPtr @0x10 - non-null means the native object is still
    // there. A destroyed pawn is the same block with this cleared.
    *(void **)(pawn + 0x10) = pawn;
    *(Transform **)(pawn + Class_Pawn_m_Mesh) = mesh;
    *(bool *)(pawn + Class_Pawn_m_IsAlive) = alive;
    *(Transform **)(pawn + Class_Pawn_m_HeadBone) = headBone;
    ++g_headBoneReads;
    return (uintptr_t)pawn;
}

static uint8_t *MakeTransform()
{
    uint8_t *transform = (uint8_t *)ArenaBlock();
    *(void **)(transform + 0x10) = transform;   // m_CachedPtr: native side alive
    return transform;
}

static bool SetUpMatch()
{
    g_live.clear();
    g_meshPositionCalls = 0;
    g_worldToScreenCalls = 0;
    g_headBoneReads = 0;
    g_positionSelfs.clear();
    InstallFakeScreen();

    Class_Gameplay_get_MatchGame = (uintptr_t)&FakeMatchGame;
    Class_Gameplay_get_LocalPawn = (uintptr_t)&FakeLocalPawn;
    Class_Camera_get_main = (uintptr_t)&FakeCameraMain;
    Class_Camera_WorldToScreenPoint = (uintptr_t)&FakeWorldToScreenPoint;
    Class_Transform_get_position = (uintptr_t)&FakeTransformGetPosition;
    Class_Pawn_get_PlayerName = (uintptr_t)&FakePlayerName;
    g_playerName = nullptr;
    g_playerNameCalls = 0;
    g_cStringText = "";

    g_camera = (Transform *)ArenaBlock();
    *(void **)((uint8_t *)g_camera + 0x10) = g_camera;   // m_CachedPtr
    g_matchGame = (uintptr_t)ArenaBlock();
    // the list and its backing array have to live in the arena too: the walk
    // checks IsPtrValid() on both before touching them.
    g_array = (FakeArray *)ArenaBlock();
    g_list = (FakeList *)ArenaBlock();
    g_array->klass = nullptr;
    g_array->bounds = nullptr;
    g_array->maxLength = 8;
    g_list->klass = nullptr;
    g_list->unk1 = nullptr;
    g_list->array = g_array;
    g_list->size = 0;
    g_list->version = 0;
    *(List<uintptr_t> **)(g_matchGame + Class_BaseGame_EnemyPawns) = (List<uintptr_t> *)g_list;

    g_localPawn = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
    return true;
}

// Host-only link stub: the include graph references the A64 hook engine.
extern "C" void A64HookFunction(void *const symbol, void *const replace, void **result)
{ (void)symbol; if (result) *result = replace; }

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    // ---- R1: a list entry that is NOT a live object ------------------------
    SetUpMatch();
    {
        uintptr_t destroyed = (uintptr_t)new uint8_t[0x400]();   // never registered
        g_array->items[0] = destroyed;
        g_list->size = 1;
        const uintptr_t target = GetClosestTarget();
        Check(target == 0, "R1 a destroyed list entry is skipped, not dereferenced");
        bool touchedDestroyed = false;
        for (uintptr_t self : g_positionSelfs)
            if (self == destroyed)
                touchedDestroyed = true;
        Check(!touchedDestroyed,
              "R1 no getter ran on the destroyed entry");
        delete[] (uint8_t *)destroyed;
    }

    // ---- R2: a live pawn that is DEAD is skipped ---------------------------
    SetUpMatch();
    {
        g_array->items[0] = MakePawn((Transform *)MakeTransform(), false, (Transform *)MakeTransform());
        g_list->size = 1;
        Check(GetClosestTarget() == 0, "R2 a dead pawn is skipped");
    }

    // ---- R3: a live, alive pawn is returned -------------------------------
    SetUpMatch();
    {
        const uintptr_t pawn = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
        g_array->items[0] = pawn;
        g_list->size = 1;
        Check(GetClosestTarget() == pawn, "R3 a live, alive pawn is returned");
    }

    // ---- R4: a destroyed entry before the live one does not poison the walk
    SetUpMatch();
    {
        const uintptr_t pawn = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
        g_array->items[0] = (uintptr_t)new uint8_t[0x400]();
        g_array->items[1] = pawn;
        g_list->size = 2;
        Check(GetClosestTarget() == pawn,
              "R4 a destroyed entry earlier in the list still lets the live pawn win");
        delete[] (uint8_t *)g_array->items[0];
    }

    // ---- R5: an implausible size is refused outright ----------------------
    SetUpMatch();
    {
        const uintptr_t pawn = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
        g_array->items[0] = pawn;
        g_list->size = 100000;
        Check(GetClosestTarget() == 0, "R5 a corrupt list size is refused, list not walked");
    }

    // ---- R6: the FOV picker survives a dead camera ------------------------
    SetUpMatch();
    {
        g_array->items[0] = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
        g_list->size = 1;
        g_camera = nullptr;
        Check(GetInsideFOVTarget() == 0, "R6 a gone camera returns no target instead of calling into it");
    }

    // ---- R7: the FOV picker still works with a live camera ----------------
    SetUpMatch();
    {
        const uintptr_t pawn = MakePawn((Transform *)MakeTransform(), true, (Transform *)MakeTransform());
        g_array->items[0] = pawn;
        g_list->size = 1;
        Check(GetInsideFOVTarget() != 0, "R7 a live camera + live pawn still produces a target");
    }

    // ---- R12: a DESTROYED pawn (readable wrapper, null m_CachedPtr) --------
    // The pawn list keeps an entry for a few frames after a death/respawn. Every
    // IsPtrValid() guard passes on it, and the old code then called
    // get_position() / get_HeadPosition() on it, which walks the cleared native
    // pointer. This is the crash being reported.
    SetUpMatch();
    {
        uint8_t *destroyedPawn = (uint8_t *)MakePawn((Transform *)MakeTransform(), true,
                                                     (Transform *)MakeTransform());
        *(void **)(destroyedPawn + 0x10) = nullptr;      // Object.m_CachedPtr cleared
        g_array->items[0] = (uintptr_t)destroyedPawn;
        g_list->size = 1;
        Check(GetClosestTarget() == 0,
              "R12 a destroyed pawn in the list is skipped");
        bool touched = false;
        for (uintptr_t self : g_positionSelfs)
            if (self == (uintptr_t)*(Transform **)(destroyedPawn + Class_Pawn_m_Mesh))
                touched = true;
        Check(!touched, "R12 its mesh was never asked for a position");
    }

    // ---- R13: a live pawn whose mesh was destroyed is skipped too ---------
    SetUpMatch();
    {
        Transform *deadMesh = (Transform *)MakeTransform();
        *(void **)((uint8_t *)deadMesh + 0x10) = nullptr;
        g_array->items[0] = MakePawn(deadMesh, true, (Transform *)MakeTransform());
        g_list->size = 1;
        Check(GetClosestTarget() == 0, "R13 a pawn with a destroyed mesh is skipped");
    }

    // ---- R8: HP of a pawn whose AttackableInfo was cleared ---------------
    // This is the per-frame read in DrawESP's pawn loop: it used to do
    // `*(float *)(m_AttackableInfo + m_Health)` with no check, and a pawn that
    // just died / is respawning has that field null while the wrapper itself
    // still passes IsPtrValid().
    SetUpMatch();
    {
        uint8_t *pawn = (uint8_t *)MakePawn((Transform *)MakeTransform(), true,
                                            (Transform *)MakeTransform());
        *(uintptr_t *)(pawn + Class_AttackableTarget_m_AttackableInfo) = 0;
        int hp = -1, maxHp = -1;
        Check(!EspPawnHealth((uintptr_t)pawn, hp, maxHp),
              "R8 a cleared AttackableInfo is refused instead of dereferenced");
        Check(hp == -1 && maxHp == -1, "R8 nothing was written to the caller's values");

        *(uintptr_t *)(pawn + Class_AttackableTarget_m_AttackableInfo) = (uintptr_t)ArenaBlock();
        uintptr_t info = *(uintptr_t *)(pawn + Class_AttackableTarget_m_AttackableInfo);
        *(float *)(info + Class_AttackableTarget_m_Health) = 73.0f;
        *(float *)(info + Class_AttackableTarget_m_MaxHealth) = 100.0f;
        Check(EspPawnHealth((uintptr_t)pawn, hp, maxHp) && hp == 73 && maxHp == 100,
              "R8 a live AttackableInfo still yields the health values");
    }

    // ---- R9/R10/R11: the name path the name ESP uses ----------------------
    SetUpMatch();
    {
        uint8_t *player = (uint8_t *)MakePawn((Transform *)MakeTransform(), true,
                                              (Transform *)MakeTransform());
        *(bool *)(player + Class_Pawn_m_IsBot) = false;

        // Pawn.get_PlayerName() answers null for a pawn whose PlayerInfo is
        // gone; the old call site ran String::CString() on that null.
        g_playerName = nullptr;
        Check(EspPawnName((Pawn *)player).empty(),
              "R9 a null player name yields no text instead of crashing");
        Check(g_playerNameCalls == 1, "R9 the null name was asked for exactly once");

        // a real name comes through
        Il2CppString *name = (Il2CppString *)ArenaBlock();
        g_playerName = name;
        g_cStringText = "PlayerOne";
        Check(EspPawnName((Pawn *)player) == "PlayerOne",
              "R10 a live player name is returned");

        // and a bot never reaches the native call at all
        uint8_t *bot = (uint8_t *)MakePawn((Transform *)MakeTransform(), true,
                                           (Transform *)MakeTransform());
        *(bool *)(bot + Class_Pawn_m_IsBot) = true;
        g_playerNameCalls = 0;
        Check(EspPawnName((Pawn *)bot) == "BOT", "R11 a bot is labelled BOT");
        Check(g_playerNameCalls == 0, "R11 the native name getter is not called for a bot");
    }

    std::printf("\n%s (%d failed)\n", g_failed == 0 ? "ALL PASS" : "FAILURES", g_failed);
    return g_failed == 0 ? 0 : 1;
}
