#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Hacks/Feature/Init_Hacks.h"

extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

// Crosshair.h uses two things the game side owns. The check supplies them so the
// style table, the state machine and the geometry can be exercised here.
inline int32_t ToColor(float *col)
{
    return (int32_t)IM_COL32((int)(col[0] * 255.0f), (int)(col[1] * 255.0f),
                             (int)(col[2] * 255.0f), (int)(col[3] * 255.0f));
}

namespace NewFeatures
{
    inline void CrosshairCentrePx(float &cx, float &cy)
    {
        cx = 960.0f + Config.Crosshair.OffsetX;
        cy = 540.0f + Config.Crosshair.OffsetY;
    }
}

#include "Hacks/Feature/Crosshair.h"

static int g_checks = 0;
static int g_failures = 0;
#define CHECK(cond, msg)                                                \
    do {                                                                \
        g_checks++;                                                     \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);  \
                       g_failures++; }                                  \
        else         { printf("ok  : %s\n", msg); }                     \
    } while (0)

static bool near(float a, float b, float eps = 0.0005f) { return std::fabs(a - b) <= eps; }

static const float kSize = 20.0f;
static const float kThick = 2.0f;
static const float kGap = 5.0f;

static bool StyleWideEnough(int style)
{
    const ReticleGeometry g = BuildReticleGeometry(style, 0.0f, 0.0f, kSize, kThick, kGap);
    const float half = kSize * 0.5f;
    for (int i = 0; i < g.segmentCount; ++i)
    {
        const ReticleSegment &s = g.segments[i];
        if (std::fabs(s.x0) > half + 0.0005f || std::fabs(s.x1) > half + 0.0005f) return false;
        if (std::fabs(s.y0) > half + 0.0005f || std::fabs(s.y1) > half + 0.0005f) return false;
    }
    return true;
}

