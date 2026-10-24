#pragma once

#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

#include "../foxcheats/include/json.hpp"
#include "../../Hacks/Feature/Init_Hacks.h"

extern float menu[4];
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;

std::string androidFilesDir = "/storage/emulated/0/Android/data/com.garena.game.codm/files/";

#include "ProfileFiles.h"

void SaveConfiguration(const std::string& filename) {
    if (!ValidProfileName(filename)) return;
    nlohmann::json config;

    config["ESPMenu"]["Box"] = Config.ESPMenu.Box;
    config["ESPMenu"]["Name"] = Config.ESPMenu.Name;
    config["ESPMenu"]["Health"] = Config.ESPMenu.Health;
    config["ESPMenu"]["Distance"] = Config.ESPMenu.Distance;
    config["ESPMenu"]["GrenadeWarn"] = Config.ESPMenu.GrenadeWarn;
    config["ESPMenu"]["Placables"] = Config.ESPMenu.Placables;
    config["ESPMenu"]["Target"] = static_cast<int>(Config.ESPMenu.Target);
    config["ESPMenu"]["BoxType"] = static_cast<int>(Config.ESPMenu.BoxType);
    config["ESPMenu"]["HealthPosition"] = static_cast<int>(Config.ESPMenu.HealthPosition);
    config["ESPMenu"]["CrosshairType"] = static_cast<int>(Config.ESPMenu.CrosshairType);
    config["ESPMenu"]["EspStyle"] = static_cast<int>(Config.ESPMenu.EspStyle);

    config["Aim"]["Aimbot360"] = Config.Aim.Aimbot360;
    config["Aim"]["AimSilent"] = Config.Aim.AimSilent;
    config["Aim"]["AimAssistSize"] = Config.Aim.AimAssistSize;
    config["Aim"]["Target"] = static_cast<int>(Config.Aim.Target);
    config["Aim"]["Trigger"] = static_cast<int>(Config.Aim.Trigger);
    config["Aim"]["By"] = static_cast<int>(Config.Aim.By);
    config["Aim"]["size"] = Config.Aim.size;

    config["ExtraMenu"]["Flash"] = Config.ExtraMenu.Flash;
    config["ExtraMenu"]["Diving"] = Config.ExtraMenu.Diving;
    config["ExtraMenu"]["Fire"] = Config.ExtraMenu.Fire;
    config["ExtraMenu"]["Hit"] = Config.ExtraMenu.Hit;
    config["ExtraMenu"]["Rpd"] = Config.ExtraMenu.Rpd;
    config["ExtraMenu"]["Parachute"] = Config.ExtraMenu.Parachute;
    config["ExtraMenu"]["Recoil"] = Config.ExtraMenu.Recoil;
    config["ExtraMenu"]["Shake"] = Config.ExtraMenu.Shake;
    config["ExtraMenu"]["Spread"] = Config.ExtraMenu.Spread;
    config["ExtraMenu"]["Reload"] = Config.ExtraMenu.Reload;
    config["ExtraMenu"]["Scope"] = Config.ExtraMenu.Scope;
    config["ExtraMenu"]["Switch"] = Config.ExtraMenu.Switch;
    config["ExtraMenu"]["Kinetic"] = Config.ExtraMenu.Kinetic;
    config["ExtraMenu"]["KineticArmor"] = Config.ExtraMenu.Kinetic;

    config["ExtraMenu"]["MythicAttachments"] = Config.ExtraMenu.MythicAttachments;
    config["ExtraMenu"]["Attachment"] = Config.ExtraMenu.Attachment;
    config["ExtraMenu"]["HideOutfit"] = Config.ExtraMenu.HideOutfit;
    config["ExtraMenu"]["OutfitScale"] = Config.ExtraMenu.OutfitScale;
    config["ExtraMenu"]["OutfitTintBypass"] = Config.ExtraMenu.OutfitTintBypass;
    config["ExtraMenu"]["OutfitTintR"] = Config.ExtraMenu.OutfitTint[0];
    config["ExtraMenu"]["OutfitTintG"] = Config.ExtraMenu.OutfitTint[1];
    config["ExtraMenu"]["OutfitTintB"] = Config.ExtraMenu.OutfitTint[2];
    config["ExtraMenu"]["OutfitTintA"] = Config.ExtraMenu.OutfitTint[3];
    config["ExtraMenu"]["BlockReportEvidence"] = Config.ExtraMenu.BlockReportEvidence;
    config["ExtraMenu"]["BlockAceReport"] = Config.ExtraMenu.BlockAceReport;
    config["ExtraMenu"]["AntiLogin"] = Config.ExtraMenu.AntiLogin;

    config["isJumpAdjustmentEnabled"] = isJumpAdjustmentEnabled;
    config["jumpHeightMultiplier"] = jumpHeightMultiplier;
    config["SnowB"] = SnowB;
    config["SnowBsize"] = SnowBsize;
    config["isSpeedHackEnabled"] = isSpeedHackEnabled;
    config["speedHackMultiplier"] = speedHackMultiplier;

    config["NewFeatures"]["Triggerbot"] = Config.NewFeaturesMenu.Triggerbot;
    config["NewFeatures"]["TriggerbotMode"] = Config.NewFeaturesMenu.TriggerbotMode;
    config["NewFeatures"]["TriggerbotRadius"] = Config.NewFeaturesMenu.TriggerbotRadius;
    config["NewFeatures"]["TriggerbotDotRay"] = Config.NewFeaturesMenu.TriggerbotDotRay;

    config["NewFeatures"]["TriggerbotDelay"] = Config.NewFeaturesMenu.TriggerbotDelay;
    config["NewFeatures"]["NoCrouch"] = Config.NewFeaturesMenu.NoCrouch;
    config["NewFeatures"]["NoHitFlinch"] = Config.NewFeaturesMenu.NoHitFlinch;
    config["NewFeatures"]["NoDrift"] = Config.NewFeaturesMenu.NoDrift;
    config["NewFeatures"]["NoSlideFatigue"] = Config.NewFeaturesMenu.NoSlideFatigue;
    config["NewFeatures"]["NoSprintToFireDelay"] = Config.NewFeaturesMenu.NoSprintToFireDelay;
    config["NewFeatures"]["NoHipfireDelay"] = Config.NewFeaturesMenu.NoHipfireDelay;

    config["Crosshair"]["Enabled"] = Config.Crosshair.Enabled;
    config["Crosshair"]["Size"] = Config.Crosshair.Size;
    config["Crosshair"]["OffsetX"] = Config.Crosshair.OffsetX;
    config["Crosshair"]["OffsetY"] = Config.Crosshair.OffsetY;
    config["Crosshair"]["Opacity"] = Config.Crosshair.Color[3];
    config["Crosshair"]["ColorR"] = Config.Crosshair.Color[0];
    config["Crosshair"]["ColorG"] = Config.Crosshair.Color[1];
    config["Crosshair"]["ColorB"] = Config.Crosshair.Color[2];

    config["menu"][0] = menu[0];
    config["menu"][1] = menu[1];
    config["menu"][2] = menu[2];

    std::string configDir = androidFilesDir + "configs";
    mkdir(configDir.c_str(), 0777);

    std::string filePath = ProfilePath(filename);
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << std::setw(4) << config;
        file.close();
    }
}

