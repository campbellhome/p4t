// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_config.h"
#include "bb_array.h"
#include "bb_string.h"
#include "imgui_core.h"
#include "imgui_themes.h"
#include "imgui_utils.h"

static config_t *s_uiConfig;
static config_t s_config;
bool s_configValid;
bool s_configOpen;
bool s_configNeedFocus;

void UIConfig_Open(config_t *config)
{
	if(s_configValid) {
		config_free(&s_config);
		s_configValid = false;
	}

	s_config = config_clone(config);
	s_configValid = true;
	s_configOpen = true;
	s_configNeedFocus = true;
}

void UIConfig_Reset()
{
	if(s_configValid) {
		config_free(&s_config);
		s_configOpen = false;
		s_configValid = false;
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
	const char *colorscheme = sb_get(&config->colorscheme);
	Style_Apply(colorscheme);
}

void UIConfig_Update(config_t *config)
{
	float startY = ImGui::GetItemsLineHeightWithSpacing();
	ImGuiIO &io = ImGui::GetIO();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(0, startY), ImGuiCond_Always);

	if(ImGui::Begin("Configuration", &s_configOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if(ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputFloat("Double-click seconds", &s_config.doubleClickSeconds);
			ImGui::Checkbox("Single instance check", &s_config.singleInstanceCheck);
			ImGui::Checkbox("Single instance prompt", &s_config.singleInstancePrompt);

			int colorschemeIndex = -1;
			for(int i = 0; i < BB_ARRAYSIZE(s_colorschemes); ++i) {
				if(!strcmp(sb_get(&s_config.colorscheme), s_colorschemes[i])) {
					colorschemeIndex = i;
					break;
				}
			}
			if(ImGui::Combo("Colorscheme", &colorschemeIndex, s_colorschemes, BB_ARRAYSIZE(s_colorschemes))) {
				if(colorschemeIndex >= 0 && colorschemeIndex < BB_ARRAYSIZE(s_colorschemes)) {
					sb_reset(&s_config.colorscheme);
					sb_append(&s_config.colorscheme, s_colorschemes[colorschemeIndex]);
					UIConfig_ApplyColorscheme(&s_config);
				}
			}
			ImGui::Checkbox("DPI Aware", &s_config.dpiAware);
			if(ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Requires restart.  Default font is not recommended if DPI Aware.");
			}
		}
		if(ImGui::CollapsingHeader("Font", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginGroup();
			ImGui::PushID("UIFont");
			ImGui::Checkbox("Custom UI Font", &s_config.uiFontConfig.enabled);
			if(s_config.uiFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_config.uiFontConfig.size;
				ImGui::InputInt("##size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_config.uiFontConfig.size = (u32)val;
				ImGui::TextUnformatted("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_config.uiFontConfig.path, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::PopID();
			ImGui::EndGroup();
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::PushID("LogFont");
			ImGui::Checkbox("Custom Log Font", &s_config.logFontConfig.enabled);
			if(s_config.logFontConfig.enabled) {
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::TextUnformatted("Font size:");
				ImGui::SameLine();
				int val = (int)s_config.logFontConfig.size;
				ImGui::InputInt("size", &val, 1, 10);
				val = BB_CLAMP(val, 1, 1024);
				s_config.logFontConfig.size = (u32)val;
				ImGui::Text("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_config.logFontConfig.path, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::EndGroup();
			ImGui::PopID();
		}
		if(ImGui::CollapsingHeader("Diff", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID("Diff");
			ImGui::Checkbox("External Program", &s_config.diff.enabled);
			if(s_config.diff.enabled) {
				ImGui::Text("Path:");
				ImGui::SameLine();
				ImGui::PushItemWidth(600.0f * g_config.dpiScale);
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				ImGui::InputText("##path", &s_config.diff.path, 1024, flags);
				ImGui::PopItemWidth();
				ImGui::Text("Arguments:");
				ImGui::SameLine();
				ImGui::PushItemWidth(300.0f * g_config.dpiScale);
				ImGui::InputText("##args", &s_config.diff.args, 1024, flags);
				ImGui::PopItemWidth();
			}
			ImGui::PopID();
		}
		if(ImGui::CollapsingHeader("Perforce", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushID("Perforce");
			ImGui::AlignFirstTextHeightToWidgets();
			ImGui::TextUnformatted("Changelists to fetch at a time:");
			ImGui::SameLine();
			int val = (int)s_config.p4.changelistBlockSize;
			ImGui::InputInt("##changelistBlockSize", &val, 100, 1000);
			val = BB_CLAMP(val, 0, 10000);
			s_config.p4.changelistBlockSize = (u32)val;
			ImGui::SameLine();
			ImGui::TextUnformatted("(0 fetches all)");
			ImGui::PopID();
		}
		ImGui::Separator();
		if(ImGui::Button("Ok")) {
			config_t tmp = *config;
			*config = s_config;
			s_config = tmp;
			s_configOpen = false;
			ImGui::GetIO().MouseDoubleClickTime = config->doubleClickSeconds;
			Imgui_Core_QueueUpdateDpiDependentResources();
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			s_configOpen = false;
			UIConfig_ApplyColorscheme();
		}
	}
	ImGui::End();
	if(!s_configOpen && s_configValid) {
		config_reset(&s_config);
		s_configOpen = false;
		s_configValid = false;
	}
}
