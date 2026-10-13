// custom_widgets.cpp — widget layer swapped to the lanx set.
// Source: https://github.com/eliwoahzja/lanx examples/example_win32_directx11/custom_widgets.cpp
// Android/touch adaptations:
//   * every fixed lanx pixel goes through T() = c::ui(v) = v * c::scale (touch hitboxes)
//   * Sounds.hpp / Winmm sound bells stripped (PC-only, winmm.lib does not exist on Android)
//   * strcpy_s (MSVC-only) replaced with snprintf
//   * KnobScalar GetAsyncKeyState(VK_OEM_PLUS/MINUS) micro-adjust stripped (PC-only;
//     knobs still drag on touch through DragBehavior)
//   * Keybind/MiniBind keep the mouse-button capture; keyboard capture is driven by
//     io.KeysDown which stays empty on Android (no physical keyboard) — untouched API.
#include "custom_widgets.hpp"
#include "imgui_settings.h"
#include "font_defines.h"
#include "Icon.h"
#include <cmath>
#include <map>
#include <string>

extern ImFont* F50;
extern ImFont* F107;
extern ImFont* Bold;

using namespace ImGui;

namespace custom
{
	// Touch scale: lanx designed fixed-pixel widgets for desktop. On Android the
	// menu renders at window-buffer scale, so multiply the small geometry by
	// c::scale (1.0 = lanx exact desktop size).
	static inline float T(float v) { return c::ui(v); }

	int rotation_start_index;
	void ImRotateStart()
	{
		rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
	}

	ImVec2 ImRotationCenter()
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

