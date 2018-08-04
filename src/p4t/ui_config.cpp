// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_config.h"
#include "imgui_themes.h"
#include "imgui_utils.h"

#include "bb_array.h"
#include "bb_string.h"

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
	ImGui::Style_Reset();
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
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);

	if(ImGui::Begin("Configuration", &s_configOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if(ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputFloat("Double-click seconds", &s_uiConfig->doubleClickSeconds);
			ImGui::Checkbox("Single instance check", &s_uiConfig->singleInstanceCheck);
			ImGui::Checkbox("Single instance prompt", &s_uiConfig->singleInstancePrompt);

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
			ImGui::Checkbox("DPI Aware", &s_uiConfig->dpiAware);
			if(ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Requires restart.  Default font is not recommended if DPI Aware.");
			}
		}
		if(ImGui::CollapsingHeader("Font", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginGroup();
			ImGui::PushID("UIFont");
			ImGui::Checkbox("Custom UI Font", &s_uiConfig->uiFontConfig.enabled);
			if(s_uiConfig->uiFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_uiConfig->uiFontConfig.size;
				ImGui::InputInt("##size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_uiConfig->uiFontConfig.size = (u32)val;
				ImGui::TextUnformatted("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_uiConfig->uiFontConfig.path, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::PopID();
			ImGui::EndGroup();
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::PushID("LogFont");
			ImGui::Checkbox("Custom Log Font", &s_uiConfig->logFontConfig.enabled);
			if(s_uiConfig->logFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_uiConfig->logFontConfig.size;
				ImGui::InputInt("size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_uiConfig->logFontConfig.size = (u32)val;
				ImGui::Text("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_uiConfig->logFontConfig.path, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::EndGroup();
			ImGui::PopID();
		}
		if(ImGui::CollapsingHeader("Diff", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID("Diff");
			ImGui::Checkbox("External Program", &s_uiConfig->diff.enabled);
			if(s_uiConfig->diff.enabled) {
				ImGui::Text("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(600.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_uiConfig->diff.path, 1024, flags);
				ImGui::PopItemWidth();
				ImGui::Text("Arguments:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGui::InputText("##args", &s_uiConfig->diff.args, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::PopID();
		}
		if(ImGui::CollapsingHeader("Perforce", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID("Perforce");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::TextUnformatted("Changelists to fetch at a time:");
			ImGui::SameLine();
			int val = (int)s_uiConfig->p4.changelistBlockSize;
			ImGui::InputInt("##changelistBlockSize", &val, 100, 1000);
			val = BB_CLAMP(val, 0, 10000);
			s_uiConfig->p4.changelistBlockSize = (u32)val;
			ImGui::SameLine();
			ImGui::TextUnformatted("(0 fetches all)");
			ImGui::PopID();
		}
		ImGui::Separator();
		if(ImGui::Button("Ok")) {
			config_t tmp = *config;
			*config = *s_uiConfig;
			*s_uiConfig = tmp;
			s_configOpen = false;
			ImGui::GetIO().MouseDoubleClickTime = config->doubleClickSeconds;
			QueueUpdateDpiDependentResources();
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			s_configOpen = false;
			UIConfig_ApplyColorscheme();
		}
	}
	ImGui::End();
	if(!s_configOpen) {
		config_free(s_uiConfig);
		s_uiConfig = nullptr;
	}
}
