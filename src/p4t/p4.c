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

p4Changeset *p4_add_changeset(b32 pending);

const changesetColumnField s_changesetColumnFields[] = {
	{ "change", false, true },
	{ "time", true, false },
	{ "user", false, false },
	{ "desc_oneline", false, false },
};
BB_CTASSERT(BB_ARRAYSIZE(s_changesetColumnFields) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

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
	p4.changesetColumnFields = s_changesetColumnFields;
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
		p4_add_changeset(true);
		p4_add_changeset(false);
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

static void p4_reset_changeset(p4Changeset *cs)
{
	sdicts_reset(&cs->changelists);
}

static void p4_reset_uichangesetentry(p4UIChangesetEntry *e)
{
	p4_free_changelist_files(&e->normalFiles);
	p4_free_changelist_files(&e->shelvedFiles);
}

static void p4_reset_uichangeset(p4UIChangeset *cs)
{
	sb_reset(&cs->user);
	sb_reset(&cs->clientspec);
	sb_reset(&cs->filter);
	for(u32 i = 0; i < cs->count; ++i) {
		p4_reset_uichangesetentry(cs->data + i);
	}
	bba_free(*cs);
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
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4_reset_changeset(p4.changesets.data + i);
	}
	bba_free(p4.changesets);
	for(u32 i = 0; i < p4.uiChangesets.count; ++i) {
		p4_reset_uichangeset(p4.uiChangesets.data + i);
	}
	bba_free(p4.uiChangesets);
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

p4Changeset *p4_find_changeset(b32 pending)
{
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4Changeset *cs = p4.changesets.data + i;
		if(cs->pending == pending)
			return cs;
	}
	return NULL;
}

static void task_p4changes_statechanged(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		b32 pending = strtos32(sdict_find_safe(&t->extraData, "pending"));
		p4Changeset *cs = p4_find_changeset(pending);
		if(cs) {
			++cs->parity;
			p4_reset_changeset(cs);
			task_p4 *p = (task_p4 *)t->userdata;
			sdicts_move(&cs->changelists, &p->dicts);
			for(u32 i = 0; i < cs->changelists.count; ++i) {
				sdict_t *sd = cs->changelists.data + i;
				const char *desc = sdict_find(sd, "desc");
				if(desc) {
					char ch;
					sb_t sb = { 0 };
					while((ch = *desc++) != '\0') {
						if(ch == '\r') {
							// do nothing
						} else if(ch == '\n' || ch == '\t') {
							sb_append_char(&sb, ' ');
						} else {
							sb_append_char(&sb, ch);
						}
					}
					sdict_add_raw(sd, "desc_oneline", sb_get(&sb));
					sb_reset(&sb);
				}
			}
		}
	}
}
p4Changeset *p4_add_changeset(b32 pending)
{
	if(bba_add(p4.changesets, 1)) {
		p4Changeset *cs = &bba_last(p4.changesets);
		cs->pending = pending;
		task *t = task_queue(p4_task_create(task_p4changes_statechanged, p4_dir(), NULL, "\"%s\" -G changes -s %s -L", p4_exe(), pending ? "pending" : "submitted"));
		if(t) {
			sdict_add_raw(&t->extraData, "pending", va("%d", cs->pending));
		}
		return cs;
	}
	return NULL;
}
void p4_refresh_changeset(p4Changeset *cs)
{
	task *t = task_queue(p4_task_create(task_p4changes_statechanged, p4_dir(), NULL, "\"%s\" -G changes -s %s -L", p4_exe(), cs->pending ? "pending" : "submitted"));
	if(t) {
		sdict_add_raw(&t->extraData, "pending", va("%d", cs->pending));
	}
}

static p4Changeset *s_sortChangeset;
static p4UIChangeset *s_sortUIChangeset;
static uiChangesetConfig *s_sortConfig;
sdict_t *p4_find_changelist_in_changeset(p4Changeset *cs, u32 number)
{
	for(u32 i = 0; i < cs->changelists.count; ++i) {
		sdict_t *sd = cs->changelists.data + i;
		if(strtou32(sdict_find_safe(sd, "change")) == number) {
			return sd;
		}
	}
	return NULL;
}
static int p4_changeset_compare(const void *_a, const void *_b)
{
	const p4UIChangesetEntry *aEntry = _a;
	const p4UIChangesetEntry *bEntry = _b;

	sdict_t *a = p4_find_changelist_in_changeset(s_sortChangeset, aEntry->changelist);
	sdict_t *b = p4_find_changelist_in_changeset(s_sortChangeset, bEntry->changelist);

	int mult = s_sortConfig->sortDescending ? -1 : 1;
	u32 columnIndex = s_sortConfig->sortColumn;
	const changesetColumnField *field = s_changesetColumnFields + columnIndex;
	const char *astr = sdict_find_safe(a, field->key);
	const char *bstr = sdict_find_safe(b, field->key);
	int val;
	if(field->numeric) {
		int aint = atoi(astr);
		int bint = atoi(bstr);
		if(aint < bint) {
			val = -1;
		} else if(aint > bint) {
			val = 1;
		} else {
			val = 0;
		}
	} else {
		val = strcmp(astr, bstr);
	}
	if(val) {
		return val * mult;
	} else {
		return s_sortConfig->sortDescending ? (a > b) : (a < b);
	}
}
void p4_sort_uichangeset(p4UIChangeset *uics)
{
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4Changeset *cs = p4.changesets.data + i;
		if(cs->pending == uics->pending) {
			s_sortChangeset = cs;
			break;
		}
	}
	if(!s_sortChangeset) {
		return;
	}
	s_sortUIChangeset = uics;
	s_sortConfig = s_sortChangeset->pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;
	qsort(uics->data, uics->count, sizeof(p4UIChangesetEntry), &p4_changeset_compare);
	s_sortConfig = NULL;
	s_sortChangeset = NULL;
	s_sortUIChangeset = NULL;
}

