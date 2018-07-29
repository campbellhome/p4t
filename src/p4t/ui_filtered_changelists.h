// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "sb.h"

typedef struct tab_filteredChangelists {
	sb_t user;
	sb_t clientspec;
	sb_t filter;
	b32 filterEnabled;
	b32 pending;
} filteredChangelists;

void UIFilteredChangelists_Reset(filteredChangelists *f);
void UIFilteredChangelists_Update(filteredChangelists *f);
