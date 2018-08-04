// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "app.h"
#include "appdata.h"
#include "cmdline.h"
#include "config.h"
#include "env_utils.h"
#include "imgui_utils.h"
#include "imgui_themes.h"
#include "message_box.h"
#include "output.h"
#include "p4.h"
#include "str.h"
#include "tasks.h"
#include "tokenize.h"
#include "ui_changelist.h"
#include "ui_changeset.h"
#include "ui_clientspec.h"
#include "ui_config.h"
#include "ui_message_box.h"
#include "ui_output.h"
#include "ui_tabs.h"
#include "va.h"
#include "win32_resource.h"
#include "wrap_shellscalingapi.h"

#include "bb.h"
#include "bb_array.h"
#include "bb_wrap_stdio.h"

globals_t globals;
bool g_shuttingDown;

static appSpecificData s_appSpecific[] = {
	{ "p4t", "p4t", "p4t", true, kAppType_Normal },
	{ "p4cl", "p4cl", "Changelist - p4t", false, kAppType_ChangelistViewer },
};

static bool s_showDemo;
static sb_t s_imguiPath;

static bool App_CreateWindow(void)
{
	extern LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if(g_config.dpiAware) {
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	}

	const char *classname = globals.appSpecific.windowClass;

	if(g_config.singleInstanceCheck) {
		HWND hExisting = FindWindowA(classname, nullptr);
		if(hExisting) {
			int response = (g_config.singleInstancePrompt)
			                   ? MessageBoxA(nullptr,
			                                 va("%s is already running - open existing window?", globals.appSpecific.title),
			                                 va("%s is already running", globals.appSpecific.title), MB_YESNO)
			                   : IDYES;
			if(response == IDYES) {
				WINDOWINFO info = {};
				info.cbSize = sizeof(WINDOWINFO);
				if(!GetWindowInfo(hExisting, &info) || info.rcClient.left == info.rcClient.right) {
					ShowWindow(hExisting, SW_RESTORE);
					SendMessageA(hExisting, WM_USER + SW_RESTORE, 0, 0);
				}
				SetForegroundWindow(hExisting);
				return false;
			}
		}
	}

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_P4_GREEN)), LoadCursor(NULL, IDC_ARROW), NULL, NULL, classname, NULL };
	globals.wc = wc;
	RegisterClassEx(&globals.wc);

	WINDOWPLACEMENT wp = g_apptypeConfig.wp;
	const char *title = globals.appSpecific.title;
	globals.hwnd = CreateWindow(classname, title, WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, globals.wc.hInstance, NULL);
	if(wp.rcNormalPosition.right > wp.rcNormalPosition.left) {
		SetWindowPlacement(globals.hwnd, &wp);
	}
	return globals.hwnd != 0;
}

extern "C" void App_SetWindowTitle(const char *title)
{
	SetWindowTextA(globals.hwnd, title);
}

bool App_Init(const char *cmdline)
{
	cmdline_init_composite(cmdline);
	if(!_stricmp(cmdline_get_exe_filename(), "p4cl.exe") ||
	   !_stricmp(cmdline_get_exe_filename(), "p4cl_d.exe") ||
	   cmdline_find("-change") >= 0) {
		globals.appSpecific = s_appSpecific[kAppType_ChangelistViewer];
	} else {
		globals.appSpecific = s_appSpecific[kAppType_Normal];
	}

	s_imguiPath = appdata_get();
	sb_va(&s_imguiPath, "\\%s_imgui.ini", globals.appSpecific.configName);
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = sb_get(&s_imguiPath);
	ImGui::Style_Init();

	const char *applicationName = globals.appSpecific.configName;
#ifdef _DEBUG
	BB_INIT_WITH_FLAGS(applicationName, 0);
#else
	BB_INIT_WITH_FLAGS(applicationName, kBBInitFlag_NoOpenView);
#endif

	BB_THREAD_SET_NAME("main");
	BB_LOG("Startup", "%s cmdline: %s\n", applicationName, cmdline);

	output_init();

	config_read(&g_config);
	config_read_apptype(&g_apptypeConfig);

	tasks_startup();
	process_init();
	p4_init();

	if(globals.appSpecific.type == kAppType_ChangelistViewer) {
		p4UIChangelist *uicl = p4_add_uichangelist();
		if(uicl) {
			UITabs_AddTab(kTabType_Changelist, uicl->id);
			int index = cmdline_find("-change");
			if(index + 1 < argc) {
				u32 cl = strtou32(argv[index + 1]);
				if(cl) {
					uicl->requested = cl;
					p4_describe_changelist(cl);
				} else {
					UIChangelist_EnterChangelist(uicl);
				}
			} else {
				UIChangelist_EnterChangelist(uicl);
			}
		} else {
			return false;
		}
	} else {
		p4UIChangeset *uics;
		uics = p4_add_uichangeset(true);
		if(uics) {
			UITabs_AddTab(kTabType_Changeset, uics->id);
			sb_append(&uics->user, "Current User");
			sb_append(&uics->clientspec, "Current Client");
		}
		uics = p4_add_uichangeset(false);
		if(uics) {
			UITabs_AddTab(kTabType_Changeset, uics->id, false);
		}
	}

	return App_CreateWindow();
}

