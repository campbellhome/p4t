// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

AUTOJSON AUTOHEADERONLY typedef struct sb_s {
	u32 count;
	u32 allocated;
	char *data;
} sb_t; // string builder

typedef struct sbs_s {
	u32 count;
	u32 allocated;
	sb_t *data;
} sbs_t;

void sb_init(sb_t *sb);
void sb_reset(sb_t *sb);
u32 sb_len(sb_t *sb);
b32 sb_reserve(sb_t *sb, u32 len);
b32 sb_grow(sb_t *sb, u32 len);
void sb_append(sb_t *sb, const char *text);
void sb_append_range(sb_t *sb, const char *start, const char *end);
void sb_append_char(sb_t *sb, char c);
void sb_va(sb_t *sb, const char *fmt, ...);
void sb_va_list(sb_t *sb, const char *fmt, va_list args);
const char *sb_get(const sb_t *sb);

void sbs_reset(sbs_t *sbs);

#if defined(__cplusplus)
}
#endif
