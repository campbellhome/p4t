// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changelist.h"
#include "app.h"
#include "bb_array.h"
#include "config.h"
#include "imgui_utils.h"
#include "message_box.h"
#include "output.h"
#include "p4.h"
#include "sdict.h"
#include "str.h"
#include "time_utils.h"
#include "ui_icons.h"
#include "va.h"
#include <math.h>

// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
BB_WARNING_PUSH(4820)
#include <time.h>
BB_WARNING_POP

//////////////////////////////////////////////////////////////////////////

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
			u32 time = strtou32(sdict_find_safe(cl, changeFields[i].key));
			ImGui::SelectableTextUnformatted(changeFields[i].tag, time ? Time_StringFromEpochTime(time) : "");
		}
		ImGui::PopItemWidth();
		ImGui::EndGroup();
	}
	ImGui::EndGroup();
}

static void UIChangelist_Files_CopySelectedToClipboard(uiChangelistFiles *files, bool extraInfo)
{
	u32 i;
	sb_t sb;
	sb_init(&sb);
	for(i = 0; i < files->count; ++i) {
		uiChangelistFile *file = files->data + i;
		if(file->selected) {
			if(extraInfo) {
				sb_va(&sb, "%s#%s\n", file->fields.field.depotPath, file->fields.field.rev);
			} else {
				sb_va(&sb, "%s\n", file->fields.field.depotPath);
			}
		}
	}
	const char *clipboardText = sb_get(&sb);
	ImGui::SetClipboardText(clipboardText);
	sb_reset(&sb);
}

static void UIChangelist_Files_DiffSelected(uiChangelistFiles *files, p4Changelist *cl)
{
	const char *infoClientName = p4_clientspec();
	const char *clientName = sdict_find_safe(&cl->normal, "client");
	bool localClient = !strcmp(infoClientName, clientName);

	const char *status = sdict_find_safe(&cl->normal, "status");
	b32 pending = !strcmp(status, "pending");
	b32 shelved = files->shelved;

	for(u32 i = 0; i < files->count; ++i) {
		uiChangelistFile *file = files->data + i;
		if(file->selected) {
			u32 rev = strtou32(file->fields.field.rev);
			if(pending) {
				if(shelved) {
					p4_diff_against_depot(file->fields.field.depotPath, va("#%u", rev), file->fields.field.depotPath, va("@=%u", cl->number));
				} else if(localClient) {
					if(*file->fields.field.localPath) {
						p4_diff_against_local(file->fields.field.depotPath, va("#%u", rev), file->fields.field.localPath);
					}
				}
			} else {
				if(rev) {
					p4_diff_against_depot(file->fields.field.depotPath, va("#%u", rev - 1), file->fields.field.depotPath, va("#%u", rev));
				}
			}
		}
	}
}

static void UIChangelist_Files_ClearSelection(uiChangelistFiles *files)
{
	files->lastClickIndex = ~0U;
	for(u32 i = 0; i < files->count; ++i) {
		files->data[i].selected = false;
	}
}

static void UIChangelist_Files_SelectAll(uiChangelistFiles *files)
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
		UIChangelist_Files_ClearSelection(files);
		UIChangelist_Logs_AddSelection(files, index);
	}
}

//////////////////////////////////////////////////////////////////////////
// for use in other UIs

