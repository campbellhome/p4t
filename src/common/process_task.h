// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "tasks.h"
#include "process.h"
#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tag_task_process {
	task header;
	process_t *process;
	sb_t dir;
	sb_t cmdline;
	sb_t stdoutBuf;
	sb_t stderrBuf;
	processSpawnType_t spawnType;
	u8 pad[4];
} task_process;

void task_process_tick(task *);
void task_process_statechanged(task *);
void task_process_reset(task *);

#if defined(__cplusplus)
}
#endif
