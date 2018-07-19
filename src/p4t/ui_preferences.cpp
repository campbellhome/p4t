// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_preferences.h"
#include "imgui_utils.h"

#include "bb_array.h"
#include "bb_string.h"

using namespace ImGui;
void QueueUpdateDpiDependentResources();

static config_t *s_preferencesConfig;
bool s_preferencesOpen;
bool s_preferencesNeedFocus;

void Preferences_Open(config_t *config)
{
	if(s_preferencesConfig) {
		config_free(s_preferencesConfig);
	}

	s_preferencesConfig = config_clone(config);
	s_preferencesOpen = s_preferencesConfig != nullptr;
	s_preferencesNeedFocus = true;
}

void Preferences_Reset()
{
	if(s_preferencesConfig) {
		config_free(s_preferencesConfig);
		s_preferencesConfig = nullptr;
		s_preferencesOpen = false;
	}
}

bool Preferences_IsOpen()
{
	return s_preferencesOpen;
}

void Preferences_Update(config_t *config)
{
	if(!s_preferencesOpen)
		return;
	//ImGui::SetNextWindowContentWidth(580.0f);
	//if(s_preferencesNeedFocus) {
	//	ImGui::SetNextWindowFocus();
	//	s_preferencesNeedFocus = false;
	//}

	float startY = ImGui::GetItemsLineHeightWithSpacing();
	ImGuiIO &io = ImGui::GetIO();
	SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
	SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);

	if(Begin("Preferences", &s_preferencesOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if(ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
			Checkbox("Auto-tile views", &s_preferencesConfig->autoTileViews);
			Checkbox("Alternate row background colors", &s_preferencesConfig->alternateRowBackground);
			InputFloat("Double-click seconds", &s_preferencesConfig->doubleClickSeconds);
		}
		if(ImGui::CollapsingHeader("Font", ImGuiTreeNodeFlags_DefaultOpen)) {
			Checkbox("DPI Aware", &s_preferencesConfig->dpiAware);
			ImGui::SameLine();
			ImGui::PushItemWidth(150.0f * g_config.dpiScale);
			InputFloat("DPI Scale", &s_preferencesConfig->dpiScale);
			ImGui::PopItemWidth();
			BeginGroup();
			PushID("UIFont");
			Checkbox("Custom UI Font", &s_preferencesConfig->uiFontConfig.enabled);
			if(s_preferencesConfig->uiFontConfig.enabled) {
				int val = (int)s_preferencesConfig->uiFontConfig.size;
				PushItemWidth(80.0f * g_config.dpiScale);
				InputInt("size", &val, 1, 10);
				PopItemWidth();
				val = BB_CLAMP(val, 1, 1024);
				s_preferencesConfig->uiFontConfig.size = (u32)val;
				Text("Path:");
				SameLine();
				PushItemWidth(300.0f * g_config.dpiScale);
				//ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				//InputText("##path", s_preferencesConfig->uiFontConfig.path, sizeof(s_preferencesConfig->uiFontConfig.path), flags);
				PopItemWidth();
			}
			PopID();
			EndGroup();
			SameLine();
			BeginGroup();
			PushID("LogFont");
			Checkbox("Custom Log Font", &s_preferencesConfig->logFontConfig.enabled);
			if(s_preferencesConfig->logFontConfig.enabled) {
				int val = (int)s_preferencesConfig->logFontConfig.size;
				PushItemWidth(80.0f * g_config.dpiScale);
				InputInt("size", &val, 1, 10);
				PopItemWidth();
				val = BB_CLAMP(val, 1, 1024);
				s_preferencesConfig->logFontConfig.size = (u32)val;
				Text("Path:");
				SameLine();
				PushItemWidth(300.0f * g_config.dpiScale);
				//ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
				//InputText("##path", s_preferencesConfig->logFontConfig.path, sizeof(s_preferencesConfig->logFontConfig.path), flags);
				PopItemWidth();
			}
			PopID();
			EndGroup();
		}
		if(ImGui::CollapsingHeader("Miscellaneous", ImGuiTreeNodeFlags_DefaultOpen)) {
			int val = (int)s_preferencesConfig->autoDeleteAfterDays;
			ImGui::Text("Auto-delete old sessions after");
			SameLine();
			InputInt("days (0 disables)", &val, 1, 10);
			val = BB_CLAMP(val, 0, 9999);
			s_preferencesConfig->autoDeleteAfterDays = (u32)val;
			Checkbox("Single instance check", &s_preferencesConfig->singleInstanceCheck);
			Checkbox("Single instance prompt", &s_preferencesConfig->singleInstancePrompt);
		}
		Separator();
		if(Button("Ok")) {
			WINDOWPLACEMENT wp = config->wp;
			config_t tmp = *config;
			*config = *s_preferencesConfig;
			config->wp = wp;
			*s_preferencesConfig = tmp;
			s_preferencesOpen = false;
			GetIO().MouseDoubleClickTime = config->doubleClickSeconds;
			QueueUpdateDpiDependentResources();
		}
		SameLine();
		if(Button("Cancel")) {
			s_preferencesOpen = false;
		}
	}
	End();
	if(!s_preferencesOpen) {
		config_free(s_preferencesConfig);
		s_preferencesConfig = nullptr;
	}
}
