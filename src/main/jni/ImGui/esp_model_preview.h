#pragma once

#include "Call_ImGui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
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
inline float Length(const Vec3 &v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
inline Vec3 Normalize(const Vec3 &v)
{
    const float len = Length(v);
    if (len < 1e-6f) return Vec3{0.0f, 0.0f, 0.0f};
    return Vec3{v.x / len, v.y / len, v.z / len};
}

struct Face
{
    Vec3 p[4];      // world (model space) corners
    Vec3 n[4];      // per corner normals, always pointing outwards
    ImU32 col;
    int count;
    bool twoSided;
    bool shiny;
};

inline std::vector<Face> &Mesh()
{
    static std::vector<Face> mesh;
    return mesh;
}

inline void PushPoly(Vec3 a, Vec3 b, Vec3 c, Vec3 d,
                     Vec3 na, Vec3 nb, Vec3 nc, Vec3 nd,
                     int count, ImU32 col, bool twoSided = false, bool shiny = false)
{
    Vec3 p[4] = {a, b, c, d};
    Vec3 nn[4] = {na, nb, nc, nd};

    // keep the winding consistent with the supplied normals
    const Vec3 avg = (nn[0] + nn[1] + nn[2] + nn[3]) * 0.25f;
    const Vec3 fn = Normalize(Cross(b - a, c - a));
    if (Dot(fn, avg) < 0.0f)
    {
        const Vec3 tmp = p[1];
        p[1] = p[3];
        p[3] = tmp;
    }

    Face f;
    for (int i = 0; i < 4; ++i)
    {
        f.p[i] = p[i];
        f.n[i] = nn[i];
    }
    f.col = col;
    f.count = count;
    f.twoSided = twoSided;
    f.shiny = shiny;
    Mesh().push_back(f);
}

inline void PushFlat(Vec3 a, Vec3 b, Vec3 c, Vec3 d, int count, ImU32 col,
                     Vec3 n, bool twoSided = false, bool shiny = false)
{
    PushPoly(a, b, c, d, n, n, n, n, count, col, twoSided, shiny);
}

inline void PushQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 na, Vec3 nb, Vec3 nc, Vec3 nd,
                     ImU32 col, bool twoSided = false, bool shiny = false)
{
    PushPoly(a, b, c, d, na, nb, nc, nd, 4, col, twoSided, shiny);
}

inline void PushTri(Vec3 a, Vec3 b, Vec3 c, Vec3 na, Vec3 nb, Vec3 nc,
                     ImU32 col, bool twoSided = false, bool shiny = false)
{
    PushPoly(a, b, c, c, na, nb, nc, nc, 3, col, twoSided, shiny);
}

inline void AddEllipsoid(Vec3 c, float rx, float ry, float rz, int segs, int rings,
                         ImU32 col, float cutY = -1.0e9f, float cutZ = -1.0e9f,
                         bool shiny = false)
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
            auto N = [&](float lat, float lon) {
                return Normalize(mk(cosf(lat) * cosf(lon) / rx,
                                     sinf(lat) / ry,
                                     cosf(lat) * sinf(lon) / rz));
            };

            const Vec3 a = P(lat0, lon0);
            const Vec3 b = P(lat0, lon1);
            const Vec3 d = P(lat1, lon1);
            const Vec3 e = P(lat1, lon0);
            const Vec3 cen = (a + b + d + e) * 0.25f;
            if (cen.y < cutY && cen.z > cutZ) continue;

            PushQuad(a, b, d, e, N(lat0, lon0), N(lat0, lon1), N(lat1, lon1), N(lat1, lon0),
                     col, false, shiny);
        }
    }
}

inline void AddTube(Vec3 p0, Vec3 p1, float r0, float r1, int segs, ImU32 col,
                     bool capEnds = false, bool shiny = false)
{
    const Vec3 axis = p1 - p0;
    const float len = Length(axis);
    if (len < 1e-5f) return;
    const Vec3 dir = axis * (1.0f / len);
    const Vec3 helper = fabsf(dir.y) > 0.95f ? mk(1.0f, 0.0f, 0.0f) : mk(0.0f, 1.0f, 0.0f);
    const Vec3 ex = Normalize(Cross(dir, helper));
    const Vec3 ez = Normalize(Cross(ex, dir));

    for (int j = 0; j < segs; ++j)
    {
        const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
        const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
        const Vec3 d0 = ex * cosf(a0) + ez * sinf(a0);
        const Vec3 d1 = ex * cosf(a1) + ez * sinf(a1);
        const Vec3 n0 = Normalize(d0 * len + dir * (r0 - r1));
        const Vec3 n1 = Normalize(d1 * len + dir * (r0 - r1));
        PushQuad(p0 + d0 * r0, p0 + d1 * r1, p1 + d1 * r1, p1 + d0 * r0,
                 n0, n1, n1, n0, col, false, shiny);
    }

    if (capEnds)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            const Vec3 d0 = ex * cosf(a0) + ez * sinf(a0);
            const Vec3 d1 = ex * cosf(a1) + ez * sinf(a1);
            PushTri(p0, p0 + d0 * r0, p0 + d1 * r0, dir * -1.0f, dir * -1.0f, dir * -1.0f, col);
            PushTri(p1, p1 + d1 * r1, p1 + d0 * r1, dir, dir, dir, col);
        }
    }
}

