// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#define ICON_CHANGELIST ICON_FK_CHEVRON_CIRCLE_UP
#define COLOR_PENDING_CHANGELIST_OTHER ImColor(128, 128, 128)
#define COLOR_PENDING_CHANGELIST_LOCAL ImColor(192, 64, 64)
#define COLOR_SUBMITTED_CHANGELIST ImColor(69, 96, 192)

#define ICON_FILE_OUT_OF_DATE ICON_FK_CARET_UP
#define COLOR_FILE_OUT_OF_DATE ImColor(256, 256, 96)

#define ICON_FILE_UNRESOLVED ICON_FK_QUESTION
#define COLOR_FILE_UNRESOLVED ImColor(192, 64, 64)

#define COLOR_FILE_SHELVED ImColor(127, 106, 0)

// unused, for future:
#define ICON_CLIENTSPEC ICON_FK_TASKS

const char *UIIcons_ClassifyFile(const char *depotPath, const char *filetype = nullptr);
const char *UIIcons_GetIconSpaces(const char *icon);
