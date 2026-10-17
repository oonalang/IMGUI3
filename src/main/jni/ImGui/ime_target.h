#pragma once

#include "imgui.h"
#include "imgui_internal.h"

// The text field the on-screen keyboard is typing into. The textbox records
// itself while it is active; the keyboard uses that record to hand the active
// id back after a key tap, because the tap lands in the keyboard's own window
// and ImGui would otherwise move the active id off the field.
namespace custom_ime
{
    struct Target
    {
        ImGuiWindow* window = nullptr;
        ImGuiID      id = 0;
    };

    inline Target& Get()
    {
        static Target target;
        return target;
    }

    inline void Remember(ImGuiWindow* window, ImGuiID id)
    {
        Get().window = window;
        Get().id = id;
    }

    inline bool Has()
    {
        const Target& target = Get();
        return target.window != nullptr && target.id != 0;
    }

    inline void Forget()
    {
        Get() = Target();
    }

    inline bool Alive(ImGuiWindow* window)
    {
        for (ImGuiWindow* candidate : GImGui->Windows)
            if (candidate == window)
                return true;
        return false;
    }

    inline void Restore()
    {
        Target& target = Get();
        if (target.window == nullptr || target.id == 0)
            return;
        if (!Alive(target.window))
        {
            Forget();
            return;
        }
        ImGui::SetActiveID(target.id, target.window);
        ImGui::SetFocusID(target.id, target.window);
    }
}
