// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "p4_task.h"
#include "app.h"
#include "appdata.h"
#include "bb_array.h"
#include "file_utils.h"
#include "output.h"

void task_p4_tick(task *_t)
{
	task_p4 *t = (task_p4 *)_t->userdata;
	processTickResult_t res = process_tick(t->base.process);
	if(res.stdoutIO.nBytes) {
		App_RequestRender();
		bba_add_array(t->parser, res.stdoutIO.buffer, res.stdoutIO.nBytes);
		while(py_parser_tick(&t->parser, &t->parsedDicts)) {
			// do nothing
		}
	}
	if(res.stderrIO.nBytes) {
		App_RequestRender();
		output_error("%.*s\n", res.stderrIO.nBytes, res.stderrIO.buffer);
	}
	if(res.done) {
		task_set_state(_t, t->base.process && t->base.process->stderrBuffer.count ? kTaskState_Failed : kTaskState_Succeeded);
	}
	task_tick_subtasks(_t);
}

void task_p4_reset(task *_t)
{
	task_p4 *t = (task_p4 *)_t->userdata;
	sdicts_reset(&t->parsedDicts);
	if(t->parser.state == kParser_Error) {
		sb_t path = appdata_get();
		sb_va(&path, "\\%s_error.bin", globals.appSpecific.configName);
		fileData_t fd = { 0 };
		fd.buffer = t->parser.data;
		fd.bufferSize = t->parser.count;
		if(fd.buffer && path.data) {
			BB_LOG("p4::parser::postmortem", "begin save err data - path:%s", sb_get(&path));
			BB_FLUSH();
			b32 wrote = fileData_writeIfChanged(path.data, NULL, fd);
			if(wrote) {
				BB_LOG("p4::parser::postmortem", "end save err data - wrote:%u", wrote);
			} else {
				BB_ERROR("p4::parser::postmortem", "end save err data");
			}
		}
		sb_reset(&path);
	}
	bba_free(t->parser);
	sdict_reset(&t->parser.dict);
	sdict_reset(&t->extraData);
	task_process_reset(_t);
}

task p4_task_create(const char *name, Task_StateChanged *statechanged, const char *dir, sdict_t *extraData, const char *cmdlineFmt, ...)
{
	task t = { 0 };
	sb_append(&t.name, name);
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
