// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#include "p4t_structs_generated.h"
#include "bb_array.h"
#include "str.h"
#include "va.h"

#include "config.h"
#include "fonts.h"
#include "sb.h"
#include "sdict.h"
#include "site_config.h"
#include "uuid_rfc4122/sysdep.h"

#include <string.h>


void POINT_reset(POINT *val)
{
	if(val) {
	}
}
POINT POINT_clone(const POINT *src)
{
	POINT dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.x = src->x;
		dst.y = src->y;
	}
	return dst;
}

void RECT_reset(RECT *val)
{
	if(val) {
	}
}
RECT RECT_clone(const RECT *src)
{
	RECT dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.left = src->left;
		dst.top = src->top;
		dst.right = src->right;
		dst.bottom = src->bottom;
	}
	return dst;
}

void WINDOWPLACEMENT_reset(WINDOWPLACEMENT *val)
{
	if(val) {
		POINT_reset(&val->ptMinPosition);
		POINT_reset(&val->ptMaxPosition);
		RECT_reset(&val->rcNormalPosition);
	}
}
WINDOWPLACEMENT WINDOWPLACEMENT_clone(const WINDOWPLACEMENT *src)
{
	WINDOWPLACEMENT dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.length = src->length;
		dst.flags = src->flags;
		dst.showCmd = src->showCmd;
		dst.ptMinPosition = POINT_clone(&src->ptMinPosition);
		dst.ptMaxPosition = POINT_clone(&src->ptMaxPosition);
		dst.rcNormalPosition = RECT_clone(&src->rcNormalPosition);
	}
	return dst;
}

void uiChangelistConfig_reset(uiChangelistConfig *val)
{
	if(val) {
	}
}
uiChangelistConfig uiChangelistConfig_clone(const uiChangelistConfig *src)
{
	uiChangelistConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.descHeight = src->descHeight;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->columnWidth); ++i) {
			dst.columnWidth[i] = src->columnWidth[i];
		}
		dst.sortDescending = src->sortDescending;
		dst.sortColumn = src->sortColumn;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}

void uiChangesetConfig_reset(uiChangesetConfig *val)
{
	if(val) {
	}
}
uiChangesetConfig uiChangesetConfig_clone(const uiChangesetConfig *src)
{
	uiChangesetConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		for(u32 i = 0; i < BB_ARRAYSIZE(src->columnWidth); ++i) {
			dst.columnWidth[i] = src->columnWidth[i];
		}
		dst.sortDescending = src->sortDescending;
		dst.sortColumn = src->sortColumn;
	}
	return dst;
}

void diffConfig_reset(diffConfig_t *val)
{
	if(val) {
		sb_reset(&val->path);
		sb_reset(&val->args);
	}
}
diffConfig_t diffConfig_clone(const diffConfig_t *src)
{
	diffConfig_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.enabled = src->enabled;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
		dst.path = sb_clone(&src->path);
		dst.args = sb_clone(&src->args);
	}
	return dst;
}

void appTypeConfig_reset(appTypeConfig *val)
{
	if(val) {
		WINDOWPLACEMENT_reset(&val->wp);
	}
}
appTypeConfig appTypeConfig_clone(const appTypeConfig *src)
{
	appTypeConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.wp = WINDOWPLACEMENT_clone(&src->wp);
		dst.version = src->version;
	}
	return dst;
}

void p4Config_reset(p4Config *val)
{
	if(val) {
		sb_reset(&val->clientspec);
	}
}
p4Config p4Config_clone(const p4Config *src)
{
	p4Config dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.clientspec = sb_clone(&src->clientspec);
		dst.changelistBlockSize = src->changelistBlockSize;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}

void changelistConfig_reset(changelistConfig *val)
{
	if(val) {
	}
}
changelistConfig changelistConfig_clone(const changelistConfig *src)
{
	changelistConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.number = src->number;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}

void changesetConfig_reset(changesetConfig *val)
{
	if(val) {
		sb_reset(&val->user);
		sb_reset(&val->clientspec);
		sb_reset(&val->filter);
		sb_reset(&val->filterInput);
	}
}
changesetConfig changesetConfig_clone(const changesetConfig *src)
{
	changesetConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.pending = src->pending;
		dst.filterEnabled = src->filterEnabled;
		dst.user = sb_clone(&src->user);
		dst.clientspec = sb_clone(&src->clientspec);
		dst.filter = sb_clone(&src->filter);
		dst.filterInput = sb_clone(&src->filterInput);
	}
	return dst;
}

