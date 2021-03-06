// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_tabs.h"
#include "bb_array.h"
#include "imgui_utils.h"
#include "p4.h"
#include "ui_changelist.h"
#include "ui_changeset.h"
#include "ui_icons.h"
#include "va.h"

extern "C" void tabsConfig_reset(tabsConfig *val);

static tabs s_tabs;

void UITabs_SetRedockAll(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}
	ts->bRedockAll = true;
}

void UITabs_Reset(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}
	bba_free(*ts);
}

tab *UITabs_AddTab(tabType type, u32 id, b32 activate, tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}
	if(bba_add(*ts, 1)) {
		tab *t = &bba_last(*ts);
		t->type = type;
		t->id = id;
		if(activate) {
			ts->activeTab = ts->count - 1;
		}
		return t;
	}
	return NULL;
}

void UITabs_AddPendingChangeset()
{
	if(p4UIChangeset *uics = p4_add_uichangeset(true)) {
		UITabs_AddTab(kTabType_Changeset, uics->id);
		sb_append(&uics->config.user, "Current User");
		sb_append(&uics->config.clientspec, "Current Client");
	}
}

void UITabs_SaveConfig(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}

	tabsConfig_reset(&g_config.tabs);
	g_config.activeTab = ts->activeTab;
	for(u32 i = 0; i < ts->count; ++i) {
		tab *t = ts->data + i;
		tabConfig tc = {};
		switch(t->type) {
		case kTabType_Changelist: {
			p4UIChangelist *uicl = p4_find_uichangelist(t->id);
			if(uicl) {
				tc.isChangeset = false;
				tc.cl = uicl->config;
				memset(&uicl->config, 0, sizeof(uicl->config));
			}
		} break;
		case kTabType_Changeset: {
			p4UIChangeset *uics = p4_find_uichangeset(t->id);
			if(uics) {
				tc.isChangeset = true;
				tc.cs = uics->config;
				memset(&uics->config, 0, sizeof(uics->config));
			}
		} break;
		case kTabType_Count:
			break;
		}
		bba_push(g_config.tabs, tc);
	}
}

void UITabs_LoadConfig(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}

	for(u32 i = 0; i < g_config.tabs.count; ++i) {
		tabConfig *tc = g_config.tabs.data + i;
		if(tc->isChangeset) {
			if(p4UIChangeset *uics = p4_add_uichangeset(tc->cs.pending)) {
				UITabs_AddTab(kTabType_Changeset, uics->id, (i == g_config.activeTab));
				uics->config = tc->cs;
				memset(&tc->cs, 0, sizeof(tc->cs));
			}
		} else {
			if(p4UIChangelist *uicl = p4_add_uichangelist()) {
				UITabs_AddTab(kTabType_Changelist, uicl->id, (i == g_config.activeTab));
				uicl->config = tc->cl;
				memset(&tc->cl, 0, sizeof(tc->cl));
				if(uicl->config.number) {
					p4_describe_changelist(uicl->config.number);
				}
			}
		}
	}

	if(ts->count == 0) {
		UITabs_AddPendingChangeset();
	}
}

struct UITab_TitleData {
	const char *title;
	const char *icon;
	ImColor color;
};

static UITab_TitleData UITabs_Title(tab *t)
{
	UITab_TitleData result = { BB_EMPTY_INITIALIZER };
	switch(t->type) {
	case kTabType_Changelist: {
		p4UIChangelist *uicl = p4_find_uichangelist(t->id);
		if(uicl) {
			result.title = uicl->config.number ? va("Changelist %u", uicl->config.number) : "Changelist";
			result.icon = ICON_CHANGELIST;
			result.color = COLOR_PENDING_CHANGELIST_OTHER;
			p4Changelist *cl = p4_find_changelist(uicl->config.number);
			if(cl) {
				b32 pending = !strcmp(sdict_find_safe(&cl->normal, "status"), "pending");
				result.color = (pending) ? COLOR_PENDING_CHANGELIST_LOCAL : COLOR_SUBMITTED_CHANGELIST;
			}
			//result.title = va("%s%s ###changelist%u", UIIcons_GetIconSpaces(ICON_CHANGELIST), title, uicl->id);
		}
		break;
	}
	case kTabType_Changeset: {
		p4UIChangeset *uics = p4_find_uichangeset(t->id);
		if(uics) {
			result.title = uics->config.pending ? "Pending Changelists" : "Submitted Changelists";
			result.icon = ICON_CHANGELIST;
			result.color = (uics->config.pending) ? COLOR_PENDING_CHANGELIST_LOCAL : COLOR_SUBMITTED_CHANGELIST;
		}
		break;
	}
	case kTabType_Count:
		break;
	}
	if(!result.title) {
		result.title = "untitled";
	}
	return result;
}

