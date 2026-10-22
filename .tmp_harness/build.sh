#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
g++ -std=c++17 -O1 -w -o .tmp_harness/esp_shot \
  .tmp_harness/esp_shot.cpp .tmp_harness/host_stubs.cpp \
  src/main/jni/ImGui/imgui.cpp src/main/jni/ImGui/imgui_draw.cpp \
  src/main/jni/ImGui/imgui_tables.cpp src/main/jni/ImGui/imgui_widgets.cpp \
  -Isrc/main/jni -Isrc/main/jni/ImGui -Isrc/main/jni/stb
