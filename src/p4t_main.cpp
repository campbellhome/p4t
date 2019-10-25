// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "app_update.h"
#include "appdata.h"
#include "bb.h"
#include "cmdline.h"
#include "config.h"
#include "crt_leak_check.h"
#include "imgui_core.h"
#include "output.h"
#include "p4.h"
#include "p4t_update.h"
#include "process_utils.h"
#include "sb.h"
#include "site_config.h"
#include "tasks.h"
#include "ui_changelist.h"
#include "ui_config.h"
#include "ui_tabs.h"
#include "win32_resource.h"

static sb_t s_imguiPath;

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
	crt_leak_check_init();

	cmdline_init_composite(CommandLine);

	BB_INIT_WITH_FLAGS("p4t", (cmdline_find("-bb") > 0) ? kBBInitFlag_None : kBBInitFlag_NoDiscovery);
	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "Arguments: %s", CommandLine);

	site_config_init();
	config_read(&g_config);

	updateData updatedata = { BB_EMPTY_INITIALIZER };
	updatedata.appName = "p4t";
	updatedata.appName = "p4t.exe";
	updatedata.windowClassname = "p4t_wndclass";
	updatedata.resultDir = sb_get(&g_site_config.updates.updateResultDir);
	updatedata.manifestDir = sb_get(&g_site_config.updates.updateManifestDir);
	updatedata.waitForDebugger = g_config.updates.waitForDebugger;
	updatedata.pauseAfterSuccess = g_config.updates.pauseAfterSuccess;
	updatedata.pauseAfterFailure = g_config.updates.pauseAfterFailure;
	updatedata.updateCheckMs = g_site_config.updates.updateCheckMs;
	updatedata.showUpdateManagement = g_config.updates.showManagement;

	if(!Update_Init(&updatedata)) {
		return false;
	}

	Imgui_Core_Init(CommandLine);

	s_imguiPath = appdata_get("p4t");
	sb_append(&s_imguiPath, "\\p4t_imgui.ini");
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = sb_get(&s_imguiPath);
	io.ConfigFlags |= (ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable);

	output_init();

	config_read_apptype(&g_apptypeConfig);

	tasks_startup();
	process_init();
	p4_init();

	UITabs_LoadConfig();

	WINDOWPLACEMENT wp = { BB_EMPTY_INITIALIZER };
	if(Imgui_Core_InitWindow("p4t_wndclass", "p4t", LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_P4_GREEN)), wp)) {
		if(g_config.bDocking) {
			UITabs_SetRedockAll();
		} else {
			io.ConfigFlags &= ~(ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable);
		}
		while(!Imgui_Core_IsShuttingDown()) {
			if(Imgui_Core_GetAndClearDirtyWindowPlacement()) {
				// config_getwindowplacement();
			}
			if(Imgui_Core_BeginFrame()) {
				p4t_update();
				ImVec4 clear_col = ImColor(34, 35, 34);
				Imgui_Core_EndFrame(clear_col);
			}
		}
		Imgui_Core_ShutdownWindow();
	}

	UITabs_SaveConfig();
	p4_diff_shutdown();
	UIChangelist_Shutdown();
	p4_shutdown();
	tasks_shutdown();
	UIConfig_Reset();
	config_write(&g_config);
	config_reset(&g_config);
	config_write_apptype(&g_apptypeConfig);
	UITabs_Reset();
	output_shutdown();
	site_config_shutdown();

	Imgui_Core_Shutdown();
	cmdline_shutdown();
	sb_reset(&s_imguiPath);

	BB_SHUTDOWN();

	return 0;
}
