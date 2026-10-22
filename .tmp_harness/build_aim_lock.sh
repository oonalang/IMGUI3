#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -O1 -w -DIMGUI_DEFINE_MATH_OPERATORS -D__fastcall= -o .tmp_harness/aim_lock_check \
  src/main/jni/Hacks/Feature/tests/aim_lock_check.cpp \
  -Isrc/main/jni/Skin/tests/stubs -Isrc/main/jni -Isrc/main/jni/ImGui -Isrc/main/jni/stb -Isrc/main/jni/Fonts
