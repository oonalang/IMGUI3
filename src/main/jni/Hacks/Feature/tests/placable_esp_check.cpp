#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/Feature/Init_Hacks.h"

extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace; (void)result;
}

ImFont *F50 = nullptr;
namespace font {
ImFont *inter_semibold = nullptr;
}

namespace Tools {
bool IsPtrValid(void *addr)
{
    return addr != nullptr && (uintptr_t)addr >= 0x10000;
}
}

struct Camera {
    static Camera *get_main() { static Camera cam; return &cam; }
    Vector3 WorldToScreenPoint(Vector3 world)
    {
        return Vector3(960.0f + world.x * 10.0f, 540.0f + world.y * 10.0f, world.z);
    }
};

struct Pawn {
    unsigned char bytes[0x100];
};
static Pawn *g_testLocalPawn = nullptr;

static void *g_testMatchGame = nullptr;
struct GamePlay {
    static Pawn *get_LocalPawn() { return g_testLocalPawn; }
    static void *get_MatchGame() { return g_testMatchGame; }
};

#define Class_AttackableTarget_m_AttackableInfo (0x78)

static std::unordered_map<void *, Vector3> g_fakePos;
Vector3 GetThrowPos(void *proj)
{
    auto it = g_fakePos.find(proj);
    return it != g_fakePos.end() ? it->second : Vector3::zero();
}

bool ThrowToScreen(Camera *cam, const Vector3 &world, int height, ImVec2 &out)
{
    Vector3 screen = cam->WorldToScreenPoint(world);
    if (screen.z <= 0.01f)
        return false;
    out = ImVec2(screen.x, height - screen.y);
    return true;
}

void DrawTextWithBorder1(ImDrawList *draw, const std::string &text, const Vector2 &position,
                         ImU32 textColor, ImU32 borderColor, float fontSize)
{
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            if (x || y)
                draw->AddText(NULL, fontSize, ImVec2(position.x + x, position.y + y), borderColor, text.c_str());
    draw->AddText(NULL, fontSize, ImVec2(position.x, position.y), textColor, text.c_str());
}

#include "System/Hooks/PlacableEsp.h"

struct FakeKlass {
    void *image;
    void *gc_desc;
    const char *name;
};

struct FakeProj {
    unsigned char mem[0x200];
    FakeKlass klass;
    explicit FakeProj(const char *className)
    {
        memset(mem, 0, sizeof(mem));
        klass.image = nullptr;
        klass.gc_desc = nullptr;
        klass.name = className;
        *(void **)mem = &klass;
    }
    void *ptr() { return mem; }
    void setExploded(bool v) { *(bool *)(mem + 0xA1) = v; }
    void setDestroyed(bool v) { *(bool *)(mem + 0x18D) = v; }
    void setCamp(int c) { *(int *)(mem + 0xB0) = c; }
};

struct FakeProjArray {
    void *klass;
    void *monitor;
    void *bounds;
    int max_length;
    void *items[8];
};

struct FakeProjList {
    void *klass;
    void *monitor;
    FakeProjArray *items;
    int size;
    int version;
};

static FakeProjList *g_fakeProjList = nullptr;
static void *FakeProjListGetter(void *) { return g_fakeProjList; }

static int g_fakePushCalls = 0;
static void FakePush(void *, void *) { g_fakePushCalls++; }
static bool g_fakeRemoveResult = true;
static bool FakeRemove(void *, void *) { return g_fakeRemoveResult; }

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

static bool AnyVertexColor(ImDrawList &draw, ImU32 col)
{
    for (const ImDrawVert &v : draw.VtxBuffer)
        if (v.col == col)
            return true;
    return false;
}

