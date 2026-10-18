#include "neverlose_menu.hpp"
#include "neverlose_menu_internal.hpp"
#include "nl_bindings.hpp"
#include "nl_ui.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "blur.hpp"
#include "hashes.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr ImVec2 kCanvas(750.0f, 756.0f);
    constexpr ImVec2 kShell(748.0f, 576.0f);
    constexpr float kTextBody = 15.0f;
    constexpr float kTextControl = 14.0f;
    constexpr float kTextSmall = 12.0f;
    constexpr float kTextCaption = 10.0f;
    constexpr float kTextTitle = 16.0f;
    constexpr float kTextIcon = 13.0f;
    constexpr ImU32 C(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

    struct Row
    {
        const char* label;
        NeverloseControl type;
        const char* value;
        float amount;
        // Binding: slot >= 0 drives this row from the feature flags instead of
        // the menu's own demo state (see nl_bindings.hpp).
        int slot;
        const char* const* options;
        int nopts;
        float lo;
        float hi;
        const char* pill;
    };
    struct Popup
    {
        bool open = false;
        bool multi = false;
        int owner = -1;
        int selected = 0;
        unsigned mask = 0x07;
        ImVec2 anchor{};
        float width = 134.0f;
        int opened_frame = 0;
        // Bound-select payload; null entries fall back to the demo options.
        const char* const* options = nullptr;
        int nopts = 0;
        int* bound = nullptr;
    };
    struct State
    {
        NeverlosePage page = NeverlosePage::Rage;
        NeverlosePage previous = NeverlosePage::Rage;
        float page_mix = 1.0f;
        bool account = false;
        bool toggles[192]{};
        bool slider_initialized[192]{};
        int selects[192]{};
        float sliders[192]{};
        Popup popup;

        State()
        {
            const int enabled[] = {
                0, 1, 2, 3, 18, 19,
                48, 57,
                79, 80, 88, 89, 95,
                120, 121, 122, 123, 124, 127, 128, 131, 132, 134, 135
            };
            for (int id : enabled) toggles[id] = true;
        }
    } state;

    float Motion(ImGuiID id, float target, float speed = 16.0f, float initial = -1.0f)
    {
        float* value = ImGui::GetStateStorage()->GetFloatRef(id, initial < 0.0f ? target : initial);
        *value = ImLerp(*value, target, 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
        if (std::fabs(*value - target) < 0.0005f) *value = target;
        return *value;
    }

    ImU32 Mix(ImU32 a, ImU32 b, float t)
    {
        return ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(a), ImGui::ColorConvertU32ToFloat4(b), ImSaturate(t)));
    }

    void Text(ImDrawList* d, ImVec2 p, ImU32 color, const char* value, float size = kTextBody, ImFont* font = nullptr)
    {
        d->AddText(font ? font : ImGui::GetFont(), size, p, color, value);
    }

    void TextY(ImDrawList* d, float x, float y, float h, ImU32 color, const char* value, float size = kTextBody, ImFont* font = nullptr)
    {
        ImFont* f = font ? font : ImGui::GetFont();
        const float th = f->CalcTextSizeA(size, FLT_MAX, 0.0f, value).y;
        Text(d, ImVec2(x, y + std::floor((h - th) * 0.5f)), color, value, size, f);
    }

    bool Hit(const char* id, ImVec2 p, ImVec2 size)
    {
        ImGui::SetCursorScreenPos(p);
        return ImGui::InvisibleButton(id, size);
    }

    void Dots(ImDrawList* d, ImVec2 p)
    {
        for (int i = 0; i < 3; ++i) d->AddCircleFilled(p + ImVec2(i * 3.0f, 0), 1.0f, C(187, 191, 202));
    }

    void Chevron(ImDrawList* d, ImVec2 p, ImU32 color)
    {
        d->AddLine(p, p + ImVec2(3, 3), color, 1.3f);
        d->AddLine(p + ImVec2(3, 3), p + ImVec2(0, 6), color, 1.3f);
    }

    void Check(ImDrawList* d, ImVec2 p, ImU32 color)
    {
        d->AddLine(p, p + ImVec2(3, 3), color, 1.5f);
        d->AddLine(p + ImVec2(3, 3), p + ImVec2(8, -3), color, 1.5f);
    }

    void Card(ImDrawList* d, ImVec2 p, ImVec2 size, const char* title)
    {
        Text(d, p + ImVec2(12, -18), C(89, 94, 106), title, kTextCaption);
        d->AddRectFilled(p, p + size, C(17, 19, 27, 224), 14.0f);
        d->AddRect(p, p + size, C(31, 34, 44), 14.0f);
    }

    void Toggle(ImDrawList* d, int id, bool* bound, ImVec2 p, bool enabled = true)
    {
        ImGui::PushID(id);
        const bool value = bound ? *bound : state.toggles[id];
        if (enabled && Hit("##toggle", p - ImVec2(5, 6), ImVec2(39, 30)))
        {
            if (bound) *bound = !value;
            else state.toggles[id] = !value;
        }
        const ImGuiID iid = ImGui::GetItemID();
        const float on = Motion(iid ^ 0x55aa721u, (bound ? *bound : state.toggles[id]) ? 1.0f : 0.0f, 19.0f);
        const float hover = Motion(iid ^ 0x118a0u, enabled && ImGui::IsItemHovered() ? 1.0f : 0.0f, 20.0f);
        ImGui::PopID();
        d->AddRectFilled(p - ImVec2(1 + hover, 1 + hover), p + ImVec2(30 + hover, 19 + hover), C(75, 126, 255, (int)(45 * hover)), 10);
        d->AddRectFilled(p, p + ImVec2(29, 18), Mix(C(29, 33, 43), C(75, 126, 255), on), 9);
        d->AddCircleFilled(p + ImVec2(ImLerp(9.0f, 20.0f, on), 9), 7, Mix(C(133, 144, 156), C(248, 249, 252), on));
    }

    void OpenPopup(int owner, bool multi, ImVec2 anchor, float width,
                   const char* const* options, int nopts, int* bound)
    {
        const bool same = state.popup.open && state.popup.owner == owner;
        state.popup.open = !same;
        state.popup.owner = owner;
        state.popup.multi = multi;
        state.popup.anchor = anchor;
        state.popup.width = width;
        state.popup.opened_frame = ImGui::GetFrameCount();
        state.popup.options = options;
        state.popup.nopts = nopts;
        state.popup.bound = bound;
    }

    void RowControl(ImDrawList* d, ImVec2 card, float width, int row, const Row& item, int& control_id)
    {
        const float y = card.y + row * 37.0f;
        if (row) d->AddLine(ImVec2(card.x + 12, y), ImVec2(card.x + width - 12, y), C(28, 31, 40));
        const bool disabled = item.type == NeverloseControl::Disabled;
        TextY(d, card.x + 13, y, 37, disabled ? C(92, 96, 107) : C(207, 209, 218), item.label, kTextBody);
        const int id = control_id++ + static_cast<int>(state.page) * 24;
        bool* bref = item.slot >= 0 ? nl_bind::ToggleRef(item.slot) : nullptr;
        int* sref = item.slot >= 0 ? nl_bind::SelectRef(item.slot) : nullptr;
        float* fref = item.slot >= 0 ? nl_bind::SliderRef(item.slot) : nullptr;
        const char* const* opts = nullptr;
        int nopts = 0;
        if (sref) nopts = nl_bind::SelectOptions(item.slot, &opts);
        switch (item.type)
        {
        case NeverloseControl::Toggle:
            Toggle(d, id, bref, ImVec2(card.x + width - 43, y + 9));
            break;
        case NeverloseControl::Disabled:
            Toggle(d, id, nullptr, ImVec2(card.x + width - 43, y + 9), false);
            break;
        case NeverloseControl::Select:
        case NeverloseControl::MultiSelect:
        {
            const float cw = ImMin(134.0f, width * 0.48f);
            const ImVec2 cp(card.x + width - cw - 13, y + 7);
            ImGui::PushID(id);
            const bool click = Hit("##select", cp, ImVec2(cw, 23));
            const ImGuiID iid = ImGui::GetItemID();
            const float response = Motion(iid ^ 0x6161u, ImGui::IsItemHovered() ? 1.0f : 0.0f);
            ImGui::PopID();
            if (click)
                OpenPopup(id, sref ? false : item.type == NeverloseControl::MultiSelect, cp, cw, opts, nopts, sref);
            d->AddRectFilled(cp, cp + ImVec2(cw, 23), Mix(C(25, 28, 38), C(31, 38, 54), response), 5);
            d->AddRect(cp, cp + ImVec2(cw, 23), Mix(C(32, 35, 46), C(75, 126, 255), response), 5);
            const char* shown = item.value ? item.value : "Select";
            if (sref && opts) shown = nl_bind::SelectFormat(item.slot, *sref);
            TextY(d, cp.x + 7, cp.y, 23, C(170, 173, 184), shown, kTextControl);
            d->AddLine(cp + ImVec2(cw - 13, 9), cp + ImVec2(cw - 9, 13), C(139, 143, 154), 1.0f);
            d->AddLine(cp + ImVec2(cw - 9, 13), cp + ImVec2(cw - 5, 9), C(139, 143, 154), 1.0f);
            break;
        }
        case NeverloseControl::Slider:
        {
            ImVec2 start(card.x + width - 145, y + 18);
            const float tw = 80.0f;
            const float lo = item.lo, hi = item.hi;
            float frac = item.amount;
            if (fref)
                frac = (hi > lo) ? ImClamp((*fref - lo) / (hi - lo), 0.0f, 1.0f) : 0.0f;
            if (!state.slider_initialized[id])
            {
                state.sliders[id] = frac;
                state.slider_initialized[id] = true;
            }
            ImGui::PushID(id);
            Hit("##slider", start - ImVec2(5, 8), ImVec2(tw + 10, 19));
            if (ImGui::IsItemActive())
            {
                frac = ImClamp((ImGui::GetIO().MousePos.x - start.x) / tw, 0.0f, 1.0f);
                state.sliders[id] = frac;
                if (fref) *fref = lo + (hi - lo) * frac;
            }
            const float shown = Motion(ImGui::GetItemID() ^ 0xa8f1u, frac, 14.0f, frac);
            ImGui::PopID();
            d->AddRectFilled(start, start + ImVec2(tw, 3), C(34, 38, 48), 2);
            d->AddRectFilled(start, start + ImVec2(tw * shown, 3), C(75, 126, 255), 2);
            d->AddCircleFilled(start + ImVec2(tw * shown, 1.5f), 5.5f, C(247, 248, 252));
            const ImVec2 pill(card.x + width - 55, y + 8);
            d->AddRectFilled(pill, pill + ImVec2(42, 21), C(25, 28, 38), 5);
            char buf[24] = {};
            const char* pill_text = item.value ? item.value : "0%";
            if (fref || item.pill)
            {
                const float v = fref ? *fref : item.amount;
                if (item.pill) std::snprintf(buf, sizeof(buf), item.pill, v);
                else std::snprintf(buf, sizeof(buf), "%.1f", v);
                pill_text = buf;
            }
            const float pill_width = ImGui::GetFont()->CalcTextSizeA(kTextControl, FLT_MAX, 0.0f, pill_text).x;
            TextY(d, pill.x + (42.0f - pill_width) * 0.5f, pill.y, 21, C(166, 169, 179), pill_text, kTextControl);
            break;
        }
        case NeverloseControl::Color:
            d->AddRectFilled(ImVec2(card.x + width - 63, y + 11), ImVec2(card.x + width - 48, y + 26), C(102, 124, 246), 5);
            Toggle(d, id, bref, ImVec2(card.x + width - 43, y + 9));
            break;
        case NeverloseControl::Chevron:
        {
            if (item.slot >= 0)
            {
                ImGui::PushID(id);
                const bool click = Hit("##action", ImVec2(card.x + width - 60, y + 4), ImVec2(width - 60, 30));
                const ImGuiID iid = ImGui::GetItemID();
                const float hov = Motion(iid ^ 0x9a11u, ImGui::IsItemHovered() ? 1.0f : 0.0f, 20.0f);
                if (hov > 0.01f)
                    d->AddRectFilled(ImVec2(card.x + width - 60, y + 4), ImVec2(card.x + width - 12, y + 33), C(39, 43, 54, (int)(120 * hov)), 6);
                if (click) nl_bind::DoAction(item.slot);
                const float t = ImGui::GetTime();
                const float pulse = 0.55f + 0.45f * std::sin(t * 3.0f);
                d->AddCircleFilled(ImVec2(card.x + width - 40, y + 18), 2.0f, C(75, 126, 255, (int)(255 * pulse)));
                ImGui::PopID();
            }
            Chevron(d, ImVec2(card.x + width - 22, y + 15), C(187, 190, 199));
            break;
        }
        }
    }

    void DrawRows(ImDrawList* d, ImVec2 card, float width, const Row* rows, int count, int& control_id)
    {
        for (int i = 0; i < count; ++i) RowControl(d, card, width, i, rows[i], control_id);
    }

    void PopupLayer(ImDrawList* d)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) state.popup.open = false;
        const float open = Motion(ImGui::GetID("##popup_open"), state.popup.open ? 1.0f : 0.0f, 20.0f, 0.0f);
        if (open < 0.002f) return;
        static const char* const multi_items[] = { "Head", "Chest", "Stomach", "Arms", "Legs", "Feet" };
        static const char* const single_items[] = { "Off", "Latency", "Performance" };
        const char* const* options = state.popup.bound && state.popup.nopts > 0 && state.popup.options
                                         ? state.popup.options
                                         : (state.popup.multi ? multi_items : single_items);
        const int count = state.popup.bound && state.popup.nopts > 0 && state.popup.options
                        ? state.popup.nopts
                        : (state.popup.multi ? 6 : 3);
        const ImVec2 size(state.popup.width, count * 32.0f + 8.0f);
        ImVec2 p(state.popup.anchor.x, state.popup.anchor.y + (23.0f - size.y) * 0.5f);
        p.x = ImClamp(p.x, 10.0f, ImGui::GetIO().DisplaySize.x - size.x - 10.0f);
        p.y = ImClamp(p.y, 10.0f, ImGui::GetIO().DisplaySize.y - size.y - 10.0f);
        const int first = d->VtxBuffer.Size;
        d->AddRectFilled(p - ImVec2(5, 2), p + size + ImVec2(5, 8), C(0, 0, 0, 55), 18);
        d->AddRectFilled(p, p + size, C(20, 20, 29, 235), 16);
        d->AddRect(p, p + size, C(57, 61, 76, 205), 16);
        d->AddLine(p + ImVec2(16, 1), p + ImVec2(size.x - 16, 1), C(255, 255, 255, 22));
        const bool accepts = ImGui::GetFrameCount() > state.popup.opened_frame;
        for (int i = 0; i < count; ++i)
        {
            ImVec2 rp = p + ImVec2(4, 4 + i * 32.0f);
            ImGui::PushID(5000 + i);
            const bool clicked = Hit("##popup_row", rp, ImVec2(size.x - 8, 32));
            const float hover = Motion(ImGui::GetItemID() ^ 0x9921u, ImGui::IsItemHovered() ? 1.0f : 0.0f, 22.0f);
            ImGui::PopID();
            if (hover > 0.001f) d->AddRectFilled(rp, rp + ImVec2(size.x - 8, 32), C(75, 126, 255, (int)(25 * hover)), 10);
            const bool selected = state.popup.bound && !state.popup.multi
                                      ? (i == (state.popup.bound ? *state.popup.bound : 0))
                                      : state.popup.multi
                                          ? (state.popup.mask & (1u << i)) != 0
                                          : state.popup.selected == i;
            if (selected) Check(d, rp + ImVec2(14, 16), C(230, 233, 241));
            TextY(d, rp.x + 35, rp.y, 32, selected ? C(224, 229, 243) : C(182, 185, 196), options[i], kTextControl);
            if (accepts && clicked)
            {
                if (state.popup.bound && !state.popup.multi) { *state.popup.bound = i; state.popup.open = false; }
                else if (state.popup.multi) state.popup.mask ^= 1u << i;
                else { state.popup.selected = i; state.popup.open = false; }
            }
        }
        const float eased = 1.0f - std::pow(1.0f - open, 3.0f);
        const ImVec2 pivot(p.x + size.x * 0.5f, p.y + size.y * 0.5f);
        for (int i = first; i < d->VtxBuffer.Size; ++i)
        {
            ImDrawVert& v = d->VtxBuffer[i];
            v.pos = pivot + (v.pos - pivot) * ImLerp(0.96f, 1.0f, eased) + ImVec2(4.0f * (1.0f - eased), 0);
            const ImU32 a = (v.col >> IM_COL32_A_SHIFT) & 255u;
            v.col = (v.col & 0x00ffffffu) | ((ImU32)(a * eased) << IM_COL32_A_SHIFT);
        }
    }

    void ChangePage(NeverlosePage next)
    {
        if (state.page == next) return;
        state.previous = state.page;
        state.page = next;
        state.page_mix = 0.0f;
        state.popup.open = false;
    }

    void Sidebar(ImDrawList* d, ImVec2 base, ImTextureID avatar)
    {
        ImFont* strong = ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();
        d->AddRectFilled(base, base + ImVec2(158, 576), C(18, 21, 30, 231), 14, ImDrawFlags_RoundCornersLeft);
        d->AddRectFilled(base + ImVec2(145, 0), base + ImVec2(158, 576), C(18, 21, 30, 231));
        d->AddLine(base + ImVec2(158, 0), base + ImVec2(158, 576), C(30, 33, 43));
        d->AddRectFilled(base + ImVec2(15, 11), base + ImVec2(45, 43), C(8, 27, 48), 7);
        Text(d, base + ImVec2(21, 18), C(94, 185, 255), "NL", kTextTitle, strong);
        Text(d, base + ImVec2(53, 14), C(228, 230, 236), "Neverlose", kTextTitle, strong);
        Text(d, base + ImVec2(53, 33), C(91, 96, 108), "Garena CODM", 8);
        d->AddLine(base + ImVec2(10, 56), base + ImVec2(147, 56), C(28, 31, 41));
        Text(d, base + ImVec2(16, 67), C(91, 96, 108), "COMBAT", kTextCaption);
        Text(d, base + ImVec2(16, 169), C(91, 96, 108), "FEATURES", kTextCaption);

        auto nav = [&](int id, const char* icon, const char* label, float y, NeverlosePage target, bool selected, float indent = 0.0f)
        {
            const ImVec2 p = base + ImVec2(7 + indent, y);
            ImGui::PushID(id);
            const bool clicked = Hit("##nav", p, ImVec2(140 - indent, 30));
            const ImGuiID iid = ImGui::GetItemID();
            const float r = Motion(iid ^ 0x7771u, selected ? 1.0f : ImGui::IsItemHovered() ? 0.48f : 0.0f);
            ImGui::PopID();
            if (clicked) ChangePage(target);
            if (r > 0.001f) d->AddRectFilled(p, p + ImVec2(140 - indent, 30), Mix(C(18, 21, 30, 0), C(39, 43, 54), r), 6);
            TextY(d, p.x + 10, p.y, 30, Mix(C(137, 142, 153), C(82, 141, 255), r), icon, kTextIcon);
            TextY(d, p.x + 31, p.y, 30, Mix(C(145, 149, 159), C(226, 228, 235), r), label, kTextBody);
        };

        const bool visuals = state.page == NeverlosePage::Players || state.page == NeverlosePage::World;
        nav(0, ICON_FA_CROSSHAIRS, "Aim", 84, NeverlosePage::Rage, state.page == NeverlosePage::Rage);
        nav(1, ICON_FA_MOUSE, "Extra", 120, NeverlosePage::Legit, state.page == NeverlosePage::Legit);
        nav(2, ICON_FA_IMAGE, "ESP", 190, NeverlosePage::Players, visuals);
        const float expand = Motion(ImGui::GetID("##visual_expand"), visuals ? 1.0f : 0.0f, 18.0f);
        if (expand > 0.02f)
        {
            const int first = d->VtxBuffer.Size;
            nav(3, ICON_FA_USER, "Players", 226, NeverlosePage::Players, state.page == NeverlosePage::Players, 14);
            // (Players/World pair: the parent opens the ESP pair below.)
            nav(4, ICON_FA_GLOBE, "World", 262, NeverlosePage::World, state.page == NeverlosePage::World, 14);
            for (int i = first; i < d->VtxBuffer.Size; ++i)
            {
                ImDrawVert& v = d->VtxBuffer[i];
                const ImU32 a = (v.col >> IM_COL32_A_SHIFT) & 255u;
                v.col = (v.col & 0xffffffu) | ((ImU32)(a * expand) << IM_COL32_A_SHIFT);
            }
        }
        const float shift = 72.0f * expand;
        nav(5, ICON_FA_LAYER_GROUP, "Skins", 226 + shift, NeverlosePage::Inventory, state.page == NeverlosePage::Inventory);
        nav(6, ICON_FA_SLIDERS_H, "Misc", 262 + shift, NeverlosePage::Miscellaneous, state.page == NeverlosePage::Miscellaneous);

        const ImVec2 account = base + ImVec2(7, 531);
        if (Hit("##account", account, ImVec2(140, 38))) state.account = !state.account;
        const float ar = Motion(ImGui::GetItemID() ^ 0x1932u, state.account ? 1.0f : ImGui::IsItemHovered() ? 0.5f : 0.0f);
        if (ar > 0.001f) d->AddRectFilled(account, account + ImVec2(140, 38), C(39, 43, 54, (int)(235 * ar)), 6);
        if (avatar) d->AddImageRounded(avatar, account + ImVec2(7, 5), account + ImVec2(35, 33), ImVec2(0,0), ImVec2(1,1), C(255,255,255), 14);
        Text(d, account + ImVec2(43, 4), C(225, 227, 233), "XPTNotFound", kTextControl);
        Text(d, account + ImVec2(43, 20), C(111, 116, 128), "3 days left", kTextSmall);
        Chevron(d, account + ImVec2(132, 15), C(181, 185, 195));
    }

    void Toolbar(ImDrawList* d, ImVec2 base)
    {
        d->AddRectFilled(base + ImVec2(158, 0), base + ImVec2(748, 56), C(11, 13, 20, 225), 14, ImDrawFlags_RoundCornersTopRight);
        d->AddLine(base + ImVec2(158, 56), base + ImVec2(748, 56), C(27, 30, 39));
        const ImVec2 profile = base + ImVec2(169, 14);
        d->AddRectFilled(profile, profile + ImVec2(166, 30), C(17, 19, 27), 6);
        d->AddRect(profile, profile + ImVec2(166, 30), C(28, 31, 41), 6);
        TextY(d, profile.x + 12, profile.y, 30, C(194, 197, 206), ICON_FA_SAVE, kTextIcon);
        TextY(d, profile.x + 48, profile.y, 30, C(184, 187, 197), "Garena CODM", kTextControl);
        Chevron(d, profile + ImVec2(148, 11), C(130, 135, 146));
        if (state.page == NeverlosePage::Rage || state.page == NeverlosePage::Legit)
        {
            const ImVec2 global = base + ImVec2(348, 14);
            d->AddRectFilled(global, global + ImVec2(85, 30), C(17, 19, 27), 6);
            d->AddRect(global, global + ImVec2(85, 30), C(28, 31, 41), 6);
            TextY(d, global.x + 13, global.y, 30, C(184, 187, 197), "Global", kTextControl);
            Chevron(d, global + ImVec2(68, 11), C(130, 135, 146));
        }
        // The round button closes the menu; nl_ui draws the reopen chip while hidden.
        if (Hit("##menu_close", base + ImVec2(706, 15), ImVec2(28, 28)))
            nl_ui::SetVisible(false);
        d->AddCircle(base + ImVec2(719, 28), 5, C(180, 184, 194), 0, 2);
        d->AddLine(base + ImVec2(723, 32), base + ImVec2(727, 36), C(180, 184, 194), 2);
    }

    void Rage(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row main[] = {
            {"Aimbot 360",      NeverloseControl::Toggle, nullptr, 0, nl_bind::Aim360, nullptr, 0, 0, 1, nullptr},
            {"Bullet Track",    NeverloseControl::Toggle, nullptr, 0, nl_bind::AimSilent, nullptr, 0, 0, 1, nullptr},
            {"Aim Assist Size", NeverloseControl::Slider, "0",     0, nl_bind::AimAssistSize, nullptr, 0, 0.0f, 100.0f, "%.0f"},
            {"Target",          NeverloseControl::Select, "Head",  0, nl_bind::AimTarget,  nullptr, 0, 0, 1, nullptr},
            {"Trigger",         NeverloseControl::Select, "None",  0, nl_bind::AimTrigger, nullptr, 0, 0, 1, nullptr},
            {"Target By",       NeverloseControl::Select, "Dist",  0, nl_bind::AimBy,      nullptr, 0, 0, 1, nullptr},
        };
        static const Row trigger[] = {
            {"Triggerbot",          NeverloseControl::Toggle, nullptr, 0, nl_bind::TriggerEnable, nullptr, 0, 0, 1, nullptr},
            {"Activation Mode",     NeverloseControl::Select, "Always",0, nl_bind::TriggerMode, nullptr, 0, 0, 1, nullptr},
            {"FOV Size",            NeverloseControl::Slider, "0",     0, nl_bind::AimFov, nullptr, 0, 0.0f, 100.0f, "%.0f"},
        };
        static const Row selection[] = {
            {"Hitbox",          NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraHit, nullptr, 0, 0, 1, nullptr},
            {"No Recoil",       NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraRecoil, nullptr, 0, 0, 1, nullptr},
            {"No Spread",       NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraSpread, nullptr, 0, 0, 1, nullptr},
            {"No Shake",        NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraShake, nullptr, 0, 0, 1, nullptr},
            {"No Overheat",     NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraRpd, nullptr, 0, 0, 1, nullptr},
            {"No Parachute",    NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraParachute, nullptr, 0, 0, 1, nullptr},
        };
        static const Row aa[] = {
            {"Anti Flashbang",  NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraFlash, nullptr, 0, 0, 1, nullptr},
            {"Firerate",        NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraFire, nullptr, 0, 0, 1, nullptr},
            {"Unlimited Fly",   NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraUnlimitedFly, nullptr, 0, 0, 1, nullptr},
            {"Fast Dive",       NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraDiving, nullptr, 0, 0, 1, nullptr},
            {"Fast Reload",     NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraReload, nullptr, 0, 0, 1, nullptr},
            {"Fast Scope",      NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraScope, nullptr, 0, 0, 1, nullptr},
        };
        const ImVec2 a=b+ImVec2(167,84), c=b+ImVec2(458,84), e=b+ImVec2(167,340), g=b+ImVec2(458,340);
        Card(d,a,ImVec2(281,6*37+22),"AIMBOT"); Card(d,c,ImVec2(277,3*37+22),"TRIGGERBOT"); Card(d,e,ImVec2(281,6*37+22),"EXTRA"); Card(d,g,ImVec2(277,6*37+22),"EXTRA");
        DrawRows(d,a,281,main,6,id); DrawRows(d,c,277,trigger,3,id); DrawRows(d,e,281,selection,6,id); DrawRows(d,g,277,aa,6,id);
    }

    void Legit(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row aim[]={
            {"Quick Switch",      NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraSwitch, nullptr, 0, 0, 1, nullptr},
            {"Weapon Kinetic",    NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraKinetic, nullptr, 0, 0, 1, nullptr},
            {"Mythic Attachments",NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraMythic, nullptr, 0, 0, 1, nullptr},
            {"No Slide Fatigue",  NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoSlideFatigue, nullptr, 0, 0, 1, nullptr},
            {"No Crouch",         NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoCrouch, nullptr, 0, 0, 1, nullptr},
            {"No Hit Flinch",     NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoHitFlinch, nullptr, 0, 0, 1, nullptr},
            {"No Drift (Hardscope)", NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoDrift, nullptr, 0, 0, 1, nullptr},
            {"No Sprint Fire Delay", NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoSprintFireDelay, nullptr, 0, 0, 1, nullptr},
            {"No Hipfire Delay",  NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoHipfire, nullptr, 0, 0, 1, nullptr},
        };
        const ImVec2 topc=b+ImVec2(167,84);
        Card(d,topc,ImVec2(281,9*37+22),"EXTRA");
        DrawRows(d,topc,281,aim,9,id);

        const ImVec2 tr=b+ImVec2(458,84);
        Card(d,tr,ImVec2(277,4*37+22),"MOVEMENT");
        const Row trig2[] = {
            {"Snowboard Speed", NeverloseControl::Slider, "0.0", 0, nl_bind::SliderSnowboard, nullptr, 0, 0.0f, 100.0f, "%.1f"},
            {"Slide Distance",  NeverloseControl::Slider, "0.0", 0, nl_bind::SliderSlide, nullptr, 0, 0.0f, 30.0f, "%.1f"},
            {"SpeedHack",       NeverloseControl::Slider, "1.0x",0, nl_bind::SliderSpeed, nullptr, 0, 0.5f, 2.0f, "%.2fx"},
            {"High Jump",       NeverloseControl::Slider, "1.00x",0, nl_bind::SliderJump, nullptr, 0, 0.5f, 5.0f, "%.2fx"},
        };
        DrawRows(d,tr,277,trig2,4,id);
    }

    void Soldier(ImDrawList* d, ImVec2 p)
    {
        const ImU32 blue=C(91,133,255,210), edge=C(211,220,255,235), dark=C(39,49,77,245);
        d->AddCircleFilled(p+ImVec2(0,-120),22,dark); d->AddCircle(p+ImVec2(0,-120),22,edge,0,2);
        d->AddRectFilled(p+ImVec2(-28,-96),p+ImVec2(28,-25),dark,12); d->AddRect(p+ImVec2(-28,-96),p+ImVec2(28,-25),blue,12,0,3);
        d->AddQuadFilled(p+ImVec2(-25,-85),p+ImVec2(-48,-30),p+ImVec2(-37,-22),p+ImVec2(-10,-66),dark);
        d->AddQuadFilled(p+ImVec2(25,-85),p+ImVec2(51,-45),p+ImVec2(41,-35),p+ImVec2(10,-66),dark);
        d->AddRectFilled(p+ImVec2(-23,-25),p+ImVec2(-4,60),dark,8); d->AddRectFilled(p+ImVec2(4,-25),p+ImVec2(23,60),dark,8);
        d->AddLine(p+ImVec2(-55,-42),p+ImVec2(62,-53),edge,5); d->AddRectFilled(p+ImVec2(30,-58),p+ImVec2(77,-49),blue,2);
        d->AddCircle(p+ImVec2(0,-120),10,blue,0,2); d->AddLine(p+ImVec2(-20,-115),p+ImVec2(20,-115),blue,2);
        d->AddLine(p+ImVec2(-35,62),p+ImVec2(-4,62),edge,3); d->AddLine(p+ImVec2(4,62),p+ImVec2(35,62),edge,3);
    }

    void Players(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row enemy[]={
            {"ESP Line",   NeverloseControl::Toggle, nullptr, 0, nl_bind::EspLine, nullptr, 0, 0, 1, nullptr},
            {"ESP Box",    NeverloseControl::Toggle, nullptr, 0, nl_bind::EspBox, nullptr, 0, 0, 1, nullptr},
            {"ESP Skeleton", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspSkeleton, nullptr, 0, 0, 1, nullptr},
            {"ESP Health", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspHealth, nullptr, 0, 0, 1, nullptr},
            {"ESP Name",   NeverloseControl::Toggle, nullptr, 0, nl_bind::EspName, nullptr, 0, 0, 1, nullptr},
            {"ESP Distance", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspDistance, nullptr, 0, 0, 1, nullptr},
        };
        static const Row model[]={
            {"Box Type",        NeverloseControl::Select, "Fill", 0, nl_bind::SelectBoxType,   nullptr, 0, 0, 1, nullptr},
            {"Line Position",   NeverloseControl::Select, "Top",  0, nl_bind::SelectLinePos,   nullptr, 0, 0, 1, nullptr},
            {"Health Position", NeverloseControl::Select, "Top",  0, nl_bind::SelectHealthPos, nullptr, 0, 0, 1, nullptr},
            {"ESP Style",       NeverloseControl::Select, "None", 0, nl_bind::SelectEspStyle,  nullptr, 0, 0, 1, nullptr},
            {"Yellow Wallhack", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspYellowWallhack, nullptr, 0, 0, 1, nullptr},
            {"Red Wallhack",    NeverloseControl::Toggle, nullptr, 0, nl_bind::EspRedWallhack, nullptr, 0, 0, 1, nullptr},
            {"360 Alert",       NeverloseControl::Toggle, nullptr, 0, nl_bind::EspAlert, nullptr, 0, 0, 1, nullptr},
        };
        ImVec2 a=b+ImVec2(177,86), m=b+ImVec2(177,352);
        Card(d,a,ImVec2(300,6*37+22),"ESP"); Card(d,m,ImVec2(300,7*37+22),"DISPLAY");
        DrawRows(d,a,300,enemy,6,id); DrawRows(d,m,300,model,7,id);
        const ImVec2 e2=b+ImVec2(520,352);
        static const Row extra2[]={
            {"ESP Count", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspCount, nullptr, 0, 0, 1, nullptr},
            {"Show AimLine", NeverloseControl::Toggle, nullptr, 0, nl_bind::EspAimline, nullptr, 0, 0, 1, nullptr},
        };
        Card(d,e2,ImVec2(214,2*37+22),"INFO");
        DrawRows(d,e2,214,extra2,2,id);
    }

    void World(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row view[]={
            {"Dot Crosshair", NeverloseControl::Toggle, nullptr, 0, nl_bind::DotEnabled, nullptr, 0, 0, 1, nullptr},
            {"Dot Opacity", NeverloseControl::Slider, "0.95", 0, nl_bind::DotOpacity, nullptr, 0, 0.0f, 1.0f, "%.2f"},
            {"Dot Size (trigger zone)", NeverloseControl::Slider, "6", 0, nl_bind::DotSize, nullptr, 0, 1.0f, 40.0f, "%.0f"},
            {"Dot Offset X", NeverloseControl::Slider, "0", 0, nl_bind::DotOffsetX, nullptr, 0, -400.0f, 400.0f, "%.0f"},
            {"Dot Offset Y", NeverloseControl::Slider, "0", 0, nl_bind::DotOffsetY, nullptr, 0, -400.0f, 400.0f, "%.0f"},
        };
        static const Row esp[]={
            {"Hidden overlay (privacy stream)", NeverloseControl::Toggle, nullptr, 0, nl_bind::MiscHideStream, nullptr, 0, 0, 1, nullptr},
        };
        ImVec2 a=b+ImVec2(177,84), c=b+ImVec2(478,84);
        Card(d,a,ImVec2(291,5*37+22),"DOT CROSSHAIR"); Card(d,c,ImVec2(257,1*37+22),"OVERLAY");
        DrawRows(d,a,291,view,5,id); DrawRows(d,c,257,esp,1,id);
    }

    void WeaponCell(ImDrawList* d, ImVec2 p, ImVec2 size, int kind, ImU32 accent)
    {
        d->AddRectFilled(p,p+size,C(24,31,45,235),9); d->AddRect(p,p+size,C(35,45,62),9); d->AddRectFilled(p+ImVec2(0,size.y-3),p+size,accent,0);
        const ImU32 gun=C(218,220,218),shade=C(104,111,118);
        const float s = ImMin(size.x / 121.0f, size.y / 91.0f);
        auto q = [&](float x, float y) { return p + ImVec2(x * s, y * s); };
        if (kind%3==0){d->AddRectFilled(q(25,28),q(86,36),gun,2*s);d->AddRectFilled(q(67,35),q(77,60),shade,2*s);d->AddRectFilled(q(32,35),q(41,55),gun,2*s);}
        else if(kind%3==1){d->AddLine(q(18,55),q(94,27),gun,7*s);d->AddRectFilled(q(70,27),q(90,35),shade,2*s);}
        else{d->AddRectFilled(q(16,32),q(96,39),gun,2*s);d->AddRectFilled(q(30,39),q(42,61),shade,2*s);d->AddRectFilled(q(67,39),q(77,57),gun,2*s);}
    }

    void Inventory(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row skins[]={
            {"Unlock Blueprints", NeverloseControl::Toggle, nullptr, 0, nl_bind::SkinBlueprints, nullptr, 0, 0, 1, nullptr},
            {"Apply Skin On Equip", NeverloseControl::Toggle, nullptr, 0, nl_bind::SkinAutoEquip, nullptr, 0, 0, 1, nullptr},
        };
        d->AddRectFilled(b+ImVec2(159,57),b+ImVec2(747,575),C(17,27,42,155),0);
        const ImVec2 sc=b+ImVec2(185,66);
        Card(d,sc,ImVec2(200,2*37+22),"SKINS");
        DrawRows(d,sc,200,skins,2,id);
        const ImU32 accents[]={C(235,237,239),C(171,70,255),C(75,116,255),C(226,43,192),C(251,65,83),C(134,72,255)};
        for(int r=0;r<5;++r)for(int c=0;c<3;++c)WeaponCell(d,b+ImVec2(390+c*116.0f,88+r*96.0f),ImVec2(108,91),r*3+c,accents[(r+c)%6]);
    }

    void Misc(ImDrawList* d, ImVec2 b, int& id)
    {
        static const Row actions[]={
            {"Save Config",   NeverloseControl::Chevron, nullptr, 0, nl_bind::ActSaveConfig, nullptr, 0, 0, 1, nullptr},
            {"Load Config",   NeverloseControl::Chevron, nullptr, 0, nl_bind::ActLoadConfig, nullptr, 0, 0, 1, nullptr},
            {"Reset Banned Guest", NeverloseControl::Chevron, nullptr, 0, nl_bind::ActGuestReset, nullptr, 0, 0, 1, nullptr},
            {"Block Report Evidence", NeverloseControl::Toggle, nullptr, 0, nl_bind::MiscBlockReport, nullptr, 0, 0, 1, nullptr},
        };
        static const Row features[]={
            {"Spectate Anyone",  NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraSpectate, nullptr, 0, 0, 1, nullptr},
            {"Walk Under Water", NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraWalkUnderWater, nullptr, 0, 0, 1, nullptr},
            {"No Smoke",         NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoSmoke, nullptr, 0, 0, 1, nullptr},
            {"No Gravity",       NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraNoGravity, nullptr, 0, 0, 1, nullptr},
            {"Smart Reload",     NeverloseControl::Toggle, nullptr, 0, nl_bind::ExtraSmartReload, nullptr, 0, 0, 1, nullptr},
        };
        ImVec2 a=b+ImVec2(171,80), c=b+ImVec2(481,80);
        Card(d,a,ImVec2(300,4*37+22),"CONFIG"); Card(d,c,ImVec2(254,5*37+22),"FEATURES");
        DrawRows(d,a,300,actions,4,id); DrawRows(d,c,254,features,5,id);
    }

    void AccountPopover(ImDrawList* d, ImVec2 b, ImTextureID avatar)
    {
        const float open=Motion(ImGui::GetID("##account_open"),state.account?1.0f:0.0f,17.0f,0.0f);if(open<.002f)return;
        const ImVec2 p=b+ImVec2(151,380), size(218,181); const int first=d->VtxBuffer.Size;
        d->AddRectFilled(p-ImVec2(4,2),p+size+ImVec2(4,7),C(0,0,0,65),18);d->AddRectFilled(p,p+size,C(24,25,34,245),16);d->AddRect(p,p+size,C(48,51,64),16);
        if(avatar)d->AddImageRounded(avatar,p+ImVec2(17,16),p+ImVec2(53,52),ImVec2(0,0),ImVec2(1,1),C(255,255,255),18);
        Text(d,p+ImVec2(64,15),C(229,231,237),"XPTNotFound",kTextBody);Text(d,p+ImVec2(64,34),C(115,164,255),"Renew subscription",kTextControl);
        const char* rows[]={"Language","Menu Scale","ESP Scale","Synchronization"};for(int i=0;i<4;++i){float y=p.y+66+i*28;TextY(d,p.x+18,y,28,C(185,188,198),rows[i],kTextControl);if(i<3)Chevron(d,ImVec2(p.x+195,y+11),C(150,154,165));}Toggle(d,190,nullptr,p+ImVec2(169,150));
        for(int i=first;i<d->VtxBuffer.Size;++i){ImDrawVert&v=d->VtxBuffer[i];const ImU32 a=(v.col>>24)&255u;v.col=(v.col&0xffffffu)|((ImU32)(a*open)<<24);}
    }
}

