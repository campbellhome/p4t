// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "task_describe_changelist.h"

#include "bb_array.h"
#include "p4.h"
#include "p4_task.h"
#include "str.h"
#include "va.h"

static u32 s_taskDescribeChangelistCount;

b32 p4_describe_task_count(void)
{
	return s_taskDescribeChangelistCount;
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
static void spawn_describe_shelved(p4Changelist *cl, task_p4 *p)
{
	task_queue(p4_task_create(
	    task_describe_changelist_statechanged_desc_shelved, p4_dir(), &p->extraData,
	    "\"%s\" -G describe -s -S %u", p4_exe(), cl->number));
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
					spawn_describe_shelved(cl, p);
				}
			}
		}
	}
}
static void spawn_fstat_normal(p4Changelist *cl, task_p4 *p)
{
	const char *clientName = sdict_find_safe(&cl->normal, "client");
	task_queue(p4_task_create(
	    task_describe_changelist_statechanged_fstat_normal, p4_dir(), &p->extraData,
	    "\"%s\" -G fstat -Olhp -Rco -e %u //%s/...", p4_exe(), cl->number, clientName));
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
					if(sdict_find(&cl->normal, "depotFile0")) {
						spawn_fstat_normal(cl, p);
					} else if(sdict_find(&cl->normal, "shelved")) {
						spawn_describe_shelved(cl, p);
					}
				}
			}
		}
	}
	if(task_done(t)) {
		--s_taskDescribeChangelistCount;
	}
}
void p4_describe_changelist(u32 cl)
{
	task *t = task_queue(p4_task_create(
	    task_describe_changelist_statechanged_desc, p4_dir(), NULL,
	    "\"%s\" -G describe -s %u", p4_exe(), cl));
	if(t) {
		++s_taskDescribeChangelistCount;
	}
}