		const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
	}

	void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
	{
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}

	const char* keys[] =
	{
		"-",
		"Mouse 1",
		"Mouse 2",
		"CN",
		"Mouse 3",
		"Mouse 4",
		"Mouse 5",
		"-",
		"Back",
		"Tab",
		"-",
		"-",
		"CLR",
		"Enter",
		"-",
		"-",
		"Shift",
		"CTL",
		"Menu",
		"Pause",
		"Caps Lock",
		"KAN",
		"-",
		"JUN",
		"FIN",
		"KAN",
		"-",
		"Escape",
		"CON",
		"NCO",
		"ACC",
		"MAD",
		"Space",
		"PGU",
		"PGD",
		"End",
		"Home",
		"Left",
		"Up",
		"Right",
		"Down",
		"SEL",
		"PRI",
		"EXE",
		"PRI",
		"INS",
		"Delete",
		"HEL",
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"A",
		"B",
		"C",
		"D",
		"E",
		"F",
		"G",
		"H",
		"I",
		"J",
		"K",
		"L",
		"M",
		"N",
		"O",
		"P",
		"Q",
		"R",
		"S",
		"T",
		"U",
		"V",
		"W",
		"X",
		"Y",
		"Z",
		"WIN",
		"WIN",
		"APP",
		"-",
		"SLE",
		"Numpad 0",
		"Numpad 1",
		"Numpad 2",
		"Numpad 3",
		"Numpad 4",
		"Numpad 5",
		"Numpad 6",
		"Numpad 7",
		"Numpad 8",
		"Numpad 9",
		"MUL",
		"ADD",
		"SEP",
		"MIN",
		"Delete",
		"DIV",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"F13",
		"F14",
		"F15",
		"F16",
		"F17",
		"F18",
		"F19",
		"F20",
		"F21",
		"F22",
		"F23",
		"F24",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"NUM",
		"SCR",
		"EQU",
		"MAS",
		"TOY",
		"OYA",
		"OYA",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"-",
		"Shift",
		"Shift",
		"Ctrl",
		"Ctrl",
		"Alt",
		"Alt"
	};

	// ------------------------------------------------------------------ shell
	// The project's own shell API (previously native-widget based); now tinted
	// with the lanx palette. F50/F107 aliases keep the legacy icon sizes.
	namespace shell
	{
		ImFont* GetTextFont()
		{
			if (font::inter_semibold)
				return font::inter_semibold;
			if (F50)
				return F50;
			return ImGui::GetFont();
		}

		ImFont* GetTitleFont()
		{
			if (F50)
				return F50;
			return GetTextFont();
		}

		ImFont* GetIconFont()
		{
			if (F107)
				return F107;
			return GetTextFont();
		}

		ImVec2 MeasureText(ImFont* font, float size, const char* text)
		{
			if (font == nullptr || text == nullptr)
				return ImVec2(0.0f, 0.0f);
			return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
		}

		float Normalize(float value, float minValue, float maxValue)
		{
			const float range = maxValue - minValue;
			if (range <= 0.0001f)
				return 0.0f;
			return ImClamp((value - minValue) / range, 0.0f, 1.0f);
		}

		void DrawPanelShell(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding, bool highlighted)
		{
			if (drawList == nullptr)
				return;

			drawList->AddRectFilled(
				min,
				max,
				ImGui::GetColorU32(highlighted ? main_runtime_theme::GetActiveTabBackgroundColor() : main_runtime_theme::GetSidebarShellBackgroundColor()),
				rounding
			);
			drawList->AddRectFilledMultiColor(
				min,
				ImVec2(max.x, min.y + (max.y - min.y) * 0.48f),
				main_runtime_theme::GetAccentTintU32(0.22f, highlighted ? 0.12f : 0.08f),
				main_runtime_theme::GetAccentTintU32(0.16f, highlighted ? 0.07f : 0.04f),
				IM_COL32(0, 0, 0, 0),
				IM_COL32(0, 0, 0, 0)
			);
		}

		bool DrawSidebarButton(const char* id, const char* icon, bool active, const ImVec2& min, const ImVec2& size)
		{
			ImGui::SetCursorScreenPos(min);
			ImGui::InvisibleButton(id, size);
			const bool hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();
			const bool clicked = ImGui::IsItemClicked();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const float slotSide = ImMax(1.0f, ImMin(size.y - 2.0f, size.x - 12.0f));
			const ImVec2 slotMin(min.x + (size.x - slotSide) * 0.5f, min.y + (size.y - slotSide) * 0.5f);
			const ImVec2 slotMax(slotMin.x + slotSide, slotMin.y + slotSide);
			if (active || hovered)
			{
				drawList->AddRectFilled(
					slotMin,
					slotMax,
					active ? ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor()) : GetColorU32(c::page::background_active),
					9.0f
				);
			}

			ImFont* iconFont = font::icomoon_page ? font::icomoon_page : GetIconFont();
			const float iconSize = T(18.5f);
			const ImVec2 iconTextSize = MeasureText(iconFont, iconSize, icon);
			drawList->AddText(
				iconFont,
				iconSize,
				ImVec2(slotMin.x + (slotSide - iconTextSize.x) * 0.5f, slotMin.y + (slotSide - iconTextSize.y) * 0.5f),
				active ? main_runtime_theme::GetAccentU32() : GetColorU32(c::text::label::defaults),
				icon
			);

			return clicked;
		}

		void DrawTitleCard(const ImVec2& min, const ImVec2& size, const char* title)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), c::child::rounding, true);

			ImFont* titleFont = GetTitleFont();
			const float titleFontSize = titleFont->FontSize * 1.05f;

			const ImVec2 titleSize = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, title);
			drawList->AddText(
				titleFont, titleFontSize,
				ImVec2(min.x + 14.0f, min.y + (size.y - titleSize.y) * 0.5f - 1.0f),
				GetColorU32(c::text::label::active),
				title
			);
		}

		bool DrawMenuColorCard(const ImVec2& min, const ImVec2& size)
		{
			const float trackHeight = 8.0f;
			const ImVec2 trackMin(min.x + 12.0f, min.y + (size.y - trackHeight) * 0.5f);
			const ImVec2 trackMax(min.x + size.x - 12.0f, trackMin.y + trackHeight);

			ImGui::SetCursorScreenPos(ImVec2(trackMin.x - 2.0f, trackMin.y - 6.0f));
			ImGui::InvisibleButton("##main_runtime_menu_color_slider", ImVec2((trackMax.x - trackMin.x) + 4.0f, 20.0f));
			const bool active = ImGui::IsItemActive();
			const bool hovered = ImGui::IsItemHovered() || active;
			bool changed = false;
			if (active)
			{
				main_runtime_theme::g_menuHue = Normalize(ImGui::GetIO().MousePos.x, trackMin.x, trackMax.x);
				main_runtime_theme::ApplyAccentFromHue();
				changed = true;
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), c::child::rounding, hovered || active);

			const float segments = 6.0f;
			const float segmentWidth = (trackMax.x - trackMin.x) / segments;
			drawList->AddRectFilled(
				ImVec2(trackMin.x - 4.0f, trackMin.y - 4.0f),
				ImVec2(trackMax.x + 4.0f, trackMax.y + 4.0f),
				IM_COL32(8, 9, 14, 220),
				5.0f
			);

			for (int i = 0; i < (int)segments; ++i)
			{
				float r0 = 0.0f, g0 = 0.0f, b0 = 0.0f;
				float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
				ImGui::ColorConvertHSVtoRGB((float)i / segments, 0.78f, 1.0f, r0, g0, b0);
				ImGui::ColorConvertHSVtoRGB((float)(i + 1) / segments, 0.78f, 1.0f, r1, g1, b1);
				const ImVec2 segMin(trackMin.x + segmentWidth * i, trackMin.y);
				const ImVec2 segMax((i == (int)segments - 1) ? trackMax.x : (segMin.x + segmentWidth + 1.0f), trackMax.y);
				drawList->AddRectFilledMultiColor(
					segMin,
					segMax,
					ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f))
				);
			}
			drawList->AddRect(trackMin, trackMax, IM_COL32(64, 67, 79, 220), 4.0f, 0, 1.0f);

			const float knobX = ImLerp(trackMin.x, trackMax.x, main_runtime_theme::g_menuHue);
			const float knobHalfWidth = active ? 4.0f : 3.5f;
			const float knobHalfHeight = active ? 8.0f : 7.0f;
			const ImVec2 knobCenter(knobX, (trackMin.y + trackMax.y) * 0.5f);
			const ImVec2 knobMin(knobCenter.x - knobHalfWidth, knobCenter.y - knobHalfHeight);
			const ImVec2 knobMax(knobCenter.x + knobHalfWidth, knobCenter.y + knobHalfHeight);
			drawList->AddRectFilled(
				knobMin - ImVec2(2.0f, 2.0f),
				knobMax + ImVec2(2.0f, 2.0f),
				main_runtime_theme::GetAccentTintU32(0.18f, active ? 0.26f : 0.18f),
				6.0f
			);
			drawList->AddRectFilled(knobMin, knobMax, main_runtime_theme::GetAccentU32(), 4.0f);

			return changed;
		}

		bool DrawCloseCard(const ImVec2& min, const ImVec2& size)
		{
			ImGui::SetCursorScreenPos(min);
			ImGui::InvisibleButton("##main_runtime_close_header_card", size);
			const bool hovered = ImGui::IsItemHovered();
			const bool held = ImGui::IsItemActive();
			const bool clicked = ImGui::IsItemClicked();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), c::child::rounding, hovered || held);

			// lanx-style single power glyph; typed as icomoon if the font is around.
			ImFont* iconFont = font::icomoon_page ? font::icomoon_page : GetIconFont();
			const char* powerIcon = ICON_POWER_LINE;
			if (!(font::icomoon_page != nullptr && MeasureText(iconFont, T(22.0f), powerIcon).x > 1.0f))
			{
				iconFont = GetIconFont();
				powerIcon = ICON_FA_POWER_OFF;
			}
			const float iconFontSize = held ? T(22.0f) : T(20.5f);
			const ImVec2 iconSize = MeasureText(iconFont, iconFontSize, powerIcon);
			drawList->AddText(
				iconFont,
				iconFontSize,
				ImVec2(min.x + (size.x - iconSize.x) * 0.5f, min.y + (size.y - iconSize.y) * 0.5f - 1.0f),
				hovered || held ? main_runtime_theme::GetAccentU32() : GetColorU32(c::text::label::active),
				powerIcon
			);

			return clicked;
		}
	}

	// ------------------------------------------------------------------ keybind
	struct key_state
	{
		ImVec4 background, text, icon;
		bool active = false;
		bool hovered = false;
		float alpha = 0.f;
		float size_x;
	};

	bool Keybind(const char* label, int* key, int* mode)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		ImGuiIO& io = g.IO;
		const ImGuiStyle& style = g.Style;

		const ImGuiID id = window->GetID(label);
		const float width = (GetContentRegionMax().x - style.WindowPadding.x);

		const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, T(19)));

		ItemSize(ImRect(rect.Min, rect.Max));
		if (!ItemAdd(rect, id)) return false;

		char buf_display[64] = "None";
		bool value_changed = false;
		int k = *key;

		std::string active_key = "";
		active_key += keys[*key];

		if (*key != 0 && g.ActiveId != id)
			snprintf(buf_display, sizeof(buf_display), "%s", active_key.c_str());
		else if (g.ActiveId == id)
			snprintf(buf_display, sizeof(buf_display), "...");
			

		const ImVec2 label_size = CalcTextSize(buf_display, NULL, true);

		ImRect clickable(ImVec2(rect.Max.x - T(25) - label_size.x, rect.Min.y), rect.Max - ImVec2(T(5.f), 0));
		bool hovered = ItemHoverable(clickable, id, ImGuiItemFlags_None);

		static std::map<ImGuiID, key_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, key_state() });
			it_anim = anim.find(id);
		}
		it_anim->second.text = ImLerp(it_anim->second.text, it_anim->second.active ? c::text::label::active : hovered ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());
		it_anim->second.background = ImLerp(it_anim->second.background, g.ActiveId == id ? c::main_color.Value : c::background_color.Value, GetAnimSpeed());
		it_anim->second.icon = ImLerp(it_anim->second.icon, g.ActiveId == id ? c::main_color.Value : hovered ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());

		window->DrawList->AddRectFilled(clickable.Min, clickable.Max, GetColorU32(it_anim->second.background), c::elements::rounding);

		PushFont(font::icomoon_page);
		window->DrawList->AddText(ImVec2(rect.Min.x + T(7.f), utils::center_text(rect.Min, rect.Max, "k").y), GetColorU32(it_anim->second.icon), "k");
		PopFont();

		window->DrawList->AddText(ImVec2(rect.Min.x, utils::center_text(rect.Min, rect.Max, label).y), GetColorU32(it_anim->second.text), label);

		PushStyleColor(ImGuiCol_Text, c::text::text_active);
		RenderTextClipped(clickable.Min, clickable.Max, buf_display, NULL, &label_size, ImVec2(0.5f, 0.5f));
		PopStyleColor();

		if ((hovered || g.ActiveId == id) && io.MouseClicked[0])
		{
			if (g.ActiveId != id) {
				memset(io.MouseDown, 0, sizeof(io.MouseDown));
				memset(io.KeysDown, 0, sizeof(io.KeysDown));
				*key = 0;
			}
			ImGui::SetActiveID(id, window);
			ImGui::FocusWindow(window);
		}
		else if (io.MouseClicked[0]) {
			if (g.ActiveId == id)
				ImGui::ClearActiveID();
		}

		if (g.ActiveId == id) {
			for (auto i = 0; i < 5; i++) {
				if (io.MouseDown[i]) {
					switch (i) {
					case 0: k = 0x01; break;
					case 1: k = 0x02; break;
					case 2: k = 0x04; break;
					case 3: k = 0x05; break;
					case 4: k = 0x06; break;
					}
					value_changed = true;
					ImGui::ClearActiveID();
				}
			}
			// Keyboard binding stays possible for external keyboard users; the
			// io.KeysDown feed is Android's soft-key layer (usually empty).
			if (!value_changed) {
				for (auto i = 0x08; i <= 0xA5; i++) {
					if (io.KeysDown[i]) {
						k = i;
						value_changed = true;
						ImGui::ClearActiveID();
					}
				}
			}

			if (IsKeyPressedMap(ImGuiKey_Escape)) {
				*key = 0;
				ImGui::ClearActiveID();
			}
			else {
				*key = k;
			}
		}

		return value_changed;
	}


	bool MiniBind(const char* label, int* key, int* mode)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		ImGuiIO& io = g.IO;
		const ImGuiStyle& style = g.Style;

		const ImGuiID id = window->GetID(label);
		const float width = (GetContentRegionMax().x - style.WindowPadding.x) - T(40);

		static std::map<ImGuiID, key_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, key_state() });
			it_anim = anim.find(id);
		}

		const ImRect rect(window->DC.CursorPos + ImVec2(width - T(20) - it_anim->second.size_x, 0), window->DC.CursorPos + ImVec2(width, T(19)));

		ItemSize(ImRect(rect.Min, rect.Max));
		if (!ImGui::ItemAdd(rect, id)) return false;

		char buf_display[64] = "None";
		bool value_changed = false;
		int k = *key;

		std::string active_key = "";
		active_key += keys[*key];

		if (*key != 0 && g.ActiveId != id)
			snprintf(buf_display, sizeof(buf_display), "%s", active_key.c_str());
		else if (g.ActiveId == id)
			snprintf(buf_display, sizeof(buf_display), "...");

		const ImVec2 label_size = CalcTextSize(buf_display, NULL, true);

		ImRect clickable(rect.Min, rect.Max);
		bool hovered = ItemHoverable(clickable, id, ImGuiItemFlags_None);

		it_anim->second.background = ImLerp(it_anim->second.background, g.ActiveId == id ? c::main_color.Value : c::background_color.Value, GetAnimSpeed());
		it_anim->second.icon = ImLerp(it_anim->second.icon, g.ActiveId == id ? c::main_color.Value : hovered ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());
		it_anim->second.size_x = ImLerp(it_anim->second.size_x, CalcTextSize(buf_display).x, GetAnimSpeed());

		window->DrawList->AddRectFilled(clickable.Min, clickable.Max, GetColorU32(it_anim->second.background), c::elements::rounding);

		PushStyleColor(ImGuiCol_Text, c::text::text_active);
		RenderTextClipped(clickable.Min, clickable.Max, buf_display, NULL, &label_size, ImVec2(0.5f, 0.5f));
		PopStyleColor();

		if ((hovered || g.ActiveId == id) && io.MouseClicked[0])
		{
			if (g.ActiveId != id) {
				memset(io.MouseDown, 0, sizeof(io.MouseDown));
				memset(io.KeysDown, 0, sizeof(io.KeysDown));
				*key = 0;
			}
			ImGui::SetActiveID(id, window);
			ImGui::FocusWindow(window);
		}
		else if (io.MouseClicked[0]) {
			if (g.ActiveId == id)
				ImGui::ClearActiveID();
		}

		if (g.ActiveId == id) {
			for (auto i = 0; i < 5; i++) {
				if (io.MouseDown[i]) {
					switch (i) {
					case 0: k = 0x01; break;
					case 1: k = 0x02; break;
					case 2: k = 0x04; break;
					case 3: k = 0x05; break;
					case 4: k = 0x06; break;
					}
					value_changed = true;
					ImGui::ClearActiveID();
				}
			}
			if (!value_changed) {
				for (auto i = 0x08; i <= 0xA5; i++) {
					if (io.KeysDown[i]) {
						k = i;
						value_changed = true;
						ImGui::ClearActiveID();
					}
				}
			}

			if (IsKeyPressedMap(ImGuiKey_Escape)) {
				*key = 0;
				ImGui::ClearActiveID();
			}
			else {
				*key = k;
			}
		}

		return value_changed;
	}

	// ------------------------------------------------------------------ child
	bool ChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* parent_window = g.CurrentWindow;

		flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
		flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);

		const ImVec2 content_avail = GetContentRegionAvail();
		ImVec2 size = ImFloor(size_arg) + ImVec2(0, cap ? T(40) : 0);
		const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
		if (size.x <= 0.0f)
			size.x = ImMax(content_avail.x + size.x, 4.0f);
		if (size.y <= 0.0f)
			size.y = ImMax(content_avail.y + size.y, 4.0f);

		if (parent_window->DC.IsSameLine)
			ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, T(40)));

		SetNextWindowPos(ImVec2(parent_window->DC.CursorPos + ImVec2(0, cap ? T(40) : 0)));
		SetNextWindowSize(size - ImVec2(0, cap ? T(40) : 0));

		const char* label_end = FindRenderedTextEnd(name);

		GetWindowDrawList()->AddRectFilled(parent_window->DC.CursorPos, parent_window->DC.CursorPos + size, GetColorU32(c::child::background), c::child::rounding, ImDrawFlags_RoundCornersAll);
		GetWindowDrawList()->AddRect(parent_window->DC.CursorPos, parent_window->DC.CursorPos + size, GetColorU32(c::child::stroke), c::child::rounding, ImDrawFlags_RoundCornersAll);

		const int vtx_idx_0 = GetWindowDrawList()->VtxBuffer.Size;
		GetWindowDrawList()->AddText(parent_window->DC.CursorPos + ImVec2(GetStyle().WindowPadding.x, T(25) - CalcTextSize(name).y / 2), utils::GetColorWithAlpha(c::text::label::defaults, ImGui::GetStyle().Alpha), name, label_end);
		const int vtx_idx_1 = GetWindowDrawList()->VtxBuffer.Size;
		ShadeVertsLinearColorGradientKeepAlpha(GetWindowDrawList(), vtx_idx_0, vtx_idx_1, parent_window->DC.CursorPos + ImVec2(GetStyle().WindowPadding.x, T(25) - CalcTextSize(name).y / 2), parent_window->DC.CursorPos + ImVec2(GetStyle().WindowPadding.x, T(25) - CalcTextSize(name).y / 2) + CalcTextSize(name), c::main_color, c::main_color);

		const char* temp_window_name;

		if (name) ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);

		else ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

		bool ret = Begin(temp_window_name, NULL, flags | ImGuiWindowFlags_NoBackground);

		ImGuiWindow* child_window = g.CurrentWindow;
		child_window->ChildId = id;
		child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

		if (child_window->BeginCount == 1) parent_window->DC.CursorPos = child_window->Pos;

		const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
		if (g.ActiveId == temp_id_for_activation) ClearActiveID();

		if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
		{
			FocusWindow(child_window);
			NavInitWindow(child_window, false);
			SetActiveID(temp_id_for_activation, child_window);
			g.ActiveIdSource = g.NavInputSource;
		}
		return ret;
	}

	bool Child(const char* str_id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags extra_flags)
	{
		ImGuiWindow* window = GetCurrentWindow();

		if (cap) {
			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(T(18), T(18)));
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(T(18), T(18)));
		}
		return ChildEx(str_id, window->GetID(str_id), size_arg, cap, extra_flags | ImGuiWindowFlags_AlwaysUseWindowPadding);
	}

	bool ChildID(ImGuiID id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags extra_flags)
	{
		IM_ASSERT(id != 0);
		return ChildEx(NULL, id, size_arg, cap, extra_flags);
	}

	void EndChild()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		PopStyleVar(2);

		IM_ASSERT(g.WithinEndChild == false);
		IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);

		g.WithinEndChild = true;
		if (window->BeginCount > 1)
		{
			ImGui::End();
		}
		else
		{
			ImVec2 sz = window->Size;

			if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) sz.x = ImMax(4.0f, sz.x);
			if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y)) sz.y = ImMax(4.0f, sz.y);

			ImGui::End();

			ImGuiWindow* parent_window = g.CurrentWindow;
			ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
			ImGui::ItemSize(sz);
			if ((window->DC.NavLayersActiveMask != 0 || window->DC.NavWindowHasScrollY) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
			{
				ImGui::ItemAdd(bb, window->ChildId);
			}
			else
			{
				ImGui::ItemAdd(bb, 0);

				if (window->Flags & ImGuiWindowFlags_NavFlattened) parent_window->DC.NavLayersActiveMaskNext |= window->DC.NavLayersActiveMaskNext;
			}
			if (g.HoveredWindow == window) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
		}
		g.WithinEndChild = false;
		g.LogLinePosY = -FLT_MAX;
	}

	void BeginGroup()
	{
		ImGui::BeginGroup();
	}

	void EndGroup()
	{
		ImGui::EndGroup();
	}
	// ------------------------------------------------------------------ separators
	void Separator_line()
	{
		GetWindowDrawList()->AddRectFilled(GetCursorScreenPos(), GetCursorScreenPos() + ImVec2(GetContentRegionMax().x - GetStyle().WindowPadding.x, T(1)), GetColorU32(c::separator));
		Spacing();
	}

	void SeparatorEx(ImGuiSeparatorFlags flags, float thickness)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return;

		ImGuiContext& g = *GImGui;
		IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));
		IM_ASSERT(thickness > 0.0f);

		if (flags & ImGuiSeparatorFlags_Vertical)
		{
			float y1 = window->DC.CursorPos.y;
			float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
			const ImRect bb(ImVec2(window->DC.CursorPos.x, y1 + (GetStyle().ItemSpacing.y / 2)), ImVec2(window->DC.CursorPos.x + thickness, y2 - (GetStyle().ItemSpacing.y / 2)));

			ItemSize(ImVec2(thickness, 0.0f));
			if (!ItemAdd(bb, 0)) return;

			window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(c::child::background));

			ImGui::SameLine();
		}
		else if (flags & ImGuiSeparatorFlags_Horizontal)
		{
			float x1 = window->Pos.x;
			float x2 = window->Pos.x + window->Size.x;

			if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID) x1 += window->DC.Indent.x;

			if (ImGuiTable* table = g.CurrentTable)
			{
				x1 = table->Columns[table->CurrentColumn].MinX;
				x2 = table->Columns[table->CurrentColumn].MaxX;
			}

			ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
			if (columns) PushColumnsBackground();

			const float thickness_for_layout = (thickness == 1.0f) ? 0.0f : thickness;
			const ImRect bb(ImVec2(x1 + GetStyle().WindowPadding.x, window->DC.CursorPos.y), ImVec2(x2 - GetStyle().WindowPadding.x, window->DC.CursorPos.y + thickness));

			ItemSize(ImVec2(0.0f, thickness_for_layout));

			if (ItemAdd(bb, 0))
			{
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(c::separator));
			}
			if (columns)
			{
				PopColumnsBackground();
				columns->LineMinY = window->DC.CursorPos.y;
			}
		}
	}

	void Separator()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems) return;

		ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
		flags |= ImGuiSeparatorFlags_SpanAllColumns;
		SeparatorEx(flags, 1.0f);
	}

	// ------------------------------------------------------------------ tabs
	struct tab_state {
		ImVec4 text_col, frame_col;
		float text_offset;
	};

	bool Tab(const char* label, int* v, int number)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		// Touch: generous press row (lanx uses a 20px row on desktop).
		const ImRect total_bb(pos, pos + ImVec2(T(20) + CalcTextSize(label).x, ImMax(CalcTextSize(label).y, T(30))));

		ItemSize(total_bb, style.FramePadding.y);
		ItemAdd(total_bb, id);

		static std::map<ImGuiID, tab_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, tab_state() });
			it_anim = anim.find(id);
		}

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			page_is_changing = true;
			wanted_idx = number;
		}
		RenderNavHighlight(total_bb, id);

		it_anim->second.frame_col = ImLerp(it_anim->second.frame_col, *v == number ? c::main_color : utils::GetColorWithAlpha(c::main_color, 0.f), GetAnimSpeed());
		it_anim->second.text_offset = ImLerp(it_anim->second.text_offset, *v == number ? T(15) : 0.f, GetAnimSpeed());
		it_anim->second.text_col = ImLerp(it_anim->second.text_col, *v == number ? c::text::label::active : hovered ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());

		window->DrawList->AddCircleFilled(ImVec2(total_bb.Min.x + T(6.f), total_bb.GetCenter().y), it_anim->second.text_offset / 5, GetColorU32(it_anim->second.frame_col));
		window->DrawList->AddText(total_bb.Min + ImVec2(it_anim->second.text_offset, ImMax(0.f, (total_bb.GetSize().y - CalcTextSize(label).y) * 0.5f)), GetColorU32(it_anim->second.text_col), label);

		return pressed;
	}

	struct subtab_state {
		ImVec4 text_col;
		ImVec4 frame_col;
	};

	bool SubTab(const char* label, int* v, int number, ImColor color)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const ImRect total_bb(pos, pos + ImVec2(T(20) + CalcTextSize(label).x, T(25)));

		ItemSize(total_bb, style.FramePadding.y);
		ItemAdd(total_bb, id);

		static std::map<ImGuiID, subtab_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, subtab_state() });
			it_anim = anim.find(id);
		}

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = number;
		}

		RenderNavHighlight(total_bb, id);

		it_anim->second.frame_col = ImLerp(it_anim->second.frame_col, *v == number ? color : utils::GetColorWithAlpha(color, 0.f), GetAnimSpeed());
		it_anim->second.text_col = ImLerp(it_anim->second.text_col, *v == number ? ImVec4(color.Value) : c::text::label::active, GetAnimSpeed());

		window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32(it_anim->second.frame_col), style.FrameRounding);
		window->DrawList->AddRect(total_bb.Min, total_bb.Max, color, style.FrameRounding);

		window->DrawList->AddText(utils::center_text(total_bb.Min, total_bb.Max, label), GetColorU32(c::text::label::active), label);

		return pressed;
	}

	// ------------------------------------------------------------------ button
	struct button_state
	{
		ImVec4 background, text;
	};

	bool Button(const char* label, const ImVec2& size_arg)
	{
		ImGuiWindow* window = GetCurrentWindow();

		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true), pos = window->DC.CursorPos;

		ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);
		// Touch: minimum press height even for zero-size buttons.
		size.y = ImMax(size.y, T(30));

		const ImRect bb(pos, pos + size);

		ItemSize(size, 0.f);
		if (!ItemAdd(bb, id)) return false;

		bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

		static std::map<ImGuiID, button_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, button_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.background = ImLerp(it_anim->second.background, IsItemActive() || hovered || held ? c::main_color : c::background_color, GetAnimSpeed());
		it_anim->second.text = ImLerp(it_anim->second.text, IsItemActive() || hovered || held ? c::text::label::active : c::text::label::defaults, GetAnimSpeed());

		GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding);

		PushClipRect(bb.Min, bb.Max, true);

		GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size.x - CalcTextSize(label).x) / 2, bb.Max.y - CalcTextSize(label).y - (size.y - CalcTextSize(label).y) / 2), GetColorU32(it_anim->second.text), label);

		PopClipRect();

		return pressed;
	}

	// ------------------------------------------------------------------ checkbox
	struct check_state
	{
		float checkbox_alpha = 0.f;
		ImVec4 text_color = ImVec4(0, 0, 0, 0);
		ImVec4 rect_color = ImVec4(0, 0, 0, 0);
		float check_alpha = 0.f;
		float rotation = 0.f;
	};

	bool Checkbox(const char* label, bool* v)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return false;

		std::string label_str = label;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const ImVec2 pos = window->DC.CursorPos;
		const float w = GetContentRegionAvail().x;
		// Touch: row height 19 -> T(19), with a 34px minimum like the previous rows.
		const float row_h = ImMax(T(19), 34.0f * c::widget_scale);
		ImRect total_bb(pos, pos + ImVec2(w, row_h));
		ItemSize(total_bb, 0.f);
		if (!ItemAdd(total_bb, id)) return false;

		bool hovered, held, pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed) *v = !*v;

		static std::map<ImGuiID, check_state> anim;
		auto it_anim = anim.emplace(id, check_state()).first;

		it_anim->second.checkbox_alpha = ImLerp(it_anim->second.checkbox_alpha, *v ? 1.f : 0.f, GetAnimSpeed());
		it_anim->second.rect_color = ImLerp(it_anim->second.rect_color, *v ? c::main_color : c::background_color, GetAnimSpeed());
		// No hover-only visuals on touch: pressed/active drives the highlight.
		it_anim->second.text_color = ImLerp(it_anim->second.text_color, (*v ? c::text::label::active : c::text::label::defaults), GetAnimSpeed());
		it_anim->second.check_alpha = ImLerp(it_anim->second.check_alpha, *v ? 1.f : 0.f, GetAnimSpeed());
		it_anim->second.rotation = ImLerp(it_anim->second.rotation, *v ? IM_PI / 2 : -IM_PI, GetAnimSpeed());

		GetWindowDrawList()->AddText(ImVec2(total_bb.Min.x, utils::center_text(total_bb.Min, total_bb.Max, label).y),
			GetColorU32(it_anim->second.text_color), label);

		// Keep the tap square square in touch space.
		const float box = ImMin(total_bb.GetSize().y, T(22));
		ImRect check_bb(ImVec2(total_bb.Max.x - box, total_bb.Max.y - box), total_bb.Max);

		GetWindowDrawList()->AddRectFilled(check_bb.Min, check_bb.Max,
			c::background_color, style.FrameRounding);

		ImRotateStart();
		int vtx_idx_0 = GetWindowDrawList()->VtxBuffer.Size;
		GetWindowDrawList()->AddRectFilled(check_bb.Min, check_bb.Max,
			GetColorU32(ImVec4(1.f, 1.f, 1.f, it_anim->second.check_alpha)), style.FrameRounding);
		int vtx_idx_1 = GetWindowDrawList()->VtxBuffer.Size;
		ShadeVertsLinearColorGradientKeepAlpha(GetWindowDrawList(), vtx_idx_0, vtx_idx_1,
			check_bb.Min, check_bb.Max, c::second_color, c::main_color);

		RenderCheckMark(GetWindowDrawList(), check_bb.GetCenter() - ImVec2(box / 2.f, box / 2.f),
			GetColorU32(ImVec4(1.f, 1.f, 1.f, it_anim->second.checkbox_alpha)), box * 0.5f);
		ImRotateEnd(it_anim->second.rotation, check_bb.GetCenter());

		return pressed;
	}

	struct checkboxclicked_state
	{
		ImVec4 text_color_offset;
		ImVec4 rect_color;
		ImVec4 circle_color;
		float circle_offset;
	};

	bool CheckboxClicked(const char* label, bool* v)
	{
		ImGuiWindow* window = GetCurrentWindow();
		std::string name = label;

		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		static std::map<ImGuiID, checkboxclicked_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, checkboxclicked_state() });
			it_anim = anim.find(id);
		}

		const float square_sz = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		ImRect total_bb(pos, pos + ImVec2(T(75) + CalcTextSize(label).x, T(50)));

		ItemSize(total_bb, style.FramePadding.y);
		ItemAdd(total_bb, id);

		bool hovered, held;
		bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed) *v = !(*v);

		it_anim->second.circle_offset = ImLerp(it_anim->second.circle_offset, *v ? T(20.f) : 0.f, GetAnimSpeed());
		it_anim->second.rect_color = ImLerp(it_anim->second.rect_color, *v ? c::main_color.Value : ImColor(0.1f, 0.1f, 0.1f, 0.5f).Value, GetAnimSpeed());
		it_anim->second.circle_color = ImLerp(it_anim->second.circle_color, *v ? ImColor(1.f, 1.f, 1.f, 1.f) : ImColor(0.6f, 0.6f, 0.6f, 1.f), GetAnimSpeed());
		it_anim->second.text_color_offset = ImLerp(it_anim->second.text_color_offset, *v ? ImColor(0.f, 0.f, 0.f, 0.0f) : ImColor(0.f, 0.f, 0.f, 0.5f), GetAnimSpeed());

		window->DrawList->AddRectFilled(total_bb.Max - ImVec2(T(60), T(35)), total_bb.Max - ImVec2(T(20), T(15)), GetColorU32(it_anim->second.rect_color), 25);
		window->DrawList->AddRect(total_bb.Max - ImVec2(T(60), T(35)), total_bb.Max - ImVec2(T(20), T(15)), c::second_color, 25, 0, T(1.5f));
		window->DrawList->AddCircleFilled(total_bb.Max - ImVec2(T(50) - it_anim->second.circle_offset, T(25)), T(7.f), GetColorU32(it_anim->second.circle_color), 60);

		window->DrawList->AddText(ImVec2(total_bb.Min.x + T(5.f), utils::center_text(total_bb.Min, total_bb.Max, label).y), GetColorU32(c::text::label::active), label);
		return pressed;
	}
	// ------------------------------------------------------------------ combo
	static float CalcMaxPopupHeightFromItemCount(int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

	struct begin_state
	{
		ImVec4 background, text;
		float combo_size = 0.f;
		bool opened_combo = false, hovered = false;
		float arrow_roll;
	};

	bool BeginCombo(const char* label, const char* preview_value, int val, bool multi, ImGuiComboFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();

		g.NextWindowData.ClearFlags();
		if (window->SkipItems) return false;

		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);

		const ImVec2 pos = window->DC.CursorPos;

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const float w = ((GetContentRegionAvail().x));

		// Touch: row 25 -> larger, scaled row height.
		const float row_h = ImMax(T(25), 40.0f * c::widget_scale);
		const ImRect total_bb(pos, pos + ImVec2(w, row_h));

		const ImRect bb_box(total_bb.Max - ImVec2(total_bb.GetWidth() / 2, total_bb.GetHeight()), total_bb.Max);

		float box_offset = bb_box.Max.x - ImGui::GetCurrentWindow()->Size.x / 2 - T(10);

		const ImRect bb(ImVec2(total_bb.Min.x + CalcTextSize(label).x + T(10) < box_offset ? box_offset : total_bb.Min.x + CalcTextSize(label).x + T(10), bb_box.Min.y), bb_box.Max);

		ItemSize(total_bb, 0.f);

		if (!ItemAdd(total_bb, id, &bb)) return false;
		bool hovered, held, pressed = ButtonBehavior(total_bb, id, &hovered, &held);

		static std::map<ImGuiID, begin_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, begin_state() });
			it_anim = anim.find(id);
		}

		// Touch: first tap opens, tap outside closes (lanx behavior with the same
		// io.MouseClicked feed used for touch).
		if ((hovered && g.IO.MouseClicked[0]) || (it_anim->second.opened_combo && g.IO.MouseClicked[0] && !it_anim->second.hovered)) it_anim->second.opened_combo = !it_anim->second.opened_combo;

		it_anim->second.arrow_roll = ImLerp(it_anim->second.arrow_roll, it_anim->second.opened_combo ? -1.f : 1.f, GetAnimSpeed());
		it_anim->second.text = ImLerp(it_anim->second.text, it_anim->second.opened_combo ? c::text::label::active : (hovered || held) ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());
		it_anim->second.background = ImLerp(it_anim->second.background, it_anim->second.opened_combo ? c::second_color.Value : c::elements::background, GetAnimSpeed());
		it_anim->second.combo_size = ImLerp(it_anim->second.combo_size, it_anim->second.opened_combo ? (val * T(32)) + T(22) : 0.f, GetAnimSpeed() * 2.f);

		GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, utils::GetColorWithAlpha(c::background_color, c::background_color.Value.w * style.Alpha), c::elements::rounding);

		GetWindowDrawList()->AddText(ImVec2(total_bb.Min.x, utils::center_text(total_bb.Min, total_bb.Max, label).y), GetColorU32(it_anim->second.text), label);

		PushClipRect(bb.Min, bb.Max, true);
		GetWindowDrawList()->AddText(ImVec2(bb.Min.x + T(7.f), utils::center_text(total_bb.Min, total_bb.Max, preview_value).y), utils::GetColorWithAlpha(c::text::label::active, style.Alpha), preview_value);
		PopClipRect();

		ImRotateStart();
		GetWindowDrawList()->AddText(ImVec2(bb.Max.x - T(24), bb_box.GetCenter().y - CalcTextSize(ICON_DOWN_SMALL_LINE).y / 2), GetColorU32(c::text::label::defaults), ICON_DOWN_SMALL_LINE);
		ImRotateEnd(1.57f * it_anim->second.arrow_roll);

		if (!IsRectVisible(bb.Min, bb.Max + ImVec2(0, 2)))
		{
			it_anim->second.opened_combo = false;
			it_anim->second.combo_size = 0.f;
		}

		if (!it_anim->second.opened_combo && it_anim->second.combo_size < 2.f) return false;

		ImGui::SetNextWindowPos(ImVec2(bb.Min.x, bb.Max.y + T(5)));
		ImGui::SetNextWindowSize(ImVec2(bb.GetWidth(), it_anim->second.combo_size));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollWithMouse;

		PushStyleColor(ImGuiCol_WindowBg, utils::ImColorToImVec4(utils::GetColorWithAlpha(c::window_bg_color, 0.9f)));
		PushStyleVar(ImGuiStyleVar_WindowRounding, c::elements::rounding);
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(T(15), T(15)));
		PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

		bool ret = Begin(label, NULL, window_flags);

		PopStyleVar(3);
		PopStyleColor(1);

		ImGui::GetForegroundDrawList()->AddRect(GetWindowPos(), GetWindowPos() + GetWindowSize(), GetColorU32(c::child::stroke), 1.f);

		it_anim->second.hovered = IsWindowHovered();

		if (multi && it_anim->second.hovered && g.IO.MouseClicked[0]) it_anim->second.opened_combo = false;

		return true;
	}

	void EndCombo()
	{
		End();
	}

	void MultiCombo(const char* label, bool variable[], const char* labels[], int count)
	{
		ImGuiContext& g = *GImGui;

		std::string preview = "None";

		for (auto i = 0, j = 0; i < count; i++)
		{
			if (variable[i])
			{
				if (j)
					preview += (", ") + (std::string)labels[i];
				else
					preview = labels[i];

				j++;
			}
		}

		if (BeginCombo(label, preview.c_str(), count))
		{
			for (auto i = 0; i < count; i++)
			{
				PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(T(15), T(15)));
				PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(T(15), T(15)));
				custom::Selectable(labels[i], &variable[i], ImGuiSelectableFlags_DontClosePopups);
				PopStyleVar(2);
			}
			End();
		}

		preview = ("None");
	}

	bool BeginComboPreview()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

		if (window->SkipItems || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)) return false;

		IM_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y);

		if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) return false;

		preview_data->BackupCursorPos = window->DC.CursorPos;
		preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
		preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
		preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
		preview_data->BackupLayout = window->DC.LayoutType;
		window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
		window->DC.CursorMaxPos = window->DC.CursorPos;
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.IsSameLine = false;
		PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

		return true;
	}

	void EndComboPreview()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

		ImDrawList* draw_list = window->DrawList;
		if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
			if (draw_list->CmdBuffer.Size > 1)
			{
				draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
				draw_list->_TryMergeDrawCmds();
			}
		PopClipRect();
		window->DC.CursorPos = preview_data->BackupCursorPos;
		window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
		window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
		window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
		window->DC.LayoutType = preview_data->BackupLayout;
		window->DC.IsSameLine = false;
		preview_data->PreviewRect = ImRect();
	}

	static const char* Items_ArrayGetter(void* data, int idx)
	{
		const char* const* items = (const char* const*)data;
		return items[idx];
	}

	static const char* Items_SingleStringGetter(void* data, int idx)
	{
		const char* items_separated_by_zeros = (const char*)data;
		int items_count = 0;
		const char* p = items_separated_by_zeros;
		while (*p)
		{
			if (idx == items_count)
				break;
			p += strlen(p) + 1;
			items_count++;
		}
		return *p ? p : NULL;
	}

	bool Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
	{
		ImGuiContext& g = *GImGui;

		const char* preview_value = NULL;
		if (*current_item >= 0 && *current_item < items_count)
			preview_value = getter(user_data, *current_item);

		if (!BeginCombo(label, preview_value, items_count, ImGuiComboFlags_None)) return false;

		// Touch: comfortable rows in the popup.
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(T(15), T(7)));
		bool value_changed = false;
		for (int i = 0; i < items_count; i++)
		{
			const char* item_text = getter(user_data, i);
			if (item_text == NULL)
				item_text = "*Unknown item*";

			PushID(i);
			const bool item_selected = (i == *current_item);
			if (custom::Selectable(item_text, item_selected) && *current_item != i)
			{
				value_changed = true;
				*current_item = i;
			}
			if (item_selected)
				SetItemDefaultFocus();
			PopID();
		}
		PopStyleVar();

		EndCombo();

		if (value_changed)
			MarkItemEdited(g.LastItemData.ID);

		return value_changed;
	}

	bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
	{
		const bool value_changed = custom::Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
		return value_changed;
	}

	bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
	{
		int items_count = 0;
		const char* p = items_separated_by_zeros;
		while (*p)
		{
			p += strlen(p) + 1;
			items_count++;
		}
		bool value_changed = custom::Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
		return value_changed;
	}

	// ------------------------------------------------------------------ selectable
	struct select_state
	{
		ImVec4 text, background, stroke;
		float circle_radius, text_offset;
	};

	bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		ImGuiID id = window->GetID(label);
		ImVec2 label_size = CalcTextSize(label, NULL, true);
		ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		// Touch: comfortable press height (lanx rows are ~20px on desktop).
		size.y = ImMax(size.y, T(28));
		ImVec2 pos = window->DC.CursorPos;
		pos.y += window->DC.CurrLineTextBaseOffset;
		ItemSize(size, 0.0f);

		const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
		const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
		const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
		if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth)) size.x = ImMax(label_size.x, max_x - min_x);

		const ImVec2 text_min = pos;
		const ImVec2 text_max(min_x + size.x, pos.y + size.y);

		ImRect bb(min_x, pos.y, text_max.x, text_max.y);
		if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
		{
			const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
			const float spacing_y = style.ItemSpacing.y;
			const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
			const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
			bb.Min.x -= spacing_L;
			bb.Min.y -= spacing_U;
			bb.Max.x += (spacing_x - spacing_L);
			bb.Max.y += (spacing_y - spacing_U);
		}

		const float backup_clip_rect_min_x = window->ClipRect.Min.x;
		const float backup_clip_rect_max_x = window->ClipRect.Max.x;
		if (span_all_columns)
		{
			window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
			window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
		}

		const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
		const bool item_add = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
		if (span_all_columns)
		{
			window->ClipRect.Min.x = backup_clip_rect_min_x;
			window->ClipRect.Max.x = backup_clip_rect_max_x;
		}

		if (!item_add) return false;

		const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
		if (disabled_item && !disabled_global) BeginDisabled();

		if (span_all_columns && window->DC.CurrentColumns) PushColumnsBackground();
		else if (span_all_columns && g.CurrentTable) TablePushBackgroundChannel();

		// PressedOnClick: rows select on touch-down like lanx desktop (and never
		// sit in a hover-limbo with a finger).
		ImGuiButtonFlags button_flags = ImGuiButtonFlags_PressedOnClick;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
		if (flags & ImGuiSelectableFlags_NoSetKeyOwner) { button_flags |= ImGuiButtonFlags_NoSetKeyOwner; }
		if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
		if ((flags & ImGuiSelectableFlags_AllowOverlap) || (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap)) { button_flags |= ImGuiButtonFlags_AllowOverlap; }

		const bool was_selected = selected;
		bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

		if (pressed) MarkItemEdited(id);

		if (selected != was_selected)  g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		if (span_all_columns && window->DC.CurrentColumns) PopColumnsBackground();
		else if (span_all_columns && g.CurrentTable) TablePopBackgroundChannel();

		static std::map<ImGuiID, select_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, select_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::text::label::active : c::text::label::defaults, GetAnimSpeed());
		it_anim->second.circle_radius = ImLerp(it_anim->second.circle_radius, selected ? T(3.f) : 0.f, GetAnimSpeed());
		it_anim->second.text_offset = ImLerp(it_anim->second.text_offset, selected ? T(20.f) : T(4.5f), GetAnimSpeed());
		it_anim->second.background = ImLerp(it_anim->second.background, selected ? c::background_color : utils::GetColorWithAlpha(c::background_color, 0.f), GetAnimSpeed());
		it_anim->second.stroke = ImLerp(it_anim->second.stroke, selected ? utils::GetColorWithAlpha(c::child::stroke, 1.f) : utils::GetColorWithAlpha(c::child::stroke, 0.f), GetAnimSpeed());

		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(it_anim->second.stroke), c::elements::rounding);
		window->DrawList->AddCircleFilled(ImVec2(bb.Min.x + T(9), bb.GetCenter().y), it_anim->second.circle_radius, c::main_color);

		window->DrawList->AddText(ImVec2(bb.Min.x + it_anim->second.text_offset, utils::center_text(bb.Min, bb.Max, label).y), GetColorU32(it_anim->second.text), label);

		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup)) CloseCurrentPopup();

		if (disabled_item && !disabled_global) EndDisabled();

		return pressed;
	}

	bool Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
	{
		if (Selectable(label, *p_selected, flags, size_arg))
		{
			*p_selected = !*p_selected;
			return true;
		}
		return false;
	}

	// ------------------------------------------------------------------ color helpers
	static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V)
	{
		ImGuiContext& g = *GImGui;
		IM_ASSERT(g.ColorEditCurrentID != 0);
		if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0))) return;

		if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
			*H = g.ColorEditSavedHue;

		if (*V == 0.0f) *S = g.ColorEditSavedSat;
	}

	bool ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, const ImVec2& size_arg)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(desc_id);
		const float default_size = GetFrameHeight();
		const ImVec2 pos = window->DC.CursorPos;
		const float width = GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x;
		const ImRect rect(pos, pos + ImVec2(width, T(19)));

		const ImRect clickable(rect.Min + ImVec2(width - T(25), 0), rect.Max - ImVec2(T(7), 0));

		ItemSize(ImRect(rect.Min, rect.Max - ImVec2(0, 0)));
		if (!ItemAdd(rect, id)) return false;

		bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);

		if (flags & ImGuiColorEditFlags_NoAlpha) flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

		ImVec4 col_rgb = col;
		if (flags & ImGuiColorEditFlags_InputHSV) ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

		ImDrawList* dl = GetWindowDrawList();
		dl->AddRectFilled(clickable.Min, clickable.Max, GetColorU32(col_rgb), c::elements::rounding);
		RenderColorRectWithAlphaCheckerboard(dl, clickable.Min, clickable.Max, GetColorU32(col_rgb), ImMin(36, 29) / 2.99f, ImVec2(0.f, 0.f), c::elements::rounding);

		return pressed;
	}

	struct edit_state
	{
		ImVec4 text;
		ImVec4 icon;
	};

	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const float square_sz = GetFrameHeight();
		const float w_full = CalcItemWidth();
		const char* label_display_end = FindRenderedTextEnd(label);
		g.NextItemData.ClearFlags();

		BeginGroup();
		PushID(label);
		const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
		if (set_current_color_edit_id)
			g.ColorEditCurrentID = window->IDStack.back();

		// Touch layout: label row with one swatch. No dense RGB input fields
		// (NoInputs implied when caller uses picker_flags from runtime code).
		const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
		float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };

		const ImVec2 pos = window->DC.CursorPos;
		const float width = GetContentRegionMax().x - style.WindowPadding.x;
		const float row_h = ImMax(T(19), ImMax(CalcTextSize(label, NULL, true).y, 34.0f * c::widget_scale));
		const ImRect row(pos, pos + ImVec2(width, row_h));

		const ImVec4 col_v4(f[0], f[1], f[2], alpha ? f[3] : 1.0f);
		ItemSize(ImRect(row.Min, row.Max));
		if (ItemAdd(row, window->GetID(label)))
		{
			const float swatch = T(22);
			const ImRect swatch_bb(row.Max - ImVec2(swatch, swatch), row.Max);

			bool hovered, held, pressed = ButtonBehavior(row, window->GetID(label), &hovered, &held);

			if (pressed)
			{
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(swatch_bb.GetBL() + ImVec2(0.0f, style.ItemSpacing.y));
			}

			ImDrawList* dl = GetWindowDrawList();
			static std::map<ImGuiID, edit_state> anim;
			auto it_anim = anim.find(GetID(label));
			if (it_anim == anim.end())
			{
				anim.insert({ GetID(label), edit_state() });
				it_anim = anim.find(GetID(label));
			}

			it_anim->second.text = ImLerp(it_anim->second.text,
				ImGui::IsPopupOpen("picker") ? c::text::label::active
				: (hovered || held) ? c::text::label::hovered : c::text::label::defaults, GetAnimSpeed());

			dl->AddText(ImVec2(row.Min.x, utils::center_text(row.Min, row.Max, label).y), GetColorU32(it_anim->second.text), label);

			dl->AddRectFilled(swatch_bb.Min, swatch_bb.Max, GetColorU32(col_v4), c::elements::rounding);
			RenderColorRectWithAlphaCheckerboard(dl, swatch_bb.Min, swatch_bb.Max, GetColorU32(col_v4), ImMin(36, 29) / 2.99f, ImVec2(0.f, 0.f), c::elements::rounding);
		}

		ImGui::SetNextWindowPos(ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowSize().x + T(15), 0));
		if (BeginPopup("picker", ImGuiWindowFlags_NoMove))
		{
			if (g.CurrentWindow->BeginCount == 1)
			{
				PushStyleColor(ImGuiCol_PopupBg, utils::ImColorToImVec4(utils::GetColorWithAlpha(c::window_bg_color, 0.95f)));
				PushStyleVar(ImGuiStyleVar_PopupRounding, c::child::rounding);

				ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV |
					ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha |
					ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoTooltip;
				SetNextItemWidth(square_sz * T(15.0f));
				ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);

				PopStyleColor(1);
				PopStyleVar(1);
			}
			EndPopup();
		}

		if (set_current_color_edit_id)
			g.ColorEditCurrentID = 0;
		PopID();
		EndGroup();

		// "Changed" = the committed swatcolor differs from the one this widget last
		// reported. The picker writes col[] when the popup is active.
		static std::map<ImGuiID, ImVec4> lastCommitted;
		const ImGuiID colId = window->GetID(label);
		const ImVec4 now(col[0], col[1], col[2], col[3]);
		auto itLast = lastCommitted.find(colId);
		bool changed = false;
		if (itLast == lastCommitted.end())
			lastCommitted[colId] = now;
		else if (itLast->second.x != now.x || itLast->second.y != now.y ||
			itLast->second.z != now.z || itLast->second.w != now.w)
		{
			itLast->second = now;
			changed = true;
			if (g.LastItemData.ID != 0)
				MarkItemEdited(g.LastItemData.ID);
		}

		return changed;
	}
	// ------------------------------------------------------------------ picker
	static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w, float alpha)
	{
		ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
		ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x + 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
		ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
		ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
		ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
	}

	struct picker_state
	{
		float hue_bar;
		float alpha_bar;
		float circle;
		ImVec2 circle_move;
	};

	bool ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImDrawList* draw_list = window->DrawList;
		ImGuiStyle& style = g.Style;
		ImGuiIO& io = g.IO;

		const float width = CalcItemWidth();
		g.NextItemData.ClearFlags();

		PushID(label);
		BeginGroup();

		if (!(flags & ImGuiColorEditFlags_NoSidePreview))
			flags |= ImGuiColorEditFlags_NoSmallPreview;

		int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
		bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
		ImVec2 picker_pos = window->DC.CursorPos;
		ImVec2 bar_pos = window->DC.CursorPos + ImVec2(0, T(133));
		float square_sz = GetFrameHeight();
		float bars_width = T(209.f);
		float sv_picker_size = ImMax(bars_width * 1, width - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)) + 0;
		float sv_bar_size = T(20);
		float bar0_pos_x = GetWindowPos().x + style.WindowPadding.x;
		float bar1_pos_x = bar0_pos_x;

		float backup_initial_col[4];
		memcpy(backup_initial_col, col, components * sizeof(float));

		float H = col[0], S = col[1], V = col[2];
		float R = col[0], G = col[1], B = col[2];
		if (flags & ImGuiColorEditFlags_InputRGB)
		{
			ColorConvertRGBtoHSV(R, G, B, H, S, V);
			ColorEditRestoreHS(col, &H, &S, &V);
		}
		else if (flags & ImGuiColorEditFlags_InputHSV)
		{
			ColorConvertHSVtoRGB(H, S, V, R, G, B);
		}

		bool value_changed = false, value_changed_h = false, value_changed_sv = false;

		PushItemFlag(ImGuiItemFlags_NoNav, true);

		// SV rectangle (touch-sized)
		InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size - T(80)));
		if (IsItemActive())
		{
			S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - T(80)));

			if (g.ColorEditSavedColor == ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
				H = g.ColorEditSavedHue;
			value_changed = value_changed_sv = true;
		}

		// Hue bar (touch)
		SetCursorScreenPos(ImVec2(bar0_pos_x, bar_pos.y));
		InvisibleButton("hue", ImVec2(bars_width, sv_bar_size));
		if (IsItemActive())
		{
			H = 1.f - ImSaturate((io.MousePos.x - bar_pos.x) / (bars_width - 1));
			value_changed = value_changed_h = true;
		}

		PopItemFlag();

		if (value_changed_h || value_changed_sv)
		{
			if (flags & ImGuiColorEditFlags_InputRGB)
			{
				ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
				if (!(flags & ImGuiColorEditFlags_NoAlpha)) col[3] = backup_initial_col[3];

				g.ColorEditSavedHue = H;
				g.ColorEditSavedSat = S;
				g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));
			}
			else if (flags & ImGuiColorEditFlags_InputHSV)
			{
				col[0] = H;
				col[1] = S;
				col[2] = V;
			}
		}

		ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha));
		const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
		const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
		const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
		const ImU32 col_hues[7] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(0,0,255,style_alpha8),IM_COL32(0,255,255,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

		ImVec2 sv_cursor_pos;

		// SV Square
		const int vtx_idx_0 = draw_list->VtxBuffer.Size;
		draw_list->AddRectFilled(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size - 2 - T(80)), col_white, 4.0f);
		const int vtx_idx_1 = draw_list->VtxBuffer.Size;
		ShadeVertsLinearColorGradientKeepAlpha(draw_list, vtx_idx_0, vtx_idx_1, picker_pos, picker_pos + ImVec2(sv_picker_size, 0.0f), col_white, ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)));

		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size - T(60)), 0, 0, col_black, col_black, 4.f);

		sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * sv_picker_size), picker_pos.x, picker_pos.x + sv_picker_size - 2);
		sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * (sv_picker_size - T(60))), picker_pos.y + 2, picker_pos.y + sv_picker_size - T(60));

		static std::map<ImGuiID, picker_state> anim;
		auto it_anim = anim.find(ImGui::GetID(label));

		if (it_anim == anim.end())
		{
			anim.insert({ ImGui::GetID(label), picker_state() });
			it_anim = anim.find(ImGui::GetID(label));
		}

		for (int i = 0; i < 6; ++i)
			GetForegroundDrawList()->AddRectFilledMultiColor(
				ImVec2(picker_pos.x + i * (bars_width / 6) - (i == 5 ? 1 : 0), picker_pos.y + T(139)),
				ImVec2(picker_pos.x + (i + 1) * (bars_width / 6) + (i == 0 ? 1 : 0), picker_pos.y + T(132) + sv_bar_size - T(7)),
				col_hues[i], col_hues[i + 1], col_hues[i + 1], col_hues[i], T(10.f),
				i == 0 ? ImDrawFlags_RoundCornersLeft : i == 5 ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersNone);

		float bar0_line_x = IM_ROUND(bar_pos.x + (1.f - H) * bars_width);
		bar0_line_x = ImClamp(bar0_line_x, picker_pos.x + T(3.f), picker_pos.x + T(204.f));

		it_anim->second.hue_bar = ImLerp(it_anim->second.hue_bar, bar0_line_x - bar_pos.x, g.IO.DeltaTime * 24.f);

		GetForegroundDrawList()->AddCircleFilled(ImVec2(it_anim->second.hue_bar + bar_pos.x, bar_pos.y + T(9)), T(6.5f), ImColor(255, 255, 255, 255), 30.f);

		it_anim->second.circle_move = ImLerp(it_anim->second.circle_move, sv_cursor_pos - bar_pos, g.IO.DeltaTime * 24.f);
		it_anim->second.circle = ImLerp(it_anim->second.circle, value_changed_sv ? T(4.f) : T(7.f), g.IO.DeltaTime * 24.f);

		GetForegroundDrawList()->AddCircle(it_anim->second.circle_move + bar_pos + ImVec2(0, 1), it_anim->second.circle, ImColor(255, 255, 255, 255), 32);

		EndGroup();

		if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0) value_changed = false;
		if (value_changed) MarkItemEdited(g.LastItemData.ID);

		PopID();
		return value_changed;
	}
	// ------------------------------------------------------------------ knob
	struct knob_state
	{
		float plus_float; int plus_int;
		ImVec4 circle, text;
		float slow_anim, circle_anim;
		float position;
	};

	bool KnobScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const float w = GetContentRegionMax().x - style.WindowPadding.x;

		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const ImRect frame_bb(window->DC.CursorPos + ImVec2(0, 0), window->DC.CursorPos + ImVec2(w, T(32)));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? label_size.x : 0.0f, 0.0f));

		ItemSize(ImRect(total_bb.Min, total_bb.Max - ImVec2(0, 0)));
		if (!ItemAdd(total_bb, id, &frame_bb)) return false;

		if (format == NULL) format = DataTypeGetInfo(data_type)->PrintFmt;

		bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags), held, pressed = ButtonBehavior(frame_bb, id, &hovered, &held, ImGuiButtonFlags_None);

		static std::map<ImGuiID, knob_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, knob_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.circle_anim = ImLerp(it_anim->second.circle_anim, IsItemActive() ? T(11.f) : T(10.f), GetAnimSpeed());

		// lanx tweaks the value via keyboard +/- (GetAsyncKeyState) while hovering;
		// that is a Windows-only path stripped for Android. The knob stays a
		// fully working touch draggable through DragBehavior below.
		it_anim->second.plus_float = ImLerp(it_anim->second.plus_float, 0.f, GetAnimSpeed());
		it_anim->second.plus_int = ImLerp(it_anim->second.plus_int, 0, GetAnimSpeed());

		it_anim->second.text = ImLerp(it_anim->second.text, g.ActiveId == id ? c::text::text_active : (hovered || held) ? c::text::text_hov : c::text::text, GetAnimSpeed());

		const bool value_changed = DragBehavior(id, data_type, p_data, 0.f, p_min, p_max, format, ImGuiSliderFlags_None);

		if (value_changed) MarkItemEdited(id);

		char value_buf[64];
		const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

		float radius = T(10.f);
		float thickness = T(3.f);
		it_anim->second.position = ImLerp(it_anim->second.position, *static_cast<float*>(p_data) / *reinterpret_cast<const float*>(p_max) * 6.25f, ImGui::GetIO().DeltaTime * 18.f);

		GetWindowDrawList()->PathClear();
		GetWindowDrawList()->PathArcTo(ImVec2(frame_bb.Max.x + radius - T(22.f), frame_bb.Min.y + (T(32) / 2)), radius, 0.f, 2.f * IM_PI, 40.f);
		GetWindowDrawList()->PathStroke(GetColorU32(c::elements::background), 0, thickness);

		GetWindowDrawList()->PathClear();
		GetWindowDrawList()->PathArcTo(ImVec2(frame_bb.Max.x + radius - T(22.f), frame_bb.Min.y + (T(32) / 2)), radius, IM_PI * 1.5f, IM_PI * 1.5f + it_anim->second.position, 40.f);
		GetWindowDrawList()->PathStroke(c::main_color, 0, thickness);

		GetWindowDrawList()->AddCircleFilled(ImVec2(frame_bb.Max.x + radius - T(22.f) + ImCos(IM_PI * 1.5f + it_anim->second.position) * radius, frame_bb.Min.y + (T(32) / 2) + ImSin(IM_PI * 1.5f + it_anim->second.position) * radius), T(2.f), GetColorU32(c::text::text_active));

		GetWindowDrawList()->AddText(ImVec2(frame_bb.Max.x - (T(40) + CalcTextSize(value_buf).x), frame_bb.Min.y + (T(32) - CalcTextSize(value_buf).y) / 2), GetColorU32(c::text::text), value_buf);

		GetWindowDrawList()->AddText(ImVec2(frame_bb.Max.x - w, frame_bb.Min.y + (T(32) - CalcTextSize(value_buf).y) / 2), GetColorU32(it_anim->second.text), label);

		return value_changed;
	}
	// ------------------------------------------------------------------ slider
	struct slider_state
	{
		float position = 0.f;
		float slow = 0.f;
		float circle_radius = 6.f;
		ImVec4 text_color;
	};

	bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		ImGuiID id = window->GetID(label);
		float w = GetContentRegionAvail().x;
		ImVec2 pos = window->DC.CursorPos;
		// Touch: total block 34px -> scaled block, with an improved hitbox around the track.
		ImRect total_bb(pos, pos + ImVec2(w, ImMax(T(34), 48.0f * c::widget_scale)));
		ImRect slider_bb(total_bb.Min + ImVec2(0, 6 + CalcTextSize(label).y), total_bb.Max);
		ItemSize(total_bb);
		if (!ItemAdd(total_bb, id)) return false;
		if (!format) format = DataTypeGetInfo(data_type)->PrintFmt;
		bool hovered, held;
		ButtonBehavior(total_bb, id, &hovered, &held);
		static std::map<ImGuiID, slider_state> anim;
		auto& st = anim[id];
		if (st.position == 0.f)
		{
			if (data_type == ImGuiDataType_S32) st.position = float(*(int*)p_data);
			else st.position = *(float*)p_data;
		}
		ImRect grab_bb;
		bool value_changed = SliderBehavior(ImRect(slider_bb.Min, ImVec2(slider_bb.Max.x + T(8), slider_bb.Max.y)), id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
		float speed = GetAnimSpeed();

		st.circle_radius = ImLerp(st.circle_radius, held ? T(7.f) : hovered ? T(6.5f) : T(6.f), speed);
		st.slow = ImLerp(st.slow, grab_bb.Min.x - slider_bb.Min.x, g.IO.DeltaTime * 12.f);
		st.text_color = ImLerp(st.text_color, held ? c::text::label::active : (hovered || held) ? c::text::label::hovered : c::text::label::defaults, speed);

		float real_val = (data_type == ImGuiDataType_S32) ? float(*(int*)p_data) : *(float*)p_data;
		st.position = ImLerp(st.position, real_val, g.IO.DeltaTime * 12.f);
		char value_buf[64];

		if (data_type == ImGuiDataType_S32)
			snprintf(value_buf, sizeof(value_buf), format, int(st.position + 0.5f));
		else
			snprintf(value_buf, sizeof(value_buf), format, (double)st.position);
		auto dl = GetWindowDrawList();
		dl->AddRectFilled(slider_bb.Min, slider_bb.Max, utils::GetColorWithAlpha(c::background_color, c::background_color.Value.w * style.Alpha), c::elements::rounding);

		const int vtx_idx_0 = dl->VtxBuffer.Size;
		dl->AddRectFilled(slider_bb.Min, ImVec2(slider_bb.Min.x + st.slow + T(6.f), slider_bb.Max.y), utils::GetColorWithAlpha(c::main_color, style.Alpha), c::elements::rounding);
		const int vtx_idx_1 = dl->VtxBuffer.Size;
		ShadeVertsLinearColorGradientKeepAlpha(dl, vtx_idx_0, vtx_idx_1, slider_bb.Min, ImVec2(slider_bb.Min.x + st.slow + T(6.f), slider_bb.Max.y), c::second_color, c::main_color);

		dl->AddShadowCircle(ImVec2(slider_bb.Min.x + st.slow + T(6.f) - slider_bb.GetSize().y * 0.5f, slider_bb.GetCenter().y), st.circle_radius, ImColor(0.f, 0.f, 0.f, 0.85f), T(36), ImVec2(0, 0), (ImDrawFlags)0, T(36));
		dl->AddCircleFilled(ImVec2(slider_bb.Min.x + st.slow + T(6.f) - slider_bb.GetSize().y * 0.5f, slider_bb.GetCenter().y), st.circle_radius, utils::GetColorWithAlpha(c::text::label::active, style.Alpha), T(36));

		dl->AddText(total_bb.Min, GetColorU32(st.text_color), label);
		dl->AddText(ImVec2(total_bb.Max.x - CalcTextSize(value_buf).x, total_bb.Min.y), GetColorU32(st.text_color), value_buf);

		if (value_changed) MarkItemEdited(id);
		return value_changed;
	}

	bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
	}

	bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
	}


	// ------------------------------------------------------------------ page icon tile (lanx style)
	struct page_state
	{
		ImVec4 background, text;
		float smooth_swap, alpha_line, line_size;
	};

	bool Page(bool selected, const char* icon, const ImVec2& size_arg, bool allow_icon_font, bool show_active_line)
	{
		ImGuiWindow* window = GetCurrentWindow();

		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(icon);
		const ImVec2 label_size = CalcTextSize(icon, NULL, true), pos = window->DC.CursorPos;

		ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

		const ImRect bb(pos, pos + size);

		ItemSize(size, 0.f);
		if (!ItemAdd(bb, id)) return false;

		bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

		static std::map<ImGuiID, page_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, page_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.background = ImLerp(it_anim->second.background, selected || hovered ? c::page::background_active : c::page::background, g.IO.DeltaTime * 6.f);
		it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::main_color.Value : hovered ? c::page::text_hov : c::page::text, g.IO.DeltaTime * 14.f);

		it_anim->second.alpha_line = ImLerp(it_anim->second.alpha_line, selected ? 1.f : 0.f, g.IO.DeltaTime * 6.f);
		it_anim->second.line_size = ImLerp(it_anim->second.line_size, selected ? (size.x / 4) : (size.x / 2), g.IO.DeltaTime * 6.f);

		it_anim->second.smooth_swap = ImLerp(it_anim->second.smooth_swap, selected ? 26.f * c::scale : 0, g.IO.DeltaTime * 12.f);

		GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);

		if (show_active_line)
			GetWindowDrawList()->AddRectFilled(bb.Min + ImVec2(+it_anim->second.line_size, (size_arg.y + 9 * c::scale)), bb.Max + ImVec2(-it_anim->second.line_size, 15 * c::scale), GetColorU32(c::main_color.Value, it_anim->second.alpha_line), c::page::rounding * c::scale, ImDrawFlags_RoundCornersTop);

		PushClipRect(bb.Min, bb.Max, true);

		const bool use_icon_font = allow_icon_font && (icon && icon[0] != '\0' && icon[1] == '\0' && font::icomoon_page != nullptr);
		if (!use_icon_font)
		{
			GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size.x - CalcTextSize(icon).x) * 0.5f, bb.Min.y), GetColorU32(it_anim->second.text), icon);
			PopClipRect();
			return pressed;
		}

		PushFont(font::icomoon_page);
		if (show_active_line)
		{
			GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size.x - CalcTextSize(icon).x) * 0.5f, bb.Min.y + (size.y - CalcTextSize(icon).y) * 0.5f + it_anim->second.smooth_swap), GetColorU32(it_anim->second.text), icon);
		}
		GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size.x - CalcTextSize(icon).x) * 0.5f, bb.Min.y + (size.y - CalcTextSize(icon).y) * 0.5f), GetColorU32(it_anim->second.text), icon);
		PopFont();

		PopClipRect();

		return pressed;
	}

	// ------------------------------------------------------------------ theme tile (lanx style)
	struct theme_state
	{
		ImVec4 background;
		float smooth_swap;
	};

	bool ThemeButton(const char* id_theme, bool dark, const ImVec2& size_arg)
	{
		ImGuiWindow* window = GetCurrentWindow();

		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(id_theme);
		const ImVec2 label_size = CalcTextSize(id_theme, NULL, true), pos = window->DC.CursorPos;

		ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

		const ImRect bb(pos, pos + size);

		ItemSize(size, 0.f);
		if (!ItemAdd(bb, id)) return false;

		bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

		static std::map<ImGuiID, theme_state> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end())
		{
			anim.insert({ id, theme_state() });
			it_anim = anim.find(id);
		}

		it_anim->second.background = ImLerp(it_anim->second.background, dark || hovered ? c::page::background_active : c::page::background, GetAnimSpeed());
		it_anim->second.smooth_swap = ImLerp(it_anim->second.smooth_swap, dark ? 26.f : 0, GetAnimSpeed() * 2.f);

		GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);

		if (font::icomoon_page)
		{
			PushClipRect(bb.Min, bb.Max, true);
			PushFont(font::icomoon_page);
			GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size_arg.x - CalcTextSize("k").x) / 2, bb.Max.y - CalcTextSize("k").y - (size.y - CalcTextSize("k").y) / 2 + it_anim->second.smooth_swap * c::scale), GetColorU32(c::main_color.Value), "k");
			GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size_arg.x - CalcTextSize("a").x) / 2, bb.Max.y - CalcTextSize("a").y - (size.y - CalcTextSize("a").y) / 2 - 25 + it_anim->second.smooth_swap * c::scale), GetColorU32(c::main_color.Value), "a");
			PopFont();
			PopClipRect();
		}

		return pressed;
	}

	bool KnobFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return KnobScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
	}

	bool KnobInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		return KnobScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
	}

}
