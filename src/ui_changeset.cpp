// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_changeset.h"
#include "appdata.h"
#include "bb_array.h"
#include "config.h"
#include "file_utils.h"
#include "filter.h"
#include "imgui_utils.h"
#include "keys.h"
#include "p4.h"
#include "sdict.h"
#include "str.h"
#include "time_utils.h"
#include "ui_changelist.h"
#include "ui_icons.h"
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

static sb_t UIChangeset_SingleLineFromMultiline(const char *src)
{
	sb_t sb = { 0 };
	char ch;
	while((ch = *src++) != '\0') {
		if(ch == '\r') {
			// do nothing
		} else if(ch == '\n' || ch == '\t') {
			sb_append_char(&sb, ' ');
		} else {
			sb_append_char(&sb, ch);
		}
	}
	return sb;
}

static u32 UIChangeset_CountSelectedChangelists(p4UIChangeset *uics)
{
	u32 count = 0;
	for(u32 i = 0; i < uics->sorted.count; ++i) {
		p4UIChangesetSortKey *s = uics->sorted.data + i;
		p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
		if(e->selected) {
			++count;
		}
	}
	return count;
}

static sb_t UIChangeset_CopySelectedToBuffer(p4UIChangeset *uics, p4Changeset *cs, ImGui::columnDrawData *data, bool /*extraInfo*/)
{
	sb_t sb = { BB_EMPTY_INITIALIZER };
	for(u32 i = 0; i < uics->sorted.count; ++i) {
		p4UIChangesetSortKey *s = uics->sorted.data + i;
		p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
		if(e->selected) {
			sdict_t *c = cs->changelists.data + e->changelistIndex;
			if(c) {
				for(u32 col = 0; col < data->numColumns; ++col) {
					if(data->columnNames[col]) {
						const changesetColumnField *field = p4.changesetColumnFields + col;
						const char *value = sdict_find_safe(c, field->key);
						if(field->type == kChangesetColumn_Time) {
							u32 time = strtou32(value);
							value = time ? Time_StringFromEpochTime(time) : "";
						}
						sb_t singleLine = { 0 };
						if(field->type == kChangesetColumn_TextMultiline) {
							singleLine = UIChangeset_SingleLineFromMultiline(value);
							value = sb_get(&singleLine);
						}
						if(col) {
							sb_append_char(&sb, '\t');
						}
						sb_append(&sb, value);
						sb_reset(&singleLine);
					}
				}
				sb_append_char(&sb, '\n');
			}
		}
	}
	return sb;
}

static void UIChangeset_CopySelectedToClipboard(p4UIChangeset *uics, p4Changeset *cs, ImGui::columnDrawData *data, bool extraInfo)
{
	sb_t sb = UIChangeset_CopySelectedToBuffer(uics, cs, data, extraInfo);
	const char *clipboardText = sb_get(&sb);
	ImGui::SetClipboardText(clipboardText);
	sb_reset(&sb);
}

// #TODO: move this
void OpenFileInExplorer(const char *path)
{
	sb_t sb = sb_from_va("C:\\Windows\\explorer.exe \"%s\"", path);
	STARTUPINFOA startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION procInfo;
	memset(&procInfo, 0, sizeof(procInfo));
	BOOL ret = CreateProcessA(nullptr, sb.data, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupInfo, &procInfo);
	if(!ret) {
		BB_ERROR("View", "Failed to create process for '%s'", sb.data);
	} else {
		BB_LOG("View", "Created process for '%s'", sb.data);
	}
	CloseHandle(procInfo.hThread);
	CloseHandle(procInfo.hProcess);
	sb_reset(&sb);
}

static void UIChangeset_CopySelectedToFile(p4UIChangeset *uics, p4Changeset *cs, ImGui::columnDrawData *data, bool extraInfo)
{
	sb_t sb = UIChangeset_CopySelectedToBuffer(uics, cs, data, extraInfo);
	fileData_t fileData = { BB_EMPTY_INITIALIZER };
	fileData.buffer = (void *)sb_get(&sb);
	fileData.bufferSize = sb_len(&sb);
	sb_t tempPath = appdata_get("p4t");
	sb_append(&tempPath, "\\p4t_selected_changesets.txt");
	fileData_write(sb_get(&tempPath), NULL, fileData);
	OpenFileInExplorer(sb_get(&tempPath));
	sb_reset(&tempPath);
	sb_reset(&sb);
}