struct Ring
{
    float y, rx, rz, cz;
};

// stacked ellipses; flutes cuts pleats into the surface and colB shades every
// other one, which is what gives the skirt its folds
inline void AddLoft(const Ring *rings, int count, int segs, ImU32 col,
                    bool caps = true, int flutes = 0, float fluteAmp = 0.0f,
                    ImU32 colB = 0, bool shiny = false)
{
    if (count < 2) return;

    auto fluteAt = [&](float a) {
        if (flutes <= 0) return 1.0f;
        return 1.0f + fluteAmp * cosf((float)flutes * a);
    };

    auto deriv = [&](int i, bool isX) {
        const int i0 = ImMax(0, i - 1);
        const int i1 = ImMin(count - 1, i + 1);
        const float dy = rings[i1].y - rings[i0].y;
        if (fabsf(dy) < 1e-5f) return 0.0f;
        const float v0 = isX ? rings[i0].rx : rings[i0].rz;
        const float v1 = isX ? rings[i1].rx : rings[i1].rz;
        return (v1 - v0) / dy;
    };

    auto P = [&](const Ring &r, float a) {
        const float f = fluteAt(a);
        return mk(r.rx * cosf(a) * f, r.y, r.cz + r.rz * sinf(a) * f);
    };
    auto N = [&](const Ring &r, int idx, float a) {
        const float rxp = deriv(idx, true);
        const float rzp = deriv(idx, false);
        const float ca = cosf(a);
        const float sa = sinf(a);
        return Normalize(mk(r.rz * ca,
                             -(r.rz * rxp * ca * ca + r.rx * rzp * sa * sa),
                             r.rx * sa));
    };

    for (int i = 0; i + 1 < count; ++i)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            ImU32 c = col;
            if (colB != 0 && flutes > 0 && (j % ImMax(1, segs / flutes)) >= (segs / flutes) / 2)
                c = colB;
            PushQuad(P(rings[i], a0), P(rings[i], a1), P(rings[i + 1], a1), P(rings[i + 1], a0),
                     N(rings[i], i, a0), N(rings[i], i, a1),
                     N(rings[i + 1], i + 1, a1), N(rings[i + 1], i + 1, a0),
                     c, false, shiny);
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
            PushTri(topC, P(top, a1), P(top, a0), mk(0, 1, 0), mk(0, 1, 0), mk(0, 1, 0), col);
            PushTri(botC, P(bottom, a0), P(bottom, a1), mk(0, -1, 0), mk(0, -1, 0), mk(0, -1, 0), col);
        }
    }
}

inline void AddSweep(const Vec3 *path, const float *radii, int count, int segs, ImU32 col,
                     bool shiny = false, bool capEnds = true)
{
    if (count < 2) return;

    Vec3 tan[16];
    Vec3 ex[16];
    Vec3 ez[16];
    for (int i = 0; i < count; ++i)
    {
        const Vec3 prev = path[ImMax(0, i - 1)];
        const Vec3 next = path[ImMin(count - 1, i + 1)];
        tan[i] = Normalize(next - prev);
        const Vec3 ref = fabsf(tan[i].z) < 0.9f ? mk(0.0f, 0.0f, 1.0f) : mk(1.0f, 0.0f, 0.0f);
        ex[i] = Normalize(Cross(ref, tan[i]));
        ez[i] = Normalize(Cross(tan[i], ex[i]));
    }

    for (int i = 0; i + 1 < count; ++i)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            const Vec3 d0 = ex[i] * cosf(a0) + ez[i] * sinf(a0);
            const Vec3 d1 = ex[i] * cosf(a1) + ez[i] * sinf(a1);
            const Vec3 e0 = ex[i + 1] * cosf(a0) + ez[i + 1] * sinf(a0);
            const Vec3 e1 = ex[i + 1] * cosf(a1) + ez[i + 1] * sinf(a1);
            PushQuad(path[i] + d0 * radii[i], path[i] + d1 * radii[i],
                     path[i + 1] + e1 * radii[i + 1], path[i + 1] + e0 * radii[i + 1],
                     d0, d1, e1, e0, col, false, shiny);
        }
    }

    if (capEnds)
    {
        for (int j = 0; j < segs; ++j)
        {
            const float a0 = 2.0f * IM_PI * ((float)j / (float)segs);
            const float a1 = 2.0f * IM_PI * ((float)(j + 1) / (float)segs);
            const Vec3 d0 = ex[0] * cosf(a0) + ez[0] * sinf(a0);
            const Vec3 d1 = ex[0] * cosf(a1) + ez[0] * sinf(a1);
            const Vec3 e0 = ex[count - 1] * cosf(a0) + ez[count - 1] * sinf(a0);
            const Vec3 e1 = ex[count - 1] * cosf(a1) + ez[count - 1] * sinf(a1);
            PushTri(path[0], path[0] + d1 * radii[0], path[0] + d0 * radii[0],
                    tan[0] * -1.0f, tan[0] * -1.0f, tan[0] * -1.0f, col);
            PushTri(path[count - 1], path[count - 1] + e0 * radii[count - 1],
                    path[count - 1] + e1 * radii[count - 1],
                    tan[count - 1], tan[count - 1], tan[count - 1], col);
        }
    }
}

