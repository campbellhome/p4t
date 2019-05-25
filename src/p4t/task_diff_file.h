// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tag_p4FileLocator p4FileLocator;
void p4_diff_against_local(const char *depotPath, const char *rev, const char *localPath, b32 depotFirst);
void p4_diff_against_depot(const char *depotPath1, const char *rev1, const char *depotPath2, const char *rev2);
void p4_diff_file_locators(const p4FileLocator *locator1, const p4FileLocator *locator2);
void p4_diff_shutdown(void);

#if defined(__cplusplus)
}
#endif
