// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum output_level_e {
	kOutput_Log,
	kOutput_Warning,
	kOutput_Error,
} output_level_t;

typedef struct output_line_s {
	sb_t text;
	output_level_t level;
	u8 pad[4];
} output_line_t;

typedef struct output_s {
	u32 count;
	u32 allocated;
	output_line_t *data;
} output_t;

extern output_t g_output;

void output_init(void);
void output_shutdown(void);

void output_log(const char *fmt, ...);
void output_warning(const char *fmt, ...);
void output_error(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif
