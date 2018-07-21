// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sdict_s sdict_t;

b32 p4_init(void);
void p4_shutdown(void);
void p4_tick(void);

//////////////////////////////////////////////////////////////////////////

// general: code(stat), change, user, client, time, desc, status, changeType, path
// per-file: depotFile0, action0, type0, rev0, fileSize0, digest0
sdict_t *p4_find_changelist(u32 cl);

//////////////////////////////////////////////////////////////////////////

typedef enum tag_p4Operation {
	kP4Op_Info,
	kP4Op_Changes,
	kP4Op_DescribeChangelist,
} p4Operation;

void p4_info(void);
void p4_changes(void);
void p4_describe_changelist(u32 cl);

#if defined(__cplusplus)
}
#endif
