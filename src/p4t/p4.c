// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4.h"

#include "app.h"
#include "appdata.h"
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
	{ "change", kChangesetColumn_Numeric },
	{ "time", kChangesetColumn_Time },
	{ "client", kChangesetColumn_Text },
	{ "user", kChangesetColumn_Text },
	{ "desc", kChangesetColumn_TextMultiline },
};
BB_CTASSERT(BB_ARRAYSIZE(s_changesetColumnFields) == BB_ARRAYSIZE(g_config.uiPendingChangesets.columnWidth));

const char *p4_exe(void)
{
	return sb_get(&p4.exe);
}

const char *p4_dir(void)
{
	const char *configClientspec = sb_get(&g_config.p4.clientspec);
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
	const char *configClientspec = sb_get(&g_config.p4.clientspec);
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
		return true;
	} else {
		output_error("Failed to find p4.exe\n");
		return false;
	}
}

void p4_reset_changelist(p4Changelist *cl)
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

void p4_reset_uichangesetentry(p4UIChangesetEntry *e)
{
	p4_free_changelist_files(&e->normalFiles);
	p4_free_changelist_files(&e->shelvedFiles);
	sb_reset(&e->client);
}

static void p4_reset_uichangeset(p4UIChangeset *uics)
{
	sb_reset(&uics->user);
	sb_reset(&uics->clientspec);
	sb_reset(&uics->filter);
	sb_reset(&uics->filterInput);
	for(u32 i = 0; i < uics->count; ++i) {
		p4_reset_uichangesetentry(uics->data + i);
	}
	bba_free(*uics);
}

void p4_reset_uichangelist(p4UIChangelist *uicl)
{
	p4_free_changelist_files(&uicl->normalFiles);
	p4_free_changelist_files(&uicl->shelvedFiles);
}

void p4_shutdown(void)
{
	sb_reset(&p4.exe);
	sdict_reset(&p4.info);
	sdict_reset(&p4.set);
	sdicts_reset(&p4.allUsers);
	sdicts_reset(&p4.allClients);
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
	for(u32 i = 0; i < p4.uiChangelists.count; ++i) {
		p4_reset_uichangelist(p4.uiChangelists.data + i);
	}
	bba_free(p4.uiChangelists);
}

void p4_update(void)
{
	for(u32 i = 0; i < p4.uiChangelists.count;) {
		p4UIChangelist *uicl = p4.uiChangelists.data + i;
		if(uicl->id == 0) {
			p4_reset_uichangelist(uicl);
			bba_erase(p4.uiChangelists, i);
		} else {
			++i;
		}
	}
}

p4Changelist *p4_find_changelist(u32 cl)
{
	if(cl) {
		for(u32 i = 0; i < p4.changelists.count; ++i) {
			p4Changelist *change = p4.changelists.data + i;
			if(change->number == cl) {
				return change;
			}
		}
	}
	return NULL;
}

p4Changelist *p4_find_default_changelist(const char *client)
{
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		p4Changelist *change = p4.changelists.data + i;
		if(change->number == 0) {
			const char *changeClient = sdict_find_safe(&change->normal, "client");
			if(!strcmp(changeClient, client)) {
				return change;
			}
		}
	}
	return NULL;
}

p4ChangelistType p4_get_changelist_type(sdict_t *cl)
{
	p4ChangelistType ret = kChangelistType_Submitted;
	if(!strcmp(sdict_find_safe(cl, "status"), "pending")) {
		if(!strcmp(sdict_find_safe(cl, "client"), p4_clientspec())) {
			ret = kChangelistType_PendingLocal;
		} else {
			ret = kChangelistType_PendingOther;
		}
	}
	return ret;
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
		sdicts_move(&p4.allClients, &t->dicts);
		sdicts_reset(&p4.selfClients);
		sdicts_reset(&p4.localClients);
		const char *clientHost = sdict_find_safe(&p4.info, "clientHost");
		const char *userName = sdict_find(&p4.info, "userName");
		for(u32 i = 0; i < p4.allClients.count; ++i) {
			sdict_t *sd = p4.allClients.data + i;
			const char *user = sdict_find_safe(sd, "Owner");
			if(!_stricmp(user, userName)) {
				if(bba_add(p4.selfClients, 1)) {
					sdict_t *target = &bba_last(p4.selfClients);
					sdict_copy(target, sd);
				}
				const char *host = sdict_find_safe(sd, "Host");
				if(!_stricmp(host, clientHost)) {
					if(bba_add(p4.localClients, 1)) {
						sdict_t *target = &bba_last(p4.localClients);
						sdict_copy(target, sd);
					}
				}
			}
		}
	}
}
static void task_p4users_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t->userdata;
		sdicts_move(&p4.allUsers, &t->dicts);
		task_queue(p4_task_create(task_p4clients_statechanged, p4_dir(), NULL, "\"%s\" -G clients", p4_exe()));
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
		task_queue(p4_task_create(task_p4users_statechanged, p4_dir(), NULL, "\"%s\" -G users", p4_exe()));
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

