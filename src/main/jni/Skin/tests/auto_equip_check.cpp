// Host test for "Apply Skin On Equip" (Skin/AutoEquipSkin.h).
//
// The feature: a skin equipped in the GAME's own loadout should land on the gun
// without first being searched for in the SKINS tab. The pick arrives as an item
// id from the equip gates (System/Hooks/BlueprintEquip.h -> AutoEquipSkin_
// NoteCandidate); this test drives the real resolver and tick over fake
// WeaponConfConfig rows (cast base +0x20: BaseWeaponId @0x34, DefWeaponSkinID
// @0x38, ID @0x40) and fake WeaponSkinConfig rows (cast base +0x10: ID @0x24,
// WeaponID @0x28) written at the exact offsets Skin/Fields.h reads.
//
// Host test, not an on-device run (no NDK needed - the Android/JNI headers are
// the stubs in Skin/tests, __fastcall is elided like the real build):
//   g++ -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
//       -I src/main/jni/Skin/tests/stubs -I src/main/jni -I src/main/jni/ImGui \
//       -I src/main/jni/Includes src/main/jni/Skin/tests/auto_equip_check.cpp \
//       src/main/jni/IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread \
//       -o /tmp/auto_equip_check && /tmp/auto_equip_check
// Exits 0 and prints "OK: all apply-on-equip checks passed" on success.
// Android.mk does not wildcard Skin/tests, so it never reaches the APK.
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
// The resolver validates every row with Tools::IsPtrValid, so the real Tools.cpp
// is linked into this test (see the command above) rather than stubbed.
#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"
// Fields.h first: it is a fragment without an include guard, and AutoEquipSkin.h
// relies on the structs/vectors it declares (same order MainFeatureIncludes.h uses).
#include "Skin/Fields.h"
#include "Skin/AutoEquipSkin.h"

// Fields.h only declares these; Skin/Thread.h owns the definitions in the app and
// the Tick() below reads the conf table through the global, so the test defines
// the two containers it drives.
std::vector<void *> weaponConfInstance;
std::vector<void *> weaponSkinConfigInstance;
std::mutex g_dataMutex;   // defined by Skin/Thread.h in the app

// ---------------------------------------------------------------------------
// Host-only link stubs for the real Tools.cpp: it references the inline-hook
// engine and OpenSSL's MD5 and this test reaches neither.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Fake config rows, read through the same casts as the resolver.
// ---------------------------------------------------------------------------
static const size_t OBJ_SIZE = 0x200;

struct ConfObj { alignas(8) unsigned char b[OBJ_SIZE]; };
struct SkinObj { alignas(8) unsigned char b[OBJ_SIZE]; };

template <typename T>
static void put(unsigned char *o, size_t off, T v) { std::memcpy(o + off, &v, sizeof v); }
template <typename T>
static T get(const unsigned char *o, size_t off) { T v; std::memcpy(&v, o + off, sizeof v); return v; }

// WeaponConfConfig, relative to its +0x20 cast base.
static const size_t CF_BASEWEAPON = 0x14;   // BaseWeaponId,    real 0x34
static const size_t CF_DEFSKIN    = 0x18;   // DefWeaponSkinID, real 0x38
static const size_t CF_ID         = 0x20;   // ID,              real 0x40

// WeaponSkinConfig, relative to its +0x10 cast base.
static const size_t SK_ID       = 0x14;     // real 0x24
static const size_t SK_WEAPONID = 0x18;     // real 0x28

static std::vector<std::unique_ptr<ConfObj>> g_confStore;
static std::vector<std::unique_ptr<SkinObj>> g_skinStore;

static ConfObj *makeConf(uint id, uint baseWeaponId, int defSkin)
{
    auto c = std::make_unique<ConfObj>();
    std::memset(c->b, 0, OBJ_SIZE);
    put<uint>(c->b + 0x20, CF_ID, id);
    put<uint>(c->b + 0x20, CF_BASEWEAPON, baseWeaponId);
    put<int>(c->b + 0x20, CF_DEFSKIN, defSkin);
    ConfObj *raw = c.get();
    g_confStore.push_back(std::move(c));
    return raw;
}

