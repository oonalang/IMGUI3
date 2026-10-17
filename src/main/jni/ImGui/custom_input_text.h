#pragma once

#include "custom_widget_utils.h"
#include "ime_target.h"

#include <map>

namespace custom
{
    struct input_state
    {
        float focus_t;
        float hover_t;
    };

    inline bool InputTextImpl(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, const ImVec2& size_arg)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;
        if (buf == nullptr || buf_size == 0)
            return false;

        ImGuiContext& g = *GImGui;
        const float control_scale = c::scale * c::widget_scale;
        const ImGuiID id = window->GetID(label);

        static std::map<ImGuiID, input_state> anim;
        auto it_anim = anim.find(id);
        if (it_anim == anim.end())
        {
            anim.insert({ id, input_state() });
            it_anim = anim.find(id);
        }

        const float height = size_arg.y > 0.0f ? size_arg.y : 40.0f * control_scale;
        const ImVec2 size(size_arg.x > 0.0f ? size_arg.x : GetContentRegionAvail().x, height);

        ImDrawList* draw = window->DrawList;
        draw->ChannelsSplit(2);
        draw->ChannelsSetCurrent(1);

        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        PushStyleVar(ImGuiStyleVar_FrameRounding, c::elements::rounding);
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * control_scale, ImMax(0.0f, (height - GetFontSize()) * 0.5f)));
        PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleColor(ImGuiCol_Text, c::text::text_active);
        PushStyleColor(ImGuiCol_TextDisabled, WithAlpha(c::text::text, 0.72f));
        PushStyleColor(ImGuiCol_TextSelectedBg, WithAlpha(c::accent, 0.35f));
        PushStyleColor(ImGuiCol_NavHighlight, c::accent);

        const bool value_changed = InputTextEx(label, hint, buf, (int)buf_size, size, flags);

        const bool hovered = IsItemHovered();
        const bool active = IsItemActive();
        const ImVec2 frame_min = GetItemRectMin();
        const ImVec2 frame_max = GetItemRectMax();

        if (active)
            custom_ime::Remember(window, id);

        PopStyleColor(8);
        PopStyleVar(3);

        const float dt = g.IO.DeltaTime;
        it_anim->second.hover_t = ImLerp(it_anim->second.hover_t, hovered ? 1.0f : 0.0f, dt * 12.0f);
        it_anim->second.focus_t = ImLerp(it_anim->second.focus_t, active ? 1.0f : 0.0f, dt * 14.0f);

        draw->ChannelsSetCurrent(0);
        const ImVec4 background = MixColor(
            MixColor(c::widget::background, c::button::background_hovered, it_anim->second.hover_t),
            c::elements::background_hovered, it_anim->second.focus_t);
        const ImU32 border = GetColorU32(MixColor(c::widget::outlinecolor, c::accent, it_anim->second.focus_t));
        draw->AddRectFilled(frame_min, frame_max, GetColorU32(background), c::elements::rounding);
        draw->AddRect(frame_min, frame_max, border, c::elements::rounding, 0, ImLerp(1.0f, 2.0f, it_anim->second.focus_t));

        if (it_anim->second.focus_t > 0.01f)
        {
            const float width = ImMax(24.0f * control_scale, (frame_max.x - frame_min.x) * 0.32f) * it_anim->second.focus_t;
            draw->AddRectFilled(
                ImVec2(frame_min.x, frame_max.y - 2.0f * control_scale),
                ImVec2(frame_min.x + width, frame_max.y),
                GetColorU32(WithAlpha(c::accent, it_anim->second.focus_t)),
                2.0f * control_scale);
        }
        draw->ChannelsMerge();

        if (value_changed)
            MarkItemEdited(id);

        return value_changed;
    }

    inline bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, const ImVec2& size_arg)
    {
        return InputTextImpl(label, NULL, buf, buf_size, flags, size_arg);
    }

    inline bool InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, const ImVec2& size_arg)
    {
        return InputTextImpl(label, hint, buf, buf_size, flags, size_arg);
    }
}
