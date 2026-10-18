// Host test for the trigger's model-based body bound.
//
// The "it still shoots even though I am not on his body" report was answered by
// replacing ONE half-width for the whole silhouette (kBodyHalfWidthM = 0.32 m,
// the shoulder width applied from boots to head) with the model's own profile -
// A_FireBodyHalfWidthM(t), where t is the dot's height fraction on the
// feet->head axis. Path A (A_FireScan) now reads that profile at the dot's own
// height, measures perpendicular to the axis AT that height, and refuses a dot
// above his head or below his feet instead of folding it onto the nearest end of
// the segment.
//
// What is pinned here:
//   * the profile's shape: narrow at the boots and the head, widest at the
//     shoulders, and never wider than the old single value anywhere except the
//     shoulder band;
//   * interpolation between the rows, and clamping outside 0..1;
//   * the two constants the scan uses (span margin in body fractions, tolerance
//     in pixels) are the tight ones, not the old 2 px / whole-body slack.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests and Tools::IsPtrValid is stubbed because Tools.cpp is
// not linked):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes \
//       src/main/jni/Hacks/Feature/tests/body_profile_check.cpp \
//       -o /tmp/body_profile_check && /tmp/body_profile_check
// Exits 0 and prints "OK: all body-profile checks passed" on success.
// Android.mk does not wildcard Hacks/Feature/tests, so it never reaches the APK.
#include <cstdint>
#include <cstdio>

#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
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

static int g_failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);    \
                       g_failures++; }                                    \
        else        { printf("ok  : %s\n", msg); }                        \
    } while (0)

using NewFeatures::A_FireBodyHalfWidthM;
using NewFeatures::A_FireFitToBody;
using NewFeatures::AF_BodyFit;
using NewFeatures::AF_BodyProbe;

static bool near(float a, float b, float eps = 0.005f)
{
    return (a > b ? a - b : b - a) <= eps;
}

// Host-only link stub: the include graph references the A64 hook engine.
extern "C" void A64HookFunction(void *const symbol, void *const replace, void **result)
{ (void)symbol; if (result) *result = replace; }

