// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changeset.h"
#include "bb_array.h"
#include "config.h"
#include "imgui_utils.h"
#include "keys.h"
#include "p4.h"
#include "sdict.h"
#include "str.h"
#include "time_utils.h"
#include "va.h"

const char *s_submittedColumnNames[] = {
	"Change",
	"Date",
	"User",
	"Description",
};
BB_CTASSERT(BB_ARRAYSIZE(s_submittedColumnNames) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

const char *s_pendingColumnNames[] = {
	"Change",
	NULL,
	"User",
	"Description",
};
BB_CTASSERT(BB_ARRAYSIZE(s_pendingColumnNames) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

float s_columnScales[] = {
	1.0f,
	1.0f,
	1.0f,
	1.0f,
};
BB_CTASSERT(BB_ARRAYSIZE(s_columnScales) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

static void UIChangeset_CopySelectedToClipboard(p4UIChangeset *uics, p4Changeset *cs, ImGui::columnDrawData *data, bool /*extraInfo*/)
{
	u32 i;
	sb_t sb;
	sb_init(&sb);
	for(i = 0; i < uics->count; ++i) {
		p4UIChangesetEntry *e = uics->data + i;
		if(e->selected) {
			sdict_t *c = p4_find_changelist_in_changeset(cs, e->changelist);
			if(c) {
				for(u32 col = 0; col < data->numColumns; ++col) {
					if(data->columnNames[col]) {
						const changesetColumnField *field = p4.changesetColumnFields + col;
						const char *value = sdict_find_safe(c, field->key);
						if(field->time) {
							u32 time = strtou32(value);
							value = time ? Time_StringFromEpochTime(time) : "";
						}
						if(col) {
							sb_append_char(&sb, '\t');
						}
						sb_append(&sb, value);
					}
				}
				sb_append_char(&sb, '\n');
			}
		}
	}
	const char *clipboardText = sb_get(&sb);
	ImGui::SetClipboardText(clipboardText);
	sb_reset(&sb);
}

static void UIChangeset_ClearSelection(p4UIChangeset *uics)
{
	uics->lastClickIndex = ~0U;
	for(u32 i = 0; i < uics->count; ++i) {
		uics->data[i].selected = false;
	}
}

static void UIChangeset_SelectAll(p4UIChangeset *uics)
{
	uics->lastClickIndex = ~0U;
	for(u32 i = 0; i < uics->count; ++i) {
		uics->data[i].selected = true;
	}
}

static void UIChangeset_AddSelection(p4UIChangeset *uics, u32 index)
{
	p4UIChangesetEntry *e = uics->data + index;
	e->selected = true;
	uics->lastClickIndex = index;
}

static void UIChangeset_ToggleSelection(p4UIChangeset *uics, u32 index)
{
	p4UIChangesetEntry *e = uics->data + index;
	e->selected = !e->selected;
	uics->lastClickIndex = (e->selected) ? index : ~0U;
}

static void UIChangeset_HandleClick(p4UIChangeset *uics, u32 index)
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.KeyAlt || (io.KeyCtrl && io.KeyShift))
		return;

	if(io.KeyCtrl) {
		UIChangeset_ToggleSelection(uics, index);
	} else if(io.KeyShift) {
		if(uics->lastClickIndex < uics->count) {
			u32 startIndex = uics->lastClickIndex;
			u32 endIndex = index;
			uics->lastClickIndex = endIndex;
			if(endIndex < startIndex) {
				u32 tmp = endIndex;
				endIndex = startIndex;
				startIndex = tmp;
			}
			for(u32 i = startIndex; i <= endIndex; ++i) {
				uics->data[i].selected = true;
			}
		}
	} else {
		UIChangeset_ClearSelection(uics);
		UIChangeset_AddSelection(uics, index);
	}
}

void UIChangeset_Update(p4UIChangeset *uics)
{
	p4Changeset *cs = p4_find_changeset(uics->pending);
	if(!cs) {
		return;
	}

	ImGui::PushID(uics);

	if(uics->parity != cs->parity) {
		uics->parity = cs->parity;
		//p4UIChangeset old = *uics; // TODO: retain selection when refreshing changelists
		uics->count = 0;
		for(u32 i = 0; i < cs->changelists.count; ++i) {
			sdict_t *sd = cs->changelists.data + i;
			p4UIChangesetEntry e;
			e.changelist = strtou32(sdict_find_safe(sd, "change"));
			e.selected = false;
			bba_push(*uics, e);
		}
		p4_sort_uichangeset(uics);
		uics->lastClickIndex = ~0u;
	}

	b32 anyActive = false;

	uiChangesetConfig *config = uics->pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;

	float columnOffsets[5] = {};
	BB_CTASSERT(BB_ARRAYSIZE(columnOffsets) == BB_ARRAYSIZE(config->columnWidth) + 1);
	ImGui::columnDrawData data = {};
	data.columnWidths = config->columnWidth;
	data.columnScales = s_columnScales;
	data.columnOffsets = columnOffsets;
	data.columnNames = uics->pending ? s_pendingColumnNames : s_submittedColumnNames;
	data.sortDescending = &config->sortDescending;
	data.sortColumn = &config->sortColumn;
	data.numColumns = BB_ARRAYSIZE(config->columnWidth);
	for(u32 i = 0; i < BB_ARRAYSIZE(config->columnWidth); ++i) {
		if(data.columnNames[i]) {
			ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
			anyActive = anyActive || res.active;
			if(res.sortChanged) {
				p4_sort_uichangeset(uics);
				uics->lastClickIndex = ~0u;
			}
		} else {
			columnOffsets[i + 1] = columnOffsets[i];
		}
	}
	ImGui::NewLine();

	for(u32 i = 0; i < uics->count; ++i) {
		p4UIChangesetEntry *e = uics->data + i;
		sdict_t *c = p4_find_changelist_in_changeset(cs, e->changelist);
		if(c) {
			ImGui::PushSelectableColors(e->selected, true);
			ImGui::Selectable(va("###%u", e->changelist), e->selected != 0);
			ImGui::PopSelectableColors(e->selected, true);
			if(ImGui::IsItemHovered()) {
				if(ImGui::IsItemClicked()) {
					UIChangeset_HandleClick(uics, i);
				}
			}
			for(u32 col = 0; col < data.numColumns; ++col) {
				if(data.columnNames[col]) {
					const changesetColumnField *field = p4.changesetColumnFields + col;
					const char *value = sdict_find_safe(c, field->key);
					if(field->time) {
						u32 time = strtou32(value);
						value = time ? Time_StringFromEpochTime(time) : "";
					}
					ImGui::DrawColumnText(data, col, value);
				}
			}
		}
	}

	ImGuiIO &io = ImGui::GetIO();
	if(ImGui::IsKeyPressed('A') && io.KeyCtrl) {
		UIChangeset_SelectAll(uics);
	} else if(ImGui::IsKeyPressed('C') && io.KeyCtrl) {
		UIChangeset_CopySelectedToClipboard(uics, cs, &data, io.KeyShift);
	} else if(ImGui::IsKeyPressed('D') && io.KeyCtrl) {
		//UIChangeset_DiffSelectedFiles(files, cl);
	} else if(ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
		UIChangeset_ClearSelection(uics);
	} else if(key_is_pressed_this_frame(Key_F5) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
		p4_refresh_changeset(cs);
	}

	ImGui::PopID();
}
