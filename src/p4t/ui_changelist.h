// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sdict.h"

typedef struct tag_p4Changelist p4Changelist;
typedef struct tag_uiChangelistFiles uiChangelistFiles;

// for use in other UIs
void UIChangelist_DrawSingleLine(sdict_t *cl);
void UIChangelist_DrawInformation(sdict_t *cl);
void UIChangelist_DrawFiles(uiChangelistFiles *files, p4Changelist *cl, uiChangelistFiles *otherFiles = nullptr, float indent = 0.0f);
void UIChangelist_DrawFilesAndHeaders(p4Changelist *cl, uiChangelistFiles *normalFiles, uiChangelistFiles *shelvedFiles, b32 shelvedOpenByDefault, float indent = 0.0f);

// for standalone CL viewer
void UIChangelist_Shutdown(void);
void UIChangelist_Update(void);

void UIChangelist_EnterChangelist(void);
void UIChangelist_InitChangelist(u32 id);
