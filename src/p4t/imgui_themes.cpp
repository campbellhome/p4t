// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "imgui_themes.h"
#include "wrap_imgui.h"

namespace ImGui
{

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

} // namespace ImGui
