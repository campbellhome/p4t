// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "span.h"

span_t span_from_string(const char *str)
{
	span_t ret;
	ret.start = ret.end = str;
	if(str) {
		while(*ret.end) {
			++ret.end;
		}
	}
	return ret;
}
