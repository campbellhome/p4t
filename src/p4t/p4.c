// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4.h"

#include "app.h"
#include "bb.h"
#include "bb_array.h"
#include "config.h"
#include "env_utils.h"
#include "file_utils.h"
#include "output.h"
#include "p4_task.h"
#include "span.h"
#include "str.h"
#include "tokenize.h"
#include "va.h"
#include <stdlib.h>

p4_t p4;

const char *p4_exe(void)
{
	return sb_get(&p4.exe);
}

const char *p4_dir(void)
{
	const char *configClientspec = sb_get(&g_config.clientspec);
	for(u32 i = 0; i < p4.localClients.count; ++i) {
		if(!strcmp(configClientspec, sdict_find_safe(p4.localClients.data + i, "client"))) {
			return sdict_find_safe(p4.localClients.data + i, "Root");
		}
	}
	const char *root = sdict_find(&p4.info, "clientRoot");
	if(root) {
		return root;
	} else {
		return "C:\\";
	}
}

const char *p4_clientspec(void)
{
	const char *configClientspec = sb_get(&g_config.clientspec);
	for(u32 i = 0; i < p4.localClients.count; ++i) {
		if(!strcmp(configClientspec, sdict_find_safe(p4.localClients.data + i, "client"))) {
			return configClientspec;
		}
	}
	return sdict_find(&p4.info, "clientName");
}

const char *p4_clientspec_arg(void)
{
	const char *clientspec = p4_clientspec();
	if(clientspec) {
		return va(" -c %s", clientspec);
	} else {
		return "";
	}
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

static void p4_reset_changelistshorts(p4ChangelistShorts *shorts)
{
	for(u32 i = 0; i < shorts->count; ++i) {
		sdict_reset(&shorts->data[i].dict);
	}
	bba_free(*shorts);
}

void p4_shutdown(void)
{
	sb_reset(&p4.exe);
	sdict_reset(&p4.info);
	sdict_reset(&p4.set);
	sdicts_reset(&p4.selfClients);
	sdicts_reset(&p4.localClients);
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		p4_reset_changelist(p4.changelists.data + i);
	}
	bba_free(p4.changelists);
	p4_reset_changelistshorts(&p4.pendingChangelistShorts);
	p4_reset_changelistshorts(&p4.submittedChangelistShorts);
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

static void task_p4clients_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t->userdata;
		sdicts_move(&p4.selfClients, &t->dicts);
		sdicts_reset(&p4.localClients);
		const char *clientHost = sdict_find_safe(&p4.info, "clientHost");
		for(u32 i = 0; i < p4.selfClients.count; ++i) {
			sdict_t *sd = p4.selfClients.data + i;
			const char *host = sdict_find_safe(sd, "Host");
			if(!strcmp(host, clientHost)) {
				if(bba_add(p4.localClients, 1)) {
					sdict_t *target = &bba_last(p4.localClients);
					sdict_copy(target, sd);
				}
			}
		}
	}
}
static void task_p4info_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t->userdata;
		if(t->dicts.count == 1) {
			sdict_move(&p4.info, t->dicts.data);
		}

		const char *userName = sdict_find(&p4.info, "userName");
		if(userName) {
			task_queue(p4_task_create(task_p4clients_statechanged, p4_dir(), NULL, "\"%s\" -G clients -u %s", p4_exe(), userName));
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

static void task_p4changes_statechanged(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		b32 pending = !strcmp(sdict_find_safe(&t->extraData, "type"), "pending");
		p4ChangelistShorts *s = pending ? &p4.pendingChangelistShorts : &p4.submittedChangelistShorts;
		p4_reset_changelistshorts(s);
		task_p4 *p = (task_p4 *)t->userdata;
		for(u32 i = 0; i < p->dicts.count; ++i) {
			if(bba_add(*s, 1)) {
				p4ChangelistShort *c = &bba_last(*s);
				sdict_move(&c->dict, p->dicts.data + i);
				c->number = strtou32(sdict_find_safe(&c->dict, "change"));
				const char *desc = sdict_find(&c->dict, "desc");
				if(desc) {
					char ch;
					sb_t sb = { 0 };
					while((ch = *desc++) != '\0') {
						if(ch == '\r') {
							// do nothing
						} else if(ch == '\n') {
							sb_append_char(&sb, ' ');
						} else {
							sb_append_char(&sb, ch);
						}
					}
					sdict_add_raw(&c->dict, "desc_oneline", sb_get(&sb));
					sb_reset(&sb);
				}
			}
		}
	}
}
void p4_changes(b32 pending)
{
	task *t = task_queue(p4_task_create(task_p4changes_statechanged, p4_dir(), NULL, "\"%s\" -G changes -s %s -L", p4_exe(), pending ? "pending" : "submitted"));
	if(t) {
		sdict_add_raw(&t->extraData, "type", pending ? "pending" : "submitted");
	}
}
