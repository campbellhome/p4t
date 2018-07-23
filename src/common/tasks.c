// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "tasks.h"
#include "bb_array.h"

#include "bb_wrap_stdio.h"

static tasks s_tasks;
static taskId s_lastId;

static void task_reset(task *t)
{
	BB_LOG("tasks", "reset task %u", t->id);
	if(t->reset) {
		t->reset(t);
	}
	for(u32 i = 0; i < t->subtasks.count; ++i) {
		task *s = t->subtasks.data + i;
		task_reset(s);
	}
	bba_free(t->subtasks);
}

void tasks_startup(void)
{
}

void tasks_shutdown(void)
{
	for(u32 i = 0; i < s_tasks.count; ++i) {
		task *t = s_tasks.data + i;
		task_reset(t);
	}
	bba_free(s_tasks);
}

static void tasks_setid(task *t)
{
	t->id = ++s_lastId;
	for(u32 i = 0; i < t->subtasks.count; ++i) {
		tasks_setid(t->subtasks.data + i);
	}
}

void task_set_state(task *t, taskState state)
{
	if(t->state != state) {
		t->prevState = t->state;
		t->state = state;
		if(t->stateChanged) {
			t->stateChanged(t);
		}
	}
}

task *task_queue(task t)
{
	if(t.tick) {
		if(bba_add_noclear(s_tasks, 1)) {
			tasks_setid(&t);
			bba_last(s_tasks) = t;
			BB_LOG("tasks", "queued task %u", t.id);
			return &bba_last(s_tasks);
		}
	}

	BB_ERROR("tasks", "failed to queue task");
	task_reset(&t);
	return NULL;
}

void task_tick_subtasks(task *t)
{
	if(t->subtasks.count) {
		u32 states[kTaskState_Count] = { 0 };
		for(u32 i = 0; i < t->subtasks.count; ++i) {
			task *s = t->subtasks.data + i;
			if(!task_started(s)) {
				if(t->parallel || !states[kTaskState_Running]) {
					task_set_state(s, kTaskState_Running);
				}
			}
			if(task_started(s) && !task_done(s)) {
				s->tick(s);
			}
			states[s->state]++;
		}
		if(!states[kTaskState_Pending] && !states[kTaskState_Running]) {
			// all children are done - finish current task too
			if(states[kTaskState_Canceled]) {
				task_set_state(t, kTaskState_Canceled);
			} else if(states[kTaskState_Failed]) {
				task_set_state(t, kTaskState_Failed);
			} else if(states[kTaskState_Succeeded]) {
				task_set_state(t, kTaskState_Succeeded);
			}
		}
	}
}

void tasks_tick(void)
{
	u32 active = 0;
	for(u32 i = s_tasks.count - 1; i < s_tasks.count; --i) {
		task *t = s_tasks.data + i;
		if(task_started(t)) {
			if(task_done(t)) {
				task_reset(t);
				bba_erase(s_tasks, i);
			} else {
				t->tick(t);
				++active;
			}
		}
	}

	if(!active) {
		for(u32 i = 0; i < s_tasks.count; ++i) {
			task *t = s_tasks.data + i;
			if(!task_started(t)) {
				task_set_state(t, kTaskState_Running);
			}
		}
	}
}
