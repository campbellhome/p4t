// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "ui_icons.h"
#include "imgui_utils.h"
#include "sdict.h"

struct uiIconExtension {
	const char *extension;
	const char *icon;
};
static uiIconExtension s_exts[] = {
	{ "cpp", ICON_FK_FILE_TEXT },
	{ "c", ICON_FK_FILE_TEXT },
	{ "h", ICON_FK_FILE_TEXT },
	{ "js", ICON_FK_FILE_TEXT },

	{ "bat", ICON_FK_FILE_TEXT },
	{ "txt", ICON_FK_FILE_TEXT },
	{ "cfg", ICON_FK_FILE_TEXT },
	{ "ini", ICON_FK_FILE_TEXT },
	{ "json", ICON_FK_FILE_TEXT },
	{ "xml", ICON_FK_FILE_TEXT },

	{ "gif", ICON_FK_FILE },
	{ "jpg", ICON_FK_FILE },
	{ "jpeg", ICON_FK_FILE },
	{ "tga", ICON_FK_FILE },
	{ "tif", ICON_FK_FILE },
	{ "tiff", ICON_FK_FILE },
	{ "psd", ICON_FK_FILE },

	{ "pdf", ICON_FK_FILE },
	{ "xls", ICON_FK_FILE },
	{ "xlsx", ICON_FK_FILE },
	{ "doc", ICON_FK_FILE },
	{ "docx", ICON_FK_FILE },
};

// ICON_FK_FILE
// ICON_FK_FILE_TEXT

// ICON_FK_FILE_O
// ICON_FK_FILE_TEXT_O
// ICON_FK_FILE_PDF_O
// ICON_FK_FILE_WORD_O
// ICON_FK_FILE_EXCEL_O
// ICON_FK_FILE_POWERPOINT_O
// ICON_FK_FILE_IMAGE_O
// ICON_FK_FILE_ARCHIVE_O
// ICON_FK_FILE_AUDIO_O
// ICON_FK_FILE_VIDEO_O
// ICON_FK_FILE_CODE_O

const char *UIIcons_ClassifyFile(const char *depotPath, const char *filetype)
{
	if(filetype) {
		if(strstr(filetype, "text")) {
			return ICON_FK_FILE_TEXT;
		} else {
			return ICON_FK_FILE;
		}
	}

	const char *ext = depotPath ? strrchr(depotPath, '.') : nullptr;
	if(ext) {
		++ext;
	}
	for(u32 i = 0; i < BB_ARRAYSIZE(s_exts); ++i) {
		if(!strcmp(s_exts[i].extension, ext))
			return s_exts[i].icon;
	}
	return ICON_FK_FILE;
}