int main()
{
    CHECK(Config.ESPMenu.Placables == false, "Placed Items ESP defaults OFF");

    CHECK(PlacableLabelFor("WeaponProjectile_TripMine") != nullptr &&
          strcmp(PlacableLabelFor("WeaponProjectile_TripMine"), "TRIP MINE") == 0, "tripmine classified");
    CHECK(PlacableLabelFor("WeaponProjectile_C4bomb") != nullptr, "C4 classified");
    CHECK(PlacableLabelFor("WeaponProjectile_TankBattleC4") != nullptr, "tank-battle C4 classified");
    CHECK(PlacableLabelFor("WeaponProjectile_TankMine") != nullptr, "tank mine classified");
    CHECK(PlacableLabelFor("WeaponProjectile_TrophySystem") != nullptr, "trophy system classified");
    CHECK(PlacableLabelFor("WeaponProjectile_TacTripWire") != nullptr, "trip sensor classified");
    CHECK(PlacableLabelFor("WeaponProjectile_TrapChipMine") != nullptr, "trap chip mine classified");
    CHECK(PlacableLabelFor("WeaponProjectile_ElectricTripWire") != nullptr, "electric trip wire classified");
    CHECK(PlacableLabelFor("WeaponProjectile_Bullet") == nullptr, "bullets are not placables");
    CHECK(PlacableLabelFor("WeaponProjectile_NA45") == nullptr, "NA45 rounds are not placables");
    CHECK(PlacableLabelFor("WeaponProjectile_Smoke") == nullptr, "smoke is not a placed device");
    CHECK(PlacableLabelFor("") == nullptr, "empty class name rejected");

    CHECK(strcmp(PlacableLabelFor("WeaponProjectile_TripMine2"), "TRIP MINE") == 0,
          "a suffixed class name still classifies (tolerant match)");
    CHECK(strcmp(PlacableLabelFor("tripMine"), "TRIP MINE") == 0, "letter case does not matter");
    CHECK(strcmp(PlacableLabelFor("ImprovisedTripMine"), "TRIP MINE") == 0,
          "a renamed (prefix-dropped) class still classifies");
    CHECK(strcmp(PlacableLabelFor("CGame.WeaponProjectile_TrophySystem"), "TROPHY SYSTEM") == 0,
          "a namespaced class still classifies");
    CHECK(strcmp(PlacableLabelFor("WeaponProjectile_TacTripWire"), "TRIP SENSOR") == 0 &&
          strcmp(PlacableLabelFor("WeaponProjectile_TripMine"), "TRIP MINE") == 0,
          "the specific device keys still win over the shortened ones");

    CHECK(!g_placableHookLanded[PlcHookStick] && !g_placableHookLanded[PlcHookTripMine] &&
          !g_placableHookLanded[PlcHookTrapChip] && !g_placableHookLanded[PlcHookTripWire],
          "no Tick hook is reported installed until the installer records one");
    NotePlacableHookLanded(PlcHookTripMine, true);
    CHECK(g_placableHookLanded[PlcHookTripMine] && !g_placableHookLanded[PlcHookStick],
          "the install verdict is recorded per hook");

    FakeProj tripmine("WeaponProjectile_TripMine");
    FakeProj c4("WeaponProjectile_C4bomb");
    FakeProj trophy("WeaponProjectile_TrophySystem");
    FakeProj sensor("WeaponProjectile_TacTripWire");
    FakeProj na45("WeaponProjectile_NA45");
    tripmine.setCamp(1);
    c4.setCamp(2);
    trophy.setCamp(2);
    sensor.setCamp(1);
    g_fakePos[tripmine.ptr()] = Vector3(5.0f, 0.0f, 10.0f);
    g_fakePos[c4.ptr()] = Vector3(-8.0f, 1.0f, 20.0f);
    g_fakePos[trophy.ptr()] = Vector3(3.0f, 0.0f, 10.0f);
    g_fakePos[sensor.ptr()] = Vector3(0.0f, 0.0f, 15.0f);
    g_fakePos[na45.ptr()] = Vector3(1.0f, 1.0f, 10.0f);

    TrackPlacable(tripmine.ptr());
    CHECK(g_Placables.empty(), "nothing tracked while the toggle is OFF");

    Config.ESPMenu.Placables = true;
    TrackPlacable(tripmine.ptr());
    TrackPlacable(c4.ptr());
    TrackPlacable(trophy.ptr());
    TrackPlacable(sensor.ptr());
    TrackPlacable(na45.ptr());
    CHECK(g_Placables.size() == 4, "four placables tracked, NA45 ignored");
    CHECK(g_Placables.count((uintptr_t)tripmine.ptr()) &&
          strcmp(g_Placables[(uintptr_t)tripmine.ptr()].label, "TRIP MINE") == 0,
          "tripmine stored with its label");
    CHECK(g_Placables[(uintptr_t)c4.ptr()].camp == 2, "owner camp stored");

    CHECK(g_placableDiag.tickCalls >= 5 && g_placableDiag.classified >= 4,
          "the diagnosis counts every tick it saw and every device it classified");
    CHECK(g_placableDiag.tracked >= 4, "... and every write into the map");
    CHECK(strcmp(g_placableDiag.lastClass, "WeaponProjectile_TacTripWire") == 0,
          "... and remembers the last device class it classified");

    const int trackedBeforeBaseRoute = g_placableDiag.tracked;
    PlacableOnProjectileTick(na45.ptr());
    CHECK(g_placableDiag.tracked == trackedBeforeBaseRoute,
          "the base-Tick route ignores a projectile that is not a device");
    PlacableOnProjectileTick(sensor.ptr());
    CHECK(g_placableDiag.tracked == trackedBeforeBaseRoute + 1,
          "the base-Tick route tracks a device");
    PlacableOnProjectileTick(nullptr);
    CHECK(g_placableDiag.tracked == trackedBeforeBaseRoute + 1,
          "the base-Tick route refuses a null instance");
    Config.ESPMenu.Placables = false;
    PlacableOnProjectileTick(sensor.ptr());
    CHECK(g_placableDiag.tracked == trackedBeforeBaseRoute + 1,
          "the base-Tick route is inert while the toggle is off");
    Config.ESPMenu.Placables = true;

    tripmine.setExploded(true);
    TrackPlacable(tripmine.ptr());
    CHECK(g_Placables.size() == 3 && !g_Placables.count((uintptr_t)tripmine.ptr()),
          "exploded tripmine removed on its next tick");

    c4.setDestroyed(true);
    TrackPlacable(c4.ptr());
    CHECK(g_Placables.size() == 2 && !g_Placables.count((uintptr_t)c4.ptr()),
          "destroyed C4 removed on its next tick");

    TrackPlacable(nullptr);
    FakeProj corrupt("WeaponProjectile_TripMine");
    *(void **)corrupt.mem = (void *)0x1234;
    TrackPlacable(corrupt.ptr());
    FakeProj nullName("WeaponProjectile_TripMine");
    nullName.klass.name = nullptr;
    TrackPlacable(nullName.ptr());
    FakeProj badName("WeaponProjectile_TripMine");
    badName.klass.name = "\x01\x02garbage";
    TrackPlacable(badName.ptr());

    FakeProj noPos("WeaponProjectile_TankMine");
    TrackPlacable(noPos.ptr());
    CHECK(g_Placables.size() == 3 && g_Placables.count((uintptr_t)noPos.ptr()),
          "null/corrupt instances ignored, no crash; a positionless device is still tracked");

    static unsigned char pawnMem[0x100] = {};
    static unsigned char infoMem[0x100] = {};
    *(uintptr_t *)(pawnMem + Class_AttackableTarget_m_AttackableInfo) = (uintptr_t)infoMem;
    *(int *)(infoMem + 0x58) = 2;
    g_testLocalPawn = (Pawn *)pawnMem;
    CHECK(PlacableLocalCamp() == 2, "local camp read through validated AttackableInfo");

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    ImGui::NewFrame();

    const ImU32 kHostileFill = IM_COL32(255, 96, 48, 190);
    const ImU32 kFriendlyFill = IM_COL32(90, 255, 170, 190);

    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList.AddDrawCmd();
    DrawPlacableESP(&drawList, 1920, 1080, Vector3::zero());
    CHECK(drawList.VtxBuffer.Size > 0, "markers emitted for in-range placables");
    CHECK(AnyVertexColor(drawList, kFriendlyFill), "same-camp trophy drawn friendly (green)");
    CHECK(AnyVertexColor(drawList, kHostileFill), "enemy-camp sensor drawn hostile (orange)");
    CHECK(g_placableDiag.drawn >= 2, "the diagnosis reports what the draw pass actually emitted");
    CHECK(g_placableDiag.posFailed >= 1,
          "a device whose position resolves to nothing is counted as no-pos, not drawn");

    *(int *)(infoMem + 0x58) = 7;
    ImDrawList drawList2(ImGui::GetDrawListSharedData());
    drawList2.AddDrawCmd();
    DrawPlacableESP(&drawList2, 1920, 1080, Vector3::zero());
    CHECK(AnyVertexColor(drawList2, kHostileFill) && !AnyVertexColor(drawList2, kFriendlyFill),
          "camp mismatch falls back to hostile coloring");

    for (int i = 0; i < 400; ++i)
        TrackPlacable(sensor.ptr());
    ImDrawList drawList3(ImGui::GetDrawListSharedData());
    drawList3.AddDrawCmd();
    DrawPlacableESP(&drawList3, 1920, 1080, Vector3::zero());
    CHECK(g_Placables.size() == 1 && g_Placables.count((uintptr_t)sensor.ptr()),
          "entries unseen for 360+ ticks expire");

    g_fakePos[sensor.ptr()] = Vector3(0.0f, 0.0f, 500.0f);
    TrackPlacable(sensor.ptr());
    ImDrawList drawList4(ImGui::GetDrawListSharedData());
    drawList4.AddDrawCmd();
    DrawPlacableESP(&drawList4, 1920, 1080, Vector3::zero());
    CHECK(drawList4.VtxBuffer.Size == 0, "placable beyond MaxDist not drawn");
    CHECK(g_placableDiag.drawn == 0,
          "the diagnosis says 0 drawn for the out-of-range pass (that is the 'in range?' question)");

    g_fakePos[sensor.ptr()] = Vector3(0.0f, 0.0f, -5.0f);
    TrackPlacable(sensor.ptr());
    ImDrawList drawList5(ImGui::GetDrawListSharedData());
    drawList5.AddDrawCmd();
    DrawPlacableESP(&drawList5, 1920, 1080, Vector3::zero());
    CHECK(drawList5.VtxBuffer.Size == 0, "placable behind camera not drawn");

    Config.ESPMenu.Placables = false;
    DrawPlacableESP(&drawList5, 1920, 1080, Vector3::zero());
    CHECK(g_Placables.empty(), "toggling OFF clears tracked placables");

    Config.ESPMenu.Placables = true;
    FakeProj pushedC4("WeaponProjectile_C4bomb");
    pushedC4.setCamp(2);
    g_fakePos[pushedC4.ptr()] = Vector3(1.0f, 0.0f, 2.0f);
    orig_PushProjectile = FakePush;
    hook_PushProjectile((void *)&g_fakeProjList, pushedC4.ptr());
    CHECK(g_fakePushCalls == 1 && g_Placables.count((uintptr_t)pushedC4.ptr()),
          "PushProjectile registers a device the game adds to its own table");
    CHECK(g_placableDiag.pushed >= 1, "the diagnosis counts push-route registrations");

    orig_RemoveWeaponProjectile = FakeRemove;
    CHECK(hook_RemoveWeaponProjectile((void *)&g_fakeProjList, pushedC4.ptr()) == true &&
          !g_Placables.count((uintptr_t)pushedC4.ptr()),
          "RemoveWeaponProjectile drops the marker with the game's own removal");
    g_fakeRemoveResult = false;
    hook_PushProjectile((void *)&g_fakeProjList, pushedC4.ptr());
    CHECK(hook_RemoveWeaponProjectile((void *)&g_fakeProjList, pushedC4.ptr()) == false &&
          g_Placables.count((uintptr_t)pushedC4.ptr()),
          "a refused removal keeps the marker");

    FakeProj listMine("WeaponProjectile_TripMine");
    g_fakePos[listMine.ptr()] = Vector3(4.0f, 0.0f, 6.0f);
    FakeProjArray arr = {};
    arr.max_length = 2;
    arr.items[0] = listMine.ptr();
    arr.items[1] = nullptr;
    FakeProjList list = {};
    list.items = &arr;
    list.size = 2;
    g_fakeProjList = &list;
    g_testMatchGame = &arr;
    g_placableListGetter = &FakeProjListGetter;
    const int scanned = PlacableScanLiveList();
    CHECK(scanned == 1 && g_Placables.count((uintptr_t)listMine.ptr()),
          "the live-projectile table is walked WITHOUT any hook: device tracked");
    CHECK(g_placableDiag.scanned >= 1, "the diagnosis counts list-route registrations");

    g_fakeProjList = nullptr;
    CHECK(PlacableScanLiveList() == -1, "a null live table is ignored and reported as unreadable");

    ImDrawList drawList6(ImGui::GetDrawListSharedData());
    drawList6.AddDrawCmd();
    DrawPlacableESP(&drawList6, 1920, 1080, Vector3::zero());
    CHECK(AnyVertexColor(drawList6, kHostileFill), "a list-route trip mine is drawn like any other");

    ImGui::EndFrame();
    ImGui::DestroyContext();

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all placable-ESP checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("FAILED: %d of %d checks failed\n", g_failures, g_checks);
    return 1;
}

