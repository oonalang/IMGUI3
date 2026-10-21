#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_settings.h"
#include "menu_layout.h"

#include "../System/UI/LoadingAnimation.h"
#include "../Fonts/poppins.h"

namespace login_screen
{
    constexpr ImU32 kPanel   = IM_COL32( 18,  11,  29, 255);
    constexpr ImU32 kHeader  = IM_COL32( 22,  14,  35, 255);

    constexpr ImU32 kArtBg   = kPanel;
    constexpr ImU32 kHair    = IM_COL32( 58,  42,  86, 255);
    constexpr ImU32 kLine    = IM_COL32( 64,  48,  94, 255);
    constexpr ImU32 kInk     = IM_COL32(244, 240, 252, 255);
    constexpr ImU32 kInkSoft = IM_COL32(172, 160, 198, 255);
    constexpr ImU32 kInkMute = IM_COL32(140, 128, 168, 255);
    constexpr ImU32 kPurple  = IM_COL32(139,  92, 246, 255);
    constexpr ImU32 kPurpleDn= IM_COL32(109,  40, 217, 255);
    constexpr ImU32 kDanger  = IM_COL32(255, 122, 148, 255);

    struct Fonts
    {
        ImFont* micro   = nullptr;
        ImFont* body    = nullptr;
        ImFont* field   = nullptr;
        ImFont* button  = nullptr;
        ImFont* heading = nullptr;
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
        cfg.FontDataOwnedByAtlas = false;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;

        const ImWchar* ranges = io.Fonts->GetGlyphRangesDefault();

