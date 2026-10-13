// imgui_settings.h — merged lanx theme configuration + IMGUI3/Android compatibility layer.
// Base: https://github.com/eliwoahzja/lanx  (imgui_settings.h, same colors/animations)
// Android touch additions are grouped under the c::scale / c::ui() scale factor so every
// lanx widget defined in fixed pixels becomes finger-sized without changing its look.
#ifndef IMGUI_SETTINGS_H
#define IMGUI_SETTINGS_H

#include "imgui.h"
#include <string>
#include "imgui_internal.h"   // ImClamp/ImLerp/ImMax helpers
#include "../System/Texture/box_shadow.h"   // RectangleShadow helper (kept from IMGUI3)

// C++17 inline variables: single definitions live in this header so every TU
// (core, widgets, Main.cpp, System/UI) links against one instance.
inline float menu[4] = { 0.6f, 0.5f, 1.0f, 1.0f };
inline ImFont* F50  = nullptr;
inline ImFont* F107 = nullptr;
inline ImFont* Bold = nullptr;

namespace font
{
    // These names are part of the pre-existing project API (used across System/UI and Main.cpp).
    // They are populated from the lanx Poppins font set in hook_eglSwapBuffers().
    inline ImFont* inter_semibold = nullptr;   // lanx: PoppinsSemiBold
    inline ImFont* icomoon_page = nullptr;     // lanx: icomoon icon page glyphs
    inline ImFont* icomoon_logo = nullptr;     // lanx: icomoon logo glyphs
    inline ImFont* esp_font = nullptr;         // lanx: Poppins regular (ESP previews)
    inline ImFont* regular_m = nullptr;        // lanx: PoppinsMedium (headers)
    inline ImFont* regular_l = nullptr;        // lanx: PoppinsMedium large (welcome banner)
    inline ImFont* bold_font = nullptr;        // lanx: PoppinsBold
    inline ImFont* s_inter_semibold = nullptr; // lanx: PoppinsSemiBold small
    inline ImFont* inter_medium = nullptr;     // lanx: PoppinsMedium small
}

// lanx theme (verbatim from https://github.com/eliwoahzja/lanx imgui_settings.h)
inline ImVec4 rgba(int r, int g, int b, float a) {
	return ImVec4(
		(float)r / 255.0f,
		(float)g / 255.0f,
		(float)b / 255.0f,
		a
	);
}

namespace utils
{
	inline ImColor GetColorWithAlpha(ImColor color, float alpha)
	{
		return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
	}

	inline ImVec2 center_text(ImVec2 min, ImVec2 max, const char* text)
	{
		return min + (max - min) / 2 - ImGui::CalcTextSize(text) / 2;
	}

	inline ImColor GetDarkColor(const ImColor& color)
	{
		float r, g, b, a;
		r = color.Value.x;
		g = color.Value.y;
		b = color.Value.z;
		a = 255;

		float darkPercentage = 0.4f;
		float darkR = r * darkPercentage;
		float darkG = g * darkPercentage;
		float darkB = b * darkPercentage;

		return ImColor(darkR, darkG, darkB, a);
	}
	inline ImVec4 ImColorToImVec4(const ImColor& color)
	{
		return ImVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
	}

}