void UITabs_Update(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}

	if(ts->activeTab >= ts->count) {
		ts->activeTab = 0;
	}

	for(u32 i = 0; i < ts->count;) {
		tab *t = ts->data + i;
		b32 hasPtr = (t->type == kTabType_Changelist && p4_find_uichangelist(t->id) != nullptr) ||
		             (t->type == kTabType_Changeset && p4_find_uichangeset(t->id) != nullptr);
		if(hasPtr) {
			++i;
		} else {
			bba_erase(*ts, i);
		}
	}

	if((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0) {
		ImGui::BeginTabButtons();
		for(u32 i = 0; i < ts->count; ++i) {
			tab *t = ts->data + i;
			switch(t->type) {
			case kTabType_Changelist: {
				p4UIChangelist *uicl = p4_find_uichangelist(t->id);
				if(uicl) {
					const char *title = uicl->config.number ? va("Changelist %u", uicl->config.number) : "Changelist";
					ImColor color = COLOR_PENDING_CHANGELIST_OTHER;
					p4Changelist *cl = p4_find_changelist(uicl->config.number);
					if(cl) {
						b32 pending = !strcmp(sdict_find_safe(&cl->normal, "status"), "pending");
						color = (pending) ? COLOR_PENDING_CHANGELIST_LOCAL : COLOR_SUBMITTED_CHANGELIST;
					}
					if(ImGui::TabButtonIconColored(ICON_CHANGELIST, color, va("%s%s ###changelist%u", UIIcons_GetIconSpaces(ICON_CHANGELIST), title, uicl->id), &ts->activeTab, i)) {
						UIChangelist_SetWindowTitle(uicl);
					}
					if(ImGui::BeginContextMenu(va("context%u", uicl->id))) {
						ts->activeTab = i;
						if(ImGui::MenuItem("Close Tab")) {
							p4_mark_uichangelist_for_removal(uicl);
						}
						ImGui::EndContextMenu();
					}
				}
				break;
			}
			case kTabType_Changeset: {
				p4UIChangeset *uics = p4_find_uichangeset(t->id);
				if(uics) {
					const char *title = uics->config.pending ? "Pending Changelists" : "Submitted Changelists";
					ImColor color = (uics->config.pending) ? COLOR_PENDING_CHANGELIST_LOCAL : COLOR_SUBMITTED_CHANGELIST;
					if(ImGui::TabButtonIconColored(ICON_CHANGELIST, color, va("%s%s ###changeset%u", UIIcons_GetIconSpaces(ICON_CHANGELIST), title, uics->id), &ts->activeTab, i)) {
						UIChangeset_SetWindowTitle(uics);
					}
					if(ImGui::BeginContextMenu(va("context%u", uics->id))) {
						ts->activeTab = i;
						if(ImGui::MenuItem("Close Tab")) {
							p4_mark_uichangeset_for_removal(uics);
						}
						ImGui::EndContextMenu();
					}
				}
				break;
			}
			case kTabType_Count:
				break;
			}
		}
		u32 dummy = 0;
		if(ImGui::TabButton("+", &dummy, 1)) {
			ImGui::OpenPopup("newtab");
		}
		if(ImGui::BeginContextMenu("newtab")) {
			if(ImGui::MenuItem("Pending Changelists")) {
				UITabs_AddPendingChangeset();
			}
			if(ImGui::MenuItem("Submitted Changelists")) {
				if(p4UIChangeset *uics = p4_add_uichangeset(false)) {
					UITabs_AddTab(kTabType_Changeset, uics->id);
				}
			}
			ImGui::EndContextMenu();
		}
		ImGui::EndTabButtons();

		if(ts->activeTab < ts->count) {
			tab *t = ts->data + ts->activeTab;
			if(ImGui::BeginTabChild(&ts->activeTab, ts->activeTab, va("tab%u", t->id))) {
				switch(t->type) {
				case kTabType_Changelist: {
					p4UIChangelist *uicl = p4_find_uichangelist(t->id);
					if(uicl) {
						UIChangelist_Update(uicl);
					}
					break;
				}
				case kTabType_Changeset: {
					p4UIChangeset *uics = p4_find_uichangeset(t->id);
					if(uics) {
						UIChangeset_Update(uics);
					}
					break;
				}
				case kTabType_Count:
					break;
				}
				ImGui::EndTabChild();
			}
		}
	} else {

		ImGuiID MainDockSpace = ImGui::GetID("MainDockSpace");
		for(u32 i = 0; i < ts->count; ++i) {
			tab *t = ts->data + i;
			UITab_TitleData titleData = UITabs_Title(t);
			ImGui::SetNextWindowDockID(MainDockSpace, ts->bRedockAll ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
			if(ImGui::Begin(va("%s##tab%u", titleData.title, t->id))) {
				switch(t->type) {
				case kTabType_Changelist: {
					p4UIChangelist *uicl = p4_find_uichangelist(t->id);
					if(uicl) {
						UIChangelist_Update(uicl);
					}
					break;
				}
				case kTabType_Changeset: {
					p4UIChangeset *uics = p4_find_uichangeset(t->id);
					if(uics) {
						UIChangeset_Update(uics);
					}
					break;
				}
				case kTabType_Count:
					break;
				}
			}
			ImGui::End();
		}
		ts->bRedockAll = false;
	}
}
