// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#if defined(_MSC_VER)
__pragma(warning(disable : 4710)); // warning C4710 : 'int printf(const char *const ,...)' : function not inlined
#endif                             // #if defined( _MSC_VER )

#if defined(__cplusplus)
extern "C" {
#endif

#include "bb.h"
#include "fonts.h"
#include "sdict.h"

// define the Windows structs for preproc
#if 0
AUTOJSON typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT;

AUTOJSON typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT;

AUTOJSON typedef struct tagWINDOWPLACEMENT {
	UINT length;
	UINT flags;
	UINT showCmd;
	POINT ptMinPosition;
	POINT ptMaxPosition;
	RECT rcNormalPosition;
} WINDOWPLACEMENT;
#endif

AUTOJSON typedef struct tag_uiChangelistConfig {
	float descHeight;
	float columnWidth[5];
	b32 sortDescending;
	u32 sortColumn;
	u8 pad[4];
} uiChangelistConfig;

AUTOJSON typedef struct tag_uiChangesetConfig {
	float columnWidth[5];
	b32 sortDescending;
	u32 sortColumn;
} uiChangesetConfig;

AUTOJSON typedef struct diffConfig_s {
	b32 enabled;
	u8 pad[4];
	sb_t path;
	sb_t args;
} diffConfig_t;

AUTOJSON typedef struct tag_appTypeConfig {
	WINDOWPLACEMENT wp;
	u32 version;
} appTypeConfig;

AUTOJSON typedef struct tag_p4Config {
	sb_t clientspec;
	u32 changelistBlockSize;
	u8 pad[4];
} p4Config;

AUTOJSON typedef struct tag_changelistConfig {
	u32 number;
	u8 pad[4];
} changelistConfig;

AUTOJSON typedef struct tag_changesetConfig {
	b32 pending;
	b32 filterEnabled;
	sb_t user;
	sb_t clientspec;
	sb_t filter;
	sb_t filterInput;
} changesetConfig;

AUTOJSON typedef struct tag_tabConfig {
	b32 isChangeset;
	u8 pad[4];
	changelistConfig cl;
	changesetConfig cs;
} tabConfig;

AUTOJSON typedef struct tag_tabsConfig {
	u32 count;
	u32 allocated;
	tabConfig *data;
} tabsConfig;

AUTOJSON typedef struct tag_updatesConfig {
	b32 waitForDebugger;
	b32 pauseAfterSuccess;
	b32 pauseAfterFailure;
	b32 showManagement;
} updatesConfig;

AUTOJSON typedef struct config_s {
	fontConfig_t logFontConfig;
	fontConfig_t uiFontConfig;
	tabsConfig tabs;
	uiChangelistConfig uiChangelist;
	uiChangesetConfig uiPendingChangesets;
	uiChangesetConfig uiSubmittedChangesets;
	updatesConfig updates;
	u32 version;
	diffConfig_t diff;
	sb_t colorscheme;
	p4Config p4;
	b32 singleInstanceCheck;
	b32 singleInstancePrompt;
	b32 dpiAware;
	float doubleClickSeconds;
	float dpiScale;
	u32 activeTab;
	b32 bDocking;
	u8 pad[4];
} config_t;

enum { kConfigVersion = 1,
	   kConfigAppTypeVersion = 1 };
extern config_t g_config;
extern appTypeConfig g_apptypeConfig;

b32 config_read(config_t *config);
b32 config_write(config_t *config);
config_t config_clone(const config_t *config);
void config_reset(config_t *config);
void config_free(config_t *config);
void config_getwindowplacement(HWND hwnd);

b32 config_read_apptype(appTypeConfig *config);
b32 config_write_apptype(appTypeConfig *config);

#if defined(__cplusplus)
}
#endif
