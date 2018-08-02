// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "wrap_imgui.h"

typedef struct sb_s sb_t;

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
	bool InputText(const char *label, sb_t *buf, u32 buf_size, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void *user_data = NULL);
	bool InputTextMultiline(const char *label, sb_t *buf, u32 buf_size, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void *user_data = NULL);

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

	void SelectableTextUnformattedMultiline(const char *label, const char *text, ImVec2 size = ImVec2(0.0f, 0.0f));
	void SelectableTextUnformatted(const char *label, const char *text);
	void SelectableText(const char *label, const char *fmt, ...);

	enum buttonType_e {
		kButton_Normal,
		kButton_Disabled,
		kButton_TabActive,
		kButton_TabInactive,
	};
	void PushButtonColors(buttonType_e colors);
	void PopButtonColors(void);
	bool Button(const char *label, buttonType_e colors, const ImVec2 &size = ImVec2(0.0f, 0.0f));

	void BeginTabButtons(void);
	bool TabButton(const char *label, sb_t *active, const char *name, const ImVec2 &size = ImVec2(0.0f, 0.0f));
	bool TabButton(const char *label, u32 *active, u32 id, const ImVec2 &size = ImVec2(0.0f, 0.0f));
	void EndTabButtons(void);

	bool BeginTabChild(sb_t *active, const char *str_id, const ImVec2 &size = ImVec2(0, 0), bool border = true, ImGuiWindowFlags extra_flags = 0);
	bool BeginTabChild(u32 *active, u32 id, const char *str_id, const ImVec2 &size = ImVec2(0, 0), bool border = true, ImGuiWindowFlags extra_flags = 0);
	void EndTabChild();

	struct columnDrawResult {
		b32 sortChanged;
		b32 active;
	};

	struct columnDrawData {
		float *columnWidths;
		float *columnScales;
		float *columnOffsets;
		const char **columnNames;
		b32 *sortDescending;
		u32 *sortColumn;
		u32 numColumns;
		u8 pad[4];
	};
	void DrawColumnText(columnDrawData h, u32 columnIndex, const char *text, const char *end = nullptr);
	void DrawColumnHeaderText(float offset, float width, const char *text, const char *end = nullptr);
	columnDrawResult DrawColumnHeader(columnDrawData h, u32 columnIndex);

	void PushSelectableColors(b32 selected, b32 viewActive);
	void PopSelectableColors(b32 selected, b32 viewActive);

} // namespace ImGui
