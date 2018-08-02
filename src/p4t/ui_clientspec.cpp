// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_clientspec.h"
#include "config.h"
#include "imgui_utils.h"
#include "p4.h"
#include "sdict.h"
#include "va.h"

const char *UIClientspec_DefaultClientspecName()
{
	const char *clientName = sdict_find(&p4.info, "clientName");
	if(clientName) {
		return va("%s (default)", clientName);
	} else {
		return "unknown";
	}
}

const char *UIClientspec_ClientspecName(u32 i)
{
	return sdict_find_safe(p4.localClients.data + i, "client");
}

const char *UIClientspec_ActiveClientspecName()
{
	const char *configClientspec = sb_get(&g_config.clientspec);
	for(u32 i = 0; i < p4.localClients.count; ++i) {
		if(!strcmp(configClientspec, sdict_find_safe(p4.localClients.data + i, "client"))) {
			return configClientspec;
		}
	}

	return UIClientspec_DefaultClientspecName();
}

void UIClientspec_MenuBar()
{
	ImGui::SameLine(0.0f, 20.0f * g_config.dpiScale);
	ImGui::Text("User: %s   Clientspec:", sdict_find_safe(&p4.info, "userName"));
	ImGui::SameLine();

	if(ImGui::BeginMenu(UIClientspec_ActiveClientspecName())) {
		if(ImGui::MenuItem(UIClientspec_DefaultClientspecName())) {
			sb_reset(&g_config.clientspec);
		}
		for(u32 i = 0; i < p4.localClients.count; ++i) {
			const char *clientspec = sdict_find_safe(p4.localClients.data + i, "client");
			if(ImGui::MenuItem(clientspec)) {
				sb_reset(&g_config.clientspec);
				sb_append(&g_config.clientspec, clientspec);
			}
		}
		ImGui::EndMenu();
	}
}