static void UIChangeset_ClearSelection(p4UIChangeset *uics)
{
	uics->lastClickIndex = ~0U;
	for(u32 i = 0; i < uics->entries.count; ++i) {
		uics->entries.data[i].selected = false;
	}
}

static void UIChangeset_SelectAll(p4UIChangeset *uics)
{
	uics->lastClickIndex = ~0U;
	for(u32 i = 0; i < uics->entries.count; ++i) {
		uics->entries.data[i].selected = true;
	}
}

static void UIChangeset_AddSelection(p4UIChangeset *uics, u32 index)
{
	p4UIChangesetSortKey *s = uics->sorted.data + index;
	p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
	e->selected = true;
	uics->lastClickIndex = index;
}

static void UIChangeset_ToggleSelection(p4UIChangeset *uics, u32 index)
{
	p4UIChangesetSortKey *s = uics->sorted.data + index;
	p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
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
		if(uics->lastClickIndex < uics->entries.count) {
			u32 startIndex = uics->lastClickIndex;
			u32 endIndex = index;
			uics->lastClickIndex = endIndex;
			if(endIndex < startIndex) {
				u32 tmp = endIndex;
				endIndex = startIndex;
				startIndex = tmp;
			}
			for(u32 i = startIndex; i <= endIndex; ++i) {
				p4UIChangesetSortKey *s = uics->sorted.data + i;
				p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
				e->selected = true;
			}
		}
	} else {
		UIChangeset_ClearSelection(uics);
		UIChangeset_AddSelection(uics, index);
	}
}

// change, time, user, client, status, changeType (public), path (submitted only?), desc
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
	BB_UNUSED(uics);
#if 0 // #TODO
	if(uics->config.pending) {
		App_SetWindowTitle("Pending Changelists - p4t");
	} else {
		App_SetWindowTitle("Submitted Changelists - p4t");
	}
#endif
}

struct sdictComboData {
	sdicts *sds;
	const char *key;
	const char *option0;
	const char *option1;
};

static bool sdictComboEntry(void *_data, int idx, const char **out_text)
{
	sdictComboData *data = (sdictComboData *)_data;
	if(idx == 0) {
		*out_text = data->option0;
		return true;
	} else if(idx == 1) {
		*out_text = data->option1;
		return true;
	} else {
		idx -= 2;
		sdicts *sds = data->sds;
		const char *key = data->key;
		if(idx >= 0 && (u32)idx < sds->count) {
			sdict_t *sd = sds->data + idx;
			*out_text = sdict_find_safe(sd, key);
			return true;
		}
	}
	return false;
}

static bool sdictCombo(const char *label, sb_t *current, sdicts *sds, const char *key, const char *option0, const char *option1)
{
	const char *currentText = sb_get(current);
	int currentIndex = -1;
	if(!_stricmp(currentText, option0)) {
		currentIndex = 0;
	} else if(!_stricmp(currentText, option1)) {
		currentIndex = 1;
	} else {
		for(u32 i = 0; i < sds->count; ++i) {
			sdict_t *sd = sds->data + i;
			const char *value = sdict_find_safe(sd, key);
			if(!_stricmp(currentText, value)) {
				currentIndex = (int)i + 2;
				break;
			}
		}
	}
	sdictComboData data = { sds, key, option0, option1 };
	bool ret = ImGui::Combo(label, &currentIndex, sdictComboEntry, &data, (int)sds->count + 2);
	if(ret) {
		const char *out = nullptr;
		if(currentIndex == 0) {
			out = option0;
		} else if(currentIndex == 1) {
			out = option1;
		} else {
			currentIndex -= 2;
			if(currentIndex >= 0 && (u32)currentIndex < sds->count) {
				sdict_t *sd = sds->data + currentIndex;
				out = sdict_find_safe(sd, key);
			}
		}
		if(out) {
			sb_reset(current);
			sb_append(current, out);
		}
	}
	return ret;
}

