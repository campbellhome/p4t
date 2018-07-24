// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "sdict.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sdict_s sdict_t;

b32 p4_init(void);
void p4_shutdown(void);

//////////////////////////////////////////////////////////////////////////

typedef struct tag_p4Changelist {
	sdict_t normal;
	sdict_t shelved;
	sdicts normalFiles;
	sdicts shelvedFiles;
	u32 number;
	u32 parity;
} p4Changelist;

// general: code(stat), change, user, client, time, desc, status, changeType, path, shelved
// per-file: depotFile0, action0, type0, rev0, fileSize0, digest0
p4Changelist *p4_find_changelist(u32 cl);

sdict_t *p4_get_info(void);

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
