// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "process_task.h"

#include "bb_array.h"

void task_process_tick(task *_t)
{
	task_process *t = (task_process *)_t->userdata;
	processTickResult_t res = process_tick(t->process);
	if(res.stdoutIO.nBytes) {
		bba_add_array(t->stdoutBuf, res.stdoutIO.buffer, res.stdoutIO.nBytes);
	}
	if(res.stderrIO.nBytes) {
		bba_add_array(t->stderrBuf, res.stderrIO.buffer, res.stderrIO.nBytes);
	}
	if(res.done) {
		task_set_state(_t, t->stderrBuf.count ? kTaskState_Failed : kTaskState_Succeeded);
	}
	task_tick_subtasks(_t);
}

void task_process_statechanged(task *_t)
{
	task_process *t = (task_process *)_t->userdata;
	if(_t->state == kTaskState_Running) {
		t->process = process_spawn(sb_get(&t->dir), sb_get(&t->cmdline), t->spawnType).process;
		if(!t->process) {
			task_set_state(_t, kTaskState_Failed);
		}
	}
}

void task_process_reset(task *_t)
{
	task_process *t = (task_process *)_t->userdata;
	sb_reset(&t->dir);
	sb_reset(&t->cmdline);
	sb_reset(&t->stdoutBuf);
	sb_reset(&t->stderrBuf);
	if(t->process) {
		process_free(t->process);
		t->process = NULL;
	}
	free(_t->userdata);
	_t->userdata = NULL;
}