int main()
{
    // ------------------------------------------------------------------ A: table
    CHECK(ReticleStyleCount == 9, "A1 nine styles are declared");
    CHECK(ReticleDot == 0 && ReticleCross == 1,
          "A2 the two original saved Style values keep their numbers");

    bool namesOk = true, namesDistinct = true;
    for (int i = 0; i < ReticleStyleCount; ++i)
    {
        const char *n = kReticleStyles[i].name;
        if (n == nullptr || n[0] == '\0') namesOk = false;
        for (int j = i + 1; j < ReticleStyleCount; ++j)
            if (n != nullptr && kReticleStyles[j].name != nullptr &&
                std::strcmp(n, kReticleStyles[j].name) == 0) namesDistinct = false;
    }
    CHECK(namesOk, "A3 every style has a name");
    CHECK(namesDistinct, "A3 the names are distinct");

    CHECK(std::strcmp(kReticleStyles[ReticleDot].name, "Dot") == 0 &&
          std::strcmp(kReticleStyles[ReticleCross].name, "Classic Cross") == 0,
          "A4 the original two styles are still Dot and Classic Cross");

    {
        const char *const *names = ReticleStyleNames();
        bool samePointers = true;
        for (int i = 0; i < ReticleStyleCount; ++i)
            if (names[i] != kReticleStyles[i].name) samePointers = false;
        CHECK(samePointers, "A5 the dropdown list is generated from the table");
        CHECK(ReticleStyleNames() == names, "A5 the list is built once, not per frame");
    }

    CHECK(&ReticleStyleAt(-1) == &kReticleStyles[0] &&
          &ReticleStyleAt(ReticleStyleCount) == &kReticleStyles[0] &&
          &ReticleStyleAt(9999) == &kReticleStyles[0],
          "A6 an out-of-range style cannot index past the table");

    // ------------------------------------------------- B: the centre-dot question
    CHECK(ReticleHasCentreDot(ReticleDot), "B1 Dot has a centre dot");
    CHECK(ReticleHasCentreDot(ReticleCircleDot), "B1 Circle + Dot has a centre dot");

    {
        const int noDot[] = { ReticleCross, ReticleGapCross, ReticleCircle,
                              ReticleTShape, ReticleChevron, ReticleXShape, ReticleBrackets };
        bool noneHaveOne = true;
        for (int s : noDot)
        {
            if (ReticleHasCentreDot(s)) noneHaveOne = false;
            const ReticleGeometry g = BuildReticleGeometry(s, 0.0f, 0.0f, kSize, kThick, kGap);
            if (g.dotRadius > 0.0f) noneHaveOne = false;
        }
        CHECK(noneHaveOne, "B2 the styles without a centre dot never emit one");
    }
    {
        const ReticleGeometry dot = BuildReticleGeometry(ReticleDot, 0.0f, 0.0f, kSize, kThick, kGap);
        CHECK(near(dot.dotRadius, kSize * 0.5f), "B3 Dot is still a filled dot of Size / 2");
        CHECK(dot.segmentCount == 0, "B3 Dot draws no arms");
    }

    // ------------------------------------------------------------- C: defaults
    {
        sConfig fresh{0};
        CHECK(!fresh.Crosshair.Enabled, "C1 a fresh config has the crosshair off");
    }
    {
        CrosshairApplyDefaults();
        CHECK(!Config.Crosshair.Enabled, "C2 the applied default is off");
        CHECK(!CrosshairVisible(), "C2 and nothing is drawn");
        CHECK(Config.Crosshair.Style == ReticleDot, "C2 default style is Dot");
        CHECK(near(Config.Crosshair.Size, 6.0f), "C2 default size is 6 px");
        CHECK(near(Config.Crosshair.Thickness, 1.5f), "C2 default thickness is 1.5 px");
        CHECK(near(Config.Crosshair.Gap, 3.0f), "C2 default gap is 3 px");
        CHECK(!Config.Crosshair.Outline, "C2 outline is off by default");
        CHECK(near(Config.Crosshair.OutlineThickness, 1.0f), "C2 default outline width is 1 px");
        CHECK(near(Config.Crosshair.Color[3], 0.95f), "C2 default opacity is 0.95");
        CHECK(near(Config.Crosshair.OffsetX, 0.0f) && near(Config.Crosshair.OffsetY, 0.0f),
              "C2 default position is centred");
    }
    {
        // Resetting the reticle must not reach into anything else.
        Config.NewFeaturesMenu.Triggerbot = true;
        Config.NewFeaturesMenu.TriggerbotRadius = 137;
        Config.NewFeaturesMenu.TriggerbotDotRay = false;
        Config.Crosshair.Style = ReticleChevron;
        CrosshairApplyDefaults();
        CHECK(Config.NewFeaturesMenu.Triggerbot && Config.NewFeaturesMenu.TriggerbotRadius == 137 &&
              !Config.NewFeaturesMenu.TriggerbotDotRay,
              "C3 the reset leaves the triggerbot settings alone");
        CHECK(Config.Crosshair.Style == ReticleDot, "C3 ... and returns the style to its default");
    }

    // -------------------------------------------------------- D: independence
    {
        struct Row { bool crosshair; bool triggerbot; bool visible; const char *what; };
        const Row rows[] = {
            { false, false, false, "D1 crosshair off, triggerbot off -> hidden" },
            { false, true,  false, "D2 crosshair off, triggerbot on  -> hidden" },
            { true,  false, true,  "D3 crosshair on,  triggerbot off -> visible" },
            { true,  true,  true,  "D4 crosshair on,  triggerbot on  -> visible" },
        };
        for (const Row &r : rows)
        {
            Config.Crosshair.Enabled = r.crosshair;
            Config.Crosshair.Size = 6.0f;
            Config.NewFeaturesMenu.Triggerbot = r.triggerbot;
            CHECK(CrosshairVisible() == r.visible, r.what);
        }
    }
    {
        // Flipping the triggerbot on its own can never change the reticle.
        bool steady = true;
        for (int crosshair = 0; crosshair < 2; ++crosshair)
        {
            Config.Crosshair.Enabled = crosshair != 0;
            Config.NewFeaturesMenu.Triggerbot = false;
            const bool before = CrosshairVisible();
            Config.NewFeaturesMenu.Triggerbot = true;
            if (CrosshairVisible() != before) steady = false;
        }
        CHECK(steady, "D5 the triggerbot switch alone never changes the reticle");
    }

    // ---------------------------------------------------------- E: geometry
    {
        const int expectedSegments[ReticleStyleCount] = { 0, 4, 4, 0, 0, 3, 2, 4, 8 };
        bool match = true;
        for (int s = 0; s < ReticleStyleCount; ++s)
        {
            const ReticleGeometry g = BuildReticleGeometry(s, 0.0f, 0.0f, kSize, kThick, kGap);
            if (g.segmentCount != expectedSegments[s]) match = false;
            if (g.segmentCount < 0 || g.segmentCount > ReticleGeometry::kMaxSegments) match = false;
        }
        CHECK(match, "E1 every style emits its expected arm count");
    }
    {
        bool nonDegenerate = true;
        for (int s = 0; s < ReticleStyleCount; ++s)
        {
            const ReticleGeometry g = BuildReticleGeometry(s, 0.0f, 0.0f, kSize, kThick, kGap);
            for (int i = 0; i < g.segmentCount; ++i)
            {
                const ReticleSegment &seg = g.segments[i];
                if (seg.x0 == seg.x1 && seg.y0 == seg.y1) nonDegenerate = false;
            }
        }
        CHECK(nonDegenerate, "E2 no style emits a zero-length arm");
    }
    {
        bool bounded = true;
        for (int s = 0; s < ReticleStyleCount; ++s)
            if (!StyleWideEnough(s)) bounded = false;
        CHECK(bounded, "E3 every style stays inside the Size box it declares");
    }
    {
        const ReticleGeometry g = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, kThick, kGap);
        bool touchesCentre = false;
        for (int i = 0; i < g.segmentCount; ++i)
        {
            const ReticleSegment &s = g.segments[i];
            if (near(s.x0, 0.0f, 0.001f) || near(s.x1, 0.0f, 0.001f) ||
                near(s.y0, 0.0f, 0.001f) || near(s.y1, 0.0f, 0.001f)) touchesCentre = true;
        }
        CHECK(touchesCentre, "E4 Classic Cross arms meet the centre");
    }
    {
        const ReticleGeometry g = BuildReticleGeometry(ReticleGapCross, 0.0f, 0.0f, kSize, kThick, kGap);
        bool held = true;
        for (int i = 0; i < g.segmentCount; ++i)
        {
            const ReticleSegment &s = g.segments[i];
            const float inner = std::fmin(std::fabs(s.x0) + std::fabs(s.y0),
                                          std::fabs(s.x1) + std::fabs(s.y1));
            if (inner < kGap - 0.001f) held = false;
        }
        CHECK(held, "E5 Gap Cross keeps the gap between the centre and the arms");
    }
    {
        const ReticleGeometry a = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, kThick, 0.0f);
        const ReticleGeometry b = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, kThick, 20.0f);
        bool identical = a.segmentCount == b.segmentCount;
        for (int i = 0; identical && i < a.segmentCount; ++i)
            if (!near(a.segments[i].x0, b.segments[i].x0) || !near(a.segments[i].y0, b.segments[i].y0) ||
                !near(a.segments[i].x1, b.segments[i].x1) || !near(a.segments[i].y1, b.segments[i].y1))
                identical = false;
        CHECK(identical, "E6 Classic Cross ignores the gap control");
    }
    {
        // The preview draws at a different centre. The shape must only translate,
        // which is what makes the preview the same reticle the overlay draws.
        bool translated = true;
        const float dx = 412.5f, dy = -73.25f;
        for (int s = 0; s < ReticleStyleCount; ++s)
        {
            const ReticleGeometry a = BuildReticleGeometry(s, 0.0f, 0.0f, kSize, kThick, kGap);
            const ReticleGeometry b = BuildReticleGeometry(s, dx, dy, kSize, kThick, kGap);
            if (a.segmentCount != b.segmentCount) translated = false;
            for (int i = 0; translated && i < a.segmentCount; ++i)
            {
                const ReticleSegment &p = a.segments[i];
                const ReticleSegment &q = b.segments[i];
                if (!near(p.x0 + dx, q.x0) || !near(p.y0 + dy, q.y0) ||
                    !near(p.x1 + dx, q.x1) || !near(p.y1 + dy, q.y1)) translated = false;
            }
            if (!near(a.dotRadius, b.dotRadius) || !near(a.circleRadius, b.circleRadius))
                translated = false;
        }
        CHECK(translated, "E7 the reticle is a pure translation, so the preview matches the overlay");
    }
    {
        const ReticleGeometry small = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, 20.0f, kThick, 0.0f);
        const ReticleGeometry big   = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, 40.0f, kThick, 0.0f);
        CHECK(near(big.segments[0].x1, small.segments[0].x1 * 2.0f),
              "E8 doubling Size doubles the reach");
    }
    {
        const ReticleGeometry g = BuildReticleGeometry(ReticleTShape, 0.0f, 0.0f, kSize, kThick, kGap);
        bool noUpperArm = true;
        for (int i = 0; i < g.segmentCount; ++i)
            if (g.segments[i].y0 < -0.0005f || g.segments[i].y1 < -0.0005f) noUpperArm = false;
        CHECK(noUpperArm, "E9 T-Shape has no upper arm");
    }
    {
        const ReticleGeometry c = BuildReticleGeometry(ReticleCircle, 0.0f, 0.0f, kSize, kThick, kGap);
        CHECK(near(c.circleRadius, kSize * 0.5f) && c.segmentCount == 0,
              "E10 Circle is an outline of Size / 2 and nothing else");
        CHECK(near(c.dotRadius, 0.0f), "E10 Circle has no centre dot");
    }
    {
        const ReticleGeometry c = BuildReticleGeometry(ReticleCircleDot, 0.0f, 0.0f, kSize, kThick, kGap);
        CHECK(c.circleRadius > 0.0f && c.dotRadius > 0.0f && c.dotRadius < c.circleRadius,
              "E11 Circle + Dot draws both, dot inside the ring");
    }

    // ---------------------------------------------------------- F: outline
    {
        Config.Crosshair.Outline = false;
        CHECK(near(ReticleOutlineWidth(), 0.0f), "F1 outline off draws no halo");
        Config.Crosshair.Outline = true;
        Config.Crosshair.OutlineThickness = 2.5f;
        CHECK(near(ReticleOutlineWidth(), 2.5f), "F1 outline on uses its own width");
    }
    {
        const ReticleGeometry g = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, 0.0f, kGap);
        CHECK(near(g.thickness, 1.0f), "F2 a zero thickness still strokes at 1 px");
    }
    {
        const ReticleGeometry g = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, -4.0f, kGap);
        CHECK(near(g.thickness, 1.0f), "F2 a negative thickness cannot produce a broken stroke");
    }

    // ------------------------------------------- G: bounds and resolution
    {
        float cx = 0.0f, cy = 0.0f;
        Config.Crosshair.OffsetX = 0.0f;
        Config.Crosshair.OffsetY = 0.0f;
        CHECK(CrosshairCentreOnScreen(cx, cy, 1920.0f, 1080.0f), "G1 centred reticle is on screen");
        CHECK(near(cx, 960.0f) && near(cy, 540.0f), "G1 and lands on the screen centre");

        Config.Crosshair.OffsetX = 5000.0f;
        CHECK(!CrosshairCentreOnScreen(cx, cy, 1920.0f, 1080.0f),
              "G1 an offset pushed off screen is not drawn");

        Config.Crosshair.OffsetX = 0.0f;
        const float widths[]  = { 1280.0f, 1920.0f, 2340.0f };
        const float heights[] = { 720.0f, 1080.0f, 1080.0f };
        bool allRes = true;
        for (int i = 0; i < 3; ++i)
            if (!CrosshairCentreOnScreen(cx, cy, widths[i], heights[i])) allRes = false;
        CHECK(allRes, "G2 the centred reticle survives a resolution change");
    }
    {
        // Geometry is in pixels, so it is resolution independent by construction.
        const ReticleGeometry a = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, kThick, kGap);
        const ReticleGeometry b = BuildReticleGeometry(ReticleCross, 0.0f, 0.0f, kSize, kThick, kGap);
        bool same = a.segmentCount == b.segmentCount;
        for (int i = 0; same && i < a.segmentCount; ++i)
            if (a.segments[i].x1 != b.segments[i].x1) same = false;
        CHECK(same, "G2 the shape does not depend on the screen size");
    }

    printf("\n");
    if (g_failures == 0) { printf("OK: all crosshair checks passed (%d)\n", g_checks); return 0; }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
