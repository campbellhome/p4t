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
void p4_info(void)
{
	task_queue(p4_task_create(task_p4info_statechanged, p4_dir(), NULL, "\"%s\" -G info", p4_exe()));
}

void p4_changes(void)
{
	task_queue(p4_task_create(task_process_statechanged, p4_dir(), NULL, "\"%s\" -G changes", p4_exe()));
}

static void task_describe_changelist_statechanged_fstat_shelved(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_p4 *p = t->userdata;
		u32 changeNumber = strtou32(sdict_find_safe(&p->extraData, "change"));
		if(changeNumber) {
			p4Changelist *cl = p4_find_changelist(changeNumber);
			if(cl) {
				sdicts_move(&cl->shelvedFiles, &p->dicts);
				++cl->parity;
			}
		}
	}
}
static void task_describe_changelist_statechanged_desc_shelved(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_p4 *p = t->userdata;
		if(p->dicts.count == 1) {
			u32 changeNumber = strtou32(sdict_find_safe(p->dicts.data, "change"));
			if(changeNumber) {
				sdictEntry_t e = { 0 };
				sb_append(&e.key, "change");
				sb_va(&e.value, va("%u", changeNumber));
				sdict_add(&p->extraData, &e);
				p4Changelist *cl = p4_find_changelist(changeNumber);
				if(cl) {
					sdict_move(&cl->shelved, p->dicts.data);
					++cl->parity;
				} else if(bba_add(p4.changelists, 1)) {
					cl = &bba_last(p4.changelists);
					cl->number = changeNumber;
					cl->parity = 1;
					sdict_move(&cl->shelved, p->dicts.data);
				}
				if(cl) {
					const char *clientName = sdict_find_safe(&cl->normal, "client");
					task_queue(p4_task_create(
					    task_describe_changelist_statechanged_fstat_shelved, p4_dir(), &p->extraData,
					    "\"%s\" -G fstat -Op -Rs -e %u //%s/...", p4_exe(), changeNumber, clientName));
				}
			}
		}
	}
}
static void task_describe_changelist_statechanged_fstat_normal(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_p4 *p = t->userdata;
		u32 changeNumber = strtou32(sdict_find_safe(&p->extraData, "change"));
		if(changeNumber) {
			p4Changelist *cl = p4_find_changelist(changeNumber);
			if(cl) {
				sdicts_move(&cl->normalFiles, &p->dicts);
				++cl->parity;
				if(sdict_find(&cl->normal, "shelved")) {
					task_queue(p4_task_create(
					    task_describe_changelist_statechanged_desc_shelved, p4_dir(), &p->extraData,
					    "\"%s\" -G describe -s -S %u", p4_exe(), changeNumber));
				}
			}
		}
	}
}
static void task_describe_changelist_statechanged_desc(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_p4 *p = t->userdata;
		if(p->dicts.count == 1) {
			u32 changeNumber = strtou32(sdict_find_safe(p->dicts.data, "change"));
			if(changeNumber) {
				sdictEntry_t e = { 0 };
				sb_append(&e.key, "change");
				sb_va(&e.value, va("%u", changeNumber));
				sdict_add(&p->extraData, &e);
				p4Changelist *cl = p4_find_changelist(changeNumber);
				if(cl) {
					sdict_move(&cl->normal, p->dicts.data);
					++cl->parity;
				} else if(bba_add(p4.changelists, 1)) {
					cl = &bba_last(p4.changelists);
					cl->number = changeNumber;
					cl->parity = 1;
					sdict_move(&cl->normal, p->dicts.data);
				}
				if(cl) {
					const char *clientName = sdict_find_safe(&cl->normal, "client");
					task_queue(p4_task_create(
					    task_describe_changelist_statechanged_fstat_normal, p4_dir(), &p->extraData,
					    "\"%s\" -G fstat -Olhp -Rco -e %u //%s/...", p4_exe(), changeNumber, clientName));
				}
			}
		}
	}
}
void p4_describe_changelist(u32 cl)
{
	task_queue(p4_task_create(
	    task_describe_changelist_statechanged_desc, p4_dir(), NULL,
	    "\"%s\" -G describe -s %u", p4_exe(), cl));
}