inline ImU32 SkinCol()     { return IM_COL32(255, 231, 214, 255); }
inline ImU32 SkinShade()   { return IM_COL32(246, 208, 190, 255); }
inline ImU32 HairCol()     { return IM_COL32(142, 108, 214, 255); }
inline ImU32 HairDeep()    { return IM_COL32( 96,  70, 162, 255); }
inline ImU32 HairLight()   { return IM_COL32(190, 162, 244, 255); }
inline ImU32 BlouseCol()   { return IM_COL32(249, 250, 254, 255); }
inline ImU32 BlouseShade() { return IM_COL32(226, 228, 240, 255); }
inline ImU32 NavyCol()     { return IM_COL32( 44,  50, 100, 255); }
inline ImU32 NavyLight()   { return IM_COL32( 66,  74, 138, 255); }
inline ImU32 RibbonCol()   { return IM_COL32(212,  60,  96, 255); }
inline ImU32 SockCol()     { return IM_COL32( 40,  40,  66, 255); }
inline ImU32 SockLight()   { return IM_COL32( 58,  58,  94, 255); }
inline ImU32 ShoeCol()     { return IM_COL32( 30,  30,  48, 255); }
inline ImU32 GoldCol()     { return IM_COL32(232, 190, 108, 255); }
inline ImU32 InkCol()      { return IM_COL32( 30,  22,  46, 255); }
inline ImU32 EyeWhite()    { return IM_COL32(253, 251, 255, 255); }
inline ImU32 EyeIris()     { return IM_COL32( 78,  48, 150, 255); }
inline ImU32 EyeIrisLit()  { return IM_COL32(150, 208, 255, 255); }
inline ImU32 EyePupil()    { return IM_COL32( 38,  22,  62, 255); }
inline ImU32 LashCol()     { return IM_COL32( 46,  28,  66, 255); }
inline ImU32 BrowCol()     { return IM_COL32(108,  78, 158, 255); }
inline ImU32 BlushCol()    { return IM_COL32(255, 140, 158, 255); }
inline ImU32 MouthCol()    { return IM_COL32(198,  96, 128, 255); }

inline const Vec3 &HeadCenter()
{
    static const Vec3 c = mk(0.0f, 1.605f, 0.004f);
    return c;
}
inline const float HeadRadius = 0.122f;

inline const float ModelBottom = -0.01f;
inline const float ModelTop = 1.80f;
inline const float ModelWidth = 0.56f;   // widest on screen: shoulders to twin tails