static SkinObj *makeSkinRow(uint id, uint weaponID)
{
    auto s = std::make_unique<SkinObj>();
    std::memset(s->b, 0, OBJ_SIZE);
    put<uint>(s->b + 0x10, SK_ID, id);
    put<uint>(s->b + 0x10, SK_WEAPONID, weaponID);
    SkinObj *raw = s.get();
    g_skinStore.push_back(std::move(s));
    return raw;
}

static int  confSkin(const ConfObj *c) { return get<int>(c->b + 0x20, CF_DEFSKIN); }
static void *confPtr(ConfObj *c)       { return (void *)c; }

static std::vector<void *> confsOf(const std::vector<ConfObj *> &rows)
{
    std::vector<void *> out;
    for (ConfObj *c : rows) out.push_back(confPtr(c));
    return out;
}

// ---------------------------------------------------------------------------

// Host-only link stub: the include graph references the A64 hook engine.
void A64HookFunction(void *const symbol, void *const replace, void **result)
{ (void)symbol; if (result) *result = replace; }
int main()
{
    // --- T1: the equipped skin lands on its base gun, nothing else ---------
    {
        ConfObj *base = makeConf(800, 800, 0);
        ConfObj *skin = makeConf(900, 800, 0);
        ConfObj *other = makeConf(700, 700, 0);
        std::vector<void *> confs = confsOf({base, skin, other});

        uint outBase = 0;
        const int written = AutoEquipSkin_ApplyToBase(900, confs, outBase);

        CHECK(written == 1, "T1 exactly one field is written for one base gun");
        CHECK(outBase == 800, "T1 the skin's BaseWeaponId names the gun it goes on");
        CHECK(confSkin(base) == 900, "T1 the base gun now wears the equipped skin");
        CHECK(confSkin(skin) == 0, "T1 the skin's own row is not treated as a gun");
        CHECK(confSkin(other) == 0, "T1 an unrelated gun is untouched");
    }

    // --- T2: applying the same skin twice writes nothing the second time ---
    {
        ConfObj *base = makeConf(810, 810, 0);
        ConfObj *skin = makeConf(910, 810, 0);
        std::vector<void *> confs = confsOf({base, skin});

        g_autoEquipSkinDiag.notSkin = 0;

        uint outBase = 0;
        CHECK(AutoEquipSkin_ApplyToBase(910, confs, outBase) == 1 &&
              outBase == 810 && confSkin(base) == 910,
              "T2 the first equip writes the skin");

        outBase = 0;
        CHECK(AutoEquipSkin_ApplyToBase(910, confs, outBase) == 0 &&
              outBase == 810 && confSkin(base) == 910,
              "T2 a second equip of the same skin resolves but writes nothing");
    }

    // --- T3: equipping another skin REPLACES the one on the gun ------------
    {
        ConfObj *base = makeConf(820, 820, 0);
        ConfObj *first = makeConf(920, 820, 0);
        ConfObj *second = makeConf(921, 820, 0);
        std::vector<void *> confs = confsOf({base, first, second});

        uint outBase = 0;
        AutoEquipSkin_ApplyToBase(920, confs, outBase);
        CHECK(confSkin(base) == 920 && outBase == 820, "T3 skin 920 is on the gun");

        outBase = 0;
        AutoEquipSkin_ApplyToBase(921, confs, outBase);
        CHECK(confSkin(base) == 921 && outBase == 820,
              "T3 a different pick replaces it instead of stacking");
    }

    // --- T4: the id IS the gun (BaseWeaponId names itself) ------------------
    {
        ConfObj *gun = makeConf(830, 830, 1234);
        std::vector<void *> confs = confsOf({gun});
        const int notSkinBefore = g_autoEquipSkinDiag.notSkin;

        uint outBase = 0;
        const int written = AutoEquipSkin_ApplyToBase(830, confs, outBase);

        CHECK(written == 0 && outBase == 0, "T4 equipping a plain gun writes nothing");
        CHECK(confSkin(gun) == 1234, "T4 the gun keeps whatever skin it had");
        CHECK(g_autoEquipSkinDiag.notSkin == notSkinBefore + 1,
              "T4 the id is counted as a gun, not a skin");
    }

    // --- T5: an id with no config row at all -------------------------------
    {
        ConfObj *base = makeConf(840, 840, 7);
        std::vector<void *> confs = confsOf({base});

        uint outBase = 0;
        const int written = AutoEquipSkin_ApplyToBase(987654, confs, outBase);

        CHECK(written == 0 && outBase == 0, "T5 an unknown id resolves to nothing");
        CHECK(confSkin(base) == 7, "T5 no gun is written for an unknown id");
    }

    // --- T6: the WeaponSkinConfig fallback names the base when the conf row
    //         does not (and the conf row is missing entirely) ----------------
    {
        ConfObj *base = makeConf(850, 850, 0);
        std::vector<void *> confs = confsOf({base});
        weaponSkinConfigInstance.push_back((void *)makeSkinRow(950, 850));

        uint outBase = 0;
        const int written = AutoEquipSkin_ApplyToBase(950, confs, outBase);

        CHECK(written == 1 && outBase == 850,
              "T6 a WeaponSkinConfig row supplies the base weapon");
        CHECK(confSkin(base) == 950, "T6 the gun wears it too");

        weaponSkinConfigInstance.clear();
    }

    // --- T7: the tick applies a recorded candidate once --------------------
    {
        weaponConfInstance.clear();
        ConfObj *base = makeConf(860, 860, 0);
        ConfObj *skin = makeConf(960, 860, 0);
        weaponConfInstance = confsOf({base, skin});

        const int candBefore = g_autoEquipSkinCandidates;
        AutoEquipSkin_NoteCandidate(960);
        CHECK(g_autoEquipSkinCandidates == candBefore + 1,
              "T7 the equip gate records the item it was asked about");
        CHECK(g_autoEquipSkinPendingID == 960, "T7 the id is pending for the tick");

        const int resolvedBefore = g_autoEquipSkinDiag.resolved;
        AutoEquipSkin_Tick(true);

        CHECK(confSkin(base) == 960, "T7 one frame later the gun wears the skin");
        CHECK(g_autoEquipSkinPendingID == 0, "T7 the pick is consumed");
        CHECK(g_autoEquipSkinDiag.resolved == resolvedBefore + 1 &&
              g_autoEquipSkinDiag.lastID == 960 && g_autoEquipSkinDiag.lastBase == 860,
              "T7 the diagnostics name the id and the gun it landed on");

        AutoEquipSkin_Tick(true);
        CHECK(confSkin(base) == 960, "T7 a later frame does not re-apply anything");

        weaponConfInstance.clear();
    }

    // --- T8: an id that cannot resolve yet is retried, then dropped --------
    {
        weaponConfInstance.clear();
        const int droppedBefore = g_autoEquipSkinDiag.unresolved;

        AutoEquipSkin_NoteCandidate(4242);
        AutoEquipSkin_Tick(true);
        CHECK(g_autoEquipSkinPendingID == 4242,
              "T8 a pick that arrives before the tables stays pending");

        for (int i = 0; i < 200; ++i)
            AutoEquipSkin_Tick(true);

        CHECK(g_autoEquipSkinPendingID == 0, "T8 the retry window ends");
        CHECK(g_autoEquipSkinDiag.unresolved == droppedBefore + 1,
              "T8 the drop is counted, not silent");
    }

    // --- T9: switching the feature off drops the pick without writing ------
    {
        weaponConfInstance.clear();
        ConfObj *base = makeConf(870, 870, 0);
        ConfObj *skin = makeConf(970, 870, 0);
        weaponConfInstance = confsOf({base, skin});

        AutoEquipSkin_NoteCandidate(970);
        AutoEquipSkin_Tick(false);

        CHECK(confSkin(base) == 0, "T9 with the switch off nothing is written");
        CHECK(g_autoEquipSkinPendingID == 0, "T9 the pending pick is discarded");

        weaponConfInstance.clear();
    }

    // --- T10: two conf rows for the same gun are both written -------------
    {
        ConfObj *first = makeConf(880, 880, 0);
        ConfObj *second = makeConf(880, 880, 0);
        ConfObj *skin = makeConf(980, 880, 0);
        std::vector<void *> confs = confsOf({first, second, skin});

        uint outBase = 0;
        CHECK(AutoEquipSkin_ApplyToBase(980, confs, outBase) == 2 &&
              outBase == 880 && confSkin(first) == 980 && confSkin(second) == 980,
              "T10 every row of the base gun is updated");
    }

    if (g_failures == 0) {
        printf("OK: all apply-on-equip checks passed (%d)\n", g_checks);
        return 0;
    }
    printf("FAILED: %d of %d checks\n", g_failures, g_checks);
    return 1;
}
