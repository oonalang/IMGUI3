#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -O1 -w -DIMGUI_DEFINE_MATH_OPERATORS -o .tmp_harness/raspody_menu_check \
  src/main/jni/ImGui/tests/raspody_menu_check.cpp \
  src/main/jni/ImGui/raspody_widgets.cpp src/main/jni/ImGui/raspody_font.cpp \
  src/main/jni/ImGui/imgui.cpp src/main/jni/ImGui/imgui_draw.cpp \
  src/main/jni/ImGui/imgui_tables.cpp src/main/jni/ImGui/imgui_widgets.cpp \
  -Isrc/main/jni -Isrc/main/jni/ImGui -Isrc/main/jni/stb -Isrc/main/jni/Fonts
