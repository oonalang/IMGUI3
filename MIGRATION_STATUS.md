# LANX migration status

Verified against the working tree with grep, not memory. Every line below names
the command or file that decided it.

## A — Audit — DONE

This file. Evidence per phase is in each section.

## B — ImGui core + build wiring — NOT DONE

| Check | Result |
|---|---|
| Android backend `ImGui/imgui_impl_android.cpp` + GLES3 `imgui_impl_opengl3.cpp` present | yes |
| `Application.mk` builds `arm64-v8a`, C++17 | yes |
| `custom_widgets.cpp` compiled into `libgspace1.so` | wildcard `$(wildcard $(LOCAL_PATH)/ImGui/*.c*)` expands to `ImGui/custom_widgets.cpp` (verified by evaluating the same `make` expression) — but the reported undefined reference to `custom::InputTextWithHint` proves the wildcard was not enough for the build that produced it |

Still to do: make the ImGui source list explicit instead of a wildcard, and prove
with a link test that every `custom::` symbol referenced by the UI resolves.

## C — Theme, fonts, widgets — NOT DONE

| Check | Result |
|---|---|
| LANX palette in `ImGui/imgui_settings.h` (`c::` namespaces, accent `rgb(173,143,233)`, rounding 15) | done |
| Every interactive control in menu code is `custom::` | done — `grep -rn "ImGui::Checkbox\|SliderFloat\|SliderInt\|Combo\|ColorEdit4\|Button\|BeginTabItem\|InputText\|Selectable"` over `Main.cpp` and `System/UI/` returns only `ImGui::Begin*` container calls, no widget |
| Text font is Poppins | **NOT done** — `Main.cpp:1308` loads `inter_semibold` (Inter), `custom::shell::GetTextFont()` returns it |
| Icon font is the LANX icon set | done — `icomoon_page` (18/20 px) and `F107` (icon ranges `0xe000..0xf8ff`) are loaded, `F50` is the title face |

## D — Pages, including Skins — NOT DONE

| Check | Result |
|---|---|
| Pages live in `Main.cpp` + `System/UI/*.h` (Guest, Search, Misc, Skins, Settings, Keyboard, Loading, Logo, FloatingInfo) | present, all included from `MainFeatureIncludes.h` |
| Old skin page code (`AstralInput`, `showKeyboard`, `activeInputID`) | gone — grep returns nothing outside tests |
| Every page compiles against the new widgets | still to prove with a host build of the UI translation units |

## E — Keyboard — DONE

`System/UI/Keyboard.h`: bottom docked, QWERTY, shift/caps, `?123` / `#+=` layers,
backspace repeat, paste, driven by `io.WantTextInput`, `PanelHeight()` for the
menu inset. `ImGui/tests/text_input_check.cpp` drives the real textbox plus the
real keyboard state machine and passes (13/13).

## F — Comment cleanup and `isLogin` — NOT DONE

| Check | Result |
|---|---|
| Leftover banner / chat-era comments | `Main.cpp` carries 302 comment lines in 2700, `System/UI/SkinTab.h` 73 in 994; several are section banners (`// === NEW FEATURES: Home ===`) and narrative paragraphs that restate the code |
| `isLogin` default | `Main.cpp:934` `static bool isLogin = false;` — must be `true` |

Scope note for the cleanup: comments that document a non-obvious constraint
(native offsets, EGL share-group rules, hook safety, why a guard exists) stay.
Banners, duplicated narration, dead "old version used to" stories and anything
whose only content is the code it sits above go.

## Cross-cutting checks (must be clean at the end)

```
grep -rniE "windows\.h|getasynckeystate|d3d|direct3d|HWND|IDirect3D|dxgi" project code
```
Clean today: hits are only third-party headers (`curl/openssl`, `stb_image_write.h`
binary tables, font byte arrays) — no project source uses them.

```
grep -rn "ImGui::\(Checkbox\|SliderFloat\|SliderInt\|Combo\|ColorEdit4\|Button\|BeginTabItem\|InputText\|Selectable\)" Main.cpp System/UI/
```
Must stay empty.
