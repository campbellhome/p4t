// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "imgui_utils.h"

#include "sb.h"

namespace ImGui
{

	bool Checkbox(const char *label, b8 *v)
	{
		bool b = *v != 0;
		bool ret = ImGui::Checkbox(label, &b);
		if(ret) {
			*v = b;
		}
		return ret;
	}

	bool Checkbox(const char *label, b32 *v)
	{
		bool b = *v != 0;
		bool ret = ImGui::Checkbox(label, &b);
		if(ret) {
			*v = b;
		}
		return ret;
	}

	bool TreeNodeEx(const char *label, ImGuiTreeNodeFlags flags, b32 *v, void *ptr_id)
	{
		bool b = *v != 0;
		bool open;
		if(ptr_id) {
			open = ImGui::TreeNodeEx(ptr_id, flags | (b ? ImGuiTreeNodeFlags_Selected : 0), "%s", label);
		} else {
			open = ImGui::TreeNodeEx(label, flags | (b ? ImGuiTreeNodeFlags_Selected : 0));
		}
		if(ImGui::IsItemClicked()) {
			*v = !*v;
		}
		return open;
	}

	bool MenuItem(const char *label, const char *shortcut, b32 *p_selected, bool enabled)
	{
		if(!p_selected) {
			return ImGui::MenuItem(label, shortcut, (bool *)nullptr, enabled);
		}
		bool b = *p_selected != 0;
		bool ret = ImGui::MenuItem(label, shortcut, &b, enabled);
		*p_selected = b;
		return ret;
	}

	bool Begin(const char *name, b32 *p_open, ImGuiWindowFlags flags)
	{
		if(!p_open) {
			return ImGui::Begin(name, (bool *)nullptr, flags);
		}
		bool b = *p_open != 0;
		bool ret = ImGui::Begin(name, &b, flags);
		*p_open = b;
		return ret;
	}

	bool IsKeyPressed(ImGuiKey_ key, bool repeat)
	{
		return IsKeyPressed(GetKeyIndex(key), repeat);
	}

	verticalScrollDir_e GetVerticalScrollDir()
	{
		if(ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
			return kVerticalScroll_PageUp;
		} else if(ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
			return kVerticalScroll_PageDown;
		} else if(ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
			return kVerticalScroll_Up;
		} else if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
			return kVerticalScroll_Down;
		} else if(ImGui::IsKeyPressed(ImGuiKey_Home)) {
			return kVerticalScroll_Start;
		} else if(ImGui::IsKeyPressed(ImGuiKey_End)) {
			return kVerticalScroll_End;
		} else {
			return kVerticalScroll_None;
		}
	}

	void SelectableTextUnformattedMultiline(const char *label, const char *text, ImVec2 size)
	{
		InputTextMultiline(label, const_cast< char * >(text), strlen(text) + 1, size, ImGuiInputTextFlags_ReadOnly);
	}

	void SelectableTextUnformatted(const char *label, const char *text)
	{
		InputText(label, const_cast< char * >(text), strlen(text) + 1, ImGuiInputTextFlags_ReadOnly);
	}

	void SelectableText(const char *label, const char *fmt, ...)
	{
		sb_t sb = { 0 };
		va_list args;
		va_start(args, fmt);
		sb_va_list(&sb, fmt, args);
		SelectableTextUnformatted(label, sb_get(&sb));
		va_end(args);
		sb_reset(&sb);
	}

} // namespace ImGui
