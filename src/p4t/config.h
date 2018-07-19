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

#if 0
BB_WARNING_PUSH(4200) // warning C4200: nonstandard extension used: zero-sized array in struct/union

typedef struct configWhitelistEntry_s {
	b32 allow;
	char addressPlusMask[128];
	char applicationName[kBBSize_ApplicationName];
} configWhitelistEntry_t;
typedef struct configWhitelist_s {
	u32 count;
	u32 allocated;
	configWhitelistEntry_t *data;
} configWhitelist_t;

typedef struct openTargetEntry_s {
	char displayName[kBBSize_ApplicationName];
	char commandLine[kBBSize_MaxPath];
} openTargetEntry_t;
typedef struct openTargetList_s {
	u32 count;
	u32 allocated;
	openTargetEntry_t *data;
} openTargetList_t;

typedef struct pathFixupEntry_s {
	char src[kBBSize_MaxPath];
	char dst[kBBSize_MaxPath];
} pathFixupEntry_t;
typedef struct pathFixupList_s {
	u32 count;
	u32 allocated;
	pathFixupEntry_t *data;
} pathFixupList_t;

typedef struct fontConfig_s {
	b32 enabled;
	u32 size;
	char path[kBBSize_MaxPath];
} fontConfig_t;
BB_WARNING_POP;

typedef struct config_s {
	configWhitelist_t whitelist;
	openTargetList_t openTargets;
	pathFixupList_t pathFixups;
	fontConfig_t logFontConfig;
	fontConfig_t uiFontConfig;
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
} config_t;
#endif

AUTOJSON typedef struct fontConfig_s {
	b32 enabled;
	u32 size;
	sb_t path;
} fontConfig_t;

AUTOJSON typedef struct config_s {
	fontConfig_t logFontConfig;
	fontConfig_t uiFontConfig;
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
	u32 pad;
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
