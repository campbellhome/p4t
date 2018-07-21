// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changelist.h"

#include "config.h"
#include "imgui_utils.h"
#include "output.h"
#include "p4.h"
#include "sdict.h"
#include "str.h"
#include "time_utils.h"
#include "va.h"

#include "bb_array.h"

#include <math.h>

// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
BB_WARNING_PUSH(4820)
#include <time.h>
BB_WARNING_POP

//////////////////////////////////////////////////////////////////////////

static const char *sdict_getPrefix(sdictEntry_t *e, const char *prefix)
{
	const char *key = sb_get(&e->key);
	if(strncmp(key, prefix, strlen(prefix))) {
		return "";
	}
	return sb_get(&e->value);
}

typedef struct tag_changelistField {
	const char *desc;
	const char *key;
	const char *tag;
} changelistField;

static changelistField s_changeFields[2][3] = {
	{
	    { "Changelist:", "change", "###change" },
	    { "Date:", "time", "###time" },
	    { "Access Type:", "changeType", "###changeType" },
	},
	{
	    { "Clientspec:", "client", "###client" },
	    { "User:", "user", "###user" },
	    { "Status:", "status", "###status" },
	},
};

static void UIChangelist_DrawInformationColumn(sdict_t *cl, float fullWidth, changelistField changeFields[3])
{
	float width = 0.0f;
	for(int i = 0; i < 3; ++i) {
		ImVec2 size = ImGui::CalcTextSize(changeFields[i].desc);
		width = BB_MAX(width, size.x);
	}
	width += ImGui::GetStyle().ItemSpacing.x;

	ImGui::BeginGroup();
	for(int i = 0; i < 3; ++i) {
		ImGui::BeginGroup();
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::TextUnformatted(changeFields[i].desc);
		ImGui::SameLine(width);
		ImGui::PushItemWidth(fullWidth - width);
		if(strcmp(changeFields[i].key, "time")) {
			ImGui::SelectableTextUnformatted(changeFields[i].tag, sdict_find_safe(cl, changeFields[i].key));
		} else {
			ImGui::SelectableTextUnformatted(changeFields[i].tag, Time_StringFromEpochTime(strtou32(sdict_find_safe(cl, changeFields[i].key))));
		}
		ImGui::PopItemWidth();
		ImGui::EndGroup();
	}
	ImGui::EndGroup();
}

static int uiChangelistFile_compare(const void *_a, const void *_b)
{
	uiChangelistFile *a = (uiChangelistFile *)_a;
	uiChangelistFile *b = (uiChangelistFile *)_b;

	int mult = g_config.uiChangelist.sortDescending ? -1 : 1;
	u32 columnIndex = g_config.uiChangelist.sortColumn;
	int val = strcmp(a->str[columnIndex], b->str[columnIndex]);
	if(val) {
		return val * mult;
	} else {
		return g_config.uiChangelist.sortDescending ? (a > b) : (a < b);
	}
}

static void UIChangelist_CopySelectedFilesToClipboard(uiChangelistFiles *files, bool extraInfo)
{
	u32 i;
	sb_t sb;
	sb_init(&sb);
	for(i = 0; i < files->count; ++i) {
		uiChangelistFile *file = files->data + i;
		if(file->selected) {
			if(extraInfo) {
				sb_va(&sb, "%s#%s\n", file->str[4], file->str[1]);
			} else {
				sb_va(&sb, "%s\n", file->str[4]);
			}
		}
	}
	const char *clipboardText = sb_get(&sb);
	ImGui::SetClipboardText(clipboardText);
	sb_reset(&sb);
}

static void UIChangelist_Logs_ClearSelection(uiChangelistFiles *files)
{
	files->lastClickIndex = ~0U;
	for(u32 i = 0; i < files->count; ++i) {
		files->data[i].selected = false;
	}
}

static void UIChangelist_Logs_SelectAll(uiChangelistFiles *files)
{
	files->lastClickIndex = ~0U;
	for(u32 i = 0; i < files->count; ++i) {
		files->data[i].selected = true;
	}
}

static void UIChangelist_Logs_AddSelection(uiChangelistFiles *files, u32 index)
{
	uiChangelistFile *file = files->data + index;
	file->selected = true;
	files->lastClickIndex = index;
}

static void UIChangelist_ToggleSelection(uiChangelistFiles *files, u32 index)
{
	uiChangelistFile *file = files->data + index;
	file->selected = !file->selected;
	files->lastClickIndex = (file->selected) ? index : ~0U;
}

