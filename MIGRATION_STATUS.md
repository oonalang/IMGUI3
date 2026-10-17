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
  widgets, runs the 41-symbol `custom_widgets_link_check` (takes the address of
  every `custom::` entry point, so a missing or drifted widget cannot reach the
  device link), the textbox/keyboard test, the 64 menu-layout checks, the
  mythic/blueprint hook tests and the skin + Feats include graphs. All pass.

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
- The old font payload is gone as well: `Fonts/fonts.h` lost the 1.9 MB Inter
  array and now only holds `icomoon_page` (the LANX icon font) and
  `icon_notify`; `Fonts/Font.h` (unused, author-tagged) and `Fonts/SPECIAL.h`
  (the unused `F50_data`) are deleted.
- Widgets are the LANX `custom::` set, with no Windows calls (`GetAsyncKeyState`,
  `wtypes`, `D3D*`, `HWND`: 0 hits in project code).
- Verified by `ImGui/tests/fonts_check.cpp`: all four Poppins weights load, the
  atlas builds, Latin/digits/Cyrillic glyphs exist, the icon font loads.

## Phase D - Pages, including Skins

**DONE.**

- Every page is on `custom::` (`Main.cpp` 91 calls, `SkinTab.h` 20,
  `SettingsTab.h` 21, `MiscTab.h` custom only, ESP/Aim in
  `runtime_preview_menu.h`). The Skins page uses `custom::InputTextWithHint`
  for both search fields, `custom::Combo` for the rarity filter and the target
  dropdown, `custom::Page` for its seven sub-tabs, `custom::Checkbox` and
  `custom::Child`; its data model and save/load path are unchanged.
- Legacy UI deleted: `ImGui/ImguiPP.{h,cpp}` (unused `ImGui::Button` layer),
  `ImGui/fontch.h` (unused Anonymous Pro font), `System/Texture/box_shadow.*`
  and `System/Texture/main.h` (shadow demo window with stock widgets and no
  caller). `runtime_preview_helpers.h` went from 604 lines of hand-drawn amber
  widgets to the three LANX child-panel helpers the pages use.
- Navigation is the project sidebar built on LANX surfaces
  (`custom::shell::DrawSidebarButton`, `ComputeSidebarLayout`), plus
  `custom::Page` for the Skins sub-tabs. No `BeginTabBar`/`BeginTabItem` and no
  stock widget call remains in menu code.
- Menu size comes from the display (`ComputeMenuLayout`, `menu_layout.h`), not
  the old fixed 850x596 window.

## Phase E - Keyboard

**DONE** in `fb54a77`, with a documented deviation.

- `System/UI/Keyboard.h` is the fallback from the brief: a GBoard-shaped ImGui
  keyboard docked at the bottom, driven by `io.WantTextInput`, with shift/caps,
  three layers, backspace repeat, done, paste and 44 dp keys. The system IME
  path (JNI `InputMethodManager` + `InputConnection` queue) is not used because
  the menu is drawn from a `libgspace1.so` EGL hook with no focusable
  `View`/`Window` of its own, so `showSoftInput` has nothing to attach to.
- Covered by `ImGui/tests/text_input_check.cpp` (13 checks, all pass on host).
- The menu now starts signed in (`isLogin = true` in `Main.cpp`), so the licence
  window and the loading gate no longer stand between the user and the pages.

## Phase F - Comment cleanup

**DONE.**