static bool sbsComboEntry(void *_data, int idx, const char **out_text)
{
	sbs_t *sbs = (sbs_t *)_data;
	if(idx >= 0 && (u32)idx < sbs->count) {
		sb_t *sb = sbs->data + idx;
		*out_text = sb_get(sb);
		return true;
	}
	return false;
}
static bool sbsCombo(const char *label, sb_t *current, sbs_t *sbs)
{
	const char *currentText = sb_get(current);
	int currentIndex = -1;
	for(u32 i = 0; i < sbs->count; ++i) {
		sb_t *sb = sbs->data + i;
		const char *value = sb_get(sb);
		if(!_stricmp(currentText, value)) {
			currentIndex = (int)i;
			break;
		}
	}
	bool ret = ImGui::Combo(label, &currentIndex, sbsComboEntry, sbs, (int)sbs->count);
	if(ret) {
		const char *out = nullptr;
		if(currentIndex >= 0 && (u32)currentIndex < sbs->count) {
			sb_t *sb = sbs->data + currentIndex;
			out = sb_get(sb);
		}
		if(out) {
			sb_reset(current);
			sb_append(current, out);
		}
	}
	return ret;
}

struct changesetDebug {
	float startY;
	float requiredEndY;
	u32 visibleEndIndex;
	float endY;
	bool drawFromStart;
	bool showChangesetOptimizations;
	u8 pad[6];
};
static changesetDebug s_debug;
void UIChangeset_Menu()
{
	ImGui::Checkbox("DEBUG Changeset Optimizations", &s_debug.showChangesetOptimizations);
}

static bool UIChangeset_TryAddChangelist(p4UIChangeset *uics, p4Changeset *cs, u32 index)
{
	sdict_t *sd = cs->changelists.data + index;
	if(UIChangeset_PassesFilter(&uics->autoFilterTokens, sd) && UIChangeset_PassesFilter(&uics->manualFilterTokens, sd)) {
		p4UIChangesetEntry e = {};
		e.changelistNumber = strtou32(sdict_find_safe(sd, "change"));
		e.changelistIndex = index;
		e.selected = false;
		sb_append(&e.client, sdict_find_safe(sd, "client"));
		if(bba_add_noclear(uics->entries, 1)) {
			bba_last(uics->entries) = e;
			return true;
		}
	}
	return false;
}

