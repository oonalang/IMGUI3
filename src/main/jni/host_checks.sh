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

# The reskinned widgets (IMGUI5 pass): a slider whose drag track moved to the
# right of the row, a select field with a tick-marked popup, the button, the
# switch and the card. Driven with real mouse events, because the hit regions
# are what the features depend on - the pixels are not.
"$CXX" -std=c++17 -O1 -I. -IImGui -o "$OUT_DIR/widget_interaction_check" \
    ImGui/tests/widget_interaction_check.cpp \
    ImGui/custom_widgets.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp
"$OUT_DIR/widget_interaction_check"

# The offscreen preview renderer (dev tool): it rasterizes the real widgets with
# the real fonts, so it rots loudly instead of silently if the widget headers
# drift. Its output is not compared pixel-wise, only produced and sized.
"$CXX" -std=c++17 -O1 -I. -IImGui -o "$OUT_DIR/ui_preview_render" \
    ImGui/tests/ui_preview_render.cpp \
    ImGui/custom_widgets.cpp \
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp
"$OUT_DIR/ui_preview_render" "$OUT_DIR/ui_preview.rgb" >/dev/null
preview_bytes=$(stat -c %s "$OUT_DIR/ui_preview.rgb" 2>/dev/null || stat -f %z "$OUT_DIR/ui_preview.rgb")
[ "$preview_bytes" -eq $((1000 * 560 * 3)) ] || { echo "ui_preview_render wrote $preview_bytes bytes"; exit 1; }

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

# Apply-on-equip under ThreadSanitizer: the render thread walks the conf table
# while the ctor hooks append to it. This is the "crashes when I join a game"
# report - pre-fix it prints 7 data races (read of the vector's buffer vs
# push_back) and exits 66; the fix takes g_dataMutex and copies.
"$CXX" -std=c++17 -fsanitize=thread -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    -o "$OUT_DIR/auto_equip_race_check" Skin/tests/auto_equip_race_check.cpp \
    IL2CppSDKGenerator/IL2Cpp/Tools.cpp -lpthread
"$OUT_DIR/auto_equip_race_check"

# Crash breadcrumb (Hacks/Feature/CrashBreadcrumb.h): the formatter and the real
# append-the-line writer the signal handler calls. No signals are raised here -
# only the bytes the user will send back are proven.
"$CXX" -std=c++17 -I. -IHacks/Feature -o "$OUT_DIR/crash_breadcrumb_check" \
    Hacks/Feature/tests/crash_breadcrumb_check.cpp
"$OUT_DIR/crash_breadcrumb_check"

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
