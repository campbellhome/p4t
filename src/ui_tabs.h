// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

typedef enum tag_tabType {
	kTabType_Changeset,
	kTabType_Changelist,
	kTabType_Count
} tabType;

typedef struct tag_tab {
	tabType type;
	u32 id;
} tab;

typedef struct tag_tabs {
	u32 count;
	u32 allocated;
	tab *data;
	u32 activeTab;
	b32 bRedockAll;
} tabs;

void UITabs_Reset(tabs *ts = nullptr);
tab *UITabs_AddTab(tabType type, u32 id, b32 activate = true, tabs *ts = nullptr);
void UITabs_Update(tabs *ts = nullptr);
void UITabs_SaveConfig(tabs *ts = nullptr);
void UITabs_LoadConfig(tabs *ts = nullptr);
void UITabs_SetRedockAll(tabs *ts = nullptr);
