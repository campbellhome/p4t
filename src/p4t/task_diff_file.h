// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void p4_diff_against_local(const char *depotPath, const char *rev, const char *localPath, bool depotFirst);
void p4_diff_against_depot(const char *depotPath1, const char *rev1, const char *depotPath2, const char *rev2);
void p4_diff_shutdown(void);

#if defined(__cplusplus)
}
#endif
