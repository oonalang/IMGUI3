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
    int count; // 3 or 4
};

inline std::vector<Face> &Mesh()
{
    static std::vector<Face> mesh;
    return mesh;
}

inline void PushPoly(Vec3 a, Vec3 b, Vec3 c, Vec3 d, int count, ImU32 col, Vec3 inner)
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

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------

inline ImU32 Skin()      { return IM_COL32(248, 221, 203, 255); }
inline ImU32 Hair()      { return IM_COL32(150, 120, 216, 255); }
inline ImU32 HairDeep()  { return IM_COL32(118, 94, 186, 255); }
inline ImU32 HairTip()   { return IM_COL32(236, 138, 222, 255); }
inline ImU32 Coat()      { return IM_COL32(58, 48, 92, 255); }
inline ImU32 CoatDark()  { return IM_COL32(44, 36, 70, 255); }
inline ImU32 Accent()    { return IM_COL32(158, 96, 255, 255); }
inline ImU32 Skirt()     { return IM_COL32(50, 40, 78, 255); }
inline ImU32 Boot()      { return IM_COL32(30, 24, 44, 255); }

// Head geometry is shared by the mesh and by the face overlay.
inline const Vec3 &HeadCenter()
{
    static const Vec3 c = mk(0.0f, 0.82f, 0.0f);
    return c;
}
inline const float HeadRadius = 0.175f;

