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
//#include "bb_connection.h"
//#include "bb_discovery_client.h"
//#include "bb_discovery_server.h"
//#include "bb_log.h"
//#include "bb_packet.h"
//#include "bb_sockets.h"
//#include "bb_string.h"
//#include "bb_thread.h"
#include "bb_time.h"

typedef struct globals_s {
	WNDCLASSEX wc;
	HWND hwnd;
} globals_t;

extern globals_t globals;

#if defined(__cplusplus)
}
#endif
