# UI replacement status - Neverlose/HexSync menu

Follows `REPLACE_EVERYTHING.md`: the whole old ImGui menu is gone, the menu is
now IMGUI5's Neverlose/HexSync one, and the render loop drives it through
`nl_ui`. Every verdict below comes from a grep or a command in this repo.

## Replaced

| Old | Now |
|---|---|
| `ImGui/` (LANX 1.90 core, `imgui_impl_android`, `imgui_impl_opengl3`, `custom_widgets.*`, `custom_input_text.h`, `imgui_settings.h`, `menu_layout.h`, `runtime_preview*`) | IMGUI5's patched core + `ImGui/backends/imgui_impl_android.*` + `backends/imgui_impl_opengl3.*`. Exactly one `imgui.cpp` in the tree. |
| `System/UI/{SkinTab,SettingsTab,MiscTab,SearchFunction,Keyboard,LoadingAnimation,Logo,TextureLoader}.h` | deleted; no `#include` of them remains |
| `Fonts/` (Inter payload, Poppins, IcoMoon subset), `IMAGE/{astral,bg}.h` | deleted; the four menu fonts, `avatar.rgba` and `logo.rgba` are compiled into `menu/nl_embedded_assets.cpp` (this mod lives in another process, APK assets are not readable) |
| old theme, old keyboard, old licence window | deleted. `namespace font` in `Main.cpp` is now only `inter_semibold`, the handle the floating-info HUD uses |

## Added unchanged from IMGUI5

- `ImGui/` core + `backends/`
- `menu/`: `neverlose_menu`, `hexsync_intro`, `hexsync_glow`, `gui`, `blur`,
  `hashes`, `color_t.hpp`, `nl_ui`, `nl_embedded_assets`
- `nl_layout.hpp`

Two edits inside the copied menu: the sidebar label of the Players sub-page
said "World" twice, and the four rows that only ever drove the menu's own demo
state ("Quick Peeking", "Quick Stopping", "Map Overview", "Radar Reveal") now
carry real features (see bindings below).

## Wiring (Main.cpp, hook_eglSwapBuffers)

```
nl_ui::SetConfig({0.97f, 1.0f, false, false});   // overlay: no intro, no background
nl_ui::SetAssetReader(nl_embedded_asset_reader);
nl_ui::Init(g_GlWidth, g_GlHeight);
... every frame:
ImGui_ImplOpenGL3_NewFrame();
nl_ui::BeginFrame(w, h);
ImGui::NewFrame();
... feature draw lists (ESP, dot crosshair, floating info) ...
nl_ui::Draw();
ImGui::Render();
nl_ui::EndFrame();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
```

Touch goes in as screen pixels (`nl_ui::OnTouch(Down/Move/Up, x, y)`), the
adapter maps it back onto the logical canvas. `nl_ui::BeginFrame` re-fits the
layout and re-rasterises the font atlas whenever the surface size changes, so a
rotation keeps the menu sharp and centred. One-time setup happens where the GL
context is guaranteed current (after `ImGui_ImplOpenGL3_Init`).

Two behaviours came out of the old menu and were dropped with it:

- the licence gate (stock-widget window, `isLogin` was already hard-wired to
  `true`, so it could never show) and its login-text loader;
- `DrawEspPreview`, the old hand-drawn skin preview (no caller left once the
  old Skins page went).

The memory-patch toggles that used to live inside the menu's signed-in branch
now run every frame, independent of `nl_ui::Visible()`: closing the menu no
longer freezes a feature in its last applied state.

## Feature bindings (menu/nl_bindings.{hpp,cpp})

Every menu row is bound to a real flag - no row is left on the menu's demo
state. Toggles: aim 360 / bullet track, triggerbot + activation mode, hitbox,
recoil, spread, shake, overheat, parachute, flash, firerate, unlimited fly, fast
dive/reload/scope, quick switch, weapon kinetic, mythic attachments, no
slide-fatigue / crouch / hit-flinch / drift / sprint-fire delay / hipfire delay,
ESP line/box/skeleton/health/name/distance/count/alert/aimline/wallhacks, dot
crosshair, blueprints, auto-equip skin, hidden overlay, block report, spectate,
walk under water, no smoke, no gravity, smart reload. Selects: aim target /
trigger / sort, triggerbot mode, box type, line position, health position, ESP
style. Sliders: aim assist size, FOV, dot opacity/size/offsets, snowboard
speed, slide distance, speed multiplier, jump height. Actions: save config,
load config, reset banned guest.

## Verification

- `src/main/jni/host_checks.sh` - skin include graph, mythic attachments, LOS
  gate, blueprint equip, apply-on-equip (under TSan), crash breadcrumb, apply
  skin on equip, Feats include graph, all eight menu TUs and `Main.cpp`.
  Result: `host checks OK`.
- `grep -rn "ImGui::Begin(" --include=*.cpp --include=*.h` - only
  `menu/neverlose_menu.cpp` and `menu/hexsync_intro.cpp`, plus ImGui's own
  core sources.
- `find . -name imgui.cpp` - one.
- `grep -rniE "custom::|imgui_settings|nl_style|SkinTab|SettingsTab|MiscTab|VirtualKeyboard|keyboard_ui"`
  - no hits outside ImGui's own sources.
- `Android.mk` lists every menu TU explicitly, adds `menu` to the include path,
  compiles C++17 with `-DIMGUI_IMPL_OPENGL_ES3`, links
  `-lGLESv3 -lGLESv2 -lEGL -llog -landroid`.

## Not verified here

- The NDK build itself: no Android toolchain in this environment, so
  `ndk-build`/AIDE has to be run by the user.
- On-device look and feel: overlay transparency, font sharpness after
  rotation, touch feel in-game.
- Config compatibility with files saved by the old menu: the `sConfig` layout
  was not touched, the new rows write the same fields.
