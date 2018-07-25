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

typedef struct tag_p4Changelist {
	sdict_t normal;
	sdict_t shelved;
	sdicts normalFiles;
	sdicts shelvedFiles;
	u32 number;
	u32 parity;
} p4Changelist;

typedef struct tag_p4Changelists {
	u32 count;
	u32 allocated;
	p4Changelist *data;
} p4Changelists;

typedef struct tag_p4 {
	sb_t exe;

	sdict_t info;
	p4Changelists changelists;
} p4_t;
extern p4_t p4;

const char *p4_exe(void);
const char *p4_dir(void);

//////////////////////////////////////////////////////////////////////////

// general: code(stat), change, user, client, time, desc, status, changeType, path, shelved
// per-file: depotFile0, action0, type0, rev0, fileSize0, digest0
p4Changelist *p4_find_changelist(u32 cl);

sdict_t *p4_get_info(void);

//////////////////////////////////////////////////////////////////////////

void p4_info(void);
void p4_changes(void);

#include "task_describe_changelist.h"
#include "task_diff_file.h"

#if defined(__cplusplus)
}
#endif