- Removed: the `// === NEW FEATURES: ... ===` and `// ---- ... ----` banners in
  `Main.cpp`, the stale style-history comments (the "wobbling glow", "no glow
  halo", "tabs used to be a radial wheel"), the `// --- Camo ---` banner and the
  "Selected (ON) first / Unselected (OFF) below" restatements in `SkinTab.h`,
  the `Dev / Debug options` label, and the two unused title helpers in
  `custom_widgets.cpp` that carried LANX's "Zenx"/"Veld" comments.
- Build instructions in the test files now point at `host_checks.sh` instead of
  repeating command lines that mention deleted files.
- Kept: comments that document a dump offset, hooking constraint, JNI/EGL
  requirement or a magic number (e.g. the `get_UseSlideTackleLimit` 8-byte
  patch explanation, the `EGL_ALPHA_SIZE` requirement, the SurfaceControl
  `SKIP_SCREENSHOT` flags). Those fall under the "non-obvious logic" rule.
- Untouched: ImGui/FreeType/stb/json/NDK sources and the embedded font headers.
- `grep -rn "TODO\|FIXME\|HACK\|@author\|====="` in project UI files (excluding
  third-party ImGui) now returns nothing.

Legend: DONE = verified by grep and by a build; NOT DONE = at least one hit
above is still in the tree.

## Final verification (re-run after all phases)

All greps below are clean in project code (`Main.cpp`, `System/`, the project's
own `ImGui` additions). Third-party ImGui keeps its own guarded `_WIN32` block.

| Check | Command | Result |
|---|---|---|
| Stock widgets in menu code | `grep -rnE "\bImGui::(Checkbox\|SliderFloat2?\|SliderInt\|Combo\|ColorEdit4\|ColorPicker[34]\|Button\|BeginTabItem\|BeginTabBar\|InputText(\|WithHint)\|Selectable\|BeginCombo)\b" Main.cpp System ImGui/runtime_preview*.h ImGui/custom_widgets.cpp` | no hits |
| Windows / DirectX / async keys | `grep -rniE "windows\.h\|wtypes\|getasynckeystate\|d3d\|dxgi\|hwnd\|hinternet\|directx_blur"` over the same files | no hits |
| Deleted legacy files still referenced | `grep -rn "ImguiPP\|fontch\|box_shadow\|System/Texture/main.h"` | no hits |
| Old fonts still loaded | `grep -n "AddFontFromMemory" Main.cpp` | Poppins x4, LANX icon font (`icomoon_page`) at 18 and 20 px, `icon_notify`, plus the Font Awesome icon font whose codepoints the project's `ICON_FA_*` icons use (the LANX `icomoon_page` array is a 16-glyph subset, far too small to replace it) |
| Useless comments | `grep -rnE "TODO\|FIXME\|@author\|@autor\|// ={6,}"` over project files | no hits |
| Signed-in start | `grep -n "isLogin" Main.cpp` | `static bool isLogin = true;` |

Repeatable build check: `src/main/jni/host_checks.sh` (textbox/keyboard 13 checks,
menu layout 64 checks, Poppins + icon font atlas, skin include graph syntax,
mythic-attachment 16 checks) - all pass.

## Acceptance checklist

| Item | Verdict |
|---|---|
| Builds with the provided build files, no unresolved symbols | PASS on the host harness (`host_checks.sh`). NOT VERIFIED on device: the NDK toolchain is not available here, so the AIDE/ndk-build link still has to be run by the user. The specific failure reported (`custom::InputTextWithHint`) is fixed root and branch. |
| No `Windows.h`, `d3d`, `dxgi`, `D3DX`, `HWND`, `HINTERNET`, `GetAsyncKeyState` left | PASS (project code) |
| LANX fonts and icon font load, sizes scale with density | PASS for loading (`fonts_check`); density scaling wired from `ro.sf.lcd_density`; visual check needs the device |
| Window visibly more compact, fits portrait and landscape | PARTIAL: layout now comes from the display (`ComputeMenuLayout`), the old fixed 850x596 is not used; eyeball needed |
| Tabs, buttons, checkboxes, sliders, combos, multi-combos, color pickers all `custom::` | PASS |
| Skins page and all other pages use the LANX textbox, dropdown, color picker | PASS (no color picker exists on the Skins page in this project; ESP colors use `custom::ColorEdit4`) |
| Tapping a textbox opens the keyboard; typing, backspace, Enter, paste, IME composition work; keyboard hides; menu not covered | PARTIAL: covered by the host test for tap/focus/typing/backspace/done/layers; the system-IME path was not used (no focusable View to attach to) and device confirmation is needed |
| Overlay focus flags restored after input | NOT APPLICABLE: the menu runs on the game's EGL surface, no `WindowManager` overlay focus flags in this build |
| All previous settings load and save correctly | NOT VERIFIED: config code was not touched, needs a device run |
| Useless comments removed, third-party untouched | PASS |
| Textures and fonts survive EGL context loss and pause/resume | NOT VERIFIED on device |