int main()
{
    printf("== trigger: model-based body bound ==\n");

    // ---- P1: the ends of the body -----------------------------------------
    CHECK(near(A_FireBodyHalfWidthM(0.0f), 0.09f),
          "P1 the boots are 0.09 m half wide (was 0.32 for the whole body)");
    CHECK(near(A_FireBodyHalfWidthM(1.0f), 0.13f),
          "P1 the head is 0.13 m half wide");

    // ---- P2: the widest point is the shoulders ----------------------------
    float widest = 0.0f, widestT = -1.0f;
    for (int i = 0; i <= 100; i++) {
        const float t = i / 100.0f;
        const float w = A_FireBodyHalfWidthM(t);
        if (w > widest) { widest = w; widestT = t; }
    }
    CHECK(near(widest, 0.27f) && widestT > 0.80f && widestT < 0.92f,
          "P2 the silhouette is widest at the shoulders (0.27 m), not everywhere");

    // ---- P3: everything except the shoulder band is tighter than the old
    //          single 0.32 m value --------------------------------------------
    int tighterThanOld = 0;
    for (int i = 0; i <= 100; i++) {
        if (A_FireBodyHalfWidthM(i / 100.0f) <= 0.27f) tighterThanOld++;
    }
    CHECK(tighterThanOld == 101,
          "P3 no height of the body is allowed the old 0.32 m cylinder width");

    // ---- P4: the parts that read as a body ---------------------------------
    CHECK(A_FireBodyHalfWidthM(0.12f) < A_FireBodyHalfWidthM(0.28f),
          "P4 the shins are narrower than the knees");
    CHECK(A_FireBodyHalfWidthM(0.66f) < A_FireBodyHalfWidthM(0.86f),
          "P4 the waist is narrower than the shoulders");
    CHECK(A_FireBodyHalfWidthM(0.93f) < A_FireBodyHalfWidthM(0.78f),
          "P4 the neck is narrower than the chest (a dot beside the head is refused)");
    CHECK(A_FireBodyHalfWidthM(0.05f) < A_FireBodyHalfWidthM(0.55f),
          "P4 the boots are narrower than the hips");

    // ---- P5: interpolation and clamping -----------------------------------
    {
        const float a = A_FireBodyHalfWidthM(0.80f);   // between chest 0.78 and shoulders 0.86
        CHECK(a > 0.24f && a < 0.27f,
              "P5 a point between two rows is interpolated, not snapped");
        CHECK(near(A_FireBodyHalfWidthM(-2.0f), A_FireBodyHalfWidthM(0.0f)),
              "P5 below the feet clamps to the boots");
        CHECK(near(A_FireBodyHalfWidthM(3.0f), A_FireBodyHalfWidthM(1.0f)),
              "P5 above the head clamps to the head");
    }

    // ---- P6: the constants the scan applies -------------------------------
    CHECK(NewFeatures::A_FireTune::kModelSpanMargin > 0.0f &&
          NewFeatures::A_FireTune::kModelSpanMargin <= 0.05f,
          "P6 the height-span slack is a few percent of the body, not the whole body");
    CHECK(NewFeatures::A_FireTune::kModelTolerancePx <= 1.5f,
          "P6 the pixel slack on the projected profile is 1 px (was 2 px on a 0.32 m width)");

    // ---- P7: the profile agrees with the old constant at the shoulders only
    CHECK(near(A_FireBodyHalfWidthM(0.86f), 0.27f) &&
          NewFeatures::A_FireTune::kBodyHalfWidthM > A_FireBodyHalfWidthM(0.86f),
          "P7 the shoulder band is the only place the old width was close");

    // ---- P8: A_FireFitToBody - the silhouette test both scan paths call ----
    // A standing body at 3.3 m, projected the way the device camera projects
    // (same focal/height the scan derives): head 0.1 m above the camera, feet
    // 1.6 m below it. Every verdict below is a screen-space distance the player
    // can see, not a mock.
    {
        const float f   = (900.0f * 0.5f) /
                          tanf(NewFeatures::A_FireTune::kFallbackFov * 0.5f *
                               NewFeatures::A_FireTune::kDegToRad);
        const float z   = 3.3f;
        const float ppm = f / z;
        auto proj = [&](float wx, float wy) {
            return Vector3{800.0f + (wx / z) * f, 450.0f - ((wy - 1.6f) / z) * f, z};
        };
        const Vector3 headW(0.0f, 1.7f, z), feetW(0.0f, 0.0f, z);
        const Vector3 scHead = proj(0.0f, 1.7f), scFeet = proj(0.0f, 0.0f);

        // chest height (t ~ 0.72) -> on him
        const float chestY = scFeet.y + (scHead.y - scFeet.y) * 0.72f;
        {
            const AF_BodyProbe p = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                  scFeet.x, chestY);
            CHECK(p.fit == NewFeatures::AF_BodyInside, "P8 the chest is ON the model");
            CHECK(near(p.t, 0.72f, 0.02f), "P8 ... at the measured height fraction");
            CHECK(near(p.limitPx, A_FireBodyHalfWidthM(0.72f) * ppm +
                                  NewFeatures::A_FireTune::kModelTolerancePx, 0.5f),
                  "P8 ... bounded by the profile at that height, in px");
        }

        // 35 px above his head -> outside, t above 1
        {
            const AF_BodyProbe p = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                  scFeet.x, scHead.y - 35.0f);
            CHECK(p.fit == NewFeatures::AF_BodyOutside,
                  "P8 35 px above his head is NOT his body");
            CHECK(p.t > 1.0f, "P8 ... reported as above the head, not folded onto it");
        }

        // beside his head at head height -> outside (head is 0.13 m half wide,
        // the shoulder width the old rule used would have accepted ~40 px)
        {
            const float off = 35.0f;
            const AF_BodyProbe p = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                  scFeet.x + off, scHead.y);
            CHECK(p.fit == NewFeatures::AF_BodyOutside,
                  "P8 35 px beside his head is refused (the head is narrower than the shoulders)");
            CHECK(near(p.latPx, off, 0.5f), "P8 ... and the lateral distance is the 35 px");
        }

        // at the shoulders 35 px is still inside (the profile widens there)
        {
            const float shoulderY = scFeet.y + (scHead.y - scFeet.y) * 0.86f;
            const AF_BodyProbe p = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                  scFeet.x + 35.0f, shoulderY);
            CHECK(p.fit == NewFeatures::AF_BodyInside,
                  "P8 the same 35 px IS on him at the shoulders");
        }

        // distance independence: the same world offset is a smaller pixel
        // offset far away, and the verdict must follow the body, not the screen
        {
            const float far = 60.0f;
            const Vector3 hW(0.0f, 1.7f, far), fW(0.0f, 0.0f, far);
            auto projFar = [&](float wy) {
                return Vector3{800.0f, 450.0f - ((wy - 1.6f) / far) * f, far};
            };
            const Vector3 sh = projFar(1.7f), sf = projFar(0.0f);
            const float ppmFar = f / far;
            const float shY = sf.y + (sh.y - sf.y) * 0.86f;
            // 0.20 m to the side of the shoulder axis: inside at any range
            const AF_BodyProbe near20 = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                        scFeet.x + 0.20f * ppm,
                                                        scFeet.y + (scHead.y - scFeet.y) * 0.86f);
            const AF_BodyProbe far20  = A_FireFitToBody(hW, fW, sh, sf,
                                                        sf.x + 0.20f * ppmFar, shY);
            CHECK(near20.fit == far20.fit,
                  "P8 the same 0.20 m beside the shoulders gives the same verdict at 3 m and 60 m");
            // 0.45 m beside: outside at both
            const AF_BodyProbe near45 = A_FireFitToBody(headW, feetW, scHead, scFeet,
                                                        scFeet.x + 0.45f * ppm,
                                                        scFeet.y + (scHead.y - scFeet.y) * 0.86f);
            const AF_BodyProbe far45  = A_FireFitToBody(hW, fW, sh, sf,
                                                        sf.x + 0.45f * ppmFar, shY);
            CHECK(near45.fit == NewFeatures::AF_BodyOutside &&
                  far45.fit  == NewFeatures::AF_BodyOutside,
                  "P8 0.45 m beside him is refused at any distance");
        }

        // a body that did not project at all is UNMEASURABLE, never "outside":
        // the caller keeps its own fallback instead of losing the target
        {
            const AF_BodyProbe p = A_FireFitToBody(headW, feetW, scHead, scHead,
                                                  scHead.x, scHead.y);
            CHECK(p.fit == NewFeatures::AF_BodyUnmeasurable,
                  "P8 a 0-px body is unmeasurable (the caller decides), not refused");
        }
    }

    if (g_failures == 0)
        printf("OK: all body-profile checks passed\n");
    else
        printf("%d check(s) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
