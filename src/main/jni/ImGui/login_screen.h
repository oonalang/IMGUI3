#pragma once

// ---------------------------------------------------------------------------
// Login card - the cloudy-ui "welcome back" screen, in this app's dark purple.
// ---------------------------------------------------------------------------
// The reference (github.com/eliwoahzja/cloudy-ui, DrawLoginPage in main.cpp) is
// a light card with artwork on the left and a form on the right: centred heading
// + subtitle, big underline fields with letter-spaced micro labels, an eye that
// reveals the password, a remember-me checkbox, and a full-width pill parked a
// tenth of the card off the bottom. This file keeps that geometry - same
// fractions of the card, same order, same tracking - and swaps the light tokens
// for the loader's purple palette (System/UI/LoadingAnimation.h, ui_loading::C).
//
// One field, not two: the card is the app's licence gate, so the username half
// is gone and the remaining field is the access key. It sits centred in the
// space the reference's two fields occupied, so the checkbox and the pill keep
// their reference positions below it.
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
    // Typeface - Poppins at the reference's sizes
    // -----------------------------------------------------------------------
    struct Fonts
    {
        ImFont* micro   = nullptr;  // 13 - tracked micro labels, footer, error
        ImFont* body    = nullptr;  // 18 - subtitle
        ImFont* input   = nullptr;  // 20 - the access key itself
        ImFont* button  = nullptr;  // 21 bold - the pill label
        ImFont* heading = nullptr;  // 43 bold - "welcome back"
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

        f.micro   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 13.0f, &cfg, ranges);
        f.body    = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 18.0f, &cfg, ranges);
        f.input   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 20.0f, &cfg, ranges);
        f.button  = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    21.0f, &cfg, ranges);
        f.heading = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    43.0f, &cfg, ranges);

        // A rejected payload must never leave a null font behind: every drawing
        // path here dereferences these.
        ImFont* fallback = io.FontDefault;
        if (!fallback) fallback = io.Fonts->AddFontDefault();
        if (!f.micro)   f.micro   = fallback;
        if (!f.body)    f.body    = fallback;
        if (!f.input)   f.input   = fallback;
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

    // Letter-spaced text - the "ACCESS KEY" micro label
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
        dl->AddEllipse(center, r, r * 0.60f, col, 0.0f, 28, 1.4f * s);
        dl->AddCircleFilled(center, r * 0.30f, col, 14);
        if (!open)
            dl->AddLine(ImVec2(center.x - r * 0.95f, center.y + r * 0.62f),
                        ImVec2(center.x + r * 0.95f, center.y - r * 0.62f), col, 1.5f * s);
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
    // The card is the menu, capped at the reference's 900x592, so fixed pixel
    // values from the reference are scaled by whatever share of that we got
    // while the big ones stay fractions of the card.
    struct Layout
    {
        float  s = 1.0f;
        ImVec2 cardMin, cardMax;
        float  cardRound = 20.0f;

        float  artR = 0.0f;            // width of the artwork column
        ImVec2 wellMin, wellMax;       // the rounded well the artwork sits in

        float  fieldL = 0.0f, fieldR = 0.0f, fieldW = 0.0f, cx = 0.0f;
        float  fieldTop = 0.0f, fieldBot = 0.0f, fieldH = 0.0f;
        float  textCY = 0.0f;          // centre of the field's text line
        float  inputTop = 0.0f;        // top of the InputText's 32 px hit box
        float  eyeSize = 0.0f;

        float  headY = 0.0f, subY = 0.0f;
        ImVec2 pillMin, pillMax;
        ImVec2 pasteMin, pasteMax;
        ImVec2 rememberMin, rememberMax;
        ImVec2 errorMin, errorMax;
        float  footerY = 0.0f;

        // Centre of the InputText hit box - what a test taps to focus the field.
        ImVec2 fieldHitCenter() const { return ImVec2(fieldL + fieldW * 0.35f, inputTop + 16.0f); }
        ImVec2 eyeCenter() const           { return ImVec2(fieldR - eyeSize * 0.5f, textCY); }
        ImVec2 pillCenter() const          { return ImVec2((pillMin.x + pillMax.x) * 0.5f, (pillMin.y + pillMax.y) * 0.5f); }
        ImVec2 pasteCenter() const         { return ImVec2((pasteMin.x + pasteMax.x) * 0.5f, (pasteMin.y + pasteMax.y) * 0.5f); }
        ImVec2 rememberCenter() const      { return ImVec2((rememberMin.x + rememberMax.x) * 0.5f, (rememberMin.y + rememberMax.y) * 0.5f); }
    };

    inline Layout ComputeLayout(ImVec2 o, float W, float H, bool hasError)
    {
        Layout L;
        const float s = ImMin(W / 900.0f, H / 592.0f);
        L.s = s;

        L.cardMin = o;
        L.cardMax = ImVec2(o.x + W, o.y + H);
        L.cardRound = 20.0f * s;

        L.artR = W * 0.48f;
        const float edge = 16.0f * s;
        L.wellMin = ImVec2(o.x + edge, o.y + edge);
        L.wellMax = ImVec2(o.x + L.artR - edge * 0.6f, o.y + H - edge);

        // The form owns everything right of the artwork.
        L.fieldL = o.x + L.artR + 22.0f * s;
        L.fieldR = o.x + W - 0.058f * W;
        L.fieldW = L.fieldR - L.fieldL;
        L.cx = (L.fieldL + L.fieldR) * 0.5f;

        Fonts& F = GetFonts();
        ImFont* heading = F.heading ? F.heading : ImGui::GetFont();
        ImFont* body = F.body ? F.body : heading;

        // Heading + subtitle, centred over the form column.
        L.headY = o.y + H * 0.127f;
        const float headH = TextSize(heading, 43.0f * s, "welcome back").y;
        L.subY = L.headY + headH + 10.0f * s;
        const float subBottom = L.subY + TextSize(body, 18.0f * s, "Please sign in to continue").y;

        // Bottom block, straight from the reference: pill at 13.8% off the
        // bottom, checkbox at 70.3% down, paste row hung under the pill.
        const float pillH = 0.120f * H;
        L.pillMin = ImVec2(L.fieldL, o.y + H - 0.138f * H - pillH);
        L.pillMax = ImVec2(L.fieldR, L.pillMin.y + pillH);

        const float box = 22.0f * s;
        L.rememberMin = ImVec2(L.fieldR - box, o.y + H * 0.703f);
        L.rememberMax = ImVec2(L.rememberMin.x + box, L.rememberMin.y + box);

        // The single access-key field is centred between the subtitle and the
        // checkbox instead of the reference's two-field stack.
        L.fieldH = 0.117f * H;
        L.fieldTop = ImMax(o.y + subBottom + 24.0f * s,
                           (subBottom + L.rememberMin.y) * 0.5f - L.fieldH * 0.5f);
        L.fieldBot = L.fieldTop + L.fieldH;
        L.textCY = L.fieldBot - 18.0f * s;
        L.eyeSize = 30.0f * s;

        // The app's InputText draws a fixed 32 px frame (ImGui/imgui_widgets.cpp).
        const float lineH = F.input ? F.input->FontSize : 20.0f;
        L.inputTop = L.textCY - lineH * 0.5f - 7.0f * s;

        const float pasteH = 30.0f * s;
        L.pasteMin = ImVec2(L.fieldL, L.pillMax.y + 10.0f * s);
        L.pasteMax = ImVec2(L.fieldR, L.pasteMin.y + pasteH);

        L.footerY = o.y + H - 24.0f * s;

        if (hasError)
        {
            L.errorMin = ImVec2(L.fieldL, L.fieldBot + 14.0f * s);
            L.errorMax = ImVec2(L.fieldR, L.errorMin.y + 30.0f * s);
        }
        return L;
    }

    // -----------------------------------------------------------------------
    // Field skin
    // -----------------------------------------------------------------------
    // The app's InputText paints its own frame from the *global* c:: palette
    // (ImGui/imgui_widgets.cpp: GetColorU32(c::elements::background) and
    // c::text::*), so an underline field cannot be made with pushed style
    // colours alone. Swap those globals for the call and put them back; the
    // theme pass rewrites them at the top of every frame anyway.
    struct FieldSkin
    {
        ImVec4 frameBg, text, textHovered, textActive;

        explicit FieldSkin(float padY)
        {
            frameBg     = c::elements::background;
            text        = c::text::text;
            textHovered = c::text::text_hov;
            textActive  = c::text::text_active;

            c::elements::background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);   // the card draws the underline
            c::text::text           = ui_loading::C::TextDim;           // hint
            c::text::text_hov       = ui_loading::C::TextDim;
            c::text::text_active    = ui_loading::C::White;             // typed key + caret

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
        ImVec2 pos;             // card top-left, in screen space
        float  W = 900.0f;      // card size (menu size - Main.cpp passes the menu)
        float  H = 592.0f;

        char*  key = nullptr;   // the access key buffer (unchanged existing state)
        int    keySize = 0;
        bool*  reveal = nullptr;
        bool*  remember = nullptr;

        const char* error = nullptr;   // null or empty when there is nothing to say

        ImTextureID art = 0;   // runtime_preview_menu::g_menuBackground
        int    artW = 0;
        int    artH = 0;
    };

    struct Result
    {
        bool authenticate = false;   // pill tapped, or Enter in the field
        bool paste = false;          // "paste from clipboard" tapped
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
        ImFont* fInput   = F.input   ? F.input   : fBody;
        ImFont* fButton  = F.button  ? F.button  : fBody;
        ImFont* fHeading = F.heading ? F.heading : fBody;

        // Dark purple tokens from the loader's palette.
        const ImU32 kPanel  = ui_loading::C::U(ui_loading::C::BgPanel, 0.55f);
        const ImU32 kText   = ui_loading::C::U(ui_loading::C::White);
        const ImU32 kSoft   = ui_loading::C::UA(ui_loading::C::TextDim, 0.95f);
        const ImU32 kLabel  = ui_loading::C::UA(ui_loading::C::TextDim, 0.80f);
        const ImU32 kLine   = ui_loading::C::UA(ui_loading::C::Border, 0.45f);
        const ImU32 kBorder = ui_loading::C::UA(ui_loading::C::Border, 0.85f);
        const ImU32 kAccent = ui_loading::C::U(ui_loading::C::Purple);
        const ImU32 kBrt    = ui_loading::C::U(ui_loading::C::PurpleBrt);
        const ImU32 kDeep   = ui_loading::C::U(ui_loading::C::PurpleDeep);
        const ImU32 kDanger = IM_COL32(255, 104, 126, 255);

        // ---- card + frame -------------------------------------------------
        dl->AddRectFilled(L.cardMin, L.cardMax, ui_loading::C::U(ui_loading::C::BgDeep, 0.97f), L.cardRound);
        dl->AddRectFilledMultiColor(L.cardMin, L.cardMax,
            IM_COL32(166, 89, 255, 30), IM_COL32(166, 89, 255, 10),
            IM_COL32(10, 2, 20, 0), IM_COL32(10, 2, 20, 0), L.cardRound);
        dl->AddRect(L.cardMin, L.cardMax, kBorder, L.cardRound, 0, 1.4f * s);
        dl->AddRect(ImVec2(o.x + 5.0f * s, o.y + 5.0f * s),
                    ImVec2(L.cardMax.x - 5.0f * s, L.cardMax.y - 5.0f * s),
                    IM_COL32(166, 89, 255, 26), L.cardRound - 5.0f * s, 0, 1.0f * s);

        // ---- artwork ------------------------------------------------------
        // The reference lets the art bleed into the card edge. On a dark card
        // the photo's own square corners then cut across the card's rounded
        // ones, so it is framed instead: same contain / nudge-right placement,
        // inside a rounded well that belongs to this theme.
        dl->AddRectFilled(o, ImVec2(o.x + L.artR, o.y + H), kPanel, L.cardRound,
                          ImDrawFlags_RoundCornersLeft);

        if (p.art && p.artW > 0 && p.artH > 0)
        {
            // The well hugs the picture instead of boxing the whole column, so
            // a square photo does not float in a tall empty frame.
            const float pw = (L.wellMax.x - L.wellMin.x) - 16.0f * s;
            const float ph = (L.wellMax.y - L.wellMin.y) - 16.0f * s;
            const float iw = (float)p.artW, ih = (float)p.artH;
            const float fit = ImMin(pw / iw, ph / ih) * 0.95f;   // contain, a little breathing room
            const float dw = iw * fit, dh = ih * fit;
            const ImVec2 imgMin(L.wellMin.x + 8.0f * s + (pw - dw) * 0.70f,
                                (L.wellMin.y + L.wellMax.y) * 0.5f - dh * 0.5f);
            const ImVec2 imgMax(imgMin.x + dw, imgMin.y + dh);
            const float gap = 8.0f * s;
            const ImVec2 fMin(imgMin.x - gap, imgMin.y - gap);
            const ImVec2 fMax(imgMax.x + gap, imgMax.y + gap);

            dl->AddRectFilled(fMin, fMax, ui_loading::C::U(ui_loading::C::BgCard, 0.55f), 18.0f * s);
            dl->AddRect(fMin, fMax, kLine, 18.0f * s, 0, 1.2f * s);
            dl->AddImageRounded(p.art, imgMin, imgMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                                kText, 14.0f * s);
            // One violet wash so a bright photo sits in the theme instead of on it.
            dl->AddRectFilled(imgMin, imgMax, IM_COL32(96, 40, 200, 26), 14.0f * s);
        }
        else
        {
            // Placeholder until the background texture is up (or if it failed).
            dl->AddRectFilled(L.wellMin, L.wellMax,
                              ui_loading::C::U(ui_loading::C::BgCard, 0.45f), 18.0f * s);
            dl->AddRect(L.wellMin, L.wellMax, kLine, 18.0f * s, 0, 1.2f * s);

            const ImVec2 c((L.wellMin.x + L.wellMax.x) * 0.5f, (L.wellMin.y + L.wellMax.y) * 0.5f);
            DrawImageGlyph(dl, ImVec2(c.x, c.y - 16.0f * s), 22.0f * s, kSoft, s);
            DrawCentered(dl, fMicro, 13.0f * s, c.x, c.y + 20.0f * s, kLabel, "NO ARTWORK");
            DrawCentered(dl, fMicro, 11.0f * s, c.x, c.y + 38.0f * s, kLine, "RUNTIME PREVIEW UNAVAILABLE");
        }

        // ---- heading ------------------------------------------------------
        DrawCentered(dl, fHeading, 43.0f * s, L.cx, L.headY, kText, "welcome back");
        DrawCentered(dl, fBody, 18.0f * s, L.cx, L.subY, kSoft, "Please sign in to continue");

        const float padY = 7.0f * s;

        // The card's own text is drawn at explicit sizes multiplied by s, but
        // InputText renders with g.FontSize, i.e. the pushed font's face size
        // (20 px) no matter how small the card got. On a 640x480 surface that
        // left the hint unscaled against a field half its usual width, and this
        // InputText clips at half the item (ImGui/imgui_widgets.cpp), so the
        // hint stopped mid-word. Scaling the window's font scales the widget's
        // text with the card; everything else here passes explicit sizes and is
        // unaffected. The window is the login card and nothing else.
        ImGui::SetWindowFontScale(s);

        // ---- reveal eye ----------------------------------------------------
        const bool reveal = p.reveal ? *p.reveal : false;
        {
            const ImVec2 eyeMin(L.fieldR - L.eyeSize, L.textCY - L.eyeSize * 0.5f);
            ImGui::SetCursorScreenPos(eyeMin);
            const bool tapped = ImGui::InvisibleButton("##login_reveal", ImVec2(L.eyeSize, L.eyeSize));
            if (tapped && p.reveal) *p.reveal = !*p.reveal;
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            DrawEye(dl, ImVec2(eyeMin.x + L.eyeSize * 0.5f, eyeMin.y + L.eyeSize * 0.5f),
                    11.0f * s, hover ? kBrt : kLabel, reveal, s);
        }

        // ---- the access key field -------------------------------------------
        {
            ImGui::SetCursorScreenPos(ImVec2(L.fieldL, L.inputTop));
            ImGui::SetNextItemWidth(L.fieldW - L.eyeSize - 6.0f * s);
            ImGui::PushFont(fInput);
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

            // Underline: dim until something happens to the field, then accent.
            const ImU32 lineCol = focused ? kAccent : (hovered ? kBrt : kLine);
            dl->AddLine(ImVec2(L.fieldL, L.fieldBot), ImVec2(L.fieldR, L.fieldBot),
                        lineCol, (focused ? 2.0f : 1.5f) * s);
            if (focused)
                dl->AddLine(ImVec2(L.fieldL, L.fieldBot + 2.0f * s), ImVec2(L.fieldR, L.fieldBot + 2.0f * s),
                            ui_loading::C::UA(ui_loading::C::Purple, 0.28f), 3.0f * s);

            // Tracked micro label, right aligned over the field.
            DrawTrackedRight(dl, fMicro, 13.0f * s, L.fieldR, L.fieldTop + 2.0f * s, kLabel,
                             "ACCESS KEY", 0.7f * s);
        }

        // ---- error ----------------------------------------------------------
        if (hasError)
        {
            dl->AddRectFilled(L.errorMin, L.errorMax, IM_COL32(255, 78, 102, 26), 8.0f * s);
            dl->AddRect(L.errorMin, L.errorMax, IM_COL32(255, 78, 102, 120), 8.0f * s, 0, 1.0f * s);
            dl->AddRectFilled(ImVec2(L.errorMin.x, L.errorMin.y + 5.0f * s),
                              ImVec2(L.errorMin.x + 2.0f * s, L.errorMax.y - 5.0f * s), kDanger, 1.0f * s);
            dl->AddText(fMicro, 12.0f * s, ImVec2(L.errorMin.x + 12.0f * s, L.errorMin.y + 8.0f * s),
                        kDanger, p.error);
        }

        // ---- remember me -----------------------------------------------------
        if (p.remember)
        {
            const float box = L.rememberMax.x - L.rememberMin.x;
            const char* label = "Remember me";
            const ImVec2 ls = TextSize(fBody, 18.0f * s, label);

            dl->AddText(fBody, 18.0f * s,
                        ImVec2(L.rememberMin.x - 12.0f * s - ls.x,
                               L.rememberMin.y + (box - ls.y) * 0.5f - 1.0f * s), kSoft, label);

            ImGui::SetCursorScreenPos(L.rememberMin);
            if (ImGui::InvisibleButton("##login_remember", ImVec2(box, box)))
                *p.remember = !*p.remember;
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            const float r = box * 0.30f;
            if (*p.remember)
            {
                dl->AddRectFilled(L.rememberMin, L.rememberMax, hover ? kBrt : kAccent, r);
                dl->AddLine(ImVec2(L.rememberMin.x + box * 0.24f, L.rememberMin.y + box * 0.53f),
                            ImVec2(L.rememberMin.x + box * 0.44f, L.rememberMin.y + box * 0.72f), kText, 1.8f * s);
                dl->AddLine(ImVec2(L.rememberMin.x + box * 0.44f, L.rememberMin.y + box * 0.72f),
                            ImVec2(L.rememberMin.x + box * 0.78f, L.rememberMin.y + box * 0.29f), kText, 1.8f * s);
            }
            else
            {
                dl->AddRectFilled(L.rememberMin, L.rememberMax,
                                  ui_loading::C::UA(ui_loading::C::BgCard, hover ? 1.0f : 0.70f), r);
                dl->AddRect(L.rememberMin, L.rememberMax, hover ? kAccent : kLine, r, 0, 1.4f * s);
            }
        }

        // ---- authenticate pill ------------------------------------------------
        {
            const float round = (L.pillMax.y - L.pillMin.y) * 0.5f;
            ImGui::SetCursorScreenPos(L.pillMin);
            const bool clicked = ImGui::InvisibleButton("##login_authenticate",
                                                        ImVec2(L.fieldW, L.pillMax.y - L.pillMin.y));
            const bool hover = ImGui::IsItemHovered();
            const bool active = ImGui::IsItemActive();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            dl->AddRectFilled(L.pillMin, L.pillMax, active ? kDeep : (hover ? kBrt : kAccent), round);
            dl->AddRectFilledMultiColor(L.pillMin, L.pillMax,
                IM_COL32(255, 255, 255, 26), IM_COL32(255, 255, 255, 6),
                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0), round);
            dl->AddRect(L.pillMin, L.pillMax, ui_loading::C::UA(ui_loading::C::PurpleBrt, 0.55f), round, 0, 1.0f * s);

            const ImVec2 ts = TextSize(fButton, 21.0f * s, "AUTHENTICATE");
            dl->AddText(fButton, 21.0f * s,
                        ImVec2(L.pillMin.x + (L.fieldW - ts.x) * 0.5f,
                               L.pillMin.y + ((L.pillMax.y - L.pillMin.y) - ts.y) * 0.5f - 1.0f * s),
                        kText, "AUTHENTICATE");

            if (clicked)
                out.authenticate = true;
        }

        // ---- paste -------------------------------------------------------------
        {
            const float pasteH = L.pasteMax.y - L.pasteMin.y;
            const float round = pasteH * 0.5f;

            ImGui::SetCursorScreenPos(L.pasteMin);
            const bool clicked = ImGui::InvisibleButton("##login_paste", ImVec2(L.fieldW, pasteH));
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            dl->AddRectFilled(L.pasteMin, L.pasteMax,
                              ui_loading::C::UA(ui_loading::C::Purple, hover ? 0.20f : 0.10f), round);
            dl->AddRect(L.pasteMin, L.pasteMax,
                        ui_loading::C::UA(ui_loading::C::Border, hover ? 0.75f : 0.40f), round, 0, 1.0f * s);

            const char* label = "PASTE FROM CLIPBOARD";
            const float tracking = 0.8f * s;
            const float lw = TrackedWidth(fMicro, 12.0f * s, label, tracking);
            const ImVec2 lsz = TextSize(fMicro, 12.0f * s, label);
            DrawTracked(dl, fMicro, 12.0f * s,
                        ImVec2(L.pasteMin.x + (L.fieldW - lw) * 0.5f,
                               L.pasteMin.y + (pasteH - lsz.y) * 0.5f - 1.0f * s),
                        hover ? kBrt : ui_loading::C::UA(ui_loading::C::PurpleBrt, 0.85f), label, tracking);

            if (clicked)
                out.paste = true;
        }

        // ---- footer -------------------------------------------------------------
        DrawCentered(dl, fMicro, 12.0f * s, L.cx, L.footerY,
                     ui_loading::C::UA(ui_loading::C::TextDim, 0.70f),
                     "Secure connection  -  All data encrypted");

        return out;
    }
}
