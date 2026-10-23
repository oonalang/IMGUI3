// Host check for the placed-item ESP. It stubs the game SDK surface the
// header expects and drives the tracked/erased decision with a fake projectile
// in ordinary host memory, so a regression in the "is this already gone?"
// logic fails here instead of on a phone.
//
//   g++ -std=c++17 -Isrc/main/jni -Isrc/main/jni/Skin/tests/stubs
//       src/main/jni/System/Hooks/tests/placable_esp_check.cpp -o /tmp/placable_check

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>

extern "C" void MSHookFunction(void *, void *, void **) {}
extern "C" int __android_log_print(int, const char *, const char *, ...) { return 0; }

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"

struct Vector2
{
    float x = 0.0f, y = 0.0f;
    Vector2() = default;
    Vector2(float ax, float ay) : x(ax), y(ay) {}
};

struct Vector3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3() = default;
    Vector3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}
    static Vector3 zero() { return Vector3{}; }
    static float Distance(const Vector3 &a, const Vector3 &b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }
    bool operator==(const Vector3 &o) const { return x == o.x && y == o.y && z == o.z; }
};

struct Transform
{
    Vector3 get_position() { return Vector3{}; }
};

struct Camera
{
    static Camera *get_main() { return nullptr; }
    Vector3 WorldToScreenPoint(Vector3) { return Vector3{}; }
};

struct Pawn {};

namespace Tools
{
    inline bool IsPtrValid(void *p) { return p != nullptr; }
}

inline void *getAbsoluteAddress(const char *, uintptr_t) { return nullptr; }

inline int get_width() { return 1080; }
inline int get_height() { return 2400; }

namespace GamePlay
{
    inline void *get_MatchGame() { return nullptr; }
    inline Pawn *get_LocalPawn() { return nullptr; }
}

template <typename T> struct Il2CppArray
{
    void *klass = nullptr;
    void *monitor = nullptr;
    void *bounds = nullptr;
    int max_length = 0;
    T m_Items[1];

    T *getPointer() { return m_Items; }
};

template <typename T> struct Il2CppList
{
    void *klass = nullptr;
    void *monitor = nullptr;
    Il2CppArray<T> *items = nullptr;
    int size = 0;
    int version = 0;

    T *getItems() { return items->getPointer(); }
    int getSize() { return size; }
};

struct EspMenu
{
    bool GrenadeWarn = false;
    bool Placables = false;
};

struct ConfigT
{
    EspMenu ESPMenu;
};
static ConfigT Config;

inline void DrawText1(ImDrawList *, const std::string &, const Vector2 &, ImU32, float) {}

inline void DrawTextWithBorder1(ImDrawList *, const std::string &, const Vector2 &,
                                ImU32, ImU32, float) {}

constexpr uintptr_t Class_AttackableTarget_m_AttackableInfo = 0x78;

#include "System/Hooks/ThrowableAlert.h"
#include "System/Hooks/PlacableEsp.h"

static int g_failures = 0;

static void Check(const char *what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

static void SetTarget(std::uint8_t *obj, std::uint8_t *klass, const char *name)
{
    *(std::uintptr_t *)(void *)obj = (std::uintptr_t)klass;
    *(const char **)(void *)(klass + 0x10) = name;
}

int main()
{
    alignas(16) std::uint8_t projectile[0x200] = {};
    alignas(16) std::uint8_t klass[0x40] = {};

    SetTarget(projectile, klass, "WeaponProjectile_TripMine");

    // bHasExplode at 0xA1: the mine is explosive from the moment it exists.
    projectile[0xA1] = 1;
    projectile[0x18D] = 0;                 // not destroyed
    *(int *)(void *)(projectile + 0xB0) = 3;

    Check("the class name maps to a label",
          PlacableLabelFor("WeaponProjectile_TripMine") != nullptr);

    Check("a live mine is not reported as exploded",
          !WeaponProjectileHasExploded(projectile));

    Config.ESPMenu.Placables = true;
    TrackPlacable(projectile, PlcRouteTick);
    Check("an explosive mine that has not gone off is tracked", g_Placables.size() == 1);

    projectile[0x18D] = 1;
    TrackPlacable(projectile, PlcRouteTick);
    Check("a destroyed mine is dropped", g_Placables.empty());

    projectile[0x18D] = 0;
    g_Placables.clear();
    Config.ESPMenu.Placables = false;
    TrackPlacable(projectile, PlcRouteTick);
    Check("nothing is tracked while the feature is off", g_Placables.empty());

    TrackPlacable(nullptr, PlcRouteList);
    Check("a null projectile is refused", g_Placables.empty());

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }

    std::printf("OK: placable_esp_check - live mines are tracked, destroyed ones dropped\n");
    return 0;
}
