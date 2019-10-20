// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

typedef struct tag_p4UIChangeset p4UIChangeset;

void UIChangeset_Menu();
void UIChangeset_Update(p4UIChangeset *uics);
void UIChangeset_SetWindowTitle(p4UIChangeset *uics);
