#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
for t in report_block ace_report; do
  g++ -std=c++17 -O1 -w -D__fastcall= -o ".tmp_harness/${t}_check" \
    "src/main/jni/System/Hooks/tests/${t}_check.cpp" \
    -Isrc/main/jni/Skin/tests/stubs -Isrc/main/jni -Isrc/main/jni/ImGui -Isrc/main/jni/stb -Isrc/main/jni/Fonts
done
