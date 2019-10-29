// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#include "appdata.h"
#include "bb.h"
#include "bb_array.h"
#include "cmdline.h"
#include "crt_leak_check.h"
#include "mc_updater/mc_updater.h"
#include "path_utils.h"
#include "sb.h"

int main(int argc, const char **argv)
{
	crt_leak_check_init();
	cmdline_init(argc, argv);

	mc_updater_globals globals = { BB_EMPTY_INITIALIZER };
	globals.appName = "p4t.exe";
	globals.appdataName = "p4t";
	globals.currentVersionJsonFilename = "p4t_current_version.json";
	globals.p4VersionDir = "..\\..\\..\\..\\..";
	globals.manifestFilename = "p4t_build_manifest.json";

	if(cmdline_find("-nolog") <= 0) {
		sb_t logPath = appdata_get(globals.appdataName);
		sb_append(&logPath, "/p4tupdater");
#if BB_USING(BB_PLATFORM_WINDOWS)
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		u64 timeVal = li.QuadPart;
		sb_va(&logPath, "/{%llu}p4tupdater.bbox", timeVal);
#else
		sb_append(&logPath, "/p4tupdater.bbox");
#endif
		path_mkdir(sb_get(&logPath));
		path_resolve_inplace(&logPath);
		bb_init_file(sb_get(&logPath));
		sb_reset(&logPath);
	}

	bb_set_send_callback(&bb_echo_to_stdout, NULL);
	BB_INIT_WITH_FLAGS("mc_updater", cmdline_find("-bb") > 0 ? kBBInitFlag_None : kBBInitFlag_NoDiscovery);
	BB_THREAD_START("main");
	BB_TRACE(kBBLogLevel_Verbose, "Startup", "Command line: %s", cmdline_get_full());

	bba_push(globals.contentsFilenames, "p4t.exe");
	bba_push(globals.contentsFilenames, "p4t.pdb");
	bba_push(globals.contentsFilenames, "p4t_site_config.json");
	bba_push(globals.contentsFilenames, "freetype.dll");

	b32 success = mc_updater_main(&globals);

	free((void *)&globals.contentsFilenames.data);

	cmdline_shutdown();
	BB_SHUTDOWN();
	return success ? 0 : 1;
}
