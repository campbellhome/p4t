// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "fonts.h"
#include "common.h"
#include "wrap_imgui.h"

#if BB_USING(FEATURE_FREETYPE)

// warning C4548: expression before comma has no effect; expected expression with side-effect
// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
// warning C4255: 'FuncName': no function prototype given: converting '()' to '(void)'
// warning C4668: '_WIN32_WINNT_WINTHRESHOLD' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
// warning C4574: 'INCL_WINSOCK_API_TYPEDEFS' is defined to be '0': did you mean to use '#if INCL_WINSOCK_API_TYPEDEFS'?
// warning C4365: 'return': conversion from 'bool' to 'BOOLEAN', signed/unsigned mismatch
BB_WARNING_PUSH(4820 4255 4668 4574 4365)
#include "misc/freetype/imgui_freetype.cpp"
BB_WARNING_POP

#pragma comment(lib, "freetype.lib")

#endif // #if BB_USING(FEATURE_FREETYPE)

struct fontBuilder {
	bool useFreeType;
	bool rebuild;

	fontBuilder()
	{
#if BB_USING(FEATURE_FREETYPE)
		useFreeType = true;
#else  // #if BB_USING(FEATURE_FREETYPE)
		useFreeType = false;
#endif // #else // #if BB_USING(FEATURE_FREETYPE)
		rebuild = true;
	}

	// Call _BEFORE_ NewFrame()
	bool UpdateRebuild()
	{
		if(!rebuild)
			return false;
		ImGuiIO &io = ImGui::GetIO();
#if BB_USING(FEATURE_FREETYPE)
		if(useFreeType) {
			ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);
		} else
#endif // #if BB_USING(FEATURE_FREETYPE)
		{
			io.Fonts->Build();
		}
		rebuild = false;
		return true;
	}
};

static fontBuilder s_fonts;
void Fonts_MarkAtlasForRebuild()
{
	s_fonts.rebuild = true;
}

bool Fonts_UpdateAtlas()
{
	return s_fonts.UpdateRebuild();
}

void Fonts_Menu()
{
#if BB_USING(FEATURE_FREETYPE)
	if(ImGui::Checkbox("DEBUG Use FreeType", &s_fonts.useFreeType)) {
		Fonts_MarkAtlasForRebuild();
	}
#endif // #if BB_USING(FEATURE_FREETYPE)
}
