// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_output.h"

#include "imgui_utils.h"
#include "output.h"

void UIOutput_Update()
{
	for(u32 i = 0; i < g_output.count; ++i) {
		ImGui::TextUnformatted(sb_get(&g_output.data[i].text));
	}
}
