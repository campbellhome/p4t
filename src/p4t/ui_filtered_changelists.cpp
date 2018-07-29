// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_filtered_changelists.h"
#include "config.h"
#include "imgui_utils.h"
#include "p4.h"
#include "sdict.h"
#include "va.h"

void UIFilteredChangelists_Reset(filteredChangelists *f)
{
	sb_reset(&f->user);
	sb_reset(&f->clientspec);
	sb_reset(&f->filter);
}

const char *s_columnNames[] = {
	"Changelist",
	"Date",
	"User",
	"Description",
};
BB_CTASSERT(BB_ARRAYSIZE(s_columnNames) == BB_ARRAYSIZE(g_config.uiChangelistList.columnWidth));

float s_columnScales[] = {
	1.0f,
	1.0f,
	1.0f,
	1.0f,
};
BB_CTASSERT(BB_ARRAYSIZE(s_columnScales) == BB_ARRAYSIZE(g_config.uiChangelistList.columnWidth));

void UIFilteredChangelists_Update(filteredChangelists *f)
{
	ImGui::PushID(f);

	if(ImGui::Button("refresh", ImGui::kButton_Normal)) {
		p4_changes(f->pending);
	}

	b32 anyActive = false;

	float columnOffsets[5] = {};
	BB_CTASSERT(BB_ARRAYSIZE(columnOffsets) == BB_ARRAYSIZE(g_config.uiChangelistList.columnWidth) + 1);
	ImGui::columnDrawData data = {};
	data.columnWidths = g_config.uiChangelistList.columnWidth;
	data.columnScales = s_columnScales;
	data.columnOffsets = columnOffsets;
	data.columnNames = s_columnNames;
	data.sortDescending = &g_config.uiChangelistList.sortDescending;
	data.sortColumn = &g_config.uiChangelistList.sortColumn;
	data.numColumns = BB_ARRAYSIZE(g_config.uiChangelistList.columnWidth);
	for(u32 i = 0; i < BB_ARRAYSIZE(g_config.uiChangelistList.columnWidth); ++i) {
		ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
		anyActive = anyActive || res.active;
		if(res.sortChanged) {
		}
	}
	ImGui::NewLine();

	p4ChangelistShorts *s = (f->pending) ? &p4.pendingChangelistShorts : &p4.submittedChangelistShorts;
	for(u32 i = 0; i < s->count; ++i) {
		p4ChangelistShort *c = s->data + i;
		ImGui::DrawColumnText(data, 0, sdict_find_safe(&c->dict, "change"));
		ImGui::DrawColumnText(data, 1, sdict_find_safe(&c->dict, "user"));
		ImGui::DrawColumnText(data, 2, sdict_find_safe(&c->dict, "client"));
		ImGui::DrawColumnText(data, 3, sdict_find_safe(&c->dict, "desc_oneline"));
	}

	ImGui::PopID();
}