inline void BuildModel()
{
    Mesh().clear();

    const ImU32 skin = SkinCol();
    const ImU32 hair = HairCol();
    const ImU32 hairDeep = HairDeep();
    const ImU32 blouse = BlouseCol();
    const ImU32 navy = NavyCol();
    const ImU32 ribbon = RibbonCol();
    const ImU32 sock = SockCol();

    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = (float)s;
        AddTube(mk(0.060f * sx, 0.900f, 0.000f), mk(0.066f * sx, 0.620f, 0.008f), 0.084f, 0.064f, 14, skin);
        AddTube(mk(0.060f * sx, 0.706f, 0.004f), mk(0.066f * sx, 0.620f, 0.008f), 0.072f, 0.067f, 14, sock);
        AddTube(mk(0.060f * sx, 0.722f, 0.003f), mk(0.060f * sx, 0.698f, 0.004f), 0.075f, 0.073f, 14, SockLight());
        AddTube(mk(0.066f * sx, 0.620f, 0.008f), mk(0.064f * sx, 0.420f, 0.014f), 0.066f, 0.055f, 14, sock);
        AddTube(mk(0.064f * sx, 0.420f, 0.014f), mk(0.058f * sx, 0.100f, 0.004f), 0.055f, 0.041f, 14, sock);
        AddEllipsoid(mk(0.058f * sx, 0.048f, 0.034f), 0.052f, 0.042f, 0.096f, 14, 8, ShoeCol(), -1.0e9f, -1.0e9f, true);
        AddEllipsoid(mk(0.058f * sx, 0.016f, 0.034f), 0.054f, 0.017f, 0.098f, 14, 6, blouse);
    }

    const Ring skirtRings[] = {
        {1.086f, 0.100f, 0.080f, 0.0f},
        {1.028f, 0.126f, 0.100f, 0.0f},
        {0.966f, 0.150f, 0.118f, 0.0f},
        {0.902f, 0.170f, 0.134f, 0.0f},
        {0.840f, 0.186f, 0.146f, 0.0f},
    };
    AddLoft(skirtRings, 5, 44, navy, true, 10, 0.045f, NavyLight());

    const Ring waistRings[] = {
        {0.998f, 0.092f, 0.064f, 0.0f},
        {1.048f, 0.090f, 0.062f, 0.0f},
        {1.090f, 0.096f, 0.068f, 0.0f},
    };
    AddLoft(waistRings, 3, 22, skin);

    const Ring blouseRings[] = {
        {1.084f, 0.100f, 0.074f, 0.0f},
        {1.142f, 0.108f, 0.080f, 0.0f},
        {1.202f, 0.117f, 0.086f, 0.0f},
        {1.262f, 0.123f, 0.089f, 0.0f},
        {1.320f, 0.114f, 0.081f, 0.0f},
    };
    AddLoft(blouseRings, 5, 24, blouse);

    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = (float)s;
        AddEllipsoid(mk(0.050f * sx, 1.226f, 0.044f), 0.062f, 0.050f, 0.040f, 14, 9, skin);
        AddEllipsoid(mk(0.112f * sx, 1.556f, -0.006f), 0.021f, 0.031f, 0.019f, 10, 7, skin);
    }

    AddEllipsoid(mk(0.0f, 1.330f, 0.004f), 0.122f, 0.042f, 0.090f, 20, 10, navy);
    AddEllipsoid(mk(0.0f, 1.300f, 0.082f), 0.056f, 0.050f, 0.019f, 14, 9, navy);
    AddEllipsoid(mk(0.0f, 1.306f, -0.088f), 0.090f, 0.054f, 0.017f, 14, 7, navy);
    AddEllipsoid(mk(0.0f, 1.336f, 0.060f), 0.044f, 0.030f, 0.025f, 14, 7, ribbon);
    AddTube(mk(0.014f, 1.324f, 0.072f), mk(0.024f, 1.230f, 0.090f), 0.018f, 0.011f, 10, ribbon);
    AddTube(mk(-0.014f, 1.324f, 0.072f), mk(-0.026f, 1.236f, 0.086f), 0.018f, 0.011f, 10, ribbon);

    AddTube(mk(0.0f, 1.320f, 0.002f), mk(0.0f, 1.440f, 0.006f), 0.046f, 0.042f, 14, skin);

    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = (float)s;
        const Vec3 shoulder = mk(0.112f * sx, 1.312f, 0.000f);
        const Vec3 cuff    = mk(0.150f * sx, 1.166f, 0.014f);
        const Vec3 elbow   = mk(0.178f * sx, 1.018f, 0.028f);
        const Vec3 wrist   = mk(0.192f * sx, 0.772f, 0.052f);
        AddTube(shoulder, cuff, 0.060f, 0.052f, 14, blouse);
        AddTube(mk(0.145f * sx, 1.192f, 0.013f), mk(0.150f * sx, 1.162f, 0.014f), 0.057f, 0.054f, 14, navy);
        AddTube(cuff, elbow, 0.048f, 0.042f, 14, skin);
        AddTube(elbow, wrist, 0.042f, 0.030f, 14, skin);
        AddEllipsoid(wrist, 0.033f, 0.050f, 0.030f, 12, 7, skin);
    }

    const Ring headRings[] = {
        {1.782f, 0.010f, 0.012f, 0.000f},
        {1.772f, 0.042f, 0.046f, -0.002f},
        {1.752f, 0.072f, 0.078f, -0.004f},
        {1.722f, 0.098f, 0.104f, -0.005f},
        {1.688f, 0.114f, 0.118f, -0.004f},
        {1.650f, 0.122f, 0.122f,  0.000f},
        {1.612f, 0.122f, 0.118f,  0.005f},
        {1.574f, 0.116f, 0.110f,  0.010f},
        {1.538f, 0.104f, 0.100f,  0.014f},
        {1.502f, 0.086f, 0.086f,  0.016f},
        {1.468f, 0.062f, 0.066f,  0.014f},
        {1.442f, 0.030f, 0.036f,  0.010f},
        {1.428f, 0.008f, 0.010f,  0.004f},
    };
    AddLoft(headRings, 13, 26, skin, false);

    AddEllipsoid(mk(0.0f, 1.630f, -0.004f), 0.137f, 0.172f, 0.135f, 26, 14, hair, 1.664f, -0.012f, true);

    const Ring backHair[] = {
        {1.720f, 0.132f, 0.124f, -0.110f},
        {1.612f, 0.156f, 0.142f, -0.120f},
        {1.496f, 0.164f, 0.148f, -0.128f},
        {1.368f, 0.158f, 0.142f, -0.134f},
        {1.226f, 0.146f, 0.130f, -0.138f},
        {1.090f, 0.130f, 0.116f, -0.138f},
        {0.986f, 0.112f, 0.100f, -0.132f},
    };
    AddLoft(backHair, 7, 24, hairDeep, false, 0, 0.0f, 0, true);

    for (int i = -3; i <= 3; ++i)
    {
        const float t = (float)i;
        const float bx = 0.040f * t;
        const float drop = 0.055f - 0.012f * fabsf(t);
        const Vec3 path[4] = {
            mk(bx * 0.72f, 1.756f, 0.020f),
            mk(bx * 0.98f, 1.726f, 0.078f),
            mk(bx * 1.10f, 1.700f, 0.104f),
            mk(bx * 1.16f, 1.700f - drop, 0.112f),
        };
        const float radii[4] = {0.040f, 0.032f, 0.022f, 0.007f};
        AddSweep(path, radii, 4, 10, hair, true, true);
    }
    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = (float)s;
        const Vec3 path[5] = {
            mk(0.118f * sx, 1.678f, 0.004f),
            mk(0.134f * sx, 1.596f, 0.030f),
            mk(0.140f * sx, 1.508f, 0.038f),
            mk(0.136f * sx, 1.440f, 0.034f),
            mk(0.126f * sx, 1.396f, 0.026f),
        };
        const float radii[5] = {0.038f, 0.034f, 0.026f, 0.015f, 0.006f};
        AddSweep(path, radii, 5, 10, hair, true, true);
    }
    for (int s = -1; s <= 1; s += 2)
    {
        const float sx = (float)s;
        const Vec3 path[6] = {
            mk(0.120f * sx, 1.632f, -0.032f),
            mk(0.188f * sx, 1.552f, -0.058f),
            mk(0.226f * sx, 1.418f, -0.072f),
            mk(0.238f * sx, 1.268f, -0.076f),
            mk(0.228f * sx, 1.132f, -0.070f),
            mk(0.214f * sx, 1.040f, -0.060f),
        };
        const float radii[6] = {0.046f, 0.050f, 0.044f, 0.032f, 0.020f, 0.007f};
        AddSweep(path, radii, 6, 12, hair, true, true);

        AddEllipsoid(mk(0.130f * sx, 1.628f, -0.034f), 0.032f, 0.026f, 0.026f, 12, 7, ribbon);
    }

    {
        auto dir = [](float dx, float dy, float dz) {
            const float l = sqrtf(dx * dx + dy * dy + dz * dz);
            return mk(dx / l, dy / l, dz / l);
        };
        const Vec3 d0 = dir(-0.56f, 0.60f, 0.57f);
        const Vec3 d1 = dir(-0.30f, 0.66f, 0.69f);
        const Vec3 d2 = dir(0.0f, 0.66f, 0.75f);
        const Vec3 d3 = dir(0.30f, 0.66f, 0.69f);
        const Vec3 d4 = dir(0.56f, 0.60f, 0.57f);
        const Vec3 c = mk(0.0f, 1.630f, -0.004f);
        auto surf = [&](const Vec3& d) {
            return mk(c.x + 0.140f * d.x, c.y + 0.176f * d.y, c.z + 0.138f * d.z);
        };
        const Vec3 path[5] = { surf(d0), surf(d1), surf(d2), surf(d3), surf(d4) };
        const float radii[5] = { 0.006f, 0.015f, 0.019f, 0.015f, 0.006f };
        AddSweep(path, radii, 5, 10, HairLight(), true, true);
    }

    AddEllipsoid(mk(-0.048f, 1.784f, 0.012f), 0.056f, 0.026f, 0.022f, 12, 7, ribbon);
    AddEllipsoid(mk(-0.114f, 1.770f, 0.004f), 0.042f, 0.022f, 0.019f, 12, 7, ribbon);
    AddEllipsoid(mk(-0.088f, 1.776f, 0.016f), 0.021f, 0.019f, 0.019f, 10, 6, ribbon);
    AddEllipsoid(mk(0.066f, 1.778f, -0.014f), 0.032f, 0.017f, 0.015f, 10, 6, GoldCol());
}

