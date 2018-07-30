// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changeset.h"
#include "config.h"
#include "imgui_utils.h"
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

void UIChangeset_Update(p4Changeset *cs)
{
	ImGui::PushID(cs);

	b32 anyActive = false;

	uiChangesetConfig *config = cs->pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;

	float columnOffsets[5] = {};
	BB_CTASSERT(BB_ARRAYSIZE(columnOffsets) == BB_ARRAYSIZE(config->columnWidth) + 1);
	ImGui::columnDrawData data = {};
	data.columnWidths = config->columnWidth;
	data.columnScales = s_columnScales;
	data.columnOffsets = columnOffsets;
	data.columnNames = cs->pending ? s_pendingColumnNames : s_submittedColumnNames;
	data.sortDescending = &config->sortDescending;
	data.sortColumn = &config->sortColumn;
	data.numColumns = BB_ARRAYSIZE(config->columnWidth);
	for(u32 i = 0; i < BB_ARRAYSIZE(config->columnWidth); ++i) {
		if(data.columnNames[i]) {
			ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
			anyActive = anyActive || res.active;
			if(res.sortChanged) {
				p4_sort_changeset(cs);
			}
		} else {
			columnOffsets[i + 1] = columnOffsets[i];
		}
	}
	ImGui::NewLine();

	for(u32 i = 0; i < cs->changelists.count; ++i) {
		sdict_t *c = cs->changelists.data + i;
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

	ImGui::PopID();
}
