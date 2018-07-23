// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "sb.h"

#if defined(__cplusplus)
extern "C" {
#endif

AUTOJSON typedef struct sdictEntry_s {
	sb_t key;
	sb_t value;
} sdictEntry_t;

AUTOJSON AUTOHEADERONLY typedef struct sdict_s {
	u32 count;
	u32 allocated;
	sdictEntry_t *data;
	b32 unique;
	u32 pad;
} sdict_t;

typedef struct tag_sdicts {
	u32 count;
	u32 allocated;
	sdict_t *data;
} sdicts;

void sdict_init(sdict_t *sd);
void sdict_reset(sdict_t *sd);
void sdict_move(sdict_t *target, sdict_t *src);
u32 sdict_find_index(sdict_t *sd, const char *key);
u32 sdict_find_index_from(sdict_t *sd, const char *key, u32 startIndex);
const char *sdict_find(sdict_t *sd, const char *key);
const char *sdict_find_safe(sdict_t *sd, const char *key);
b32 sdict_grow(sdict_t *sd, u32 len);
b32 sdict_add(sdict_t *sd, sdictEntry_t *entry);
b32 sdict_remove(sdict_t *sd, const char *key);
void sdict_sort(sdict_t *sd);

#if defined(__cplusplus)
}
#endif
