// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4.h"

#include "app.h"
#include "env_utils.h"
#include "file_utils.h"
#include "output.h"
#include "p4_task.h"
#include "span.h"
#include "str.h"
#include "tokenize.h"
#include "va.h"

#include "bb.h"
#include "bb_array.h"

#include <stdlib.h>

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
static p4_t p4;

static const char *p4_exe(void)
{
	return sb_get(&p4.exe);
}

static const char *p4_dir(void)
{
	return "D:\\Backups\\MattC_Home\\Projects";
}

b32 p4_init(void)
{
	sb_t temp = env_get("PATH");
	const char *cursor = sb_get(&temp);
	span_t token = tokenize(&cursor, ";");
	while(token.start) {
		sb_t path;
		sb_init(&path);
		sb_va(&path, "%.*s\\p4.exe", token.end - token.start, token.start);
		if(file_readable(sb_get(&path))) {
			p4.exe = path;
			break;
		} else {
			sb_reset(&path);
		}
		token = tokenize(&cursor, ";");
	}
	sb_reset(&temp);

	if(p4.exe.count) {
		output_log("Using %s\n", p4_exe());
		p4_info();
		return true;
	} else {
		output_error("Failed to find p4.exe\n");
		return false;
	}
}

void p4_shutdown(void)
{
	sb_reset(&p4.exe);
	sdict_reset(&p4.info);
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		sdict_reset(&p4.changelists.data[i].normal);
		sdict_reset(&p4.changelists.data[i].shelved);
	}
	bba_free(p4.changelists);
}

p4Changelist *p4_find_changelist(u32 cl)
{
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		p4Changelist *change = p4.changelists.data + i;
		if(change->number == cl) {
			return change;
		}
	}
	return NULL;
}

sdict_t *p4_get_info(void)
{
	return &p4.info;
}

static void task_p4info_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t;
		if(t->dicts.count == 1) {
			sdict_move(&p4.info, t->dicts.data);
		}
	}
}
void p4_info(void)
{
	p4_task_queue(task_p4info_statechanged, p4_dir(), "\"%s\" -G info", p4_exe());
}

void p4_changes(void)
{
	p4_task_queue(task_process_statechanged, p4_dir(), "\"%s\" -G changes", p4_exe());
}

static void task_p4describe_statechanged(task *_t)
{
	if(_t->state == kTaskState_Succeeded) {
		if(_t->subtasks.count == 2) {
			task *s = _t->subtasks.data + 0;
			task *s2 = _t->subtasks.data + 1;
			task_p4 *t = (task_p4 *)s->userdata;
			task_p4 *t2 = (task_p4 *)s2->userdata;
			if(t->dicts.count == 1) {
				const char *change = sdict_find(t->dicts.data, "change");
				if(change) {
					p4Changelist cl = { 0 };
					cl.number = strtou32(change);
					sdict_move(&cl.normal, t->dicts.data);
					if(t2->dicts.count == 1) {
						sdict_move(&cl.shelved, t2->dicts.data);
					}
					p4Changelist *existing = p4_find_changelist(cl.number);
					if(existing) {
						sdict_reset(&existing->normal);
						sdict_reset(&existing->shelved);
						*existing = cl;
					} else if(bba_add_noclear(p4.changelists, 1)) {
						bba_last(p4.changelists) = cl;
					} else {
						sdict_reset(&cl.normal);
						sdict_reset(&cl.shelved);
					}
				}
			}
		}
	}
}
void p4_describe_changelist(u32 cl)
{
	p4Changelist *changelist = p4_find_changelist(cl);
	if(!changelist || strcmp(sdict_find_safe(&changelist->normal, "status"), "submitted")) {
		task t = { 0 };
		t.tick = task_tick_subtasks;
		t.stateChanged = task_p4describe_statechanged;
		task t1 = p4_task_create(task_process_statechanged, p4_dir(), "\"%s\" -G describe -s %u", p4_exe(), cl);
		bba_push(t.subtasks, t1);
		task t2 = p4_task_create(task_process_statechanged, p4_dir(), "\"%s\" -G describe -s -S %u", p4_exe(), cl);
		bba_push(t.subtasks, t2);
		task_queue(t);
	}
}
