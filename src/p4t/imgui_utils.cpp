// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "imgui_utils.h"
#include "app.h"
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
		bool ret = Checkbox(label, &b);
		if(ret) {
			*v = b;
		}
		return ret;
	}

	bool Checkbox(const char *label, b32 *v)
	{
		bool b = *v != 0;
		bool ret = Checkbox(label, &b);
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
			open = TreeNodeEx(ptr_id, flags | (b ? ImGuiTreeNodeFlags_Selected : 0), "%s", label);
		} else {
			open = TreeNodeEx(label, flags | (b ? ImGuiTreeNodeFlags_Selected : 0));
		}
		if(IsItemClicked()) {
			*v = !*v;
		}
		return open;
	}

	bool MenuItem(const char *label, const char *shortcut, b32 *p_selected, bool enabled)
	{
		if(!p_selected) {
			return MenuItem(label, shortcut, (bool *)nullptr, enabled);
		}
		bool b = *p_selected != 0;
		bool ret = MenuItem(label, shortcut, &b, enabled);
		*p_selected = b;
		return ret;
	}

	bool Begin(const char *name, b32 *p_open, ImGuiWindowFlags flags)
	{
		if(!p_open) {
			return Begin(name, (bool *)nullptr, flags);
		}
		bool b = *p_open != 0;
		bool ret = Begin(name, &b, flags);
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
		if(IsItemActive() && App_HasFocus()) {
			App_RequestRender();
		}
		return ret;
	}

	bool InputTextMultiline(const char *label, sb_t *sb, u32 buf_size, const ImVec2 &size, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *user_data)
	{
		if(sb->allocated < buf_size) {
			sb_reserve(sb, buf_size);
		}
		bool ret = InputTextMultiline(label, (char *)sb->data, sb->allocated, size, flags, callback, user_data);
		sb->count = sb->data ? (u32)strlen(sb->data) + 1 : 0;
		if(IsItemActive() && App_HasFocus()) {
			App_RequestRender();
		}
		return ret;
	}

	verticalScrollDir_e GetVerticalScrollDir()
	{
		if(IsKeyPressed(ImGuiKey_PageUp)) {
			return kVerticalScroll_PageUp;
		} else if(IsKeyPressed(ImGuiKey_PageDown)) {
			return kVerticalScroll_PageDown;
		} else if(IsKeyPressed(ImGuiKey_UpArrow)) {
			return kVerticalScroll_Up;
		} else if(IsKeyPressed(ImGuiKey_DownArrow)) {
			return kVerticalScroll_Down;
		} else if(IsKeyPressed(ImGuiKey_Home)) {
			return kVerticalScroll_Start;
		} else if(IsKeyPressed(ImGuiKey_End)) {
			return kVerticalScroll_End;
		} else {
			return kVerticalScroll_None;
		}
	}

	void SelectableTextUnformattedMultiline(const char *label, const char *text, ImVec2 size)
	{
		InputTextMultiline(label, const_cast< char * >(text), strlen(text) + 1, size, ImGuiInputTextFlags_ReadOnly);
		if(IsItemActive() && App_HasFocus()) {
			App_RequestRender();
		}
	}

	void SelectableTextUnformatted(const char *label, const char *text)
	{
		InputText(label, const_cast< char * >(text), strlen(text) + 1, ImGuiInputTextFlags_ReadOnly);
		if(IsItemActive() && App_HasFocus()) {
			App_RequestRender();
		}
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
			PushStyleColor(colorNormal, styleColors[colorNormal]);
			PushStyleColor(colorHoverd, styleColors[colorHoverd]);
			PushStyleColor(colorActive, styleColors[colorActive]);
			break;
		case kButton_Disabled:
			PushStyleColor(colorNormal, (ImVec4)ImColor(64, 64, 64));
			PushStyleColor(colorHoverd, (ImVec4)ImColor(64, 64, 64));
			PushStyleColor(colorActive, (ImVec4)ImColor(64, 64, 64));
			break;
		case kButton_TabActive:
			PushStyleColor(colorNormal, styleColors[ImGuiCol_Header]);
			PushStyleColor(colorHoverd, styleColors[ImGuiCol_HeaderHovered]);
			PushStyleColor(colorActive, styleColors[ImGuiCol_HeaderActive]);
			break;
		case kButton_TabInactive:
			PushStyleColor(colorNormal, (ImVec4)ImColor(255, 255, 255, 12));
			PushStyleColor(colorHoverd, styleColors[ImGuiCol_HeaderHovered]);
			PushStyleColor(colorActive, styleColors[ImGuiCol_HeaderActive]);
			break;
		case kButton_ColumnHeader:
			PushStyleColor(colorNormal, styleColors[colorNormal]);
			PushStyleColor(colorHoverd, styleColors[colorHoverd]);
			PushStyleColor(colorActive, styleColors[colorActive]);
			break;
		case kButton_ResizeBar:
			PushStyleColor(colorNormal, styleColors[colorNormal]);
			PushStyleColor(colorHoverd, styleColors[colorHoverd]);
			PushStyleColor(colorActive, styleColors[colorActive]);
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
	bool TabButton(const char *label, u32 *active, u32 id, const ImVec2 &size)
	{
		if(s_tabCount++) {
			SameLine();
		}
		bool isActive = *active == id;
		bool ret = Button(label, isActive ? kButton_TabActive : kButton_TabInactive, size);
		if(ret && !isActive) {
			*active = id;
		}
		return ret;
	}
	bool TabButtonIconColored(const char *icon, ImColor iconColor, const char *label, u32 *active, u32 id, const ImVec2 &size)
	{
		if(s_tabCount++) {
			SameLine();
		}
		bool isActive = *active == id;
		ImVec2 iconPos = GetIconPosForButton();
		bool ret = Button(label, isActive ? kButton_TabActive : kButton_TabInactive, size);
		if(ret && !isActive) {
			*active = id;
		}
		DrawIconAtPos(iconPos, icon, iconColor, true);
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
		SetCursorPosY(GetCursorPosY() - 4 * g_config.dpiScale);
		return BeginChild(str_id, size, border, extra_flags);
	}

	bool BeginTabChild(u32 *active, u32 id, const char *str_id, const ImVec2 &size, bool border, ImGuiWindowFlags extra_flags)
	{
		if(*active != id) {
			return false;
		}
		SetCursorPosY(GetCursorPosY() - 4 * g_config.dpiScale);
		return BeginChild(str_id, size, border, extra_flags);
	}

	void EndTabChild()
	{
		EndChild();
	}

	void PushColumnHeaderClipRect(float offset, float width)
	{
		float windowX = GetWindowPos().x;
		float x1 = floorf(0.5f + windowX + offset - 1.0f);
		float x2 = floorf(0.5f + windowX + offset + width - 1.0f);
		PushClipRect(ImVec2(x1, -FLT_MAX), ImVec2(x2, +FLT_MAX), true);
	}

	void DrawColumnText(columnDrawData h, u32 columnIndex, const char *text, const char *end)
	{
		float offset = h.columnOffsets[columnIndex];
		float width = h.columnWidths[columnIndex] * g_config.dpiScale;
		PushColumnHeaderClipRect(offset, width);
		if(columnIndex) {
			SameLine(offset);
		}
		TextUnformatted(text, end);
		PopClipRect();
	}

	void DrawColumnHeaderText(float offset, float width, const char *text, const char *end)
	{
		PushColumnHeaderClipRect(offset, width);
		SameLine(offset);
		TextUnformatted(text, end);
		PopClipRect();
	}

	columnDrawResult DrawColumnHeader(columnDrawData h, u32 columnIndex)
	{
#define ICON_SORT_UP ICON_FK_CARET_UP
#define ICON_SORT_DOWN ICON_FK_CARET_DOWN

		columnDrawResult res = {};
		const char *text = h.columnNames[columnIndex];
		float *width = h.columnWidths + columnIndex;
		bool last = columnIndex + 1 == h.numColumns;
		if(last) {
			*width = GetContentRegionAvailWidth();
		} else if(*width <= 0.0f) {
			*width = CalcTextSize(text).x + CalcTextSize(" " ICON_SORT_UP).x + GetStyle().ItemSpacing.x * 2.0f;
		}
		float scale = (g_config.dpiScale <= 0.0f) ? 1.0f : g_config.dpiScale;
		float startOffset = GetCursorPosX();

		if(Button(va("###%s", text), kButton_ColumnHeader, ImVec2(*width * scale, 0.0f))) {
			res.sortChanged = true;
			if(*h.sortColumn == columnIndex) {
				*h.sortDescending = !*h.sortDescending;
			} else {
				*h.sortColumn = columnIndex;
				*h.sortDescending = false;
			}
		}

		res.active = res.active || IsItemActive();
		float endOffset = startOffset + *width * scale + GetStyle().ItemSpacing.x;
		const char *columnText = (*h.sortColumn == columnIndex) ? va("%s %s", *h.sortDescending ? ICON_SORT_DOWN : ICON_SORT_UP, text) : text;
		const float itemPad = GetStyle().ItemSpacing.x;
		DrawColumnHeaderText(startOffset + GetStyle().ItemInnerSpacing.x, *width * scale - itemPad, columnText);
		SameLine(endOffset);
		if(!last) {
			Button(va("|###sep%s", text), kButton_ResizeBar, ImVec2(2.0f * g_config.dpiScale, 0.0f));
			if(IsItemActive()) {
				res.active = true;
				*width += GetIO().MouseDelta.x / scale;
			}
			SameLine();
		}

		h.columnOffsets[columnIndex + 1] = GetCursorPosX();
		return res;
	}

	void PushSelectableColors(b32 selected, b32 viewActive)
	{
		if(selected && !viewActive) {
			ImVec4 col = GetStyle().Colors[ImGuiCol_Header];
			col.x *= 0.5f;
			col.y *= 0.5f;
			col.z *= 0.5f;
			PushStyleColor(ImGuiCol_Header, col);
		}
	}

	void PopSelectableColors(b32 selected, b32 viewActive)
	{
		if(selected && !viewActive) {
			PopStyleColor();
		}
	}

	bool BeginContextMenu(const char *name)
	{
		if(IsMouseClicked(1) && IsItemHoveredRect()) {
			OpenPopup(name);
		}
		return BeginPopup(name);
	}

	void EndContextMenu()
	{
		EndPopup();
	}

	void IconOverlayColored(ImColor iconColor, const char *icon, ImColor overlayColor, const char *overlay)
	{
		ImVec2 pos = GetIconPosForText();
		IconColored(iconColor, icon);
		float end = pos.x + CalcTextSize(icon).x;
		pos.x = pos.x + (end - pos.x) / 2;
		ImDrawList *drawList = GetWindowDrawList();
		ImVec2 size = CalcTextSize(overlay);
		float lineHeight = GetTextLineHeightWithSpacing();
		float deltaY = lineHeight - size.y + 2 * g_config.dpiScale;
		pos.y += deltaY;
		drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), ImColor(0, 0, 0), overlay);
		drawList->AddText(ImVec2(pos.x + 0, pos.y + 0), overlayColor, overlay);
	}

	void IconOverlay(const char *icon, const char *overlay, ImColor overlayColor)
	{
		ImColor textColor = GetStyle().Colors[ImGuiCol_Text];
		IconOverlayColored(textColor, icon, overlayColor, overlay);
	}

	void IconColored(ImColor color, const char *icon)
	{
		ImVec2 pos = GetIconPosForText();
		TextColored(ImColor(0, 0, 0, 0), "%s", icon);
		ImDrawList *drawList = GetWindowDrawList();
		ImVec2 size = CalcTextSize(icon);
		float lineHeight = GetTextLineHeightWithSpacing();
		float deltaY = lineHeight - size.y;
		pos.y += deltaY;
		drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), ImColor(0, 0, 0), icon);
		drawList->AddText(ImVec2(pos.x + 0, pos.y + 0), color, icon);
	}

	void Icon(const char *icon)
	{
		ImColor textColor = GetStyle().Colors[ImGuiCol_Text];
		IconColored(textColor, icon);
	}

	void DrawIconAtPos(ImVec2 pos, const char *icon, ImColor color, bool align, float scale)
	{
		ImFont *font = GetFont();
		float fontSize = GetFontSize();
		fontSize *= scale;

		if(align) {
			ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, icon);
			float lineHeight = GetTextLineHeightWithSpacing();
			float deltaY = lineHeight - size.y;
			pos.y += deltaY;
		}

		ImDrawList *drawList = GetWindowDrawList();
		if(color.Value.x > 0.0f || color.Value.y > 0.0f || color.Value.z > 0.0f) {
			drawList->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y + 1), ImColor(0, 0, 0), icon);
		}
		drawList->AddText(font, fontSize, ImVec2(pos.x + 0, pos.y + 0), color, icon);
	}

	ImVec2 GetIconPosForButton()
	{
		ImVec2 windowPos = GetWindowPos();
		ImVec2 cursorPos = GetCursorPos();
		ImVec2 framePadding = GetStyle().FramePadding;
		ImVec2 textPos(windowPos.x + cursorPos.x + framePadding.x + GetScrollX(), windowPos.y + cursorPos.y - framePadding.y * 0.5f + 1 - GetScrollY());
		return textPos;
	}

	ImVec2 GetIconPosForText()
	{
		ImVec2 windowPos = GetWindowPos();
		ImVec2 cursorPos = GetCursorPos();
		float lineHeight = GetTextLineHeightWithSpacing();
		float fontHeight = CalcTextSize(nullptr).y;
		ImVec2 textPos(windowPos.x + cursorPos.x + GetScrollX(), windowPos.y + cursorPos.y - lineHeight + fontHeight - GetScrollY());
		return textPos;
	}

	static const void *s_activeSelectables;
	void SetActiveSelectables(const void *data)
	{
		s_activeSelectables = data;
	}

	bool IsActiveSelectables(const void *data)
	{
		return s_activeSelectables == data;
	}

} // namespace ImGui
