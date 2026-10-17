#pragma once

#include "ImGui/Call_ImGui.h"
#include "../Core/SaveConfig.h"
#include "Logo.h"
#include "../ImGui/imgui_settings.h"
#include "../ImGui/custom_widgets.hpp"
// ReportBlock.h brings the toggle's own on-device counters into this tab.
#include "../Hooks/ReportBlock.h"

extern ImFont* F50;
extern ImFont* F48;
extern ImFont* JAAT;
extern float menu[4];
extern std::string usedKey;
extern std::string EXP;
extern std::string userType;

void RenderLicenseInfo(ImDrawList* draw, const ImVec2& startPos)
{
    const ImVec4 titleColor = c::text::text_active;
    const ImVec4 valueColor = c::text::text;

    ImGui::SetCursorPosX(15);
    ImGui::PushFont(F50);
    ImGui::SetWindowFontScale(0.6f);
    ImGui::TextColored(titleColor, "LICENSE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    
    if (!usedKey.empty())
    {
        ImGui::SameLine(0, 15.0f);
        ImGui::TextColored(valueColor, ": %s", usedKey.c_str());
    }
    
    custom::Separator_line();
    
    
    std::string countdownText = getExpiryCountdown();
    
    ImGui::SetCursorPosX(15);
    ImGui::PushFont(F50);
    ImGui::SetWindowFontScale(0.6f);
    ImGui::TextColored(titleColor, "EXPIRY");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    
    if (!EXP.empty())
    {
        ImGui::SameLine(0, 15.0f);
        ImGui::TextColored(valueColor, ": %s", EXP.c_str());
        
        if (!countdownText.empty())
        {
            ImGui::SetCursorPosX(15);
            ImGui::TextColored(valueColor, "                 (%s)", countdownText.c_str());
        }
    }
    
    custom::Separator_line();
    
    
    ImGui::SetCursorPosX(15);
    ImGui::PushFont(F50);
    ImGui::SetWindowFontScale(0.6f);
    ImGui::TextColored(titleColor, "SUBSCRIPTION");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    
    if (!userType.empty())
    {
        ImGui::SameLine(0, 15.0f);
        if (userType == "PREMIUM PAID")
        {
            ImGui::TextColored(ImColor(191, 153, 0, 165), ": %s", userType.c_str());
        }
        else
        {
            ImGui::TextColored(ImColor(0, 204, 0, 165), ": %s", userType.c_str());
        }
    }
}

void RenderLogoSettings(ImDrawList* draw)
{
    float tempOpacity = GetLogoOpacity();
    float tempSize = GetLogoSizeMultiplier();
    
    custom::SliderFloat("Opacity", &tempOpacity, 0.0f, 1.0f, "%.2f");
    SetLogoOpacity(tempOpacity);

    custom::SliderFloat("Size", &tempSize, 0.1f, 2.0f, "%.2f");
    SetLogoSizeMultiplier(tempSize);

    const float buttonHeight = 50.0f;
    const float buttonInsetX = 10.0f;
    ImGui::SetCursorPosX(buttonInsetX);
    ImVec2 buttonSize = ImVec2(ImMax(1.0f, ImGui::GetContentRegionAvail().x - buttonInsetX), buttonHeight);
    
    if (custom::Button("RESET LOGO", buttonSize))
    {
        SetLogoOpacity(1.0f);
        SetLogoSizeMultiplier(1.0f);
    }
}

void RenderConfigManagement(ImDrawList* draw) {}

void RenderEnhancement()
{
    custom::Checkbox("Clear Display", &Config.ExtraMenu.ClearDisplay);
    custom::Separator_line();
    custom::Checkbox("Reset Guest", &Config.ExtraMenu.ResetGuest);
    custom::Separator_line();

    const float row_x = ImGui::GetCursorPosX();
    const float row_w = ImGui::GetContentRegionAvail().x;
    const float preview_w = ImGui::GetFrameHeight() * c::scale;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Menu Accent");
    ImGui::SameLine();
    ImGui::SetCursorPosX(row_x + row_w - preview_w);
    ImGui::SetNextItemWidth(preview_w);
    custom::ColorEdit4(
        "##menu_accent_preview",
        menu,
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_PickerHueWheel
    );

    menu[3] = 1.0f;
}

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
        // Config load/save is handled by the side panel (SAV/LOD buttons).
        // Nothing to render here — kept for layout spacing only.
        const float sidePad = 15.0f;
        ImGui::SetCursorPosX(sidePad);
        ImGui::TextColored(c::text::text, "Config is managed");
        ContentGap(2.0f);
        ImGui::SetCursorPosX(sidePad);
        ImGui::TextColored(c::text::text, "via the side panel.");
        ContentGap(2.0f);
        ImGui::SetCursorPosX(sidePad);
        ImGui::TextColored(c::text::text_active, "SAV  /  LOD  /  RST");
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

    // PRIVACY - "Block Report Evidence" (System/Hooks/ReportBlock.h). ON by
    // default: the client's own report / evidence senders are swallowed, so a
    // report against this account never uploads the payload from this phone.
    // The counter line is the on-device proof that the four hooks are live: it
    // stays at 0/0 while the game never called them, and with the toggle OFF
    // every call goes straight to the game's own function.
    inline void RenderPrivacyCard()
    {
        custom::Checkbox("Block Report Evidence", &Config.ExtraMenu.BlockReport);

        int seen = 0;
        int blocked = 0;
        int landed = 0;
        for (int i = 0; i < report_block::GateCount; i++)
        {
            seen    += report_block::g_diag.seen[i];
            blocked += report_block::g_diag.blocked[i];
            if (report_block::g_landed[i]) landed++;
        }

        const float sidePad = 15.0f;
        ImGui::SetCursorPosX(sidePad);
        ImGui::TextColored(c::text::text, "blocked %d / %d  |  hooks %d/%d",
            blocked, seen, landed, (int)report_block::GateCount);
    }
}
