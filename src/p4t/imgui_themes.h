// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

struct ImGuiStyle;

namespace ImGui
{
	void Style_Init();
	void Style_Apply();
	void Style_Reset();
	void StyleColorsVSDark(ImGuiStyle *dst = nullptr);
	void StyleColorsWindows(ImGuiStyle *dst = nullptr);
} // namespace ImGui
