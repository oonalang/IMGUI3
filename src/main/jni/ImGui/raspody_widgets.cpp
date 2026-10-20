#include "raspody_widgets.h"
#include "raspody_font.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace raspody
{
    namespace
    {
        float g_columnWidth = 0.0f;
        bool  g_lastRowSettings = false;
        char  g_lastRowLabel[128] = { 0 };
        ImVec2 g_lastRowMin = ImVec2(0, 0);
        ImVec2 g_lastRowMax = ImVec2(0, 0);

        std::string g_searchQuery;

        EspPreviewFn g_espPreview = nullptr;

        struct CardFrame
        {
            ImVec2 pos;
            float  width = 0.0f;
            float  padding = 0.0f;
            float  top = 0.0f;
            float  contentTop = 0.0f;
            float  previousWidth = 0.0f;
            ImVec2 previousCursor = ImVec2(0, 0);
        };

        std::vector<CardFrame> g_cards;

        struct PopupState
        {
            std::string name;
            bool  open = false;
            bool  stayOpen = false;
            float alpha = 0.0f;
            ImVec2 pos = ImVec2(0, 0);
            ImVec2 actualPos = ImVec2(0, 0);
            ImVec2 size = ImVec2(0, 0);
            double openedAt = 0.0;
            unsigned lastRenderedFrame = 0;
        };

        std::vector<PopupState> g_popups;
        unsigned g_popupFrame = 0;
        int g_popupDepth = 0;

        PopupState* FindPopup(const char* name)
        {
            for (auto& popup : g_popups)
                if (popup.name == name)
                    return &popup;
            return nullptr;
        }

        const PopupState* FindPopupConst(const char* name)
        {
            for (const auto& popup : g_popups)
                if (popup.name == name)
                    return &popup;
            return nullptr;
        }

        float& Anim(ImGuiID id, int slot)
        {
            static std::unordered_map<unsigned long long, float> values;
            const unsigned long long key = ((unsigned long long)id << 4) ^ (unsigned long long)(slot & 0xF);
            return values[key];
        }

        float RowWidth()
        {
            if (g_columnWidth > 1.0f)
                return g_columnWidth;

            const float avail = ImGui::GetContentRegionAvail().x;
            return avail > 1.0f ? avail : 220.0f;
        }

        void FinishRow(ImVec2 pos, float width, float height)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float y = pos.y + height;
            dl->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + width, y),
                        Col(clr.popupScrollBg, 0.9f), 1.0f);
            ImGui::SetCursorScreenPos(ImVec2(pos.x, y + S(6)));
        }

        void DrawCheckGlyph(ImDrawList* dl, ImVec2 center, float size, ImU32 color)
        {
            dl->AddLine(ImVec2(center.x - size * 0.45f, center.y),
                        ImVec2(center.x - size * 0.10f, center.y + size * 0.35f), color, ImMax(1.0f, size * 0.16f));
            dl->AddLine(ImVec2(center.x - size * 0.10f, center.y + size * 0.35f),
                        ImVec2(center.x + size * 0.48f, center.y - size * 0.35f), color, ImMax(1.0f, size * 0.16f));
        }

        void DrawEllipsis(ImDrawList* dl, ImVec2 center, float radius, ImU32 color)
        {
            for (int i = -1; i <= 1; ++i)
                dl->AddCircleFilled(ImVec2(center.x + i * radius * 2.2f, center.y), radius, color, 8);
        }

        bool IsHovering(ImVec2 min, ImVec2 max)
        {
            return ImGui::IsMouseHoveringRect(min, max, false);
        }

        void LowerCopy(const char* text, std::string& out)
        {
            out.clear();
            if (!text)
                return;
            for (const char* p = text; *p; ++p)
                out.push_back((char)std::tolower((unsigned char)*p));
        }
    } // namespace

    // -------------------------------------------------------------------------
    // Theme / frame
    // -------------------------------------------------------------------------
    void Style()
    {
        ImGuiStyle& s = ImGui::GetStyle();

        s.WindowPadding = ImVec2(0, 0);
        s.FramePadding = ImVec2(8, 4);
        s.ItemSpacing = ImVec2(6, 6);
        s.ItemInnerSpacing = ImVec2(6, 4);
        s.WindowRounding = 0.0f;
        s.ChildRounding = 20.0f;
        s.PopupRounding = 16.0f;
        s.FrameRounding = 8.0f;
        s.GrabRounding = 8.0f;
        s.ScrollbarRounding = 8.0f;
        s.WindowBorderSize = 0.0f;
        s.ChildBorderSize = 0.0f;
        s.PopupBorderSize = 0.0f;
        s.FrameBorderSize = 0.0f;
        s.WindowShadowSize = 0.0f;
        s.ScrollbarSize = 6.0f;
        s.AntiAliasedLines = true;
        s.AntiAliasedFill = true;

        s.Colors[ImGuiCol_Text] = clr.white;
        s.Colors[ImGuiCol_TextDisabled] = clr.text2;
        s.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_PopupBg] = clr.popup;
        s.Colors[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_FrameBg] = clr.widget;
        s.Colors[ImGuiCol_FrameBgHovered] = clr.checkboxHover;
        s.Colors[ImGuiCol_FrameBgActive] = clr.checkboxHover;
        s.Colors[ImGuiCol_TitleBg] = clr.layout;
        s.Colors[ImGuiCol_TitleBgActive] = clr.layout;
        s.Colors[ImGuiCol_TitleBgCollapsed] = clr.layout;
        s.Colors[ImGuiCol_MenuBarBg] = clr.layout;
        s.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_ScrollbarGrab] = clr.popupScrollThumb;
        s.Colors[ImGuiCol_ScrollbarGrabHovered] = clr.checkboxHover;
        s.Colors[ImGuiCol_ScrollbarGrabActive] = clr.accent;
        s.Colors[ImGuiCol_CheckMark] = clr.accent;
        s.Colors[ImGuiCol_SliderGrab] = clr.accent;
        s.Colors[ImGuiCol_SliderGrabActive] = clr.accentSoft;
        s.Colors[ImGuiCol_Button] = clr.widget;
        s.Colors[ImGuiCol_ButtonHovered] = clr.checkboxHover;
        s.Colors[ImGuiCol_ButtonActive] = clr.accent;
        s.Colors[ImGuiCol_Header] = clr.widget;
        s.Colors[ImGuiCol_HeaderHovered] = clr.checkboxHover;
        s.Colors[ImGuiCol_HeaderActive] = clr.accent;
        s.Colors[ImGuiCol_Separator] = clr.separator;
        s.Colors[ImGuiCol_SeparatorHovered] = clr.text;
        s.Colors[ImGuiCol_SeparatorActive] = clr.accent;
        s.Colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
        s.Colors[ImGuiCol_Tab] = clr.widget;
        s.Colors[ImGuiCol_TabHovered] = clr.checkboxHover;
        s.Colors[ImGuiCol_TabActive] = clr.accent;
        s.Colors[ImGuiCol_PlotLines] = clr.accent;
        s.Colors[ImGuiCol_TextSelectedBg] = WithAlpha(clr.accent, 0.35f);
        s.Colors[ImGuiCol_NavHighlight] = clr.accent;
    }

    void SetScale(float scale)
    {
        g_scale = ImClamp(scale, 0.55f, 2.0f);
        ImGuiStyle& s = ImGui::GetStyle();
        s.ScrollbarSize = S(6);
        s.ItemSpacing = ImVec2(S(6), S(4));
        s.FramePadding = ImVec2(S(8), S(4));
        s.WindowPadding = ImVec2(0, 0);
    }

    float Scale()
    {
        return g_scale;
    }

    void BeginFrame()
    {
        g_anyPopupOpen = false;
        g_popupOpenedThisFrame = false;
        g_lastRowSettings = false;
        ++g_popupFrame;

        const float dt = ImGui::GetIO().DeltaTime;
        for (auto& popup : g_popups)
        {
            // A popup whose owning page vanished (tab switch, search filter)
            // must not leave the blackout active forever.
            if (popup.open && popup.lastRenderedFrame + 2 < g_popupFrame)
                popup.open = false;

            if (!popup.open && popup.alpha > 0.0f)
                popup.alpha = ImMax(0.0f, popup.alpha - dt * 8.0f);
            if (popup.open || popup.alpha > 0.001f)
                g_anyPopupOpen = true;
        }
    }

    // -------------------------------------------------------------------------
    // Text helpers
    // -------------------------------------------------------------------------
    ImVec2 TextSize(ImFont* font, float size, const char* text)
    {
        if (!font || !text)
            return ImVec2(0, 0);
        return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    }

    void DrawText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
                  ImU32 color, const char* text, ImVec2 align)
    {
        if (!dl || !font || !text || !*text)
            return;

        const ImVec2 sz = TextSize(font, size, text);
        const ImVec2 at(pos.x - sz.x * align.x, pos.y - sz.y * align.y);
        dl->AddText(font, size, at, color, text);
    }

    void DrawTextClipped(ImDrawList* dl, ImFont* font, float size,
                         ImVec2 min, ImVec2 max, ImU32 color, const char* text,
                         ImVec2 align)
    {
        if (!dl || !font || !text || !*text || max.x <= min.x || max.y <= min.y)
            return;

        const ImVec2 sz = TextSize(font, size, text);
        // align.x/align.y are fractions of the *box*: 0 is the min edge, 0.5 the
        // centre and 1 the max edge. Scaling by the text size instead (the old
        // form) pushed fractional alignments outside the row, which drew labels
        // on top of the section heading and clipped them away.
        const ImVec2 box(max.x - min.x, max.y - min.y);
        const ImVec2 at(min.x + (box.x - sz.x) * align.x,
                        min.y + (box.y - sz.y) * align.y);
        dl->PushClipRect(min, max, true);
        dl->AddText(font, size, at, color, text);
        dl->PopClipRect();
    }

    void DrawSeparator(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 color)
    {
        dl->AddLine(ImVec2(min.x, (min.y + max.y) * 0.5f),
                    ImVec2(max.x, (min.y + max.y) * 0.5f), color, 1.0f);
    }

    // -------------------------------------------------------------------------
    // Vector icons
    // -------------------------------------------------------------------------
    void DrawTabIcon(ImDrawList* dl, int icon, ImVec2 c, float size, ImU32 color)
    {
        const float h = size * 0.5f;
        const float th = ImMax(1.0f, size * 0.075f);

        switch (icon)
        {
        case Icon_Aimbot:
        {
            dl->AddCircle(c, h * 0.78f, color, 24, th);
            dl->AddLine(ImVec2(c.x - h * 1.05f, c.y), ImVec2(c.x - h * 0.42f, c.y), color, th);
            dl->AddLine(ImVec2(c.x + h * 0.42f, c.y), ImVec2(c.x + h * 1.05f, c.y), color, th);
            dl->AddLine(ImVec2(c.x, c.y - h * 1.05f), ImVec2(c.x, c.y - h * 0.42f), color, th);
            dl->AddLine(ImVec2(c.x, c.y + h * 0.42f), ImVec2(c.x, c.y + h * 1.05f), color, th);
            dl->AddCircleFilled(c, h * 0.12f, color, 8);
            break;
        }
        case Icon_Visuals:
        {
            ImVec2 left(c.x - h * 1.05f, c.y);
            ImVec2 right(c.x + h * 1.05f, c.y);
            dl->AddBezierCubic(left, ImVec2(c.x - h * 0.45f, c.y - h * 0.95f),
                               ImVec2(c.x + h * 0.45f, c.y - h * 0.95f), right, color, th);
            dl->AddBezierCubic(left, ImVec2(c.x - h * 0.45f, c.y + h * 0.95f),
                               ImVec2(c.x + h * 0.45f, c.y + h * 0.95f), right, color, th);
            dl->AddCircleFilled(c, h * 0.30f, color, 18);
            break;
        }
        case Icon_Weapons:
        {
            const ImVec2 a(c.x - h * 0.95f, c.y - h * 0.18f);
            const ImVec2 b(c.x + h * 0.55f, c.y + h * 0.16f);
            dl->AddRectFilled(a, b, color, ImMax(1.0f, size * 0.08f));
            dl->AddLine(ImVec2(c.x + h * 0.5f, c.y + h * 0.02f),
                        ImVec2(c.x + h * 1.1f, c.y + h * 0.02f), color, th);
            dl->AddLine(ImVec2(c.x - h * 0.50f, c.y + h * 0.12f),
                        ImVec2(c.x - h * 0.22f, c.y + h * 0.78f), color, th * 1.4f);
            dl->AddLine(ImVec2(c.x - h * 0.95f, c.y - h * 0.18f),
                        ImVec2(c.x - h * 1.15f, c.y + h * 0.30f), color, th);
            break;
        }
        case Icon_Scripts:
        {
            dl->AddLine(ImVec2(c.x - h * 0.35f, c.y - h * 0.62f),
                        ImVec2(c.x - h * 0.95f, c.y), color, th);
            dl->AddLine(ImVec2(c.x - h * 0.95f, c.y),
                        ImVec2(c.x - h * 0.35f, c.y + h * 0.62f), color, th);
            dl->AddLine(ImVec2(c.x + h * 0.35f, c.y - h * 0.62f),
                        ImVec2(c.x + h * 0.95f, c.y), color, th);
            dl->AddLine(ImVec2(c.x + h * 0.95f, c.y),
                        ImVec2(c.x + h * 0.35f, c.y + h * 0.62f), color, th);
            dl->AddLine(ImVec2(c.x + h * 0.16f, c.y - h * 0.85f),
                        ImVec2(c.x - h * 0.16f, c.y + h * 0.85f), color, th);
            break;
        }
        case Icon_Configs:
        {
            const ImVec2 min(c.x - h * 0.95f, c.y - h * 0.62f);
            const ImVec2 max(c.x + h * 0.95f, c.y + h * 0.62f);
            dl->AddRect(min, max, color, ImMax(1.0f, size * 0.12f), 0, th);
            dl->AddLine(ImVec2(min.x, min.y + h * 0.20f),
                        ImVec2(min.x + h * 0.55f, min.y + h * 0.20f), color, th);
            dl->AddLine(ImVec2(min.x + h * 0.45f, min.y),
                        ImVec2(min.x + h * 0.45f, min.y + h * 0.20f), color, th);
            break;
        }
        case Icon_Settings:
        default:
        {
            DrawGearIcon(dl, c, size, color);
            break;
        }
        }
    }

    void DrawSearchIcon(ImDrawList* dl, ImVec2 c, float size, ImU32 color)
    {
        const float r = size * 0.32f;
        const float th = ImMax(1.0f, size * 0.08f);
        dl->AddCircle(ImVec2(c.x - size * 0.08f, c.y - size * 0.08f), r, color, 20, th);
        dl->AddLine(ImVec2(c.x + r * 0.62f, c.y + r * 0.62f),
                    ImVec2(c.x + size * 0.36f, c.y + size * 0.36f), color, th);
    }

    void DrawChevron(ImDrawList* dl, ImVec2 c, float size, ImU32 color, float angleRad)
    {
        const float th = ImMax(1.0f, size * 0.10f);
        const float cs = cosf(angleRad);
        const float sn = sinf(angleRad);
        const ImVec2 points[3] = {
            ImVec2(-size * 0.32f, -size * 0.16f),
            ImVec2(0.0f, size * 0.16f),
            ImVec2(size * 0.32f, -size * 0.16f)
        };

        ImVec2 rotated[3];
        for (int i = 0; i < 3; ++i)
            rotated[i] = ImVec2(c.x + points[i].x * cs - points[i].y * sn,
                                c.y + points[i].x * sn + points[i].y * cs);

        dl->AddLine(rotated[0], rotated[1], color, th);
        dl->AddLine(rotated[1], rotated[2], color, th);
    }

    void DrawMinusIcon(ImDrawList* dl, ImVec2 c, float size, ImU32 color)
    {
        dl->AddLine(ImVec2(c.x - size * 0.28f, c.y), ImVec2(c.x + size * 0.28f, c.y),
                    color, ImMax(1.0f, size * 0.09f));
    }

    void DrawGearIcon(ImDrawList* dl, ImVec2 c, float size, ImU32 color)
    {
        const float r = size * 0.30f;
        const float th = ImMax(1.0f, size * 0.075f);
        dl->AddCircle(c, r, color, 20, th);
        dl->AddCircleFilled(c, r * 0.30f, color, 12);
        for (int i = 0; i < 8; ++i)
        {
            const float a = (float)i * (IM_PI * 2.0f / 8.0f);
            dl->AddLine(ImVec2(c.x + cosf(a) * r * 1.08f, c.y + sinf(a) * r * 1.08f),
                        ImVec2(c.x + cosf(a) * r * 1.48f, c.y + sinf(a) * r * 1.48f),
                        color, th * 1.15f);
        }
    }

    void DrawUserIcon(ImDrawList* dl, ImVec2 c, float size, ImU32 color)
    {
        dl->AddCircleFilled(ImVec2(c.x, c.y - size * 0.14f), size * 0.22f, color, 20);
        dl->AddCircleFilled(ImVec2(c.x, c.y + size * 0.34f), size * 0.34f, color, 24);
    }

    // -------------------------------------------------------------------------
    // Header widgets
    // -------------------------------------------------------------------------
    void Brand(ImVec2 pos, float height, const char* name, const char* tagline)
    {
        ImFont* titleFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* descFont = fonts::small ? fonts::small : ImGui::GetFont();
        ImFont* logoFont = fonts::brand ? fonts::brand : titleFont;

        const float titleSize = 30.0f;
        const float descSize = 16.0f;
        const float logoSize = 40.0f;
        const float titleW = TextSize(titleFont, titleSize, name).x;
        const float descW = TextSize(descFont, descSize, tagline).x;
        const float textW = ImMax(titleW, descW);

        const float left = S(12);
        const float right = S(18);
        const float spacing = S(10);
        const float logoBox = S(34);
        const float width = left + logoBox + spacing + textW + right;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                          Col(clr.child, 0.92f), S(20));

        // Brand mark: the accent "E" that opens the ETHNIR wordmark.
        DrawText(dl, logoFont, logoSize,
                 ImVec2(pos.x + left + logoBox * 0.5f, pos.y + height * 0.5f),
                 Col(clr.accent), "E", ImVec2(0.5f, 0.5f));

        const float textX = pos.x + left + logoBox + spacing;
        DrawText(dl, titleFont, titleSize,
                 ImVec2(textX, pos.y + height * 0.34f), Col(clr.white), name, ImVec2(0.0f, 0.5f));
        DrawText(dl, descFont, descSize,
                 ImVec2(textX, pos.y + height * 0.72f), Col(clr.text), tagline, ImVec2(0.0f, 0.5f));

        elements.brand.calcWidth = width;
    }

    bool ProfileButton(ImVec2 pos, float height, const char* name, const char* subtitle)
    {
        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const float nameSize = 20.0f;
        const float subSize = 16.0f;
        const float avatar = S(38);
        const float gap = S(10);
        const float textW = ImMax(TextSize(bodyFont, nameSize, name).x,
                                  TextSize(smallFont, subSize, subtitle).x);

        const float width = avatar + gap + textW + S(8);
        const ImVec2 min(pos.x - width, pos.y);
        const ImVec2 max(pos.x, pos.y + height);

        ImGui::PushID("##raspody_profile_button");
        ImGui::SetCursorScreenPos(min);
        ImGui::InvisibleButton("##profile", ImVec2(width, height));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 titleColor = hovered ? Col(clr.white) : Col(clr.white, 0.92f);
        const ImU32 subColor = hovered ? Col(clr.text3) : Col(clr.text2);

        DrawTextClipped(dl, bodyFont, nameSize,
                        ImVec2(min.x, min.y), ImVec2(max.x - avatar - gap, min.y + height * 0.62f),
                        titleColor, name, ImVec2(1.0f, 0.5f));
        DrawTextClipped(dl, smallFont, subSize,
                        ImVec2(min.x, min.y + height * 0.38f), ImVec2(max.x - avatar - gap, max.y),
                        subColor, subtitle, ImVec2(1.0f, 0.5f));

        const ImVec2 avatarCenter(pos.x - avatar * 0.5f, pos.y + height * 0.5f);
        const float radius = avatar * 0.5f;

        dl->AddCircleFilled(avatarCenter, radius, Col(clr.widget), 32);
        dl->AddCircle(avatarCenter, radius, Col(clr.avatarBorder), 32, ImMax(1.0f, S(2)));
        DrawUserIcon(dl, ImVec2(avatarCenter.x, avatarCenter.y + S(1.5f)), avatar * 0.62f, Col(clr.text3));

        const ImVec2 gearCenter(pos.x - S(3), pos.y + height - S(4));
        dl->AddCircleFilled(gearCenter, S(10), Col(clr.popup), 20);
        dl->AddCircle(gearCenter, S(10), Col(clr.separator), 20, 1.0f);
        DrawGearIcon(dl, gearCenter, S(11), hovered ? Col(clr.accent) : Col(clr.text3));

        elements.profile.calcWidth = width;
        elements.profile.topRight = ImVec2(pos.x, pos.y);
        return pressed;
    }

    bool MinimizeButton(ImVec2 pos, float size)
    {
        ImGui::PushID("##raspody_minimize");
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##minimize", ImVec2(size, size));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 center(pos.x + size * 0.5f, pos.y + size * 0.5f);
        dl->AddCircleFilled(center, size * 0.5f,
                            hovered ? Col(clr.accent) : Col(clr.child, 0.9f), 24);
        DrawMinusIcon(dl, center, size * 0.9f,
                      hovered ? Col(clr.layout) : Col(clr.text3));
        return pressed;
    }

    bool TabButton(const char* label, int icon, bool selected, ImVec2 pos, ImVec2 size)
    {
        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##tab", size);
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiID id = ImGui::GetID(label);
        float& hot = Anim(id, 20);
        Ease(hot, (hovered || selected) ? 1.0f : 0.0f, 18.0f);

        const ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
        const float tile = ImMin(size.x, size.y) * 0.82f;
        const ImVec2 tmin(center.x - tile * 0.5f, center.y - tile * 0.5f);
        const ImVec2 tmax(center.x + tile * 0.5f, center.y + tile * 0.5f);

        if (selected)
            dl->AddRectFilled(tmin, tmax, Col(clr.widget, 0.34f), S(16));
        else if (hovered)
            dl->AddRectFilled(tmin, tmax, Col(clr.widget, 0.18f), S(16));

        const ImU32 iconColor = selected ? Col(clr.white)
                             : hovered ? Col(clr.text3) : Col(clr.text);
        DrawTabIcon(dl, icon, center, ImMin(size.x, size.y) * 0.50f, iconColor);

        if (selected)
        {
            dl->AddRectFilled(ImVec2(pos.x + size.x + S(4), center.y - S(13)),
                              ImVec2(pos.x + size.x + S(8), center.y + S(13)),
                              Col(clr.accent), S(99));
        }

        if (hovered)
        {
            ImFont* font = fonts::small ? fonts::small : ImGui::GetFont();
            const float textW = TextSize(font, 16.0f, label).x;
            const ImVec2 chipMin(pos.x + size.x + S(16), center.y - S(14));
            const ImVec2 chipMax(chipMin.x + textW + S(22), center.y + S(14));
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            fg->AddRectFilled(chipMin, chipMax, Col(clr.popup, 0.97f), S(9));
            fg->AddRect(chipMin, chipMax, Col(clr.separator), S(9), 0, 1.0f);
            DrawText(fg, font, S(11), ImVec2((chipMin.x + chipMax.x) * 0.5f, center.y),
                     selected ? Col(clr.accent) : Col(clr.text3), label, ImVec2(0.5f, 0.5f));
        }

        return pressed;
    }

    // -------------------------------------------------------------------------
    // Cards
    // -------------------------------------------------------------------------
    void BeginCard(const char* title, ImVec2 pos, float width)
    {
        CardFrame card;
        card.pos = pos;
        card.width = width;
        card.padding = S(15);
        card.previousWidth = g_columnWidth;
        card.previousCursor = ImGui::GetCursorScreenPos();
        card.top = pos.y + S(26);
        card.contentTop = card.top + card.padding;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->ChannelsSplit(2);
        dl->ChannelsSetCurrent(1);

        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), S(12),
                 ImVec2(pos.x + S(2), pos.y + S(7)), Col(clr.text2), title, ImVec2(0.0f, 0.5f));

        g_columnWidth = width - card.padding * 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(pos.x + card.padding, card.contentTop));
        g_cards.push_back(card);
    }

    float EndCard()
    {
        if (g_cards.empty())
            return ImGui::GetCursorScreenPos().y;

        CardFrame card = g_cards.back();
        g_cards.pop_back();

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        float bottom = cursor.y - S(6) + card.padding;
        if (bottom < card.contentTop + card.padding)
            bottom = card.contentTop + card.padding;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->ChannelsSetCurrent(0);
        dl->AddRectFilled(ImVec2(card.pos.x, card.top), ImVec2(card.pos.x + card.width, bottom),
                          Col(clr.child, 0.96f), S(20));
        dl->AddRect(ImVec2(card.pos.x, card.top), ImVec2(card.pos.x + card.width, bottom),
                    Col(clr.separator, 0.65f), S(20), 0, 1.0f);
        dl->ChannelsSetCurrent(1);
        dl->ChannelsMerge();

        g_columnWidth = card.previousWidth;
        ImGui::SetCursorScreenPos(ImVec2(card.pos.x, bottom + S(14)));
        return bottom;
    }

    float CardInnerWidth()
    {
        return RowWidth();
    }

    void SetColumnWidth(float width)
    {
        g_columnWidth = width;
    }

    void Spacer(float height)
    {
        ImGui::Dummy(ImVec2(RowWidth(), height));
    }

    void SectionLabel(const char* text)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), S(11),
                 ImVec2(pos.x + S(2), pos.y + S(10)), Col(clr.text2), text, ImVec2(0.0f, 0.5f));
        ImGui::Dummy(ImVec2(RowWidth(), S(22)));
    }

    void ContentShadow(ImVec2 min, ImVec2 max, bool top)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (top)
            dl->AddRectFilledMultiColor(min, max,
                Col(clr.layout, 0.95f), Col(clr.layout, 0.95f),
                Col(clr.layout, 0.0f), Col(clr.layout, 0.0f));
        else
            dl->AddRectFilledMultiColor(min, max,
                Col(clr.layout, 0.0f), Col(clr.layout, 0.0f),
                Col(clr.layout, 0.95f), Col(clr.layout, 0.95f));
    }

    // -------------------------------------------------------------------------
    // Row label / description stack
    //
    // Rows used to place the label and its description with fractional aligns
    // of the row box, so with the body font's line height the description landed
    // on top of the label. Both lines are now measured and stacked, and the row
    // is at least tall enough for them at any menu scale.
    // -------------------------------------------------------------------------
    inline float RowTextHeight(ImFont* bodyFont, ImFont* smallFont, const char* label,
                               const char* description, float minHeight)
    {
        const float titleH = TextSize(bodyFont, 20.0f, label).y;
        if (!description || !*description)
            return ImMax(minHeight, titleH + S(10));
        const float descH = TextSize(smallFont, 16.0f, description).y;
        return ImMax(minHeight, titleH + descH + S(3) + S(7));
    }

    inline void DrawRowText(ImDrawList* dl, ImFont* bodyFont, ImFont* smallFont,
                            ImVec2 min, ImVec2 max, ImU32 titleColor,
                            const char* label, const char* description)
    {
        const float titleH = TextSize(bodyFont, 20.0f, label).y;
        if (!description || !*description)
        {
            DrawTextClipped(dl, bodyFont, 20.0f, min, max, titleColor, label, ImVec2(0.0f, 0.5f));
            return;
        }

        const float descH = TextSize(smallFont, 16.0f, description).y;
        const float gap = S(3);
        const float stackH = titleH + gap + descH;
        const float top = min.y + ImMax(0.0f, ((max.y - min.y) - stackH) * 0.5f);
        DrawTextClipped(dl, bodyFont, 20.0f, ImVec2(min.x, top), ImVec2(max.x, top + titleH),
                        titleColor, label, ImVec2(0.0f, 0.0f));
        DrawTextClipped(dl, smallFont, 16.0f, ImVec2(min.x, top + titleH + gap),
                        ImVec2(max.x, top + stackH), Col(clr.text), description, ImVec2(0.0f, 0.0f));
    }

    // -------------------------------------------------------------------------
    // Checkbox
    // -------------------------------------------------------------------------
    bool Checkbox(const char* label, const char* description, bool* value, bool settings)
    {
        if (!value || !MatchesSearch(label, description))
            return false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const bool hasDesc = description && *description;
        const float width = RowWidth();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = RowTextHeight(bodyFont, smallFont, label, hasDesc ? description : nullptr, S(30));
        const float toggleW = S(36);
        const float toggleH = S(20);
        const ImVec2 toggleMin(pos.x + width - toggleW, pos.y + (height - toggleH) * 0.5f);
        const ImVec2 toggleMax(toggleMin.x + toggleW, toggleMin.y + toggleH);
        const ImVec2 ellipsis(pos.x + width - toggleW - S(24), pos.y + height * 0.5f);
        const bool overSettings = settings &&
            IsHovering(ImVec2(ellipsis.x - S(11), pos.y), ImVec2(ellipsis.x + S(11), pos.y + height));

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##checkbox_row", ImVec2(width, height));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        if (pressed && !overSettings && !ui.dragScrolling)
            *value = !*value;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiID id = ImGui::GetID(label);

        float& hoverAnim = Anim(id, 1);
        Ease(hoverAnim, hovered ? 1.0f : 0.0f, 18.0f);
        float& toggleAnim = Anim(id, 2);
        Ease(toggleAnim, *value ? 1.0f : 0.0f, 22.0f);

        const ImU32 titleColor = *value ? Col(clr.white)
                              : hovered ? Col(clr.text3) : Col(clr.text2);
        DrawRowText(dl, bodyFont, smallFont, pos,
                    ImVec2(pos.x + width - toggleW - (settings ? S(38) : S(8)), pos.y + height),
                    titleColor, label, hasDesc ? description : nullptr);

        const ImVec4 bg = *value ? clr.widget
                        : hovered ? clr.checkboxHover : clr.widget;
        dl->AddRectFilled(toggleMin, toggleMax, Col(bg), S(999));
        dl->AddRectFilled(toggleMin, toggleMax, Col(clr.accent, 0.10f * toggleAnim), S(999));

        const float knobRadius = S(7);
        const float knobX = toggleMin.x + S(3) + knobRadius +
                            (toggleW - S(6) - knobRadius * 2.0f) * toggleAnim;
        const ImU32 knobColor = *value ? Col(clr.accent)
                              : hovered ? Col(clr.circleCheckboxHover) : Col(clr.text);
        dl->AddCircleFilled(ImVec2(knobX, toggleMin.y + toggleH * 0.5f), knobRadius, knobColor, 20);

        if (settings)
        {
            DrawEllipsis(dl, ellipsis, S(1.6f),
                         overSettings ? Col(clr.text3) : Col(clr.text2, 0.9f));
        }

        FinishRow(pos, width, height);

        g_lastRowSettings = settings;
        g_lastRowMin = pos;
        g_lastRowMax = ImVec2(pos.x + width, pos.y + height);
        if (settings)
        {
            strncpy(g_lastRowLabel, label, sizeof(g_lastRowLabel) - 1);
            g_lastRowLabel[sizeof(g_lastRowLabel) - 1] = '\0';
        }

        return pressed && !overSettings;
    }

    void CheckboxSettings(const char* label, bool* option1, const char* option1Label,
                          bool* option2, const char* option2Label)
    {
        if (!g_lastRowSettings || std::strcmp(label, g_lastRowLabel) != 0)
            return;

        const std::string popupStorage = std::string("Row settings##") + label;
        const char* popupName = popupStorage.c_str();
        const ImVec2 ellipsis(g_lastRowMax.x - S(60), (g_lastRowMin.y + g_lastRowMax.y) * 0.5f);

        if (IsHovering(ImVec2(ellipsis.x - S(12), g_lastRowMin.y),
                       ImVec2(ellipsis.x + S(12), g_lastRowMax.y)) &&
            ImGui::IsMouseClicked(0))
        {
            OpenPopup(popupName,
                      ImVec2((g_lastRowMin.x + g_lastRowMax.x) * 0.5f - S(100),
                             g_lastRowMax.y + S(4)));
        }

        if (!IsPopupOpen(popupName))
            return;

        if (!BeginPopup(popupName, ImVec2(S(240), S(174)),
                        ImVec2((g_lastRowMin.x + g_lastRowMax.x) * 0.5f - S(120),
                               g_lastRowMax.y + S(4)),
                        ImVec2(S(15), S(15))))
            return;

        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        KeybindChip("Ctrl + X", cursor, ImGui::GetContentRegionAvail().x);
        ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + S(36)));
        ImGui::Dummy(ImVec2(1, S(2)));

        DrawText(ImGui::GetWindowDrawList(), fonts::small ? fonts::small : ImGui::GetFont(), S(11),
                 ImGui::GetCursorScreenPos(), Col(clr.text2), "Settings", ImVec2(0.0f, 0.0f));
        ImGui::Dummy(ImVec2(1, S(22)));

        if (option1 && option1Label)
        {
            if (Selectable(option1Label, *option1))
                *option1 = !*option1;
        }
        if (option2 && option2Label)
        {
            if (Selectable(option2Label, *option2))
                *option2 = !*option2;
        }

        EndPopup();
    }

    // -------------------------------------------------------------------------
    // Sliders
    // -------------------------------------------------------------------------
    bool SliderFloat(const char* label, const char* description, float* value,
                     float min, float max, const char* format)
    {
        if (!value || !MatchesSearch(label, description))
            return false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const bool hasDesc = description && *description;
        const float width = RowWidth();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = RowTextHeight(bodyFont, smallFont, label, hasDesc ? description : nullptr, S(32));
        const float barW = S(120);
        const float barH = S(10);
        const ImVec2 barMin(pos.x + width - barW, pos.y + (height - barH) * 0.5f);
        const ImVec2 barMax(barMin.x + barW, barMin.y + barH);

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(barMin);
        ImGui::InvisibleButton("##slider_bar", ImVec2(barW, ImMax(barH, S(24))));
        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered() ||
            IsHovering(ImVec2(pos.x, pos.y), ImVec2(pos.x + width, pos.y + height));
        ImGui::PopID();

        if (active && max > min)
        {
            const float t = ImSaturate((ImGui::GetIO().MousePos.x - barMin.x) / barW);
            *value = ImClamp(min + (max - min) * t, ImMin(min, max), ImMax(min, max));
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiID id = ImGui::GetID(label);

        float& display = Anim(id, 3);
        if (display == 0.0f && *value != 0.0f)
            display = *value;
        Ease(display, *value, active ? 40.0f : 20.0f);

        float& hot = Anim(id, 4);
        Ease(hot, (active || hovered) ? 1.0f : 0.0f, 18.0f);

        const ImU32 titleColor = active ? Col(clr.white)
                              : hovered ? Col(clr.text3) : Col(clr.text2);
        DrawRowText(dl, bodyFont, smallFont, pos,
                    ImVec2(pos.x + width - barW - S(12), pos.y + height),
                    titleColor, label, hasDesc ? description : nullptr);

        char buffer[64];
        snprintf(buffer, sizeof(buffer), format ? format : "%.0f", display);
        DrawTextClipped(dl, smallFont, 16.0f,
                        ImVec2(pos.x, pos.y), ImVec2(barMin.x - S(8), pos.y + height),
                        Col(clr.white), buffer, ImVec2(1.0f, hasDesc ? 0.34f : 0.5f));

        dl->AddRectFilled(barMin, barMax, Col(clr.widget), S(4));

        const float t = max > min ? ImSaturate((display - min) / (max - min)) : 0.0f;
        if (t > 0.001f)
        {
            const ImVec2 fillMax(barMin.x + barW * t, barMax.y);
            dl->AddRectFilled(barMin, fillMax, Col(clr.accent), S(4));
            dl->AddRectFilledMultiColor(barMin, fillMax,
                Col(clr.black, 0.0f), Col(clr.black, 0.28f),
                Col(clr.black, 0.28f), Col(clr.black, 0.0f));
            dl->AddRectFilled(barMin, fillMax, Col(clr.white, 0.18f * hot), S(4));
        }

        const float handleX = barMin.x + barW * t;
        const float handlePadding = active ? S(6) : S(4);
        dl->AddRectFilled(ImVec2(handleX - S(1.5f), barMin.y - handlePadding),
                          ImVec2(handleX + S(1.5f), barMax.y + handlePadding),
                          Col(clr.white), S(2));

        FinishRow(pos, width, height);
        return active;
    }

    bool SliderInt(const char* label, const char* description, int* value,
                   int min, int max, const char* format)
    {
        if (!value)
            return false;

        // SliderFloat formats a float; never hand a %d format to it.
        char safeFormat[32] = "%.0f";
        if (format && *format)
        {
            strncpy(safeFormat, format, sizeof(safeFormat) - 1);
            safeFormat[sizeof(safeFormat) - 1] = '\0';
            for (char* c = safeFormat; *c; ++c)
                if (*c == 'd')
                    *c = 'f';
        }

        float temp = (float)*value;
        const bool active = SliderFloat(label, description, &temp, (float)min, (float)max, safeFormat);
        if (active)
        {
            int next = (int)std::lround(temp);
            next = (int)ImClamp((float)next, (float)ImMin(min, max), (float)ImMax(min, max));
            if (next != *value)
            {
                *value = next;
                return true;
            }
        }
        return false;
    }

    bool Range(const char* label, const char* description, float* valueMin, float* valueMax,
               float min, float max, const char* format)
    {
        if (!valueMin || !valueMax || !MatchesSearch(label, description))
            return false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const bool hasDesc = description && *description;
        const float width = RowWidth();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = RowTextHeight(bodyFont, smallFont, label, hasDesc ? description : nullptr, S(34));
        const float barW = S(120);
        const float barH = S(10);
        const ImVec2 barMin(pos.x + width - barW, pos.y + (height - barH) * 0.5f);
        const ImVec2 barMax(barMin.x + barW, barMin.y + barH);

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(barMin);
        ImGui::InvisibleButton("##range_bar", ImVec2(barW, ImMax(barH, S(24))));
        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered() ||
            IsHovering(ImVec2(pos.x, pos.y), ImVec2(pos.x + width, pos.y + height));
        ImGui::PopID();

        ImGuiID id = ImGui::GetID(label);
        float& dragHigh = Anim(id, 8);

        const float gap = S(1);
        const float lowT = max > min ? ImSaturate((*valueMin - min) / (max - min)) : 0.0f;
        const float highT = max > min ? ImSaturate((*valueMax - min) / (max - min)) : 1.0f;

        if (active && max > min)
        {
            const float mouseT = ImSaturate((ImGui::GetIO().MousePos.x - barMin.x) / barW);
            if (ImGui::IsItemActivated())
                dragHigh = ImAbs(mouseT - highT) < ImAbs(mouseT - lowT) ? 1.0f : 0.0f;

            float next = min + (max - min) * mouseT;
            if (dragHigh > 0.5f)
                *valueMax = ImClamp(next, *valueMin + gap, max);
            else
                *valueMin = ImClamp(next, min, *valueMax - gap);
        }

        float& lowAnim = Anim(id, 9);
        float& highAnim = Anim(id, 10);
        if (lowAnim == 0.0f && *valueMin != 0.0f) lowAnim = *valueMin;
        if (highAnim == 0.0f && *valueMax != 0.0f) highAnim = *valueMax;
        Ease(lowAnim, *valueMin, 22.0f);
        Ease(highAnim, *valueMax, 22.0f);

        float& hot = Anim(id, 11);
        Ease(hot, (active || hovered) ? 1.0f : 0.0f, 18.0f);

        const ImU32 titleColor = active ? Col(clr.white)
                              : hovered ? Col(clr.text3) : Col(clr.text2);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        DrawRowText(dl, bodyFont, smallFont, pos,
                    ImVec2(pos.x + width - barW - S(12), pos.y + height),
                    titleColor, label, hasDesc ? description : nullptr);

        char lowBuf[32], highBuf[32];
        snprintf(lowBuf, sizeof(lowBuf), format ? format : "%.0f", lowAnim);
        snprintf(highBuf, sizeof(highBuf), format ? format : "%.0f", highAnim);
        char combined[80];
        snprintf(combined, sizeof(combined), "(%s, %s)", lowBuf, highBuf);
        DrawTextClipped(dl, smallFont, 16.0f,
                        ImVec2(barMin.x - S(150), pos.y), ImVec2(barMin.x - S(8), pos.y + height),
                        Col(clr.white), combined, ImVec2(1.0f, hasDesc ? 0.34f : 0.5f));

        dl->AddRectFilled(barMin, barMax, Col(clr.widget), S(4));

        const float aT = ImSaturate((lowAnim - min) / (max - min));
        const float bT = ImSaturate((highAnim - min) / (max - min));
        const ImVec2 fillMin(barMin.x + barW * ImMin(aT, bT), barMin.y);
        const ImVec2 fillMax(barMin.x + barW * ImMax(aT, bT), barMax.y);
        if (fillMax.x > fillMin.x)
        {
            dl->AddRectFilled(fillMin, fillMax, Col(clr.accent), S(4));
            dl->AddRectFilledMultiColor(fillMin, fillMax,
                Col(clr.black, 0.0f), Col(clr.black, 0.28f),
                Col(clr.black, 0.28f), Col(clr.black, 0.0f));
            dl->AddRectFilled(fillMin, fillMax, Col(clr.white, 0.18f * hot), S(4));
        }

        const float lowHandle = barMin.x + barW * aT;
        const float highHandle = barMin.x + barW * bT;
        dl->AddRectFilled(ImVec2(lowHandle - S(2), barMin.y - S(5)),
                          ImVec2(lowHandle + S(2), barMax.y + S(5)), Col(clr.white), S(2));
        dl->AddRectFilled(ImVec2(highHandle - S(2), barMin.y - S(5)),
                          ImVec2(highHandle + S(2), barMax.y + S(5)), Col(clr.white), S(2));

        FinishRow(pos, width, height);
        return active;
    }

    // -------------------------------------------------------------------------
    // Dropdown
    // -------------------------------------------------------------------------
    bool Dropdown(const char* label, const char* description, int* current,
                  const char* const* items, int itemCount)
    {
        if (!current || !items || itemCount <= 0 || !MatchesSearch(label, description))
            return false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const bool hasDesc = description && *description;
        const float width = RowWidth();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = RowTextHeight(bodyFont, smallFont, label, hasDesc ? description : nullptr, S(34));
        const float buttonW = S(140);
        const float buttonH = S(24);
        const ImVec2 buttonMin(pos.x + width - buttonW, pos.y + (height - buttonH) * 0.5f);
        const ImVec2 buttonMax(buttonMin.x + buttonW, buttonMin.y + buttonH);

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(buttonMin);
        ImGui::InvisibleButton("##dropdown", ImVec2(buttonW, buttonH));
        const bool hovered = ImGui::IsItemHovered() ||
            IsHovering(ImVec2(pos.x, pos.y), ImVec2(pos.x + width, pos.y + height));
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        std::string popupName = std::string(label) + "##Dropdown";
        if (pressed)
        {
            OpenPopup(popupName.c_str(), ImVec2(buttonMax.x - S(160), buttonMax.y + S(4)));
        }

        const bool open = IsPopupOpen(popupName.c_str());
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImGuiID id = ImGui::GetID(label);
        float& hot = Anim(id, 12);
        Ease(hot, (hovered || open) ? 1.0f : 0.0f, 18.0f);
        float& rot = Anim(id, 13);
        Ease(rot, open ? 1.0f : 0.0f, 20.0f);

        const ImU32 titleColor = open ? Col(clr.white)
                              : hovered ? Col(clr.text3) : Col(clr.text2);
        DrawRowText(dl, bodyFont, smallFont, pos,
                    ImVec2(pos.x + width - buttonW - S(12), pos.y + height),
                    titleColor, label, hasDesc ? description : nullptr);

        dl->AddRectFilled(buttonMin, buttonMax,
                          Col(open ? clr.checkboxHover : (hovered ? clr.checkboxHover : clr.widget)), S(6));
        dl->AddRectFilled(buttonMin, buttonMax, Col(clr.accent, 0.08f * hot), S(6));

        const int safeIndex = (int)ImClamp((float)*current, 0.0f, (float)(itemCount - 1));
        std::string preview = items[safeIndex];
        if (preview.size() > 16)
            preview = preview.substr(0, 13) + "...";
        DrawTextClipped(dl, smallFont, S(11.5f),
                        ImVec2(buttonMin.x + S(10), buttonMin.y),
                        ImVec2(buttonMax.x - S(24), buttonMax.y),
                        Col(clr.white), preview.c_str(), ImVec2(0.0f, 0.5f));

        DrawChevron(dl, ImVec2(buttonMax.x - S(12), (buttonMin.y + buttonMax.y) * 0.5f),
                    S(15), Col(clr.white), IM_PI * rot);

        FinishRow(pos, width, height);

        if (open)
        {
            const float popupH = ImMin(S(220), S(16) + itemCount * S(27));
            if (BeginPopup(popupName.c_str(), ImVec2(S(160), popupH),
                           ImVec2(buttonMax.x - S(160), buttonMax.y + S(4)),
                           ImVec2(S(8), S(8))))
            {
                for (int i = 0; i < itemCount; ++i)
                {
                    if (Selectable(items[i], i == *current))
                    {
                        *current = i;
                        ClosePopup(popupName.c_str());
                    }
                }
                EndPopup();
            }
        }

        return pressed;
    }

    // -------------------------------------------------------------------------
    // Colour row
    // -------------------------------------------------------------------------
    bool ColorRow(const char* label, const char* description, float color[4])
    {
        if (!MatchesSearch(label, description))
            return false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const bool hasDesc = description && *description;
        const float width = RowWidth();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float height = hasDesc ? S(42) : S(32);
        const float swatchW = S(38);
        const float swatchH = S(20);
        const ImVec2 swatchMin(pos.x + width - swatchW, pos.y + (height - swatchH) * 0.5f);
        const ImVec2 swatchMax(swatchMin.x + swatchW, swatchMin.y + swatchH);

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(swatchMin);
        ImGui::InvisibleButton("##color_swatch", ImVec2(swatchW, ImMax(swatchH, S(24))));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        std::string popupName = std::string(label) + "##Color";
        if (pressed && color)
            OpenPopup(popupName.c_str(), ImVec2(swatchMax.x - S(250), swatchMax.y + S(8)));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 titleColor = hovered ? Col(clr.text3) : Col(clr.text2);
        DrawRowText(dl, bodyFont, smallFont, pos,
                    ImVec2(pos.x + width - swatchW - S(12), pos.y + height),
                    titleColor, label, hasDesc ? description : nullptr);

        if (color)
        {
            dl->AddRectFilled(swatchMin, swatchMax, Col(ImVec4(color[0], color[1], color[2], 1.0f)), S(6));
            dl->AddRect(swatchMin, swatchMax, Col(clr.white, 0.25f), S(6), 0, 1.0f);
        }
        else
        {
            dl->AddRectFilled(swatchMin, swatchMax, Col(clr.widget), S(6));
        }

        FinishRow(pos, width, height);

        if (color && IsPopupOpen(popupName.c_str()))
        {
            if (BeginPopup(popupName.c_str(), ImVec2(S(266), S(306)),
                           ImVec2(swatchMax.x - S(266), swatchMax.y + S(8)),
                           ImVec2(S(12), S(12)), false))
            {
                ImGui::SetNextItemWidth(S(240));
                ImGui::ColorPicker4("##raspody_picker", color,
                    ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf |
                    ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

                if (Selectable("Close", false))
                    ClosePopup(popupName.c_str());
                EndPopup();
            }
        }

        return pressed;
    }

    // -------------------------------------------------------------------------
    // Popup manager
    // -------------------------------------------------------------------------
    void OpenPopup(const char* name, ImVec2 pos)
    {
        if (!name)
            return;

        for (auto& popup : g_popups)
        {
            if (popup.name != name && popup.open)
                popup.open = false;
        }

        PopupState* popup = FindPopup(name);
        if (!popup)
        {
            PopupState fresh;
            fresh.name = name;
            g_popups.push_back(fresh);
            popup = &g_popups.back();
        }

        popup->open = true;
        popup->openedAt = ImGui::GetTime();
        popup->lastRenderedFrame = g_popupFrame;
        if (pos.x != 0.0f || pos.y != 0.0f)
            popup->pos = pos;
        popup->alpha = ImMax(popup->alpha, 0.02f);
        g_popupOpenedThisFrame = true;
        g_anyPopupOpen = true;
    }

    void ClosePopup(const char* name)
    {
        PopupState* popup = FindPopup(name);
        if (popup)
            popup->open = false;
    }

    bool IsPopupOpen(const char* name)
    {
        const PopupState* popup = FindPopupConst(name);
        return popup && (popup->open || popup->alpha > 0.01f);
    }

    bool BeginPopup(const char* name, ImVec2 size, ImVec2 pos,
                    ImVec2 padding, bool stayOpen)
    {
        PopupState* popup = FindPopup(name);
        if (!popup)
            return false;

        if (pos.x != 0.0f || pos.y != 0.0f)
            popup->pos = pos;
        popup->size = size;
        popup->stayOpen = stayOpen;
        popup->lastRenderedFrame = g_popupFrame;

        Ease(popup->alpha, popup->open ? 1.0f : 0.0f, 15.0f);
        if (popup->alpha <= 0.001f && !popup->open)
            return false;

        g_anyPopupOpen = true;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float margin = S(6);
        ImVec2 windowPos = popup->pos;
        windowPos.x = ImClamp(windowPos.x, margin, ImMax(margin, display.x - size.x - margin));
        windowPos.y = ImClamp(windowPos.y, margin, ImMax(margin, display.y - size.y - margin));
        windowPos.y += S(10) * (1.0f - popup->alpha);
        popup->actualPos = windowPos;

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(size, size);
        ImGui::SetNextWindowFocus();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(6), S(4)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, popup->alpha);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

        ImGui::Begin(name, nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse);
        ++g_popupDepth;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        dl->AddRectFilled(wp, ImVec2(wp.x + size.x, wp.y + size.y),
                          Col(clr.popup, 0.98f), S(16));
        dl->AddRect(wp, ImVec2(wp.x + size.x, wp.y + size.y),
                    Col(clr.separator, 0.95f), S(16), 0, 1.0f);

        ImGui::SetCursorScreenPos(ImVec2(wp.x + padding.x, wp.y + padding.y));
        ImGui::PushItemWidth(ImMax(20.0f, size.x - padding.x * 2.0f));

        if (!stayOpen && popup->alpha > 0.6f &&
            !g_popupOpenedThisFrame &&
            ImGui::GetTime() - popup->openedAt > 0.12 &&
            ImGui::IsMouseClicked(0) &&
            !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            popup->open = false;
        }

        return true;
    }

    void EndPopup()
    {
        --g_popupDepth;
        ImGui::PopItemWidth();
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(5);
    }

    bool AnyPopupOpen()
    {
        return g_anyPopupOpen;
    }

    void TickPopups()
    {
        BeginFrame();
    }

    void DrawBlackout(float alpha)
    {
        if (alpha <= 0.001f)
            return;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        ImVec2 holeMin = display;
        ImVec2 holeMax = ImVec2(0, 0);

        for (const auto& popup : g_popups)
        {
            if (popup.alpha <= 0.5f || popup.size.x <= 0.0f)
                continue;

            holeMin.x = ImMin(holeMin.x, popup.actualPos.x);
            holeMin.y = ImMin(holeMin.y, popup.actualPos.y);
            holeMax.x = ImMax(holeMax.x, popup.actualPos.x + popup.size.x);
            holeMax.y = ImMax(holeMax.y, popup.actualPos.y + popup.size.y);
        }

        const float inset = S(8);
        holeMin = ImVec2(holeMin.x - inset, holeMin.y - inset);
        holeMax = ImVec2(holeMax.x + inset, holeMax.y + inset);

        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const ImU32 color = Col(clr.black, 0.52f * alpha);

        if (holeMax.x <= holeMin.x || holeMax.y <= holeMin.y)
        {
            fg->AddRectFilled(ImVec2(0, 0), display, color);
            return;
        }

        fg->AddRectFilled(ImVec2(0, 0), ImVec2(display.x, holeMin.y), color);
        fg->AddRectFilled(ImVec2(0, holeMax.y), display, color);
        fg->AddRectFilled(ImVec2(0, holeMin.y), ImVec2(holeMin.x, holeMax.y), color);
        fg->AddRectFilled(ImVec2(holeMax.x, holeMin.y), ImVec2(display.x, holeMax.y), color);
    }

    // -------------------------------------------------------------------------
    // Lists / small controls
    // -------------------------------------------------------------------------
    bool Selectable(const char* label, bool selected)
    {
        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float width = g_popupDepth > 0 ? ImGui::GetContentRegionAvail().x : RowWidth();
        const float height = S(24);

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##selectable", ImVec2(ImMax(width, S(40)), height));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (hovered)
            dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                              Col(clr.widget, 0.75f), S(6));
        if (selected)
            dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                              Col(clr.accent, 0.10f), S(6));

        if (selected)
            DrawCheckGlyph(dl, ImVec2(pos.x + S(10), pos.y + height * 0.5f), S(11), Col(clr.accent));
        DrawTextClipped(dl, bodyFont, 16.0f,
                        ImVec2(pos.x + S(22), pos.y), ImVec2(pos.x + width - S(6), pos.y + height),
                        selected ? Col(clr.white) : Col(clr.text3), label, ImVec2(0.0f, 0.5f));

        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + height + S(3)));
        return pressed && !ui.dragScrolling;
    }

    void KeybindChip(const char* key, ImVec2 pos, float width)
    {
        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        const float height = S(28);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.widget), S(8));
        DrawEllipsis(dl, ImVec2(pos.x + S(14), pos.y + height * 0.5f), S(1.4f), Col(clr.text3));
        DrawTextClipped(dl, bodyFont, S(12),
                        ImVec2(pos.x + S(28), pos.y), ImVec2(pos.x + width - S(28), pos.y + height),
                        Col(clr.white), key, ImVec2(0.0f, 0.5f));
        DrawEllipsis(dl, ImVec2(pos.x + width - S(14), pos.y + height * 0.5f), S(1.4f), Col(clr.text3));
    }

    bool ConfigButton(const char* name, const char* created, bool loaded,
                      bool& loadPressed, bool& savePressed)
    {
        loadPressed = false;
        savePressed = false;

        ImFont* bodyFont = fonts::body ? fonts::body : ImGui::GetFont();
        ImFont* smallFont = fonts::small ? fonts::small : ImGui::GetFont();

        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float width = RowWidth();
        const float height = S(58);

        ImGui::PushID(name);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##config_row", ImVec2(width, height));
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked(0);
        ImGui::PopID();

        const ImVec2 loadMin(pos.x + width - S(136), pos.y + S(18));
        const ImVec2 loadMax(loadMin.x + S(56), loadMin.y + S(24));
        const ImVec2 saveMin(pos.x + width - S(72), pos.y + S(18));
        const ImVec2 saveMax(saveMin.x + S(56), saveMin.y + S(24));

        if (clicked)
        {
            if (IsHovering(loadMin, loadMax))
                loadPressed = true;
            else if (IsHovering(saveMin, saveMax))
                savePressed = true;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                          Col(hovered ? clr.widget : clr.child, 0.95f), S(16));
        if (loaded)
            dl->AddRectFilled(ImVec2(pos.x + S(1), pos.y + S(12)),
                              ImVec2(pos.x + S(4), pos.y + height - S(12)),
                              Col(clr.accent), S(99));

        DrawTextClipped(dl, bodyFont, 20.0f,
                        ImVec2(pos.x + S(16), pos.y + S(8)),
                        ImVec2(pos.x + width - S(150), pos.y + height),
                        Col(clr.white), name, ImVec2(0.0f, 0.34f));
        DrawTextClipped(dl, smallFont, 16.0f,
                        ImVec2(pos.x + S(16), pos.y + S(8)),
                        ImVec2(pos.x + width - S(150), pos.y + height),
                        Col(clr.text), created, ImVec2(0.0f, 0.75f));

        dl->AddRectFilled(loadMin, loadMax, Col(clr.accent, loaded ? 1.0f : 0.85f), S(8));
        DrawText(dl, smallFont, 16.0f, ImVec2((loadMin.x + loadMax.x) * 0.5f, (loadMin.y + loadMax.y) * 0.5f),
                 Col(clr.layout), "LOAD", ImVec2(0.5f, 0.5f));

        dl->AddRectFilled(saveMin, saveMax, Col(clr.widget), S(8));
        DrawText(dl, smallFont, 16.0f, ImVec2((saveMin.x + saveMax.x) * 0.5f, (saveMin.y + saveMax.y) * 0.5f),
                 Col(clr.white), "SAVE", ImVec2(0.5f, 0.5f));

        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + height + S(8)));
        return loadPressed || savePressed;
    }

    // -------------------------------------------------------------------------
    // Search
    // -------------------------------------------------------------------------
    bool SearchButton(ImVec2 pos, float size, bool open)
    {
        ImGui::PushID("##raspody_search_button");
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##search", ImVec2(size, size));
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked(0);
        ImGui::PopID();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size),
                          Col(open ? clr.widget : clr.child), S(14));
        if (hovered || open)
            dl->AddRect(pos, ImVec2(pos.x + size, pos.y + size),
                        Col(clr.accent, hovered ? 0.75f : 0.45f), S(14), 0, 1.0f);
        DrawSearchIcon(dl, ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), size * 0.68f,
                       open ? Col(clr.accent) : Col(clr.white));
        return pressed;
    }

    bool SearchField(char* buffer, int bufferSize, ImVec2 pos, float width)
    {
        const float height = S(38);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.child), S(14));
        dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.separator, 0.9f), S(14), 0, 1.0f);
        DrawSearchIcon(dl, ImVec2(pos.x + S(18), pos.y + height * 0.5f), S(16), Col(clr.text2));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(6), S(8)));
        ImGui::PushFont(fonts::small ? fonts::small : ImGui::GetFont());

        ImGui::SetCursorScreenPos(ImVec2(pos.x + S(32), pos.y + S(3)));
        ImGui::SetNextItemWidth(ImMax(S(30), width - S(40)));

        const bool changed = ImGui::InputText("##raspody_search_input", buffer,
            (size_t)ImMax(bufferSize, 1), ImGuiInputTextFlags_CharsNoBlank);
        const bool active = ImGui::IsItemActive();

        if (buffer && *buffer == '\0' && !active)
        {
        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                 ImVec2(pos.x + S(34), pos.y + height * 0.5f),
                 Col(clr.text), "Search settings...", ImVec2(0.0f, 0.5f));
        }

        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        LowerCopy(buffer, g_searchQuery);
        return changed || active;
    }

    void ClearSearch()
    {
        g_searchQuery.clear();
    }

    bool MatchesSearch(const char* label, const char* description)
    {
        if (g_searchQuery.empty())
            return true;

        std::string haystack;
        LowerCopy(label, haystack);
        if (haystack.find(g_searchQuery) != std::string::npos)
            return true;
        LowerCopy(description, haystack);
        return haystack.find(g_searchQuery) != std::string::npos;
    }

    // -------------------------------------------------------------------------
    // ESP preview
    // -------------------------------------------------------------------------
    void SetEspPreview(EspPreviewFn fn)
    {
        g_espPreview = fn;
    }

    void DrawEspPreview(ImDrawList* dl, ImVec2 pos, float width, float height)
    {
        if (!dl || width <= 0.0f || height <= 0.0f)
            return;
        if (g_espPreview)
        {
            g_espPreview(dl, pos, width, height);
            return;
        }

        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                          Col(clr.layout, 0.9f), S(12));
        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                 ImVec2(pos.x + width * 0.5f, pos.y + height * 0.5f),
                 Col(clr.text2), "ESP preview unavailable", ImVec2(0.5f, 0.5f));
    }
}
