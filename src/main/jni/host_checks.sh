#!/usr/bin/env bash
# Host checks for the LANX UI. These compile the real ImGui core, the real
# custom widgets, the textbox/keyboard and the whole skin include graph, so a
# missing definition (custom::InputTextWithHint was one) fails here instead of
# at ndk-build link time.
set -euo pipefail
cd "$(dirname "$0")"

CXX=${CXX:-g++}
OUT_DIR=$(mktemp -d)
trap 'rm -rf "$OUT_DIR"' EXIT

"$CXX" -std=c++17 -O1 -I. -IImGui -o "$OUT_DIR/text_input_check" \
    ImGui/tests/text_input_check.cpp \
    ImGui/custom_widgets.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp
"$OUT_DIR/text_input_check"

"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' -I. -IImGui -IIncludes \
    -o "$OUT_DIR/custom_widgets_link_check" \
    ImGui/tests/custom_widgets_link_check.cpp ImGui/custom_widgets.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_tables.cpp ImGui/imgui_widgets.cpp \
    ImGui/stb_image.cpp
"$OUT_DIR/custom_widgets_link_check"

"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -I. -IImGui -o "$OUT_DIR/menu_layout_check" \
    ImGui/tests/menu_layout_check.cpp
"$OUT_DIR/menu_layout_check"

"$CXX" -std=c++17 -O1 -I. -IImGui -o "$OUT_DIR/fonts_check" \
    ImGui/tests/fonts_check.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp
"$OUT_DIR/fonts_check"

"$CXX" -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    Skin/tests/skin_headers_check.cpp

"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/mythic_attachments_check" System/Hooks/tests/mythic_attachments_check.cpp
"$OUT_DIR/mythic_attachments_check"

# The triggerbot's LOS gate, target choice, fire pacing and release guard, run
# against a fake il2cpp physics world. It was outside this script, so a change
# to the fire path could regress without any check noticing.
"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/los_gate_check" Hacks/Feature/tests/los_gate_check.cpp
"$OUT_DIR/los_gate_check"

"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/blueprint_equip_check" System/Hooks/tests/blueprint_equip_check.cpp
"$OUT_DIR/blueprint_equip_check"

# Apply Skin On Equip (Skin/AutoEquipSkin.h): the real resolver + tick, over fake
# WeaponConfConfig / WeaponSkinConfig rows. Links the real Tools.cpp, so
# Tools::IsPtrValid is the one the device runs.
"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/auto_equip_check" Skin/tests/auto_equip_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/auto_equip_check"

"$CXX" -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -include Skin/tests/stubs/host_aarch64_shim.h \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -I curl/curl-android-arm64-v8a/include -I curl/openssl-android-arm64-v8a/include \
    -I foxcheats -I foxcheats/includes -I SDK/Xhook -I libzip \
    System/Hooks/tests/feats_include_check.cpp

# Main.cpp itself: the menu is built there, so a broken edit has to fail here
# and not at ndk-build. PROP_VALUE_MAX is the only Android property header
# constant the shim does not carry.
"$CXX" -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -DPROP_VALUE_MAX=92 -include Skin/tests/stubs/host_aarch64_shim.h \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -I curl/curl-android-arm64-v8a/include -I curl/openssl-android-arm64-v8a/include \
    -I foxcheats -I foxcheats/includes -I SDK/Xhook -I libzip \
    Main.cpp

echo "host checks OK"
