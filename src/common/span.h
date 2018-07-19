// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct span_s {
	const char *start;
	const char *end;
} span_t;

span_t span_from_string(const char *str);

#if defined(__cplusplus)
}
#endif
