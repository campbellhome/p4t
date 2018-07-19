// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "appdata.h"
#include "path_utils.h"

#if BB_USING(BB_PLATFORM_WINDOWS)
// warning C4820 : 'StructName' : '4' bytes padding added after data member 'MemberName'
// warning C4255: 'FuncName': no function prototype given: converting '()' to '(void)'
BB_WARNING_PUSH(4820 4255)
#include <ShlObj.h>
sb_t appdata_get(void)
{
	sb_t ret;
	sb_init(&ret);

	//size_t len;
	char appData[_MAX_PATH] = "C:";
	if(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appData) == S_OK) {
		sb_append(&ret, appData);
	}
	sb_append(&ret, "\\p4t");
	path_mkdir(sb_get(&ret));
	return ret;
}
BB_WARNING_POP;
#else
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
void appdata_get(char *buffer, size_t bufferSize)
{
	char temp[1024] = "~";
	struct passwd pwd;
	char *home = getenv("HOME");
	if(!home) {
		struct passwd *ppwd = NULL;
		getpwuid_r(getuid(), &pwd, temp, sizeof(temp), &ppwd);
		if(ppwd) {
			home = ppwd->pw_dir;
		}
	}

	bb_snprintf(buffer, bufferSize, "%s/.bb", home);
	buffer[bufferSize - 1] = '\0';
	path_mkdir(buffer);
}
#endif
