// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

extern bool App_Init(const char *commandLine);
extern void App_Shutdown(void);
extern void App_Update(void);
extern bool App_IsShuttingDown(void);
extern void App_SingleInstanceRestored(void);

#if defined(__cplusplus)
#include "wrap_imgui.h"
extern "C" {
#endif
extern void App_RequestRender(void);
extern bool App_GetAndClearRequestRender(void);
extern void App_SetWindowTitle(const char *title);
extern void App_RequestShutdown(void);
#include "bb_time.h"

typedef enum tag_appType
{
	kAppType_Normal,
	kAppType_ChangelistViewer,
	kAppType_Count
} appType;

typedef struct tag_appSpecificData
{
	const char *windowClass;
	const char *configName;
	const char *title;
	b32 allowSingleInstance;
	appType type;
} appSpecificData;

typedef struct globals_s {
	WNDCLASSEX wc;
	HWND hwnd;
	appSpecificData appSpecific;
} globals_t;

extern globals_t globals;

#if defined(__cplusplus)
}
#endif
