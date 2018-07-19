// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "wrap_imgui.h"

namespace ImGui
{
	const ImGuiTreeNodeFlags DefaultOpenTreeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
	                                                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
	                                                    ImGuiTreeNodeFlags_DefaultOpen;

	const ImGuiTreeNodeFlags DefaultClosedTreeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
	                                                      ImGuiTreeNodeFlags_OpenOnDoubleClick;

	bool Checkbox(const char *label, b8 *v);
	bool Checkbox(const char *label, b32 *v);
	bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags, b32 *v, void *ptr_id = nullptr);
	bool MenuItem(const char *label, const char *shortcut, b32 *p_selected, bool enabled = true);
	bool Begin(const char *name, b32 *p_open, ImGuiWindowFlags flags = 0);
	bool IsKeyPressed(ImGuiKey_ key, bool repeat = true);

	enum verticalScrollDir_e {
		kVerticalScroll_None,
		kVerticalScroll_PageUp,
		kVerticalScroll_PageDown,
		kVerticalScroll_Up,
		kVerticalScroll_Down,
		kVerticalScroll_Start,
		kVerticalScroll_End,
	};
	verticalScrollDir_e GetVerticalScrollDir();

} // namespace ImGui