        f.micro   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 12.0f, &cfg, ranges);
        f.body    = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 15.0f, &cfg, ranges);
        f.field   = io.Fonts->AddFontFromMemoryTTF((void*)poppins_regular, (int)sizeof(poppins_regular), 16.0f, &cfg, ranges);
        f.button  = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    17.0f, &cfg, ranges);
        f.heading = io.Fonts->AddFontFromMemoryTTF((void*)poppins_bold,    (int)sizeof(poppins_bold),    40.0f, &cfg, ranges);

        ImFont* fallback = io.FontDefault;
        if (!fallback) fallback = io.Fonts->AddFontDefault();
        if (!f.micro)   f.micro   = fallback;
        if (!f.body)    f.body    = fallback;
        if (!f.field)   f.field   = fallback;
        if (!f.button)  f.button  = fallback;
        if (!f.heading) f.heading = fallback;
    }

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

    struct Layout
    {
        float  s = 1.0f;

        ImVec2 cardMin, cardMax;
        float  cardRound = 12.0f;
        float  headerH   = 0.0f;
        ImVec2 closeMin, closeMax;
        float  brandX    = 0.0f;

        float  artR = 0.0f;
        ImVec2 wellMin, wellMax;

        float  fieldL = 0.0f, fieldR = 0.0f, fieldW = 0.0f, cx = 0.0f;
        float  textCY = 0.0f;
        float  fieldBot = 0.0f;
        float  inputTop = 0.0f;
        float  eyeSize = 0.0f;

        float  headY = 0.0f, subY = 0.0f;
        ImVec2 pillMin, pillMax;
        ImVec2 pasteMin, pasteMax;
        ImVec2 rememberMin, rememberMax;
        ImVec2 errorMin, errorMax;

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

        const MenuWindow win = ComputeMenuWindow(ImVec2(W, H));
        L.cardMin = ImVec2(o.x + win.pos.x, o.y + win.pos.y);
        L.cardMax = ImVec2(L.cardMin.x + win.size.x, L.cardMin.y + win.size.y);

        const float pw = ImMax(1.0f, L.cardMax.x - L.cardMin.x);
        const float ph = ImMax(1.0f, L.cardMax.y - L.cardMin.y);

        const float s = ImClamp(ImMin(ph / 578.0f, pw / 904.0f), 0.60f, 2.20f);
        L.s = s;
        L.cardRound = ImClamp(12.0f * s, 8.0f, 20.0f);
        L.headerH   = ImClamp(ph * 0.107f, 34.0f, 78.0f);

        L.brandX = L.cardMin.x + ImMax(pw * 0.031f, 14.0f * s);
        const float closeBox = ImClamp(34.0f * s, 26.0f, 44.0f);
        const float closeCX  = L.cardMax.x - ImMax(pw * 0.028f, 14.0f * s) - closeBox * 0.5f;
        L.closeMin = ImVec2(closeCX - closeBox * 0.5f, L.cardMin.y + (L.headerH - closeBox) * 0.5f);
        L.closeMax = ImVec2(L.closeMin.x + closeBox, L.closeMin.y + closeBox);

        L.artR = L.cardMin.x + pw * (pw >= ph ? 0.469f : 0.42f);
        L.wellMin = ImVec2(L.cardMin.x, L.cardMin.y + L.headerH);
        L.wellMax = ImVec2(L.artR, L.cardMax.y);

        const float artW = L.artR - L.cardMin.x;
        (void)artW;
        L.fieldL = L.artR + ImMax(pw * 0.038f, 16.0f * s);
        L.fieldR = L.cardMax.x - ImMax(pw * 0.055f, 18.0f * s);
        if (L.fieldR - L.fieldL < 90.0f * s)
        {
            const float g = ImMax(10.0f * s, pw * 0.02f);
            L.fieldL = o.x + g + pw * 0.44f;
            L.fieldR = L.cardMax.x - g;
        }
        L.fieldW = ImMax(60.0f, L.fieldR - L.fieldL);
        L.cx = (L.fieldL + L.fieldR) * 0.5f;

        Fonts& F = GetFonts();
        ImFont* heading = F.heading ? F.heading : ImGui::GetFont();
        ImFont* body = F.body ? F.body : heading;

        L.headY = L.cardMin.y + ph * 0.192f;
        const float headH = TextSize(heading, 40.0f * s, "welcome back").y;
        L.subY = L.headY + headH + 6.0f * s;
        const float subH = TextSize(body, 15.0f * s, "Please sign in to continue").y;

        L.textCY = L.cardMin.y + ph * 0.575f;
        L.fieldBot = L.textCY + 12.0f * s;
        L.eyeSize = ImClamp(30.0f * s, 24.0f, 42.0f);

        L.inputTop = L.textCY - 16.0f;

        const float pillH = ImClamp(ph * 0.107f, 34.0f, 74.0f);
        L.pillMin = ImVec2(L.fieldL, L.cardMin.y + ph * 0.746f);
        L.pillMax = ImVec2(L.fieldR, L.pillMin.y + pillH);

        const float box = ImClamp(ph * 0.036f, 14.0f, 26.0f);
        L.rememberMax = ImVec2(L.fieldR, L.pillMin.y - ImMax(9.0f * s, 6.0f));
        L.rememberMin = ImVec2(L.rememberMax.x - box, L.rememberMax.y - box);

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

    struct FieldSkin
    {
        ImVec4 frameBg, text, textHovered, textActive;

        explicit FieldSkin(float padY)
        {
            frameBg     = c::elements::background;
            text        = c::text::text;
            textHovered = c::text::text_hov;
            textActive  = c::text::text_active;

            c::elements::background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            c::text::text           = ImVec4(0.541f, 0.498f, 0.643f, 1.0f);
            c::text::text_hov       = ImVec4(0.541f, 0.498f, 0.643f, 1.0f);
            c::text::text_active    = ImVec4(0.957f, 0.941f, 0.988f, 1.0f);

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

    struct Params
    {
        ImVec2 pos;
        float  W = 1280.0f;
        float  H = 720.0f;

        char*  key = nullptr;
        int    keySize = 0;
        bool*  reveal = nullptr;
        bool*  remember = nullptr;

        const char* error = nullptr;
        const char* brand = nullptr;

        ImTextureID art = 0;
        int    artW = 0;
        int    artH = 0;
    };

    struct Result
    {
        bool authenticate = false;
        bool paste = false;
        bool close = false;
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

        dl->AddRectFilled(L.cardMin, L.cardMax, kPanel, L.cardRound);
        dl->AddRect(L.cardMin, L.cardMax, IM_COL32(139, 92, 246, 54), L.cardRound, 0, 1.0f);

        {
            const ImVec2 barMax(L.cardMax.x, L.cardMin.y + L.headerH);
            dl->AddRectFilled(L.cardMin, barMax, kHeader, L.cardRound, ImDrawFlags_RoundCornersTop);
            dl->AddLine(ImVec2(L.cardMin.x, barMax.y), ImVec2(L.cardMax.x, barMax.y), kHair, 1.0f);

            const float brandY = L.cardMin.y + (L.headerH - TextSize(fMicro, 12.0f * s, brand).y) * 0.5f;
            dl->AddText(fMicro, ImClamp(12.0f * s, 10.0f, 17.0f),
                        ImVec2(L.brandX, brandY), kInkSoft, brand);

            ImGui::SetCursorScreenPos(L.closeMin);
            const bool tapped = ImGui::InvisibleButton("##login_close",
                                                       ImVec2(L.closeMax.x - L.closeMin.x,
                                                              L.closeMax.y - L.closeMin.y));
            const bool hover = ImGui::IsItemHovered();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            const ImVec2 cc = L.closeCenter();
            const float cr = ImMin(7.0f * s, (L.closeMax.x - L.closeMin.x) * 0.24f);
            if (hover)
                dl->AddCircleFilled(cc, cr * 1.9f, IM_COL32(255, 255, 255, 16));
            DrawClose(dl, cc, cr, hover ? kInk : kInkSoft, ImMax(1.4f * s, 1.0f));
            if (tapped)
                out.close = true;
        }

        dl->AddRectFilled(L.wellMin, L.wellMax, kArtBg, L.cardRound, ImDrawFlags_RoundCornersBottomLeft);

        if (p.art && p.artW > 0 && p.artH > 0)
        {
            const float iw = (float)p.artW, ih = (float)p.artH;
            const float colW = L.wellMax.x - L.wellMin.x;
            const float colH = L.wellMax.y - L.wellMin.y;

            const float k = ImMin(colW / iw, colH / ih);
            const float dw = iw * k, dh = ih * k;
            const float x = L.wellMin.x + (colW - dw) * 0.62f;
            const float y = L.wellMax.y - dh;

            const bool atCorner = (x <= L.wellMin.x + 0.5f && y + dh >= L.wellMax.y - 0.5f);
            dl->AddImageRounded(p.art, ImVec2(x, y), ImVec2(x + dw, y + dh),
                                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE,
                                atCorner ? L.cardRound : 0.0f,
                                atCorner ? ImDrawFlags_RoundCornersBottomLeft : 0);
        }
        else
        {
            const ImVec2 c((L.wellMin.x + L.wellMax.x) * 0.5f, (L.wellMin.y + L.wellMax.y) * 0.5f);
            DrawImageGlyph(dl, ImVec2(c.x, c.y - 16.0f * s), 22.0f * s, kInkMute, s);
            DrawCentered(dl, fMicro, 12.0f * s, c.x, c.y + 20.0f * s, kInkMute, "NO ARTWORK");
        }

        DrawCentered(dl, fHeading, 40.0f * s, L.cx, L.headY, kInk, "welcome back");
        DrawCentered(dl, fBody, 15.0f * s, L.cx, L.subY, kInkSoft, "Please sign in to continue");

        ImGui::SetWindowFontScale(s);

        const float padY = ImMax(6.0f * s, 4.0f);

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

        {
            ImGui::SetCursorScreenPos(ImVec2(L.fieldL, L.inputTop));
            ImGui::SetNextItemWidth(L.fieldW - L.eyeSize - 6.0f * s);
            ImGui::PushFont(fField);

            ImGui::PushID("login_access_key");
            {
                FieldSkin skin(padY);

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

            const ImU32 lineCol = focused ? kPurple : (hovered ? ImU32(IM_COL32(108, 84, 152, 255)) : kLine);
            dl->AddLine(ImVec2(L.fieldL, L.fieldBot), ImVec2(L.fieldR, L.fieldBot),
                        lineCol, (focused ? 1.8f : 1.2f) * ImMax(s, 0.8f));

            const float labelSize = ImClamp(11.0f * s, 9.0f, 16.0f);
            const float labelH = TextSize(fMicro, labelSize, "ACCESS KEY").y;
            DrawTrackedRight(dl, fMicro, labelSize, L.fieldR,
                             L.textCY - ImMax(20.0f * s, 14.0f) - labelH,
                             focused ? kInkSoft : kInkMute, "ACCESS KEY", 0.9f * s);
        }

        if (hasError)
        {
            char line[160];
            int n = 0;
            for (const char* c = p.error; *c && n < (int)sizeof(line) - 1; ++c)
                line[n++] = (*c == '\n' || *c == '\r') ? ' ' : *c;
            line[n] = '\0';

            dl->AddRectFilled(L.errorMin, L.errorMax, IM_COL32(58, 20, 34, 255), 6.0f * s);
            dl->AddRect(L.errorMin, L.errorMax, IM_COL32(126, 52, 74, 255), 6.0f * s, 0, 1.0f);
            dl->AddText(fMicro, ImClamp(11.0f * s, 9.0f, 15.0f),
                        ImVec2(L.errorMin.x + 8.0f * s, (L.errorMin.y + L.errorMax.y) * 0.5f -
                                                       TextSize(fMicro, 11.0f * s, line).y * 0.5f),
                        kDanger, line);
        }

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
                dl->AddRectFilled(L.rememberMin, L.rememberMax, hover ? kPurpleDn : kPurple, box * 0.26f);
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

        {
            const float round = (L.pillMax.y - L.pillMin.y) * 0.5f;
            ImGui::SetCursorScreenPos(L.pillMin);
            const bool clicked = ImGui::InvisibleButton("##login_authenticate",
                                                        ImVec2(L.fieldW, L.pillMax.y - L.pillMin.y));
            const bool hover = ImGui::IsItemHovered();
            const bool active = ImGui::IsItemActive();
            if (hover) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            dl->AddRectFilled(L.pillMin, L.pillMax, active ? kPurpleDn : (hover ? kPurpleDn : kPurple), round);

            dl->AddRectFilledMultiColor(L.pillMin, L.pillMax,
                IM_COL32(255, 255, 255, 34), IM_COL32(255, 255, 255, 8),
                IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0), round);

            const float buttonSize = ImClamp(17.0f * s, 13.0f, 24.0f);
            const ImVec2 ts = TextSize(fButton, buttonSize, "sign in");
            dl->AddText(fButton, buttonSize,
                        ImVec2(L.pillMin.x + (L.fieldW - ts.x) * 0.5f,
                               L.pillMin.y + ((L.pillMax.y - L.pillMin.y) - ts.y) * 0.5f - 1.0f * s),
                        IM_COL32(255, 255, 255, 255), "sign in");

            if (clicked)
                out.authenticate = true;
        }

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
                        hover ? kPurple : kInkMute, label, tracking);

            if (clicked)
                out.paste = true;
        }

        return out;
    }
}

