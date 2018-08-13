// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "thread_task.h"
#include <stdlib.h>

void task_thread_tick(task *t)
{
	task_thread *th = t->userdata;
	if(th && th->handle) {
		if(th->threadDesiredState > kTaskState_Running && t->state != th->threadDesiredState) {
			task_set_state(t, th->threadDesiredState);
		}
	}
}

void task_thread_statechanged(task *t)
{
	if(t->state == kTaskState_Running) {
		task_thread *th = t->userdata;
		th->handle = bbthread_create(th->func, th);
	}
}

void task_thread_reset(task *t)
{
	task_thread *th = t->userdata;
	if(th && th->handle) {
		th->shouldTerminate = true;
		bbthread_join(th->handle);
	}
}

task thread_task_create(const char *name, Task_StateChanged statechanged, bb_thread_func func, void *data)
{
	task t = { 0 };
	sb_append(&t.name, name);
	t.tick = task_thread_tick;
	t.stateChanged = statechanged ? statechanged : task_thread_statechanged;
	t.reset = task_thread_reset;
	t.userdata = malloc(sizeof(task_thread));
	if(t.userdata) {
		task_thread *th = t.userdata;
		memset(th, 0, sizeof(*th));
		th->func = func;
		th->data = data;
	}
	return t;
}