static void UIChangelist_HandleClick(uiChangelistFiles *files, u32 index)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.KeyAlt || (io.KeyCtrl && io.KeyShift))
		return;

	if(io.KeyCtrl) {
		UIChangelist_ToggleSelection(files, index);
	} else if(io.KeyShift) {
		if(files->lastClickIndex < files->count) {
			u32 startIndex = files->lastClickIndex;
			u32 endIndex = index;
			files->lastClickIndex = endIndex;
			if(endIndex < startIndex) {
				u32 tmp = endIndex;
				endIndex = startIndex;
				startIndex = tmp;
			}
			for(u32 i = startIndex; i <= endIndex; ++i) {
				files->data[i].selected = true;
			}
		}
	} else {
		UIChangelist_Logs_ClearSelection(files);
		UIChangelist_Logs_AddSelection(files, index);
	}
}

//////////////////////////////////////////////////////////////////////////
// for use in other UIs

void UIChangelist_DrawSingleLine(sdict_t *)
{
	// Submitted CL has columns: Changelist, Date Submitted, Submitted By, Description
	// Pending CL has columns: Changelist, User, Description
}

void UIChangelist_DrawInformation(sdict_t *cl)
{
	float fullWidth = ImGui::GetContentRegionAvailWidth();
	UIChangelist_DrawInformationColumn(cl, fullWidth * 0.5f, s_changeFields[0]);
	ImGui::SameLine();
	UIChangelist_DrawInformationColumn(cl, fullWidth * 0.5f, s_changeFields[1]);

	ImGui::AlignFirstTextHeightToWidgets();
	ImGui::TextUnformatted("Description:");
	ImGui::SelectableTextUnformattedMultiline("###desc", sdict_find_safe(cl, "desc"), ImVec2(fullWidth, 0.0f));
}

static void UIChangelist_DrawFileColumn(float offset, float width, const char *text, const char *end = nullptr)
{
	float windowX = ImGui::GetWindowPos().x;
	float x1 = floorf(0.5f + windowX + offset - 1.0f);
	float x2 = floorf(0.5f + windowX + offset + width - 1.0f);
	ImGui::SameLine(offset);
	ImGui::PushClipRect(ImVec2(x1, -FLT_MAX), ImVec2(x2, +FLT_MAX), true);
	ImGui::TextUnformatted(text, end);
	ImGui::PopClipRect();
}

static b32 UIChangelist_DrawFileColumnHeader(uiChangelistFiles *files, const char *text, uiChangelistConfig *config, u32 columnIndex, float *offset)
{
	b32 anyActive = false;

	float *width = config->columnWidth + columnIndex;
	bool last = columnIndex + 1 == BB_ARRAYSIZE(config->columnWidth);
	if(last) {
		*width = ImGui::GetContentRegionAvailWidth();
	} else if(*width <= 0.0f) {
		*width = ImGui::CalcTextSize(text).x + ImGui::CalcTextSize(" ^").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
		if(!columnIndex) {
			*width *= 1.5f;
		}
	}
	float scale = (g_config.dpiScale <= 0.0f) ? 1.0f : g_config.dpiScale;
	float startOffset = ImGui::GetCursorPosX();
	if(ImGui::Button(va("###%s", text), ImVec2(*width * scale, 0.0f))) {
		if(config->sortColumn == columnIndex) {
			config->sortDescending = !config->sortDescending;
		} else {
			config->sortColumn = columnIndex;
			config->sortDescending = false;
		}
		qsort(files->data, files->count, sizeof(uiChangelistFile), &uiChangelistFile_compare);
	}
	anyActive = anyActive || ImGui::IsItemActive();
	float endOffset = startOffset + *width * scale + ImGui::GetStyle().ItemSpacing.x;
	const char *columnText = (config->sortColumn == columnIndex) ? va("%s %s", config->sortDescending ? "^" : "v", text) : text;
	const float itemPad = ImGui::GetStyle().ItemSpacing.x;
	UIChangelist_DrawFileColumn(startOffset + ImGui::GetStyle().ItemInnerSpacing.x, *width * scale - itemPad, columnText);
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
			anyActive = true;
			*width += ImGui::GetIO().MouseDelta.x / scale;
		}
		ImGui::SameLine();
	}

	*offset = ImGui::GetCursorPosX();
	return anyActive;
}

