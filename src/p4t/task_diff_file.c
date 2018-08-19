// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "task_diff_file.h"

#include "bb_array.h"
#include "config.h"
#include "env_utils.h"
#include "file_utils.h"
#include "p4.h"
#include "p4_task.h"
#include "path_utils.h"
#include "process_task.h"
#include "str.h"
#include "va.h"

static sbs_t s_diffFiles;
static sbs_t s_diffDirs;
static u32 s_diffCount;

const char *diff_exe(void)
{
	if(g_config.diff.enabled && g_config.diff.path.count) {
		return g_config.diff.path.data;
	}
	const char *diffExe = sdict_find(&p4.set, "P4DIFF");
	if(diffExe)
		return diffExe;
	diffExe = p4_exe();
	const char *end = strrchr(diffExe, '\\');
	if(end) {
		return va("%.*s\\p4merge.exe", end - diffExe, diffExe);
	}
	return "";
}

void p4_diff_against_local(const char *depotPath, const char *rev, const char *localPath, bool depotFirst)
{
	const char *filename = strrchr(depotPath, '/');
	if(!filename++)
		return;

	const char *ext = strrchr(filename, '.');

	sb_t temp = env_get("TEMP");
	if(!temp.data)
		return;

	DWORD procId = GetCurrentProcessId();
	sb_t diffDir = { 0 }, target = { 0 };
	sb_va(&diffDir, "%s\\p4t\\%u\\%u", sb_get(&temp), procId, s_diffCount);
	if(ext > filename) {
		sb_va(&target, "%s\\%.*s%s%s", sb_get(&diffDir), ext - filename, filename, rev, ext);
	} else {
		sb_va(&target, "%s\\%s%s", sb_get(&diffDir), filename, rev);
	}
	sb_reset(&temp);
	++s_diffCount;

	const char *p4dir = p4_dir();
	const char *p4exe = p4_exe();
	const char *diffExe = diff_exe();

	task t = { 0 };
	t.tick = task_tick_subtasks;
	sb_append(&t.name, "diff_against_local");
	bba_push(t.subtasks, p4_task_create(va("diff_fetch_%s%s", depotPath, rev), task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&target), depotPath, rev));
	bba_push(t.subtasks, process_task_create("diff", kProcessSpawn_OneShot, p4dir,
	                                         "\"%s\" \"%s\" \"%s\"",
	                                         diffExe, (depotFirst) ? sb_get(&target) : localPath,
	                                         (depotFirst) ? localPath : sb_get(&target)));
	task_queue(t);

	bba_push(s_diffDirs, diffDir);
	bba_push(s_diffFiles, target);
}

void p4_diff_against_depot(const char *depotPathA, const char *revA, const char *depotPathB, const char *revB)
{
	const char *filenameA = strrchr(depotPathA, '/');
	const char *filenameB = strrchr(depotPathB, '/');
	if(!filenameA++ || !filenameB++)
		return;

	sb_t temp = env_get("TEMP");
	if(!temp.data)
		return;

	const char *extA = strrchr(filenameA, '.');
	const char *extB = strrchr(filenameB, '.');

	DWORD procId = GetCurrentProcessId();
	sb_t diffDir = { 0 }, targetA = { 0 }, targetB = { 0 };
	sb_va(&diffDir, "%s\\p4t\\%u\\%u", sb_get(&temp), procId, s_diffCount);
	if(extA > filenameA) {
		sb_va(&targetA, "%s\\%.*s%s%s", sb_get(&diffDir), extA - filenameA, filenameA, revA, extA);
	} else {
		sb_va(&targetA, "%s\\%s%s", sb_get(&diffDir), filenameA, revA);
	}
	if(extB > filenameB) {
		sb_va(&targetB, "%s\\%.*s%s%s", sb_get(&diffDir), extB - filenameB, filenameB, revB, extB);
	} else {
		sb_va(&targetB, "%s\\%s%s", sb_get(&diffDir), filenameB, revB);
	}
	sb_reset(&temp);
	++s_diffCount;

	const char *p4dir = p4_dir();
	const char *p4exe = p4_exe();
	const char *diffExe = diff_exe();

	task t = { 0 };
	sb_append(&t.name, "diff_against_depot");
	t.tick = task_tick_subtasks;
	bba_push(t.subtasks, p4_task_create(va("diff_fetch_%s%s", depotPathA, revA), task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&targetA), depotPathA, revA));
	bba_push(t.subtasks, p4_task_create(va("diff_fetch_%s%s", depotPathB, revB), task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&targetB), depotPathB, revB));
	bba_push(t.subtasks, process_task_create("diff", kProcessSpawn_OneShot, p4dir,
	                                         "\"%s\" \"%s\" \"%s\"",
	                                         diffExe, sb_get(&targetA), sb_get(&targetB)));
	task_queue(t);

	bba_push(s_diffDirs, diffDir);
	bba_push(s_diffFiles, targetA);
	bba_push(s_diffFiles, targetB);
}

