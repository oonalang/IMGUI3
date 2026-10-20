#pragma once

// ---------------------------------------------------------------------------
// ESP tab character preview.
//
// The old preview was a flat 2D vector drawing. This one is a real (if tiny)
// 3D character: the mesh is built once in model space, then every frame it is
// rotated by the drag input, perspective-projected and painted as shaded
// quads - back faces culled, faces depth-sorted back to front (painter's
// algorithm). No GL state, no textures, no engine access: everything goes
// through ImDrawList, so it draws anywhere the menu draws.
// ---------------------------------------------------------------------------

#include "Call_ImGui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <vector>

namespace esp_model_preview
{

struct Vec3
{
    float x, y, z;
};

inline Vec3 mk(float x, float y, float z) { return Vec3{x, y, z}; }
inline Vec3 operator+(const Vec3 &a, const Vec3 &b) { return Vec3{a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3 &a, const Vec3 &b) { return Vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(const Vec3 &a, float s) { return Vec3{a.x * s, a.y * s, a.z * s}; }
inline float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(const Vec3 &a, const Vec3 &b)
{
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vec3 Normalize(const Vec3 &v)
{
    const float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) return Vec3{0.0f, 0.0f, 0.0f};
    return Vec3{v.x / len, v.y / len, v.z / len};
}

// One flat-shaded polygon of the character. "inner" orientation is resolved at
// build time, so the winding of the builder loops never has to be perfect.
struct Face
{
    Vec3 p[4];
    Vec3 n;
    ImU32 col;
    int count;       // 3 or 4
    bool twoSided;   // never culled: the cape is a single-surface shell
};

inline std::vector<Face> &Mesh()
{
    static std::vector<Face> mesh;
    return mesh;
}

inline void PushPoly(Vec3 a, Vec3 b, Vec3 c, Vec3 d, int count, ImU32 col, Vec3 inner,
                     bool twoSided = false)
{
    Vec3 n = Normalize(Cross(b - a, c - a));
    const Vec3 center = (a + b + c + d) * 0.25f;
    if (Dot(n, center - inner) < 0.0f)
    {
        // Flip the winding so the normal always points away from the solid.
        const Vec3 tmp = b;
        b = d;
        d = tmp;
        n = n * -1.0f;
    }
    Face f;
    f.p[0] = a; f.p[1] = b; f.p[2] = c; f.p[3] = d;
    f.n = n;
    f.col = col;
    f.count = count;
    f.twoSided = twoSided;
    Mesh().push_back(f);
}

inline void PushQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, ImU32 col, Vec3 inner)
{
    PushPoly(a, b, c, d, 4, col, inner);
}

inline void PushTri(Vec3 a, Vec3 b, Vec3 c, ImU32 col, Vec3 inner)
{
    PushPoly(a, b, c, c, 3, col, inner);
}

inline void AddEllipsoid(Vec3 c, float rx, float ry, float rz, int segs, int rings,
                         ImU32 col, float cutY = -1.0e9f, float cutZ = -1.0e9f)
{
    for (int i = 0; i < rings; ++i)
    {
        const float lat0 = -IM_PI * 0.5f + IM_PI * ((float)i / (float)rings);
        const float lat1 = -IM_PI * 0.5f + IM_PI * ((float)(i + 1) / (float)rings);
        for (int j = 0; j < segs; ++j)
        {
            const float lon0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float lon1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);

            auto P = [&](float lat, float lon) {
                return mk(c.x + rx * cosf(lat) * cosf(lon),
                          c.y + ry * sinf(lat),
                          c.z + rz * cosf(lat) * sinf(lon));
            };
            const Vec3 a = P(lat0, lon0);
            const Vec3 b = P(lat0, lon1);
            const Vec3 d = P(lat1, lon1);
            const Vec3 e = P(lat1, lon0);
            const Vec3 cen = (a + b + d + e) * 0.25f;
            if (cen.y < cutY && cen.z > cutZ) continue; // e.g. leave the face open
            PushQuad(a, b, d, e, col, c);
        }
    }
}

// Tapered tube between two points (limbs, hair tails, straps).
inline void AddTube(Vec3 p0, Vec3 p1, float r0, float r1, int segs, ImU32 col, bool capEnds = false)
{
    const Vec3 axis = p1 - p0;
    const Vec3 dir = Normalize(axis);
    if (Dot(dir, dir) < 0.5f) return;
    const Vec3 helper = fabsf(dir.y) > 0.95f ? mk(1.0f, 0.0f, 0.0f) : mk(0.0f, 1.0f, 0.0f);
    const Vec3 ex = Normalize(Cross(dir, helper));
    const Vec3 ez = Normalize(Cross(ex, dir));
    const Vec3 inner = (p0 + p1) * 0.5f;

    for (int j = 0; j < segs; ++j)
    {
        const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
        const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
        const Vec3 d0 = ex * cosf(a0) + ez * sinf(a0);
        const Vec3 d1 = ex * cosf(a1) + ez * sinf(a1);
        const Vec3 a = p0 + d0 * r0;
        const Vec3 b = p0 + d1 * r0;
        const Vec3 c = p1 + d1 * r1;
        const Vec3 e = p1 + d0 * r1;
        PushQuad(a, b, c, e, col, inner);
    }

    if (capEnds)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            const Vec3 d0 = ex * cosf(a0) + ez * sinf(a0);
            const Vec3 d1 = ex * cosf(a1) + ez * sinf(a1);
            PushTri(p0, p0 + d0 * r0, p0 + d1 * r0, col, inner);
            PushTri(p1, p1 + d0 * r1, p1 + d1 * r1, col, inner);
        }
    }
}

