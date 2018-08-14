// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "filter.h"
#include "sdict.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sdict_s sdict_t;

b32 p4_init(void);
void p4_shutdown(void);
void p4_update(void);

typedef struct tag_p4Changelist {
	sdict_t normal;
	sdict_t shelved;
	sdicts normalFiles;
	sdicts shelvedFiles;
	u32 number;
	u32 parity;
} p4Changelist;

typedef struct tag_p4Changelists {
	u32 count;
	u32 allocated;
	p4Changelist *data;
} p4Changelists;

typedef struct tag_uiChangelistFile {
	union {
		char *str[6];
		struct {
			char *filename;
			char *rev;
			char *action;
			char *filetype;
			char *depotPath;
			char *localPath;
		} field;
	} fields;
	b32 unresolved;
	b32 selected;
} uiChangelistFile;

typedef struct tag_uiChangelistFiles {
	u32 count;
	u32 allocated;
	uiChangelistFile *data;
	u32 lastClickIndex;
	b32 shelved;
	u32 sortColumn;
	b32 sortDescending;
	u32 selectedCount;
	u8 pad[4];
} uiChangelistFiles;

typedef struct tag_p4Changeset {
	b32 pending;
	u32 parity;
	sdicts changelists;
	u32 highestReceived;
	b32 refreshed;
	b32 updating;
	u8 pad[4];
} p4Changeset;

typedef struct tag_p4Changesets {
	u32 count;
	u32 allocated;
	p4Changeset *data;
} p4Changesets;

typedef struct tag_p4UIChangesetEntry {
	u32 changelistNumber;
	u32 changelistIndex;
	b32 selected;
	b32 described;
	u32 parity;
	u8 pad[4];
	const char *sortKey;
	sb_t client;
	float startY;
	float height;
	uiChangelistFiles normalFiles;
	uiChangelistFiles shelvedFiles;
} p4UIChangesetEntry;

typedef struct tag_p4UIChangesetEntries {
	u32 count;
	u32 allocated;
	p4UIChangesetEntry *data;
} p4UIChangesetEntries;

typedef struct tag_p4UIChangesetSortKey {
	u32 entryIndex;
	u8 pad[4];
	const char *sortKey;
} p4UIChangesetSortKey;

typedef struct tag_p4UIChangesetSortKeys {
	u32 count;
	u32 allocated;
	p4UIChangesetSortKey *data;
} p4UIChangesetSortKeys;

typedef struct tag_p4UIChangeset {
	b32 pending;
	u32 parity;
	u32 numChangelistsAppended;
	u8 pad[4];
	p4UIChangesetEntries entries;
	p4UIChangesetSortKeys sorted;
	sb_t user;
	sb_t clientspec;
	sb_t filter;
	sb_t filterInput;
	filterTokens filterTokens;
	b32 filterEnabled;
	u32 id;
	u32 lastClickIndex;
	u32 numValidStartY;
	u32 lastStartIndex;
	float lastStartY;
} p4UIChangeset;

typedef struct tag_p4UIChangesets {
	u32 count;
	u32 allocated;
	p4UIChangeset *data;
} p4UIChangesets;

typedef enum tag_changesetColumnType {
	kChangesetColumn_Text,
	kChangesetColumn_Time,
	kChangesetColumn_Numeric,
	kChangesetColumn_TextMultiline,
} changesetColumnType;

typedef struct tag_changesetColumnField {
	const char *key;
	changesetColumnType type;
	u8 pad[4];
} changesetColumnField;

typedef struct tag_p4UIChangelist {
	u32 id;
	u32 requested;
	u32 displayed;
	u32 parity;
	uiChangelistFiles normalFiles;
	uiChangelistFiles shelvedFiles;
} p4UIChangelist;

typedef struct tag_p4UIChangelists {
	u32 count;
	u32 allocated;
	p4UIChangelist *data;
} p4UIChangelists;

typedef struct tag_p4FileLocator
{
	sb_t path;
	sb_t revision;
	b32 depotPath;
	u8 pad[4];
} p4FileLocator;

typedef struct tag_p4 {
	sb_t exe;

	sdict_t info;
	sdict_t set;
	sdicts allUsers;
	sdicts allClients;
	sdicts selfClients;
	sdicts localClients;
	p4Changelists changelists;
	p4UIChangelists uiChangelists;
	p4Changesets changesets;
	p4UIChangesets uiChangesets;
	const changesetColumnField *changesetColumnFields;
	p4FileLocator diffLeftSide;
	u32 lastId;
	u8 pad[4];
} p4_t;
extern p4_t p4;

const char *p4_exe(void);
const char *p4_dir(void);
const char *p4_clientspec(void);
const char *p4_clientspec_arg(void);
void p4_set_clientspec(const char *client);

//////////////////////////////////////////////////////////////////////////

// general: code(stat), change, user, client, time, desc, status, changeType, path, shelved
// per-file: depotFile0, action0, type0, rev0, fileSize0, digest0
p4Changelist *p4_find_changelist(u32 cl);
p4Changelist *p4_find_default_changelist(const char *client);

typedef enum tag_p4ChangelistType {
	kChangelistType_PendingOther,
	kChangelistType_PendingLocal,
	kChangelistType_Submitted,
} p4ChangelistType;
p4ChangelistType p4_get_changelist_type(sdict_t *cl);

sdict_t *p4_get_info(void);

//////////////////////////////////////////////////////////////////////////

void p4_info(void);

p4Changeset *p4_find_or_add_changeset(b32 pending);
void p4_refresh_changeset(p4Changeset *cs);
void p4_refresh_changelist_no_cache(p4Changeset *cs);
void p4_request_newer_changes(p4Changeset *cs, u32 blockSize);

p4UIChangeset *p4_add_uichangeset(b32 pending);
p4UIChangeset *p4_find_uichangeset(u32 id);
void p4_sort_uichangeset(p4UIChangeset *cs);

void p4_build_changelist_files(p4Changelist *cl, uiChangelistFiles *normalFiles, uiChangelistFiles *shelvedFiles);
void p4_free_changelist_files(uiChangelistFiles *files);
int p4_changelist_files_compare(const void *_a, const void *_b);

void p4_reset_file_locator(p4FileLocator *locator);

//////////////////////////////////////////////////////////////////////////

void p4_reset_uichangelist(p4UIChangelist *uicl);
p4UIChangelist *p4_add_uichangelist(void);
p4UIChangelist *p4_find_uichangelist(u32 id);
void p4_mark_uichangelist_for_removal(p4UIChangelist *uicl);

//////////////////////////////////////////////////////////////////////////
// internal:
void p4_build_default_changelist(sdict_t *sd, const char *owner, const char *client);
void p4_reset_changelist(p4Changelist *cl);
void p4_reset_uichangesetentry(p4UIChangesetEntry *e);

#include "task_describe_changelist.h"
#include "task_diff_file.h"

#if defined(__cplusplus)
}
#endif
