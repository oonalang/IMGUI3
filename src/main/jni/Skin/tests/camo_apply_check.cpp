#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <utility>

#include "IL2CppSDKGenerator/BasicStructs/Vector3.hpp"
#include "IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"

#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
#include "Skin/Fields.h"

std::vector<camoInfo> camoData;

#include <openssl/md5.h>

extern "C" int DobbyHook(void *address, void *replace_call, void **origin_call)
{
    (void)address; (void)replace_call; (void)origin_call;
    return 0;
}
extern "C" int MD5_Init(MD5_CTX *) { return 0; }
extern "C" int MD5_Update(MD5_CTX *, const void *, size_t) { return 0; }
extern "C" int MD5_Final(unsigned char *, MD5_CTX *) { return 0; }

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

static const size_t OBJ_SIZE = 0x200;

struct ConfObj { alignas(8) unsigned char b[OBJ_SIZE]; };

template <typename T>
static void put(unsigned char *o, size_t off, T v) { std::memcpy(o + off, &v, sizeof v); }
template <typename T>
static T get(const unsigned char *o, size_t off) { T v; std::memcpy(&v, o + off, sizeof v); return v; }

static const size_t CF_ID     = 0x20;
static const size_t CF_DEFSKIN = 0x18;

static std::vector<std::unique_ptr<ConfObj>> g_store;

static ConfObj *makeConf(uint id, int defSkin)
{
    auto c = std::make_unique<ConfObj>();
    std::memset(c->b, 0, OBJ_SIZE);
    put<uint>(c->b + 0x20, CF_ID, id);
    put<int>(c->b + 0x20, CF_DEFSKIN, defSkin);
    ConfObj *raw = c.get();
    g_store.push_back(std::move(c));
    return raw;
}

static int  confSkin(const ConfObj *c) { return get<int>(c->b + 0x20, CF_DEFSKIN); }
static uint confID(const ConfObj *c)   { return get<uint>(c->b + 0x20, CF_ID); }

struct SkinObj { alignas(8) unsigned char b[OBJ_SIZE]; };

static const size_t SK_CAMOID   = 0x0C;
static const size_t SK_COLORSUB = 0x10;
static const size_t SK_ID       = 0x14;

static std::vector<std::unique_ptr<SkinObj>> g_skinStore;

static SkinObj *makeSkin(uint id, int camoID, int colorSub)
{
    auto s = std::make_unique<SkinObj>();
    std::memset(s->b, 0, OBJ_SIZE);
    put<uint>(s->b + 0x10, SK_ID, id);
    put<int>(s->b + 0x10, SK_CAMOID, camoID);
    put<int>(s->b + 0x10, SK_COLORSUB, colorSub);
    SkinObj *raw = s.get();
    g_skinStore.push_back(std::move(s));
    return raw;
}

static int skinCamoID(const SkinObj *s)   { return get<int>(s->b + 0x10, SK_CAMOID); }
static int skinColorSub(const SkinObj *s) { return get<int>(s->b + 0x10, SK_COLORSUB); }

static const int ID_GOLD     = 0x1D37F755;
static const int ID_REDSPR   = 0x1D37F77E;
static const int ID_DIAMOND  = 0x1D37F758;

static const int CAMO_GOLD_ID = 131071;