inline void AddBox(Vec3 c, float hx, float hy, float hz, ImU32 col)
{
    Vec3 v[8];
    for (int i = 0; i < 8; ++i)
        v[i] = mk(c.x + (i & 1 ? hx : -hx), c.y + (i & 2 ? hy : -hy), c.z + (i & 4 ? hz : -hz));
    static const int quads[6][4] = {
        {0, 2, 6, 4}, {1, 3, 7, 5},
        {0, 4, 5, 1}, {2, 3, 7, 6},
        {0, 1, 3, 2}, {4, 5, 7, 6},
    };
    for (int i = 0; i < 6; ++i)
        PushQuad(v[quads[i][0]], v[quads[i][1]], v[quads[i][2]], v[quads[i][3]], col, c);
}

// Stacked elliptical rings - torso, skirt, hair sheets.
struct Ring
{
    float y, rx, rz, cz;
};

inline void AddLoft(const Ring *rings, int count, int segs, ImU32 col, bool caps = true)
{
    if (count < 2) return;
    const Vec3 inner = mk(0.0f, (rings[0].y + rings[count - 1].y) * 0.5f, 0.0f);
    auto P = [](const Ring &r, float a) {
        return mk(r.rx * cosf(a), r.y, r.cz + r.rz * sinf(a));
    };

    for (int i = 0; i + 1 < count; ++i)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            PushQuad(P(rings[i], a0), P(rings[i], a1), P(rings[i + 1], a1), P(rings[i + 1], a0), col, inner);
        }
    }

    if (caps)
    {
        const Ring &top = rings[0];
        const Ring &bottom = rings[count - 1];
        const Vec3 topC = mk(0.0f, top.y, top.cz);
        const Vec3 botC = mk(0.0f, bottom.y, bottom.cz);
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            PushTri(topC, P(top, a0), P(top, a1), col, inner);
            PushTri(botC, P(bottom, a0), P(bottom, a1), col, inner);
        }
    }
}

// Open lofted sheet spanning a slice of the ring - used for the cape, which is
// a single surface with no thickness, so its faces are never back-face culled.
// "fold" ripples the radius across the sheet so Lambert shading breaks the
// cloth into light/dark folds instead of one flat panel.
inline void AddShell(const Ring *rings, int count, int segs, float a0, float a1, ImU32 col,
                     float fold = 0.0f)
{
    if (count < 2) return;
    auto P = [&](const Ring &r, float a, float ripple) {
        return mk(r.rx * cosf(a) * ripple, r.y, r.cz + r.rz * sinf(a) * ripple);
    };
    for (int i = 0; i + 1 < count; ++i)
    {
        const float ringT = (float)i / (float)(count - 1);
        for (int j = 0; j < segs; ++j)
        {
            const float u0 = (float)j / (float)segs;
            const float u1 = (float)(j + 1) / (float)segs;
            const float t0 = a0 + (a1 - a0) * u0;
            const float t1 = a0 + (a1 - a0) * u1;
            const float r0 = 1.0f + fold * (0.30f + 0.70f * ringT) * sinf(u0 * IM_PI * 7.0f);
            const float r1 = 1.0f + fold * (0.30f + 0.70f * ringT) * sinf(u1 * IM_PI * 7.0f);
            const Vec3 a = P(rings[i], t0, r0);
            const Vec3 b = P(rings[i], t1, r1);
            const Vec3 c = P(rings[i + 1], t1, r1);
            const Vec3 d = P(rings[i + 1], t0, r0);
            PushPoly(a, b, c, d, 4, col, mk(0.0f, 0.0f, 0.0f), true);
        }
    }
}

