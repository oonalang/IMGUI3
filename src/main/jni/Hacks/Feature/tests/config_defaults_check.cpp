#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector2.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Hacks/Feature/Init_Hacks.h"

extern "C" void MSHookFunction(void *symbol, void *replace, void **result)
{
    (void)symbol; (void)replace; (void)result;
}

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        g_checks++;                                                            \
        if (cond) {                                                            \
            printf("ok  : %s\n", msg);                                         \
        } else {                                                               \
            g_failures++;                                                      \
            printf("FAIL: %s\n", msg);                                         \
        }                                                                      \
    } while (0)

int main()
{
    CHECK(Config.ExtraMenu.MythicAttachments == false,
          "Blueprint (MythicAttachments) is OFF by default");
    CHECK(Config.ExtraMenu.Attachment == true,
          "Attachment unlock is ON by default");
    CHECK(Config.ExtraMenu.Kinetic == false && Config.ExtraMenu.Recoil == false,
          "the other ExtraMenu switches still default off");

    CHECK(Config.NewFeaturesMenu.TriggerbotRadius == 40,
          "the triggerbot radius default still comes from its initialiser");
    CHECK(Config.NewFeaturesMenu.TriggerbotDotRay == true,
          "the dot-ray gate is ON by default (the strict dot decides)");
    CHECK(Config.NewFeaturesMenu.TriggerbotMode == 0 &&
          Config.NewFeaturesMenu.NoSlideFatigue == false,
          "the NewFeatures switches still default off");
    CHECK(Config.Crosshair.Size == 6.0f && Config.Crosshair.Enabled == true,
          "the crosshair defaults still come from their initialisers");

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all config-default checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("%d of %d check(s) FAILED\n", g_failures, g_checks);
    return 1;
}

