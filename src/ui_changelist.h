// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sdict.h"

typedef struct tag_p4Changelist p4Changelist;
typedef struct tag_p4UIChangelist p4UIChangelist;
typedef struct tag_uiChangelistFiles uiChangelistFiles;

// for use in other UIs
void UIChangelist_DrawInformation(sdict_t *cl);
b32 UIChangelist_DrawFiles(uiChangelistFiles *files, p4Changelist *cl, float indent = 0.0f);
void UIChangelist_DrawFilesAndHeaders(p4Changelist *cl, uiChangelistFiles *normalFiles, uiChangelistFiles *shelvedFiles, b32 shelvedOpenByDefault, float indent = 0.0f);
void UIChangelist_SetWindowTitle(p4UIChangelist *uicl);
b32 UIChangelist_DrawFilesNoColumns(uiChangelistFiles *files, p4Changelist *cl, float indent);

// for standalone CL viewer
void UIChangelist_Shutdown(void);
void UIChangelist_Update(p4UIChangelist *uicl);

void UIChangelist_EnterChangelist(p4UIChangelist *uicl);