int main()
{
    {
        camoData.clear();
        InsertCamo(0x1111, "Zulu Camo", 10, 0);
        InsertCamo(0x2222, "alpha camo", 20, 0);
        InsertCamo(0x1111, "Zulu Camo", 10, 955);
        InsertCamo(0x2222, "alpha camo", 20, 0);
        CHECK(camoData.size() == 2, "T1 a camo repeated once per weapon appears once");
        CHECK(camoData.size() == 2 && camoData[0].camoItemID == 0x2222 &&
              camoData[1].camoItemID == 0x1111,
              "T1 the list is ordered by name, case-insensitively");
    }

    {
        camoData.clear();
        InsertCamo(CAMO_GOLD_ID, "Camo 131071", 0, 955);
        CHECK(camoData.size() == 1 && camoData[0].applyID == CAMO_GOLD_ID,
              "T2 an unresolved name applies the row's own id");
        CHECK(camoData.size() == 1 && camoData[0].belongWeaponID == 955,
              "T2 a per-weapon row is kept as it came");

        InsertCamo(CAMO_GOLD_ID, "Gold", 4242, 0);
        CHECK(camoData.size() == 1, "T2 the second row does not add an entry");
        CHECK(camoData.size() == 1 && camoData[0].camoName == "Gold",
              "T2 the real name replaces the placeholder");
        CHECK(camoData.size() == 1 && camoData[0].applyID == ID_GOLD,
              "T2 Gold applies the proven constant, not the table id");
        CHECK(camoData.size() == 1 && camoData[0].textureAssetID == 4242,
              "T2 the texture id is upgraded too");
        CHECK(camoData.size() == 1 && camoData[0].belongWeaponID == 0,
              "T2 a row usable on every weapon wins over a per-weapon one");
    }

    {
        camoData.clear();
        InsertCamo(5001, "Golden Emerald", 1, 0);
        InsertCamo(5002, "Gold", 1, 0);
        InsertCamo(5003, "krm gold", 1, 0);
        CHECK(camoData.size() == 2,
              "T2b the two Gold spellings are ONE row (they apply the same id)");
        const camoInfo *ge = nullptr, *gold = nullptr;
        for (const auto &c : camoData) {
            if (c.camoName == "Golden Emerald") ge = &c;
            if (c.camoName == "Gold") gold = &c;
        }
        CHECK(ge && ge->applyID == 5001,
              "T2b \"Golden Emerald\" is not read as Gold");
        CHECK(gold && gold->applyID == ID_GOLD,
              "T2b the Gold row applies the proven constant");
        CHECK(gold && gold->camoItemID == 5002,
              "T2b ... and it is the first table row that showed it");
        CHECK(gold && gold->textureAssetID == 1, "T2b its texture id is kept");
    }

    {
        camoData.clear();
        InsertCamo(6001, "Camo 6001", 0, 0);
        InsertCamo(6001, "Aether Crystal", 7, 0);
        CHECK(camoData.size() == 1 && camoData[0].applyID == 0x1D37F77D,
              "T2c Aether Crystal applies its proven constant");
        camoData.clear();
        InsertCamo(6002, "sakura red sprite", 0, 0);
        CHECK(camoData.size() == 1 && camoData[0].applyID == 0x1D37F77E,
              "T2c a prefixed Red Sprite row applies its proven constant");
    }

    {
        std::unordered_map<uint, int> originals;
        ConfObj *krm  = makeConf(100, 555);
        ConfObj *m4   = makeConf(200, 0);
        ConfObj *ak   = makeConf(300, 777);
        std::vector<void *> confs = { krm, m4, ak };

        int written = ApplyCamoToEveryGun(ID_DIAMOND, confs, originals);
        CHECK(written == 3, "T3 every conf is written, not only those with a skin row");
        CHECK(confSkin(krm) == ID_DIAMOND && confSkin(m4) == ID_DIAMOND &&
              confSkin(ak) == ID_DIAMOND,
              "T3 the pick replaced the equipped camo on every gun");
        CHECK(originals.size() == 3 && originals[100] == 555 && originals[200] == 0 &&
              originals[300] == 777,
              "T3 each conf's own previous value was remembered once");

        ApplyCamoToEveryGun(ID_REDSPR, confs, originals);
        CHECK(confSkin(krm) == ID_REDSPR && confSkin(m4) == ID_REDSPR &&
              confSkin(ak) == ID_REDSPR,
              "T3 a second pick replaces the first on every gun");
        CHECK(originals[100] == 555 && originals[200] == 0 && originals[300] == 777,
              "T3 the remembered originals are still the game's own values");

        ConfObj *newGun = makeConf(400, 99);
        confs.push_back(newGun);
        ApplyCamoToEveryGun(ID_REDSPR, confs, originals);
        CHECK(confSkin(newGun) == ID_REDSPR,
              "T3 a conf added after the first pick is written on the next one");

        ApplyCamoToEveryGun(0, confs, originals);
        CHECK(confSkin(krm) == 555 && confSkin(m4) == 0 && confSkin(ak) == 777,
              "T3 OFF restores each gun's own camo");
        CHECK(confSkin(newGun) == 99,
              "T3 OFF leaves a conf it never wrote completely alone");
    }

    {
        std::unordered_map<uint, int> originals;
        ConfObj *noId = makeConf(0, 1234);
        ConfObj *ok   = makeConf(500, 4321);
        std::vector<void *> confs = { nullptr, noId, ok };

        int written = ApplyCamoToEveryGun(ID_GOLD, confs, originals);
        CHECK(written == 1, "T4 a null conf and an id==0 conf are skipped");
        CHECK(confSkin(noId) == 1234, "T4 an id==0 conf is not written");
        CHECK(confSkin(ok) == ID_GOLD, "T4 the real conf in the same list is written");
        CHECK(originals.size() == 1 && originals.count(500) == 1,
              "T4 no bogus original is stored under id 0");
    }

    {
        camoData.clear();
        InsertCamo(7001, "Gold", 0, 0);
        InsertCamo(7002, "Diamond", 0, 0);
        InsertCamo(7003, "Aether Crystal", 0, 0);
        InsertCamo(7004, "Red Sprite", 0, 0);

        auto matches = [](const char *query) {
            const std::string lower = CamoNameLower(query);
            int n = 0;
            for (const auto &c : camoData)
                if (CamoNameLower(c.camoName).find(lower) != std::string::npos)
                    n++;
            return n;
        };

        CHECK(matches("gold") == 1, "T5 a search for gold finds exactly the Gold camo");
        CHECK(matches("GOLD") == 1, "T5 the search is case-insensitive");
        CHECK(matches("crystal") == 1, "T5 a mid-name search finds Aether Crystal");

        CHECK(matches("a") == 2, "T5 a one-letter search matches mid-name characters");
        CHECK(matches("zzz") == 0, "T5 a search that matches nothing returns nothing");
    }

    {
        camoData.clear();
        InsertCamo(8101, "Gold",       0, 955);
        InsertCamo(8102, "KRM Gold",   0, 955);
        InsertCamo(8103, "AK117 Gold", 0, 0);
        InsertCamo(8104, "M4 Gold",    0, 0);
        CHECK(camoData.size() == 1, "T6 four per-weapon Gold rows = one list entry");
        CHECK(camoData[0].camoName == "Gold", "T6 ... under the canonical name");
        CHECK(camoData[0].applyID == ID_GOLD, "T6 ... applying the proven id");

        int found = 0;
        for (const auto &c : camoData)
            if (CamoNameLower(c.camoName).find("gold") != std::string::npos)
                found++;
        CHECK(found == 1, "T6 a search for \"gold\" lists it exactly once");

        InsertCamo(8105, "Golden Emerald", 0, 0);
        CHECK(camoData.size() == 2, "T6 Golden Emerald stays a separate camo");

        camoData.clear();
        InsertCamo(8201, "Zulu Camo", 0, 0);
        InsertCamo(8202, "Zulu Camo", 0, 955);
        CHECK(camoData.size() == 2,
              "T6 same name + different applied id: both are kept");
    }

    {
        std::unordered_map<uint, WeaponSkinCamoOriginal> originals;
        SkinObj *goldGun = makeSkin(900, ID_GOLD, 3);
        SkinObj *plain   = makeSkin(901, 0, 0);
        std::vector<void *> skins = { goldGun, plain };

        int written = ApplyCamoToEveryWeaponSkin(ID_DIAMOND, skins, originals);
        CHECK(written == 2, "T7 every weapon-skin record is written, camo or not");
        CHECK(skinCamoID(goldGun) == ID_DIAMOND && skinCamoID(plain) == ID_DIAMOND,
              "T7 the gun that already had a camo is changed too");
        CHECK(skinColorSub(goldGun) == 0, "T7 the colour sub-id is reset with the camo");
        CHECK(originals.size() == 2 && originals[900].camoID == ID_GOLD &&
              originals[900].colorSubID == 3 && originals[901].camoID == 0,
              "T7 each record's own camo was remembered once");

        ApplyCamoToEveryWeaponSkin(ID_REDSPR, skins, originals);
        CHECK(skinCamoID(goldGun) == ID_REDSPR,
              "T7 a second pick replaces the first on every record");
        CHECK(originals[900].camoID == ID_GOLD,
              "T7 the remembered original is still the game's own camo");

        SkinObj *later = makeSkin(902, 777, 1);
        skins.push_back(later);
        ApplyCamoToEveryWeaponSkin(ID_REDSPR, skins, originals);
        CHECK(skinCamoID(later) == ID_REDSPR,
              "T7 a record added after the first pick is written on the next one");

        ApplyCamoToEveryWeaponSkin(0, skins, originals);
        CHECK(skinCamoID(goldGun) == ID_GOLD && skinColorSub(goldGun) == 3 &&
              skinCamoID(plain) == 0,
              "T7 OFF restores every record's own camo");
        CHECK(skinCamoID(later) == 777 && skinColorSub(later) == 1,
              "T7 OFF leaves a record it never wrote completely alone");
    }

    {
        std::unordered_map<uint, WeaponSkinCamoOriginal> originals;
        SkinObj *noId = makeSkin(0, 1234, 9);
        SkinObj *ok   = makeSkin(950, 4321, 2);
        std::vector<void *> skins = { nullptr, noId, ok };

        int written = ApplyCamoToEveryWeaponSkin(ID_GOLD, skins, originals);
        CHECK(written == 1, "T8 a null record and an id==0 record are skipped");
        CHECK(skinCamoID(noId) == 1234, "T8 an id==0 record is not written");
        CHECK(skinCamoID(ok) == ID_GOLD, "T8 the real record in the same list is written");
        CHECK(originals.size() == 1 && originals.count(950) == 1,
              "T8 no bogus original is stored under id 0");
    }

    printf("\n");
    if (g_failures == 0) {
        printf("OK: all camo-apply checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("FAILED: %d of %d checks\n", g_failures, g_checks);
    return 1;
}

