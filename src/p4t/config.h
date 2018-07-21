// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#if defined(_MSC_VER)
__pragma(warning(disable : 4710)); // warning C4710 : 'int printf(const char *const ,...)' : function not inlined
#endif                             // #if defined( _MSC_VER )

#if defined(__cplusplus)
extern "C" {
#endif

#include "sdict.h"

#include "bb.h"

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

AUTOJSON typedef struct fontConfig_s {
	b32 enabled;
	u32 size;
	sb_t path;
} fontConfig_t;

AUTOJSON typedef struct tag_uiChangelistConfig {
	float descHeight;
	float columnWidth[5];
	b32 sortDescending;
	u32 sortColumn;
	u8 pad[4];
} uiChangelistConfig;

AUTOJSON typedef struct config_s {
	fontConfig_t logFontConfig;
	fontConfig_t uiFontConfig;
	uiChangelistConfig uiChangelist;
	WINDOWPLACEMENT wp;
	b32 autoTileViews;
	b32 alternateRowBackground;
	b32 recordingsOpen;
	b32 singleInstanceCheck;
	b32 singleInstancePrompt;
	b32 dpiAware;
	u32 autoDeleteAfterDays;
	float doubleClickSeconds;
	float dpiScale;
	u32 version;
} config_t;

enum { kConfigVersion = 1 };
extern config_t g_config;

b32 config_read(config_t *config);
b32 config_write(config_t *config);
config_t *config_clone(config_t *config);
void config_reset(config_t *config);
void config_free(config_t *config);
void config_getwindowplacement(HWND hwnd);

#if defined(__cplusplus)
}
#endif