void UIChangelist_DrawFileIcon(uiChangelistFiles *files, uiChangelistFile *file, p4ChangelistType cltype)
{
	const char *icon = UIIcons_ClassifyFile(file->fields.field.depotPath, file->fields.field.filetype);
	ImColor disabledColor = ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
	ImColor iconColor = files->shelved ? COLOR_FILE_SHELVED : disabledColor;

	const char *actionIcon = nullptr;
	ImColor actionIconColor = COLOR_FILE_SHELVED_ACTION;
	if(!files->shelved) {
		if(cltype == kChangelistType_PendingLocal) {
			actionIconColor = COLOR_FILE_LOCAL_ACTION;
		} else if(cltype == kChangelistType_PendingOther) {
			actionIconColor = COLOR_FILE_REMOTE_ACTION;
		} else {
			actionIconColor = COLOR_FILE_SUBMITTED_ACTION;
		}
	}
	if(!strcmp(file->fields.field.action, "add")) {
		actionIcon = ICON_FK_PLUS;
	} else if(!strcmp(file->fields.field.action, "edit")) {
		actionIcon = ICON_FK_CHECK;
	} else if(!strcmp(file->fields.field.action, "integrate")) {
		actionIcon = ICON_FK_SHARE;
	} else if(!strcmp(file->fields.field.action, "branch")) {
		actionIcon = ICON_FK_SHARE;
	} else if(!strcmp(file->fields.field.action, "delete")) {
		actionIcon = ICON_FK_TIMES;
	}

	const char *statusIcon = nullptr;
	ImColor statusIconColor;
	if(file->unresolved) {
		statusIcon = ICON_FILE_UNRESOLVED;
		statusIconColor = COLOR_FILE_UNRESOLVED;
	} else if(strchr(file->fields.field.rev, '/') != nullptr) {
		statusIcon = ICON_FILE_OUT_OF_DATE;
		statusIconColor = COLOR_FILE_OUT_OF_DATE;
	}

	ImVec2 size = ImGui::CalcTextSize(icon);
	ImVec2 pos = ImGui::GetIconPosForText();
	ImGui::IconColored(iconColor, icon);
	if(statusIcon) {
		ImVec2 statusPos(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
		ImGui::DrawIconAtPos(statusPos, statusIcon, statusIconColor, false, 1.0f);
	}
	if(actionIcon) {
		if(cltype == kChangelistType_Submitted) {
			ImVec2 actionPos(pos.x - size.x * 0.25f, pos.y);
			ImGui::DrawIconAtPos(actionPos, actionIcon, actionIconColor, false, 0.5f);
		} else if(cltype == kChangelistType_PendingLocal) {
			ImVec2 actionPos(pos.x - size.x * 0.25f, pos.y);
			ImGui::DrawIconAtPos(actionPos, actionIcon, actionIconColor, false, 0.5f);
		} else {
			ImVec2 actionPos(pos.x + size.x * 0.75f, pos.y);
			ImGui::DrawIconAtPos(actionPos, actionIcon, actionIconColor, false, 0.5f);
		}
	}
}

void UIChangelist_DrawInformation(sdict_t *cl)
{
	float fullWidth = ImGui::GetContentRegionAvailWidth();
	UIChangelist_DrawInformationColumn(cl, fullWidth * 0.5f, s_changeFields[0]);
	ImGui::SameLine();
	UIChangelist_DrawInformationColumn(cl, fullWidth * 0.5f, s_changeFields[1]);

	float scale = (g_config.dpiScale <= 0.0f) ? 1.0f : g_config.dpiScale;
	if(g_config.uiChangelist.descHeight <= 0.0f) {
		g_config.uiChangelist.descHeight = (ImGui::GetTextLineHeight() * 8.0f + ImGui::GetStyle().FramePadding.y * 2.0f) / scale;
	}

	ImGui::AlignFirstTextHeightToWidgets();
	ImGui::TextUnformatted("Description:");
	ImGui::SelectableTextUnformattedMultiline("###desc", sdict_find_safe(cl, "desc"), ImVec2(fullWidth, g_config.uiChangelist.descHeight * scale));

	ImGui::Button("-###sepDesc", ImGui::kButton_ResizeBar, ImVec2(ImGui::GetContentRegionAvailWidth(), 2.0f * g_config.dpiScale));
	if(ImGui::IsItemActive()) {
		g_config.uiChangelist.descHeight += ImGui::GetIO().MouseDelta.y / scale;
	}
}

static float s_columnScales[] = { 1.5f, 1.0f, 1.0f, 1.0f, 1.0f };
BB_CTASSERT(BB_ARRAYSIZE(s_columnScales) == BB_ARRAYSIZE(g_config.uiChangelist.columnWidth));

static const char *s_columnNames[] = {
	"File Name",
	"Revision",
	"Action",
	"Filetype",
	"In Folder",
};
BB_CTASSERT(BB_ARRAYSIZE(s_columnNames) == BB_ARRAYSIZE(g_config.uiChangelist.columnWidth));

b32 UIChangelist_FileSelectable(uiChangelistFiles *files, uiChangelistFile &file, u32 index)
{
	b32 anyActive = false;
	ImGui::PushSelectableColors(file.selected, ImGui::IsActiveSelectables(files));
	ImGui::Selectable(va("###%s", file.fields.field.filename), file.selected != 0);
	ImGui::PopSelectableColors(file.selected, ImGui::IsActiveSelectables(files));
	if(ImGui::IsItemActive()) {
		anyActive = true;
	}
	if(ImGui::IsItemHovered()) {
		if(ImGui::IsItemClicked()) {
			anyActive = true;
			UIChangelist_HandleClick(files, index);
		}
		if(ImGui::BeginContextMenu(va("context_%p_%d", files, index))) {
			anyActive = true;
			if(!file.selected) {
				UIChangelist_Files_ClearSelection(files);
				UIChangelist_Logs_AddSelection(files, index);
			}
			ImGui::EndContextMenu();
		}
	}
	return anyActive;
}

void UIChangelist_FinishFiles(uiChangelistFiles *files, p4Changelist *cl, b32 anyActive)
{
	if(anyActive) {
		ImGui::SetActiveSelectables(files);
	} else if(ImGui::IsAnyItemActive() && ImGui::IsActiveSelectables(files)) {
		ImGui::SetActiveSelectables(nullptr);
	}

	if(ImGui::IsActiveSelectables(files)) {
		if(ImGui::IsKeyPressed('A') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_Files_SelectAll(files);
		} else if(ImGui::IsKeyPressed('C') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_Files_CopySelectedToClipboard(files, ImGui::GetIO().KeyShift);
		} else if(ImGui::IsKeyPressed('D') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_Files_DiffSelected(files, cl);
		} else if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape])) {
			UIChangelist_Files_ClearSelection(files);
			ImGui::SetActiveSelectables(nullptr);
		}
	}
}

