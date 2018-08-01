// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "imgui_utils.h"
#include "config.h"
#include "sb.h"
#include "va.h"
#include <math.h>

namespace ImGui
{
	int s_tabCount;

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

	bool InputText(const char *label, sb_t *sb, u32 buf_size, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *user_data)
	{
		if(sb->allocated < buf_size) {
			sb_reserve(sb, buf_size);
		}
		bool ret = InputText(label, (char *)sb->data, sb->allocated, flags, callback, user_data);
		sb->count = sb->data ? (u32)strlen(sb->data) + 1 : 0;
		return ret;
	}

	bool InputTextMultiline(const char *label, sb_t *sb, u32 buf_size, const ImVec2 &size, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *user_data)
	{
		if(sb->allocated < buf_size) {
			sb_reserve(sb, buf_size);
		}
		bool ret = InputTextMultiline(label, (char *)sb->data, sb->allocated, size, flags, callback, user_data);
		sb->count = sb->data ? (u32)strlen(sb->data) + 1 : 0;
		return ret;
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

	void PushButtonColors(buttonType_e colors)
	{
		const auto &styleColors = GetStyle().Colors;
		const ImGuiCol_ colorNormal = ImGuiCol_Button;
		const ImGuiCol_ colorHoverd = ImGuiCol_ButtonHovered;
		const ImGuiCol_ colorActive = ImGuiCol_ButtonActive;
		switch(colors) {
		case kButton_Normal:
			PushStyleColor(colorNormal, ImColor(76, 76, 76));
			PushStyleColor(colorHoverd, ImColor(128, 128, 128));
			PushStyleColor(colorActive, ImColor(112, 112, 112));
			break;
		case kButton_Disabled:
			PushStyleColor(colorNormal, ImColor(64, 64, 64));
			PushStyleColor(colorHoverd, ImColor(64, 64, 64));
			PushStyleColor(colorActive, ImColor(64, 64, 64));
			break;
		case kButton_TabActive:
			PushStyleColor(colorNormal, styleColors[ImGuiCol_Header]);
			PushStyleColor(colorHoverd, styleColors[ImGuiCol_HeaderHovered]);
			PushStyleColor(colorActive, styleColors[ImGuiCol_HeaderActive]);
			break;
		case kButton_TabInactive:
			PushStyleColor(colorNormal, ImColor(0, 0, 0, 0));
			PushStyleColor(colorHoverd, styleColors[ImGuiCol_HeaderHovered]);
			PushStyleColor(colorActive, styleColors[ImGuiCol_HeaderActive]);
			break;
		}
	}
	void PopButtonColors(void)
	{
		PopStyleColor(3);
	}
	bool Button(const char *label, buttonType_e colors, const ImVec2 &size)
	{
		PushButtonColors(colors);
		bool ret = Button(label, size);
		PopButtonColors();
		return ret;
	}

	void BeginTabButtons(void)
	{
		s_tabCount = 0;
	}
	bool TabButton(const char *label, sb_t *active, const char *name, const ImVec2 &size)
	{
		if(s_tabCount++) {
			SameLine();
		}
		bool isActive = !strcmp(sb_get(active), name);
		bool ret = Button(label, isActive ? kButton_TabActive : kButton_TabInactive, size);
		if(ret && !isActive) {
			sb_reset(active);
			sb_append(active, name);
		}
		return ret;
	}
	void EndTabButtons(void)
	{
	}

	bool BeginTabChild(sb_t *active, const char *str_id, const ImVec2 &size, bool border, ImGuiWindowFlags extra_flags)
	{
		if(strcmp(sb_get(active), str_id)) {
			return false;
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);
		return BeginChild(str_id, size, border, extra_flags);
	}

	void EndTabChild()
	{
		EndChild();
	}

	void DrawColumnText(columnDrawData h, u32 columnIndex, const char *text, const char *end)
	{
		float windowX = ImGui::GetWindowPos().x;
		float offset = h.columnOffsets[columnIndex];
		float width = h.columnWidths[columnIndex];
		float x1 = floorf(0.5f + windowX + offset - 1.0f);
		float x2 = floorf(0.5f + windowX + offset + width - 1.0f);
		ImGui::SameLine(offset);
		ImGui::PushClipRect(ImVec2(x1, -FLT_MAX), ImVec2(x2, +FLT_MAX), true);
		ImGui::TextUnformatted(text, end);
		ImGui::PopClipRect();
	}

	void DrawColumnHeaderText(float offset, float width, const char *text, const char *end)
	{
		float windowX = ImGui::GetWindowPos().x;
		float x1 = floorf(0.5f + windowX + offset - 1.0f);
		float x2 = floorf(0.5f + windowX + offset + width - 1.0f);
		ImGui::SameLine(offset);
		ImGui::PushClipRect(ImVec2(x1, -FLT_MAX), ImVec2(x2, +FLT_MAX), true);
		ImGui::TextUnformatted(text, end);
		ImGui::PopClipRect();
	}

	columnDrawResult DrawColumnHeader(columnDrawData h, u32 columnIndex)
	{
		columnDrawResult res = {};
		const char *text = h.columnNames[columnIndex];
		float *width = h.columnWidths + columnIndex;
		bool last = columnIndex + 1 == h.numColumns;
		if(last) {
			*width = ImGui::GetContentRegionAvailWidth();
		} else if(*width <= 0.0f) {
			*width = h.columnScales[columnIndex] * ImGui::CalcTextSize(text).x + ImGui::CalcTextSize(" ^").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
		}
		float scale = (g_config.dpiScale <= 0.0f) ? 1.0f : g_config.dpiScale;
		float startOffset = ImGui::GetCursorPosX();

		{
			const float normal = 0.3f;
			const float hovered = 0.6f;
			const float active = 0.6f;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(normal, normal, normal, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hovered, hovered, hovered, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(active, active, active, 1.0f));
			if(ImGui::Button(va("###%s", text), ImVec2(*width * scale, 0.0f))) {
				res.sortChanged = true;
				if(*h.sortColumn == columnIndex) {
					*h.sortDescending = !*h.sortDescending;
				} else {
					*h.sortColumn = columnIndex;
					*h.sortDescending = false;
				}
			}
			ImGui::PopStyleColor(3);
		}

		res.active = res.active || ImGui::IsItemActive();
		float endOffset = startOffset + *width * scale + ImGui::GetStyle().ItemSpacing.x;
		const char *columnText = (*h.sortColumn == columnIndex) ? va("%s %s", *h.sortDescending ? "^" : "v", text) : text;
		const float itemPad = ImGui::GetStyle().ItemSpacing.x;
		DrawColumnHeaderText(startOffset + ImGui::GetStyle().ItemInnerSpacing.x, *width * scale - itemPad, columnText);
		ImGui::SameLine(endOffset);
		if(!last) {
			const float normal = 0.4f;
			const float hovered = 0.8f;
			const float active = 0.8f;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(normal, normal, normal, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hovered, hovered, hovered, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(active, active, active, 1.0f));
			ImGui::Button(va("|###sep%s", text), ImVec2(2.0f * g_config.dpiScale, 0.0f));
			ImGui::PopStyleColor(3);
			if(ImGui::IsItemActive()) {
				res.active = true;
				*width += ImGui::GetIO().MouseDelta.x / scale;
			}
			ImGui::SameLine();
		}

		h.columnOffsets[columnIndex + 1] = ImGui::GetCursorPosX();
		return res;
	}

	void PushSelectableColors(b32 selected, b32 viewActive)
	{
		if(selected && !viewActive) {
			ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Header];
			col.x *= 0.5f;
			col.y *= 0.5f;
			col.z *= 0.5f;
			ImGui::PushStyleColor(ImGuiCol_Header, col);
		}
	}

	void PopSelectableColors(b32 selected, b32 viewActive)
	{
		if(selected && !viewActive) {
			ImGui::PopStyleColor();
		}
	}

} // namespace ImGui