void tabConfig_reset(tabConfig *val)
{
	if(val) {
		changelistConfig_reset(&val->cl);
		changesetConfig_reset(&val->cs);
	}
}
tabConfig tabConfig_clone(const tabConfig *src)
{
	tabConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.isChangeset = src->isChangeset;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
		dst.cl = changelistConfig_clone(&src->cl);
		dst.cs = changesetConfig_clone(&src->cs);
	}
	return dst;
}

void tabsConfig_reset(tabsConfig *val)
{
	if(val) {
		for(u32 i = 0; i < val->count; ++i) {
			tabConfig_reset(val->data + i);
		}
		bba_free(*val);
	}
}
tabsConfig tabsConfig_clone(const tabsConfig *src)
{
	tabsConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		for(u32 i = 0; i < src->count; ++i) {
			if(bba_add_noclear(dst, 1)) {
				bba_last(dst) = tabConfig_clone(src->data + i);
			}
		}
	}
	return dst;
}

void updatesConfig_reset(updatesConfig *val)
{
	if(val) {
	}
}
updatesConfig updatesConfig_clone(const updatesConfig *src)
{
	updatesConfig dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.waitForDebugger = src->waitForDebugger;
		dst.pauseAfterSuccess = src->pauseAfterSuccess;
		dst.pauseAfterFailure = src->pauseAfterFailure;
		dst.showManagement = src->showManagement;
	}
	return dst;
}

void config_reset(config_t *val)
{
	if(val) {
		fontConfig_reset(&val->logFontConfig);
		fontConfig_reset(&val->uiFontConfig);
		tabsConfig_reset(&val->tabs);
		uiChangelistConfig_reset(&val->uiChangelist);
		uiChangesetConfig_reset(&val->uiPendingChangesets);
		uiChangesetConfig_reset(&val->uiSubmittedChangesets);
		updatesConfig_reset(&val->updates);
		diffConfig_reset(&val->diff);
		sb_reset(&val->colorscheme);
		p4Config_reset(&val->p4);
	}
}
config_t config_clone(const config_t *src)
{
	config_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.logFontConfig = fontConfig_clone(&src->logFontConfig);
		dst.uiFontConfig = fontConfig_clone(&src->uiFontConfig);
		dst.tabs = tabsConfig_clone(&src->tabs);
		dst.uiChangelist = uiChangelistConfig_clone(&src->uiChangelist);
		dst.uiPendingChangesets = uiChangesetConfig_clone(&src->uiPendingChangesets);
		dst.uiSubmittedChangesets = uiChangesetConfig_clone(&src->uiSubmittedChangesets);
		dst.updates = updatesConfig_clone(&src->updates);
		dst.version = src->version;
		dst.diff = diffConfig_clone(&src->diff);
		dst.colorscheme = sb_clone(&src->colorscheme);
		dst.p4 = p4Config_clone(&src->p4);
		dst.singleInstanceCheck = src->singleInstanceCheck;
		dst.singleInstancePrompt = src->singleInstancePrompt;
		dst.dpiAware = src->dpiAware;
		dst.doubleClickSeconds = src->doubleClickSeconds;
		dst.dpiScale = src->dpiScale;
		dst.activeTab = src->activeTab;
		dst.bDocking = src->bDocking;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}

void updateConfig_reset(updateConfig_t *val)
{
	if(val) {
		sb_reset(&val->updateResultDir);
		sb_reset(&val->updateManifestDir);
	}
}
updateConfig_t updateConfig_clone(const updateConfig_t *src)
{
	updateConfig_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.updateResultDir = sb_clone(&src->updateResultDir);
		dst.updateManifestDir = sb_clone(&src->updateManifestDir);
		dst.updateCheckMs = src->updateCheckMs;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}

void site_config_reset(site_config_t *val)
{
	if(val) {
		updateConfig_reset(&val->updates);
		sb_reset(&val->bugAssignee);
		sb_reset(&val->bugProject);
	}
}
site_config_t site_config_clone(const site_config_t *src)
{
	site_config_t dst = { BB_EMPTY_INITIALIZER };
	if(src) {
		dst.updates = updateConfig_clone(&src->updates);
		dst.bugAssignee = sb_clone(&src->bugAssignee);
		dst.bugProject = sb_clone(&src->bugProject);
		dst.bugPort = src->bugPort;
		for(u32 i = 0; i < BB_ARRAYSIZE(src->pad); ++i) {
			dst.pad[i] = src->pad[i];
		}
	}
	return dst;
}