inline void BuildModel()
{
    Mesh().clear();

    // ---- legs: thigh, boot, cuff, foot ------------------------------------
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = 0.085f * (float)s;
        AddTube(mk(x, 0.06f, 0.0f), mk(x * 1.06f, -0.28f, 0.01f), 0.082f, 0.066f, 12, Skin());
        AddTube(mk(x * 1.06f, -0.26f, 0.01f), mk(x * 1.02f, -0.87f, 0.0f), 0.070f, 0.050f, 12, Boot());
        AddTube(mk(x * 1.06f, -0.25f, 0.01f), mk(x * 1.06f, -0.33f, 0.01f), 0.075f, 0.072f, 12, Accent());
        AddBox(mk(x * 1.0f, -0.93f, 0.045f), 0.058f, 0.048f, 0.105f, Boot());
    }

    // ---- skirt ------------------------------------------------------------
    const Ring skirt[] = {
        {0.17f, 0.150f, 0.112f, 0.0f},
        {0.03f, 0.205f, 0.150f, 0.0f},
        {-0.075f, 0.288f, 0.208f, 0.0f},
    };
    AddLoft(skirt, 3, 20, Skirt());

    // ---- torso ------------------------------------------------------------
    const Ring torso[] = {
        {0.60f, 0.176f, 0.106f, 0.0f},
        {0.50f, 0.170f, 0.112f, 0.0f},
        {0.38f, 0.152f, 0.118f, 0.0f},
        {0.26f, 0.114f, 0.092f, 0.0f},
        {0.16f, 0.142f, 0.106f, 0.0f},
        {0.06f, 0.152f, 0.112f, 0.0f},
    };
    AddLoft(torso, 6, 20, Coat());
    // Collar and hem trim.
    const Ring collar[] = {
        {0.605f, 0.178f, 0.108f, 0.0f},
        {0.565f, 0.176f, 0.108f, 0.0f},
    };
    AddLoft(collar, 2, 20, CoatDark(), false);
    AddEllipsoid(mk(0.0f, 0.545f, 0.115f), 0.030f, 0.024f, 0.020f, 10, 6, Accent());

    // ---- neck / head ------------------------------------------------------
    AddTube(mk(0.0f, 0.56f, 0.0f), mk(0.0f, 0.70f, 0.0f), 0.052f, 0.048f, 12, Skin());
    AddEllipsoid(HeadCenter(), HeadRadius, HeadRadius * 1.06f, HeadRadius * 0.96f, 20, 14, Skin());

    // ---- hair -------------------------------------------------------------
    // Cap with the face cut open, long back sheet, two twintails.
    AddEllipsoid(mk(0.0f, 0.845f, -0.018f), 0.196f, 0.205f, 0.190f, 20, 14, Hair(), 0.856f, 0.0f);
    const Ring backHair[] = {
        {0.88f, 0.160f, 0.075f, -0.060f},
        {0.74f, 0.188f, 0.088f, -0.088f},
        {0.54f, 0.180f, 0.080f, -0.100f},
        {0.34f, 0.140f, 0.060f, -0.110f},
        {0.18f, 0.078f, 0.036f, -0.110f},
    };
    AddLoft(backHair, 5, 16, HairDeep());
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = (float)s;
        AddEllipsoid(mk(0.185f * x, 0.790f, -0.045f), 0.048f, 0.040f, 0.040f, 12, 8, Accent());
        AddTube(mk(0.190f * x, 0.780f, -0.050f), mk(0.330f * x, 0.230f, -0.100f), 0.088f, 0.048f, 12, Hair());
        AddTube(mk(0.330f * x, 0.230f, -0.100f), mk(0.365f * x, -0.140f, -0.120f), 0.048f, 0.016f, 12, HairTip());
    }

    // Fringe locks over the forehead, ending just above the eyes, plus side
    // locks that frame the face so the head reads as anime rather than bald.
    for (int i = -2; i <= 2; ++i)
    {
        const float bx = 0.062f * (float)i;
        const float len = 0.088f - fabsf((float)i) * 0.006f;
        AddTube(mk(bx, 0.905f, 0.055f), mk(bx * 1.18f, 0.905f - len, 0.150f),
                0.042f, 0.014f, 8, Hair());
    }
    for (int s2 = -1; s2 <= 1; s2 += 2)
    {
        const float sx = (float)s2;
        AddTube(mk(0.176f * sx, 0.870f, 0.010f), mk(0.205f * sx, 0.640f, 0.030f),
                0.048f, 0.030f, 10, Hair());
        AddTube(mk(0.205f * sx, 0.640f, 0.030f), mk(0.220f * sx, 0.470f, 0.020f),
                0.030f, 0.012f, 10, HairTip());
    }

    // ---- arms -------------------------------------------------------------
    for (int s = -1; s <= 1; s += 2)
    {
        const float x = (float)s;
        const Vec3 shoulder = mk(0.176f * x, 0.545f, 0.0f);
        const Vec3 elbow = mk(0.236f * x, 0.285f, 0.010f);
        const Vec3 wrist = mk(0.246f * x, 0.020f, 0.025f);
        AddEllipsoid(shoulder, 0.062f, 0.062f, 0.062f, 12, 8, Coat());
        AddTube(shoulder, elbow, 0.055f, 0.042f, 10, Coat());
        AddTube(elbow, wrist, 0.041f, 0.032f, 10, Skin());
        AddEllipsoid(wrist, 0.043f, 0.052f, 0.040f, 12, 8, Skin());
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
        if (n.z > 0.02f) continue; // back facing

        float lit = Dot(n, light);
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
        const float eyeW = out.headRadius * 0.340f;
        const float eyeH = out.headRadius * 0.420f;
        for (int s = -1; s <= 1; s += 2)
        {
            const Vec3 eyeModel = mk(0.070f * (float)s, 0.822f, 0.148f);
            ImVec2 eye;
            const float persp = Project(Rotate(eyeModel), &eye, nullptr);
            const float w = eyeW * (persp / kUnit);
            const float h = eyeH * (persp / kUnit);
            draw->AddEllipseFilled(eye, w, h, IM_COL32(30, 16, 48, alpha), 0.0f, 18);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.10f), w * 0.78f, h * 0.76f,
                                   IM_COL32(168, 104, 246, alpha), 0.0f, 18);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.20f), w * 0.36f, h * 0.34f,
                                   IM_COL32(36, 16, 60, alpha), 0.0f, 14);
            draw->AddCircleFilled(ImVec2(eye.x - w * 0.30f, eye.y - h * 0.24f),
                                  w * 0.20f, IM_COL32(255, 255, 255, alpha), 10);
        }
        ImVec2 mouth;
        const float mouthScale = Project(Rotate(mk(0.0f, 0.752f, 0.150f)), &mouth, nullptr) / kUnit;
        draw->PathClear();
        draw->PathArcTo(mouth, out.headRadius * 0.060f * mouthScale, 0.30f, IM_PI - 0.30f, 10);
        draw->PathStroke(IM_COL32(196, 108, 138, alpha), false, 1.2f);
    }

    return out;
}

} // namespace esp_model_preview
