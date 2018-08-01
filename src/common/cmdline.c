// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "cmdline.h"
#include "bb_array.h"
#include "tokenize.h"
#include <stdlib.h>

int argc;
const char **argv;

char *s_argBuffer;
char **s_argvBuffer;
static char s_argv0[_MAX_PATH];

void cmdline_init_composite(const char *src)
{
	s_argBuffer = _strdup(src);
	if(s_argBuffer) {
		++argc;
		char *cursor = s_argBuffer;
		span_t token = tokenize(&cursor, " ");
		while(token.start) {
			++argc;
			token = tokenize(&cursor, " ");
		}

		argv = s_argvBuffer = malloc(sizeof(char *) * argc);
		if(argv) {
			argv[0] = s_argv0;
			GetModuleFileNameA(NULL, s_argv0, BB_ARRAYSIZE(s_argv0));
			cursor = s_argBuffer;
			for(int i = 1; i < argc; ++i) {
				token = tokenize(&cursor, " ");
				if(*cursor)
					++cursor;
				*(char *)token.end = '\0';
				argv[i] = token.start;
			}
		} else {
			argc = 0;
		}
	}
}

void cmdline_init(int _argc, const char **_argv)
{
	argc = _argc;
	argv = _argv;
}

void cmdline_shutdown(void)
{
	if(s_argBuffer) {
		free(s_argBuffer);
	}
	if(s_argvBuffer) {
		free(s_argvBuffer);
	}
}

int cmdline_find(const char *arg)
{
	for(int i = 0; i < argc; ++i) {
		if(!_stricmp(argv[i], arg)) {
			return i;
		}
	}
	return -1;
}
