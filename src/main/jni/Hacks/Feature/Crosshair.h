#pragma once

// One definition per style. The renderer, the menu dropdown and the host check
// all read this table, so a style can never exist in one place and be missing in
// another.
enum ReticleGeom : unsigned
{
    ReticleGeomNone     = 0u,
    ReticleGeomDot      = 1u << 0,
    ReticleGeomCross    = 1u << 1,
    ReticleGeomTShape   = 1u << 2,
    ReticleGeomDiagonal = 1u << 3,
    ReticleGeomCircle   = 1u << 4,
    ReticleGeomChevron  = 1u << 5,
    ReticleGeomBrackets = 1u << 6
};

struct ReticleStyleDef
{
    const char* name;
    unsigned    geometry;
    float       dotScale;     // dot radius as a fraction of Size * 0.5, 0 = no dot
    float       circleScale;  // circle radius as a fraction of Size * 0.5, 0 = no circle
    bool        usesGap;      // the Gap control applies to this style
    bool        usesThickness;
};

// Indexed by ReticleStyle, so the saved Style values keep their meaning:
// 0 stays Dot and 1 stays the classic cross.
inline const ReticleStyleDef kReticleStyles[ReticleStyleCount] = {
    { "Dot",             ReticleGeomDot,                                1.00f, 0.00f, false, false },
    { "Classic Cross",   ReticleGeomCross,                              0.00f, 0.00f, false, true  },
    { "Gap Cross",       ReticleGeomCross,                              0.00f, 0.00f, true,  true  },
    { "Circle",          ReticleGeomCircle,                             0.00f, 1.00f, false, true  },
    { "Circle + Dot",    ReticleGeomCircle | ReticleGeomDot,            0.22f, 1.00f, false, true  },
    { "T-Shape",         ReticleGeomTShape,                             0.00f, 0.00f, true,  true  },
    { "Chevron",         ReticleGeomChevron,                            0.00f, 0.00f, true,  true  },
    { "X-Shape",         ReticleGeomDiagonal,                           0.00f, 0.00f, true,  true  },
    { "Corner Brackets", ReticleGeomBrackets,                           0.00f, 0.00f, true,  true  },
};

inline const ReticleStyleDef& ReticleStyleAt(int style)
{
    if (style < 0 || style >= ReticleStyleCount) return kReticleStyles[0];
    return kReticleStyles[style];
}

inline bool ReticleUsesGap(int style)       { return ReticleStyleAt(style).usesGap; }
inline bool ReticleUsesThickness(int style) { return ReticleStyleAt(style).usesThickness; }
inline bool ReticleHasCentreDot(int style)  { return (ReticleStyleAt(style).geometry & ReticleGeomDot) != 0u; }

// The dropdown list is generated from the table rather than written out beside
// it, so the two cannot drift apart.
inline const char* const* ReticleStyleNames()
{
    static const char* names[ReticleStyleCount];
    static bool built = false;
    if (!built)
    {
        for (int i = 0; i < ReticleStyleCount; ++i) names[i] = kReticleStyles[i].name;
        built = true;
    }
    return names;
}

// ---------------------------------------------------------------------------
// Geometry
//
// Plain floats only: no ImGui, no allocation. The overlay and the menu preview
// both consume this, so the preview cannot show something the game does not.
// ---------------------------------------------------------------------------

struct ReticleSegment { float x0, y0, x1, y1; };

struct ReticleGeometry
{
    static const int kMaxSegments = 12;
    ReticleSegment segments[kMaxSegments];
    int   segmentCount;
    float dotRadius;
    float circleRadius;
    float thickness;
};