p4UIChangeset *p4_add_uichangeset(b32 pending)
{
	if(bba_add(p4.uiChangesets, 1)) {
		p4UIChangeset *cs = &bba_last(p4.uiChangesets);
		cs->id = ++p4.uiChangesets.lastId;
		cs->pending = pending;
		cs->lastClickIndex = ~0U;
		return cs;
	}
	return NULL;
}

int p4_changelist_files_compare(const void *_a, const void *_b)
{
	uiChangelistFile *a = (uiChangelistFile *)_a;
	uiChangelistFile *b = (uiChangelistFile *)_b;

	int mult = g_config.uiChangelist.sortDescending ? -1 : 1;
	u32 columnIndex = g_config.uiChangelist.sortColumn;
	int val = strcmp(a->fields.str[columnIndex], b->fields.str[columnIndex]);
	if(val) {
		return val * mult;
	} else {
		return g_config.uiChangelist.sortDescending ? (a > b) : (a < b);
	}
}
void p4_free_changelist_files(uiChangelistFiles *files)
{
	for(u32 i = 0; i < files->count; ++i) {
		for(u32 col = 0; col < BB_ARRAYSIZE(files->data[i].fields.str); ++col) {
			free(files->data[i].fields.str[col]);
		}
	}
	bba_free(*files);
}
static void p4_build_changelist_files_internal(sdict_t *change, sdicts *sds, uiChangelistFiles *files)
{
	p4_free_changelist_files(files);
	u32 sdictIndex = 0;
	while(1) {
		u32 fileIndex = files->count;
		u32 depotFileIndex = sdict_find_index_from(change, va("depotFile%u", fileIndex), sdictIndex);
		u32 actionIndex = sdict_find_index_from(change, va("action%u", fileIndex), depotFileIndex);
		u32 typeIndex = sdict_find_index_from(change, va("type%u", fileIndex), actionIndex);
		u32 revIndex = sdict_find_index_from(change, va("rev%u", fileIndex), typeIndex);
		sdictIndex = revIndex;
		if(revIndex < change->count) {
			const char *depotFile = sb_get(&change->data[depotFileIndex].value);
			const char *action = sb_get(&change->data[actionIndex].value);
			const char *type = sb_get(&change->data[typeIndex].value);
			const char *rev = sb_get(&change->data[revIndex].value);
			const char *lastSlash = strrchr(depotFile, '/');
			const char *filename = (lastSlash) ? lastSlash + 1 : NULL;
			const char *localPath = "";
			const char *headRev = NULL;
			for(u32 i = 0; i < sds->count; ++i) {
				sdict_t *sd = sds->data + i;
				const char *detailedDepotFile = sdict_find_safe(sd, "depotFile");
				if(!strcmp(detailedDepotFile, depotFile)) {
					localPath = sdict_find_safe(sd, "path");
					headRev = sdict_find(sd, "headRev");
				}
			}
			if(filename && bba_add(*files, 1)) {
				uiChangelistFile *file = &bba_last(*files);
				file->fields.field.filename = _strdup(filename);
				if(headRev && strcmp(headRev, rev)) {
					file->fields.field.rev = _strdup(va("%s/%s", rev, headRev));
				} else {
					file->fields.field.rev = _strdup(rev);
				}
				file->fields.field.action = _strdup(action);
				file->fields.field.filetype = _strdup(type);
				file->fields.field.depotPath = _strdup(depotFile);
				file->fields.field.localPath = _strdup(localPath);
			}
		} else {
			break;
		}
	}
	qsort(files->data, files->count, sizeof(uiChangelistFile), &p4_changelist_files_compare);
	files->lastClickIndex = ~0u;
	files->active = false;
}
void p4_build_changelist_files(p4Changelist *cl, uiChangelistFiles *normalFiles, uiChangelistFiles *shelvedFiles)
{
	p4_build_changelist_files_internal(&cl->normal, &cl->normalFiles, normalFiles);
	if(shelvedFiles && sdict_find(&cl->normal, "shelved")) {
		shelvedFiles->shelved = true;
		p4_build_changelist_files_internal(&cl->shelved, &cl->shelvedFiles, shelvedFiles);
	} else {
		p4_free_changelist_files(shelvedFiles);
	}
}
