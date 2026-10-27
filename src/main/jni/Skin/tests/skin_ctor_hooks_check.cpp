#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string ReadAll(const char *path)
{
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static int failures = 0;

static void Fail(const std::string &what)
{
    std::printf("FAIL %s\n", what.c_str());
    ++failures;
}

int main()
{
    const std::string src = ReadAll("Skin/Thread.h");
    if (src.empty())
    {
        std::printf("FAIL Skin/Thread.h not readable from the jni root\n");
        return 1;
    }

    // MythicArmorConfig..ctor is at RVA 0x90DD884 in the 1.6.57 dump. 0x5C90E84
    // is FrontEndUIScene.RefreshNavigation, which the skin scanner would then
    // read as if it were a config row.
    if (src.find("uintptr_t MythicArmorConfigAddress = 0x90DD884;") == std::string::npos)
        Fail("MythicArmorConfigAddress is not MythicArmorConfig..ctor (0x90DD884)");
    if (src.find("0x5C90E84") != std::string::npos)
        Fail("MythicArmorConfigAddress still carries FrontEndUIScene.RefreshNavigation");

    const char *ctors[] = {
        "Item2InventoryCtor", "WeaponConfExtraCtor", "WeaponFireEffectCtor", "WeaponConfCtor",
        "WeaponAssetGroupCtor", "MythicArmorCtor", "MythicSightCtor", "WeaponSkinCtor",
        "WeaponCamosCtor", "KillEffectItemCtor", "SPKillIconCtor", "ItemResourceCtor",
        "CharacterModelCtor", "RoleConfCtor", "RoleSkinCtor", "RolePackConfCtor",
        "BRDeadboxSkinCtor", "BRDropPlaneSkinCtor", "VehicleSkinConfCtor", "VehicleItemConfCtor"
    };

    int guarded = 0;
    for (const char *name : ctors)
    {
        std::string space = std::string("    orig_") + name + "(instance);";
        std::string guard = std::string("if (orig_") + name + " == nullptr) return;";

        const size_t call = src.find(space);
        if (call == std::string::npos)
        {
            Fail(std::string("no call to orig_") + name);
            continue;
        }

        const size_t window = call > 160 ? call - 160 : 0;
        if (src.find(guard, window) == std::string::npos || src.find(guard, window) > call)
        {
            Fail(std::string("orig_") + name + " is called with no null guard");
            continue;
        }

        if (src.find(std::string("HookCtorAt(") ) == std::string::npos)
        {
            Fail("the constructor installs do not go through HookCtorAt");
            break;
        }
        ++guarded;
    }

    if (guarded != 20)
        Fail("only " + std::to_string(guarded) + " of 20 constructor hooks are guarded");

    if (src.find("DobbyHook((void*)getRealOffset(") != std::string::npos)
        Fail("a constructor hook is installed without checking that it landed");

    if (src.find("DobbyHook(target, hook, orig);") == std::string::npos)
        Fail("HookCtorAt does not install through DobbyHook");

    if (src.find("if (*orig == nullptr)") == std::string::npos)
        Fail("HookCtorAt does not verify the trampoline it was handed");

    if (failures != 0)
    {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }

    std::printf("skin constructor hooks: 20 guarded, mythic armour on the real ctor\n");
    return 0;
}