void UIChangelist_DrawFiles(uiChangelistFiles *files, p4Changelist *cl, float indent)
{
	// Columns: File Name, Revision, Action, Filetype, In Folder

	ImGui::PushID(files);

	b32 anyActive = false;

	float columnOffsets[6] = {};
	BB_CTASSERT(BB_ARRAYSIZE(columnOffsets) == BB_ARRAYSIZE(g_config.uiChangelist.columnWidth) + 1);
	ImGui::columnDrawData data = {};
	data.columnWidths = g_config.uiChangelist.columnWidth;
	data.columnScales = s_columnScales;
	data.columnOffsets = columnOffsets;
	data.columnNames = s_columnNames;
	data.sortDescending = &g_config.uiChangelist.sortDescending;
	data.sortColumn = &g_config.uiChangelist.sortColumn;
	data.numColumns = BB_ARRAYSIZE(g_config.uiChangelist.columnWidth);
	ImGui::TextUnformatted("");
	ImGui::SameLine(0.0f, indent);
	columnOffsets[0] = ImGui::GetCursorPosX();
	for(u32 i = 0; i < BB_ARRAYSIZE(g_config.uiChangelist.columnWidth); ++i) {
		ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
		anyActive = anyActive || res.active;
		if(res.sortChanged) {
			qsort(files->data, files->count, sizeof(uiChangelistFile), &p4_changelist_files_compare);
		}
	}
	ImGui::NewLine();

	p4ChangelistType cltype = p4_get_changelist_type(&cl->normal);

	const float itemPad = ImGui::GetStyle().ItemSpacing.x;
	for(u32 i = 0; i < files->count; ++i) {
		float start = ImGui::GetIconPosForText().x - ImGui::GetStyle().ItemSpacing.x;
		uiChangelistFile &file = files->data[i];
		if(UIChangelist_FileSelectable(files, file, i)) {
			anyActive = true;
		}

		ImGui::SameLine(start);
		ImGui::PushColumnHeaderClipRect(columnOffsets[0], g_config.uiChangelist.columnWidth[0] * g_config.dpiScale + itemPad);
		UIChangelist_DrawFileIcon(files, &file, cltype);
		ImGui::PopClipRect();
		ImGui::DrawColumnHeaderText(columnOffsets[0], g_config.uiChangelist.columnWidth[0] * g_config.dpiScale + itemPad, va("%s%s", UIIcons_GetIconSpaces(ICON_FK_FILE), file.fields.str[0]));
		ImGui::DrawColumnHeaderText(columnOffsets[1], g_config.uiChangelist.columnWidth[1] * g_config.dpiScale + itemPad, file.fields.str[1]);
		ImGui::DrawColumnHeaderText(columnOffsets[2], g_config.uiChangelist.columnWidth[2] * g_config.dpiScale + itemPad, file.fields.str[2]);
		ImGui::DrawColumnHeaderText(columnOffsets[3], g_config.uiChangelist.columnWidth[3] * g_config.dpiScale + itemPad, file.fields.str[3]);
		ImGui::DrawColumnHeaderText(columnOffsets[4], g_config.uiChangelist.columnWidth[4] * g_config.dpiScale + itemPad, file.fields.str[4], strrchr(file.fields.str[4], '/'));
	}

	UIChangelist_FinishFiles(files, cl, anyActive);

	ImGui::PopID();
}

void UIChangelist_DrawFilesNoColumns(uiChangelistFiles *files, p4Changelist *cl, float indent)
{
	ImGui::PushID(files);

	b32 anyActive = false;

	p4ChangelistType cltype = p4_get_changelist_type(&cl->normal);

	const float itemPad = ImGui::GetStyle().ItemSpacing.x;
	for(u32 i = 0; i < files->count; ++i) {
		uiChangelistFile &file = files->data[i];
		if(UIChangelist_FileSelectable(files, file, i)) {
			anyActive = true;
		}

		ImGui::SameLine(0.0f, indent);
		UIChangelist_DrawFileIcon(files, &file, cltype);
		ImGui::SameLine();
		if(cltype == kChangelistType_Submitted) {
			ImGui::Text("%s#%s", file.fields.field.depotPath, file.fields.field.rev);
		} else {
			ImGui::Text("%s#%s <%s>", file.fields.field.depotPath, file.fields.field.rev, file.fields.field.filetype);
		}
	}

	UIChangelist_FinishFiles(files, cl, anyActive);

	ImGui::PopID();
}