inline ReticleGeometry BuildReticleGeometry(int style, float cx, float cy, float size,
                                           float thickness, float gap)
{
    ReticleGeometry g = {};
    g.segmentCount = 0;
    g.dotRadius = 0.0f;
    g.circleRadius = 0.0f;
    g.thickness = thickness > 0.0f ? thickness : 1.0f;

    if (!(size > 0.0f)) return g;

    const ReticleStyleDef& def = ReticleStyleAt(style);
    const float half = size * 0.5f;

    // A gap only means something for the styles that declare it, and it can
    // never grow past the arm it shortens.
    float spacing = def.usesGap ? gap : 0.0f;
    if (!(spacing > 0.0f)) spacing = 0.0f;
    const float maxSpacing = half * 0.7f;
    if (spacing > maxSpacing) spacing = maxSpacing;

    if (def.dotScale > 0.0f && (def.geometry & ReticleGeomDot))
        g.dotRadius = half * def.dotScale;
    if (def.circleScale > 0.0f && (def.geometry & ReticleGeomCircle))
        g.circleRadius = half * def.circleScale;

    if (def.geometry & ReticleGeomCross)
    {
        g.segments[g.segmentCount++] = { cx + spacing, cy, cx + half, cy };
        g.segments[g.segmentCount++] = { cx - half, cy, cx - spacing, cy };
        g.segments[g.segmentCount++] = { cx, cy + spacing, cx, cy + half };
        g.segments[g.segmentCount++] = { cx, cy - half, cx, cy - spacing };
    }
    else if (def.geometry & ReticleGeomTShape)
    {
        // Right, left and lower arms only: there is no upper arm to obscure the
        // target.
        g.segments[g.segmentCount++] = { cx + spacing, cy, cx + half, cy };
        g.segments[g.segmentCount++] = { cx - half, cy, cx - spacing, cy };
        g.segments[g.segmentCount++] = { cx, cy + spacing, cx, cy + half };
    }
    else if (def.geometry & ReticleGeomDiagonal)
    {
        const float inset = spacing * 0.7071f;
        const float reach = half * 0.7071f;
        g.segments[g.segmentCount++] = { cx + inset, cy - inset, cx + reach, cy - reach };
        g.segments[g.segmentCount++] = { cx - reach, cy - reach, cx - inset, cy - inset };
        g.segments[g.segmentCount++] = { cx - reach, cy + reach, cx - inset, cy + inset };
        g.segments[g.segmentCount++] = { cx + inset, cy + inset, cx + reach, cy + reach };
    }
    else if (def.geometry & ReticleGeomChevron)
    {
        const float apexY = cy - half + spacing;
        const float legY  = cy + half * 0.55f;
        g.segments[g.segmentCount++] = { cx, apexY, cx - half * 0.85f, legY };
        g.segments[g.segmentCount++] = { cx, apexY, cx + half * 0.85f, legY };
    }
    else if (def.geometry & ReticleGeomBrackets)
    {
        const float inset = spacing < half * 0.35f ? spacing : half * 0.35f;
        const float x0 = cx - half + inset;
        const float y0 = cy - half + inset;
        const float x1 = cx + half - inset;
        const float y1 = cy + half - inset;
        const float arm = half * 0.45f;
        g.segments[g.segmentCount++] = { x0, y0, x0 + arm, y0 };
        g.segments[g.segmentCount++] = { x0, y0, x0, y0 + arm };
        g.segments[g.segmentCount++] = { x1 - arm, y0, x1, y0 };
        g.segments[g.segmentCount++] = { x1, y0, x1, y0 + arm };
        g.segments[g.segmentCount++] = { x0, y1, x0 + arm, y1 };
        g.segments[g.segmentCount++] = { x0, y1 - arm, x0, y1 };
        g.segments[g.segmentCount++] = { x1 - arm, y1, x1, y1 };
        g.segments[g.segmentCount++] = { x1, y1 - arm, x1, y1 };
    }

    return g;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

inline ImU32 ReticleColor(float color[4])
{
    return (ImU32)ToColor(color);
}

inline void DrawReticleGeometry(ImDrawList* draw, const ReticleGeometry& g,
                                float cx, float cy, ImU32 color, float outline)
{
    if (draw == nullptr) return;

    const ImVec2 centre(cx, cy);

    if (outline > 0.0f)
    {
        // Dark pass first, slightly inflated, so the stroke stays readable over a
        // bright background.
        const ImU32 halo = IM_COL32(0, 0, 0, (int)(255.0f * 0.55f));
        if (g.dotRadius > 0.0f)
            draw->AddCircleFilled(centre, g.dotRadius + outline, halo, 24);
        if (g.circleRadius > 0.0f)
            draw->AddCircle(centre, g.circleRadius, halo, 0, g.thickness + outline * 2.0f);
        for (int i = 0; i < g.segmentCount; ++i)
            draw->AddLine(ImVec2(g.segments[i].x0, g.segments[i].y0),
                          ImVec2(g.segments[i].x1, g.segments[i].y1),
                          halo, g.thickness + outline * 2.0f);
    }

    if (g.dotRadius > 0.0f)
        draw->AddCircleFilled(centre, g.dotRadius, color, 24);
    if (g.circleRadius > 0.0f)
        draw->AddCircle(centre, g.circleRadius, color, 0, g.thickness);
    for (int i = 0; i < g.segmentCount; ++i)
        draw->AddLine(ImVec2(g.segments[i].x0, g.segments[i].y0),
                      ImVec2(g.segments[i].x1, g.segments[i].y1),
                      color, g.thickness);
}

inline float ReticleOutlineWidth()
{
    return Config.Crosshair.Outline && Config.Crosshair.OutlineThickness > 0.0f
         ? Config.Crosshair.OutlineThickness
         : 0.0f;
}

// The single renderer. The overlay and the menu preview both call this with the
// live config, which is what keeps the preview honest.
inline void DrawReticle(ImDrawList* draw, float cx, float cy)
{
    const ReticleGeometry g = BuildReticleGeometry(Config.Crosshair.Style, cx, cy,
                                                  Config.Crosshair.Size,
                                                  Config.Crosshair.Thickness,
                                                  Config.Crosshair.Gap);
    DrawReticleGeometry(draw, g, cx, cy, ReticleColor(Config.Crosshair.Color), ReticleOutlineWidth());
}

// The visual reticle reads only its own enable state. Nothing here consults the
// triggerbot, so enabling one never implies the other.
inline bool CrosshairVisible()
{
    return Config.Crosshair.Enabled && Config.Crosshair.Size > 0.0f;
}

inline void CrosshairApplyDefaults()
{
    Config.Crosshair.Enabled = false;
    Config.Crosshair.Style = ReticleDot;
    Config.Crosshair.Color[0] = 1.0f;
    Config.Crosshair.Color[1] = 1.0f;
    Config.Crosshair.Color[2] = 1.0f;
    Config.Crosshair.Color[3] = 0.95f;
    Config.Crosshair.Size = 6.0f;
    Config.Crosshair.Thickness = 1.5f;
    Config.Crosshair.Gap = 3.0f;
    Config.Crosshair.Outline = false;
    Config.Crosshair.OutlineThickness = 1.0f;
    Config.Crosshair.OffsetX = 0.0f;
    Config.Crosshair.OffsetY = 0.0f;
}

// A reticle whose centre has been pushed off the screen is not drawn at all, so
// the overlay can never paint outside the frame.
inline bool CrosshairCentreOnScreen(float& cx, float& cy, float screenW, float screenH)
{
    NewFeatures::CrosshairCentrePx(cx, cy);
    return cx >= 0.0f && cy >= 0.0f && cx <= screenW && cy <= screenH;
}

inline void DrawCrosshair(ImDrawList* draw, float screenW, float screenH)
{
    if (draw == nullptr || !CrosshairVisible()) return;

    float cx = 0.0f, cy = 0.0f;
    if (!CrosshairCentreOnScreen(cx, cy, screenW, screenH)) return;

    DrawReticle(draw, cx, cy);
}