void RenderNeverloseMenu(ImTextureID avatar, float reveal_progress)
{
    ImGui::SetNextWindowPos(ImVec2(0,0),ImGuiCond_Always);ImGui::SetNextWindowSize(kCanvas,ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);ImGui::PushStyleColor(ImGuiCol_WindowBg,C(0,0,0,0));
    ImGuiWindowFlags flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoBringToFrontOnFocus;
    const float progress=ImClamp(reveal_progress,0.0f,1.0f);if(progress<1.0f)flags|=ImGuiWindowFlags_NoInputs;
    if(ImGui::Begin("Neverlose Menu",nullptr,flags))
    {
        ImVec2 b=ImGui::GetWindowPos();ImDrawList*d=ImGui::GetWindowDrawList();const int shell_first=d->VtxBuffer.Size;
        draw_blur_region(d,b,b+kShell,14);d->AddRectFilled(b,b+kShell,C(13,15,22,222),14);Sidebar(d,b,avatar);Toolbar(d,b);
        const int content_first=d->VtxBuffer.Size;int id=0;ImGui::PushID((int)state.page);
        switch(state.page){case NeverlosePage::Rage:Rage(d,b,id);break;case NeverlosePage::Legit:Legit(d,b,id);break;case NeverlosePage::Players:Players(d,b,id);break;case NeverlosePage::World:World(d,b,id);break;case NeverlosePage::Inventory:Inventory(d,b,id);break;case NeverlosePage::Miscellaneous:Misc(d,b,id);break;}
        ImGui::PopID();state.page_mix=ImLerp(state.page_mix,1.0f,1.0f-std::exp(-15.0f*ImGui::GetIO().DeltaTime));const float pe=1.0f-std::pow(1.0f-state.page_mix,3.0f);
        for(int i=content_first;i<d->VtxBuffer.Size;++i){ImDrawVert&v=d->VtxBuffer[i];v.pos+=ImVec2(9.0f*(1.0f-pe),0);const ImU32 a=(v.col>>24)&255u;v.col=(v.col&0xffffffu)|((ImU32)(a*pe)<<24);}
        AccountPopover(d,b,avatar);PopupLayer(d);
        if(progress<1.0f){const float e=1.0f-std::pow(1.0f-progress,4.0f),scale=ImLerp(.92f,1.0f,e);const ImVec2 pivot=b+kCanvas*.5f;for(int i=shell_first;i<d->VtxBuffer.Size;++i){ImDrawVert&v=d->VtxBuffer[i];v.pos=pivot+(v.pos-pivot)*scale+ImVec2(0,(1-e)*16);const ImU32 a=(v.col>>24)&255u;v.col=(v.col&0xffffffu)|((ImU32)(a*e)<<24);}}
    }
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar(2);
}
