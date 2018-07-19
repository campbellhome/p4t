// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "app.h"
#include "appdata.h"
#include "config.h"
#include "env_utils.h"
#include "imgui_utils.h"
#include "output.h"
#include "p4.h"
#include "tokenize.h"
#include "ui_changelist.h"
#include "ui_output.h"
#include "ui_preferences.h"
#include "va.h"
#include "win32_resource.h"
#include "wrap_shellscalingapi.h"

#include "bb.h"
#include "bb_array.h"
#include "bb_wrap_stdio.h"

globals_t globals;
bool g_shuttingDown;

static bool s_showDemo;
static sb_t s_imguiPath;
static sb_t app_get_imgui_path()
{
	sb_t s = appdata_get();
	sb_append(&s, "\\imgui.ini");
	return s;
}

static bool App_CreateWindow(void)
{
	extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if(g_config.dpiAware) {
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	}

	const char *classname = "p4t";

	if(g_config.singleInstanceCheck) {
		HWND hExisting = FindWindowA(classname, nullptr);
		if(hExisting) {
			int response = (g_config.singleInstancePrompt)
			                   ? MessageBoxA(nullptr, "p4t is already running - open existing window?", "p4t is already running", MB_YESNO)
			                   : IDYES;
			if(response == IDYES) {
				WINDOWINFO info = {};
				info.cbSize = sizeof(WINDOWINFO);
				if(!GetWindowInfo(hExisting, &info) || info.rcClient.left == info.rcClient.right) {
					ShowWindow(hExisting, SW_RESTORE);
				}
				SetForegroundWindow(hExisting);
				return false;
			}
		}
	}

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_P4_GREEN)), LoadCursor(NULL, IDC_ARROW), NULL, NULL, classname, NULL };
	globals.wc = wc;
	RegisterClassEx(&globals.wc);

	WINDOWPLACEMENT wp = g_config.wp;
	const char *title = "p4t";
	globals.hwnd = CreateWindow(classname, title, WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, globals.wc.hInstance, NULL);
	if(wp.rcNormalPosition.right > wp.rcNormalPosition.left) {
		SetWindowPlacement(globals.hwnd, &wp);
	}
	return globals.hwnd != 0;
}

bool App_Init(const char *cmdline)
{
	s_imguiPath = app_get_imgui_path();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = sb_get(&s_imguiPath);

	const char *applicationName = "p4t";
	BB_INIT(applicationName);

	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "%s cmdline: %s\n", applicationName, cmdline);

	output_init();

	config_read(&g_config);
	g_config.recordingsOpen = false;
	g_config.autoTileViews = 1;
	g_config.autoDeleteAfterDays = 0;

	process_init();
	p4_init();

	return App_CreateWindow();
}

void App_Shutdown()
{
	p4_shutdown();
	Preferences_Reset();
	config_write(&g_config);
	config_reset(&g_config);
	BB_SHUTDOWN();

	if(globals.hwnd) {
		DestroyWindow(globals.hwnd);
	}
	if(globals.wc.hInstance) {
		UnregisterClass(globals.wc.lpszClassName, globals.wc.hInstance);
	}

	output_shutdown();
	sb_reset(&s_imguiPath);
}

int g_appRequestRenderCount;
extern "C" void App_RequestRender(void)
{
	g_appRequestRenderCount = 3;
}
extern "C" bool App_GetAndClearRequestRender(void)
{
	bool ret = g_appRequestRenderCount > 0;
	g_appRequestRenderCount = BB_MAX(0, g_appRequestRenderCount - 1);
	return ret;
}

void App_Update()
{
	p4_tick();
	BB_TICK();
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("Exit")) {
				g_shuttingDown = true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Edit")) {
			if(ImGui::MenuItem("Preferences")) {
				BB_LOG("UI::Menu::Preferences", "Preferences_Open");
				Preferences_Open(&g_config);
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Help")) {
			if(ImGui::MenuItem("Demo")) {
				BB_LOG("UI::Menu::Demo", "s_showDemo -> %d", !s_showDemo);
				s_showDemo = !s_showDemo;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if(s_showDemo) {
		ImGui::ShowTestWindow();
	}
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImColor(63, 63, 70, 255)); // VS Dark Active Tab
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImColor(45, 45, 48, 255));       // VS Dark Inactive Tab
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(42, 42, 44, 255));      // VS Dark Output Window
	Preferences_Update(&g_config);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(30, 30, 30, 255)); // VS Dark Text Window
	UIChangelist_Update();
	UIOutput_Update();
	ImGui::PopStyleColor(3);
}

bool App_IsShuttingDown()
{
	return g_shuttingDown;
}
