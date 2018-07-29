// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changelist.h"

#include "config.h"
#include "imgui_utils.h"
#include "message_box.h"
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

static bool s_doneInit = false;

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
				sb_va(&sb, "%s#%s\n", file->field.depotPath, file->field.rev);
			} else {
				sb_va(&sb, "%s\n", file->field.depotPath);
			}
		}
	}
	const char *clipboardText = sb_get(&sb);
	ImGui::SetClipboardText(clipboardText);
	sb_reset(&sb);
}

static void UIChangelist_DiffSelectedFiles(uiChangelistFiles *files, p4Changelist *cl)
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
			u32 rev = strtou32(file->field.rev);
			if(pending) {
				if(shelved) {
					p4_diff_against_depot(file->field.depotPath, va("#%u", rev), file->field.depotPath, va("@=%u", cl->number));
				} else if(localClient) {
					if(*file->field.localPath) {
						p4_diff_against_local(file->field.depotPath, va("#%u", rev), file->field.localPath);
					}
				}
			} else {
				if(rev) {
					p4_diff_against_depot(file->field.depotPath, va("#%u", rev - 1), file->field.depotPath, va("#%u", rev));
				}
			}
		}
	}
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
		if(files->active) {
			UIChangelist_Logs_ClearSelection(files);
		}
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

	float scale = (g_config.dpiScale <= 0.0f) ? 1.0f : g_config.dpiScale;
	if(g_config.uiChangelist.descHeight <= 0.0f) {
		g_config.uiChangelist.descHeight = (ImGui::GetTextLineHeight() * 8.0f + ImGui::GetStyle().FramePadding.y * 2.0f) / scale;
	}

	ImGui::AlignFirstTextHeightToWidgets();
	ImGui::TextUnformatted("Description:");
	ImGui::SelectableTextUnformattedMultiline("###desc", sdict_find_safe(cl, "desc"), ImVec2(fullWidth, g_config.uiChangelist.descHeight * scale));

	const float normal = 0.4f;
	const float hovered = 0.8f;
	const float active = 0.8f;
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(normal, normal, normal, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(hovered, hovered, hovered, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(active, active, active, 1.0f));
	ImGui::Button("-###sepDesc", ImVec2(ImGui::GetContentRegionAvailWidth(), 2.0f * g_config.dpiScale));
	ImGui::PopStyleColor(3);
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

void UIChangelist_DrawFiles(uiChangelistFiles *files, p4Changelist *cl, uiChangelistFiles *otherFiles)
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
	for(u32 i = 0; i < BB_ARRAYSIZE(g_config.uiChangelist.columnWidth); ++i) {
		ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
		anyActive = anyActive || res.active;
		if(res.sortChanged) {
			qsort(files->data, files->count, sizeof(uiChangelistFile), &uiChangelistFile_compare);
		}
	}
	ImGui::NewLine();

	const float itemPad = ImGui::GetStyle().ItemSpacing.x;
	for(u32 i = 0; i < files->count; ++i) {
		uiChangelistFile &file = files->data[i];
		if(file.selected && !files->active) {
			ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Header];
			col.x *= 0.5f;
			col.y *= 0.5f;
			col.z *= 0.5f;
			ImGui::PushStyleColor(ImGuiCol_Header, col);
		}
		ImGui::Selectable(va("###%s", file.field.filename), file.selected != 0);
		if(file.selected && !files->active) {
			ImGui::PopStyleColor();
		}
		if(ImGui::IsItemHovered()) {
			if(ImGui::IsItemClicked()) {
				UIChangelist_HandleClick(files, i);
				if(otherFiles) {
					UIChangelist_Logs_ClearSelection(otherFiles);
				}
			}
		}
		if(ImGui::IsItemActive()) {
			anyActive = true;
		}

		ImGui::DrawColumnHeaderText(columnOffsets[0], g_config.uiChangelist.columnWidth[0] + itemPad, file.str[0]);
		ImGui::DrawColumnHeaderText(columnOffsets[1], g_config.uiChangelist.columnWidth[1] + itemPad, file.str[1]);
		ImGui::DrawColumnHeaderText(columnOffsets[2], g_config.uiChangelist.columnWidth[2] + itemPad, file.str[2]);
		ImGui::DrawColumnHeaderText(columnOffsets[3], g_config.uiChangelist.columnWidth[3] + itemPad, file.str[3]);
		ImGui::DrawColumnHeaderText(columnOffsets[4], g_config.uiChangelist.columnWidth[4] + itemPad, file.str[4], strrchr(file.str[4], '/'));
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
		} else if(ImGui::IsKeyPressed('D') && ImGui::GetIO().KeyCtrl) {
			UIChangelist_DiffSelectedFiles(files, cl);
		} else if(ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape])) {
			UIChangelist_Logs_ClearSelection(files);
			files->active = false;
		}
	}

	ImGui::PopID();
}

