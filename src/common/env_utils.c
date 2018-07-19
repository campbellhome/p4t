// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "env_utils.h"

sb_t env_get(const char *name)
{
	DWORD required = GetEnvironmentVariable(name, NULL, 0);

	sb_t ret;
	sb_init(&ret);

	if(required) {
		if(sb_grow(&ret, required)) {
			GetEnvironmentVariable(name, ret.data, ret.count);
		}
	}

	return ret;
}

sb_t env_resolve(const char *src)
{
	DWORD required = ExpandEnvironmentStringsA(src, NULL, 0);

	sb_t ret;
	sb_init(&ret);

	if(required) {
		if(sb_grow(&ret, required)) {
			ExpandEnvironmentStringsA(src, ret.data, ret.count);
		}
	}

	return ret;
}
