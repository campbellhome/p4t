// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4.h"

#include "app.h"
#include "env_utils.h"
#include "file_utils.h"
#include "output.h"
#include "process.h"
#include "py_parser.h"
#include "span.h"
#include "str.h"
#include "tokenize.h"
#include "va.h"

#include "bb.h"
#include "bb_array.h"

#include <stdlib.h>

typedef struct tag_p4Process {
	process_t *process;
	sdicts dicts;
	pyParser parser;
	p4Operation op;
	u8 pad[4];
} p4Process;

typedef struct tag_p4Processes {
	u32 count;
	u32 allocated;
	p4Process *data;
} p4Processes;

typedef struct tag_p4Changelist {
	sdict_t dict;
	u32 number;
	u8 pad[4];
} p4Changelist;

typedef struct tag_p4Changelists {
	u32 count;
	u32 allocated;
	p4Changelist *data;
} p4Changelists;

typedef struct tag_p4 {
	sb_t exe;
	p4Processes procs;

	sdict_t info;
	p4Changelists changelists;
} p4_t;
static p4_t p4;

static void p4_remove_proc(u32 index)
{
	p4Process proc = p4.procs.data[index];
	for(u32 i = 0; i < proc.dicts.count; ++i) {
		sdict_reset(proc.dicts.data + i);
	}
	bba_free(proc.dicts);
	bba_free(proc.parser);
	sdict_reset(&proc.parser.dict);
	process_free(proc.process);
	bba_erase(p4.procs, index);
}

static const char *p4_exe(void)
{
	return sb_get(&p4.exe);
}

static const char *p4_dir(void)
{
	return "D:\\Backups\\MattC_Home\\Projects";
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

void p4_shutdown(void)
{
	sb_reset(&p4.exe);
	while(p4.procs.count) {
		p4_remove_proc(0);
	}
	sdict_reset(&p4.info);
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		sdict_reset(&p4.changelists.data[i].dict);
	}
	bba_free(p4.changelists);
	bba_free(p4.procs);
}

sdict_t *p4_find_changelist(u32 cl)
{
	for(u32 i = 0; i < p4.changelists.count; ++i) {
		p4Changelist *change = p4.changelists.data + i;
		if(change->number == cl) {
			return &change->dict;
		}
	}
	return NULL;
}

static void p4_store_info(p4Process *proc)
{
	if(proc->dicts.count == 1) {
		sdict_move(&p4.info, proc->dicts.data);
	}
}

static void p4_store_changelist(p4Process *proc)
{
	if(proc->dicts.count == 1) {
		const char *change = sdict_find(proc->dicts.data, "change");
		if(change) {
			p4Changelist cl = { 0 };
			cl.number = strtou32(change);
			sdict_move(&cl.dict, proc->dicts.data);
			//for(u32 i = 0; i < cl.dict.count; ++i) {
			//	sdictEntry_t *e = cl.dict.data + i;
			//	output_log("changelist [%s]=%s\n", sb_get(&e->key), sb_get(&e->value));
			//}
			if(bba_add_noclear(p4.changelists, 1)) {
				bba_last(p4.changelists) = cl;
			} else {
				sdict_reset(&cl.dict);
			}
		}
	}
}

void p4_tick(void)
{
	u32 index = 0;
	while(index < p4.procs.count) {
		p4Process *proc = p4.procs.data + index;
		processTickResult_t res = process_tick(proc->process);
		if(res.stdoutIO.nBytes) {
			App_RequestRender();
			bba_add_array(proc->parser, res.stdoutIO.buffer, res.stdoutIO.nBytes);
			while(py_parser_tick(&proc->parser, &proc->dicts)) {
				// do nothing
			}
		}
		if(res.stderrIO.nBytes) {
			App_RequestRender();
			output_error("%.*s\n", res.stderrIO.nBytes, res.stderrIO.buffer);
		}
		if(res.done) {
			//output_log("process finished: %s\n", proc->process->command);
			switch(proc->op) {
			case kP4Op_Info:
				p4_store_info(proc);
				break;

			case kP4Op_DescribeChangelist:
				p4_store_changelist(proc);
				break;

			case kP4Op_Changes:
			default:
				break;
			}
			p4_remove_proc(index);
		} else {
			++index;
		}
	}
}

static b32 p4_track_process(p4Operation op, processSpawnResult_t spawn)
{
	b32 ret;
	if(spawn.process) {
		p4Process proc;
		memset(&proc, 0, sizeof(proc));
		proc.op = op;
		proc.process = spawn.process;
		proc.parser.cmdline = proc.process->command;
		bba_push(p4.procs, proc);
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}

void p4_info(void)
{
	p4_track_process(kP4Op_Info, process_spawn(p4_dir(), va("\"%s\" -G info", p4_exe()), kProcessSpawn_Tracked));
	//p4_track_process(kP4Op_Info, process_spawn(p4_dir(), va("\"%s\" -G describe 1", p4_exe()), kProcessSpawn_Tracked));
	//p4_track_process(kP4Op_Info, process_spawn(p4_dir(), va("\"%s\" -G describe 4", p4_exe()), kProcessSpawn_Tracked));
}

void p4_changes(void)
{
	p4_track_process(kP4Op_Changes, process_spawn(p4_dir(), va("\"%s\" -G changes", p4_exe()), kProcessSpawn_Tracked));
}

void p4_describe_changelist(u32 cl)
{
	p4_track_process(kP4Op_DescribeChangelist, process_spawn(p4_dir(), va("\"%s\" -G describe -s %u", p4_exe(), cl), kProcessSpawn_Tracked));
}