inline namespace c
{
	inline float scale = 1.5f;        // Android touch/UI scale factor (equip the fixed lanx pixels)
	inline float widget_scale = 1.0f; // legacy project knob (kept for existing callers)

	// lanx succinct helper: converts a lanx-designed pixel value into touch space.
	inline float ui(float px) { return px * scale; }

	// lanx palette (unchanged)
	// main_color / second_color follow the project's hue slider (see main_runtime_theme below).
	inline ImColor main_color = rgba(173, 143, 233, 1);
	inline ImColor second_color = rgba(100, 92, 122, 1);

	inline ImColor background_color = rgba(20, 20, 20, 0.5);
	inline ImColor stroke_color(255, 255, 255, 0);
	inline ImColor window_bg_color = rgba(22, 22, 22, 0.71);

	inline ImVec4 separator = ImColor(22, 23, 26);
	// Legacy project consumers also refer to c::accent; it tracks main_color on Android.
	inline ImVec4 accent = rgba(173, 143, 233, 1);

	inline namespace anim
	{
		inline float speed;
		inline ImColor active(114, 149, 255, 255);
		inline ImColor defaults(22, 23, 26, 255);
	}

	inline namespace bg
	{
		inline ImVec4 background = rgba(22, 22, 22, 0.71);
		// Android: 850x596 is desktop-sized; downscaled for phones, scaled up by c::scale in the shell.
		inline ImVec2 size = ImVec2(680, 520);
		inline float rounding = 15.f;
	}

	inline namespace child
	{
		inline ImVec4 background = rgba(60, 60, 60, 0.25);
		inline ImVec4 stroke = ImColor(18, 18, 24, 0);
		inline float rounding = 8.f;
		// legacy project consumers
		inline ImVec4 cap = ImColor(22, 22, 22, 0.71);
		inline float padding = 13.f;
		inline float spacing = 13.f;
	}

	namespace page
	{
		inline ImVec4 background_active = ImColor(21, 22, 25);
		inline ImVec4 background = ImColor(16, 17, 18);

		inline ImVec4 text_hov = ImColor(150, 162, 205);
		inline ImVec4 text = ImColor(150, 162, 205);

		inline float rounding = 4.f;
	}

	inline namespace elements
	{
		inline ImVec4 background_hovered = ImColor(21, 22, 25);
		inline ImVec4 background = ImColor(16, 17, 18);
		inline float rounding = 2.5f;
	}

	inline namespace checkbox
	{
		inline ImVec4 mark = ImColor(255, 255, 255, 255);
		// legacy project consumers (old toggle visuals; kept API, lanx-tinted)
		inline ImVec4 background_on = ImColor(173, 143, 233);
		inline ImVec4 background_off = ImColor(21, 22, 25);
		inline ImVec4 circle_inactive = ImColor(150, 162, 205);
		inline float rounding = 8.f;
	}

	namespace text
	{
		inline namespace label
		{
			inline ImVec4 active = ImColor(255, 255, 255, 255);
			inline ImVec4 hovered = ImColor(205, 205, 205, 255);
			inline ImVec4 defaults = ImColor(150, 150, 150, 220);
		}

		inline namespace description
		{
			inline ImVec4 active = ImColor(200, 200, 200, 102);
			inline ImVec4 hovered = ImColor(200, 200, 200, 63);
			inline ImVec4 defaults = ImColor(200, 200, 200, 40);
		}

		inline ImVec4 text_active = ImColor(255, 255, 255);
		inline ImVec4 text_hov = ImColor(150, 162, 205);
		inline ImVec4 text = ImColor(150, 162, 205);
	}

	// ===== legacy widget block (API kept so existing screens compile; lanx-tinted) =====
	inline namespace widget
	{
		inline ImVec2 size = ImVec2(0, 34.f);
		inline ImVec4 background = ImColor(16, 17, 18);
		inline ImVec4 outlinecolor = ImColor(40, 42, 48);
		inline float rounding = 4.f;
		inline float outline = 1.f;
	}

	inline namespace button
	{
		inline ImVec4 background = ImColor(16, 17, 18);
		inline ImVec4 background_hovered = ImColor(21, 22, 25);
		inline ImVec4 background_active = ImColor(30, 32, 38);
		inline ImVec4 outline = ImColor(40, 42, 48);
		inline float rounding = 4.f;
	}

	inline namespace scrollbar
	{
		// Touch hitbox extension for the lanx scrollbar (px, scaled).
		inline float hitbox_area = 30.f;
		inline bool left_side = false;    // lanx core renders scrollbars on the right again
		inline float gutter_spacing = 4.f;
	}

	inline void ApplyMainWindowStyle(ImGuiStyle& style)
	{
		style.WindowPadding = ImVec2(0.0f, 0.0f);
		style.ItemSpacing = ImVec2(10.0f * scale, 10.0f * scale);
		style.WindowBorderSize = 0.0f;
		style.ScrollbarSize = 8.0f * scale;
	}

	inline float MainTopAreaHeight()
	{
		return 40.0f * scale;
	}

	inline void UpdateTheme(bool dark_mode, const float* accent_rgba, float dt)
	{
		const float k = ImClamp(dt * 12.0f, 0.0f, 0.5f);
		if (accent_rgba) {
			ImVec4 target(accent_rgba[0], accent_rgba[1], accent_rgba[2], 1.f);
			main_color.Value = ImLerp(main_color.Value, target, k);
		}
		second_color = utils::GetDarkColor(main_color);
		accent = main_color.Value;
		stroke_color = utils::GetColorWithAlpha(main_color, 0.5f);
	}

	inline void DrawWindowShadow(const ImVec2& menuSize)
	{
		RectangleShadowSettings shadowSettings;
		shadowSettings.rectPos = ImVec2(0.0f, 0.0f);
		shadowSettings.rectSize = menuSize;
		shadowSettings.sigma = 14.0f;
		shadowSettings.padding = ImVec2(0.0f, 0.0f);
		shadowSettings.rings = 5;
		shadowSettings.spacingBetweenRings = 2;
		shadowSettings.samplesPerCornerSide = 2;
		shadowSettings.shadowColor = ImGui::ColorConvertU32ToFloat4(IM_COL32(8, 11, 14, 210));
		shadowSettings.shadowSize = ImVec2(0.0f, 0.0f);
		drawRectangleShadowVerticesAdaptive(shadowSettings);
	}

	inline void DrawMenuBackdrop(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding, ImTextureID backgroundTexture = nullptr)
	{
		if (drawList == nullptr)
			return;

		if (backgroundTexture != nullptr) {
			drawList->AddImageRounded(backgroundTexture, min, max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), rounding);
		}
		else {
			drawList->AddRectFilled(min, max, ImGui::GetColorU32(background_color.Value), rounding);
		}
	}
}