bool LoadConfiguration(const std::string& filename) {
    try {
        const std::string filePath = ProfilePath(filename);
        if (filePath.empty()) return false;
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json config;
        file >> config;

        if (config.contains("ESPMenu")) {
            if (config["ESPMenu"].contains("Box")) Config.ESPMenu.Box = config["ESPMenu"]["Box"].get<bool>();
            if (config["ESPMenu"].contains("Name")) Config.ESPMenu.Name = config["ESPMenu"]["Name"].get<bool>();
            if (config["ESPMenu"].contains("Health")) Config.ESPMenu.Health = config["ESPMenu"]["Health"].get<bool>();
            if (config["ESPMenu"].contains("Distance")) Config.ESPMenu.Distance = config["ESPMenu"]["Distance"].get<bool>();
            if (config["ESPMenu"].contains("GrenadeWarn")) Config.ESPMenu.GrenadeWarn = config["ESPMenu"]["GrenadeWarn"].get<bool>();
            if (config["ESPMenu"].contains("Placables")) Config.ESPMenu.Placables = config["ESPMenu"]["Placables"].get<bool>();
            if (config["ESPMenu"].contains("Target")) Config.ESPMenu.Target = static_cast<LineTarget>(config["ESPMenu"]["Target"].get<int>());
            if (config["ESPMenu"].contains("BoxType")) Config.ESPMenu.BoxType = static_cast<EspBoxType>(config["ESPMenu"]["BoxType"].get<int>());
            if (config["ESPMenu"].contains("HealthPosition")) Config.ESPMenu.HealthPosition = static_cast<EspHealthPosition>(config["ESPMenu"]["HealthPosition"].get<int>());
            if (config["ESPMenu"].contains("CrosshairType")) Config.ESPMenu.CrosshairType = static_cast<CrosshairTarget>(config["ESPMenu"]["CrosshairType"].get<int>());
            if (config["ESPMenu"].contains("EspStyle")) Config.ESPMenu.EspStyle = static_cast<EspStyleTarget>(config["ESPMenu"]["EspStyle"].get<int>());
        }

        if (config.contains("Aim")) {
            if (config["Aim"].contains("Aimbot360")) Config.Aim.Aimbot360 = config["Aim"]["Aimbot360"].get<bool>();
            if (config["Aim"].contains("AimSilent")) Config.Aim.AimSilent = config["Aim"]["AimSilent"].get<bool>();
            if (config["Aim"].contains("AimAssistSize")) Config.Aim.AimAssistSize = config["Aim"]["AimAssistSize"].get<float>();
            if (config["Aim"].contains("Target")) Config.Aim.Target = static_cast<EAimTarget>(config["Aim"]["Target"].get<int>());
            if (config["Aim"].contains("Trigger")) Config.Aim.Trigger = static_cast<EAimTrigger>(config["Aim"]["Trigger"].get<int>());
            if (config["Aim"].contains("By")) Config.Aim.By = static_cast<EAim>(config["Aim"]["By"].get<int>());
            if (config["Aim"].contains("size")) Config.Aim.size = config["Aim"]["size"].get<float>();
        }

        if (config.contains("ExtraMenu")) {
            if (config["ExtraMenu"].contains("Flash")) Config.ExtraMenu.Flash = config["ExtraMenu"]["Flash"].get<bool>();
            if (config["ExtraMenu"].contains("Diving")) Config.ExtraMenu.Diving = config["ExtraMenu"]["Diving"].get<bool>();
            if (config["ExtraMenu"].contains("Fire")) Config.ExtraMenu.Fire = config["ExtraMenu"]["Fire"].get<bool>();
            if (config["ExtraMenu"].contains("Hit")) Config.ExtraMenu.Hit = config["ExtraMenu"]["Hit"].get<bool>();
            if (config["ExtraMenu"].contains("Rpd")) Config.ExtraMenu.Rpd = config["ExtraMenu"]["Rpd"].get<bool>();
            if (config["ExtraMenu"].contains("Parachute")) Config.ExtraMenu.Parachute = config["ExtraMenu"]["Parachute"].get<bool>();
            if (config["ExtraMenu"].contains("Recoil")) Config.ExtraMenu.Recoil = config["ExtraMenu"]["Recoil"].get<bool>();
            if (config["ExtraMenu"].contains("Shake")) Config.ExtraMenu.Shake = config["ExtraMenu"]["Shake"].get<bool>();
            if (config["ExtraMenu"].contains("Spread")) Config.ExtraMenu.Spread = config["ExtraMenu"]["Spread"].get<bool>();
            if (config["ExtraMenu"].contains("Reload")) Config.ExtraMenu.Reload = config["ExtraMenu"]["Reload"].get<bool>();
            if (config["ExtraMenu"].contains("Scope")) Config.ExtraMenu.Scope = config["ExtraMenu"]["Scope"].get<bool>();
            if (config["ExtraMenu"].contains("Switch")) Config.ExtraMenu.Switch = config["ExtraMenu"]["Switch"].get<bool>();
            if (config["ExtraMenu"].contains("Kinetic"))
                Config.ExtraMenu.Kinetic = config["ExtraMenu"]["Kinetic"].get<bool>();
            else if (config["ExtraMenu"].contains("KineticArmor"))
                Config.ExtraMenu.Kinetic = config["ExtraMenu"]["KineticArmor"].get<bool>();
            if (config["ExtraMenu"].contains("Attachment"))
                Config.ExtraMenu.Attachment = config["ExtraMenu"]["Attachment"].get<bool>();
            if (config["ExtraMenu"].contains("MythicAttachments"))
                Config.ExtraMenu.MythicAttachments = config["ExtraMenu"]["MythicAttachments"].get<bool>();
            if (config["ExtraMenu"].contains("HideOutfit"))
                Config.ExtraMenu.HideOutfit = config["ExtraMenu"]["HideOutfit"].get<bool>();
            if (config["ExtraMenu"].contains("OutfitScale"))
                Config.ExtraMenu.OutfitScale = config["ExtraMenu"]["OutfitScale"].get<float>();
            if (config["ExtraMenu"].contains("OutfitTintBypass"))
                Config.ExtraMenu.OutfitTintBypass = config["ExtraMenu"]["OutfitTintBypass"].get<bool>();
            if (config["ExtraMenu"].contains("OutfitTintR")) Config.ExtraMenu.OutfitTint[0] = config["ExtraMenu"]["OutfitTintR"].get<float>();
            if (config["ExtraMenu"].contains("OutfitTintG")) Config.ExtraMenu.OutfitTint[1] = config["ExtraMenu"]["OutfitTintG"].get<float>();
            if (config["ExtraMenu"].contains("OutfitTintB")) Config.ExtraMenu.OutfitTint[2] = config["ExtraMenu"]["OutfitTintB"].get<float>();
            if (config["ExtraMenu"].contains("OutfitTintA")) Config.ExtraMenu.OutfitTint[3] = config["ExtraMenu"]["OutfitTintA"].get<float>();
            if (config["ExtraMenu"].contains("BlockReportEvidence"))
                Config.ExtraMenu.BlockReportEvidence = config["ExtraMenu"]["BlockReportEvidence"].get<bool>();
            if (config["ExtraMenu"].contains("BlockAceReport"))
                Config.ExtraMenu.BlockAceReport = config["ExtraMenu"]["BlockAceReport"].get<bool>();
            if (config["ExtraMenu"].contains("AntiLogin"))
                Config.ExtraMenu.AntiLogin = config["ExtraMenu"]["AntiLogin"].get<bool>();
    if (config.contains("NewFeatures")) {
        if (config["NewFeatures"].contains("NoCrouch")) Config.NewFeaturesMenu.NoCrouch = config["NewFeatures"]["NoCrouch"].get<bool>();
        if (config["NewFeatures"].contains("Triggerbot")) Config.NewFeaturesMenu.Triggerbot = config["NewFeatures"]["Triggerbot"].get<bool>();
        if (config["NewFeatures"].contains("TriggerbotMode")) Config.NewFeaturesMenu.TriggerbotMode = config["NewFeatures"]["TriggerbotMode"].get<int>();
        if (config["NewFeatures"].contains("TriggerbotDelay")) Config.NewFeaturesMenu.TriggerbotDelay = config["NewFeatures"]["TriggerbotDelay"].get<int>();
        if (config["NewFeatures"].contains("TriggerbotRadius")) Config.NewFeaturesMenu.TriggerbotRadius = config["NewFeatures"]["TriggerbotRadius"].get<int>();
        if (config["NewFeatures"].contains("TriggerbotDotRay")) Config.NewFeaturesMenu.TriggerbotDotRay = config["NewFeatures"]["TriggerbotDotRay"].get<bool>();
        if (config["NewFeatures"].contains("NoHitFlinch")) Config.NewFeaturesMenu.NoHitFlinch = config["NewFeatures"]["NoHitFlinch"].get<bool>();
        if (config["NewFeatures"].contains("NoDrift")) Config.NewFeaturesMenu.NoDrift = config["NewFeatures"]["NoDrift"].get<bool>();
        if (config["NewFeatures"].contains("NoSlideFatigue")) Config.NewFeaturesMenu.NoSlideFatigue = config["NewFeatures"]["NoSlideFatigue"].get<bool>();
        if (config["NewFeatures"].contains("NoSprintToFireDelay")) Config.NewFeaturesMenu.NoSprintToFireDelay = config["NewFeatures"]["NoSprintToFireDelay"].get<bool>();
        if (config["NewFeatures"].contains("NoHipfireDelay")) Config.NewFeaturesMenu.NoHipfireDelay = config["NewFeatures"]["NoHipfireDelay"].get<bool>();
    }

        if (config.contains("Crosshair")) {
            if (config["Crosshair"].contains("Enabled")) Config.Crosshair.Enabled = config["Crosshair"]["Enabled"].get<bool>();
            if (config["Crosshair"].contains("Size")) Config.Crosshair.Size = config["Crosshair"]["Size"].get<float>();
            if (config["Crosshair"].contains("OffsetX")) Config.Crosshair.OffsetX = config["Crosshair"]["OffsetX"].get<float>();
            if (config["Crosshair"].contains("OffsetY")) Config.Crosshair.OffsetY = config["Crosshair"]["OffsetY"].get<float>();
            if (config["Crosshair"].contains("Opacity")) Config.Crosshair.Color[3] = config["Crosshair"]["Opacity"].get<float>();
            if (config["Crosshair"].contains("ColorR")) Config.Crosshair.Color[0] = config["Crosshair"]["ColorR"].get<float>();
            if (config["Crosshair"].contains("ColorG")) Config.Crosshair.Color[1] = config["Crosshair"]["ColorG"].get<float>();
            if (config["Crosshair"].contains("ColorB")) Config.Crosshair.Color[2] = config["Crosshair"]["ColorB"].get<float>();
        }
        }

        if (config.contains("isJumpAdjustmentEnabled"))
            isJumpAdjustmentEnabled = config["isJumpAdjustmentEnabled"].get<bool>();
        if (config.contains("jumpHeightMultiplier"))
            jumpHeightMultiplier = config["jumpHeightMultiplier"].get<float>();
        if (config.contains("SnowB"))
            SnowB = config["SnowB"].get<bool>();
        if (config.contains("SnowBsize"))
            SnowBsize = config["SnowBsize"].get<float>();
        if (config.contains("isSpeedHackEnabled"))
            isSpeedHackEnabled = config["isSpeedHackEnabled"].get<bool>();
        if (config.contains("speedHackMultiplier"))
            speedHackMultiplier = config["speedHackMultiplier"].get<float>();

        if (config.contains("menu")) {
            menu[0] = config["menu"][0].get<float>();
            menu[1] = config["menu"][1].get<float>();
            menu[2] = config["menu"][2].get<float>();
        }

        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

