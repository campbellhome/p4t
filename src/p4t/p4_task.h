// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "process_task.h"
#include "sdict.h"
#include "py_parser.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tag_task_p4 {
	task_process base;
	sdicts dicts;
	pyParser parser;
} task_p4;

void task_p4_tick(task *);
void task_p4_reset(task *);
task p4_task_create(Task_StateChanged *statechanged, const char *dir, const char *cmdlineFmt, ...);
task *p4_task_queue(Task_StateChanged *statechanged, const char *dir, const char *cmdlineFmt, ...);

#if defined(__cplusplus)
}
#endif
