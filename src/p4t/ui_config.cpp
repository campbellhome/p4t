// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_config.h"
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

void UIConfig_Update(config_t *config)
{
	if(!s_configOpen)
		return;
	//ImGui::SetNextWindowContentWidth(580.0f);
	//if(s_configNeedFocus) {
	//	ImGui::SetNextWindowFocus();
	//	s_configNeedFocus = false;
	//}

	float startY = ImGui::GetItemsLineHeightWithSpacing();
	ImGuiIO &io = ImGui::GetIO();
	SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - startY), ImGuiSetCond_Always);
	SetNextWindowPos(ImVec2(0, startY), ImGuiSetCond_Always);

	if(Begin("Configuration", &s_configOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
		if(ImGui::CollapsingHeader("Interface", ImGuiTreeNodeFlags_DefaultOpen)) {
			InputFloat("Double-click seconds", &s_uiConfig->doubleClickSeconds);
			Checkbox("Single instance check", &s_uiConfig->singleInstanceCheck);
			Checkbox("Single instance prompt", &s_uiConfig->singleInstancePrompt);
		}
		if(ImGui::CollapsingHeader("Font", ImGuiTreeNodeFlags_DefaultOpen)) {
			Checkbox("DPI Aware", &s_uiConfig->dpiAware);
			ImGui::SameLine();
			ImGui::PushItemWidth(150.0f * g_config.dpiScale);
			InputFloat("DPI Scale", &s_uiConfig->dpiScale);
			ImGui::PopItemWidth();
			BeginGroup();
			PushID("UIFont");
			Checkbox("Custom UI Font", &s_uiConfig->uiFontConfig.enabled);
			if(s_uiConfig->uiFontConfig.enabled) {
				int val = (int)s_uiConfig->uiFontConfig.size;
				PushItemWidth(80.0f * g_config.dpiScale);
				InputInt("size", &val, 1, 10);
				PopItemWidth();
				val = BB_CLAMP(val, 1, 1024);
				s_uiConfig->uiFontConfig.size = (u32)val;
				Text("Path:");
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
				int val = (int)s_uiConfig->logFontConfig.size;
				PushItemWidth(80.0f * g_config.dpiScale);
				InputInt("size", &val, 1, 10);
				PopItemWidth();
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
		Separator();
		if(Button("Ok")) {
			WINDOWPLACEMENT wp = config->wp;
			config_t tmp = *config;
			*config = *s_uiConfig;
			config->wp = wp;
			*s_uiConfig = tmp;
			s_configOpen = false;
			GetIO().MouseDoubleClickTime = config->doubleClickSeconds;
			QueueUpdateDpiDependentResources();
		}
		SameLine();
		if(Button("Cancel")) {
			s_configOpen = false;
		}
	}
	End();
	if(!s_configOpen) {
		config_free(s_uiConfig);
		s_uiConfig = nullptr;
	}
}
