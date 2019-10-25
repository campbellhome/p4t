// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "p4t_update.h"
#include "app_update.h"
#include "bb_string.h"
#include "common.h"
#include "crt_leak_check.h"
#include "fonts.h"
#include "imgui_core.h"
#include "imgui_utils.h"
#include "message_box.h"
#include "p4.h"
#include "tasks.h"
#include "ui_changelist.h"
#include "ui_changeset.h"
#include "ui_clientspec.h"
#include "ui_config.h"
#include "ui_message_box.h"
#include "ui_output.h"
#include "ui_tabs.h"
#include "va.h"

static bool s_showImguiDemo;
static bool s_showImguiAbout;
static bool s_showImguiMetrics;
static bool s_showImguiUserGuide;
static bool s_showImguiStyleEditor;

static const char *s_colorschemes[] = {
	"ImGui Dark",
	"Light",
	"Classic",
	"Visual Studio Dark",
	"Windows",
};

static void p4t_menubar(void)
{
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("Exit")) {
				Imgui_Core_RequestShutDown();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Edit")) {
			if(ImGui::MenuItem("Config")) {
				BB_LOG("UI::Menu::Config", "UIConfig_Open");
				UIConfig_Open(&g_config);
			}
			if(ImGui::BeginMenu("Debug")) {
				if(ImGui::BeginMenu("Color schemes")) {
					const char *colorscheme = Imgui_Core_GetColorScheme();
					for(int i = 0; i < BB_ARRAYSIZE(s_colorschemes); ++i) {
						bool bSelected = !strcmp(colorscheme, s_colorschemes[i]);
						if(ImGui::MenuItem(s_colorschemes[i], nullptr, &bSelected)) {
							Imgui_Core_SetColorScheme(s_colorschemes[i]);
						}
					}
					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("DEBUG Scale")) {
					float dpiScale = Imgui_Core_GetDpiScale();
					if(ImGui::MenuItem("1", nullptr, dpiScale == 1.0f)) {
						Imgui_Core_SetDpiScale(1.0f);
					}
					if(ImGui::MenuItem("1.25", nullptr, dpiScale == 1.25f)) {
						Imgui_Core_SetDpiScale(1.25f);
					}
					if(ImGui::MenuItem("1.5", nullptr, dpiScale == 1.5f)) {
						Imgui_Core_SetDpiScale(1.5f);
					}
					if(ImGui::MenuItem("1.75", nullptr, dpiScale == 1.75f)) {
						Imgui_Core_SetDpiScale(1.75f);
					}
					if(ImGui::MenuItem("2", nullptr, dpiScale == 2.0f)) {
						Imgui_Core_SetDpiScale(2.0f);
					}
					ImGui::EndMenu();
				}
				Fonts_Menu();
				if(ImGui::Checkbox("Docking", &g_config.bDocking)) {
					ImGuiIO &io = ImGui::GetIO();
					if(g_config.bDocking) {
						io.ConfigFlags |= (ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable);
					} else {
						io.ConfigFlags &= ~(ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable);
					}
					config_write(&g_config);
					UITabs_SetRedockAll();
				}
				UIChangeset_Menu();
				if(ImGui::BeginMenu("Imgui Help")) {
					ImGui::MenuItem("Demo", nullptr, &s_showImguiDemo);
					ImGui::MenuItem("About", nullptr, &s_showImguiAbout);
					ImGui::MenuItem("Metrics", nullptr, &s_showImguiMetrics);
					ImGui::MenuItem("User Guide", nullptr, &s_showImguiUserGuide);
					ImGui::MenuItem("Style Editor", nullptr, &s_showImguiStyleEditor);
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		Update_Menu();
		UIClientspec_MenuBar();
		ImGui::EndMainMenuBar();
	}
}

void p4t_update(void)
{
	tasks_tick();
	p4_update();
	p4t_menubar();
	Update_Tick();

	if(s_showImguiDemo) {
		ImGui::ShowDemoWindow(&s_showImguiDemo);
	}
	if(s_showImguiAbout) {
		ImGui::ShowAboutWindow(&s_showImguiAbout);
	}
	if(s_showImguiMetrics) {
		ImGui::ShowMetricsWindow(&s_showImguiMetrics);
	}
	if(s_showImguiUserGuide) {
		ImGui::ShowUserGuide();
	}
	if(s_showImguiStyleEditor) {
		ImGui::ShowStyleEditor();
	}

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	float startY = ImGui::GetFrameHeight();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiCond_Always);
	if((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0) {
		ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiCond_Always);
		bool open = true;
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
	} else {
		bool open = true;
		ImVec2 ViewportPos = ImGui::GetWindowViewport()->Pos;
		ImGui::SetNextWindowPos(ImVec2(ViewportPos.x, ViewportPos.y + startY), ImGuiCond_Always);
		if(ImGui::Begin("mainwindow", &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
			UITabs_Update();
		}
		ImGui::End();
	}

	UIOutput_Update();
}
