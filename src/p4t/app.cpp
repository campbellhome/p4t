// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "app.h"
#include "appdata.h"
#include "cmdline.h"
#include "config.h"
#include "env_utils.h"
#include "imgui_utils.h"
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
	{ "p4cl", "p4cl", "CL - p4t", false, kAppType_ChangelistViewer },
};

static sb_t s_activeTab;
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

	WINDOWPLACEMENT wp = g_config.wp;
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
	if(cmdline_find("-change") >= 0) {
		globals.appSpecific = s_appSpecific[kAppType_ChangelistViewer];
	} else {
		globals.appSpecific = s_appSpecific[kAppType_Normal];
	}

	s_imguiPath = appdata_get();
	sb_va(&s_imguiPath, "\\%s_imgui.ini", globals.appSpecific.configName);
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = sb_get(&s_imguiPath);

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

	tasks_startup();
	process_init();
	p4_init();

	//sb_append(&s_activeTab, "changelist");
	p4_add_uichangeset(true);
	p4_add_uichangeset(false);

	if(globals.appSpecific.type == kAppType_ChangelistViewer) {
		int index = cmdline_find("-change");
		if(index + 1 < argc) {
			u32 cl = strtou32(argv[index + 1]);
			if(cl) {
				UIChangelist_InitChangelist(cl);
			}
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
	BB_SHUTDOWN();

	if(globals.hwnd) {
		DestroyWindow(globals.hwnd);
	}
	if(globals.wc.hInstance) {
		UnregisterClass(globals.wc.lpszClassName, globals.wc.hInstance);
	}

	output_shutdown();
	sb_reset(&s_imguiPath);
	sb_reset(&s_activeTab);
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
	UIChangelist_EnterChangelist();
}

void App_Update()
{
	BB_TICK();
	tasks_tick();

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
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Help")) {
			if(ImGui::MenuItem("Demo")) {
				BB_LOG("UI::Menu::Demo", "s_showDemo -> %d", !s_showDemo);
				s_showDemo = !s_showDemo;
			}
			ImGui::EndMenu();
		}
		UIClientspec_MenuBar();
		ImGui::EndMainMenuBar();
	}

	if(s_showDemo) {
		ImGui::ShowTestWindow();
	} else {
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImColor(63, 63, 70, 255)); // VS Dark Active Tab
		ImGui::PushStyleColor(ImGuiCol_TitleBg, ImColor(45, 45, 48, 255));       // VS Dark Inactive Tab
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(42, 42, 44, 255));      // VS Dark Output Window

		float startY = ImGui::GetItemsLineHeightWithSpacing();
		ImGuiIO &io = ImGui::GetIO();
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);
		bool open = true;
		if(ImGui::Begin("mainwindow", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {

			if(globals.appSpecific.type == kAppType_Normal) {
				ImGui::BeginTabButtons();
				ImGui::TabButton(" Changelist ", &s_activeTab, "changelist");
				for(u32 i = 0; i < p4.uiChangesets.count; ++i) {
					p4UIChangeset *cs = p4.uiChangesets.data + i;
					const char *title = cs->pending ? " Pending Changelists " : " Submitted Changelists ";
					ImGui::TabButton(title, &s_activeTab, va("changeset%u", cs->id));
				}
				ImGui::EndTabButtons();

				if(ImGui::BeginTabChild(&s_activeTab, "changelist")) {
					UIChangelist_Update();
					ImGui::EndTabChild();
				} else {
					for(u32 i = 0; i < p4.uiChangesets.count; ++i) {
						p4UIChangeset *cs = p4.uiChangesets.data + i;
						if(ImGui::BeginTabChild(&s_activeTab, va("changeset%u", cs->id))) {
							UIChangeset_Update(cs);
							ImGui::EndTabChild();
						}
					}
				}
			} else if(globals.appSpecific.type == kAppType_ChangelistViewer) {
				UIChangelist_Update();
			}
		}
		ImGui::End();

		UIOutput_Update();
		UIMessageBox_Update();

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(42, 42, 44, 255)); // VS Dark Output Window
		UIConfig_Update(&g_config);
		ImGui::PopStyleColor();

		ImGui::PopStyleColor(3);
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
