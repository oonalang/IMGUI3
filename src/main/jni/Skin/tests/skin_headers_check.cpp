// Host syntax check for the whole skin / kill-decoration graph.
//
// MainFeatureIncludes.h pulls in Skin/Fields.h + Skin/Thread.h + Skin/Camo.h -
// i.e. every file the skin work lives in - and this TU instantiates the render
// entry point, so a change that only breaks in that include graph fails here
// instead of in ndk-build.
//
// This file is compiled with -fsyntax-only (nothing is linked, so every game
// symbol stays an unresolved stub) and it is never part of the APK: Android.mk's
// wildcards only cover ImGui, IL2CppSDKGenerator/IL2Cpp, KittyMemory,
// System/Texture and the jni root.
//
// Run it from the repo root:
//   g++ -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/skin_headers_check.cpp
// Exits 0 with no output on success.
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

struct Skin_ExtraMenuStub { bool CamoTest; int CamoTestMode; bool AutoSkin; };
struct Skin_ConfigStub { Skin_ExtraMenuStub ExtraMenu; };
static Skin_ConfigStub Config;

// Externs the skin headers expect from Main.cpp.
float menu[4];
ImFont* F50 = nullptr;
ImFont* JAAT = nullptr;
namespace font { ImFont* inter_semibold = nullptr; }

// The skin graph is consumed the exact way MainFeatureIncludes.h consumes it.
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
// Host-only link stubs for the real Tools.cpp (same set auto_equip_check uses).
#include <openssl/md5.h>
extern "C" int DobbyHook(void *address, void *replace_call, void **origin_call)
{
    (void)address; (void)replace_call; (void)origin_call;
    return 0;
}
extern "C" int MD5_Init(MD5_CTX *) { return 0; }
extern "C" int MD5_Update(MD5_CTX *, const void *, size_t) { return 0; }
extern "C" int MD5_Final(unsigned char *, MD5_CTX *) { return 0; }

namespace KittyMemory
{
inline uintptr_t getAbsoluteAddress(const char *libraryName, uintptr_t relativeAddr, bool useCache)
{ (void)libraryName; (void)useCache; return relativeAddr; }
}
// The skin graph reaches the Substrate hook trampoline and the il2cpp string
// helper; both are stubbed like the rest.
void MSHookFunction(void *symbol, void *replace, void **original)
{ (void)symbol; if (original) *original = replace; }

// Il2CppString::CString comes from Il2Cpp.cpp; the include-graph check never
// touches real strings, so a stub-definition keeps the link self-contained.
const char *Il2CppString::CString() { return ""; }

// And64InlineHook's A64 trampoline: referenced by the include graph, never called.
void A64HookFunction(void *const symbol, void *const replace, void **result)
{ (void)symbol; if (result) *result = replace; }

static std::string ToLower(std::string s)
{
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}

#include "Skin/Fields.h"
#include "Skin/Thread.h"
#include "Skin/AutoEquipSkin.h"
#include "Skin/Camo.h"

void tu_use(void) { AutoEquipSkin_Tick(true); }

int main()
{
    tu_use();
    return 0;
}
