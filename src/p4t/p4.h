// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

b32 p4_init(void);
void p4_shutdown(void);
void p4_tick(void);

typedef enum tag_p4Operation {
	kP4Op_Info,
	kP4Op_Changes,
} p4Operation;

void p4_info(void);
void p4_changes(void);

#if defined(__cplusplus)
}
#endif