void p4_build_default_changelist(sdict_t *sd, const char *owner, const char *client)
{
	sdict_add_raw(sd, "code", "stat");
	sdict_add_raw(sd, "change", "default");
	sdict_add_raw(sd, "time", "0");
	sdict_add_raw(sd, "user", owner);
	sdict_add_raw(sd, "client", client);
	sdict_add_raw(sd, "status", "pending");
	sdict_add_raw(sd, "changeType", "public");
	sdict_add_raw(sd, "desc", "");
}

p4Changeset *p4_find_or_add_changeset(b32 pending)
{
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4Changeset *cs = p4.changesets.data + i;
		if(cs->pending == pending)
			return cs;
	}
	return p4_add_changeset(pending);
}

p4Changeset *p4_add_changeset(b32 pending)
{
	if(bba_add(p4.changesets, 1)) {
		p4Changeset *cs = &bba_last(p4.changesets);
		cs->pending = pending;
		cs->highestReceived = 0;
		cs->refreshed = false;
		cs->updating = false;
		return cs;
	}
	return NULL;
}

static void p4_save_submitted_changeset(p4Changeset *cs)
{
	sb_t path = appdata_get();
	sb_va(&path, "\\%s_submitted_changesets.bin", globals.appSpecific.configName);
	BB_LOG("p4::cache", "begin save submitted changelists - path:%s", sb_get(&path));
	BB_FLUSH();

	pyWriter pw = { 0 };
	b32 built = py_write_sdicts(&pw, &cs->changelists);
	b32 wrote = false;
	if(built) {
		fileData_t fd = { 0 };
		fd.buffer = pw.data;
		fd.bufferSize = pw.count;
		if(fd.buffer && path.data) {
			wrote = fileData_writeIfChanged(path.data, NULL, fd);
		}
	}
	if(wrote) {
		BB_LOG("p4::cache", "end save submitted changelists - count:%u highest:%u built:%u wrote:%u", cs->changelists.count, cs->highestReceived, built, wrote);
	} else {
		BB_ERROR("p4::cache", "end save submitted changelists - count:%u highest:%u built:%u wrote:%u", cs->changelists.count, cs->highestReceived, built, wrote);
	}
	BB_FLUSH();
	bba_free(pw);
	sb_reset(&path);
}

static void p4_load_submitted_changeset(p4Changeset *cs)
{
	sb_t path = appdata_get();
	sb_va(&path, "\\%s_submitted_changesets.bin", globals.appSpecific.configName);
	BB_LOG("p4::cache", "begin load submitted changelists - path:%s", sb_get(&path));
	BB_FLUSH();

	fileData_t fd = fileData_read(sb_get(&path));
	if(fd.buffer) {
		sdicts dicts = { 0 };
		pyParser parser = { 0 };
		parser.cmdline = sb_get(&path);
		parser.data = fd.buffer;
		parser.count = fd.bufferSize;
		while(py_parser_tick(&parser, &dicts)) {
			// do nothing
		}
		if(dicts.count) {
			p4_reset_changeset(cs);
			++cs->parity;
			cs->refreshed = true;
			sdicts_move(&cs->changelists, &dicts);
			for(u32 i = 0; i < cs->changelists.count; ++i) {
				sdict_t *sd = cs->changelists.data + i;
				u32 number = strtou32(sdict_find(sd, "change"));
				cs->highestReceived = BB_MAX(cs->highestReceived, number);
			}
		}
		// don't bba_free(parser) because the memory is borrowed from fd
		sdict_reset(&parser.dict);
		sdicts_reset(&dicts);
		BB_LOG("p4::cache", "end load submitted changelists - count:%u highest:%u", cs->changelists.count, cs->highestReceived);
	} else {
		BB_LOG("p4::cache", "end load submitted changelists - no data");
	}
	BB_FLUSH();
	fileData_reset(&fd);
	sb_reset(&path);
}