// ---------------------------------------------------------------------------
// Palette - silver-haired warrior: white cape, gold-trimmed armour, tan boots
// ---------------------------------------------------------------------------

inline ImU32 Skin()      { return IM_COL32(250, 229, 214, 255); }
inline ImU32 Hair()      { return IM_COL32(230, 233, 242, 255); }
inline ImU32 HairDeep()  { return IM_COL32(188, 194, 214, 255); }
inline ImU32 Ribbon()    { return IM_COL32(92, 138, 224, 255); }
inline ImU32 Cape()      { return IM_COL32(236, 240, 249, 255); }
inline ImU32 Armor()     { return IM_COL32(238, 241, 248, 255); }
inline ImU32 Gold()      { return IM_COL32(203, 163, 92, 255); }
inline ImU32 GoldDark()  { return IM_COL32(157, 120, 62, 255); }
inline ImU32 Cloth()     { return IM_COL32(40, 38, 48, 255); }
inline ImU32 Boot()      { return IM_COL32(190, 158, 110, 255); }

// Head geometry is shared by the mesh and by the face overlay.
inline const Vec3 &HeadCenter()
{
    static const Vec3 c = mk(0.0f, 0.86f, 0.0f);
    return c;
}
inline const float HeadRadius = 0.175f;

inline void BuildModel()
{
    Mesh().clear();

    // ---- legs: bare thighs above tan leather knee boots -------------------
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = 0.086f * (float)s;
        AddTube(mk(x, 0.10f, 0.0f), mk(x * 1.05f, -0.26f, 0.010f), 0.090f, 0.062f, 12, Skin());
        // gold rim at the boot cuff, then the tan shaft down to the ankle
        AddTube(mk(x * 1.05f, -0.245f, 0.010f), mk(x * 1.05f, -0.290f, 0.010f), 0.075f, 0.073f, 12, Gold());
        AddTube(mk(x * 1.05f, -0.280f, 0.010f), mk(x * 1.02f, -0.850f, 0.004f), 0.074f, 0.055f, 12, Boot());
        AddBox(mk(x * 1.00f, -0.925f, 0.050f), 0.058f, 0.050f, 0.100f, Boot());
    }

    // ---- shorts, belt, hip plates ----------------------------------------
    const Ring shorts[] = {
        {0.185f, 0.166f, 0.126f, 0.0f},
        {0.090f, 0.180f, 0.134f, 0.0f},
        {0.000f, 0.176f, 0.130f, 0.0f},
    };
    AddLoft(shorts, 3, 20, Cloth());
    const Ring belt[] = {
        {0.196f, 0.170f, 0.130f, 0.0f},
        {0.166f, 0.172f, 0.131f, 0.0f},
    };
    AddLoft(belt, 2, 20, Gold(), false);
    AddBox(mk(0.0f, 0.120f, 0.118f), 0.062f, 0.048f, 0.026f, GoldDark());

    // ---- bare midriff -----------------------------------------------------
    const Ring waistRings[] = {
        {0.350f, 0.128f, 0.096f, 0.0f},
        {0.270f, 0.114f, 0.086f, 0.0f},
        {0.200f, 0.134f, 0.100f, 0.0f},
        {0.160f, 0.154f, 0.116f, 0.0f},
    };
    AddLoft(waistRings, 4, 20, Skin());

    // ---- chest / shoulders ------------------------------------------------
    const Ring chestRings[] = {
        {0.585f, 0.170f, 0.104f, 0.0f},
        {0.500f, 0.156f, 0.100f, 0.0f},
        {0.420f, 0.146f, 0.100f, 0.0f},
        {0.350f, 0.130f, 0.096f, 0.0f},
    };
    AddLoft(chestRings, 4, 20, Skin());

    // Armoured bra: white cups with a gold under-rim and shoulder straps.
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = (float)s;
        AddEllipsoid(mk(0.072f * x, 0.430f, 0.056f), 0.086f, 0.074f, 0.070f, 14, 10, Armor());
        AddTube(mk(0.078f * x, 0.476f, 0.048f), mk(0.150f * x, 0.572f, 0.008f), 0.016f, 0.014f, 8, Armor());
    }
    const Ring braRim[] = {
        {0.362f, 0.142f, 0.100f, 0.0f},
        {0.338f, 0.140f, 0.098f, 0.0f},
    };
    AddLoft(braRim, 2, 20, Gold(), false);
    AddEllipsoid(mk(0.0f, 0.418f, 0.112f), 0.030f, 0.030f, 0.020f, 10, 6, Gold());

    // ---- neck + choker ----------------------------------------------------
    AddTube(mk(0.0f, 0.550f, 0.0f), mk(0.0f, 0.710f, 0.0f), 0.050f, 0.046f, 12, Skin());
    const Ring choker[] = {
        {0.640f, 0.057f, 0.053f, 0.0f},
        {0.608f, 0.059f, 0.055f, 0.0f},
    };
    AddLoft(choker, 2, 14, Cloth(), false);
    const Ring chokerTrim[] = {
        {0.606f, 0.060f, 0.056f, 0.0f},
        {0.596f, 0.059f, 0.055f, 0.0f},
    };
    AddLoft(chokerTrim, 2, 14, Gold(), false);

    // ---- head -------------------------------------------------------------
    AddEllipsoid(HeadCenter(), HeadRadius, HeadRadius * 1.045f, HeadRadius * 0.96f, 20, 14, Skin());

    // ---- hair: silver bob, straight fringe, braid down the back ----------
    AddEllipsoid(mk(0.0f, 0.878f, -0.012f), 0.192f, 0.198f, 0.186f, 20, 14, Hair(), 0.882f, 0.004f);

    const Ring backHair[] = {
        {0.900f, 0.168f, 0.086f, -0.048f},
        {0.780f, 0.190f, 0.100f, -0.072f},
        {0.660f, 0.176f, 0.092f, -0.084f},
        {0.588f, 0.130f, 0.062f, -0.088f},
    };
    AddLoft(backHair, 4, 16, HairDeep());

    // straight fringe above the eyes
    for (int i = -2; i <= 2; ++i)
    {
        const float bx = 0.060f * (float)i;
        const float len = 0.088f - fabsf((float)i) * 0.007f;
        AddTube(mk(bx, 0.958f, 0.050f), mk(bx * 1.20f, 0.958f - len, 0.148f),
                0.044f, 0.016f, 8, Hair());
    }
    // side locks framing the jaw
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = (float)s;
        AddTube(mk(0.166f * x, 0.900f, 0.010f), mk(0.188f * x, 0.700f, 0.020f),
                0.058f, 0.040f, 10, Hair());
        AddTube(mk(0.188f * x, 0.700f, 0.020f), mk(0.196f * x, 0.640f, 0.018f),
                0.040f, 0.020f, 10, HairDeep());
    }
    // braid: alternating beads down the back, blue ribbon at the tip
    for (int i = 0; i < 9; ++i)
    {
        const float t = (float)i;
        const float y = 0.790f - t * 0.070f;
        const float bx = 0.020f * sinf(t * 1.55f);
        const float bz = -0.180f - t * 0.009f;
        const ImU32 col = (i % 2 == 0) ? Hair() : HairDeep();
        AddEllipsoid(mk(bx, y, bz), 0.038f, 0.043f, 0.034f, 10, 8, col);
        if (i == 8)
        {
            AddEllipsoid(mk(bx, y - 0.045f, bz), 0.030f, 0.026f, 0.026f, 10, 6, Ribbon());
            AddTube(mk(bx, y - 0.020f, bz), mk(bx + 0.055f, y - 0.105f, bz - 0.020f), 0.016f, 0.008f, 6, Ribbon());
            AddTube(mk(bx, y - 0.020f, bz), mk(bx - 0.050f, y - 0.120f, bz - 0.010f), 0.016f, 0.008f, 6, Ribbon());
        }
    }

    // ---- arms: bare upper arms, black forearm wraps, gold bands ----------
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = (float)s;
        const Vec3 shoulder = mk(0.172f * x, 0.548f, 0.0f);
        const Vec3 elbow = mk(0.232f * x, 0.300f, 0.012f);
        const Vec3 wrist = mk(0.244f * x, 0.045f, 0.028f);
        AddEllipsoid(shoulder, 0.058f, 0.058f, 0.058f, 12, 8, Skin());
        AddTube(shoulder, elbow, 0.052f, 0.042f, 10, Skin());
        AddTube(elbow, wrist, 0.043f, 0.033f, 10, Cloth());
        AddEllipsoid(wrist, 0.042f, 0.050f, 0.038f, 12, 8, Skin());
        AddTube(mk(0.188f * x, 0.545f, 0.0f), mk(0.198f * x, 0.500f, 0.004f), 0.056f, 0.052f, 10, Gold());
        AddEllipsoid(mk(0.198f * x, 0.568f, -0.004f), 0.080f, 0.048f, 0.074f, 14, 10, Gold());
    }

    // ---- cape: white two-sided shell around the back ---------------------
    const Ring capeRings[] = {
        {0.620f, 0.196f, 0.150f, -0.010f},
        {0.460f, 0.250f, 0.220f, -0.035f},
        {0.220f, 0.330f, 0.300f, -0.060f},
        {-0.030f, 0.420f, 0.370f, -0.085f},
        {-0.170f, 0.462f, 0.405f, -0.100f},
    };
    AddShell(capeRings, 5, 26, IM_PI * 1.04f, IM_PI * 1.96f, Cape(), 0.045f);
    for (int s = -1; s <= 1; s += 2)
    {
        AddEllipsoid(mk(0.156f * (float)s, 0.580f, 0.010f), 0.042f, 0.034f, 0.034f, 10, 6, Gold());
    }
}

