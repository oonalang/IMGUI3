// Host check for the crosshair's configuration path.
//
// It drives the real SaveConfiguration / LoadConfiguration pair against a
// temporary directory, so "settings survive a save and a load" and "a profile
// written by the older build still loads" are executed rather than assumed.
//
//   g++ -std=c++17 -O1 -w -Itests/stubs -I. -IImGui -IIL2CppSDKGenerator \
//       -Ifoxcheats -IIncludes -Icurl/curl-android-arm64-v8a/include \
//       -Icurl/openssl-android-arm64-v8a/include -ISDK/Xhook -Isubstrate \
//       System/Core/tests/crosshair_config_check.cpp -o /tmp/crosshair_config_check
//
// Never build this into the app: it redirects androidFilesDir.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Hacks/Feature/Init_Hacks.h"

extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace;
    if (result) *result = nullptr;
}

inline int32_t ToColor(float *col)
{
    return (int32_t)IM_COL32((int)(col[0] * 255.0f), (int)(col[1] * 255.0f),
                             (int)(col[2] * 255.0f), (int)(col[3] * 255.0f));
}

namespace NewFeatures
{
    inline void CrosshairCentrePx(float &cx, float &cy)
    {
        cx = 960.0f + Config.Crosshair.OffsetX;
        cy = 540.0f + Config.Crosshair.OffsetY;
    }
}

// Globals the profile serialiser owns outside sConfig.
float menu[4] = {0.0f, 0.0f, 0.0f, 0.0f};
bool isJumpAdjustmentEnabled = false;
float jumpHeightMultiplier = 1.0f;
bool SnowB = false;
float SnowBsize = 1.0f;
bool isSpeedHackEnabled = false;
float speedHackMultiplier = 1.0f;

#include "Hacks/Feature/Crosshair.h"
#include "System/Core/SaveConfig.h"

#include "foxcheats/include/json.hpp"
#include <fstream>

static int g_checks = 0;
static int g_failures = 0;
#define CHECK(cond, msg)                                                \
    do {                                                                \
        g_checks++;                                                     \
        if (!(cond)) { printf("FAIL: %s  (line %d)\n", msg, __LINE__);  \
                       g_failures++; }                                  \
        else         { printf("ok  : %s\n", msg); }                     \
    } while (0)

static bool near(float a, float b, float eps = 0.0005f) { return std::fabs(a - b) <= eps; }

static const char *kProfile = "xhair_roundtrip";

static void WipeConfig()
{
    std::memset(&Config, 0, sizeof(sConfig));
    CrosshairApplyDefaults();
}

static void ReadCrosshairJson(nlohmann::json &out)
{
    std::ifstream file(ProfilePath(kProfile));
    out = nlohmann::json::parse(file, nullptr, false);
}

