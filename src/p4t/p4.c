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

p4_t p4;

const char *p4_exe(void)
{
	return sb_get(&p4.exe);
}

const char *p4_dir(void)
{
	return "C:\\";
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

static void p4_reset_changelist(p4Changelist *cl)
{
	sdict_reset(&cl->normal);
	sdict_reset(&cl->shelved);
	sdicts_reset(&cl->normalFiles);
	sdicts_reset(&cl->shelvedFiles);
}

void p4_shutdown(void)
{
	sb_reset(&p4.exe);
	sdict_reset(&p4.info);
	sdict_reset(&p4.set);
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		p4_reset_changelist(p4.changelists.data + i);
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
		task_p4 *t = (task_p4 *)_t->userdata;
		if(t->dicts.count == 1) {
			sdict_move(&p4.info, t->dicts.data);
		}
	}
}
static void task_p4set_statechanged(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_process *p = t->userdata;
		const char *cursor = sb_get(&p->stdoutBuf);
		while(*cursor) {
			span_t key = tokenize(&cursor, "\r\n=");
			if(!key.start)
				break;
			if(*cursor++ != '=')
				break;
			span_t value = tokenize(&cursor, "\r\n");
			if(!value.end)
				break;
			while(value.end > value.start && *value.end != '(')
				--value.end;
			--value.end;

			sdictEntry_t entry = { 0 };
			sb_va(&entry.key, "%.*s", (key.end - key.start), key.start);
			sb_va(&entry.value, "%.*s", (value.end - value.start), value.start);
			BB_LOG("p4::set", "%s=%s", sb_get(&entry.key), sb_get(&entry.value));
			sdict_add(&p4.set, &entry);
		}
	}
}
void p4_info(void)
{
	task_queue(p4_task_create(task_p4info_statechanged, p4_dir(), NULL, "\"%s\" -G info", p4_exe()));
	task setTask = process_task_create(kProcessSpawn_Tracked, p4_dir(), "\"%s\" set", p4_exe());
	setTask.stateChanged = task_p4set_statechanged;
	task_queue(setTask);
}

void p4_changes(void)
{
	task_queue(p4_task_create(task_process_statechanged, p4_dir(), NULL, "\"%s\" -G changes", p4_exe()));
}
