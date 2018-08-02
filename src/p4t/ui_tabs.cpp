// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_tabs.h"
#include "app.h"
#include "bb_array.h"
#include "imgui_utils.h"
#include "p4.h"
#include "ui_changelist.h"
#include "ui_changeset.h"
#include "va.h"

static tabs s_tabs;

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

void UITabs_Update(tabs *ts)
{
	if(!ts) {
		ts = &s_tabs;
	}

	if(ts->activeTab >= ts->count) {
		ts->activeTab = 0;
	}

	for(u32 i = ts->count - 1; i < ts->count; --i) {
		tab *t = ts->data + i;
		b32 hasPtr = (t->type == kTabType_Changelist && p4_find_uichangelist(t->id) != nullptr) ||
		             (t->type == kTabType_Changeset && p4_find_uichangeset(t->id) != nullptr);
		if(!hasPtr) {
			bba_erase(*ts, i);
		}
	}

	ImGui::BeginTabButtons();
	for(u32 i = 0; i < ts->count; ++i) {
		tab *t = ts->data + i;
		switch(t->type) {
		case kTabType_Changelist: {
			p4UIChangelist *uicl = p4_find_uichangelist(t->id);
			if(uicl) {
				const char *title = uicl->requested ? va("Changelist %u", uicl->requested) : "Changelist";
				if(ImGui::TabButton(va(" %s ###changelist%u", title, uicl->id), &ts->activeTab, i)) {
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
				const char *title = uics->pending ? "Pending Changelists" : "Submitted Changelists";
				if(ImGui::TabButton(va(" %s ###changeset%u", title, uics->id), &ts->activeTab, i)) {
					UIChangeset_SetWindowTitle(uics);
				}
			}
			break;
		}
		case kTabType_Count:
			break;
		}
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
}