int main()
{
    // A scratch directory instead of the device path, so this cannot disturb a
    // real installation.
    androidFilesDir = "/tmp/xhair_cfg_check/";
    mkdir(androidFilesDir.c_str(), 0775);
    mkdir((androidFilesDir + "configs").c_str(), 0775);

    // ------------------------------------------------------ A: fresh default
    {
        WipeConfig();
        CHECK(!Config.Crosshair.Enabled, "A1 a fresh config leaves the crosshair off");
        CHECK(near(Config.Crosshair.Thickness, 1.5f) && near(Config.Crosshair.Gap, 3.0f) &&
              !Config.Crosshair.Outline && near(Config.Crosshair.OutlineThickness, 1.0f),
              "A2 a fresh config has the new appearance defaults");
    }

    // ------------------------------------------- B: save then load, unchanged
    {
        WipeConfig();
        Config.Crosshair.Enabled = true;
        Config.Crosshair.Style = ReticleBrackets;
        Config.Crosshair.Size = 17.0f;
        Config.Crosshair.Thickness = 3.5f;
        Config.Crosshair.Gap = 8.0f;
        Config.Crosshair.Outline = true;
        Config.Crosshair.OutlineThickness = 2.5f;
        Config.Crosshair.OffsetX = 12.0f;
        Config.Crosshair.OffsetY = -9.0f;
        Config.Crosshair.Color[0] = 0.2f;
        Config.Crosshair.Color[1] = 0.4f;
        Config.Crosshair.Color[2] = 0.6f;
        Config.Crosshair.Color[3] = 0.8f;

        SaveConfiguration(kProfile);
        CHECK(ProfileExists(kProfile), "B1 saving writes the profile");

        nlohmann::json written;
        ReadCrosshairJson(written);
        CHECK(written.contains("Crosshair") && written["Crosshair"]["Thickness"] == 3.5f &&
              written["Crosshair"]["Gap"] == 8.0f && written["Crosshair"]["Outline"] == true &&
              written["Crosshair"]["OutlineThickness"] == 2.5f,
              "B2 the new settings are written, so they survive a restart");

        WipeConfig();
        CHECK(!Config.Crosshair.Enabled, "B3 the wipe really reset the reticle");

        CHECK(LoadConfiguration(kProfile), "B4 the profile loads");
        CHECK(Config.Crosshair.Enabled, "B5 enabled comes back");
        CHECK(Config.Crosshair.Style == ReticleBrackets, "B5 a style the old build did not have comes back");
        CHECK(near(Config.Crosshair.Size, 17.0f), "B5 size comes back");
        CHECK(near(Config.Crosshair.Thickness, 3.5f), "B5 thickness comes back");
        CHECK(near(Config.Crosshair.Gap, 8.0f), "B5 gap comes back");
        CHECK(Config.Crosshair.Outline, "B5 outline comes back");
        CHECK(near(Config.Crosshair.OutlineThickness, 2.5f), "B5 outline width comes back");
        CHECK(near(Config.Crosshair.OffsetX, 12.0f) && near(Config.Crosshair.OffsetY, -9.0f),
              "B5 position comes back");
        CHECK(near(Config.Crosshair.Color[0], 0.2f) && near(Config.Crosshair.Color[1], 0.4f) &&
              near(Config.Crosshair.Color[2], 0.6f) && near(Config.Crosshair.Color[3], 0.8f),
              "B5 colour and opacity come back");
    }

    // ------------------------------------ C: a profile from the older build
    {
        // Exactly the Crosshair block the pre-change SaveConfiguration produced:
        // no Thickness, Gap, Outline or OutlineThickness.
        nlohmann::json legacy;
        legacy["Crosshair"]["Enabled"] = true;
        legacy["Crosshair"]["Style"] = (int)ReticleCross;
        legacy["Crosshair"]["Size"] = 12.5f;
        legacy["Crosshair"]["OffsetX"] = 3.0f;
        legacy["Crosshair"]["OffsetY"] = -4.0f;
        legacy["Crosshair"]["Opacity"] = 0.5f;
        legacy["Crosshair"]["ColorR"] = 1.0f;
        legacy["Crosshair"]["ColorG"] = 0.0f;
        legacy["Crosshair"]["ColorB"] = 0.0f;
        {
            std::ofstream file(ProfilePath(kProfile), std::ios::trunc);
            file << legacy.dump(2);
        }

        WipeConfig();
        CHECK(LoadConfiguration(kProfile), "C1 an older profile still loads");
        CHECK(Config.Crosshair.Enabled, "C2 the saved on/off preference is preserved");
        CHECK(Config.Crosshair.Style == ReticleCross,
              "C3 Style 1 still means Classic Cross, the old \"Crosshair\"");
        CHECK(near(Config.Crosshair.Size, 12.5f), "C4 the saved size is preserved");
        CHECK(near(Config.Crosshair.OffsetX, 3.0f) && near(Config.Crosshair.OffsetY, -4.0f),
              "C4 the saved position is preserved");
        CHECK(near(Config.Crosshair.Color[0], 1.0f) && near(Config.Crosshair.Color[1], 0.0f) &&
              near(Config.Crosshair.Color[2], 0.0f) && near(Config.Crosshair.Color[3], 0.5f),
              "C4 the saved colour and opacity are preserved");
        CHECK(near(Config.Crosshair.Thickness, 1.5f) && near(Config.Crosshair.Gap, 3.0f) &&
              !Config.Crosshair.Outline && near(Config.Crosshair.OutlineThickness, 1.0f),
              "C5 the fields the old build never wrote fall back to their defaults");
    }
    {
        // An older profile that had the reticle switched off must not switch it on.
        nlohmann::json legacy;
        legacy["Crosshair"]["Enabled"] = false;
        legacy["Crosshair"]["Size"] = 6.0f;
        {
            std::ofstream file(ProfilePath(kProfile), std::ios::trunc);
            file << legacy.dump(2);
        }
        WipeConfig();
        LoadConfiguration(kProfile);
        CHECK(!Config.Crosshair.Enabled, "C6 an older profile keeps the crosshair off");
    }

    // --------------------------------------------- D: a missing profile
    {
        WipeConfig();
        Config.Crosshair.Enabled = true;
        Config.Crosshair.Style = ReticleChevron;
        const bool loaded = LoadConfiguration("xhair_does_not_exist");
        CHECK(!loaded, "D1 loading a missing profile reports failure");
        CHECK(Config.Crosshair.Enabled && Config.Crosshair.Style == ReticleChevron,
              "D2 and leaves the live crosshair untouched");
    }

    // ------------------------------------------ E: bad names are rejected
    {
        WipeConfig();
        SaveConfiguration("../escape");
        CHECK(!ProfileExists("../escape"), "E1 an invalid profile name writes nothing");
        CHECK(!ProfileExists("escape"), "E1 and cannot escape the config directory");
    }

    // ------------------------------------------------- F: reset vs. save
    {
        WipeConfig();
        Config.Crosshair.Style = ReticleXShape;
        Config.Crosshair.Outline = true;
        SaveConfiguration(kProfile);
        WipeConfig();
        LoadConfiguration(kProfile);
        CHECK(Config.Crosshair.Style == ReticleXShape && Config.Crosshair.Outline,
              "F1 a reset then a save then a load keeps the chosen style");
        CrosshairApplyDefaults();
        CHECK(Config.Crosshair.Style == ReticleDot && !Config.Crosshair.Outline,
              "F2 resetting returns the style rather than clearing it");
        CHECK(Config.Crosshair.Style == ReticleDot && ReticleStyleAt(Config.Crosshair.Style).name != nullptr,
              "F3 a reset style is always a renderable one");
    }

    std::remove(ProfilePath(kProfile).c_str());

    printf("\n");
    if (g_failures == 0) { printf("OK: all crosshair config checks passed (%d)\n", g_checks); return 0; }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}