void UIChangeset_Update(p4UIChangeset *uics)
{
	ImGui::PushID(uics);

	b32 anyActive = false;
	b32 anyChangelistFileActive = false;
	b32 forceRebuild = false;

	ImGui::TextUnformatted("Pending:");
	ImGui::SameLine();
	if(ImGui::Checkbox("###pending", &uics->config.pending)) {
		uics->parity = 0;
		forceRebuild = true;
	}

	ImGui::SameLine();
	ImGui::TextUnformatted("  User:");
	ImGui::SameLine();
	ImGui::PushItemWidth(140.0f * g_config.dpiScale);
	if(sdictCombo("###user", &uics->config.user, &p4.allUsers, "User", "", "Current User")) {
		uics->parity = 0;
	}
	ImGui::PopItemWidth();

	const char *user = sb_get(&uics->config.user);
	if(*user) {
		if(!strcmp(user, "Current User")) {
			user = sdict_find_safe(&p4.info, "userName");
		}
	}
	sbs_t clientspecs = {};
	sb_t emptyClientspec = {};
	sb_t currentClientspec = {};
	sb_append(&currentClientspec, "Current Client");
	bba_push(clientspecs, emptyClientspec);
	bba_push(clientspecs, currentClientspec);
	for(u32 i = 0; i < p4.allClients.count; ++i) {
		sdict_t *sd = p4.allClients.data + i;
		const char *clientUser = sdict_find_safe(sd, "Owner");
		if(*user && _stricmp(user, clientUser))
			continue;
		sb_t clientspec = {};
		sb_append(&clientspec, sdict_find_safe(sd, "client"));
		bba_push(clientspecs, clientspec);
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("  Client:");
	ImGui::SameLine();
	ImGui::PushItemWidth(140.0f * g_config.dpiScale);
	if(sbsCombo("###clientspec", &uics->config.clientspec, &clientspecs)) {
		uics->parity = 0;
	}
	ImGui::PopItemWidth();
	sbs_reset(&clientspecs);

	if(s_debug.showChangesetOptimizations) {
		ImGui::SameLine();
		ImGui::Checkbox("drawAll", &s_debug.drawFromStart);
		ImGui::SameLine();
		u32 startCL = (uics->lastStartIndex < uics->sorted.count) ? uics->entries.data[uics->sorted.data[uics->lastStartIndex].entryIndex].changelistNumber : 0;
		u32 endCL = (s_debug.visibleEndIndex < uics->sorted.count) ? uics->entries.data[uics->sorted.data[s_debug.visibleEndIndex].entryIndex].changelistNumber : 0;
		ImGui::Text("%.0f/%.0f(%.0f) range:%u(%u)-%u(%u) numValid:%u",
		            s_debug.startY, s_debug.requiredEndY, s_debug.endY,
		            uics->lastStartIndex, startCL,
		            s_debug.visibleEndIndex, endCL,
		            uics->numValidStartY, uics->lastStartIndex);
	}

	ImGui::TextUnformatted("Filter:");
	ImGui::SameLine();
	if(ImGui::Checkbox("###filter", &uics->config.filterEnabled)) {
		uics->parity = 0;
	}
	ImGui::SameLine();
	if(ImGui::InputText("###filterInput", &uics->config.filterInput, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
		sb_reset(&uics->config.filter);
		sb_append(&uics->config.filter, sb_get(&uics->config.filterInput));
		uics->config.filterEnabled = true;
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

	p4Changeset *cs = p4_find_or_add_changeset(uics->config.pending);
	if(!cs) {
		ImGui::PopID();
		return;
	}

	if(!cs->refreshed) {
		p4_refresh_changeset(cs);
	}

	u32 paritySort = cs->parity;

	if(uics->parity != cs->parity || forceRebuild) {
		uics->parity = cs->parity;
		uics->numChangelistsAppended = cs->changelists.count;
		paritySort = 0;
		BB_LOG("changeset::rebuild_changeset", "rebuild tokens");

		//p4UIChangeset old = *uics; // TODO: retain selection when refreshing changelists

		if(uics->config.filterEnabled) {
			build_filter_tokens(&uics->manualFilterTokens, sb_get(&uics->config.filter));
		} else {
			reset_filter_tokens(&uics->manualFilterTokens);
		}

		reset_filter_tokens(&uics->autoFilterTokens);
		if(*user) {
			if(filterToken *t = add_filter_token(&uics->autoFilterTokens, "user", user)) {
				t->required = true;
				t->prohibited = false;
				t->exact = true;
			}
		}
		const char *clientspec = sb_get(&uics->config.clientspec);
		if(*clientspec) {
			if(!strcmp(clientspec, "Current Client")) {
				clientspec = p4_clientspec();
			}
			if(filterToken *t = add_filter_token(&uics->autoFilterTokens, "client", clientspec)) {
				t->required = true;
				t->prohibited = false;
				t->exact = true;
			}
		}

		BB_LOG("changeset::rebuild_changeset", "reset old entries");
		for(u32 i = 0; i < uics->entries.count; ++i) {
			p4_reset_uichangesetentry(uics->entries.data + i);
		}
		uics->entries.count = 0;
		uics->sorted.count = 0;
		BB_LOG("changeset::rebuild_changeset", "adding new entries");
		for(u32 i = 0; i < cs->changelists.count; ++i) {
			sdict_t *sd = cs->changelists.data + i;
			if(UIChangeset_PassesFilter(&uics->autoFilterTokens, sd) && UIChangeset_PassesFilter(&uics->manualFilterTokens, sd)) {
				p4UIChangesetEntry e = {};
				e.changelistNumber = strtou32(sdict_find_safe(sd, "change"));
				e.changelistIndex = i;
				e.selected = false;
				sb_append(&e.client, sdict_find_safe(sd, "client"));
				if(bba_add_noclear(uics->entries, 1)) {
					bba_last(uics->entries) = e;
					p4UIChangesetSortKey s = {};
					s.entryIndex = uics->sorted.count;
					bba_push(uics->sorted, s);
				}
			}
		}
		UIChangeset_SetWindowTitle(uics);
		BB_LOG("changeset::rebuild_changeset", "done");
	}

	uiChangesetConfig *config = uics->config.pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;

	ImGui::columnDrawData data = {};
	float columnOffsets[6] = {};
	BB_CTASSERT(BB_ARRAYSIZE(columnOffsets) == BB_ARRAYSIZE(config->columnWidth) + 1);
	data.columnWidths = config->columnWidth;
	data.columnOffsets = columnOffsets;
	data.columnNames = uics->config.pending ? s_pendingColumnNames : s_submittedColumnNames;
	data.sortDescending = &config->sortDescending;
	data.sortColumn = &config->sortColumn;
	data.numColumns = BB_ARRAYSIZE(config->columnWidth);
	for(u32 i = 0; i < BB_ARRAYSIZE(config->columnWidth); ++i) {
		if(data.columnNames[i]) {
			ImGui::columnDrawResult res = ImGui::DrawColumnHeader(data, i);
			anyActive = anyActive || res.active;
			if(res.sortChanged) {
				paritySort = 0;
			}
		} else {
			columnOffsets[i + 1] = columnOffsets[i];
		}
	}

	if(uics->numChangelistsAppended < cs->changelists.count) {
		BB_LOG("changeset::append_changeset", "start append");
		for(u32 i = uics->numChangelistsAppended; i < cs->changelists.count; ++i) {
			if(UIChangeset_TryAddChangelist(uics, cs, i)) {
				paritySort = 0;
			}
		}
		uics->numChangelistsAppended = cs->changelists.count;
		BB_LOG("changeset::append_changeset", "end append");
	}

	if(paritySort != cs->parity) {
		paritySort = cs->parity;
		BB_LOG("changeset::sort_changeset", "start sort");
		p4_sort_uichangeset(uics);
		uics->lastClickIndex = ~0u;
		uics->numValidStartY = 0;
		uics->lastStartIndex = 0;
		uics->lastStartY = 0.0f;
		BB_LOG("changeset::sort_changeset", "end sort");
	}

	ImGui::NewLine();

	if(ImGui::BeginChild("##changelists", ImVec2(0, 0), false, ImGuiWindowFlags_None)) {
		b32 debug = uics->numValidStartY == 0 && uics->entries.count > 0;
		if(debug) {
			BB_LOG("changeset::rebuild_offsets", "start rebuild_offsets");
		}

		u32 startIndex = 0;
		float startY = 0.0f;
		const float scrollY = ImGui::GetScrollY();
		s_debug.startY = scrollY;
		if(!s_debug.drawFromStart) {
			startIndex = uics->lastStartIndex;
			startY = uics->lastStartY;
			while(startY < scrollY) {
				if(startIndex + 1 >= uics->sorted.count)
					break;
				p4UIChangesetSortKey *s = uics->sorted.data + startIndex + 1;
				p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
				if(e->startY < scrollY) {
					++startIndex;
					startY = e->startY;
				} else {
					break;
				}
			}
			while(startY > scrollY) {
				if(!startIndex)
					break;
				p4UIChangesetSortKey *s = uics->sorted.data + startIndex;
				p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
				if(e->startY < scrollY)
					break;
				--startIndex;
				s = uics->sorted.data + startIndex;
				e = uics->entries.data + s->entryIndex;
				startY = e->startY;
			}
		}
		uics->lastStartIndex = startIndex;
		uics->lastStartY = startY;

		float visibleEndY = ImGui::GetWindowHeight() + ImGui::GetScrollY();

		const ImGuiStyle &style = ImGui::GetStyle();
		const float fontSize = ImGui::GetFontSize();

		if(uics->numValidStartY < startIndex) {
			BB_LOG("changeset::rebuild_offsets_pre", "start rebuild_offsets_pre");
			float y = 0.0f;
			for(u32 i = 0; i < startIndex; ++i) {
				p4UIChangesetSortKey *s = uics->sorted.data + i;
				p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
				e->startY = y;
				if(!e->height) {
					e->height = fontSize + style.FramePadding.y * 2;
				}
				y += e->height + style.ItemSpacing.y;
			}
			uics->numValidStartY = startIndex;
			BB_LOG("changeset::rebuild_offsets_pre", "start rebuild_offsets_pre");
		}

		if(startY) {
			ImGui::Button("##spacerstart", ImVec2(0, startY - ImGui::GetStyle().ItemSpacing.y));
		}
		for(u32 i = startIndex; i < uics->sorted.count; ++i) {
			p4UIChangesetSortKey *s = uics->sorted.data + i;
			p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
			e->startY = ImGui::GetCursorPosY();
			uics->numValidStartY = BB_MAX(uics->numValidStartY, i);
			sdict_t *c = cs->changelists.data + e->changelistIndex;
			if(c) {
				b32 expanded = ImGui::TreeNode(va("###node%u%s", e->changelistNumber, sb_get(&e->client)));
				if(expanded) {
					ImGui::TreePop();
				}
				ImGui::SameLine();
				ImGui::PushSelectableColors(e->selected, ImGui::IsActiveSelectables(uics));
				ImGui::Selectable(va("###%u%s", e->changelistNumber, sb_get(&e->client)), e->selected != 0);
				ImGui::PopSelectableColors(e->selected, ImGui::IsActiveSelectables(uics));
				if(ImGui::IsItemActive()) {
					anyActive = true;
				}
				if(ImGui::IsItemHovered()) {
					if(ImGui::IsItemClicked()) {
						UIChangeset_HandleClick(uics, i);
					}
				}
				if(ImGui::BeginContextMenu(va("context_%u_%d", uics->id, i))) {
					BB_LOG("popup", "context_%u_%d", uics->id, i);
					anyActive = true;
					UIChangeset_AddSelection(uics, i);

					u32 selected = UIChangeset_CountSelectedChangelists(uics);
					if(ImGui::MenuItem(va("Copy %d %s to clipboard", selected, selected == 1 ? "changelist" : "changelists"))) {
						ImGuiIO &io = ImGui::GetIO();
						UIChangeset_CopySelectedToClipboard(uics, cs, &data, io.KeyShift);
					}
					if(ImGui::MenuItem(va("Copy %d %s to file", selected, selected == 1 ? "changelist" : "changelists"))) {
						ImGuiIO &io = ImGui::GetIO();
						UIChangeset_CopySelectedToFile(uics, cs, &data, io.KeyShift);
					}
					//if(ImGui::MenuItem(va("Diff %d %s against depot", selected, selected == 1 ? "changelist" : "changelists"))) {
					//	UIChangelist_DiffSelected(cltype, files, cl);
					//}

					ImGui::EndContextMenu();
				}

				ImGui::SameLine();
				float iconWidth = ImGui::CalcTextSize(ICON_CHANGELIST).x;
				ImVec2 pos = ImGui::GetIconPosForText();
				pos.x -= iconWidth * 0.5f;
				ImColor iconColor;
				switch(p4_get_changelist_type(c)) {
				case kChangelistType_PendingLocal:
					iconColor = COLOR_PENDING_CHANGELIST_LOCAL;
					break;
				case kChangelistType_PendingOther:
					iconColor = COLOR_PENDING_CHANGELIST_OTHER;
					break;
				case kChangelistType_Submitted:
					iconColor = COLOR_SUBMITTED_CHANGELIST;
					break;
				}
				//ImGui::DrawIconAtPos(pos, ICON_CHANGELIST, iconColor);
				ImGui::TextColored(iconColor, "%s", ICON_CHANGELIST);
				//ImGui::SameLine(1.0f * g_config.dpiScale, 0.0f);

				ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
				ImGui::GetStyle().ItemSpacing.x = -1 * ImGui::CalcTextSize(ICON_CHANGELIST).x * 0.5f; // -5.0f * g_config.dpiScale;
				ImGui::SameLine();
				ImGui::GetStyle().ItemSpacing = spacing;

				for(u32 col = 0; col < data.numColumns; ++col) {
					if(data.columnNames[col]) {
						const changesetColumnField *field = p4.changesetColumnFields + col;
						const char *value = sdict_find_safe(c, field->key);
						if(field->type == kChangesetColumn_Time) {
							u32 time = strtou32(value);
							value = time ? Time_StringFromEpochTime(time) : "";
						}
						sb_t singleLine = { 0 };
						if(field->type == kChangesetColumn_TextMultiline) {
							singleLine = UIChangeset_SingleLineFromMultiline(value);
							value = sb_get(&singleLine);
						}
						if(!col) {
							value = va("  %s", value);
						}
						if(col == data.numColumns - 1) {
							ImGui::SameLine(data.columnOffsets[col]);
							ImGui::TextUnformatted(value);
						} else {
							ImGui::DrawColumnText(data, col, value);
						}
						sb_reset(&singleLine);
					}
				}
				if(expanded) {
					p4Changelist *cl = e->changelistNumber ? p4_find_changelist(e->changelistNumber) : p4_find_default_changelist(sb_get(&e->client));
					if(cl) {
						if(e->parity != cl->parity) {
							e->parity = cl->parity;
							p4_build_changelist_files(cl, &e->normalFiles, &e->shelvedFiles);
						}

						UIChangelist_DrawFilesNoColumns(&e->normalFiles, cl, 30.0f * g_config.dpiScale);
						if(ImGui::IsActiveSelectables(&e->normalFiles)) {
							anyChangelistFileActive = true;
						}
						if(e->shelvedFiles.count) {
							ImGui::TextUnformatted("");
							ImGui::SameLine(0.0f, 20.0f * g_config.dpiScale);
							const char *title = va("Shelved File%s: %u", e->shelvedFiles.count == 1 ? "" : "s", e->shelvedFiles.count);
							bool shelvedOpenByDefault = false;
							bool shelvedExpanded = ImGui::TreeNodeEx(va("%s###shelved%u%s", title, cl->number, sdict_find_safe(&cl->normal, "client")), shelvedOpenByDefault ? ImGuiTreeNodeFlags_DefaultOpen : 0);
							if(shelvedExpanded) {
								UIChangelist_DrawFilesNoColumns(&e->shelvedFiles, cl, 40.0f * g_config.dpiScale);
								if(ImGui::IsActiveSelectables(&e->shelvedFiles)) {
									anyChangelistFileActive = true;
								}
								ImGui::TreePop();
							}
						}
					} else {
						if(!e->described) {
							e->described = true;
							if(e->changelistNumber) {
								p4_describe_changelist(e->changelistNumber);
							} else {
								p4_describe_default_changelist(sb_get(&e->client));
							}
						}
					}
				}
			}
			e->height = ImGui::GetCursorPosY() - e->startY;
			s_debug.visibleEndIndex = i;
			if(!s_debug.drawFromStart && e->startY > visibleEndY /*&& uics->numValidStartY == uics->sorted.count - 1*/)
				break;
		}

		if(uics->numValidStartY < uics->sorted.count && uics->numValidStartY > 0) {
			BB_LOG("changeset::rebuild_offsets_post", "start rebuild_offsets_post");
			p4UIChangesetSortKey *lastS = uics->sorted.data + uics->numValidStartY - 1;
			p4UIChangesetEntry *lastE = uics->entries.data + lastS->entryIndex;
			float y = lastE->startY + lastE->height + style.ItemSpacing.y;
			for(u32 i = uics->numValidStartY; i < uics->sorted.count; ++i) {
				p4UIChangesetSortKey *s = uics->sorted.data + i;
				p4UIChangesetEntry *e = uics->entries.data + s->entryIndex;
				e->startY = y;
				if(!e->height) {
					e->height = fontSize + style.FramePadding.y * 2;
				}
				y += e->height + style.ItemSpacing.y;
			}
			uics->numValidStartY = uics->sorted.count;
			BB_LOG("changeset::rebuild_offsets_post", "start rebuild_offsets_post");
		}

		if(uics->sorted.count) {
			p4UIChangesetEntry &e = uics->entries.data[bba_last(uics->sorted).entryIndex];
			float requiredY = e.startY + e.height;
			float curY = ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y;
			if(requiredY > curY) {
				ImGui::Button("##spacerend", ImVec2(0, requiredY - curY));
			}
			s_debug.requiredEndY = requiredY;
			s_debug.endY = ImGui::GetCursorPosY();
		}
		if(debug) {
			BB_LOG("changeset::rebuild_offsets", "end rebuild_offsets");
		}
	}
	ImGui::EndChild();

	if(anyActive) {
		ImGui::SetActiveSelectables(uics);
	} else if(ImGui::IsAnyItemActive() && ImGui::IsActiveSelectables(uics)) {
		ImGui::SetActiveSelectables(nullptr);
	}

	ImGuiIO &io = ImGui::GetIO();
	if(ImGui::IsActiveSelectables(uics)) {
		if(ImGui::IsKeyPressed('A') && io.KeyCtrl) {
			UIChangeset_SelectAll(uics);
		} else if(ImGui::IsKeyPressed('C') && io.KeyCtrl) {
			UIChangeset_CopySelectedToClipboard(uics, cs, &data, io.KeyShift);
		} else if(ImGui::IsKeyPressed('D') && io.KeyCtrl) {
			//UIChangeset_DiffSelectedFiles(files, cl);
		} else if(ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
			UIChangeset_ClearSelection(uics);
		}
	} else if(anyChangelistFileActive || !ImGui::IsAnyItemActive()) {
		if(ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
			UIChangeset_ClearSelection(uics);
		}
	}

	if(key_is_pressed_this_frame(Key_F5) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
		p4_request_newer_changes(cs, g_config.p4.changelistBlockSize);
	}

	ImGui::PopID();
}
