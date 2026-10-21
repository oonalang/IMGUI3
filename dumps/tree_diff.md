# IMGUI3 (this repo, main) vs VARIANT4 — per-file comparison

Generated 2026-10-18T03:06:59Z from the checked-out working trees.

Shared files: 298, of which byte-identical: 265

## Files only in VARIANT4 (vars: /tmp/VARIANT4/src/main/jni)

| lines | file |
|---|---|
| 11738 | Fonts/Font.h |
| 1867 | Fonts/Icon.h |
| 2607 | Fonts/Iconcpp.h |
| 508 | Fonts/SPECIAL.h |
| 20202 | Fonts/fonts.h |
| 6 | IL2CppSDKGenerator/IL2CppSDKGenerator.h |
| 49284 | IMAGE/astral.h |
| 106089 | IMAGE/bg.h |
| 15 | ImGui/Call_ImGui.h |
| 1868 | ImGui/Icon.h |
| 2606 | ImGui/Iconcpp.h |
| 108 | ImGui/ImguiPP.cpp |
| 36 | ImGui/ImguiPP.h |
| 2733 | ImGui/custom_widgets.cpp |
| 88 | ImGui/custom_widgets.hpp |
| 1369 | ImGui/fontch.h |
| 267 | ImGui/imgui_impl_android.cpp |
| 28 | ImGui/imgui_impl_android.h |
| 871 | ImGui/imgui_impl_opengl3.cpp |
| 55 | ImGui/imgui_impl_opengl3.h |
| 786 | ImGui/imgui_impl_opengl3_loader.h |
| 509 | ImGui/imgui_settings.h |
| 604 | ImGui/runtime_preview_helpers.h |
| 561 | ImGui/runtime_preview_menu.h |
| 3529 | ImGui/stb_image.cpp |
| 353 | ImGui/stb_image.h |
| 190 | Includes/Dobby/dobby.h |
| 19 | Includes/get_device_api_level_inlines.h |
| 10203 | System/Texture/DOT.h |
| 328 | System/Texture/box_shadow.cpp |
| 30 | System/Texture/box_shadow.h |
| 3 | System/Texture/main.h |
| 326 | System/UI/Keyboard.h |
| 528 | System/UI/LoadingAnimation.h |
| 27 | System/UI/Logo.h |
| 241 | System/UI/MiscTab.h |
| 48 | System/UI/SearchFunction.h |
| 240 | System/UI/SettingsTab.h |
| 1193 | System/UI/SkinTab.h |
| 80 | System/UI/TextureLoader.h |
| 103 | foxcheats/include/ScanEngine.hpp |
| 25570 | foxcheats/include/json.hpp |
| 187 | foxcheats/include/json_fwd.hpp |

## Files only in this repo (vars: src/main/jni)