typedef struct tag_diffDepotStrings {
	const char *filename;
	const char *ext;
	sb_t target;
} diffDepotStrings;

static diffDepotStrings p4_diff_get_depotLocations(const char *diffDir, const char *srcPath, const char *srcRevision, b32 isDepotPath)
{
	diffDepotStrings out = { 0 };
	if(isDepotPath) {
		out.filename = strrchr(srcPath, '/');
		if(!out.filename++)
			return out;
		out.ext = strrchr(out.filename, '.');
		if(out.ext > out.filename) {
			sb_va(&out.target, "%s\\%.*s%s%s", diffDir, out.ext - out.filename, out.filename, srcRevision, out.ext);
		} else {
			sb_va(&out.target, "%s\\%s%s", diffDir, out.filename, srcRevision);
		}
	} else {
		sb_append(&out.target, srcPath);
	}
	return out;
}

void p4_diff_file_locators(const p4FileLocator *locatorA, const p4FileLocator *locatorB)
{
	sb_t temp = env_get("TEMP");
	if(!temp.data)
		return;

	DWORD procId = GetCurrentProcessId();
	sb_t diffDir = { 0 };
	sb_va(&diffDir, "%s\\p4t\\%u\\%u", sb_get(&temp), procId, s_diffCount++);

	const char *depotPathA = sb_get(&locatorA->path);
	const char *revA = sb_get(&locatorA->revision);
	const char *depotPathB = sb_get(&locatorB->path);
	const char *revB = sb_get(&locatorB->revision);
	diffDepotStrings strA = p4_diff_get_depotLocations(sb_get(&diffDir), depotPathA, revA, locatorA->depotPath);
	diffDepotStrings strB = p4_diff_get_depotLocations(sb_get(&diffDir), depotPathB, revB, locatorB->depotPath);

	if(strA.target.data && strB.target.data) {
		const char *p4dir = p4_dir();
		const char *p4exe = p4_exe();
		const char *diffExe = diff_exe();
		task t = { 0 };
		sb_append(&t.name, "diff_against_depot");
		t.tick = task_tick_subtasks;
		if(locatorA->depotPath) {
			bba_push(t.subtasks, p4_task_create(va("diff_fetch_%s%s", depotPathA, revA), task_process_statechanged, p4dir, NULL,
			                                    "\"%s\" -G print -o %s %s%s",
			                                    p4exe, sb_get(&strA.target), depotPathA, revA));
		}
		if(locatorB->depotPath) {
			bba_push(t.subtasks, p4_task_create(va("diff_fetch_%s%s", depotPathB, revB), task_process_statechanged, p4dir, NULL,
			                                    "\"%s\" -G print -o %s %s%s",
			                                    p4exe, sb_get(&strB.target), depotPathB, revB));
		}
		bba_push(t.subtasks, process_task_create("diff", kProcessSpawn_OneShot, p4dir,
		                                         "\"%s\" \"%s\" \"%s\"",
		                                         diffExe, sb_get(&strA.target), sb_get(&strB.target)));
		bba_push(s_diffDirs, diffDir);
		if(locatorA->depotPath) {
			bba_push(s_diffFiles, strA.target);
		} else {
			sb_reset(&strA.target);
		}
		if(locatorB->depotPath) {
			bba_push(s_diffFiles, strB.target);
		} else {
			sb_reset(&strB.target);
		}
		task_queue(t);
	} else {
		sb_reset(&diffDir);
		sb_reset(&strA.target);
		sb_reset(&strB.target);
	}

	sb_reset(&temp);
}

void p4_diff_shutdown(void)
{
	for(u32 i = 0; i < s_diffFiles.count; ++i) {
		file_delete(sb_get(s_diffFiles.data + i));
	}
	sbs_reset(&s_diffFiles);

	for(u32 i = 0; i < s_diffDirs.count; ++i) {
		path_rmdir(sb_get(s_diffDirs.data + i));
	}
	sbs_reset(&s_diffDirs);

	DWORD procId = GetCurrentProcessId();
	sb_t dir = env_resolve(va("%%TEMP%%\\p4t\\diff\\%u", procId));
	path_rmdir(sb_get(&dir));
	sb_reset(&dir);
}