void App_Shutdown()
{
	p4_diff_shutdown();
	UIChangelist_Shutdown();
	p4_shutdown();
	tasks_shutdown();
	mb_shutdown();
	UIConfig_Reset();
	config_write(&g_config);
	config_reset(&g_config);
	config_write_apptype(&g_apptypeConfig);
	UITabs_Reset();
	BB_SHUTDOWN();

	if(globals.hwnd) {
		DestroyWindow(globals.hwnd);
	}
	if(globals.wc.hInstance) {
		UnregisterClass(globals.wc.lpszClassName, globals.wc.hInstance);
	}

	output_shutdown();
	sb_reset(&s_imguiPath);
	cmdline_shutdown();
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

void App_SingleInstanceRestored(void)
{
}

void App_Update()
{
	BB_TICK();
	tasks_tick();
	p4_update();

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
				UIConfig_Open(&g_config);
			}
			if(ImGui::BeginMenu("DPI Scale Override")) {
				void QueueUpdateDpiDependentResources();
				if(ImGui::MenuItem("1")) {
					g_config.dpiScale = 1.0f;
					QueueUpdateDpiDependentResources();
				}
				if(ImGui::MenuItem("1.25")) {
					g_config.dpiScale = 1.25f;
					QueueUpdateDpiDependentResources();
				}
				if(ImGui::MenuItem("1.5")) {
					g_config.dpiScale = 1.5f;
					QueueUpdateDpiDependentResources();
				}
				if(ImGui::MenuItem("1.75")) {
					g_config.dpiScale = 1.75f;
					QueueUpdateDpiDependentResources();
				}
				if(ImGui::MenuItem("2")) {
					g_config.dpiScale = 2.0f;
					QueueUpdateDpiDependentResources();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		//if(ImGui::BeginMenu("Help")) {
		//	if(ImGui::MenuItem("Demo")) {
		//		BB_LOG("UI::Menu::Demo", "s_showDemo -> %d", !s_showDemo);
		//		s_showDemo = !s_showDemo;
		//	}
		//	ImGui::EndMenu();
		//}
		UIClientspec_MenuBar();

		ImGui::EndMainMenuBar();
	}

	if(s_showDemo) {
		ImGui::ShowTestWindow();
	} else {
		float startY = ImGui::GetFrameHeight();
		ImGuiIO &io = ImGui::GetIO();
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);
		bool open = true;
		ImGuiStyle &style = ImGui::GetStyle();
		float oldWindowRounding = style.WindowRounding;
		style.WindowRounding = 0.0f;
		if(ImGui::Begin("mainwindow", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
			if(UIConfig_IsOpen()) {
				UIConfig_Update(&g_config);
			} else {
				UITabs_Update();

				if(ImGui::IsKeyPressed('G') && ImGui::GetIO().KeyCtrl) {
					p4UIChangelist *uicl = p4_add_uichangelist();
					if(uicl) {
						UITabs_AddTab(kTabType_Changelist, uicl->id);
						UIChangelist_EnterChangelist(uicl);
					}
				}
			}
		}
		ImGui::End();
		style.WindowRounding = oldWindowRounding;

		UIOutput_Update();
		UIMessageBox_Update();

		if(globals.appSpecific.type == kAppType_ChangelistViewer && !p4.uiChangelists.count) {
			App_RequestShutdown();
		}
	}
}

bool App_IsShuttingDown()
{
	return g_shuttingDown;
}

extern "C" void App_RequestShutdown()
{
	g_shuttingDown = true;
}
