#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "menu_layout.h"
#include "raspody_style.h"
#include "raspody_widgets.h"
#include "System/UI/Keyboard.h"
#include "raspody_font.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>

extern bool  HIDEESP;
extern bool  UnlimitedFly;
extern float SnowBsize;
extern float SlideRange;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;

extern std::string usedKey;
extern std::string userType;

void SaveConfiguration(const std::string& filename);
bool LoadConfiguration(const std::string& filename);
std::vector<std::string> ListProfiles();

namespace raspody
{
    inline bool minimize_requested = false;

    inline int g_tabHistory[Tab_Count] = { 0 };
    inline int g_tabHistoryCount = 0;

    inline void PushTabHistory(int tab)
    {
        if (tab < 0 || tab >= Tab_Count)
            return;
        if (g_tabHistoryCount > 0 && g_tabHistory[g_tabHistoryCount - 1] == tab)
            return;
        if (g_tabHistoryCount < Tab_Count)
            g_tabHistory[g_tabHistoryCount++] = tab;
    }

    inline void RequestTab(int tab)
    {
        if (tab < 0 || tab >= Tab_Count || tab == var.tab.stored)
            return;

        PushTabHistory(var.tab.stored);
        var.tab.stored = tab;
        var.subtab.stored = 0;
        if (var.tab.current == tab)
        {
            var.tab.alpha = 1.0f;
            var.subtab.current = var.subtab.stored = 0;
        }
    }

    inline void GoBack()
    {
        if (var.subtab.stored != 0)
        {
            var.subtab.stored = 0;
            return;
        }

        while (g_tabHistoryCount > 0)
        {
            const int previous = g_tabHistory[--g_tabHistoryCount];
            if (previous != var.tab.stored)
            {
                RequestTab(previous);
                return;
            }
        }
    }

    inline std::string TrimName(const char* text)
    {
        if (!text)
            return std::string();

        const char* start = text;
        while (*start && std::isspace((unsigned char)*start))
            ++start;

        std::string out(start);
        while (!out.empty() && std::isspace((unsigned char)out.back()))
            out.pop_back();
        return out;
    }

    inline float TextRowHeight(float designHeight, float textSize, const char* text, float padY)
    {
        ImFont* font = fonts::small ? fonts::small : ImGui::GetFont();
        return ImMax(S(designHeight), TextSize(font, textSize, text ? text : "X").y + S(padY));
    }

    inline void InfoRow(const char* label, const char* value)
    {
        const ImVec2 pos = RowOrigin();
        const float width = CardInnerWidth();
        const float height = TextRowHeight(25.0f, 16.0f, label, 9.0f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        DrawTextClipped(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                        pos, ImVec2(pos.x + width * 0.52f, pos.y + height),
                        Col(clr.text2), label, ImVec2(0.0f, 0.5f));
        DrawTextClipped(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                        ImVec2(pos.x + width * 0.48f, pos.y), ImVec2(pos.x + width, pos.y + height),
                        Col(clr.white), value ? value : "", ImVec2(1.0f, 0.5f));

        ImGui::Dummy(ImVec2(width, height));
    }

    inline void ParagraphRow(const char* text)
    {
        const ImVec2 pos = RowOrigin();
        const float width = CardInnerWidth();
        ImFont* font = fonts::small ? fonts::small : ImGui::GetFont();
        const float size = 16.0f;
        const float lineH = TextSize(font, size, "Ag").y + S(2);

        std::vector<std::string> lines;
        std::string current;
        if (text)
        {
            std::string word;
            const char* p = text;
            for (;;)
            {
                const bool atEnd = (*p == '\0');
                if (!atEnd && *p != ' ')
                {
                    word.push_back(*p++);
                    continue;
                }

                if (!word.empty())
                {
                    const std::string candidate = current.empty() ? word : current + " " + word;
                    if (!current.empty() &&
                        TextSize(font, size, candidate.c_str()).x > width)
                    {
                        lines.push_back(current);
                        current = word;
                    }
                    else
                    {
                        current = candidate;
                    }
                    word.clear();
                }

                if (atEnd)
                    break;
                ++p;
            }
            if (!current.empty())
                lines.push_back(current);
        }

        const float textH = lineH * (float)lines.size();
        const float height = ImMax(S(24), textH + S(8));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        float y = pos.y + (height - textH) * 0.5f;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            DrawText(dl, font, size, ImVec2(pos.x, y), Col(clr.text), lines[i].c_str(), ImVec2(0.0f, 0.0f));
            y += lineH;
        }

        ImGui::Dummy(ImVec2(width, height));
    }

    inline void ColorGroup(float* line, float* a, float* b, float* c, float* d, float* e)
    {
        if (!line)
            return;

        for (int i = 0; i < 4; ++i)
            if (line[i] > 1.0f)
                line[i] /= 255.0f;

        float* targets[5] = { a, b, c, d, e };
        for (float* target : targets)
            if (target)
                std::memcpy(target, line, sizeof(float) * 4);
    }

