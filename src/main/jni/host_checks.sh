#!/usr/bin/env bash
# Host checks: the logic tests that must keep passing now that the UI is the
# Neverlose/HexSync menu (menu/ + ImGui/ are the IMGUI5 copies). The old UI
# test binaries went away with the old widget layer; what is left here is the
# feature logic, the skin include graph and Main.cpp itself.
set -euo pipefail
cd "$(dirname "$0")"

CXX=${CXX:-g++}
OUT_DIR=$(mktemp -d)
trap 'rm -rf "$OUT_DIR"' EXIT

COMMON="-DIMGUI_DEFINE_MATH_OPERATORS -D__fastcall= -D__aarch64__ -DHOST_SYNTAX_ONLY -DPROP_VALUE_MAX=92"
INCLUDES="-I Skin/tests/stubs -I . -I ImGui -I Immenu -I menu -I Includes \
 -I curl/curl-android-arm64-v8a/include -I curl/openssl-android-arm64-v8a/include \
 -I SDK/Xhook -I libzip"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/skin_headers_check" Skin/tests/skin_headers_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/skin_headers_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/mythic_attachments_check" System/Hooks/tests/mythic_attachments_check.cpp
"$OUT_DIR/mythic_attachments_check"

# The triggerbot's LOS gate, target choice, fire pacing and release guard, run
# against a fake il2cpp physics world.
"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/los_gate_check" Hacks/Feature/tests/los_gate_check.cpp
"$OUT_DIR/los_gate_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/blueprint_equip_check" System/Hooks/tests/blueprint_equip_check.cpp
"$OUT_DIR/blueprint_equip_check"

# Apply-on-equip under ThreadSanitizer: the render thread walks the conf table
# while the ctor hooks append to it.
"$CXX" -std=c++17 -fsanitize=thread $COMMON $INCLUDES \
    -o "$OUT_DIR/auto_equip_race_check" Skin/tests/auto_equip_race_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/auto_equip_race_check"

# Crash breadcrumb (Hacks/Feature/CrashBreadcrumb.h): the formatter and the real
# append-the-line writer the signal handler calls.
"$CXX" -std=c++17 -I. -IHacks/Feature -DHOST_SYNTAX_ONLY \
    -o "$OUT_DIR/crash_breadcrumb_check" \
    Hacks/Feature/tests/crash_breadcrumb_check.cpp
"$OUT_DIR/crash_breadcrumb_check"

# Apply Skin On Equip (Skin/AutoEquipSkin.h): the real resolver + tick, over fake
# WeaponConfConfig / WeaponSkinConfig rows.
"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/auto_equip_check" Skin/tests/auto_equip_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/auto_equip_check"

# The skin loaders' offsets, the camo catalogue and the character-skin apply.
"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/offset_check" Skin/tests/offset_check.cpp AstralPrtctn/md5.cpp
"$OUT_DIR/offset_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/camo_apply_check" Skin/tests/camo_apply_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/camo_apply_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/character_apply_check" Skin/tests/character_apply_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/character_apply_check"

# Tools::IsPtrValid under ThreadSanitizer.
"$CXX" -std=c++17 -fsanitize=thread -g -O1 -ffunction-sections -fdata-sections \
    -D__fastcall= -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/tools_isvalid_tsan" \
    IL2CppSDKGenerator/IL2Cpp/tests/tools_isvalid_tsan.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -Wl,--gc-sections
"$OUT_DIR/tools_isvalid_tsan"

# Combat-side logic: the ADS answer, the dot geometry, the body profile, the
# feature defaults, and the entity walks (needs the patched ImGui core).
"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/ads_safe_check" Hacks/Feature/tests/ads_safe_check.cpp
"$OUT_DIR/ads_safe_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/body_profile_check" Hacks/Feature/tests/body_profile_check.cpp
"$OUT_DIR/body_profile_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/crosshair_check" Hacks/Feature/tests/crosshair_check.cpp
"$OUT_DIR/crosshair_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/config_defaults_check" Hacks/Feature/tests/config_defaults_check.cpp
"$OUT_DIR/config_defaults_check"

"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/esp_walk_check" Hacks/Feature/tests/esp_walk_check.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp \
    menu/blur.cpp menu/gui.cpp
"$OUT_DIR/esp_walk_check"

# The license gate (System/UI/LoginGate.h): the phase machine, the clipboard
# paste, the off-thread authenticate, the saved key, and the touch hit test.
"$CXX" -std=c++17 $COMMON $INCLUDES \
    -o "$OUT_DIR/login_gate_check" System/UI/tests/login_gate_check.cpp -lpthread
"$OUT_DIR/login_gate_check"

"$CXX" -std=c++17 $COMMON -fsyntax-only $INCLUDES \
    -include Skin/tests/stubs/host_aarch64_shim.h \
    System/Hooks/tests/feats_include_check.cpp

# The Neverlose menu + adapter TUs.
for f in menu/neverlose_menu.cpp menu/hexsync_intro.cpp menu/hexsync_glow.cpp \
         menu/gui.cpp menu/blur.cpp menu/nl_bindings.cpp menu/nl_ui.cpp \
         menu/nl_embedded_assets.cpp; do
    "$CXX" -std=c++17 $COMMON -fsyntax-only $INCLUDES "$f"
done

# Main.cpp itself: the render hook now drives nl_ui, so a broken edit must fail
# here and not at ndk-build.
"$CXX" -std=c++17 -fsyntax-only $COMMON $INCLUDES Main.cpp

echo "host checks OK"