static void task_p4changes_refresh_statechanged(task *t)
{
	task_process_statechanged(t);
	if(task_done(t)) {
		b32 pending = strtos32(sdict_find_safe(&t->extraData, "pending"));
		p4Changeset *cs = p4_find_or_add_changeset(pending);
		if(cs) {
			cs->updating = false;
			if(t->state == kTaskState_Succeeded) {
				++cs->parity;
				cs->refreshed = true;
				p4_reset_changeset(cs);
				task_p4 *p = (task_p4 *)t->userdata;
				sdicts_move(&cs->changelists, &p->dicts);
				for(u32 i = 0; i < cs->changelists.count; ++i) {
					sdict_t *sd = cs->changelists.data + i;
					u32 number = strtou32(sdict_find(sd, "change"));
					cs->highestReceived = BB_MAX(cs->highestReceived, number);
				}
				if(pending) {
					for(u32 clientIdx = 0; clientIdx < p4.allClients.count; ++clientIdx) {
						sdict_t *clientDict = p4.allClients.data + clientIdx;
						const char *client = sdict_find(clientDict, "client");
						const char *owner = sdict_find(clientDict, "Owner");
						if(client && owner) {
							if(bba_add(cs->changelists, 1)) {
								sdict_t *sd = &bba_last(cs->changelists);
								p4_build_default_changelist(sd, owner, client);
							}
						}
					}
				} else {
					p4_save_submitted_changeset(cs);
				}
			}
		}
	}
}
void p4_refresh_changeset(p4Changeset *cs)
{
	if(!cs->updating && p4.allClients.count > 0) {
		cs->highestReceived = 0;
		cs->refreshed = false;
		if(cs->pending) {
			task *t = task_queue(
			    p4_task_create(task_p4changes_refresh_statechanged, p4_dir(), NULL,
			                   "\"%s\" -G changes -s pending -l", p4_exe()));
			if(t) {
				cs->updating = true;
				sdict_add_raw(&t->extraData, "pending", "1");
			}
		} else {
			if(!cs->refreshed) {
				p4_load_submitted_changeset(cs);
				if(cs->refreshed) {
					p4_request_newer_changes(cs, g_config.p4.changelistBlockSize);
					return;
				}
			}
			task *t = task_queue(
			    p4_task_create(task_p4changes_refresh_statechanged, p4_dir(), NULL,
			                   "\"%s\" -G changes -s submitted -l", p4_exe()));
			if(t) {
				cs->updating = true;
				sdict_add_raw(&t->extraData, "pending", "0");
			}
		}
	}
}

static void task_p4changes_newer_statechanged(task *t)
{
	task_process_statechanged(t);
	if(task_done(t)) {
		u32 blockSize = strtou32(sdict_find_safe(&t->extraData, "blockSize"));
		p4Changeset *cs = p4_find_or_add_changeset(false);
		if(cs) {
			cs->updating = false;
			if(t->state == kTaskState_Succeeded) {
				b32 complete = false;
				task_p4 *p = (task_p4 *)t->userdata;
				for(u32 i = 0; i < p->dicts.count; ++i) {
					sdict_t *sd = p->dicts.data + i;
					u32 number = strtou32(sdict_find(sd, "change"));
					if(number == cs->highestReceived) {
						complete = true;
						break;
					}
				}
				if(complete) {
					b32 added = false;
					u32 highestReceived = cs->highestReceived;
					for(u32 i = 0; i < p->dicts.count; ++i) {
						sdict_t *sd = p->dicts.data + i;
						u32 number = strtou32(sdict_find(sd, "change"));
						if(number > cs->highestReceived) {
							if(bba_add(cs->changelists, 1)) {
								sdict_t *target = &bba_last(cs->changelists);
								sdict_move(target, sd);
								added = true;
								highestReceived = BB_MAX(highestReceived, number);
							}
						}
					}
					if(added) {
						++cs->parity;
						cs->highestReceived = highestReceived;
						p4_save_submitted_changeset(cs);
					}
				} else {
					p4_request_newer_changes(cs, blockSize * 2);
				}
			}
		}
	}
}
void p4_request_newer_changes(p4Changeset *cs, u32 blockSize)
{
	if(!cs->updating) {
		if(cs->pending) {
			p4_refresh_changeset(cs);
		} else {
			BB_LOG("p4", "requesting newer submitted changelists - blockSize is %u", blockSize);
			if(blockSize) {
				task *t = task_queue(
				    p4_task_create(task_p4changes_newer_statechanged, p4_dir(), NULL,
				                   "\"%s\" -G changes -s submitted -l -m %u", p4_exe(), blockSize));
				if(t) {
					cs->updating = true;
					sdict_add_raw(&t->extraData, "blockSize", va("%u", blockSize));
				}
			} else {
				p4_refresh_changeset(cs);
			}
		}
	}
}

