// Host syntax check for the hook install graph: Feats.h is included here in the
// same order Main.cpp includes it (NewFeatures.h / Crosshair.h first), so a
// header or a HOOK_LIB line that only breaks in that graph fails on the desktop
// instead of in ndk-build. Nothing is linked; run it with host_checks.sh.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "IL2CppSDKGenerator/IL2Cpp/Call_IL2Cpp.h"
#include "Includes/OverlayGeometry.h"
#include "Hacks/Hacks.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "AstralPrtctn/AntiHook.h"
#include "foxcheats/include/ScanEngine.hpp"
#include "Includes/Jni_Stuff.h"
#include "Hacks/Feature/NewFeatures.h"
#include "Hacks/Feature/Crosshair.h"
#include "MainFeatureIncludes.h"
