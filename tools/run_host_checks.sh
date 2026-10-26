#!/bin/bash
# Host check sweep. Compiles every tests/*.cpp twice: plain, then with the ImGui
# translation units if the first link failed. Records PASS / COMPILE-FAIL / RUN-FAIL.
ROOT=${1:-/workspace/chats/100fd02c9c2bf05da96b99fe}
cd "$ROOT/src/main/jni" || exit 1

FLAGS="-std=c++17 -O1 -w -D__fastcall= -Itests/stubs -I. -IImGui -IIL2CppSDKGenerator -Ifoxcheats -IIncludes \
-Icurl/curl-android-arm64-v8a/include -Icurl/openssl-android-arm64-v8a/include -ISDK/Xhook -Isubstrate \
-include IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h"
IMGUI="ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp"
IMGUI_EXTRA="-DIMGUI_DEFINE_MATH_OPERATORS"

pass=0; cfail=0; rfail=0
for src in $(find . -path "*tests/*.cpp" | sort); do
    name=$(basename "$src" .cpp)
    out=/tmp/hc_$name
    log=/tmp/hc_$name.log
    if ! timeout 240 g++ $FLAGS $IMGUI_EXTRA "$src" -o "$out" >"$log" 2>&1; then
        if ! timeout 300 g++ $FLAGS $IMGUI_EXTRA "$src" $IMGUI -o "$out" >"$log" 2>&1; then
            echo "COMPILE-FAIL  $src"
            cfail=$((cfail+1))
            continue
        fi
    fi
    if timeout 60 "$out" >"$log.out" 2>&1; then
        echo "PASS          $src"
        pass=$((pass+1))
    else
        echo "RUN-FAIL      $src"
        rfail=$((rfail+1))
    fi
done
echo "----"
echo "pass=$pass compile-fail=$cfail run-fail=$rfail"