static p4Changeset *s_sortChangeset;
static p4UIChangeset *s_sortUIChangeset;
static uiChangesetConfig *s_sortConfig;
sdict_t *p4_find_changelist_in_changeset(p4Changeset *cs, u32 number, const char *client)
{
	for(u32 i = 0; i < cs->changelists.count; ++i) {
		sdict_t *sd = cs->changelists.data + i;
		if(strtou32(sdict_find_safe(sd, "change")) == number) {
			if(number || !strcmp(sdict_find_safe(sd, "client"), client)) {
				return sd;
			}
		}
	}
	return NULL;
}
static int p4_changeset_compare(const void *_a, const void *_b)
{
	const p4UIChangesetEntry *aEntry = _a;
	const p4UIChangesetEntry *bEntry = _b;

	sdict_t *a = p4_find_changelist_in_changeset(s_sortChangeset, aEntry->changelist, sb_get(&aEntry->client));
	sdict_t *b = p4_find_changelist_in_changeset(s_sortChangeset, bEntry->changelist, sb_get(&bEntry->client));

	int mult = s_sortConfig->sortDescending ? -1 : 1;
	u32 columnIndex = s_sortConfig->sortColumn;
	const changesetColumnField *field = s_changesetColumnFields + columnIndex;
	const char *astr = sdict_find_safe(a, field->key);
	const char *bstr = sdict_find_safe(b, field->key);
	int val;
	if(field->type == kChangesetColumn_Numeric) {
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
		cs->id = ++p4.lastId;
		cs->pending = pending;
		cs->lastClickIndex = ~0U;
		return cs;
	}
	return NULL;
}

p4UIChangeset *p4_find_uichangeset(u32 id)
{
	for(u32 i = 0; i < p4.uiChangesets.count; ++i) {
		p4UIChangeset *uics = p4.uiChangesets.data + i;
		if(uics->id == id) {
			return uics;
		}
	}
	return NULL;
}

p4UIChangelist *p4_add_uichangelist(void)
{
	if(bba_add(p4.uiChangelists, 1)) {
		p4UIChangelist *uicl = &bba_last(p4.uiChangelists);
		uicl->id = ++p4.lastId;
		return uicl;
	}
	return NULL;
}

p4UIChangelist *p4_find_uichangelist(u32 id)
{
	for(u32 i = 0; i < p4.uiChangelists.count; ++i) {
		p4UIChangelist *uicl = p4.uiChangelists.data + i;
		if(uicl->id == id) {
			return uicl;
		}
	}
	return NULL;
}

void p4_mark_uichangelist_for_removal(p4UIChangelist *uicl)
{
	uicl->id = 0;
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
			bool unresolved = false;
			for(u32 i = 0; i < sds->count; ++i) {
				sdict_t *sd = sds->data + i;
				const char *detailedDepotFile = sdict_find_safe(sd, "depotFile");
				if(!strcmp(detailedDepotFile, depotFile)) {
					localPath = sdict_find_safe(sd, "path");
					headRev = sdict_find(sd, "headRev");
					unresolved = sdict_find(sd, "unresolved") != NULL;
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
				file->unresolved = unresolved;
			}
		} else {
			break;
		}
	}
	qsort(files->data, files->count, sizeof(uiChangelistFile), &p4_changelist_files_compare);
	files->lastClickIndex = ~0u;
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