//////////////////////////////////////////////////////////////////////////
// for standalone CL viewer

static u32 s_requestedChangelist;
static u32 s_displayedChangelist;
static u32 s_parity;
static uiChangelistFiles s_normalFiles;
static uiChangelistFiles s_shelvedFiles;

static void UIChangelist_FreeFiles(uiChangelistFiles *files)
{
	for(u32 i = 0; i < files->count; ++i) {
		for(u32 col = 0; col < BB_ARRAYSIZE(files->data[i].str); ++col) {
			free(files->data[i].str[col]);
		}
	}
	bba_free(*files);
}

void UIChangelist_Shutdown(void)
{
	UIChangelist_FreeFiles(&s_normalFiles);
	UIChangelist_FreeFiles(&s_shelvedFiles);
}

static void UIChangelist_PopulateFiles(sdict_t *change, sdicts *sds, uiChangelistFiles *files)
{
	UIChangelist_FreeFiles(files);
	u32 sdictIndex = 0;
	while(1) {
		u32 fileIndex = files->count;
		u32 depotFileIndex = sdict_find_index_from(change, va("depotFile%u", fileIndex), sdictIndex);
		u32 actionIndex = sdict_find_index_from(change, va("action%u", fileIndex), depotFileIndex);
		u32 typeIndex = sdict_find_index_from(change, va("type%u", fileIndex), actionIndex);
		u32 revIndex = sdict_find_index_from(change, va("rev%u", fileIndex), typeIndex);
		sdictIndex = revIndex;
		if(revIndex < change->count) {
			const char *depotFile = sb_get(&change->data[depotFileIndex].value);
			const char *action = sb_get(&change->data[actionIndex].value);
			const char *type = sb_get(&change->data[typeIndex].value);
			const char *rev = sb_get(&change->data[revIndex].value);
			const char *lastSlash = strrchr(depotFile, '/');
			const char *filename = (lastSlash) ? lastSlash + 1 : nullptr;
			const char *localPath = "";
			for(u32 i = 0; i < sds->count; ++i) {
				sdict_t *sd = sds->data + i;
				const char *detailedDepotFile = sdict_find_safe(sd, "depotFile");
				if(!strcmp(detailedDepotFile, depotFile)) {
					localPath = sdict_find_safe(sd, "path");
				}
			}
			if(filename && bba_add(*files, 1)) {
				uiChangelistFile &file = bba_last(*files);
				file.field.filename = _strdup(filename);
				file.field.rev = _strdup(rev);
				file.field.action = _strdup(action);
				file.field.filetype = _strdup(type);
				file.field.depotPath = _strdup(depotFile);
				file.field.localPath = _strdup(localPath);
			}
		} else {
			break;
		}
	}
	qsort(files->data, files->count, sizeof(uiChangelistFile), &uiChangelistFile_compare);
	files->lastClickIndex = ~0u;
	files->active = false;
}