void UIChangelist_DrawFiles(uiChangelistFiles *files)
{
	// Columns: File Name, Revision, Action, Filetype, In Folder

	float columnOffsets[6] = {};

	b32 anyActive = false;

	anyActive = UIChangelist_DrawFileColumnHeader(files, "File Name", &g_config.uiChangelist, 0, columnOffsets + 1) || anyActive;
	anyActive = UIChangelist_DrawFileColumnHeader(files, "Revision", &g_config.uiChangelist, 1, columnOffsets + 2) || anyActive;
	anyActive = UIChangelist_DrawFileColumnHeader(files, "Action", &g_config.uiChangelist, 2, columnOffsets + 3) || anyActive;
	anyActive = UIChangelist_DrawFileColumnHeader(files, "Filetype", &g_config.uiChangelist, 3, columnOffsets + 4) || anyActive;
	anyActive = UIChangelist_DrawFileColumnHeader(files, "In Folder", &g_config.uiChangelist, 4, columnOffsets + 5) || anyActive;
	ImGui::NewLine();
	ImGui::Separator();

	const float itemPad = ImGui::GetStyle().ItemSpacing.x;
	for(u32 i = 0; i < files->count; ++i) {
		uiChangelistFile &file = files->data[i];
		ImGui::Selectable(va("###%s", file.str[0]), file.selected != 0);
		if(ImGui::IsItemHovered()) {
			if(ImGui::IsItemClicked()) {
				UIChangelist_HandleClick(files, i);
			}
			//scrollDir = ImGui::GetVerticalScrollDir();
		}
		if(ImGui::IsItemActive()) {
			anyActive = true;
		}

		UIChangelist_DrawFileColumn(columnOffsets[0], g_config.uiChangelist.columnWidth[0] + itemPad, file.str[0]);
		UIChangelist_DrawFileColumn(columnOffsets[1], g_config.uiChangelist.columnWidth[1] + itemPad, file.str[1]);
		UIChangelist_DrawFileColumn(columnOffsets[2], g_config.uiChangelist.columnWidth[2] + itemPad, file.str[2]);
		UIChangelist_DrawFileColumn(columnOffsets[3], g_config.uiChangelist.columnWidth[3] + itemPad, file.str[3]);
		UIChangelist_DrawFileColumn(columnOffsets[4], g_config.uiChangelist.columnWidth[4] + itemPad, file.str[4], strrchr(file.str[4], '/'));
	}

	if(anyActive) {
		files->active = true;
	} else if(ImGui::IsAnyItemActive()) {
		files->active = false;
	}

	if(files->active) {
		if(ImGui::IsKeyPressed('A') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_Logs_SelectAll(files);
		} else if(ImGui::IsKeyPressed('C') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_CopySelectedFilesToClipboard(files, ImGui::GetIO().KeyShift);
		} else if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape])) {
			UIChangelist_Logs_ClearSelection(files);
			files->active = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// for standalone CL viewer

static u32 requestedChangelist;
static u32 displayedChangelist;
static uiChangelistFiles s_files;

static void UIChangelist_FreeFiles(void)
{
	for(u32 i = 0; i < s_files.count; ++i) {
		for(u32 col = 0; col < BB_ARRAYSIZE(s_files.data[i].str); ++col) {
			free(s_files.data[i].str[col]);
		}
	}
	bba_free(s_files);
}

void UIChangelist_Shutdown(void)
{
	UIChangelist_FreeFiles();
}

static void UIChangelist_PopulateFiles(sdict_t *cl)
{
	UIChangelist_FreeFiles();
	for(u32 i = 9; i + 5 < cl->count; i += 6) {
		const char *depotFile = sdict_getPrefix(cl->data + i + 0, "depotFile");
		const char *action = sdict_getPrefix(cl->data + i + 1, "action");
		const char *type = sdict_getPrefix(cl->data + i + 2, "type");
		const char *rev = sdict_getPrefix(cl->data + i + 3, "rev");
		const char *lastSlash = strrchr(depotFile, '/');
		const char *filename = (lastSlash) ? lastSlash + 1 : nullptr;

		if(filename && bba_add(s_files, 1)) {
			uiChangelistFile &file = bba_last(s_files);
			file.str[0] = _strdup(filename);
			file.str[1] = _strdup(rev);
			file.str[2] = _strdup(action);
			file.str[3] = _strdup(type);
			file.str[4] = _strdup(depotFile);
		}
	}
	qsort(s_files.data, s_files.count, sizeof(uiChangelistFile), &uiChangelistFile_compare);
	s_files.lastClickIndex = ~0u;
}

void UIChangelist_Update(void)
{
	if(!requestedChangelist) {
		ImGui::TextUnformatted("Changelist:");
		ImGui::SameLine();
		static char inputChangelist[64];
		ImGui::SetKeyboardFocusHere();
		if(ImGui::InputText("###CL", inputChangelist, sizeof(inputChangelist), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
			s32 testChangelist = strtos32(inputChangelist);
			if(testChangelist > 0) {
				requestedChangelist = (u32)testChangelist;
				p4_describe_changelist(requestedChangelist);
			}
		}
	}

	sdict_t *cl = p4_find_changelist(requestedChangelist);
	if(cl) {
		if(!displayedChangelist) {
			displayedChangelist = requestedChangelist;
			UIChangelist_PopulateFiles(cl);
		}
		UIChangelist_DrawInformation(cl);
		UIChangelist_DrawFiles(&s_files);
		ImGui::Separator();
	}
}
