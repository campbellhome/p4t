// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "tokenize.h"
#include "common.h"

#include <string.h>

span_t tokenize(const char **bufferCursor, const char *delimiters)
{
	span_t ret = { 0 };

	if(!delimiters) {
		delimiters = " \t\r\n";
	}

	const char *buffer = *bufferCursor;

	if(!buffer || !*buffer) {
		return ret;
	}

	// skip whitespace
	while(strchr(delimiters, *buffer)) {
		++buffer;
	}

	if(!*buffer) {
		return ret;
	}

	// look for '\"'
	bool isQuotedString = (*buffer == '\"');
	if(isQuotedString) {
		++buffer;
	}

	// step to end of string
	const char *start = buffer;
	if(isQuotedString) {
		while(*buffer && *buffer != '\"' && *buffer != '\n') {
			++buffer;
		}
	} else {
		while(*buffer && !strchr(delimiters, *buffer)) {
			++buffer;
		}
	}

	// remove trailing '\"'
	const char *end = buffer;
	if(isQuotedString && *end == '\"') {
		buffer++;
	}

	ret.start = start;
	ret.end = end;

	*bufferCursor = buffer;

	return ret;
}
