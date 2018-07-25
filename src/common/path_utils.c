// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "path_utils.h"
#include "bb_array.h"

#include "bb_wrap_stdio.h"

sb_t path_resolve(sb_t src)
{
	sb_t pathData;
	sb_init(&pathData);
	sb_t *path = &pathData;

	const char *in = sb_get(&src);
	while(*in) {
		char c = *in;
		if(c == '/') {
			c = '\\';
		}

		b32 handled = false;
		if(c == '.' && path->count > 1 && path->data[path->count - 2] == '\\') {
			if(in[1] == '.' && (in[2] == '/' || in[2] == '\\' || !in[2])) {
				char *prevSep = path->data + path->count - 2;
				while(prevSep > path->data) {
					--prevSep;
					if(*prevSep == '\\') {
						*prevSep = 0;
						path->count = (u32)(prevSep - path->data) + 1;
						sb_append_char(path, '\\');
						in += 2;
						if(*in) {
							++in;
						}
						handled = true;
						break;
					}
				}
			} else if(in[1] == '/' || in[1] == '\\' || !in[1]) {
				++in;
				if(*in) {
					++in;
				}
				handled = true;
			}
		}

		if(!handled) {
			sb_append_char(path, c);
			++in;
		}
	}

	if(path->count > 1 && path->data[path->count - 2] == '\\') {
		path->data[path->count - 2] = '\0';
		--path->count;
	}
	return pathData;
}

void path_resolve_inplace(sb_t *path)
{
	sb_t tmp = path_resolve(*path);
	sb_reset(path);
	*path = tmp;
}

#if BB_USING(BB_PLATFORM_WINDOWS)
#include <direct.h>
bool path_mkdir(const char *path)
{
	bool success = true;
	char *temp = _strdup(path);
	char *s = temp;
	while(*s) {
		if(*s == '/' || *s == '\\') {
			char c = *s;
			*s = '\0';
			if(s - temp > 2) {
				if(_mkdir(temp) == -1) {
					if(errno != EEXIST) {
						success = false;
					}
				}
			}
			*s = c;
		}
		++s;
	}
	free(temp);
	if(_mkdir(path) == -1) {
		if(errno != EEXIST) {
			success = false;
		}
	}
	return success;
}
bool path_rmdir(const char *path)
{
	return _rmdir(path);
}
#else
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
static const char *errno_str(int e)
{
	switch(e) {
#define CASE(x) \
	case x:     \
		return #x
		CASE(EACCES);
		CASE(EEXIST);
		CASE(ELOOP);
		CASE(EMLINK);
		CASE(ENAMETOOLONG);
		CASE(ENOENT);
		CASE(ENOSPC);
		CASE(ENOTDIR);
		CASE(EROFS);
	case 0:
		return "success";
	default:
		return "???";
	}
}

void path_mkdir(const char *path)
{
	mode_t process_mask = umask(0);
	int ret = mkdir(path, S_IRWXU);
	umask(process_mask);
	BB_LOG("mkdir", "mkdir '%s' returned %d (errno %d %s)\n", path, ret, errno, errno_str(errno));
}
#endif
