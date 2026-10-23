// Host check for ApplyCharacterSkin. The config objects the game passes in are
// plain il2cpp objects with the fields at +0x10, so the same layout can be
// built in host memory and the write rules exercised without a phone.
//
//   g++ -std=c++17 -Isrc/main/jni
//       src/main/jni/Skin/tests/fields_apply_check.cpp -o /tmp/fields_check

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <vector>

using std::string;

struct String
{
    int length;
};

template <typename T> struct Array
{
    void *klass;
    void *monitor;
    void *bounds;
    int max_length;
    T items[8];
};

namespace Tools
{
    inline bool IsPtrValid(void *p) { return p != nullptr; }
}

#include "Skin/Fields.h"

static int g_failures = 0;

static void Check(const char *what, bool ok)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_failures;
}

template <typename T> struct Slot
{
    alignas(16) std::uint8_t raw[0x10 + sizeof(T)];

    Slot() { std::memset(raw, 0, sizeof(raw)); }

    void *base() { return (void *)raw; }
    T *fields() { return (T *)(raw + 0x10); }
};

static charInfo MakeSkin()
{
    charInfo skin{};
    skin.charModel = {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112};
    skin.charRes = {201, 202, 203};
    skin.ItemRes2 = {11, 12, 13, 14};
    skin.charRole = {301, 302, 303, 4, 5, 1, 6, 7};
    skin.charRole2[0] = nullptr;
    skin.charPack = {0, 401, 402, 403, 404, 405};
    return skin;
}

int main()
{
    const charInfo skin = MakeSkin();

    Slot<CharacterModelFields> model;
    Slot<ItemResourceFields> item;
    Slot<RoleConfFields> role;
    Slot<RolePackFields> pack;
    Slot<RolePackFields> existingPack;
    Slot<RolePackFields> unrelatedPack;

    model.fields()->Traitor1P = 500;
    model.fields()->Traitor3P = 600;
    item.fields()->ID = 700;
    role.fields()->ID = 800;
    pack.fields()->RolePackID = 900;

    existingPack.fields()->RolePackID = 900;
    existingPack.fields()->LobbySceneType = 3;
    existingPack.fields()->LoadingFrame = 0;

    unrelatedPack.fields()->RolePackID = 901;

    std::vector<void *> models{model.base()};
    std::vector<void *> items{item.base()};
    std::vector<void *> roles{role.base()};
    std::vector<void *> packs{existingPack.base(), unrelatedPack.base(), pack.base()};

    ApplyCharacterSkin(skin, 500, 600, 700, 800, 900, models, items, roles, packs);

    Check("the selected body receives the model ids",
          model.fields()->BRModel == 104 && model.fields()->DefaultModelID == 107);
    Check("the selected item resource receives the asset ids",
          item.fields()->FxAssetID == 201 && item.fields()->ModelAssetIDRaw == 203);
    Check("the selected role receives the outfit ids",
          role.fields()->roleFinalSuitID == 302 && role.fields()->RolePackID == 7);
    Check("the selected role pack is written",
          pack.fields()->EntryAnimID == 401 && pack.fields()->KillStreakSkinID == 405);

    // A pack that already had a lobby scene and no loading frame used to be
    // skipped entirely, which left the outfit pointing at a pack that kept its
    // old ids - the half-applied state that renders as no model.
    Check("a role pack with an existing lobby scene is still written",
          existingPack.fields()->GestureId == 402 && existingPack.fields()->LoadingFrame == 404);
    Check("another role pack is left alone",
          unrelatedPack.fields()->EntryAnimID == 0 && unrelatedPack.fields()->GestureId == 0);

    Slot<CharacterModelFields> stockModel;
    Slot<ItemResourceFields> stockItem;
    Slot<RoleConfFields> stockRole;
    stockModel.fields()->Traitor1P = 710001101;
    stockModel.fields()->Traitor3P = 710001102;
    stockItem.fields()->ID = 100301208;
    stockRole.fields()->ID = 100301208;

    std::vector<void *> stockModels{stockModel.base()};
    std::vector<void *> stockItems{stockItem.base()};
    std::vector<void *> stockRoles{stockRole.base()};

    ApplyCharacterSkin(skin, 999, 998, 997, 996, 995, stockModels, stockItems, stockRoles, {});

    Check("an unknown body falls back to the stock config",
          stockModel.fields()->BRModel == 104);
    Check("an unknown item resource falls back to the stock config",
          stockItem.fields()->FxAssetID == 201);
    Check("an unknown role falls back to the stock config",
          stockRole.fields()->roleLeftArmID == 301);

    if (g_failures)
    {
        std::printf("FAIL: %d check(s)\n", g_failures);
        return 1;
    }

    std::printf("OK: fields_apply_check - both bodies, item, role and packs get the skin\n");
    return 0;
}
