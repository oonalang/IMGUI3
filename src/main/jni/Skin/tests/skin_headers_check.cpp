#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"
#include <thread>
#include <mutex>
#include "Hacks/StructGame/StructGame.h"

struct Skin_ExtraMenuStub { bool CamoTest; int CamoTestMode; };
struct Skin_ConfigStub { Skin_ExtraMenuStub ExtraMenu; };
static Skin_ConfigStub Config;

float menu[4];
bool showKeyboard = false;
ImFont* F50 = nullptr;
ImFont* JAAT = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

#include "System/UI/SkinTab.h"

void tu_use(void) { RenderSkinCategoryContent(0, true); }

