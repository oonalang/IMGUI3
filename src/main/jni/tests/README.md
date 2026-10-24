# Host tests

Every `tests/` directory next to a component holds host-side checks that compile
with a normal desktop compiler. They are never part of the NDK build.

`tests/stubs` is the shared include root for those checks: Android/JNI headers,
EGL/GLES, `sys/system_properties.h` and the OpenSSL headers the licence code
expects are stubbed there, so no NDK or system OpenSSL is required.

Run a check from `src/main/jni`:

```
g++ -std=c++17 -O1 -w -Itests/stubs -I. -IImGui -IIL2CppSDKGenerator -Ifoxcheats -IIncludes \
    ImGui/tests/slider_entry_check.cpp ImGui/imgui.cpp ImGui/imgui_draw.cpp \
    ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp -o /tmp/slider_entry_check
/tmp/slider_entry_check
```

`ImGui/tests/*` that render real windows need the four `imgui_*.cpp` files and
define `font::inter_semibold` plus `F50`, which `imgui_widgets.cpp` references.
Checks that only use headers (`menu_layout_check`) compile without them.