static void UIChangelist_MessageBoxCallback(messageBox *mb, const char *action)
{
	if(action) {
		const char *inputNumber = sdict_find_safe(&mb->data, "inputNumber");
		s32 testChangelist = strtos32(inputNumber);
		if(testChangelist > 0) {
			s_requestedChangelist = (u32)testChangelist;
			s_displayedChangelist = 0;
			p4_describe_changelist(s_requestedChangelist);
		}
	}
}

static void UIChangelist_FilesHeader(const char *text, b32 shelved)
{
	float r = 0.3f;
	float g = 0.3f;
	float b = (shelved && s_shelvedFiles.active || !shelved && s_normalFiles.active) ? 0.4f : 0.3f;
	ImVec4 col(r, g, b, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Header, col);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, col);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, col);
	ImGui::PushStyleColor(ImGuiCol_SelectableBg, col);
	if(ImGui::Selectable(text)) {
		s_normalFiles.active = !shelved;
		s_shelvedFiles.active = shelved;
	}
	ImGui::PopStyleColor(4);
}

void UIChangelist_EnterChangelist(void)
{
	s_doneInit = true;
	messageBox mb = {};
	mb.callback = UIChangelist_MessageBoxCallback;
	sdict_add_raw(&mb.data, "title", "View Changelist");
	sdict_add_raw(&mb.data, "text", "Enter changelist to view:");
	sdict_add_raw(&mb.data, "inputNumber", va("%u", s_requestedChangelist));
	mb_queue(mb);
}

void UIChangelist_Update(void)
{
	//float startY = ImGui::GetItemsLineHeightWithSpacing();
	//ImGuiIO &io = ImGui::GetIO();
	//ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
	//ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);

	if(!s_doneInit || ImGui::IsKeyPressed('G') && ImGui::GetIO().KeyCtrl) {
		UIChangelist_EnterChangelist();
	}

	//bool open = true;
	//if(ImGui::Begin("ViewChangelist", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
	p4Changelist *cl = p4_find_changelist(s_requestedChangelist);
	p4Changelist empty = {};
	if(!cl) {
		cl = &empty;
	}
	if(cl) {
		if(!s_displayedChangelist || s_parity != cl->parity) {
			s_displayedChangelist = s_requestedChangelist;
			s_parity = cl->parity;
			UIChangelist_PopulateFiles(&cl->normal, &cl->normalFiles, &s_normalFiles);
			if(sdict_find(&cl->normal, "shelved")) {
				s_shelvedFiles.shelved = true;
				UIChangelist_PopulateFiles(&cl->shelved, &cl->shelvedFiles, &s_shelvedFiles);
				s_shelvedFiles.active = true;
			} else {
				s_normalFiles.active = true;
				UIChangelist_FreeFiles(&s_shelvedFiles);
			}
		}
		UIChangelist_DrawInformation(&cl->normal);
		const char *status = sdict_find_safe(&cl->normal, "status");
		b32 pending = !strcmp(status, "pending");
		if(pending) {
			UIChangelist_FilesHeader(va("Pending File%s: %u", s_normalFiles.count == 1 ? "" : "s", s_normalFiles.count), false);
		} else if(*status) {
			UIChangelist_FilesHeader(va("Submitted File%s: %u", s_normalFiles.count == 1 ? "" : "s", s_normalFiles.count), false);
		} else {
			UIChangelist_FilesHeader("Files: 0", false);
		}
		UIChangelist_DrawFiles(&s_normalFiles, cl, &s_shelvedFiles);
		if(s_shelvedFiles.count) {
			UIChangelist_FilesHeader(va("Shelved File%s: %u", s_shelvedFiles.count == 1 ? "" : "s", s_shelvedFiles.count), true);
			UIChangelist_DrawFiles(&s_shelvedFiles, cl, &s_normalFiles);
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		if(ImGui::Button("###blank", ImGui::GetContentRegionAvail()) || ImGui::IsItemActive()) {
			s_normalFiles.active = s_shelvedFiles.count == 0;
			s_shelvedFiles.active = s_shelvedFiles.count != 0;
		}
		ImGui::PopStyleColor(3);
	}
	//}
	//ImGui::End();
}
