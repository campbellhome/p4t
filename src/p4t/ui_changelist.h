// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sdict.h"

typedef struct tag_uiChangelistFile {
	union {
		char *str[6];
		struct {
			char *filename;
			char *rev;
			char *action;
			char *filetype;
			char *depotPath;
			char *localPath;
		} field;
	};
	bool selected;
	u8 pad[7];
} uiChangelistFile;

typedef struct tag_uiChangelistFiles {
	u32 count;
	u32 allocated;
	uiChangelistFile *data;
	u32 lastClickIndex;
	b32 active;
	b32 shelved;
	u32 sortColumn;
	b32 sortDescending;
	u8 pad[4];
} uiChangelistFiles;

// for use in other UIs
void UIChangelist_DrawSingleLine(sdict_t *cl);
void UIChangelist_DrawInformation(sdict_t *cl);
void UIChangelist_DrawFiles(uiChangelistFiles *files, struct tag_p4Changelist *cl, uiChangelistFiles *otherFiles = nullptr);

// for standalone CL viewer
void UIChangelist_Shutdown(void);
void UIChangelist_Update(void);

void UIChangelist_EnterChangelist(void);
