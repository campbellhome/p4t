// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "span.h"

#if defined(__cplusplus)
extern "C" {
#endif

span_t tokenize(const char **bufferCursor, const char *delimiters);

#if defined(__cplusplus)
}
#endif
