# LANX migration status

Checked at commit `b06af1f` (after `fb54a77`, which only added the textbox and the
keyboard). Every status below comes from a grep against the tree, not from the
commit messages.

## Phase A - Audit

**DONE.**

- Entry point: `src/main/jni/Main.cpp`. The menu lives in the `hook_eglSwapBuffers`
  hook, on the render thread. Backend: `imgui_impl_opengl3` over the game's EGL
  surface (`ImGui_ImplOpenGL3_Init("#version 300 es")`), input from
  `imgui_impl_android`.
- Menu: `Main.cpp` draws the shell and switch-cases the pages. Page bodies:
  `System/UI/{MiscTab,SettingsTab,SkinTab}.h`, ESP/Aim in
  `ImGui/runtime_preview_menu.h`.
- Config: `System/Core/{ConfigManager,SaveConfig}.h`, `Config` struct.
- Theme: `ImGui/imgui_settings.h` (`c::`, `font::`, `main_runtime_theme`).
- Widgets: `ImGui/custom_widgets.{hpp,cpp}` (namespace `custom`).
- Fonts: `Fonts/fonts.h` (`icomoon_page`, `inter_semibold`, `icon_notify`),
  `Fonts/Iconcpp.h` (Font Awesome), `ImGui/fontch.h` (Anonymous Pro).
- ImGui core is `1.90 WIP (18993)`, i.e. the LANX base, and the Android backends
  are the matching pair.

## Phase B - ImGui core + build

**DONE.**

- ImGui core is LANX's `1.90 WIP (18993)` and the backend pair is the matching
  `imgui_impl_android` + `imgui_impl_opengl3` (no Win32/DX11 backend in the
  tree).
- `Android.mk` now lists the eight ImGui translation units explicitly (core,
  tables, widgets, draw, both backends, `custom_widgets.cpp`, `stb_image.cpp`)
  instead of `$(wildcard ImGui/*.c*)`, the reason
  `custom::InputTextWithHint` could be declared but not linked.
- `custom::InputText` / `InputTextWithHint` moved to the header-only
  `ImGui/custom_input_text.h` (`inline`), so the symbol is always emitted in the
  TU that calls it, whatever build files a checkout has.
- FreeType is not built for the NDK and `IMGUI_ENABLE_FREETYPE` is undefined;
  fonts go through stb_truetype. This is now stated in `Android.mk`.
- Repeatable check: `src/main/jni/host_checks.sh` builds the ImGui core + custom
  widgets + `box_shadow.cpp`, runs the textbox/keyboard test and the 64
  menu-layout checks, then syntax-checks the whole skin include graph. All
  pass.

## Phase C - Theme, fonts, widgets

**DONE.**

- Fonts: `Fonts/poppins_fonts.h` now carries Poppins Regular / Medium /
  SemiBold / Bold, subset to the exact ranges the menu loads (basic Latin,
  Latin-1, Cyrillic, general punctuation, currency; 18 KB per weight).
  `Main.cpp` loads all four, plus the LANX IcoMoon icon font
  (`icomoon_page`, 18 px and 20 px logo) and `icon_notify`. The `font::` handles
  from the brief (`regular_m`, `regular_l`, `inter_medium`, `inter_semibold`,
  `bold_font`, `esp_font`, `icomoon_page`, `icomoon_logo`, `icon_notify`) exist;
  `F50`/`F107` stay as aliases because the shell uses them.
- Sizes scale with density: `ro.sf.lcd_density` / 160 feeds `font::SetDensity()`,
  `ui_scale = clamp(density / 2.75, 0.8, 2.5)`, every AddFont size is
  multiplied by it.
- Theme: the second palette system (`g_themes[5]`, `THEME_*`, `ApplyThemePreset`),
  the "warm amber" backdrops and the green `ImColor(118, 187, 117)` accent are
  gone. `c::` holds the LANX values (accent 173,143,233, window 22,22,22/0.71,
  child 60,60,60/0.25, page rounding 4, element rounding 2.5) and
  `main_runtime_theme` is only the accent helpers the shell needs.
- Widgets are the LANX `custom::` set, with no Windows calls (`GetAsyncKeyState`,
  `wtypes`, `D3D*`, `HWND`: 0 hits in project code).
- Verified by `ImGui/tests/fonts_check.cpp`: all four Poppins weights load, the
  atlas builds, Latin/digits/Cyrillic glyphs exist, the icon font loads.

## Phase D - Pages, including Skins

**NOT DONE.**

Evidence:

- Pages are already on `custom::` (`Main.cpp` 91 uses, `SkinTab.h` 20,
  `SettingsTab.h` 21, `MiscTab.h` custom only), including the Skins search
  fields, filter combo and color rows.
- Legacy UI is still compiled and reachable:
  - `ImGui/ImguiPP.{h,cpp}`: unused helper layer with `ImGui::Button` and an
    author tag.
  - `ImGui/runtime_preview_helpers.h`: 604 lines of the old amber/purple menu
    (hand-drawn sliders, `ImGui::BeginCombo` / `Selectable` / `ColorEdit4` /
    `Button`), included by `runtime_preview_menu.h`.
  - `System/Texture/box_shadow.cpp`: the LANX shadow demo window keeps
    `ImGui::Checkbox` / `SliderFloat` / `SliderInt` / `ColorPicker3`.
- The sidebar is custom (`custom::shell::DrawSidebarButton`, `menu_layout.h`),
  not the stock `BeginTabBar` / `BeginTabItem` path; no stock tab calls remain.

## Phase E - Keyboard

**DONE** in `fb54a77`, with a documented deviation.

- `System/UI/Keyboard.h` is the fallback from the brief: a GBoard-shaped ImGui
  keyboard docked at the bottom, driven by `io.WantTextInput`, with shift/caps,
  three layers, backspace repeat, done, paste and 44 dp keys. The system IME
  path (JNI `InputMethodManager` + `InputConnection` queue) is not used because
  the menu is drawn from a `libgspace1.so` EGL hook with no focusable
  `View`/`Window` of its own, so `showSoftInput` has nothing to attach to.
- Covered by `ImGui/tests/text_input_check.cpp` (13 checks, all pass on host).

## Phase F - Comment cleanup

**NOT DONE.**

Evidence:

- `Main.cpp`: 302 whole-line comments, part of them banners and restatements.
- `SkinTab.h`: 73, `SettingsTab.h`: 9, `imgui_settings.h`: 23.
- Unused-comment markers (`TODO`, `FIXME`, `HACK`, `@author`, `=====` banners)
  are still present in project files.

Legend: DONE = verified by grep and by a build; NOT DONE = at least one hit
above is still in the tree.
