// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void p4_describe_changelist(u32 cl);
void p4_describe_default_changelist(const char *client);
b32 p4_describe_task_count(void);

#if defined(__cplusplus)
}
#endif
