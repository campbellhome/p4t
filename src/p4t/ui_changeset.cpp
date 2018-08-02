// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changeset.h"
#include "app.h"
#include "bb_array.h"
#include "config.h"
#include "filter.h"
#include "imgui_utils.h"
#include "keys.h"
#include "p4.h"
#include "sdict.h"
#include "str.h"
#include "time_utils.h"
#include "ui_changelist.h"
#include "va.h"

const char *s_submittedColumnNames[] = {
	"Change",
	"Date",
	NULL,
	"User",
	"Description",
};
BB_CTASSERT(BB_ARRAYSIZE(s_submittedColumnNames) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

const char *s_pendingColumnNames[] = {
	"Change",
	NULL,
	"Clientspec",
	"User",
	"Description",
};
BB_CTASSERT(BB_ARRAYSIZE(s_pendingColumnNames) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

float s_columnScales[] = {
	1.0f,
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
			sdict_t *c = p4_find_changelist_in_changeset(cs, e->changelist, sb_get(&e->client));
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

// change, time, user, client, status, changeType (public), path (submitted only?), desc, desc_oneline
static const char *s_filterKeys[] = {
	"user",
	"client",
	"desc",
};
static bool UIChangeset_PassesFilter(filterTokens *tokens, sdict_t *sd)
{
	return passes_filter_tokens(tokens, sd, s_filterKeys, BB_ARRAYSIZE(s_filterKeys)) != 0;
}

void UIChangeset_SetWindowTitle(p4UIChangeset *uics)
{
	if(uics->pending) {
		App_SetWindowTitle("Pending Changelists - p4t");
	} else {
		App_SetWindowTitle("Submitted Changelists - p4t");
	}
}

void UIChangeset_Update(p4UIChangeset *uics)
{
	ImGui::PushID(uics);

	b32 anyActive = false;

	ImGui::TextUnformatted("Pending:");
	ImGui::SameLine();
	if(ImGui::Checkbox("###pending", &uics->pending)) {
		uics->parity = 0;
	}

	ImGui::SameLine();
	ImGui::TextUnformatted("  User:");
	ImGui::SameLine();
	ImGui::PushItemWidth(96.0f * g_config.dpiScale);
	if(ImGui::InputText("###user", &uics->user, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
		uics->parity = 0;
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::TextUnformatted("  Client:");
	ImGui::SameLine();
	ImGui::PushItemWidth(96.0f * g_config.dpiScale);
	if(ImGui::InputText("###clientspec", &uics->clientspec, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
		uics->parity = 0;
	}
	ImGui::PopItemWidth();

	ImGui::TextUnformatted("Filter:");
	ImGui::SameLine();
	if(ImGui::Checkbox("###filter", &uics->filterEnabled)) {
		uics->parity = 0;
	}
	ImGui::SameLine();
	if(ImGui::InputText("###filterInput", &uics->filterInput, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
		sb_reset(&uics->filter);
		sb_append(&uics->filter, sb_get(&uics->filterInput));
		uics->filterEnabled = true;
		uics->parity = 0;
	}
	if(ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::TextUnformatted("Filter: +<Category:>RequireThis -<Category:>WithoutThis <Category:>AtLeast <Category:>OneOfTheseOrRequireThis");
		ImGui::Separator();
		ImGui::TextUnformatted("Categories: user, client, desc");
		ImGui::TextUnformatted("Category can be omitted, and the filter component will be applied to all categories.");
		ImGui::Separator();
		ImGui::TextUnformatted("Examples:");
		ImGui::TextUnformatted("-user:autointegrator");
		ImGui::TextUnformatted("user:matt user:nickj user:zach");
		ImGui::TextUnformatted("desc:matchmaking desc:online desc:network");
		ImGui::EndTooltip();
	}

	p4Changeset *cs = p4_find_changeset(uics->pending);
	if(!cs) {
		ImGui::PopID();
		return;
	}

	if(uics->parity != cs->parity) {
		uics->parity = cs->parity;
		//p4UIChangeset old = *uics; // TODO: retain selection when refreshing changelists

		filterTokens tokens = {};
		if(uics->filterEnabled) {
			build_filter_tokens(&tokens, sb_get(&uics->filter));
		}
		add_filter_token(&tokens, "user", sb_get(&uics->user));
		add_filter_token(&tokens, "client", sb_get(&uics->clientspec));

		for(u32 i = 0; i < uics->count; ++i) {
			p4_reset_uichangesetentry(uics->data + i);
		}
		uics->count = 0;
		for(u32 i = 0; i < cs->changelists.count; ++i) {
			sdict_t *sd = cs->changelists.data + i;
			if(UIChangeset_PassesFilter(&tokens, sd)) {
				p4UIChangesetEntry e = {};
				e.changelist = strtou32(sdict_find_safe(sd, "change"));
				e.selected = false;
				sb_append(&e.client, sdict_find_safe(sd, "client"));
				bba_push(*uics, e);
			}
		}
		p4_sort_uichangeset(uics);
		uics->lastClickIndex = ~0u;
		reset_filter_tokens(&tokens);
		UIChangeset_SetWindowTitle(uics);
	}

	uiChangesetConfig *config = uics->pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;

	float columnOffsets[6] = {};
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
		sdict_t *c = p4_find_changelist_in_changeset(cs, e->changelist, sb_get(&e->client));
		if(c) {
			b32 expanded = ImGui::TreeNode(va("###node%u%s", e->changelist, sb_get(&e->client)));
			if(expanded) {
				ImGui::TreePop();
			}
			ImGui::SameLine();
			ImGui::PushSelectableColors(e->selected, uics->active);
			ImGui::Selectable(va("###%u%s", e->changelist, sb_get(&e->client)), e->selected != 0);
			ImGui::PopSelectableColors(e->selected, uics->active);
			if(ImGui::IsItemActive()) {
				anyActive = true;
			}
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
			if(expanded) {
				p4Changelist *cl = e->changelist ? p4_find_changelist(e->changelist) : p4_find_default_changelist(sb_get(&e->client));
				if(cl) {
					if(e->parity != cl->parity) {
						e->parity = cl->parity;
						p4_build_changelist_files(cl, &e->normalFiles, &e->shelvedFiles);
					}
					UIChangelist_DrawFilesAndHeaders(cl, &e->normalFiles, &e->shelvedFiles, false, 20.0f * g_config.dpiScale);
				} else {
					if(!e->described) {
						e->described = true;
						if(e->changelist) {
							p4_describe_changelist(e->changelist);
						} else {
							p4_describe_default_changelist(sb_get(&e->client));
						}
					}
				}
			}
		}
	}

	if(anyActive) {
		uics->active = true;
	} else if(ImGui::IsAnyItemActive()) {
		uics->active = false;
	}

	if(uics->active) {
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
	}

	ImGui::PopID();
}
