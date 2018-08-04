// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "imgui_themes.h"
#include "ui_config.h"
#include "wrap_imgui.h"

namespace ImGui
{
	static ImGuiStyle s_defaultStyle;

	void Style_Init()
	{
		s_defaultStyle = GetStyle();
	}

	void Style_Apply()
	{
		UIConfig_ApplyColorscheme();
	}

	void Style_Reset()
	{
		ImGuiStyle &s = ImGui::GetStyle();
		s = s_defaultStyle;
		s.WindowPadding.x *= g_config.dpiScale;
		s.WindowPadding.y *= g_config.dpiScale;
		s.WindowMinSize.x *= g_config.dpiScale;
		s.WindowMinSize.y *= g_config.dpiScale;
		s.ChildRounding *= g_config.dpiScale;
		s.PopupRounding *= g_config.dpiScale;
		s.FramePadding.x *= g_config.dpiScale;
		s.FramePadding.y *= g_config.dpiScale;
		s.FrameRounding *= g_config.dpiScale;
		s.ItemSpacing.x *= g_config.dpiScale;
		s.ItemSpacing.y *= g_config.dpiScale;
		s.ItemInnerSpacing.x *= g_config.dpiScale;
		s.ItemInnerSpacing.y *= g_config.dpiScale;
		s.TouchExtraPadding.x *= g_config.dpiScale;
		s.TouchExtraPadding.y *= g_config.dpiScale;
		s.IndentSpacing *= g_config.dpiScale;
		s.ColumnsMinSpacing *= g_config.dpiScale;
		s.ScrollbarSize *= g_config.dpiScale;
		s.ScrollbarRounding *= g_config.dpiScale;
		s.GrabMinSize *= g_config.dpiScale;
		s.GrabRounding *= g_config.dpiScale;
		s.DisplayWindowPadding.x *= g_config.dpiScale;
		s.DisplayWindowPadding.y *= g_config.dpiScale;
		s.DisplaySafeAreaPadding.x *= g_config.dpiScale;
		s.DisplaySafeAreaPadding.y *= g_config.dpiScale;
	}

	void StyleColorsVSDark(ImGuiStyle *dst)
	{
		StyleColorsClassic(dst);
		ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
		ImVec4 *colors = style->Colors;
		colors[ImGuiCol_TitleBgActive] = ImColor(63, 63, 70, 255); // VS Dark Active Tab
		colors[ImGuiCol_TitleBg] = ImColor(45, 45, 48, 255);       // VS Dark Inactive Tab
		colors[ImGuiCol_WindowBg] = ImColor(42, 42, 44, 255);      // VS Dark Output Window
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		colors[ImGuiCol_Button] = ImColor(76, 76, 76);
		colors[ImGuiCol_ButtonHovered] = ImColor(128, 128, 128);
		colors[ImGuiCol_ButtonActive] = ImColor(112, 112, 112);
	}

	// taken from https://github.com/ocornut/imgui/issues/707
	void StyleColorsWindows(ImGuiStyle *dst)
	{
		StyleColorsClassic(dst);
		ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
		ImVec4 *colors = style->Colors;

		float hspacing = 8 * g_config.dpiScale;
		float vspacing = 6 * g_config.dpiScale;
		style->DisplaySafeAreaPadding = ImVec2(0, 0);
		style->WindowPadding = ImVec2(hspacing / 2, vspacing);
		style->FramePadding = ImVec2(hspacing, vspacing);
		style->ItemSpacing = ImVec2(hspacing, vspacing);
		style->ItemInnerSpacing = ImVec2(hspacing, vspacing);
		style->IndentSpacing = 20.0f * g_config.dpiScale;

		style->WindowRounding = 0.0f;
		style->FrameRounding = 0.0f;

		style->WindowBorderSize = 0.0f;
		style->FrameBorderSize = 1.0f * g_config.dpiScale;
		style->PopupBorderSize = 1.0f * g_config.dpiScale;

		style->ScrollbarSize = 20.0f * g_config.dpiScale;
		style->ScrollbarRounding = 0.0f;
		style->GrabMinSize = 5.0f * g_config.dpiScale;
		style->GrabRounding = 0.0f;

		ImVec4 white = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		ImVec4 transparent = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		ImVec4 dark = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
		ImVec4 darker = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

		ImVec4 background = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
		ImVec4 text = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		ImVec4 border = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		ImVec4 grab = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
		ImVec4 header = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		ImVec4 active = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
		ImVec4 hover = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);

		colors[ImGuiCol_Text] = text;
		colors[ImGuiCol_WindowBg] = background;
		colors[ImGuiCol_ChildBg] = background;
		colors[ImGuiCol_PopupBg] = white;

		colors[ImGuiCol_Border] = border;
		colors[ImGuiCol_BorderShadow] = transparent;

		colors[ImGuiCol_Button] = header;
		colors[ImGuiCol_ButtonHovered] = hover;
		colors[ImGuiCol_ButtonActive] = active;

		colors[ImGuiCol_FrameBg] = white;
		colors[ImGuiCol_FrameBgHovered] = hover;
		colors[ImGuiCol_FrameBgActive] = active;

		colors[ImGuiCol_MenuBarBg] = header;
		colors[ImGuiCol_Header] = header;
		colors[ImGuiCol_HeaderHovered] = hover;
		colors[ImGuiCol_HeaderActive] = active;

		colors[ImGuiCol_CheckMark] = text;
		colors[ImGuiCol_SliderGrab] = grab;
		colors[ImGuiCol_SliderGrabActive] = darker;

		colors[ImGuiCol_ScrollbarBg] = header;
		colors[ImGuiCol_ScrollbarGrab] = grab;
		colors[ImGuiCol_ScrollbarGrabHovered] = dark;
		colors[ImGuiCol_ScrollbarGrabActive] = darker;
	}

} // namespace ImGui
