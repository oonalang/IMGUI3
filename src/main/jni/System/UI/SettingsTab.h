#pragma once

#include "ImGui/Call_ImGui.h"
#include "../Core/SaveConfig.h"
#include "Logo.h"
#include "../ImGui/imgui_settings.h"
#include "../ImGui/custom_widgets.hpp"

extern ImFont* F50;
extern ImFont* F48;
extern ImFont* JAAT;
extern float menu[4];
extern std::string usedKey;
extern std::string EXP;
extern std::string userType;

namespace settings_tab
{
    inline void ContentGap(float height = 6.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, height));
    }

    inline std::string BuildRuntimeExpiryLabel()
    {
        std::string label = EXP;
        const std::string countdown = getExpiryCountdown();
        if (!countdown.empty())
        {
            if (!label.empty())
                label += " ";
            label += "(" + countdown + ")";
        }
        return label;
    }

    inline void RenderLicenseCard()
    {
        const float sidePad = 15.0f;
        auto drawLine = [sidePad](const char *name, const char *value, const ImVec4 &valueColor) {
            ImGui::SetCursorPosX(sidePad);
            ImGui::TextColored(c::text::text_active, "%s", name);
            ImGui::SameLine(0.0f, 15.0f);
            ImGui::TextColored(valueColor, ": %s", value ? value : "");
            ContentGap();
        };

        const std::string expiryLabel = BuildRuntimeExpiryLabel();
        const ImVec4 subColor = (userType == "PREMIUM PAID")
            ? ImVec4(0.75f, 0.60f, 0.00f, 0.86f)
            : ImVec4(0.00f, 0.80f, 0.00f, 0.86f);

        drawLine("LICENSE", usedKey.c_str(), c::text::text);
        drawLine("EXPIRY", expiryLabel.c_str(), c::text::text);
        drawLine("SUBSCRIPTION", userType.c_str(), subColor);
    }

    inline void RenderLogoCard()
    {
        float opacity = GetLogoOpacity();
        float size = GetLogoSizeMultiplier();
        custom::SliderFloat("Opacity", &opacity, 0.0f, 1.0f, "%.2f");
        custom::SliderFloat("Size", &size, 0.1f, 2.0f, "%.2f");
        SetLogoOpacity(opacity);
        SetLogoSizeMultiplier(size);
        if (custom::Button("RESET LOGO", ImVec2(ImMax(1.0f, ImGui::GetContentRegionAvail().x), 50.0f)))
        {
            SetLogoOpacity(1.0f);
            SetLogoSizeMultiplier(1.0f);
        }
    }

    inline void RenderConfigCard()
    {
        // Real JSON profile via SaveConfig.h (files/configs/default.json).
        // This card used to claim config was "managed via the side panel"
        // with SAV/LOD/RST buttons that never existed anywhere in the menu.
        const float sidePad = 15.0f;
        ImGui::SetCursorPosX(sidePad);
        ImGui::TextColored(c::text::text, "Profile: default");
        ContentGap(4.0f);
        const float btnW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - sidePad * 2.0f);
        ImGui::SetCursorPosX(sidePad);
        if (custom::Button("SAVE CONFIG", ImVec2(btnW, 50.0f)))
        {
            SaveConfiguration("default");
        }
        ContentGap(4.0f);
        ImGui::SetCursorPosX(sidePad);
        if (custom::Button("LOAD CONFIG", ImVec2(btnW, 50.0f)))
        {
            LoadConfiguration("default");
        }
    }

    inline void RenderEnhancementCard()
    {
        custom::Checkbox("Clear Display", &Config.ExtraMenu.ClearDisplay);
        custom::Checkbox("VSync Mode", &Config.ExtraMenu.Grap);
        if (custom::Checkbox("Frame Smoothness", &Config.Aim.FpsLevel)) {
            Config.Aim.showFPSLevelSlider = Config.Aim.FpsLevel;
        }
        if (Config.Aim.showFPSLevelSlider) {
            custom::SliderFloat("Frame Level", &Config.Aim.FpsLevel1, 30, 120, "%.0f");
        }
        ImGui::Separator();
        if (custom::Checkbox("Ultra Frame Boost", &Config.Aim.FpsLevelUltra)) {
            Config.Aim.showFPSLevelUltraSlider = Config.Aim.FpsLevelUltra;
        }
        if (Config.Aim.showFPSLevelUltraSlider) {
            custom::SliderFloat("Ultra Frame Level", &Config.Aim.FpsLevelUltra1, 60, 144, "%.0f");
        }
        if (custom::Button("RESET GUEST", ImVec2(ImMax(1.0f, ImGui::GetContentRegionAvail().x), 50.0f)))
            Config.ExtraMenu.ResetGuest = true;
    }
}