// fixed front view, no motion
inline const float g_yaw = IM_PI;
inline const float g_pitch = 0.0f;

struct Result
{
    bool valid = false;
    ImVec2 min = ImVec2(0.0f, 0.0f);
    ImVec2 max = ImVec2(0.0f, 0.0f);
    ImVec2 center = ImVec2(0.0f, 0.0f);
    ImVec2 head = ImVec2(0.0f, 0.0f);
    float headRadius = 0.0f;
    float facing = 0.0f;

    ImVec2 joint[16];
};

inline void AddCrescent(ImDrawList *draw, ImVec2 c, float rx, float ry, float thick,
                        ImU32 col, float a0, float a1, int segs)
{
    for (int i = 0; i < segs; ++i)
    {
        const float t0 = a0 + (a1 - a0) * ((float)i / (float)segs);
        const float t1 = a0 + (a1 - a0) * ((float)(i + 1) / (float)segs);
        const float c0 = cosf(t0), s0 = sinf(t0);
        const float c1 = cosf(t1), s1 = sinf(t1);
        const ImVec2 o0(c.x + c0 * rx, c.y + s0 * ry);
        const ImVec2 o1(c.x + c1 * rx, c.y + s1 * ry);
        const ImVec2 i0(c.x + c0 * rx * 0.84f, c.y + s0 * (ry - thick));
        const ImVec2 i1(c.x + c1 * rx * 0.84f, c.y + s1 * (ry - thick));
        draw->AddTriangleFilled(o0, o1, i0, col);
        draw->AddTriangleFilled(o1, i1, i0, col);
    }
}