// ---------------------------------------------------------------------------
// Interaction state
// ---------------------------------------------------------------------------

inline float g_yaw = 3.56f;
inline float g_pitch = 0.04f;
inline float g_yawVel = 0.0f;
inline float g_clock = 0.0f;
inline bool g_dragging = false;

inline void DragDelta(float dx, float dy)
{
    g_yaw -= dx * 0.014f;
    g_pitch = ImClamp(g_pitch + dy * 0.006f, -0.32f, 0.42f);
    g_yawVel = ImClamp(dx * 0.014f * 6.0f, -6.0f, 6.0f);
}

inline void SetDragging(bool dragging) { g_dragging = dragging; }

inline void ResetView()
{
    g_yaw = 3.56f;
    g_pitch = 0.04f;
    g_yawVel = 0.0f;
}

struct Result
{
    bool valid = false;
    ImVec2 min = ImVec2(0.0f, 0.0f);
    ImVec2 max = ImVec2(0.0f, 0.0f);
    ImVec2 center = ImVec2(0.0f, 0.0f);
    ImVec2 head = ImVec2(0.0f, 0.0f);
    float headRadius = 0.0f;
    float facing = 0.0f;
    // 0 head, 1 neck, 2 shoulderL, 3 shoulderR, 4 elbowL, 5 elbowR, 6 wristL,
    // 7 wristR, 8 chest, 9 waist, 10 hipL, 11 hipR, 12 kneeL, 13 kneeR,
    // 14 ankleL, 15 ankleR
    ImVec2 joint[16];
};