| lines | file |
|---|---|
| 409 | FeedbackService.cpp |
| 31 | FeedbackService.h |
| 229 | Hacks/Feature/CrashBreadcrumb.h |
| 43 | Hacks/Feature/Crosshair.h |
| 3839 | Hacks/Feature/NewFeatures.h |
| 205 | Hacks/Feature/tests/ads_safe_check.cpp |
| 247 | Hacks/Feature/tests/body_profile_check.cpp |
| 90 | Hacks/Feature/tests/config_defaults_check.cpp |
| 165 | Hacks/Feature/tests/crash_breadcrumb_check.cpp |
| 450 | Hacks/Feature/tests/crosshair_check.cpp |
| 482 | Hacks/Feature/tests/esp_walk_check.cpp |
| 2180 | Hacks/Feature/tests/los_gate_check.cpp |
| 115 | IL2CppSDKGenerator/IL2Cpp/tests/tools_isvalid_tsan.cpp |
| 296 | ImGui/backends/imgui_impl_android.cpp |
| 29 | ImGui/backends/imgui_impl_android.h |
| 936 | ImGui/backends/imgui_impl_opengl3.cpp |
| 60 | ImGui/backends/imgui_impl_opengl3.h |
| 809 | ImGui/backends/imgui_impl_opengl3_loader.h |
| 5 | Includes/OverlayGeometry.h |
| 173 | Skin/AutoEquipSkin.h |
| 29 | Skin/AutoEquipState.h |
| 321 | Skin/tests/auto_equip_check.cpp |
| 115 | Skin/tests/auto_equip_race_check.cpp |
| 387 | Skin/tests/camo_apply_check.cpp |
| 369 | Skin/tests/character_apply_check.cpp |
| 534 | Skin/tests/offset_check.cpp |
| 98 | Skin/tests/skin_headers_check.cpp |
| 99 | Skin/tests/stubs/EGL/egl.h |
| 32 | Skin/tests/stubs/EGL/eglext.h |
| 303 | Skin/tests/stubs/GLES3/gl3.h |
| 11 | Skin/tests/stubs/android/log.h |
| 41 | Skin/tests/stubs/android/native_activity.h |
| 40 | Skin/tests/stubs/android/native_window.h |
| 18 | Skin/tests/stubs/android/native_window_jni.h |
| 9 | Skin/tests/stubs/host_aarch64_shim.h |
| 250 | Skin/tests/stubs/jni.h |
| 2 | Skin/tests/stubs/openssl/err.h |
| 2 | Skin/tests/stubs/openssl/evp.h |
| 18 | Skin/tests/stubs/openssl/md5.h |
| 2 | Skin/tests/stubs/openssl/pem.h |
| 2 | Skin/tests/stubs/openssl/rsa.h |
| 9 | Skin/tests/stubs/sys/system_properties.h |
| 176 | System/Hooks/BlueprintEquip.h |
| 455 | System/Hooks/MythicAttachments.h |
| 165 | System/Hooks/ReportBlock.h |
| 224 | System/Hooks/tests/blueprint_equip_check.cpp |
| 25 | System/Hooks/tests/feats_include_check.cpp |
| 666 | System/Hooks/tests/mythic_attachments_check.cpp |
| 310 | System/Hooks/tests/report_block_check.cpp |
| 120 | host_checks.sh |
| 9 | menu/blur.cpp |
| 11 | menu/blur.hpp |
| 19 | menu/color_t.hpp |
| 108 | menu/gui.cpp |
| 60 | menu/gui.hpp |
| 1859 | menu/hashes.hpp |
| 18 | menu/hexsync_glow.cpp |
| 282 | menu/hexsync_intro.cpp |
| 14 | menu/hexsync_intro.hpp |
| 635 | menu/neverlose_menu.cpp |
| 11 | menu/neverlose_menu.hpp |
| 22 | menu/neverlose_menu_internal.hpp |
| 204 | menu/nl_bindings.cpp |
| 70 | menu/nl_bindings.hpp |
| 104043 | menu/nl_embedded_assets.cpp |
| 23 | menu/nl_embedded_assets.hpp |
| 366 | menu/nl_ui.cpp |
| 64 | menu/nl_ui.hpp |
| 66 | nl_layout.hpp |
| 1724 | stb_image_write.h |

## Shared files that differ

| VARIANT4 | here | delta | file |
|---|---|---|---|
| 112 | 103 | -9 | Android.mk |
| 78 | 300 | +222 | AstralPrtctn/AntiHook.h |
| 1152 | 1209 | +57 | Hacks/Feature/DrawESP.h |
| 229 | 283 | +54 | Hacks/Feature/Init_Hacks.h |
| 223 | 227 | +4 | Hacks/StructGame/Defines.h |
| 253 | 281 | +28 | Hacks/StructGame/StructSDK.h |
| 200 | 211 | +11 | Hacks/XPremium/Skeleton.h |
| 332 | 332 | +0 | IL2CppSDKGenerator/IL2Cpp/Il2Cpp.cpp |
| 303 | 308 | +5 | IL2CppSDKGenerator/IL2Cpp/Il2Cpp.h |
| 275 | 459 | +184 | IL2CppSDKGenerator/IL2Cpp/Tools.cpp |
| 45 | 47 | +2 | IL2CppSDKGenerator/IL2Cpp/Tools.h |
| 15 | 124 | +109 | ImGui/imconfig.h |
| 15106 | 14862 | -244 | ImGui/imgui.cpp |
| 3267 | 3151 | -116 | ImGui/imgui.h |
| 5315 | 4259 | -1056 | ImGui/imgui_draw.cpp |
| 3440 | 3312 | -128 | ImGui/imgui_internal.h |
| 4162 | 4114 | -48 | ImGui/imgui_tables.cpp |
| 9098 | 8715 | -383 | ImGui/imgui_widgets.cpp |
| 1447 | 1437 | -10 | ImGui/imstb_textedit.h |
| 5085 | 5085 | +0 | ImGui/imstb_truetype.h |
| 367 | 370 | +3 | Includes/Jni_Stuff.h |
| 120 | 116 | -4 | Includes/Macros.h |
| 99 | 97 | -2 | Includes/Utils.h |
| 2638 | 1340 | -1298 | Main.cpp |
| 36 | 32 | -4 | MainFeatureIncludes.h |
| 591 | 1106 | +515 | Skin/Fields.h |
| 1420 | 1919 | +499 | Skin/Thread.h |
| 59 | 81 | +22 | System/Core/Bypass.h |
| 119 | 170 | +51 | System/Core/ConfigManager.h |
| 151 | 221 | +70 | System/Core/SaveConfig.h |
| 966 | 1032 | +66 | System/Hooks/Feats.h |
| 157 | 159 | +2 | System/Hooks/ThrowableAlert.h |
| 88 | 198 | +110 | System/UI/FloatingInfo.h |