inline Result DrawModel(ImDrawList *draw, const ImVec2 &pos, const ImVec2 &size)
{
    Result out;
    if (draw == nullptr || size.x < 8.0f || size.y < 8.0f) return out;
    if (Mesh().empty()) BuildModel();

    const float cyaw = cosf(g_yaw), syaw = sinf(g_yaw);
    const float cpit = cosf(g_pitch), spit = sinf(g_pitch);
    auto Rotate = [&](const Vec3 &p) {
        const float x = p.x * cyaw + p.z * syaw;
        const float z = -p.x * syaw + p.z * cyaw;
        const float y = p.y * cpit - z * spit;
        const float z2 = p.y * spit + z * cpit;
        return mk(x, y, z2);
    };

    const float cx = pos.x + size.x * 0.5f;
    const float modelH = ModelTop - ModelBottom;
    const float kUnit = ImMin(size.y * 0.97f / modelH, size.x * 0.96f / ModelWidth);
    const float cyModel = pos.y + size.y * 0.50f + ((ModelTop + ModelBottom) * 0.5f) * kUnit;
    const float camDist = 4.2f;

    auto Project = [&](const Vec3 &v, ImVec2 *screen, float *depth) {
        float d = camDist + v.z;
        if (d < 0.45f) d = 0.45f;
        const float persp = kUnit * camDist / d;
        if (screen) *screen = ImVec2(cx + v.x * persp, cyModel - v.y * persp);
        if (depth) *depth = v.z;
        return persp;
    };

    {
        ImVec2 feet;
        Project(Rotate(mk(0.0f, 0.0f, 0.0f)), &feet, nullptr);
        draw->AddEllipseFilled(ImVec2(feet.x, feet.y + kUnit * 0.012f),
                               kUnit * 0.36f, kUnit * 0.090f, IM_COL32(0, 0, 0, 80), 0.0f, 28);
        draw->AddEllipseFilled(ImVec2(feet.x, feet.y + kUnit * 0.012f),
                               kUnit * 0.22f, kUnit * 0.052f, IM_COL32(0, 0, 0, 70), 0.0f, 24);
    }

    const Vec3 key = Normalize(mk(-0.42f, 0.62f, -0.66f));
    const Vec3 fill = Normalize(mk(0.68f, 0.18f, -0.40f));
    const Vec3 bounce = Normalize(mk(0.10f, -0.60f, -0.79f));
    const Vec3 half = Normalize(key + mk(0.0f, 0.0f, -1.0f));

    struct PFace
    {
        ImVec2 p[4];
        float depth;
        ImU32 col;
        int count;
        ImVec2 hull[4];
        bool outline;
    };
    static std::vector<PFace> faces;
    faces.clear();

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
    const float outlineW = 0.0062f;

    for (const Face &f : Mesh())
    {
        ImVec2 scr[4];
        float depthSum = 0.0f;
        Vec3 avgN = f.n[0];
        for (int i = 0; i < f.count; ++i)
        {
            const Vec3 v = Rotate(f.p[i]);
            float d = 0.0f;
            Project(v, &scr[i], &d);
            depthSum += d;
            if (i > 0) avgN = avgN + f.n[i];
        }
        depthSum /= (float)f.count;
        avgN = Normalize(avgN);

        for (int i = 0; i < f.count; ++i)
        {
            if (scr[i].x < minX) minX = scr[i].x;
            if (scr[i].y < minY) minY = scr[i].y;
            if (scr[i].x > maxX) maxX = scr[i].x;
            if (scr[i].y > maxY) maxY = scr[i].y;
        }

        const Vec3 n = Rotate(avgN);
        const bool backFacing = n.z > 0.02f;

        PFace pf;
        for (int i = 0; i < 4; ++i) pf.p[i] = scr[i];
        pf.depth = depthSum;
        pf.count = f.count;

        // back faces only feed the inverted-hull ink outline; shading them too
        // would draw a wireframe instead of a contour
        if (backFacing && !f.twoSided)
        {
            for (int i = 0; i < f.count; ++i)
            {
                const Vec3 e = Rotate(f.p[i] + f.n[i] * outlineW);
                Project(e, &pf.hull[i], nullptr);
            }
            pf.outline = true;
            pf.col = InkCol();
            faces.push_back(pf);
            continue;
        }

        float lit = (Dot(n, key) * 0.5f + 0.5f) * 0.66f
                  + (Dot(n, fill) * 0.5f + 0.5f) * 0.17f
                  + (Dot(n, bounce) * 0.5f + 0.5f) * 0.17f;
        float band;
        if (lit < 0.40f) band = 0.74f;
        else if (lit < 0.62f) band = 0.89f;
        else band = 1.0f;

        const int br = (f.col >> IM_COL32_R_SHIFT) & 0xFF;
        const int bg = (f.col >> IM_COL32_G_SHIFT) & 0xFF;
        const int bb = (f.col >> IM_COL32_B_SHIFT) & 0xFF;

        int r = (int)(br * band);
        int g = (int)(bg * band);
        int b = (int)(bb * band);
        if (band < 0.9f)
        {
            r += (int)(6.0f * band);
            g += (int)(8.0f * band);
            b += (int)(24.0f * band);
        }

        const float fres = 1.0f - ImClamp(-n.z, 0.0f, 1.0f);
        const float rim = fres * fres * fres * ImClamp(lit * 0.5f + 0.55f, 0.0f, 1.0f);
        r += (int)(rim * 22.0f);
        g += (int)(rim * 28.0f);
        b += (int)(rim * 52.0f);

        if (f.shiny)
        {
            const float spec = powf(ImClamp(Dot(n, half), 0.0f, 1.0f), 30.0f);
            r += (int)(spec * 70.0f);
            g += (int)(spec * 70.0f);
            b += (int)(spec * 85.0f);
        }

        pf.col = IM_COL32(ImMin(r, 255), ImMin(g, 255), ImMin(b, 255), 255);
        pf.outline = false;
        faces.push_back(pf);
    }

    std::sort(faces.begin(), faces.end(), [](const PFace &a, const PFace &b) {
        return a.depth > b.depth;
    });

    for (const PFace &pf : faces)
    {
        if (pf.outline)
        {
            if (pf.count == 4) draw->AddConvexPolyFilled(pf.hull, 4, pf.col);
            else draw->AddConvexPolyFilled(pf.hull, 3, pf.col);
        }
        else
        {
            if (pf.count == 4) draw->AddConvexPolyFilled(pf.p, 4, pf.col);
            else draw->AddConvexPolyFilled(pf.p, 3, pf.col);
        }
    }

    if (minX > maxX || minY > maxY) return out;

    out.valid = true;
    out.min = ImVec2(minX, minY);
    out.max = ImVec2(maxX, maxY);
    out.center = ImVec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);

    const Vec3 joints[16] = {
        mk(0.0f, 1.740f, 0.0f),
        mk(0.0f, 1.340f, 0.0f),
        mk(-0.112f, 1.312f, 0.0f), mk(0.112f, 1.312f, 0.0f),
        mk(-0.178f, 1.018f, 0.03f), mk(0.178f, 1.018f, 0.03f),
        mk(-0.192f, 0.772f, 0.05f), mk(0.192f, 0.772f, 0.05f),
        mk(0.0f, 1.180f, 0.0f),
        mk(0.0f, 0.980f, 0.0f),
        mk(-0.064f, 0.420f, 0.01f), mk(0.064f, 0.420f, 0.01f),
        mk(-0.058f, 0.075f, 0.0f), mk(0.058f, 0.075f, 0.0f),
        mk(-0.058f, 0.030f, 0.05f), mk(0.058f, 0.030f, 0.05f),
    };
    for (int i = 0; i < 16; ++i) Project(Rotate(joints[i]), &out.joint[i], nullptr);

    const Vec3 faceNormal = Rotate(mk(0.0f, 0.0f, 1.0f));
    out.facing = ImClamp(-faceNormal.z, 0.0f, 1.0f);
    const Vec3 head = Rotate(HeadCenter());
    Project(head, &out.head, nullptr);
    const float headPersp = Project(head, nullptr, nullptr);
    out.headRadius = headPersp * HeadRadius;

    if (out.facing > 0.10f)
    {
        const float fade = ImClamp((out.facing - 0.10f) / 0.45f, 0.0f, 1.0f);
        const int alpha = (int)(255.0f * fade);
        const float hr = out.headRadius;

        draw->AddEllipseFilled(ImVec2(out.head.x + hr * 0.56f, out.head.y - hr * 0.04f),
                               hr * 0.30f, hr * 0.62f, IM_COL32(126, 86, 116, (int)(70 * fade)), 0.0f, 20);
        draw->AddEllipseFilled(ImVec2(out.head.x, out.head.y - hr * 0.56f),
                               hr * 0.74f, hr * 0.17f, IM_COL32(126, 86, 116, (int)(52 * fade)), 0.0f, 20);
        draw->AddEllipseFilled(ImVec2(out.head.x, out.head.y - hr * 0.46f),
                               hr * 0.60f, hr * 0.10f, IM_COL32(126, 86, 116, (int)(34 * fade)), 0.0f, 20);

        for (int s = -1; s <= 1; s += 2)
        {
            const float sx = (float)s;
            ImVec2 eye;
            const float persp = Project(Rotate(mk(0.060f * sx, 1.612f, 0.100f)), &eye, nullptr);
            const float w = 0.330f * hr * (persp / headPersp);
            const float h = 0.290f * hr * (persp / headPersp);
            if (w < 1.5f || h < 1.0f) continue;

            draw->AddEllipseFilled(ImVec2(eye.x + sx * w * 0.30f, eye.y + h * 1.55f),
                                   w * 0.62f, h * 0.34f, IM_COL32(255, 142, 158, (int)(80 * fade)), 0.0f, 16);
            draw->AddEllipseFilled(ImVec2(eye.x + sx * w * 0.30f, eye.y + h * 1.55f),
                                   w * 0.42f, h * 0.22f, IM_COL32(255, 130, 150, (int)(90 * fade)), 0.0f, 14);

            draw->AddEllipseFilled(eye, w, h, IM_COL32(253, 251, 255, alpha), 0.0f, 20);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.08f), w * 0.84f, h * 0.90f,
                                   IM_COL32(78, 48, 150, alpha), 0.0f, 20);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.42f), w * 0.66f, h * 0.48f,
                                   IM_COL32(150, 208, 255, alpha), 0.0f, 18);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.16f), w * 0.40f, h * 0.56f,
                                   IM_COL32(48, 28, 92, alpha), 0.0f, 16);
            draw->AddEllipseFilled(ImVec2(eye.x, eye.y + h * 0.20f), w * 0.24f, h * 0.34f,
                                   IM_COL32(38, 22, 62, alpha), 0.0f, 14);
            draw->AddCircleFilled(ImVec2(eye.x - w * 0.34f, eye.y - h * 0.34f), w * 0.26f,
                                  IM_COL32(255, 255, 255, alpha), 12);
            draw->AddCircleFilled(ImVec2(eye.x + w * 0.30f, eye.y + h * 0.34f), w * 0.13f,
                                  IM_COL32(255, 255, 255, (int)(180 * fade)), 10);

            AddCrescent(draw, eye, w * 1.08f, h * 1.10f, h * 0.46f,
                        IM_COL32(46, 28, 66, alpha), IM_PI * 1.03f, IM_PI * 1.97f, 14);
            const float flick = (sx < 0.0f) ? -1.0f : 1.0f;
            draw->AddTriangleFilled(
                ImVec2(eye.x + flick * w * 1.00f, eye.y - h * 0.16f),
                ImVec2(eye.x + flick * w * 1.52f, eye.y - h * 0.58f),
                ImVec2(eye.x + flick * w * 0.92f, eye.y - h * 0.68f),
                IM_COL32(46, 28, 66, alpha));

            AddCrescent(draw, eye, w * 0.90f, h * 0.88f, h * 0.16f,
                        IM_COL32(46, 28, 66, (int)(150 * fade)), IM_PI * 0.12f, IM_PI * 0.88f, 10);

            AddCrescent(draw, ImVec2(eye.x, eye.y - h * 1.72f), w * 1.00f, h * 0.26f, h * 0.09f,
                        IM_COL32(154, 124, 196, (int)(170 * fade)), IM_PI * 1.22f, IM_PI * 1.78f, 12);
        }

        ImVec2 nose;
        Project(Rotate(mk(0.0f, 1.560f, 0.112f)), &nose, nullptr);
        draw->AddTriangleFilled(ImVec2(nose.x - hr * 0.035f, nose.y - hr * 0.02f),
                                ImVec2(nose.x + hr * 0.035f, nose.y - hr * 0.02f),
                                ImVec2(nose.x, nose.y + hr * 0.035f),
                                IM_COL32(226, 178, 168, (int)(190 * fade)));

        ImVec2 mouth;
        Project(Rotate(mk(0.0f, 1.508f, 0.108f)), &mouth, nullptr);
        AddCrescent(draw, mouth, hr * 0.075f, hr * 0.050f, hr * 0.020f,
                    IM_COL32(198, 96, 128, alpha), IM_PI * 0.20f, IM_PI * 0.80f, 10);
    }

    return out;
}
}
