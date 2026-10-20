#pragma once

// ---------------------------------------------------------------------------
// Login screen - the cloudy-ui "welcome back" panel, full page.
// ---------------------------------------------------------------------------
// The reference (github.com/eliwoahzja/cloudy-ui) is a light page, not a card:
//
//   +--------------------------------------------------------------+
//   |  cloudyspoofer.cc                                          X  |   title bar
//   +--------------------------+-----------------------------------+
//   |                          |                                   |
//   |      artwork, bled       |          welcome back             |
//   |      into the panel      |    Please sign in to continue     |
//   |      and cut off by      |                                   |
//   |      its bottom edge     |    Enter your access key  ACCESS  |
//   |                          |    ______________________________  |
//   |                          |                                   |
//   |                          |                    Remember me []  |
//   |                          |   (  sign in  )                   |
//   +--------------------------+-----------------------------------+
//
// This file keeps that composition, and swaps the reference's photo for the
// artwork the app already ships: IMAGE/astral.h, the same picture the loader's
// logo is made from. It is drawn the way the reference draws its photo - scaled
// to cover the left column, standing on the panel's bottom edge, clipped by it -
// so the character bleeds off the bottom instead of sitting in a framed box.
//
// The panel fills the display (it is the whole page, not a floating card) and
// every measurement in the reference is a fraction of the panel, so it holds up
// from a 640x480 surface to a 3120x1440 one.
//
// One field, not two: the app is gated on a single licence key, so the
// reference's username half is gone and the remaining field keeps the reference
// field's geometry - hint on the left, tracked micro label on the right, hairline
// underline, eye at the end of the line.
//
// Presentation only. Nothing here calls Login(), touches the clipboard, the
// config file or the loading screen: Render() reports what the user did and
// Main.cpp keeps running the auth flow it always ran.
//
// Geometry lives in ComputeLayout() so the drawing code and the input tests
// read the same rectangles instead of re-deriving them. This is a header, not a
// translation unit, so the exact shipped code can be rendered and driven
// headlessly (ImGui/tests/, .tmp_harness/).
// ---------------------------------------------------------------------------

// Call_ImGui.h (pulled in by LoadingAnimation.h) has already defined
// IMGUI_DEFINE_MATH_OPERATORS before imgui.h, which is what imgui wants.
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_settings.h"

#include "../System/UI/LoadingAnimation.h"
#include "../Fonts/poppins.h"

namespace login_screen
{
    // -----------------------------------------------------------------------
    // Palette - the reference's light page
    // -----------------------------------------------------------------------
    // Plain constants rather than the app's purple theme: this screen is the
    // one place the reference's own light look is the point, and the values are
    // the ones picked off the screenshot.
    constexpr ImU32 kPanel   = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 kHeader  = IM_COL32(250, 251, 252, 255);
    // The artwork is a transparent PNG, so this is what shows through it: the
    // reference's photo has a white studio background, and white is what makes
    // the character sit on the page instead of in a coloured column.
    constexpr ImU32 kArtBg   = IM_COL32(255, 255, 255, 255);
    constexpr ImU32 kHair    = IM_COL32(231, 234, 239, 255);
    constexpr ImU32 kLine    = IM_COL32(212, 217, 224, 255);
    constexpr ImU32 kInk     = IM_COL32( 17,  20,  24, 255);
    constexpr ImU32 kInkSoft = IM_COL32(107, 114, 128, 255);
    constexpr ImU32 kInkMute = IM_COL32(154, 161, 172, 255);
    constexpr ImU32 kBlue    = IM_COL32( 29, 155, 240, 255);
    constexpr ImU32 kBlueDn  = IM_COL32( 21, 131, 206, 255);
    constexpr ImU32 kDanger  = IM_COL32(214,  48,  66, 255);

    // -----------------------------------------------------------------------
    // Typeface - Poppins at the reference's sizes
    // -----------------------------------------------------------------------
    struct Fonts
    {
        ImFont* micro   = nullptr;  // 12   - tracked micro label, title bar
        ImFont* body    = nullptr;  // 15   - subtitle, remember me
        ImFont* field   = nullptr;  // 16   - what InputText renders with
        ImFont* button  = nullptr;  // 17 bold - the pill label
        ImFont* heading = nullptr;  // 40 bold - "welcome back"
    };

