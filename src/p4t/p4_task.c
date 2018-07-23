// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4_task.h"

#include "app.h"
#include "output.h"

#include "bb_array.h"

void task_p4_tick(task *_t)
{
	task_p4 *t = (task_p4 *)_t;
	processTickResult_t res = process_tick(t->base.process);
	if(res.stdoutIO.nBytes) {
		bba_add_array(t->base.stdoutBuf, res.stdoutIO.buffer, res.stdoutIO.nBytes);
		App_RequestRender();
		bba_add_array(t->parser, res.stdoutIO.buffer, res.stdoutIO.nBytes);
		while(py_parser_tick(&t->parser, &t->dicts)) {
			// do nothing
		}
	}
	if(res.stderrIO.nBytes) {
		bba_add_array(t->base.stderrBuf, res.stderrIO.buffer, res.stderrIO.nBytes);
		App_RequestRender();
		output_error("%.*s\n", res.stderrIO.nBytes, res.stderrIO.buffer);
	}
	if(res.done) {
		task_set_state(_t, t->base.stderrBuf.count ? kTaskState_Failed : kTaskState_Succeeded);
	}
	task_tick_subtasks(_t);
}

void task_p4_reset(task *_t)
{
	task_p4 *t = (task_p4 *)_t;
	for(u32 i = 0; i < t->dicts.count; ++i) {
		sdict_reset(t->dicts.data + i);
	}
	bba_free(t->dicts);
	bba_free(t->parser);
	sdict_reset(&t->parser.dict);
	task_process_reset(&t->base.header);
}

task *p4_task_queue(Task_StateChanged *statechanged, const char *dir, const char *cmdlineFmt, ...)
{
	task_p4 *p = malloc(sizeof(task_p4));
	memset(p, 0, sizeof(*p));
	p->base.header.tick = task_p4_tick;
	p->base.header.stateChanged = statechanged;
	p->base.header.reset = task_p4_reset;
	p->base.header.autoReset = true;
	sb_append(&p->base.dir, dir);
	va_list args;
	va_start(args, cmdlineFmt);
	sb_va_list(&p->base.cmdline, cmdlineFmt, args);
	va_end(args);
	p->base.spawnType = kProcessSpawn_Tracked;
	return task_queue(&p->base.header);
}
