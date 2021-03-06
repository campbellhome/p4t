// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4.h"
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
#include "thread_task.h"
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
		return ".";
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

void p4_set_clientspec(const char *client)
{
	sb_reset(&g_config.p4.clientspec);
	if(client && *client) {
		sb_append(&g_config.p4.clientspec, client);
	}
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4Changeset *cs = p4.changesets.data + i;
		++cs->parity;
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
	sb_reset(&uics->config.user);
	sb_reset(&uics->config.clientspec);
	sb_reset(&uics->config.filter);
	sb_reset(&uics->config.filterInput);
	reset_filter_tokens(&uics->autoFilterTokens);
	reset_filter_tokens(&uics->manualFilterTokens);
	for(u32 i = 0; i < uics->entries.count; ++i) {
		p4_reset_uichangesetentry(uics->entries.data + i);
	}
	bba_free(uics->entries);
	bba_free(uics->sorted);
}

void p4_reset_uichangelist(p4UIChangelist *uicl)
{
	p4_free_changelist_files(&uicl->normalFiles);
	p4_free_changelist_files(&uicl->shelvedFiles);
}

void p4_reset_file_locator(p4FileLocator *locator)
{
	sb_reset(&locator->path);
	sb_reset(&locator->revision);
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
	p4_reset_file_locator(&p4.diffLeftSide);
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
	for(u32 i = 0; i < p4.uiChangesets.count;) {
		p4UIChangeset *uicl = p4.uiChangesets.data + i;
		if(uicl->id == 0) {
			p4_reset_uichangeset(uicl);
			bba_erase(p4.uiChangesets, i);
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
		task_p4 *t = (task_p4 *)_t->taskData;
		sdicts_move(&p4.allClients, &t->parsedDicts);
		sdicts_reset(&p4.selfClients);
		sdicts_reset(&p4.localClients);
		const char *clientHost = sdict_find_safe(&p4.info, "clientHost");
		const char *userName = sdict_find(&p4.info, "userName");
		if(userName) {
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
}
static void task_p4users_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t->taskData;
		sdicts_move(&p4.allUsers, &t->parsedDicts);
		task_queue(p4_task_create("refresh_clientspecs", task_p4clients_statechanged, p4_dir(), NULL, "\"%s\" -G clients", p4_exe()));
	}
}
static void task_p4info_statechanged(task *_t)
{
	task_process_statechanged(_t);
	if(_t->state == kTaskState_Succeeded) {
		task_p4 *t = (task_p4 *)_t->taskData;
		if(t->parsedDicts.count == 1) {
			sdict_move(&p4.info, t->parsedDicts.data);
		}
		task_queue(p4_task_create("refresh_users", task_p4users_statechanged, p4_dir(), NULL, "\"%s\" -G users", p4_exe()));
	}
}
static void task_p4set_statechanged(task *t)
{
	task_process_statechanged(t);
	if(t->state == kTaskState_Succeeded) {
		task_process *p = t->taskData;
		if(p->process) {
			const char *cursor = p->process->stdoutBuffer.data ? p->process->stdoutBuffer.data : "";
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
}
void p4_info(void)
{
	task_queue(p4_task_create("refresh_info", task_p4info_statechanged, p4_dir(), NULL, "\"%s\" -G info", p4_exe()));
	task setTask = process_task_create("refresh_environment", kProcessSpawn_Tracked, p4_dir(), "\"%s\" set", p4_exe());
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
	sb_t path = appdata_get("p4t");
	sb_append(&path, "\\p4_submitted_changesets.bin");
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
typedef struct tag_cachedChangesetLoad {
	sb_t path;
	sdicts dicts;
} cachedChangesetLoad;
bb_thread_return_t p4_load_cached_changeset_thread(void *args)
{
	task_thread *th = args;
	cachedChangesetLoad *data = th->data;
	pyParser parser = { 0 };
	fileData_t fd = fileData_read(sb_get(&data->path));
	if(fd.buffer && !th->shouldTerminate) {
		parser.cmdline = sb_get(&data->path);
		parser.data = fd.buffer;
		parser.count = fd.bufferSize;
		while(py_parser_tick(&parser, &data->dicts, false) && !th->shouldTerminate) {
			// do nothing
		}
		// don't bba_free(parser) because the memory is borrowed from fd
		fileData_reset(&fd);
	}
	sdict_reset(&parser.dict);
	th->threadDesiredState = th->shouldTerminate ? kTaskState_Canceled : kTaskState_Succeeded;
	return 0;
}
void p4_load_cached_changeset_statechanged(task *t)
{
	task_thread_statechanged(t);
	if(task_done(t)) {
		p4Changeset *cs = p4_find_or_add_changeset(false);
		cs->updating = false;
		task_thread *th = t->taskData;
		cachedChangesetLoad *data = th->data;
		if(cs && data->dicts.count && t->state == kTaskState_Succeeded) {
			p4_reset_changeset(cs);
			++cs->parity;
			cs->refreshed = true;
			sdicts_move(&cs->changelists, &data->dicts);
			for(u32 i = 0; i < cs->changelists.count; ++i) {
				sdict_t *sd = cs->changelists.data + i;
				u32 number = strtou32(sdict_find(sd, "change"));
				cs->highestReceived = BB_MAX(cs->highestReceived, number);
			}
			BB_LOG("p4::cache", "end load submitted changelists - count:%u highest:%u", cs->changelists.count, cs->highestReceived);
		} else {
			BB_LOG("p4::cache", "end load submitted changelists - no data");
		}
		BB_FLUSH();
		sdicts_reset(&data->dicts);
		sb_reset(&data->path);
		free(data);
		th->data = NULL;

		if(cs) {
			if(cs->refreshed) {
				p4_request_newer_changes(cs, g_config.p4.changelistBlockSize);
			} else {
				p4_refresh_changelist_no_cache(cs);
			}
		}
	}
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
				task_p4 *p = (task_p4 *)t->taskData;
				sdicts_move(&cs->changelists, &p->parsedDicts);
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
void p4_refresh_changelist_no_cache(p4Changeset *cs)
{
	if(!cs->updating && p4.allClients.count > 0) {
		task *t = task_queue(
		    p4_task_create(
		        "refresh_changelists",
		        task_p4changes_refresh_statechanged, p4_dir(), NULL,
		        "\"%s\" -G changes -s %s -l", p4_exe(), cs->pending ? "pending" : "submitted"));
		if(t) {
			cs->updating = true;
			sdict_add_raw(&t->extraData, "pending", cs->pending ? "1" : "0");
		}
	}
}
void p4_refresh_changeset(p4Changeset *cs)
{
	if(!cs->updating && p4.allClients.count > 0) {
		cs->highestReceived = 0;
		cs->refreshed = false;
		if(cs->pending) {
			p4_refresh_changelist_no_cache(cs);
		} else {
			if(!cs->refreshed) {
				cachedChangesetLoad *data = malloc(sizeof(cachedChangesetLoad));
				if(data) {
					memset(data, 0, sizeof(cachedChangesetLoad));
					data->path = appdata_get("p4t");
					sb_append(&data->path, "\\p4_submitted_changesets.bin");
					BB_LOG("p4::cache", "begin load submitted changelists - path:%s", sb_get(&data->path));
					BB_FLUSH();
					task *t = task_queue(
					    thread_task_create("load_cached_changelists", p4_load_cached_changeset_statechanged, p4_load_cached_changeset_thread, data));
					if(t) {
						cs->updating = true;
					}
				}
			}
			if(!cs->updating) {
				p4_refresh_changelist_no_cache(cs);
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
				task_p4 *p = (task_p4 *)t->taskData;
				for(u32 i = 0; i < p->parsedDicts.count; ++i) {
					sdict_t *sd = p->parsedDicts.data + i;
					u32 number = strtou32(sdict_find(sd, "change"));
					if(number == cs->highestReceived) {
						complete = true;
						break;
					}
				}
				if(complete) {
					b32 added = false;
					u32 highestReceived = cs->highestReceived;
					for(u32 i = 0; i < p->parsedDicts.count; ++i) {
						sdict_t *sd = p->parsedDicts.data + i;
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
				    p4_task_create(
				        "find_newer_changelists",
				        task_p4changes_newer_statechanged, p4_dir(), NULL,
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

static uiChangesetConfig *s_sortConfig;
static p4Changeset *s_sortChangeset;
static inline const char *p4_get_uichangesetentry_sort_key(const p4UIChangesetEntry *e)
{
	u32 columnIndex = s_sortConfig->sortColumn;
	const changesetColumnField *field = s_changesetColumnFields + columnIndex;
	sdict_t *sd = s_sortChangeset->changelists.data + e->changelistIndex;
	const char *str = sdict_find_safe(sd, field->key);
	if(field->type == kChangesetColumn_Numeric) {
		str = (const char *)(ptrdiff_t)atoi(str);
	}
	return str;
}
static int p4_changeset_compare(const void *_a, const void *_b)
{
	const p4UIChangesetSortKey *a = _a;
	const p4UIChangesetSortKey *b = _b;
	const char *astr = a->sortKey;
	const char *bstr = b->sortKey;

	int mult = s_sortConfig->sortDescending ? -1 : 1;
	u32 columnIndex = s_sortConfig->sortColumn;
	const changesetColumnField *field = s_changesetColumnFields + columnIndex;
	int val;
	if(field->type == kChangesetColumn_Numeric) {
		int aint = (int)(ptrdiff_t)astr;
		int bint = (int)(ptrdiff_t)bstr;
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
		return s_sortConfig->sortDescending ? (a->entryIndex > b->entryIndex) : (a->entryIndex < b->entryIndex);
	}
}
void p4_sort_uichangeset(p4UIChangeset *uics)
{
	for(u32 i = 0; i < p4.changesets.count; ++i) {
		p4Changeset *cs = p4.changesets.data + i;
		if(cs->pending == uics->config.pending) {
			s_sortChangeset = cs;
			break;
		}
	}
	if(!s_sortChangeset) {
		return;
	}
	s_sortConfig = s_sortChangeset->pending ? &g_config.uiPendingChangesets : &g_config.uiSubmittedChangesets;
	BB_LOG("p4::sort", "prep changeset sort - entries:%u", s_sortChangeset->changelists.count);
	uics->sorted.count = 0;
	for(u32 i = 0; i < uics->entries.count; ++i) {
		p4UIChangesetEntry *e = uics->entries.data + i;
		p4UIChangesetSortKey s = { 0 };
		s.entryIndex = i;
		s.sortKey = p4_get_uichangesetentry_sort_key(e);
		bba_push(uics->sorted, s);
	}
	BB_LOG("p4::sort", "start changeset sort - entries:%u sorted:%u", uics->entries.count, uics->sorted.count);
	qsort(uics->sorted.data, uics->sorted.count, sizeof(p4UIChangesetSortKey), &p4_changeset_compare);
	BB_LOG("p4::sort", "end changeset sort");
	s_sortConfig = NULL;
	s_sortChangeset = NULL;
}

p4UIChangeset *p4_add_uichangeset(b32 pending)
{
	if(bba_add(p4.uiChangesets, 1)) {
		p4UIChangeset *cs = &bba_last(p4.uiChangesets);
		cs->id = ++p4.lastId;
		cs->config.pending = pending;
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

void p4_mark_uichangeset_for_removal(p4UIChangeset *uics)
{
	uics->id = 0;
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
	files->selectedCount = 0;
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
			b32 unresolved = false;
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
