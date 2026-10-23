// Host syntax check for the skeleton ESP: stubs the game SDK surface the
#include <cstdio>
// header expects, so a typo in Skeleton.h fails here instead of in the NDK
// build.
#include <cmath>
#include <cstdint>

#include "ImGui/imgui.h"

struct Vector3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3() = default;
    Vector3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}
    static Vector3 zero() { return Vector3{}; }
};

struct Transform
{
    Vector3 get_position() { return Vector3{1.0f, 2.0f, 3.0f}; }
};

struct Camera
{
    static Camera *get_main() { return nullptr; }
    Vector3 WorldToScreenPoint(Vector3) { return Vector3{}; }
};

struct Pawn {};

namespace Tools
{
    inline bool IsPtrValid(void *) { return false; }
}

struct EspMenu { bool Skeleton = true; };
struct ConfigT
{
    EspMenu ESPMenu;
    struct { float SkeletonBOT[4] = {1.0f, 1.0f, 1.0f, 1.0f}; } sColorsESPBOT;
    struct { float SkeletonPLAYER[4] = {1.0f, 1.0f, 1.0f, 1.0f}; } sColorsESPPLAYER;
    float BskelLine = 1.0f;
    float PskelLine = 1.0f;
};
static ConfigT Config;

#include "Hacks/XPremium/Skeleton.h"

int main()
{
    ImDrawList *draw = nullptr;

    // A null pawn and an invalid bone map must be refused without a fault.
    if (DrawBoneMapSkeleton(draw, nullptr, IM_COL32_WHITE, 1.0f, 1080)) return 1;
    if (SkeletonCooldownActive(0x1234)) return 2;

    SkeletonStartCooldown(0x1234);
    if (!SkeletonCooldownActive(0x1234)) return 3;

    // The guard is what turns a torn bone chain into a skipped frame.
    if (!fault_guard::Run([] {})) return 4;

    // Non-finite bone positions have to be rejected, not drawn.
    if (IsFiniteVec3(Vector3{NAN, 0.0f, 0.0f})) return 5;
    if (IsFiniteVec3(Vector3{1.0e9f, 0.0f, 0.0f})) return 6;
    if (!IsFiniteVec3(Vector3{1.0f, 2.0f, 3.0f})) return 7;

    std::printf("OK: skeleton_check - invalid pawns refused, cooldown works, non-finite bones rejected\n");
    return 0;
}
