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

void p4_diff_against_local(const char *depotPath, const char *rev, const char *localPath)
{
	const char *filename = strrchr(depotPath, '/');
	if(!filename++)
		return;

	sb_t temp = env_get("TEMP");
	if(!temp.data)
		return;

	DWORD procId = GetCurrentProcessId();
	sb_t diffDir = { 0 }, target = { 0 };
	sb_va(&diffDir, "%s\\p4t\\%u\\%u", sb_get(&temp), procId, s_diffCount);
	sb_va(&target, "%s\\%s%s", sb_get(&diffDir), filename, rev);
	sb_reset(&temp);
	++s_diffCount;

	const char *p4dir = p4_dir();
	const char *p4exe = p4_exe();
	const char *diffExe = diff_exe();

	task t = { 0 };
	t.tick = task_tick_subtasks;
	bba_push(t.subtasks, p4_task_create(task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&target), depotPath, rev));
	bba_push(t.subtasks, process_task_create(kProcessSpawn_OneShot, p4dir,
	                                         "\"%s\" \"%s\" \"%s\"",
	                                         diffExe, sb_get(&target), localPath));
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

	DWORD procId = GetCurrentProcessId();
	sb_t diffDir = { 0 }, targetA = { 0 }, targetB = { 0 };
	sb_va(&diffDir, "%s\\p4t\\%u\\%u", sb_get(&temp), procId, s_diffCount);
	sb_va(&targetA, "%s\\%s%s", sb_get(&diffDir), filenameA, revA);
	sb_va(&targetB, "%s\\%s%s", sb_get(&diffDir), filenameB, revB);
	sb_reset(&temp);
	++s_diffCount;

	const char *p4dir = p4_dir();
	const char *p4exe = p4_exe();
	const char *diffExe = diff_exe();

	task t = { 0 };
	t.tick = task_tick_subtasks;
	bba_push(t.subtasks, p4_task_create(task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&targetA), depotPathA, revA));
	bba_push(t.subtasks, p4_task_create(task_process_statechanged, p4dir, NULL,
	                                    "\"%s\" -G print -o %s %s%s",
	                                    p4exe, sb_get(&targetB), depotPathB, revB));
	bba_push(t.subtasks, process_task_create(kProcessSpawn_OneShot, p4dir,
	                                         "\"%s\" \"%s\" \"%s\"",
	                                         diffExe, sb_get(&targetA), sb_get(&targetB)));
	task_queue(t);

	bba_push(s_diffDirs, diffDir);
	bba_push(s_diffFiles, targetA);
	bba_push(s_diffFiles, targetB);
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