// page slide animation state (lanx example, unchanged)
inline float page_offset;
inline bool page_is_changing;
inline std::string wanted_category;
inline int wanted_idx;

// ============================================================================
// Legacy project theme block (main_runtime_theme) — kept so every existing
// screen (ESP colors, popup dimming, hue slider) keeps working. It now drives
// the lanx palette above instead of the previous amber/slate one.
// ============================================================================
namespace main_runtime_theme
{
	inline float g_menuHue = 0.76f;

	inline ImVec4 GetAccentVec4(float alpha = 1.0f)
	{
		return ImVec4(menu[0], menu[1], menu[2], alpha);
	}

	inline ImU32 GetAccentU32(float alpha = 1.0f)
	{
		return ImGui::ColorConvertFloat4ToU32(GetAccentVec4(alpha));
	}

	inline ImVec4 GetAccentTint(float strength, float alpha = 1.0f)
	{
		return ImVec4(menu[0] * strength, menu[1] * strength, menu[2] * strength, alpha);
	}

	inline ImU32 GetAccentTintU32(float strength, float alpha = 1.0f)
	{
		return ImGui::ColorConvertFloat4ToU32(GetAccentTint(strength, alpha));
	}

	inline void ApplyAccentFromHue()
	{
		ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.82f, 0.94f, menu[0], menu[1], menu[2]);
		ImGui::ColorConvertHSVtoRGB(g_menuHue, 0.82f, 0.94f,
			c::main_color.Value.x, c::main_color.Value.y, c::main_color.Value.z);
		c::main_color.Value.w = 1.0f;
	}

	inline ImVec4 GetSidebarShellBackgroundColor()
	{
		return ImColor(0, 0, 0, 130);
	}

	inline ImVec4 GetActiveTabBackgroundColor()
	{
		return ImColor(0, 0, 0, 150);
	}

	// Bridges the legacy NAS/palette call sites to the lanx palette. The old
	// multi-palette table collapsed into the lanx dark look + hue slider.
	inline void ApplyThemeState()
	{
		c::scale = 1.35f;              // touch scale: menunat fixed pixels become finger-sized
		c::widget_scale = 1.0f;
		c::accent = ImColor(GetAccentVec4());
		c::main_color = ImColor(GetAccentVec4());
		c::second_color = utils::GetDarkColor(c::main_color);
		c::stroke_color = utils::GetColorWithAlpha(c::main_color, 0.45f);

		// lanx dark look (values from lanx imgui_settings.h)
		c::background_color = rgba(20, 20, 20, 0.5);
		c::window_bg_color = rgba(22, 22, 22, 0.71);
		c::separator = ImColor(22, 23, 26);

		c::bg::background = rgba(22, 22, 22, 0.71);

		c::child::background = rgba(60, 60, 60, 0.25);
		c::child::stroke = ImColor(18, 18, 24, 0);
		c::child::rounding = 8.f;

		c::page::background_active = ImColor(21, 22, 25);
		c::page::background = ImColor(16, 17, 18);
		c::page::text_hov = ImColor(150, 162, 205);
		c::page::text = ImColor(150, 162, 205);
		c::page::rounding = 4.f;

		c::elements::background_hovered = ImColor(21, 22, 25);
		c::elements::background = ImColor(16, 17, 18);
		c::elements::rounding = 2.5f;

		c::checkbox::mark = ImColor(255, 255, 255, 255);
		c::checkbox::background_on = ImColor(c::main_color.Value.x, c::main_color.Value.y, c::main_color.Value.z, 1.0f);
		c::checkbox::background_off = ImColor(21, 22, 25);
		c::checkbox::circle_inactive = ImColor(150, 162, 205);

		c::text::label::active = ImColor(255, 255, 255, 255);
		c::text::label::hovered = ImColor(205, 205, 205, 255);
		c::text::label::defaults = ImColor(150, 150, 150, 220);
		c::text::text_active = ImColor(255, 255, 255);
		c::text::text_hov = ImColor(150, 162, 205);
		c::text::text = ImColor(150, 162, 205);

		c::widget::background = ImColor(16, 17, 18);
		c::widget::outlinecolor = ImColor(40, 42, 48);
		c::button::background = ImColor(16, 17, 18);
		c::button::background_hovered = ImColor(21, 22, 25);
		c::button::background_active = ImColor(30, 32, 38);
		c::button::outline = ImColor(40, 42, 48);

		ImGuiStyle& style = ImGui::GetStyle();
		style.ScrollbarSize = ImMax(6.0f, 3.0f * c::scale);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.07f, 0.60f);
		style.Colors[ImGuiCol_ScrollbarGrab] = utils::ImColorToImVec4(utils::GetColorWithAlpha(c::main_color, 0.7f));
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = utils::ImColorToImVec4(utils::GetColorWithAlpha(c::main_color, 0.85f));
		style.Colors[ImGuiCol_ScrollbarGrabActive] = utils::ImColorToImVec4(c::main_color);
	}
}

#endif // IMGUI_SETTINGS_H
