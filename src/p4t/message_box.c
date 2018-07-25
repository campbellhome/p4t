// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "message_box.h"

#include "bb_array.h"

typedef struct tag_messageBoxes {
	u32 count;
	u32 allocated;
	messageBox *data;
} messageBoxes;
static messageBoxes s_mb;

void mb_reset(messageBox *mb)
{
	sdict_reset(&mb->data);
}

void mb_queue(messageBox mb)
{
	if(bba_add_noclear(s_mb, 1)) {
		bba_last(s_mb) = mb;
	} else {
		mb_reset(&mb);
	}
}

messageBox *mb_get_active(void)
{
	if(s_mb.count) {
		return s_mb.data;
	} else {
		return NULL;
	}
}

void mb_remove_active(void)
{
	if(s_mb.count) {
		mb_reset(s_mb.data);
		bba_erase(s_mb, 0);
	}
}

void mb_shutdown(void)
{
	for(u32 i = 0; i < s_mb.count; ++i) {
		mb_reset(s_mb.data + i);
	}
	bba_free(s_mb);
}
