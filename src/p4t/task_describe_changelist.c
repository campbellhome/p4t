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

static void task_describe_default_changelist_statechanged(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_p4 *p = t->userdata;
		const char *client = sdict_find_safe(&t->extraData, "client");
		p4Changelist *cl = p4_find_default_changelist(client);
		if(cl) {
			++cl->parity;
		} else if(bba_add(p4.changelists, 1)) {
			cl = &bba_last(p4.changelists);
			cl->number = 0;
			cl->parity = 1;
		}
		if(cl) {
			p4_reset_changelist(cl);
			p4_build_default_changelist(&cl->normal, sdict_find_safe(&t->extraData, "user"), client);
			sdicts_move(&cl->normalFiles, &p->dicts);
			for(u32 fileIdx = 0; fileIdx < cl->normalFiles.count; ++fileIdx) {
				sdict_t *f = cl->normalFiles.data + fileIdx;
				const char *depotFile = sdict_find_safe(f, "depotFile");
				const char *action = sdict_find_safe(f, "action");
				const char *type = sdict_find_safe(f, "type");
				const char *rev = sdict_find(f, "rev");
				if(!rev) {
					rev = sdict_find_safe(f, "haveRev");
				}
				sdict_add_raw(&cl->normal, va("depotFile%u", fileIdx), depotFile);
				sdict_add_raw(&cl->normal, va("action%u", fileIdx), action);
				sdict_add_raw(&cl->normal, va("type%u", fileIdx), type);
				sdict_add_raw(&cl->normal, va("rev%u", fileIdx), rev);
			}
		}
	}
	if(task_done(t)) {
		--s_taskDescribeChangelistCount;
	}
}
void p4_describe_default_changelist(const char *client)
{
	const char *localHost = sdict_find_safe(&p4.info, "clientHost");
	const char *localUser = sdict_find(&p4.info, "userName");
	for(u32 clientIdx = 0; clientIdx < p4.allClients.count; ++clientIdx) {
		sdict_t *sd = p4.allClients.data + clientIdx;
		if(!strcmp(sdict_find_safe(sd, "client"), client)) {
			const char *user = sdict_find_safe(sd, "Owner");
			const char *host = sdict_find_safe(sd, "Host");
			task *t;
			if(!_stricmp(user, localUser) && !_stricmp(host, localHost)) {
				// default changelist for a local clientspec
				t = task_queue(p4_task_create(
				    task_describe_default_changelist_statechanged, p4_dir(), NULL,
				    "\"%s\" -G -c %s fstat -Olhp -Rco -e default //%s/...", p4_exe(), client, client));
			} else {
				t = task_queue(p4_task_create(
				    task_describe_default_changelist_statechanged, p4_dir(), NULL,
				    "\"%s\" -G opened -C %s -c default", p4_exe(), client));
			}
			if(t) {
				sdict_add_raw(&t->extraData, "client", client);
				sdict_add_raw(&t->extraData, "user", user);
				++s_taskDescribeChangelistCount;
			}
			break;
		}
	}
}
