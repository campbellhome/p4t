// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_config.h"
#include "imgui_themes.h"
#include "imgui_utils.h"

#include "bb_array.h"
#include "bb_string.h"

using namespace ImGui;
void QueueUpdateDpiDependentResources();

static config_t *s_uiConfig;
bool s_configOpen;
bool s_configNeedFocus;

void UIConfig_Open(config_t *config)
{
	if(s_uiConfig) {
		config_free(s_uiConfig);
	}

	s_uiConfig = config_clone(config);
	s_configOpen = s_uiConfig != nullptr;
	s_configNeedFocus = true;
}

void UIConfig_Reset()
{
	if(s_uiConfig) {
		config_free(s_uiConfig);
		s_uiConfig = nullptr;
		s_configOpen = false;
	}
}

bool UIConfig_IsOpen()
{
	return s_configOpen;
}

static const char *s_colorschemes[] = {
	"ImGui Dark",
	"Light",
	"Classic",
	"Visual Studio Dark",
	"Windows",
};

void UIConfig_ApplyColorscheme(config_t *config)
{
	if(!config) {
		config = &g_config;
	}
	Style_Reset();
	const char *colorscheme = sb_get(&config->colorscheme);
	if(!strcmp(colorscheme, "Visual Studio Dark")) {
		ImGui::StyleColorsVSDark();
	} else if(!strcmp(colorscheme, "Classic")) {
		ImGui::StyleColorsClassic();
	} else if(!strcmp(colorscheme, "Light")) {
		ImGui::StyleColorsLight();
	} else if(!strcmp(colorscheme, "Windows")) {
		ImGui::StyleColorsWindows();
	} else /*if(!strcmp(colorscheme, "Dark"))*/ {
		ImGui::StyleColorsDark();
		// modal background for default Dark theme looks like a non-responding Window
		ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	}
}

void UIConfig_Update(config_t *config)
{
	float startY = ImGui::GetItemsLineHeightWithSpacing();
	ImGuiIO &io = ImGui::GetIO();
	SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
	SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);

	if(Begin("Configuration", &s_configOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if(ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
			InputFloat("Double-click seconds", &s_uiConfig->doubleClickSeconds);
			Checkbox("Single instance check", &s_uiConfig->singleInstanceCheck);
			Checkbox("Single instance prompt", &s_uiConfig->singleInstancePrompt);

			int colorschemeIndex = -1;
			for(int i = 0; i < BB_ARRAYSIZE(s_colorschemes); ++i) {
				if(!strcmp(sb_get(&s_uiConfig->colorscheme), s_colorschemes[i])) {
					colorschemeIndex = i;
					break;
				}
			}
			if(ImGui::Combo("Colorscheme", &colorschemeIndex, s_colorschemes, BB_ARRAYSIZE(s_colorschemes))) {
				if(colorschemeIndex >= 0 && colorschemeIndex < BB_ARRAYSIZE(s_colorschemes)) {
					sb_reset(&s_uiConfig->colorscheme);
					sb_append(&s_uiConfig->colorscheme, s_colorschemes[colorschemeIndex]);
					UIConfig_ApplyColorscheme(s_uiConfig);
				}
			}
			Checkbox("DPI Aware", &s_uiConfig->dpiAware);
			if(ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Requires restart.  Default font is not recommended if DPI Aware.");
			}
		}
		if(ImGui::CollapsingHeader("Font", ImGuiTreeNodeFlags_DefaultOpen)) {
			BeginGroup();
			PushID("UIFont");
			Checkbox("Custom UI Font", &s_uiConfig->uiFontConfig.enabled);
			if(s_uiConfig->uiFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_uiConfig->uiFontConfig.size;
				InputInt("##size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_uiConfig->uiFontConfig.size = (u32)val;
				TextUnformatted("Path:");
				SameLine();
				PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				InputText("##path", &s_uiConfig->uiFontConfig.path, 1024, flags);
				PopItemWidth();
			}
			PopID();
			EndGroup();
			SameLine();
			BeginGroup();
			PushID("LogFont");
			Checkbox("Custom Log Font", &s_uiConfig->logFontConfig.enabled);
			if(s_uiConfig->logFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_uiConfig->logFontConfig.size;
				InputInt("size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_uiConfig->logFontConfig.size = (u32)val;
				Text("Path:");
				SameLine();
				PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				InputText("##path", &s_uiConfig->logFontConfig.path, 1024, flags);
				PopItemWidth();
			}
			EndGroup();
			PopID();
		}
		if(ImGui::CollapsingHeader("Diff", ImGuiTreeNodeFlags_DefaultOpen)) {
			PushID("Diff");
			Checkbox("External Program", &s_uiConfig->diff.enabled);
			if(s_uiConfig->diff.enabled) {
				Text("Path:");
				SameLine();
				PushItemWidth(600.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				InputText("##path", &s_uiConfig->diff.path, 1024, flags);
				PopItemWidth();
				Text("Arguments:");
				SameLine();
				PushItemWidth(300.0f * g_config.dpiScale);
				InputText("##args", &s_uiConfig->diff.args, 1024, flags);
				PopItemWidth();
			}
			PopID();
		}
		if(ImGui::CollapsingHeader("Perforce", ImGuiTreeNodeFlags_DefaultOpen)) {
			PushID("Perforce");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::TextUnformatted("Changelists to fetch at a time:");
			ImGui::SameLine();
			int val = (int)s_uiConfig->p4.changelistBlockSize;
			InputInt("##changelistBlockSize", &val, 100, 1000);
			val = BB_CLAMP(val, 0, 10000);
			s_uiConfig->p4.changelistBlockSize = (u32)val;
			ImGui::SameLine();
			ImGui::TextUnformatted("(0 fetches all)");
			PopID();
		}
		Separator();
		if(Button("Ok")) {
			config_t tmp = *config;
			*config = *s_uiConfig;
			*s_uiConfig = tmp;
			s_configOpen = false;
			GetIO().MouseDoubleClickTime = config->doubleClickSeconds;
			QueueUpdateDpiDependentResources();
		}
		SameLine();
		if(Button("Cancel")) {
			s_configOpen = false;
			UIConfig_ApplyColorscheme();
		}
	}
	End();
	if(!s_configOpen) {
		config_free(s_uiConfig);
		s_uiConfig = nullptr;
	}
}
