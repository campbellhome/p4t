// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

void cmdline_init_composite(const char *cmdline);
void cmdline_init(int argc, const char **argv);
void cmdline_shutdown(void);

extern int argc;
extern const char **argv;

int cmdline_find(const char *arg);
const char *cmdline_get_exe_dir(void);
const char *cmdline_get_exe_filename(void);

#if defined(__cplusplus)
}
#endif