// Paints the character inside [pos, pos + size]. The caller owns the card
// background; the model is drawn on top of it.
inline Result DrawModel(ImDrawList *draw, const ImVec2 &pos, const ImVec2 &size)
{
    Result out;
    if (draw == nullptr || size.x < 8.0f || size.y < 8.0f) return out;
    if (Mesh().empty()) BuildModel();

    const float dt = ImMin(ImGui::GetIO().DeltaTime, 0.05f);
    g_clock += dt;
    if (!g_dragging)
    {
        g_yaw += (0.32f + g_yawVel) * dt;
        g_yawVel *= powf(0.08f, dt);
        g_pitch += (0.04f - g_pitch) * dt * 1.6f;
    }

    const float cyaw = cosf(g_yaw), syaw = sinf(g_yaw);
    const float cpit = cosf(g_pitch), spit = sinf(g_pitch);
    auto Rotate = [&](const Vec3 &p) {
        const float x = p.x * cyaw + p.z * syaw;
        const float z = -p.x * syaw + p.z * cyaw;
        const float y = p.y * cpit - z * spit;
        const float z2 = p.y * spit + z * cpit;
        return mk(x, y, z2);
    };

    const float bob = sinf(g_clock * 1.7f) * 0.012f;
    const float cx = pos.x + size.x * 0.5f;
    const float cyModel = pos.y + size.y * 0.53f;
    const float kUnit = size.y * 0.415f;
    const float camDist = 3.1f;

    auto Project = [&](const Vec3 &v, ImVec2 *screen, float *depth) {
        float d = camDist + v.z;
        if (d < 0.45f) d = 0.45f;
        const float persp = kUnit * camDist / d;
        if (screen) *screen = ImVec2(cx + v.x * persp, cyModel - (v.y + bob) * persp);
        if (depth) *depth = v.z;
        return persp;
    };

    // Turntable base: a flat ring under the feet reads as a stage.
    {
        const float baseK = kUnit * camDist / (camDist + 0.0f);
        const float radius = 0.34f;
        ImVec2 pts[28];
        for (int i = 0; i < 24; ++i)
        {
            const float a = 2.0f * IM_PI * ((float)i / 24.0f);
            const Vec3 p = Rotate(mk(cosf(a) * radius, -0.985f, sinf(a) * radius));
            Project(p, &pts[i], nullptr);
        }
        ImU32 accent = IM_COL32(158, 96, 255, 70);
        for (int i = 0; i < 24; ++i)
            draw->AddLine(pts[i], pts[(i + 1) % 24], accent, 1.4f);
        (void)baseK;
    }

    const Vec3 light = Normalize(mk(-0.42f, 0.66f, -0.62f));

    struct PFace
    {
        ImVec2 p[4];
        float depth;
        ImU32 col;
        int count;
    };
    static std::vector<PFace> faces;
    faces.clear();

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

    for (const Face &f : Mesh())
    {
        ImVec2 scr[4];
        float depthSum = 0.0f;
        for (int i = 0; i < f.count; ++i)
        {
            const Vec3 v = Rotate(f.p[i]);
            float d = 0.0f;
            Project(v, &scr[i], &d);
            depthSum += d;
        }
        depthSum /= (float)f.count;

        // Bounds come from every vertex so the ESP box wraps the silhouette.
        for (int i = 0; i < f.count; ++i)
        {
            if (scr[i].x < minX) minX = scr[i].x;
            if (scr[i].y < minY) minY = scr[i].y;
            if (scr[i].x > maxX) maxX = scr[i].x;
            if (scr[i].y > maxY) maxY = scr[i].y;
        }

        const Vec3 n = Rotate(f.n);
        if (!f.twoSided && n.z > 0.02f) continue; // back facing

        float lit = f.twoSided ? fabsf(Dot(n, light)) : Dot(n, light);
        if (lit < 0.0f) lit = 0.0f;
        float bright = 0.56f + 0.55f * lit;
        bright *= 1.0f - ImClamp(depthSum * 0.14f, 0.0f, 0.20f);

        const int r = (int)(((f.col >> IM_COL32_R_SHIFT) & 0xFF) * bright);
        const int g = (int)(((f.col >> IM_COL32_G_SHIFT) & 0xFF) * bright);
        const int b = (int)(((f.col >> IM_COL32_B_SHIFT) & 0xFF) * bright);

        PFace pf;
        pf.p[0] = scr[0]; pf.p[1] = scr[1]; pf.p[2] = scr[2]; pf.p[3] = scr[3];
        pf.depth = depthSum;
        pf.count = f.count;
        pf.col = IM_COL32(ImMin(r, 255), ImMin(g, 255), ImMin(b, 255), 255);
        faces.push_back(pf);
    }

    std::sort(faces.begin(), faces.end(), [](const PFace &a, const PFace &b) {
        return a.depth > b.depth;
    });

    for (const PFace &pf : faces)
    {
        if (pf.count == 4) draw->AddConvexPolyFilled(pf.p, 4, pf.col);
        else draw->AddConvexPolyFilled(pf.p, 3, pf.col);
    }

    if (minX > maxX || minY > maxY) return out;

    out.valid = true;
    out.min = ImVec2(minX, minY);
    out.max = ImVec2(maxX, maxY);
    out.center = ImVec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);

    // Joints for the skeleton / line overlays.
    const Vec3 joints[16] = {
        mk(0.0f, 0.98f, 0.0f),
        mk(0.0f, 0.62f, 0.0f),
        mk(-0.176f, 0.545f, 0.0f), mk(0.176f, 0.545f, 0.0f),
        mk(-0.236f, 0.285f, 0.01f), mk(0.236f, 0.285f, 0.01f),
        mk(-0.246f, 0.02f, 0.025f), mk(0.246f, 0.02f, 0.025f),
        mk(0.0f, 0.40f, 0.0f),
        mk(0.0f, 0.16f, 0.0f),
        mk(-0.09f, 0.06f, 0.0f), mk(0.09f, 0.06f, 0.0f),
        mk(-0.09f, -0.28f, 0.01f), mk(0.09f, -0.28f, 0.01f),
        mk(-0.087f, -0.87f, 0.0f), mk(0.087f, -0.87f, 0.0f),
    };
    for (int i = 0; i < 16; ++i) Project(Rotate(joints[i]), &out.joint[i], nullptr);

    // Face overlay: crisp anime eyes drawn on the projected head so they turn
    // with the model and fade out when the head faces away.
    const Vec3 faceNormal = Rotate(mk(0.0f, 0.0f, 1.0f));
    out.facing = ImClamp(-faceNormal.z, 0.0f, 1.0f);
    Project(Rotate(HeadCenter()), &out.head, nullptr);
    out.headRadius = kUnit * camDist / (camDist + Rotate(HeadCenter()).z) * HeadRadius;

    if (out.facing > 0.12f)
    {
        const int alpha = (int)(255.0f * ImClamp((out.facing - 0.12f) * 1.8f, 0.0f, 1.0f));
        const float eyeW = out.headRadius * 0.300f;
        const float eyeH = out.headRadius * 0.370f;
        for (int s = -1; s <= 1; s += 2)
        {
            const Vec3 eyeModel = mk(0.068f * (float)s, 0.848f, 0.146f);
            ImVec2 eye;
            const float persp = Project(Rotate(eyeModel), &eye, nullptr);
            // foreshorten horizontally as the head turns away from the camera
            const float w = eyeW * (persp / kUnit) * (0.55f + 0.45f * out.facing);
            const float h = eyeH * (persp / kUnit);
            // sclera, iris, pupil, highlight and a lash line - no dark ring,
            // which made the pair read as goggles on a small head.
            draw->AddEllipseFilled(eye, w, h, IM_COL32(252, 250, 255, alpha), 0.0f, 18);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.08f), w * 0.72f, h * 0.80f,
                                   IM_COL32(148, 126, 202, alpha), 0.0f, 18);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.14f), w * 0.33f, h * 0.40f,
                                   IM_COL32(42, 26, 66, alpha), 0.0f, 14);
            draw->AddCircleFilled(ImVec2(eye.x - w * 0.28f, eye.y - h * 0.22f),
                                  w * 0.19f, IM_COL32(255, 255, 255, alpha), 10);
            draw->PathClear();
            draw->PathArcTo(eye, w * 1.02f, IM_PI * 1.03f, IM_PI * 1.97f, 12);
            draw->PathStroke(IM_COL32(52, 40, 66, alpha), false, 1.6f);
        }
        ImVec2 mouth;
        const float mouthScale = Project(Rotate(mk(0.0f, 0.788f, 0.148f)), &mouth, nullptr) / kUnit;
        draw->PathClear();
        draw->PathArcTo(mouth, out.headRadius * 0.060f * mouthScale, 0.30f, IM_PI - 0.30f, 10);
        draw->PathStroke(IM_COL32(196, 108, 138, alpha), false, 1.2f);
    }

    return out;
}

} // namespace esp_model_preview
