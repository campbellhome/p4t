// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4_task.h"

#include "app.h"
#include "output.h"

#include "bb_array.h"

void task_p4_tick(task *_t)
{
	task_p4 *t = (task_p4 *)_t->userdata;
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
	task_p4 *t = (task_p4 *)_t->userdata;
	sdicts_reset(&t->dicts);
	bba_free(t->parser);
	sdict_reset(&t->parser.dict);
	sdict_reset(&t->extraData);
	task_process_reset(_t);
}

task p4_task_create(Task_StateChanged *statechanged, const char *dir, sdict_t *extraData, const char *cmdlineFmt, ...)
{
	task t = { 0 };
	t.tick = task_p4_tick;
	t.stateChanged = statechanged;
	t.reset = task_p4_reset;
	t.userdata = malloc(sizeof(task_p4));

	task_p4 *p = t.userdata;
	if(p) {
		memset(p, 0, sizeof(*p));
		sb_append(&p->base.dir, dir);
		if(extraData) {
			sdict_move(&p->extraData, extraData);
		}
		va_list args;
		va_start(args, cmdlineFmt);
		sb_va_list(&p->base.cmdline, cmdlineFmt, args);
		va_end(args);
		p->base.spawnType = kProcessSpawn_Tracked;
	}
	return t;
}
