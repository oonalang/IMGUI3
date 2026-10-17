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
    ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp \
    System/Texture/box_shadow.cpp
"$OUT_DIR/text_input_check"

"$CXX" -std=c++17 -DIMGUI_DEFINE_MATH_OPERATORS -I. -IImGui -o "$OUT_DIR/menu_layout_check" \
    ImGui/tests/menu_layout_check.cpp
"$OUT_DIR/menu_layout_check"

"$CXX" -std=c++17 -fsyntax-only -DIMGUI_DEFINE_MATH_OPERATORS -D'__fastcall=' \
    -I Skin/tests/stubs -I . -I ImGui -I Includes \
    Skin/tests/skin_headers_check.cpp

echo "host checks OK"
