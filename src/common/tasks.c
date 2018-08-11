// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "tasks.h"
#include "bb_array.h"
#include "bb_wrap_stdio.h"
#include "va.h"

static tasks s_tasks;
static taskId s_lastId;

static const char *s_taskStateNames[] = {
	"kTaskState_Pending",
	"kTaskState_Running",
	"kTaskState_Succeeded",
	"kTaskState_Failed",
	"kTaskState_Canceled",
	"kTaskState_Count"
};
BB_CTASSERT(BB_ARRAYSIZE(s_taskStateNames) == kTaskState_Count + 1);

static const char *task_get_name(task *t)
{
	return va("^=%s^7", sb_get(&t->name));
}

static void task_reset(task *t)
{
	BB_LOG("task::task_reset", "reset %s", task_get_name(t));
	if(t->reset) {
		t->reset(t);
	}
	for(u32 i = 0; i < t->subtasks.count; ++i) {
		task *s = t->subtasks.data + i;
		task_reset(s);
	}
	bba_free(t->subtasks);
	sdict_reset(&t->extraData);
	sb_reset(&t->name);
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

static void task_prep(task *t, const char *prefix)
{
	if(!t->id) {
		t->id = ++s_lastId;
	}
	const char *name = va("%s%u:%s", prefix, t->id, sb_get(&t->name));
	sb_reset(&t->name);
	sb_append(&t->name, name);
	sb_t subPrefix = { 0 };
	sb_va(&subPrefix, "%s%u:", prefix, t->id);
	for(u32 i = 0; i < t->subtasks.count; ++i) {
		task_prep(t->subtasks.data + i, sb_get(&subPrefix));
		t->subtasks.data[i].parent = t;
	}
	sb_reset(&subPrefix);
}

void task_set_state(task *t, taskState state)
{
	if(t->state != state) {
		BB_LOG("task::task_set_state", "^<%s^7 -> ^:%s^7 %s",
		       s_taskStateNames[t->state], s_taskStateNames[state], task_get_name(t));
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
			task_prep(&t, "");
			bba_last(s_tasks) = t;
			BB_LOG("task::task_queue", "queued %s", task_get_name(&t));
			return &bba_last(s_tasks);
		}
	}

	BB_ERROR("task::task_queue", "failed to queue %s", task_get_name(&t));
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
	for(u32 i = 0; i < s_tasks.count;) {
		task *t = s_tasks.data + i;
		if(task_started(t)) {
			if(task_done(t)) {
				task_reset(t);
				bba_erase(s_tasks, i);
			} else {
				t->tick(t);
				++active;
				++i;
			}
		} else {
			++i;
		}
	}

	if(!active || 1) {
		for(u32 i = 0; i < s_tasks.count; ++i) {
			task *t = s_tasks.data + i;
			if(!task_started(t)) {
				task_set_state(t, kTaskState_Running);
			}
		}
	}
}