    inline void DrawCrosshairPreview()
    {
        const ImVec2 pos = RowOrigin();
        const float width = CardInnerWidth();
        const float height = S(150);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.layout, 0.92f), S(14));
        for (int i = 1; i < 4; ++i)
        {
            const float x = pos.x + width * (float)i / 4.0f;
            const float y = pos.y + height * (float)i / 4.0f;
            dl->AddLine(ImVec2(x, pos.y + S(8)), ImVec2(x, pos.y + height - S(8)),
                        Col(clr.separator, 0.55f), 1.0f);
            dl->AddLine(ImVec2(pos.x + S(8), y), ImVec2(pos.x + width - S(8), y),
                        Col(clr.separator, 0.55f), 1.0f);
        }

        const float cx = pos.x + width * 0.5f + ImClamp(Config.Crosshair.OffsetX * 0.25f * g_scale,
                                                        -width * 0.40f, width * 0.40f);
        const float cy = pos.y + height * 0.5f + ImClamp(Config.Crosshair.OffsetY * 0.25f * g_scale,
                                                        -height * 0.40f, height * 0.40f);
        const float radius = ImClamp(Config.Crosshair.Size * 0.5f * g_scale * 0.7f, 1.5f, S(10));

        if (Config.Crosshair.Enabled)
        {
            const ImVec4 dot(Config.Crosshair.Color[0], Config.Crosshair.Color[1],
                             Config.Crosshair.Color[2], 1.0f);
            dl->AddCircleFilled(ImVec2(cx, cy), radius + S(0.5f), Col(clr.black, 0.35f), 20);
            dl->AddCircleFilled(ImVec2(cx, cy), radius, Col(dot), 20);
        }

        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                 ImVec2(pos.x + S(14), pos.y + S(13)), Col(clr.text2),
                 Config.Crosshair.Enabled ? "LIVE CROSSHAIR" : "CROSSHAIR DISABLED", ImVec2(0.0f, 0.5f));

        ImGui::Dummy(ImVec2(width, height + S(8)));
    }

    inline bool TextFieldRow(const char* id, char* buffer, int bufferSize, const char* placeholder)
    {
        const ImVec2 pos = RowOrigin();
        const float width = CardInnerWidth();
        const float height = ImMax(S(36), ImGui::GetFontSize() + S(16));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.widget), S(10));
        dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height), Col(clr.separator, 0.8f), S(10), 0, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, clr.white);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(6), S(7)));
        ImGui::PushFont(fonts::small ? fonts::small : ImGui::GetFont());

        ImGui::SetCursorScreenPos(ImVec2(pos.x + S(8), pos.y + S(2)));
        ImGui::SetNextItemWidth(width - S(16));
        const bool changed = ImGui::InputText(id, buffer, (size_t)ImMax(bufferSize, 1),
            ImGuiInputTextFlags_CharsNoBlank);
        const bool active = ImGui::IsItemActive();

        if (buffer && *buffer == '\0' && !active && placeholder)
        {
            DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), S(11),
                     ImVec2(pos.x + S(12), pos.y + height * 0.5f),
                     Col(clr.text), placeholder, ImVec2(0.0f, 0.5f));
        }

        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(5);

        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + height + S(8)));
        return changed || active;
    }

    struct PageLayout
    {
        ImVec2 origin = ImVec2(0, 0);
        float  width = 0.0f;
        float  gap = 0.0f;
        float  colW = 0.0f;
        bool   narrow = false;
        float  leftY = 0.0f;
        float  rightY = 0.0f;
        float  singleY = 0.0f;
    };

    inline PageLayout MakePageLayout(ImVec2 origin, float width)
    {
        PageLayout layout;
        layout.origin = origin;
        layout.width = width;
        layout.gap = S(14);
        layout.narrow = width < S(430);
        layout.colW = layout.narrow ? width : (width - layout.gap) * 0.5f;
        return layout;
    }

    template <typename Body>
    inline void AddPageCard(PageLayout& layout, int column, const char* title, Body&& body)
    {
        const float x = layout.narrow
            ? layout.origin.x
            : layout.origin.x + (column == 0 ? 0.0f : layout.colW + layout.gap);
        const float y = layout.narrow
            ? layout.singleY
            : (column == 0 ? layout.leftY : layout.rightY);
        const float width = layout.narrow ? layout.width : layout.colW;

        BeginCard(title, ImVec2(x, layout.origin.y + y), width);
        body();
        const float bottom = EndCard();
        const float next = bottom - layout.origin.y + layout.gap;

        if (layout.narrow)
            layout.singleY = next;
        else if (column == 0)
            layout.leftY = next;
        else
            layout.rightY = next;
    }

    inline float PageBottom(const PageLayout& layout)
    {
        if (layout.narrow)
            return layout.singleY;
        return ImMax(layout.leftY, layout.rightY);
    }

    inline void AimbotPage(PageLayout& p, int subtab)
    {
        static const char* triggers[] = { "None", "Shooting", "Scoping" };
        static const char* targets[] = { "Head", "Chest", "Body" };
        static const char* targetBy[] = { "Distance", "FOV" };
        static const char* triggerModes[] = { "Always", "ADS only", "Firing only" };

        if (subtab == 0)
        {
            AddPageCard(p, 0, "Aim control", [&] {
                Checkbox("Aim Assist 360", "Rotates the aim toward any nearby target.",
                         &Config.Aim.Aimbot360, true);
                CheckboxSettings("Aim Assist 360", &Config.Aim.AimSilent, "Silent aim (bullet track)",
                                 &Config.ESPMenu.ShowFov, "Show FOV circle");
                SliderFloat("Assist size", "Reach of the assist around the target.",
                            &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
            });

            AddPageCard(p, 1, "Targeting", [&] {
                Dropdown("Aim trigger", "When the assist is allowed to fire.",
                         (int*)&Config.Aim.Trigger, triggers, IM_ARRAYSIZE(triggers));
                Dropdown("Hit part", "Preferred aim point on the target.",
                         (int*)&Config.Aim.Target, targets, IM_ARRAYSIZE(targets));
                Dropdown("Target by", "Closest distance or smallest FOV.",
                         (int*)&Config.Aim.By, targetBy, IM_ARRAYSIZE(targetBy));
                SliderFloat("Aim range", "Maximum distance to consider a target.",
                            &Config.Aim.size, 0.0f, 500.0f, "%.0f m");
            });

            AddPageCard(p, 0, "Field of view", [&] {
                SliderFloat("FOV radius", "Visible radius of the aim cone.",
                            &Config.Aim.Cross, 0.0f, 180.0f, "%.0f");
                Checkbox("Hide Stream", "Keep the overlay out of screenshots.", &HIDEESP);
            });
        }
        else
        {
            AddPageCard(p, 0, "Triggerbot", [&] {
                Checkbox("Triggerbot", "Fire automatically while the crosshair is on a target.",
                         &Config.NewFeaturesMenu.Triggerbot);
                Dropdown("Mode", "Always, only while aiming, or only while firing.",
                         &Config.NewFeaturesMenu.TriggerbotMode, triggerModes, IM_ARRAYSIZE(triggerModes));
                SliderInt("Reaction delay", "Humanised delay before a shot.",
                          &Config.NewFeaturesMenu.TriggerbotDelay, 0, 500, "%.0f ms");
            });

            AddPageCard(p, 1, "Trigger zone", [&] {
                InfoRow("Trigger point", "Crosshair dot");
                InfoRow("Dot state", Config.Crosshair.Enabled ? "Enabled" : "Disabled");
                ParagraphRow("The dot's radius is the triggerbot's zone. Size, opacity and offset are on the Crosshair page.");
                if (Selectable("Open crosshair settings", false))
                {
                    if (var.tab.stored != Tab_Scripts)
                        RequestTab(Tab_Scripts);

                    var.subtab.stored = 1;
                }
            });
        }
    }

    inline void VisualsPage(PageLayout& p, int subtab)
    {
        static const char* boxTypes[] = { "Fill", "Outline", "Corner", "3D" };
        static const char* linePositions[] = { "Top", "Middle", "Bottom" };
        static const char* healthPositions[] = { "Top", "Side" };
        static const char* espStyles[] = { "None", "3D sphere", "Player signal" };
        static const char* crosshairTypes[] = { "Normal", "Circle", "Cross" };

        if (subtab == 0)
        {
            AddPageCard(p, 0, "Player ESP", [&] {
                Checkbox("Enable ESP line", "Draw a line to every player.", &Config.ESPMenu.isPlayerLine);
                Checkbox("Box", "Draw a box around the player.", &Config.ESPMenu.Box);
                Checkbox("Skeleton", "Draw the player skeleton.", &Config.ESPMenu.Skeleton);
                Checkbox("Health", "Draw a health bar.", &Config.ESPMenu.Health);
                Checkbox("Name", "Draw the player name.", &Config.ESPMenu.Name);
                Checkbox("Distance", "Draw the distance to the player.", &Config.ESPMenu.Distance);
                Checkbox("Player count", "Show the visible player count.", &Config.ESPMenu.Count);
                Checkbox("360 alert", "Warn when a player is behind you.", &Config.ESPMenu.Alert);
                Checkbox("Aim line", "Draw a line toward the aim target.", &Config.ESPMenu.Aimline);
            });

            AddPageCard(p, 1, "ESP style", [&] {
                Dropdown("Box type", "How the player box is rendered.",
                         (int*)&Config.ESPMenu.BoxType, boxTypes, IM_ARRAYSIZE(boxTypes));
                Dropdown("Line position", "Where the player line is anchored.",
                         (int*)&Config.ESPMenu.Target, linePositions, IM_ARRAYSIZE(linePositions));
                Dropdown("Health position", "Top bar or side bar.",
                         (int*)&Config.ESPMenu.HealthPosition, healthPositions, IM_ARRAYSIZE(healthPositions));
                Dropdown("ESP style", "Extra target decoration.",
                         (int*)&Config.ESPMenu.EspStyle, espStyles, IM_ARRAYSIZE(espStyles));
                Dropdown("Crosshair type", "Shape of the target marker.",
                         (int*)&Config.ESPMenu.CrosshairType, crosshairTypes, IM_ARRAYSIZE(crosshairTypes));
                Range("Line thickness", "Player and bot line width.",
                      &Config.Pline, &Config.Bline, 0.0f, 5.0f, "%.1f");
                Checkbox("Armor", "Draw armor state.", &Config.ESPMenu.Armor);
            });

            AddPageCard(p, 0, "Player colours", [&] {
                ColorRow("Player ESP colour", "Applied to the line, box, name, health and distance.",
                         Config.sColorsESPPLAYER.LinePLAYER);
                ColorGroup(Config.sColorsESPPLAYER.LinePLAYER,
                           Config.sColorsESPPLAYER.BoxPLAYER,
                           Config.sColorsESPPLAYER.NamePLAYER,
                           Config.sColorsESPPLAYER.HealthPLAYER,
                           Config.sColorsESPPLAYER.DistancePLAYER,
                           Config.sColorsESPPLAYER.SkeletonPLAYER);
            });

            AddPageCard(p, 1, "Bot colours", [&] {
                ColorRow("Bot ESP colour", "Applied to every bot visual.",
                         Config.sColorsESPBOT.LineBOT);
                ColorGroup(Config.sColorsESPBOT.LineBOT,
                           Config.sColorsESPBOT.BoxBOT,
                           Config.sColorsESPBOT.NameBOT,
                           Config.sColorsESPBOT.HealthBOT,
                           Config.sColorsESPBOT.DistanceBOT,
                           Config.sColorsESPBOT.SkeletonBOT);
            });
        }
        else
        {
            AddPageCard(p, 0, "World", [&] {
                Checkbox("No gravity", "Remove world gravity.", &Config.ExtraMenu.NoGravity);
                SliderFloat("Gravity scale", "Multiplier applied while gravity is active.",
                            &Config.ExtraMenu.NoGravityScale, 0.1f, 3.0f, "%.2fx");
                Checkbox("Clear terrain", "Remove terrain clutter.", &Config.ExtraMenu.ClearTerrain);
                Checkbox("No smoke", "Remove smoke and dust effects.", &Config.ExtraMenu.NoSmoke);
                Checkbox("Walk underwater", "Move normally below the water line.",
                         &Config.ExtraMenu.WalkUnderWater);
                Checkbox("Spectate X", "Extended spectator controls.", &Config.ExtraMenu.Spectatex);
            });

            AddPageCard(p, 1, "Camera", [&] {
                Checkbox("Camera POV", "Override the third-person camera distance.",
                         &Config.ExtraMenu.CameraPov);
                SliderFloat("POV size", "Camera distance multiplier.",
                            &Config.ExtraMenu.CameraPovSize, 0.0f, 100.0f, "%.0f");
                Checkbox("Stretch screen", "Widen the horizontal field of view.",
                         &Config.ExtraMenu.StretchScreen);
                if (Config.ExtraMenu.StretchScreen)
                {
                    SliderFloat("Aspect", "Target camera aspect ratio.",
                                &Config.ExtraMenu.StretchAspect, 1.0f, 3.0f, "%.2f");
                    if (Selectable("16:9", false)) Config.ExtraMenu.StretchAspect = 1.7778f;
                    if (Selectable("21:9", false)) Config.ExtraMenu.StretchAspect = 2.3333f;
                    if (Selectable("4:3", false))  Config.ExtraMenu.StretchAspect = 1.3333f;
                }
                Checkbox("Clear display", "Use the clean display path.", &Config.ExtraMenu.ClearDisplay);
                Checkbox("VSync mode", "Match frame pacing to the display.", &Config.ExtraMenu.Grap);
            });

            AddPageCard(p, 0, "Awareness", [&] {
                Checkbox("Grenade warning", "Warn about nearby grenades.", &Config.ESPMenu.GrenadeWarn);
                Checkbox("Placed items", "Highlight placed equipment.", &Config.ESPMenu.Placables);
                Checkbox("BR class", "Show the BR class marker.", &Config.ESPMenu.BRClass);
                Checkbox("Vehicle ESP", "Highlight vehicles.", &Config.ESPMenu.Vehicle);
                Checkbox("Vehicle health", "Draw vehicle health.", &Config.ESPMenu.VehicleHealth);
                Checkbox("Signal", "Show the player signal marker.", &Config.ESPMenu.Signal);
            });

            AddPageCard(p, 1, "Environment", [&] {
                Checkbox("Black sky", "Replace the sky with black.", &Config.ExtraMenu.BlackSky);
                Checkbox("Yellow wallhack", "Render players through walls.", &Config.ExtraMenu.WallHack);
                Checkbox("Red wallhack", "Render targets through walls in red.",
                         &Config.ExtraMenu.RedWallhack);
                Checkbox("Display crosshair", "Draw the configurable crosshair.", &Config.ESPMenu.Crosshair);
                SliderFloat("Vehicle scale", "Scale of rendered vehicles.",
                            &Config.ExtraMenu.VehicleScale, 0.5f, 3.0f, "%.2fx");
            });

            AddPageCard(p, 0, "Other colours", [&] {
                ColorRow("POV marker colour", "Colour of the other-player marker.",
                         Config.sColorsESPOTHERS.PovOTHERS);
                ColorRow("Vehicle colour", "Colour used by the vehicle ESP.",
                         Config.ColorsESP.Vehicle);
            });
        }
    }

    inline void WeaponsPage(PageLayout& p, int subtab)
    {
        if (subtab == 0)
        {
            AddPageCard(p, 0, "Weapon mods", [&] {
                Checkbox("Hitbox", "Expand the weapon hitbox.", &Config.ExtraMenu.Hit);
                Checkbox("No recoil", "Remove weapon recoil.", &Config.ExtraMenu.Recoil);
                Checkbox("No spread", "Remove bullet spread.", &Config.ExtraMenu.Spread);
                Checkbox("No shake", "Remove camera and weapon shake.", &Config.ExtraMenu.Shake);
                Checkbox("No overheat", "Disable the overheat limit.", &Config.ExtraMenu.Rpd);
                Checkbox("Firerate", "Remove the fire-rate cap.", &Config.ExtraMenu.Fire);
                Checkbox("Fast reload", "Shorten the reload animation.", &Config.ExtraMenu.Reload);
                Checkbox("Fast scope", "Shorten the scope animation.", &Config.ExtraMenu.Scope);
                Checkbox("Quick switch", "Remove the weapon-switch delay.", &Config.ExtraMenu.Switch);
                Checkbox("Fast dive", "Speed up the dive animation.", &Config.ExtraMenu.Diving);
                Checkbox("No parachute", "Skip the parachute deployment.", &Config.ExtraMenu.Parachute);
                Checkbox("Anti flashbang", "Remove the flash effect.", &Config.ExtraMenu.Flash);
                Checkbox("Weapon kinetic", "Apply the kinetic armour patch.", &Config.ExtraMenu.Kinetic);
            });

            AddPageCard(p, 1, "Ammo & attachments", [&] {
                Checkbox("Unlimited ammo", "Keep the magazine full.", &Config.UnlimitedAmmo);
                Checkbox("Unlimited reserve", "Keep reserve ammunition full.", &Config.ExtraMenu.UnliAmmo);
                Checkbox("Blueprint", "Turn on blueprint / skins (BlueprintWeaponInfo).",
                          &Config.ExtraMenu.MythicAttachments);
                Checkbox("Attachment unlock", "Treat every attachment as owned.",
                         &Config.ExtraMenu.Attachment);
                Checkbox("Smart reload", "Reload only when it is safe.", &Config.ExtraMenu.SmartReload);
            });
        }
        else
        {
            AddPageCard(p, 0, "Movement", [&] {
                Checkbox("Unlimited fly", "Hold jump to climb without a wingsuit.", &UnlimitedFly);
                Checkbox("Long slide", "Extend the slide duration.", &Config.ExtraMenu.LongSlide);
                Checkbox("Unlimited slide", "Remove the slide cooldown.", &Config.ExtraMenu.UnliSlide);
                Checkbox("No wingsuit", "Disable the automatic wingsuit.", &Config.ExtraMenu.NoWingsuit);
                SliderFloat("Slide distance", "Tuning multiplier for the slide.",
                            &SlideRange, 0.0f, 30.0f, "%.1f");
                SliderFloat("Snowboard speed", "Snowboard / slide speed tuning.",
                            &SnowBsize, 0.0f, 100.0f, "%.1f");
            });

            AddPageCard(p, 1, "Speed", [&] {
                SliderFloat("SpeedHack", "Movement speed multiplier.",
                            &speedHackMultiplier, 0.5f, 2.0f, "%.2fx");
                SliderFloat("High jump", "Jump height multiplier.",
                            &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
                Checkbox("Speedhack X", "Apply the speedhack patch.", &Config.ExtraMenu.SpeedhackX);
                Checkbox("Frame pacing", "Enable the frame limiter.", &Config.ExtraMenu.Frame);
                Checkbox("High FPS", "Prefer the high frame-rate path.", &Config.ExtraMenu.HighFps);
                Checkbox("FPS boost", "Enable the general FPS patch.", &Config.ExtraMenu.Fps);
                Checkbox("Enhance FPS", "Apply the render-side FPS improvement.",
                         &Config.ExtraMenu.EnhanceFPS);
            });
        }
    }

    inline void ScriptsPage(PageLayout& p, int subtab)
    {
        if (subtab == 0)
        {
            AddPageCard(p, 0, "Firing timing", [&] {
                Checkbox("No sprint-fire delay", "Remove the sprint-out timer before firing.",
                         &Config.NewFeaturesMenu.NoSprintToFireDelay);
                Checkbox("No hipfire delay", "Remove the hipfire wind-up gate.",
                         &Config.NewFeaturesMenu.NoHipfireDelay);
                Checkbox("No hit flinch", "Remove the flinch applied when hit.",
                         &Config.NewFeaturesMenu.NoHitFlinch);
                Checkbox("No drift", "Remove hardscope drift.", &Config.NewFeaturesMenu.NoDrift);
            });

            AddPageCard(p, 1, "Movement patches", [&] {
                Checkbox("No crouch", "Remove the crouch state.", &Config.NewFeaturesMenu.NoCrouch);
                Checkbox("No slide fatigue", "Remove the slide fatigue limit.",
                         &Config.NewFeaturesMenu.NoSlideFatigue);
            });

            AddPageCard(p, 0, "Patch status", [&] {
                const int active =
                    (Config.NewFeaturesMenu.NoSprintToFireDelay ? 1 : 0) +
                    (Config.NewFeaturesMenu.NoHipfireDelay ? 1 : 0) +
                    (Config.NewFeaturesMenu.NoHitFlinch ? 1 : 0) +
                    (Config.NewFeaturesMenu.NoDrift ? 1 : 0) +
                    (Config.NewFeaturesMenu.NoCrouch ? 1 : 0) +
                    (Config.NewFeaturesMenu.NoSlideFatigue ? 1 : 0);

                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%d / 6", active);
                InfoRow("Active patches", buffer);
                ParagraphRow("Patches are applied every frame and stay active while the menu is closed.");

                if (Selectable("Disable all patches", false))
                {
                    Config.NewFeaturesMenu.NoSprintToFireDelay = false;
                    Config.NewFeaturesMenu.NoHipfireDelay = false;
                    Config.NewFeaturesMenu.NoHitFlinch = false;
                    Config.NewFeaturesMenu.NoDrift = false;
                    Config.NewFeaturesMenu.NoCrouch = false;
                    Config.NewFeaturesMenu.NoSlideFatigue = false;
                }
            });
        }
        else
        {
            AddPageCard(p, 0, "Dot crosshair", [&] {
                Checkbox("Enabled", "Draw the dot and use it as the trigger zone.",
                         &Config.Crosshair.Enabled);
                SliderFloat("Size", "Diameter of the dot and trigger zone.",
                            &Config.Crosshair.Size, 1.0f, 40.0f, "%.0f px");
                SliderFloat("Opacity", "Dot opacity.", &Config.Crosshair.Color[3], 0.0f, 1.0f, "%.2f");
                ParagraphRow("The dot's radius is the triggerbot's zone: shrinking the dot tightens the trigger by the same amount.");
            });

            AddPageCard(p, 1, "Offset", [&] {
                SliderFloat("Horizontal", "Pixels right of the render centre.",
                            &Config.Crosshair.OffsetX, -400.0f, 400.0f, "%.0f px");
                SliderFloat("Vertical", "Pixels below the render centre.",
                            &Config.Crosshair.OffsetY, -400.0f, 400.0f, "%.0f px");
                if (Selectable("Reset to centre", false))
                {
                    Config.Crosshair.OffsetX = 0.0f;
                    Config.Crosshair.OffsetY = 0.0f;
                }
            });

            AddPageCard(p, 0, "Colour", [&] {
                ColorRow("Crosshair colour", "Dot colour and transparency.",
                         Config.Crosshair.Color);
                DrawCrosshairPreview();
            });
        }
    }

    inline void ConfigsPage(PageLayout& p, int subtab)
    {
        std::vector<std::string> profiles = ListProfiles();
        if (profiles.empty())
            profiles.push_back("ethnir");

        if (subtab == 0)
        {
            AddPageCard(p, 0, "Create a profile", [&] {
                if (TextFieldRow("##new_profile_name", ui.newProfile, (int)sizeof(ui.newProfile),
                              "Profile name"))
                    ui.profileKeyboard = true;
                if (Selectable("Create & save profile", false))
                {
                    const std::string name = TrimName(ui.newProfile);
                    if (!name.empty())
                    {
                        if (std::find(profiles.begin(), profiles.end(), name) == profiles.end())
                            profiles.push_back(name);
                         ui.activeProfile = name;
                         SaveConfiguration(name);
                         std::ifstream verify(androidFilesDir + "configs/" + name + ".json");
                         if (verify.is_open())
                         {
                             ui.activeProfile = name;
                             ui.configStatus = "Saved " + name + ".json";
                             ui.newProfile[0] = '\0';
                         }
                         else
                         {
                             ui.configStatus = "Failed to save " + name + ".json";
                         }
                    }
                }
                ParagraphRow("Profiles live in the app's configs/ directory as JSON.");
            });

            AddPageCard(p, 1, "Saved profiles", [&] {
                for (auto& profile : profiles)
                {
                    bool loadPressed = false;
                    bool savePressed = false;
                    ConfigButton(profile.c_str(), "Config profile", profile == ui.activeProfile,
                                 loadPressed, savePressed);

                    if (loadPressed)
                    {
                        if (LoadConfiguration(profile))
                        {
                            ui.activeProfile = profile;
                            ui.configStatus = "Loaded " + profile + ".json";
                        }
                        else
                        {
                            ui.configStatus = profile + ".json not found";
                        }
                    }
                    if (savePressed)
                    {
                        SaveConfiguration(profile);
                        std::ifstream verify(androidFilesDir + "configs/" + profile + ".json");
                        if (verify.is_open())
                        {
                            ui.activeProfile = profile;
                            ui.configStatus = "Saved " + profile + ".json";
                        }
                        else
                        {
                            ui.configStatus = "Failed to save " + profile + ".json";
                        }
                    }
                }
            });

            AddPageCard(p, 0, "Quick actions", [&] {
                if (Selectable("Save current config", false))
                {
                    SaveConfiguration(ui.activeProfile);
                    {
                        std::ifstream verify(androidFilesDir + "configs/" + ui.activeProfile + ".json");
                        ui.configStatus = verify.is_open()
                            ? "Saved " + ui.activeProfile + ".json"
                            : "Failed to save " + ui.activeProfile + ".json";
                    }
                }
                if (Selectable("Reload current config", false))
                {
                    ui.configStatus = LoadConfiguration(ui.activeProfile)
                        ? "Loaded " + ui.activeProfile + ".json"
                        : ui.activeProfile + ".json not found";
                }
                if (Selectable("Reset targeting", false))
                {
                    Config.Crosshair.OffsetX = 0.0f;
                    Config.Crosshair.OffsetY = 0.0f;
                    Config.Crosshair.Size = 6.0f;
                    Config.Crosshair.Color[3] = 0.95f;
                    ui.configStatus = "Crosshair reset to defaults";
                }
                if (Selectable("Open settings", false))
                    RequestTab(Tab_Settings);
            });

            AddPageCard(p, 1, "Status", [&] {
                InfoRow("Format", "JSON");
                InfoRow("Directory", (androidFilesDir + "configs").c_str());
                ParagraphRow(ui.configStatus.c_str());
            });
        }
        else
        {
            AddPageCard(p, 0, "Session", [&] {
                InfoRow("License", usedKey.empty() ? "not signed in" : usedKey.c_str());
                InfoRow("Plan", userType.empty() ? "unknown" : userType.c_str());
                InfoRow("Build", "26.10");
                InfoRow("Active profile", ui.activeProfile.c_str());
                ParagraphRow("Session values are read from the loader; menu options live on the Settings tab.");
            });
        }
    }

    inline void SettingsPage(PageLayout& p, int subtab)
    {
        static bool logoInit = false;
        static float logoOpacity = 1.0f;
        static float logoSize = 1.0f;
        if (!logoInit)
        {
            logoOpacity = GetLogoOpacity();
            logoSize = GetLogoSizeMultiplier();
            logoInit = true;
        }

        if (subtab == 0)
        {
            AddPageCard(p, 0, "Menu appearance", [&] {
                if (SliderFloat("Logo opacity", "Opacity of the on-screen logo.",
                                &logoOpacity, 0.0f, 1.0f, "%.2f"))
                    SetLogoOpacity(logoOpacity);
                if (SliderFloat("Logo size", "Size multiplier of the on-screen logo.",
                                &logoSize, 0.1f, 2.0f, "%.2f"))
                    SetLogoSizeMultiplier(logoSize);

                if (Selectable("Reset logo", false))
                {
                    logoOpacity = 1.0f;
                    logoSize = 1.0f;
                    SetLogoOpacity(1.0f);
                    SetLogoSizeMultiplier(1.0f);
                }
            });

            AddPageCard(p, 1, "Menu", [&] {
                InfoRow("Build", "26.10");
                InfoRow("Renderer", "OpenGL ES 3");
                InfoRow("Backend", "Dear ImGui 1.90");
                InfoRow("Profile", ui.activeProfile.c_str());
                ParagraphRow("Drag a page to scroll it. Hold the avatar in the header for save, load and minimize.");
            });
        }
        else
        {
            AddPageCard(p, 0, "Reports & account", [&] {
                Checkbox("Block report evidence", "Drop player-report evidence.",
                         &Config.ExtraMenu.BlockReportEvidence);
                Checkbox("Block ACE report", "Empty the ACE report payload.",
                         &Config.ExtraMenu.BlockAceReport);
                Checkbox("Anti-login", "Block the forced relogin on location change.",
                         &Config.ExtraMenu.AntiLogin);
                Checkbox("Reset guest", "Reset the guest account state.", &Config.ExtraMenu.ResetGuest);
            });

            AddPageCard(p, 1, "Performance", [&] {
                Checkbox("Frame smoothness", "Cap the frame rate to the level below.",
                         &Config.Aim.FpsLevel);
                if (Config.Aim.FpsLevel)
                    SliderFloat("Frame level", "Target frames per second.",
                                &Config.Aim.FpsLevel1, 30.0f, 120.0f, "%.0f");
                Checkbox("Ultra frame boost", "Enable the higher frame ceiling.",
                         &Config.Aim.FpsLevelUltra);
                if (Config.Aim.FpsLevelUltra)
                    SliderFloat("Ultra level", "Ultra frame ceiling.",
                                &Config.Aim.FpsLevelUltra1, 60.0f, 240.0f, "%.0f");
            });
        }
    }

    inline void RenderPage(int tab, int subtab, ImVec2 origin, float width)
    {
        PageLayout layout = MakePageLayout(origin, width);

        switch (tab)
        {
        case Tab_Aimbot:   AimbotPage(layout, subtab); break;
        case Tab_Visuals:  VisualsPage(layout, subtab); break;
        case Tab_Weapons:  WeaponsPage(layout, subtab); break;
        case Tab_Scripts:  ScriptsPage(layout, subtab); break;
        case Tab_Configs:  ConfigsPage(layout, subtab); break;
        case Tab_Settings: SettingsPage(layout, subtab); break;
        default: break;
        }

        const float bottom = PageBottom(layout);
        ImGui::SetCursorScreenPos(ImVec2(layout.origin.x, layout.origin.y + bottom));
        ImGui::Dummy(ImVec2(1, 1));
    }

    inline void RenderHeader(ImVec2 pos, ImVec2 size, float pad, float headerH)
    {
        Brand(ImVec2(pos.x + pad, pos.y + pad), headerH, "ETHNIR", "The cheat, if there is one.");

        const float minimizeSize = ImMax(S(28), headerH * 0.58f);
        const ImVec2 minimizePos(pos.x + size.x - pad - minimizeSize,
                                 pos.y + pad + (headerH - minimizeSize) * 0.5f);
        if (MinimizeButton(minimizePos, minimizeSize))
            minimize_requested = true;

        const ImVec2 profileRight(minimizePos.x - S(14), pos.y + pad);
        const char* plan = userType.empty() ? "Session active" : userType.c_str();
        if (ProfileButton(profileRight, headerH, "Operator", plan))
        {
            OpenPopup("##raspody_profile_menu",
                      ImVec2(profileRight.x - S(232), pos.y + pad + headerH + S(6)));
        }

        if (IsPopupOpen("##raspody_profile_menu"))
        {
            if (BeginPopup("##raspody_profile_menu", ImVec2(S(232), S(218)),
                           ImVec2(profileRight.x - S(232), pos.y + pad + headerH + S(6)),
                           ImVec2(S(15), S(15))))
            {
                DrawText(ImGui::GetWindowDrawList(), fonts::body ? fonts::body : ImGui::GetFont(), S(13),
                         ImGui::GetCursorScreenPos(), Col(clr.white), "Operator", ImVec2(0.0f, 0.0f));
                ImGui::Dummy(ImVec2(1, S(18)));
                DrawText(ImGui::GetWindowDrawList(), fonts::small ? fonts::small : ImGui::GetFont(), S(10.5f),
                         ImGui::GetCursorScreenPos(), Col(clr.text),
                         userType.empty() ? "Session active" : userType.c_str(), ImVec2(0.0f, 0.0f));
                ImGui::Dummy(ImVec2(1, S(18)));

                if (Selectable("Open settings", false))
                {
                    RequestTab(Tab_Settings);
                    ClosePopup("##raspody_profile_menu");
                }
                if (Selectable("Save config", false))
                {
                    SaveConfiguration(ui.activeProfile);
                    {
                        std::ifstream verify(androidFilesDir + "configs/" + ui.activeProfile + ".json");
                        ui.configStatus = verify.is_open()
                            ? "Saved " + ui.activeProfile + ".json"
                            : "Failed to save " + ui.activeProfile + ".json";
                    }
                    ClosePopup("##raspody_profile_menu");
                }
                if (Selectable("Reload config", false))
                {
                    ui.configStatus = LoadConfiguration(ui.activeProfile)
                        ? "Loaded " + ui.activeProfile + ".json"
                        : ui.activeProfile + ".json not found";
                    ClosePopup("##raspody_profile_menu");
                }
                if (Selectable("Minimize menu", false))
                {
                    minimize_requested = true;
                    ClosePopup("##raspody_profile_menu");
                }

                EndPopup();
            }
        }
    }

    inline void RenderTabRail(ImVec2 pos, float pad, float headerH, float gap,
                              float tabW, float contentH)
    {
        const float railTop = pos.y + pad + headerH + gap;
        const ImVec2 railMin(pos.x + pad, railTop);
        const ImVec2 railMax(railMin.x + tabW, railTop + contentH);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(railMin, railMax, Col(clr.child, 0.58f), S(20));
        dl->AddRect(railMin, railMax, Col(clr.separator, 0.45f), S(20), 0, 1.0f);

        const float innerPad = S(8);
        const float itemGap = S(6);
        const float available = contentH - innerPad * 2.0f - itemGap * (Tab_Count - 1);
        const float itemH = ImMax(S(38), ImMin(S(54), available / (float)Tab_Count));
        const float total = itemH * (float)Tab_Count + itemGap * (float)(Tab_Count - 1);
        float y = railTop + (contentH - total) * 0.5f;

        for (int i = 0; i < Tab_Count; ++i)
        {
            const ImVec2 itemPos(railMin.x + innerPad, y);
            const ImVec2 itemSize(tabW - innerPad * 2.0f, itemH);
            if (TabButton(kTabNames[i], i, var.tab.stored == i, itemPos, itemSize))
                RequestTab(i);
            y += itemH + itemGap;
        }
    }

    inline void RenderContentHeader(float contentX, float contentY, float contentW,
                                    float headerBlockH)
    {
        const int tab = ImClamp(var.tab.current, 0, Tab_Count - 1);
        const float pillH = S(30);
        const float searchSize = S(36);
        const float centerY = contentY + headerBlockH * 0.5f;

        ImDrawList* dl = ImGui::GetWindowDrawList();

        float nextX = contentX;
        const float nameW = TextSize(fonts::small ? fonts::small : ImGui::GetFont(), S(11),
                                     kTabNames[tab]).x;
        const float breadcrumbW = nameW + S(42);
        const ImVec2 breadcrumbMin(contentX, centerY - pillH * 0.5f);
        const ImVec2 breadcrumbMax(breadcrumbMin.x + breadcrumbW, breadcrumbMin.y + pillH);

        ImGui::PushID("##raspody_breadcrumb");
        ImGui::SetCursorScreenPos(breadcrumbMin);
        ImGui::InvisibleButton("##breadcrumb", ImVec2(breadcrumbW, pillH));
        const bool backHovered = ImGui::IsItemHovered();
        const bool backPressed = ImGui::IsItemClicked(0);
        ImGui::PopID();
        if (backPressed && !ui.dragScrolling)
            GoBack();

        dl->AddRectFilled(breadcrumbMin, breadcrumbMax,
                          Col(backHovered ? clr.widget : clr.child), S(11));
        dl->AddRect(breadcrumbMin, breadcrumbMax,
                    Col(backHovered ? clr.accent : clr.separator, 0.8f), S(11), 0, 1.0f);
        DrawChevron(dl, ImVec2(breadcrumbMin.x + S(16), centerY), S(11),
                    backHovered ? Col(clr.accent) : Col(clr.text3), IM_PI * 0.5f);
        DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                 ImVec2(breadcrumbMin.x + S(30), centerY), Col(clr.white),
                 kTabNames[tab], ImVec2(0.0f, 0.5f));
        nextX = breadcrumbMax.x + S(10);

        for (int i = 0; i < 2; ++i)
        {
            const char* label = kSubtabNames[tab][i];
            if (!label)
                continue;

            const float width = TextSize(fonts::small ? fonts::small : ImGui::GetFont(), 16.0f, label).x + S(24);
            if (nextX + width > contentX + contentW - searchSize - S(20))
                break;

            const ImVec2 min(nextX, centerY - pillH * 0.5f);
            const ImVec2 max(min.x + width, min.y + pillH);
            const bool selected = var.subtab.stored == i;

            ImGui::PushID(label);
            ImGui::SetCursorScreenPos(min);
            ImGui::InvisibleButton("##subtab", ImVec2(width, pillH));
            const bool hovered = ImGui::IsItemHovered();
            const bool pressed = ImGui::IsItemClicked(0);
            ImGui::PopID();

            if (pressed && !selected)
            {
                var.subtab.stored = i;
                if (var.subtab.current == i)
                    var.subtab.alpha = 1.0f;
            }

            dl->AddRectFilled(min, max,
                              selected ? Col(clr.accent, 0.16f)
                                       : Col(hovered ? clr.widget : clr.child, 0.95f), S(11));
            if (selected)
                dl->AddRect(min, max, Col(clr.accent, 0.55f), S(11), 0, 1.0f);
            DrawText(dl, fonts::small ? fonts::small : ImGui::GetFont(), 16.0f,
                     ImVec2((min.x + max.x) * 0.5f, centerY),
                     selected ? Col(clr.white) : (hovered ? Col(clr.text3) : Col(clr.text2)),
                     label, ImVec2(0.5f, 0.5f));

            nextX = max.x + S(6);
        }

        if (ui.searchOpen)
            Ease(ui.searchAnim, 1.0f, 16.0f);
        else
            Ease(ui.searchAnim, 0.0f, 16.0f);

        const float searchX = contentX + contentW - searchSize;
        if (ui.searchAnim > 0.01f)
        {
            const float maxField = ImMin(S(220), contentW * 0.50f);
            const float fieldW = maxField * ui.searchAnim;
            if (fieldW > S(30))
            {
                const ImVec2 fieldPos(searchX - fieldW - S(6), centerY - S(19));
                if (SearchField(ui.search, (int)sizeof(ui.search), fieldPos, fieldW))
                    ui.searchKeyboard = true;
            }
        }
        else
        {
            ClearSearch();
        }

        if (SearchButton(ImVec2(searchX, centerY - searchSize * 0.5f), searchSize, ui.searchOpen))
        {
            ui.searchOpen = !ui.searchOpen;
            if (!ui.searchOpen)
            {
                ui.search[0] = '\0';
                ClearSearch();
            }
        }
    }

    inline void Render(bool* open)
    {
        if (open && !*open)
            return;

        BeginFrame();

        static bool historySeeded = false;
        if (!historySeeded)
        {
            PushTabHistory(var.tab.current);
            historySeeded = true;
        }

        const ImVec2 display = ImGui::GetIO().DisplaySize;

        const MenuWindow win = ComputeMenuWindow(display);
        const ImVec2 pos = win.pos;
        const ImVec2 size = win.size;

        const float scale = ImClamp(ImMin(size.x / 900.0f, size.y / 592.0f), 0.70f, 1.45f);
        SetScale(scale);

        const float pad = ImClamp(size.x * 0.017f, S(10), S(18));
        const float headerH = ImClamp(size.y * 0.088f, S(40), S(56));
        const float gap = S(13);
        const float tabW = ImClamp(size.x * 0.095f, S(58), S(120));
        const float contentY = pos.y + pad + headerH + gap;
        const float contentH = ImMax(S(80), size.y - (contentY - pos.y) - pad);
        const float contentX = pos.x + pad + tabW + gap;
        const float contentW = ImMax(S(120), size.x - (contentX - pos.x) - pad);
        const float headerBlockH = S(46);
        const float childH = ImMax(S(40), contentH - headerBlockH);

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

        ImGui::Begin("##raspody_menu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        {
            dl->PushClipRect(ImVec2(0, 0), display, false);
            dl->AddRectFilled(ImVec2(pos.x - S(12), pos.y - S(12)),
                              ImVec2(pos.x + size.x + S(12), pos.y + size.y + S(12)),
                              Col(clr.accent, 0.055f), S(42));
            dl->AddRectFilled(ImVec2(pos.x - S(8), pos.y - S(8)),
                              ImVec2(pos.x + size.x + S(8), pos.y + size.y + S(8)),
                              Col(clr.layout, 0.55f), S(36));
            dl->PopClipRect();
        }

        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), Col(clr.layout), S(30));
        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), Col(clr.separator, 0.55f), S(30), 0, 1.0f);

        RenderHeader(pos, size, pad, headerH);
        RenderTabRail(pos, pad, headerH, gap, tabW, contentH);

        bool switched = false;
        const float dt = ImGui::GetIO().DeltaTime;

        if (var.tab.current != var.tab.stored)
        {
            var.tab.alpha = ImMax(0.0f, var.tab.alpha - dt * 6.0f);
            if (var.tab.alpha <= 0.02f)
            {
                var.tab.current = var.tab.stored;
                var.tab.alpha = 0.02f;
                var.subtab.current = var.subtab.stored = 0;
                var.subtab.alpha = 1.0f;
                switched = true;
            }
        }
        else
        {
            var.tab.alpha = ImMin(1.0f, var.tab.alpha + dt * 5.0f);
        }

        if (var.subtab.current != var.subtab.stored)
        {
            var.subtab.alpha = ImMax(0.0f, var.subtab.alpha - dt * 8.0f);
            if (var.subtab.alpha <= 0.02f)
            {
                var.subtab.current = var.subtab.stored;
                var.subtab.alpha = 0.02f;
                switched = true;
            }
        }
        else
        {
            var.subtab.alpha = ImMin(1.0f, var.subtab.alpha + dt * 6.0f);
        }

        if (switched)
            ui.scrollToTop = true;

        RenderContentHeader(contentX, contentY, contentW, headerBlockH);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::SetCursorScreenPos(ImVec2(contentX, contentY + headerBlockH));
        ImGui::BeginChild("##raspody_content", ImVec2(contentW, childH), false,
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

        if (ui.scrollToTop)
        {
            ImGui::SetScrollY(0.0f);
            ui.scrollToTop = false;
        }

        {
            ImGuiIO& io = ImGui::GetIO();
            static bool dragging = false;
            static ImVec2 dragOrigin(0, 0);
            static float dragOriginScroll = 0.0f;
            const ImVec2 cpos = ImGui::GetWindowPos();
            const ImVec2 csize = ImGui::GetWindowSize();
            const bool inChild = io.MousePos.x >= cpos.x && io.MousePos.x <= cpos.x + csize.x &&
                                 io.MousePos.y >= cpos.y && io.MousePos.y <= cpos.y + csize.y;
            if (io.MouseDown[0])
            {
                if (!dragging && inChild)
                {
                    dragging = true;
                    dragOrigin = io.MousePos;
                    dragOriginScroll = ImGui::GetScrollY();
                }
                if (dragging)
                {
                    const float dy = io.MousePos.y - dragOrigin.y;
                    if (fabsf(dy) > S(10))
                    {
                        ui.dragScrolling = true;
                        ImGui::SetScrollY(dragOriginScroll - dy);
                    }
                }
            }
            else
            {
                dragging = false;
                ui.dragScrolling = false;
            }
        }

        const ImVec2 origin(ImGui::GetWindowPos().x,
                            ImGui::GetWindowPos().y - ImGui::GetScrollY());
        const float scrollY = ImGui::GetScrollY();
        const float scrollMax = ImGui::GetScrollMaxY();
        const float availW = ImMax(S(140), ImGui::GetWindowSize().x - S(6) - ImGui::GetStyle().ScrollbarSize);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, var.tab.alpha * var.subtab.alpha);
        RenderPage(var.tab.current, var.subtab.current, origin, availW);
        ImGui::PopStyleVar();

        if (scrollMax > 0.0f)
        {
            ImDrawList* sdl = ImGui::GetWindowDrawList();
            const ImVec2 wpos = ImGui::GetWindowPos();
            const ImVec2 wsize = ImGui::GetWindowSize();
            const float trackX = wpos.x + wsize.x - S(6);
            const float trackY0 = wpos.y;
            const float trackY1 = wpos.y + wsize.y;
            sdl->AddRectFilled(ImVec2(trackX, trackY0), ImVec2(trackX + S(4), trackY1),
                               Col(clr.widget, 0.6f), S(2));
            const float frac = scrollY / scrollMax;
            const float thumbH = ImMax(S(20), wsize.y * (wsize.y / (wsize.y + scrollMax)));
            const float thumbY = trackY0 + (trackY1 - trackY0 - thumbH) * frac;
            sdl->AddRectFilled(ImVec2(trackX, thumbY), ImVec2(trackX + S(4), thumbY + thumbH),
                               Col(clr.accent), S(2));
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        if (scrollY > S(1))
            ContentShadow(ImVec2(contentX, contentY + headerBlockH),
                          ImVec2(contentX + contentW, contentY + headerBlockH + S(22)), true);
        if (scrollMax > 0.0f && scrollY < scrollMax - S(1))
            ContentShadow(ImVec2(contentX, contentY + headerBlockH + childH - S(22)),
                          ImVec2(contentX + contentW, contentY + headerBlockH + childH), false);

        ImGui::End();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        if (var.tab.current == Tab_Visuals)
        {
            static ImVec2 cardPos(-1.0f, -1.0f);
            static bool cardMoved = false;

            float cardW = ImClamp(size.x * 0.21f, S(150), S(226));

            const float roomW = display.x - (pos.x + size.x) - S(16) - S(6);
            if (roomW >= S(150))
                cardW = ImMin(cardW, roomW);

            const float cardH = ImClamp(cardW * 1.34f, S(196), S(310));

            if (!cardMoved)
            {
                float cardX = pos.x + size.x + S(16);
                if (cardX + cardW > display.x - S(6))
                    cardX = display.x - cardW - S(6);

                cardPos = ImVec2(cardX, pos.y + (size.y - cardH) * 0.5f);
            }
            cardPos.x = ImClamp(cardPos.x, 0.0f, ImMax(0.0f, display.x - cardW));
            cardPos.y = ImClamp(cardPos.y, 0.0f, ImMax(0.0f, display.y - cardH));

            ImGui::SetNextWindowPos(cardPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(cardW, cardH), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::Begin("##raspody_esp_preview", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
            {
                DrawEspPreview(ImGui::GetWindowDrawList(), cardPos, cardW, cardH);

                ImGui::SetCursorScreenPos(ImVec2(cardPos.x + S(6), cardPos.y + S(2)));
                ImGui::InvisibleButton("##raspody_esp_grip",
                                       ImVec2(ImMax(1.0f, cardW - S(12)), S(18)));
                if (ImGui::IsItemActive())
                {
                    const ImVec2 delta = ImGui::GetIO().MouseDelta;
                    if (delta.x != 0.0f || delta.y != 0.0f)
                    {
                        cardPos.x += delta.x;
                        cardPos.y += delta.y;
                        cardMoved = true;
                    }
                }
            }
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }

        Ease(ui.blackout, AnyPopupOpen() ? 1.0f : 0.0f, 8.0f);
        DrawBlackout(ui.blackout);

        if (ui.searchKeyboard)
            RenderVirtualKeyboard("##raspody_search_keyboard", ui.search,
                                  (int)sizeof(ui.search), &ui.searchKeyboard);
        if (ui.profileKeyboard)
            RenderVirtualKeyboard("##raspody_profile_keyboard", ui.newProfile,
                                  (int)sizeof(ui.newProfile), &ui.profileKeyboard);
    }
}

