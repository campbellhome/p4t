// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "sdict.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tag_messageBox messageBox;
typedef void(messageBoxFunc)(messageBox *mb, const char *action);

typedef struct tag_messageBox {
	sdict_t data;
	messageBoxFunc *callback;
} messageBox;

void mb_queue(messageBox mb);
messageBox *mb_get_active(void);
void mb_remove_active(void);
void mb_shutdown(void);

#if defined(__cplusplus)
}
#endif