    inline Fonts& GetFonts()
    {
        static Fonts fonts;
        return fonts;
    }

    inline void LoadFonts(ImGuiIO& io)
    {
        Fonts& f = GetFonts();

        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;   // static .rodata: the atlas must not free it
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;

        const ImWchar* ranges = io.Fonts->GetGlyphRangesDefault();

        f.micro   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 12.0f, &cfg, ranges);
        f.body    = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 15.0f, &cfg, ranges);
        f.field   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 16.0f, &cfg, ranges);
        f.button  = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    17.0f, &cfg, ranges);
        f.heading = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    40.0f, &cfg, ranges);

        // A rejected payload must never leave a null font behind: every drawing
        // path here dereferences these.
        ImFont* fallback = io.FontDefault;
        if (!fallback) fallback = io.Fonts->AddFontDefault();
        if (!f.micro)   f.micro   = fallback;
        if (!f.body)    f.body    = fallback;
        if (!f.field)   f.field   = fallback;
        if (!f.button)  f.button  = fallback;
        if (!f.heading) f.heading = fallback;
    }

    // -----------------------------------------------------------------------
    // Text helpers (the reference's drawing kit)
    // -----------------------------------------------------------------------
    inline ImVec2 TextSize(ImFont* font, float size, const char* text)
    {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    }

    inline void DrawCentered(ImDrawList* dl, ImFont* font, float size, float centerX,
                             float topY, ImU32 col, const char* text)
    {
        const ImVec2 ts = TextSize(font, size, text);
        dl->AddText(font, size, ImVec2(centerX - ts.x * 0.5f, topY), col, text);
    }

    // Letter-spaced text - the "ACCESS KEY" micro label and the title bar
    inline float TrackedWidth(ImFont* font, float size, const char* text, float tracking)
    {
        float w = 0.0f;
        for (const char* c = text; *c; ++c)
        {
            char one[2] = { *c, 0 };
            w += font->CalcTextSizeA(size, FLT_MAX, 0.0f, one).x + tracking;
        }
        return w > 0.0f ? w - tracking : 0.0f;
    }

    inline void DrawTracked(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col,
                            const char* text, float tracking)
    {
        for (const char* c = text; *c; ++c)
        {
            char one[2] = { *c, 0 };
            dl->AddText(font, size, pos, col, one);
            pos.x += font->CalcTextSizeA(size, FLT_MAX, 0.0f, one).x + tracking;
        }
    }

    inline void DrawTrackedRight(ImDrawList* dl, ImFont* font, float size, float rightX, float topY,
                                 ImU32 col, const char* text, float tracking)
    {
        DrawTracked(dl, font, size, ImVec2(rightX - TrackedWidth(font, size, text, tracking), topY),
                    col, text, tracking);
    }

    // -----------------------------------------------------------------------
    // Procedural icons (no icon font: the reference draws these with ImDrawList)
    // -----------------------------------------------------------------------
    inline void DrawEye(ImDrawList* dl, ImVec2 center, float r, ImU32 col, bool open, float s)
    {
        dl->AddEllipse(center, r, r * 0.60f, col, 0.0f, 28, 1.3f * s);
        dl->AddCircleFilled(center, r * 0.30f, col, 14);
        if (!open)
            dl->AddLine(ImVec2(center.x - r * 0.95f, center.y + r * 0.62f),
                        ImVec2(center.x + r * 0.95f, center.y - r * 0.62f), col, 1.4f * s);
    }

    inline void DrawClose(ImDrawList* dl, ImVec2 center, float r, ImU32 col, float thick)
    {
        dl->AddLine(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), col, thick);
        dl->AddLine(ImVec2(center.x + r, center.y - r), ImVec2(center.x - r, center.y + r), col, thick);
    }

    inline void DrawImageGlyph(ImDrawList* dl, ImVec2 center, float r, ImU32 col, float s)
    {
        dl->AddRect(ImVec2(center.x - r, center.y - r * 0.78f), ImVec2(center.x + r, center.y + r * 0.78f),
                    col, r * 0.22f, 0, 1.6f * s);
        dl->AddCircleFilled(ImVec2(center.x - r * 0.38f, center.y - r * 0.34f), r * 0.16f, col, 12);
        dl->AddLine(ImVec2(center.x - r * 0.72f, center.y + r * 0.52f),
                    ImVec2(center.x - r * 0.06f, center.y - r * 0.10f), col, 1.6f * s);
        dl->AddLine(ImVec2(center.x - r * 0.06f, center.y - r * 0.10f),
                    ImVec2(center.x + r * 0.72f, center.y + r * 0.52f), col, 1.6f * s);
    }

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------
    // `o`, `W`, `H` are the window the panel is drawn in. On the device that is
    // the whole render surface: the panel is the page, inset by a hairline so
    // its rounded corner reads as a page edge rather than as a floating card.
    // Every number below is the reference's own measurement, expressed as a
    // share of the panel so it scales with the surface.
    struct Layout
    {
        float  s = 1.0f;

        ImVec2 cardMin, cardMax;       // the panel itself
        float  cardRound = 12.0f;
        float  headerH   = 0.0f;       // the title bar
        ImVec2 closeMin, closeMax;     // the X on the right of the title bar
        float  brandX    = 0.0f;       // left edge of the domain text

        float  artR = 0.0f;            // right edge of the artwork column
        ImVec2 wellMin, wellMax;       // the artwork column (header bottom -> panel bottom)

        float  fieldL = 0.0f, fieldR = 0.0f, fieldW = 0.0f, cx = 0.0f;
        float  textCY = 0.0f;          // centre of the field's text line
        float  fieldBot = 0.0f;        // the hairline underline
        float  inputTop = 0.0f;        // top of the InputText hit box
        float  eyeSize = 0.0f;

        float  headY = 0.0f, subY = 0.0f;
        ImVec2 pillMin, pillMax;
        ImVec2 pasteMin, pasteMax;
        ImVec2 rememberMin, rememberMax;
        ImVec2 errorMin, errorMax;

        // Centre of the InputText hit box - what a test taps to focus the field.
        ImVec2 fieldHitCenter() const { return ImVec2(fieldL + fieldW * 0.35f, inputTop + 16.0f); }
        ImVec2 eyeCenter() const           { return ImVec2(fieldR - eyeSize * 0.5f, textCY); }
        ImVec2 pillCenter() const          { return ImVec2((pillMin.x + pillMax.x) * 0.5f, (pillMin.y + pillMax.y) * 0.5f); }
        ImVec2 pasteCenter() const         { return ImVec2((pasteMin.x + pasteMax.x) * 0.5f, (pasteMin.y + pasteMax.y) * 0.5f); }
        ImVec2 rememberCenter() const      { return ImVec2((rememberMin.x + rememberMax.x) * 0.5f, (rememberMin.y + rememberMax.y) * 0.5f); }
        ImVec2 closeCenter() const         { return ImVec2((closeMin.x + closeMax.x) * 0.5f, (closeMin.y + closeMax.y) * 0.5f); }
    };

    inline Layout ComputeLayout(ImVec2 o, float W, float H, bool hasError)
    {
        Layout L;

        // The page, not a card: the whole surface less a hairline margin.
        const float pad = ImClamp(ImMin(W, H) * 0.022f, 6.0f, 22.0f);
        L.cardMin = ImVec2(o.x + pad,         o.y + pad);
        L.cardMax = ImVec2(o.x + W - pad,     o.y + H - pad);

        const float pw = ImMax(1.0f, L.cardMax.x - L.cardMin.x);
        const float ph = ImMax(1.0f, L.cardMax.y - L.cardMin.y);

        // The reference's panel is 904x578. Everything scales off the smaller of
        // its two ratios, so the type keeps the reference's proportions on any
        // surface and a portrait one does not end up with a poster for a heading.
        // Clamped so a small surface still has readable type.
        const float s = ImClamp(ImMin(ph / 578.0f, pw / 904.0f), 0.60f, 2.20f);
        L.s = s;
        L.cardRound = ImClamp(12.0f * s, 8.0f, 20.0f);
        L.headerH   = ImClamp(ph * 0.107f, 34.0f, 78.0f);

        // The title bar: domain on the left, close X on the right.
        L.brandX = L.cardMin.x + ImMax(pw * 0.031f, 14.0f * s);
        const float closeBox = ImClamp(34.0f * s, 26.0f, 44.0f);
        const float closeCX  = L.cardMax.x - ImMax(pw * 0.028f, 14.0f * s) - closeBox * 0.5f;
        L.closeMin = ImVec2(closeCX - closeBox * 0.5f, L.cardMin.y + (L.headerH - closeBox) * 0.5f);
        L.closeMax = ImVec2(L.closeMin.x + closeBox, L.closeMin.y + closeBox);

        // Artwork column: below the title bar, down the panel's bottom edge, to
        // just short of half its width. On a surface taller than it is wide a
        // 47% column would be a sliver, so it narrows there.
        L.artR = L.cardMin.x + pw * (pw >= ph ? 0.469f : 0.42f);
        L.wellMin = ImVec2(L.cardMin.x, L.cardMin.y + L.headerH);
        L.wellMax = ImVec2(L.artR, L.cardMax.y);

        // The form column.
        const float artW = L.artR - L.cardMin.x;
        (void)artW;
        L.fieldL = L.artR + ImMax(pw * 0.038f, 16.0f * s);
        L.fieldR = L.cardMax.x - ImMax(pw * 0.055f, 18.0f * s);
        if (L.fieldR - L.fieldL < 90.0f * s)
        {
            // A surface narrow enough that the reference's insets would collapse
            // the field: fall back to a symmetric gutter.
            const float g = ImMax(10.0f * s, pw * 0.02f);
            L.fieldL = o.x + g + pw * 0.44f;
            L.fieldR = L.cardMax.x - g;
        }
        L.fieldW = ImMax(60.0f, L.fieldR - L.fieldL);
        L.cx = (L.fieldL + L.fieldR) * 0.5f;

        Fonts& F = GetFonts();
        ImFont* heading = F.heading ? F.heading : ImGui::GetFont();
        ImFont* body = F.body ? F.body : heading;

        // Heading + subtitle, centred over the form column (reference: 0.204 and
        // just under it).
        L.headY = L.cardMin.y + ph * 0.192f;
        const float headH = TextSize(heading, 40.0f * s, "welcome back").y;
        L.subY = L.headY + headH + 6.0f * s;
        const float subH = TextSize(body, 15.0f * s, "Please sign in to continue").y;

        // The one field sits between the subtitle and the remember row, where the
        // reference's two fields were - same air above it, same gap below.
        L.textCY = L.cardMin.y + ph * 0.575f;
        L.fieldBot = L.textCY + 12.0f * s;
        L.eyeSize = ImClamp(30.0f * s, 24.0f, 42.0f);

        // The app's InputText centres its text 16 px below the top of its frame
        // (ImGui/imgui_widgets.cpp: (32 - textHeight) / 2 + textHeight / 2), a
        // constant that does not scale, so the hit box follows from that.
        L.inputTop = L.textCY - 16.0f;

        // Bottom block, straight from the reference: pill 14.7% off the bottom,
        // checkbox on the line above it.
        const float pillH = ImClamp(ph * 0.107f, 34.0f, 74.0f);
        L.pillMin = ImVec2(L.fieldL, L.cardMin.y + ph * 0.746f);
        L.pillMax = ImVec2(L.fieldR, L.pillMin.y + pillH);

        const float box = ImClamp(ph * 0.036f, 14.0f, 26.0f);
        L.rememberMax = ImVec2(L.fieldR, L.pillMin.y - ImMax(9.0f * s, 6.0f));
        L.rememberMin = ImVec2(L.rememberMax.x - box, L.rememberMax.y - box);

        // The clipboard shortcut the phone needs, hung under the pill as a text
        // link (the reference has nothing there, but a phone cannot paste a
        // licence key without it).
        const float pasteH = ImClamp(28.0f * s, 22.0f, 38.0f);
        L.pasteMin = ImVec2(L.fieldL, L.pillMax.y + ImMax(7.0f * s, 4.0f));
        L.pasteMax = ImVec2(L.fieldR, L.pasteMin.y + pasteH);

        if (hasError)
        {
            L.errorMin = ImVec2(L.fieldL, L.fieldBot + 10.0f * s);
            L.errorMax = ImVec2(L.fieldR, L.errorMin.y + ImMax(18.0f * s, 14.0f));
        }

        (void)subH;
        return L;
    }

    // -----------------------------------------------------------------------
    // Field skin
    // -----------------------------------------------------------------------
    // The app's InputText paints its own frame from the *global* c:: palette
    // (ImGui/imgui_widgets.cpp: GetColorU32(c::elements::background) for the
    // frame and c::text::* for hint / typed text / caret), so a light underline
    // field on a white panel cannot be made with pushed style colours alone.
    // Swap those globals for the call and put them back; the theme pass rewrites
    // them at the top of every frame anyway.
    struct FieldSkin
    {
        ImVec4 frameBg, text, textHovered, textActive;

        explicit FieldSkin(float padY)
        {
            frameBg     = c::elements::background;
            text        = c::text::text;
            textHovered = c::text::text_hov;
            textActive  = c::text::text_active;

            c::elements::background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);   // the panel draws the underline
            c::text::text           = ImVec4(0.604f, 0.631f, 0.675f, 1.0f);  // hint
            c::text::text_hov       = ImVec4(0.604f, 0.631f, 0.675f, 1.0f);
            c::text::text_active    = ImVec4(0.067f, 0.078f, 0.094f, 1.0f);  // typed key + caret

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, padY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }

        ~FieldSkin()
        {
            c::elements::background = frameBg;
            c::text::text           = text;
            c::text::text_hov       = textHovered;
            c::text::text_active    = textActive;
            ImGui::PopStyleVar(2);
        }
    };

    // -----------------------------------------------------------------------
    // Render
    // -----------------------------------------------------------------------
    struct Params
    {
        ImVec2 pos;             // window top-left, in screen space
        float  W = 1280.0f;     // the surface the panel is drawn in
        float  H = 720.0f;

        char*  key = nullptr;   // the access key buffer (unchanged existing state)
        int    keySize = 0;
        bool*  reveal = nullptr;
        bool*  remember = nullptr;

        const char* error = nullptr;   // null or empty when there is nothing to say
        const char* brand = nullptr;   // title bar text; defaults to "ethnir.cc"

        ImTextureID art = 0;   // the artwork texture (IMAGE/astral.h)
        int    artW = 0;
        int    artH = 0;
    };

    struct Result
    {
        bool authenticate = false;   // pill tapped, or Enter in the field
        bool paste = false;          // "paste from clipboard" tapped
        bool close = false;          // the title bar's X tapped
    };

    inline Result Render(const Params& p)
    {
        Result out;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 o = p.pos;
        const float W = p.W, H = p.H;

        const bool hasError = p.error && p.error[0] != '\0' &&
                              !(p.error[0] == 'O' && p.error[1] == 'K' && p.error[2] == '\0');
        const Layout L = ComputeLayout(o, W, H, hasError);
        const float s = L.s;

        Fonts& F = GetFonts();
        ImFont* fMicro   = F.micro   ? F.micro   : ImGui::GetFont();
        ImFont* fBody    = F.body    ? F.body    : fMicro;
        ImFont* fField   = F.field   ? F.field   : fBody;
        ImFont* fButton  = F.button  ? F.button  : fBody;
        ImFont* fHeading = F.heading ? F.heading : fBody;

        const char* brand = (p.brand && p.brand[0]) ? p.brand : "ethnir.cc";

        // ---- the panel -----------------------------------------------------
        dl->AddRectFilled(L.cardMin, L.cardMax, kPanel, L.cardRound);
        dl->AddRect(L.cardMin, L.cardMax, IM_COL32(0, 0, 0, 20), L.cardRound, 0, 1.0f);

        // ---- title bar ------------------------------------------------------
        {
            const ImVec2 barMax(L.cardMax.x, L.cardMin.y + L.headerH);
            dl->AddRectFilled(L.cardMin, barMax, kHeader, L.cardRound, ImDrawFlags_RoundCornersTop);
            dl->AddLine(ImVec2(L.cardMin.x, barMax.y), ImVec2(L.cardMax.x, barMax.y), kHair, 1.0f);

            const float brandY = L.cardMin.y + (L.headerH - TextSize(fMicro, 12.0f * s, brand).y) * 0.5f;
            dl->AddText(fMicro, ImClamp(12.0f * s, 10.0f, 17.0f),
                        ImVec2(L.brandX, brandY), kInkSoft, brand);

            // Close: the reference's X. It folds the menu away (Main.cpp keeps
            // the collapse bar, so it can always be brought back).
            ImGui::SetCursorScreenPos(L.closeMin);
            const bool tapped = ImGui::InvisibleButton("##login_close",
                                                       ImVec2(L.closeMax.x - L.closeMin.x,
                                                              L.closeMax.y - L.closeMin.y));
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const ImVec2 cc = L.closeCenter();
            const float cr = ImMin(7.0f * s, (L.closeMax.x - L.closeMin.x) * 0.24f);
            if (hover)
                dl->AddCircleFilled(cc, cr * 1.9f, IM_COL32(0, 0, 0, 10));
            DrawClose(dl, cc, cr, hover ? kInk : kInkSoft, ImMax(1.4f * s, 1.0f));
            if (tapped)
                out.close = true;
        }

        // ---- artwork --------------------------------------------------------
        // The reference lets its photo run to the panel's own bottom-left
        // corner. This is the same picture the loader uses (IMAGE/astral.h, a
        // transparent PNG), scaled to cover the column and stood on the bottom
        // edge so the character is cut off by it - not boxed, not floating.
        dl->AddRectFilled(L.wellMin, L.wellMax, kArtBg, L.cardRound, ImDrawFlags_RoundCornersBottomLeft);

        if (p.art && p.artW > 0 && p.artH > 0)
        {
            const float iw = (float)p.artW, ih = (float)p.artH;
            const float colW = L.wellMax.x - L.wellMin.x;
            const float colH = L.wellMax.y - L.wellMin.y;

            // Cover, but never zoom past 2.2x the width fit: on a very tall
            // column (a portrait surface) a pure cover would crop the artwork
            // down to one shoulder.
            const float kFill  = colW / iw;
            const float kCover = ImMax(kFill, colH / ih);
            const float k      = ImMin(kCover, kFill * 2.2f);

            const float dw = iw * k, dh = ih * k;
            // Anchored on the character (she sits slightly right of centre in
            // the source), so the sides crop evenly instead of lopping an arm
            // off at the panel edge.
            const float x = (L.wellMin.x + L.wellMax.x) * 0.5f - 0.53f * dw;
            const float y = L.wellMax.y - dh;

            const float dx0 = ImMax(L.wellMin.x, x);
            const float dx1 = ImMin(L.wellMax.x, x + dw);
            const float dy0 = ImMax(L.wellMin.y, y);
            const float dy1 = ImMin(L.wellMax.y, y + dh);

            if (dx1 - dx0 > 1.0f && dy1 - dy0 > 1.0f)
            {
                const ImVec2 uv0((dx0 - x) / dw, (dy0 - y) / dh);
                const ImVec2 uv1((dx1 - x) / dw, (dy1 - y) / dh);
                // Only the corner the panel actually rounds is rounded here.
                const bool atCorner = (dx0 <= L.wellMin.x + 0.5f && dy1 >= L.wellMax.y - 0.5f);
                dl->AddImageRounded(p.art, ImVec2(dx0, dy0), ImVec2(dx1, dy1), uv0, uv1,
                                    IM_COL32_WHITE, atCorner ? L.cardRound : 0.0f,
                                    atCorner ? ImDrawFlags_RoundCornersBottomLeft : 0);
            }
        }
        else
        {
            // Placeholder until the artwork texture is up (or if it failed).
            const ImVec2 c((L.wellMin.x + L.wellMax.x) * 0.5f, (L.wellMin.y + L.wellMax.y) * 0.5f);
            DrawImageGlyph(dl, ImVec2(c.x, c.y - 16.0f * s), 22.0f * s, kInkMute, s);
            DrawCentered(dl, fMicro, 12.0f * s, c.x, c.y + 20.0f * s, kInkMute, "NO ARTWORK");
        }

        // ---- heading --------------------------------------------------------
        DrawCentered(dl, fHeading, 40.0f * s, L.cx, L.headY, kInk, "welcome back");
        DrawCentered(dl, fBody, 15.0f * s, L.cx, L.subY, kInkSoft, "Please sign in to continue");

        // The card's own text is drawn at explicit sizes multiplied by s, but
        // InputText renders with g.FontSize, i.e. the pushed font's face size
        // times the window's font scale. Scaling the window scales the widget's
        // text with the panel; everything else here passes explicit sizes and is
        // unaffected. The window is the login screen and nothing else.
        ImGui::SetWindowFontScale(s);

        const float padY = ImMax(6.0f * s, 4.0f);

        // ---- reveal eye -----------------------------------------------------
        const bool reveal = p.reveal ? *p.reveal : false;
        {
            const ImVec2 eyeMin(L.fieldR - L.eyeSize, L.textCY - L.eyeSize * 0.5f);
            ImGui::SetCursorScreenPos(eyeMin);
            const bool tapped = ImGui::InvisibleButton("##login_reveal", ImVec2(L.eyeSize, L.eyeSize));
            if (tapped && p.reveal) *p.reveal = !*p.reveal;
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            DrawEye(dl, ImVec2(eyeMin.x + L.eyeSize * 0.5f, eyeMin.y + L.eyeSize * 0.5f),
                    11.0f * s, hover ? kInk : kInkMute, reveal, s);
        }

        // ---- the access key field -------------------------------------------
        {
            ImGui::SetCursorScreenPos(ImVec2(L.fieldL, L.inputTop));
            ImGui::SetNextItemWidth(L.fieldW - L.eyeSize - 6.0f * s);
            ImGui::PushFont(fField);
            // The label must be empty, not "##id": this InputText paints its
            // label with a raw AddText(), which does not strip the ## marker.
            ImGui::PushID("login_access_key");
            {
                FieldSkin skin(padY);
                // EnterReturnsTrue is also how Enter submits: by the time the
                // widget returns it has already dropped focus, so polling
                // IsItemActive() afterwards would never see the key press.
                ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsNoBlank |
                                            ImGuiInputTextFlags_EnterReturnsTrue;
                if (!reveal) flags |= ImGuiInputTextFlags_Password;
                if (ImGui::InputTextWithHint("", "Enter your access key",
                                             p.key, (size_t)p.keySize, flags))
                    out.authenticate = true;
            }
            ImGui::PopID();
            const bool focused = ImGui::IsItemActive();
            const bool hovered = ImGui::IsItemHovered();
            ImGui::PopFont();

            // The underline: grey until something happens to the field, then blue.
            const ImU32 lineCol = focused ? kBlue : (hovered ? ImU32(IM_COL32(150, 160, 172, 255)) : kLine);
            dl->AddLine(ImVec2(L.fieldL, L.fieldBot), ImVec2(L.fieldR, L.fieldBot),
                        lineCol, (focused ? 1.8f : 1.2f) * ImMax(s, 0.8f));

            // Tracked micro label, right aligned above the line - the reference
            // puts it on the line, but the eye already lives there.
            const float labelSize = ImClamp(11.0f * s, 9.0f, 16.0f);
            const float labelH = TextSize(fMicro, labelSize, "ACCESS KEY").y;
            DrawTrackedRight(dl, fMicro, labelSize, L.fieldR,
                             L.textCY - ImMax(20.0f * s, 14.0f) - labelH,
                             focused ? kInkSoft : kInkMute, "ACCESS KEY", 0.9f * s);
        }

        // ---- error ----------------------------------------------------------
        if (hasError)
        {
            // One line only: the server's reply is JSON-shaped and can be long,
            // and this row sits 30 px above the remember box.
            char line[160];
            int n = 0;
            for (const char* c = p.error; *c && n < (int)sizeof(line) - 1; ++c)
                line[n++] = (*c == '\n' || *c == '\r') ? ' ' : *c;
            line[n] = '\0';

            dl->AddRectFilled(L.errorMin, L.errorMax, IM_COL32(255, 240, 242, 255), 6.0f * s);
            dl->AddRect(L.errorMin, L.errorMax, IM_COL32(246, 196, 202, 255), 6.0f * s, 0, 1.0f);
            dl->AddText(fMicro, ImClamp(11.0f * s, 9.0f, 15.0f),
                        ImVec2(L.errorMin.x + 8.0f * s, (L.errorMin.y + L.errorMax.y) * 0.5f -
                                                       TextSize(fMicro, 11.0f * s, line).y * 0.5f),
                        kDanger, line);
        }

        // ---- remember me ----------------------------------------------------
        if (p.remember)
        {
            const float box = L.rememberMax.x - L.rememberMin.x;
            const char* label = "Remember me";
            const float labelSize = ImClamp(14.0f * s, 11.0f, 20.0f);
            const ImVec2 ls = TextSize(fBody, labelSize, label);

            dl->AddText(fBody, labelSize,
                        ImVec2(L.rememberMin.x - ImMax(10.0f * s, 7.0f) - ls.x,
                               L.rememberMin.y + (box - ls.y) * 0.5f - 1.0f * s), kInkSoft, label);

            ImGui::SetCursorScreenPos(L.rememberMin);
            const bool tapped = ImGui::InvisibleButton("##login_remember", ImVec2(box, box));
            if (tapped) *p.remember = !*p.remember;
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            if (*p.remember)
            {
                dl->AddRectFilled(L.rememberMin, L.rememberMax, hover ? kBlueDn : kBlue, box * 0.26f);
                dl->AddLine(ImVec2(L.rememberMin.x + box * 0.24f, L.rememberMin.y + box * 0.53f),
                            ImVec2(L.rememberMin.x + box * 0.44f, L.rememberMin.y + box * 0.72f),
                            IM_COL32(255, 255, 255, 255), 1.8f * s);
                dl->AddLine(ImVec2(L.rememberMin.x + box * 0.44f, L.rememberMin.y + box * 0.72f),
                            ImVec2(L.rememberMin.x + box * 0.78f, L.rememberMin.y + box * 0.29f),
                            IM_COL32(255, 255, 255, 255), 1.8f * s);
            }
            else
            {
                dl->AddRectFilled(L.rememberMin, L.rememberMax, kPanel, box * 0.26f);
                dl->AddRect(L.rememberMin, L.rememberMax, hover ? kInkSoft : kLine, box * 0.26f, 0, 1.4f);
            }
        }

        // ---- sign in --------------------------------------------------------
        {
            const float round = (L.pillMax.y - L.pillMin.y) * 0.5f;
            ImGui::SetCursorScreenPos(L.pillMin);
            const bool clicked = ImGui::InvisibleButton("##login_authenticate",
                                                        ImVec2(L.fieldW, L.pillMax.y - L.pillMin.y));
            const bool hover = ImGui::IsItemHovered();
            const bool active = ImGui::IsItemActive();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            dl->AddRectFilled(L.pillMin, L.pillMax, active ? kBlueDn : (hover ? kBlueDn : kBlue), round);

            const float buttonSize = ImClamp(17.0f * s, 13.0f, 24.0f);
            const ImVec2 ts = TextSize(fButton, buttonSize, "sign in");
            dl->AddText(fButton, buttonSize,
                        ImVec2(L.pillMin.x + (L.fieldW - ts.x) * 0.5f,
                               L.pillMin.y + ((L.pillMax.y - L.pillMin.y) - ts.y) * 0.5f - 1.0f * s),
                        IM_COL32(255, 255, 255, 255), "sign in");

            if (clicked)
                out.authenticate = true;
        }

        // ---- paste ----------------------------------------------------------
        {
            const float pasteH = L.pasteMax.y - L.pasteMin.y;

            ImGui::SetCursorScreenPos(L.pasteMin);
            const bool clicked = ImGui::InvisibleButton("##login_paste", ImVec2(L.fieldW, pasteH));
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            const char* label = "PASTE FROM CLIPBOARD";
            const float labelSize = ImClamp(10.5f * s, 9.0f, 15.0f);
            const float tracking = 1.0f * s;
            const float lw = TrackedWidth(fMicro, labelSize, label, tracking);
            const ImVec2 lsz = TextSize(fMicro, labelSize, label);
            DrawTracked(dl, fMicro, labelSize,
                        ImVec2(L.pasteMin.x + (L.fieldW - lw) * 0.5f,
                               L.pasteMin.y + (pasteH - lsz.y) * 0.5f - 1.0f * s),
                        hover ? kBlue : kInkMute, label, tracking);

            if (clicked)
                out.paste = true;
        }

        return out;
    }
}
