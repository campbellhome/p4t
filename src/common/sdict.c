// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "sdict.h"
#include "bb_array.h"

#include "bb_wrap_stdio.h"
#include <stdlib.h>

void sdict_init(sdict_t *sd)
{
	sd->count = sd->allocated = 0;
	sd->data = NULL;
	sd->unique = false;
}

void sdictEntry_reset(sdictEntry_t *e)
{
	sb_reset(&e->key);
	sb_reset(&e->value);
}

void sdict_reset(sdict_t *sd)
{
	for(u32 i = 0; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		sdictEntry_reset(e);
	}
	bba_free(*sd);
}

void sdict_move(sdict_t *target, sdict_t *src)
{
	sdict_reset(target);
	memcpy(target, src, sizeof(*src));
	memset(src, 0, sizeof(*src));
}

void sdict_copy(sdict_t *target, sdict_t *src)
{
	sdict_reset(target);
	for(u32 i = 0; i < src->count; ++i) {
		const sdictEntry_t *e = src->data + i;
		if(bba_add(*target, 1) != NULL) {
			sdictEntry_t *te = &bba_last(*target);
			sb_append(&te->key, sb_get(&e->key));
			sb_append(&te->value, sb_get(&e->value));
		}
	}
}

u32 sdict_find_index(sdict_t *sd, const char *key)
{
	for(u32 i = 0; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		if(!strcmp(key, sb_get(&e->key))) {
			return i;
		}
	}
	return ~0U;
}

u32 sdict_find_index_from(sdict_t *sd, const char *key, u32 startIndex)
{
	for(u32 i = startIndex; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		if(!strcmp(key, sb_get(&e->key))) {
			return i;
		}
	}
	return ~0U;
}

sdictEntry_t *sdict_find_entry(sdict_t *sd, const char *key)
{
	for(u32 i = 0; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		if(!strcmp(key, sb_get(&e->key))) {
			return e;
		}
	}
	return NULL;
}

const char *sdict_find(sdict_t *sd, const char *key)
{
	for(u32 i = 0; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		if(!strcmp(key, sb_get(&e->key))) {
			return sb_get(&e->value);
		}
	}
	return NULL;
}

const char *sdict_find_safe(sdict_t *sd, const char *key)
{
	const char *result = sdict_find(sd, key);
	if(!result) {
		result = "";
	}
	return result;
}

b32 sdict_grow(sdict_t *sd, u32 len)
{
	u32 originalCount = sd->count;
	u32 desiredCount = originalCount + len;
	bba_add_noclear(*sd, len);
	if(sd->data && sd->count == desiredCount) {
		return true;
	}
	return false;
}

b32 sdict_add(sdict_t *sd, sdictEntry_t *entry)
{
	if(sd->unique) {
		sdict_remove(sd, sb_get(&entry->key));
	}
	if(sdict_grow(sd, 1)) {
		sdictEntry_t *e = sd->data + sd->count - 1;
		*e = *entry;
		memset(entry, 0, sizeof(*entry));
		return true;
	}
	sdictEntry_reset(entry);
	return false;
}

b32 sdict_add_raw(sdict_t *sd, const char *key, const char *value)
{
	sdictEntry_t e = { 0 };
	sb_append(&e.key, key);
	sb_append(&e.value, value);
	return sdict_add(sd, &e);
}

b32 sdict_remove(sdict_t *sd, const char *key)
{
	for(u32 i = 0; i < sd->count; ++i) {
		sdictEntry_t *e = sd->data + i;
		if(e->key.data && !strcmp(key, e->key.data)) {
			u32 lastIndex = sd->count - 1;
			sdictEntry_reset(e);
			if(lastIndex != i) {
				*e = sd->data[lastIndex];
			}
			--sd->count;
			return true;
		}
	}
	return false;
}

int sdict_compare(const void *_a, const void *_b)
{
	sdictEntry_t *a = (sdictEntry_t *)_a;
	sdictEntry_t *b = (sdictEntry_t *)_b;
	return strcmp(sb_get(&a->key), sb_get(&b->key));
}

void sdict_sort(sdict_t *sd)
{
	qsort(sd->data, sd->count, sizeof(sdictEntry_t), &sdict_compare);
}

void sdicts_init(sdicts *sds)
{
	sds->count = sds->allocated = 0;
	sds->data = NULL;
}

void sdicts_reset(sdicts *sds)
{
	for(u32 i = 0; i < sds->count; ++i) {
		sdict_reset(sds->data + i);
	}
	bba_free(*sds);
}

void sdicts_move(sdicts *target, sdicts *src)
{
	sdicts_reset(target);
	memcpy(target, src, sizeof(*src));
	memset(src, 0, sizeof(*src));
}
