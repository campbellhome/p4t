// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

sb_t path_resolve(sb_t src);
void path_resolve_inplace(sb_t *path);
bool path_mkdir(const char *path);
bool path_rmdir(const char *path); // non-recursive, must be empty

#if defined(__cplusplus)
}
#endif