//////////////////////////////////////////////////////////////////////////
// for standalone CL viewer

void UIChangelist_Shutdown(void)
{
}

static void UIChangelist_MessageBoxCallback(messageBox *mb, const char *action)
{
	if(action) {
		p4UIChangelist *uicl = p4_find_uichangelist(strtou32(sdict_find_safe(&mb->data, "id")));
		if(uicl) {
			if(!strcmp(action, "escape")) {
				if(uicl->requested == 0) {
					p4_mark_uichangelist_for_removal(uicl);
				}
				return;
			}
			const char *inputNumber = sdict_find_safe(&mb->data, "inputNumber");
			s32 testChangelist = strtos32(inputNumber);
			if(testChangelist > 0) {
				uicl->requested = (u32)testChangelist;
				uicl->displayed = 0;
				p4_describe_changelist(uicl->requested);
			}
		}
	}
}

void UIChangelist_EnterChangelist(p4UIChangelist *uicl)
{
	messageBox mb = {};
	mb.callback = UIChangelist_MessageBoxCallback;
	sdict_add_raw(&mb.data, "title", "View Changelist");
	sdict_add_raw(&mb.data, "text", "Enter changelist to view:");
	sdict_add_raw(&mb.data, "inputNumber", va("%u", uicl->requested));
	sdict_add_raw(&mb.data, "id", va("%u", uicl->id));
	mb_queue(mb);
}

void UIChangelist_DrawFilesAndHeaders(p4Changelist *cl, uiChangelistFiles *normalFiles, uiChangelistFiles *shelvedFiles, b32 shelvedOpenByDefault, float indent)
{
	const char *status = sdict_find_safe(&cl->normal, "status");
	b32 pending = !strcmp(status, "pending");
	const char *title;
	if(pending) {
		title = va("Pending File%s: %u", normalFiles->count == 1 ? "" : "s", normalFiles->count);
	} else if(*status) {
		title = va("Submitted File%s: %u", normalFiles->count == 1 ? "" : "s", normalFiles->count);
	} else {
		title = "Files: 0";
	}

	ImGui::TextUnformatted("");
	ImGui::SameLine(0.0f, indent);
	b32 expanded = ImGui::TreeNodeEx(va("%s###files%u%s", title, cl->number, sdict_find_safe(&cl->normal, "client")), ImGuiTreeNodeFlags_DefaultOpen);
	if(expanded) {
		UIChangelist_DrawFiles(normalFiles, cl, indent);
		ImGui::TreePop();
	}
	if(shelvedFiles->count) {
		ImGui::TextUnformatted("");
		ImGui::SameLine(0.0f, indent);
		title = va("Shelved File%s: %u", shelvedFiles->count == 1 ? "" : "s", shelvedFiles->count);
		expanded = ImGui::TreeNodeEx(va("%s###shelved%u%s", title, cl->number, sdict_find_safe(&cl->normal, "client")), shelvedOpenByDefault ? ImGuiTreeNodeFlags_DefaultOpen : 0);
		if(expanded) {
			UIChangelist_DrawFiles(shelvedFiles, cl, indent);
			ImGui::TreePop();
		}
	}
}

void UIChangelist_SetWindowTitle(p4UIChangelist *uicl)
{
	if(uicl->requested) {
		App_SetWindowTitle(va("Changelist %u - p4t", uicl->requested));
	} else {
		App_SetWindowTitle("Changelist - p4t");
	}
}

void UIChangelist_Update(p4UIChangelist *uicl)
{
	p4Changelist *cl = p4_find_changelist(uicl->requested);
	p4Changelist empty = {};
	if(!cl) {
		cl = &empty;
	}
	if(cl) {
		if(!uicl->displayed || uicl->parity != cl->parity) {
			uicl->displayed = uicl->requested;
			uicl->parity = cl->parity;
			p4_build_changelist_files(cl, &uicl->normalFiles, &uicl->shelvedFiles);
			ImGui::SetActiveSelectables(uicl->shelvedFiles.count == 0 ? &uicl->normalFiles : &uicl->shelvedFiles);
			UIChangelist_SetWindowTitle(uicl);
		}
		UIChangelist_DrawInformation(&cl->normal);
		UIChangelist_DrawFilesAndHeaders(cl, &uicl->normalFiles, &uicl->shelvedFiles, true);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		if(ImGui::Button("###blank", ImGui::GetContentRegionAvail()) || ImGui::IsItemActive()) {
			ImGui::SetActiveSelectables(uicl->shelvedFiles.count == 0 ? &uicl->normalFiles : &uicl->shelvedFiles);
		}
		ImGui::PopStyleColor(3);
	}
}
