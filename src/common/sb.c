// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "sb.h"
#include "bb_array.h"

#include "bb_wrap_stdio.h"

void sb_init(sb_t *sb)
{
	sb->count = sb->allocated = 0;
	sb->data = NULL;
}

void sb_reset(sb_t *sb)
{
	bba_free(*sb);
}

u32 sb_len(sb_t *sb)
{
	return sb->count ? sb->count - 1 : 0;
}

b32 sb_reserve(sb_t *sb, u32 len)
{
	if(sb->allocated < len) {
		sb_t tmp = { 0 };
		if(bba_add_noclear(tmp, len)) {
			if(sb->count) {
				memcpy(tmp.data, sb->data, sb->count + 1);
			} else {
				*tmp.data = '\0';
			}
			tmp.count = sb->count;
			sb_reset(sb);
			*sb = tmp;
			return true;
		}
		return false;
	}
	return true;
}

b32 sb_grow(sb_t *sb, u32 len)
{
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear(*sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		sb->data[sb->count - 1] = '\0';
		return true;
	}
	return false;
}

void sb_move(sb_t *target, sb_t *src)
{
	sb_reset(target);
	*target = *src;
	memset(src, 0, sizeof(*src));
}

void sb_append(sb_t *sb, const char *text)
{
	u32 len = (u32)strlen(text);
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear(*sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		memcpy(sb->data + (originalCount ? originalCount - 1 : 0), text, len + 1);
	}
}

void sb_append_range(sb_t *sb, const char *start, const char *end)
{
	u32 len = (u32)(end - start);
	u32 originalCount = sb->count;
	u32 addedCount = (originalCount) ? len : len + 1;
	u32 desiredCount = originalCount + addedCount;
	bba_add_noclear(*sb, addedCount);
	if(sb->data && sb->count == desiredCount) {
		memcpy(sb->data + (originalCount ? originalCount - 1 : 0), start, len);
		sb->data[sb->count - 1] = '\0';
	}
}

void sb_append_char(sb_t *sb, char c)
{
	char text[2];
	text[0] = c;
	text[1] = '\0';
	sb_append(sb, text);
}

const char *sb_get(const sb_t *sb)
{
	if(sb && sb->data) {
		return sb->data;
	}
	return "";
}

void sb_va(sb_t *sb, const char *fmt, ...)
{
	u32 offset = sb_len(sb);
	int len;
	va_list args;
	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args);
	if(len > 0) {
		if(sb_grow(sb, len)) {
			vsnprintf(sb->data + offset, len + 1, fmt, args);
		}
	}
	va_end(args);
}

void sb_va_list(sb_t *sb, const char *fmt, va_list args)
{
	u32 offset = sb_len(sb);
	int len;
	len = vsnprintf(NULL, 0, fmt, args);
	if(len > 0) {
		if(sb_grow(sb, len)) {
			vsnprintf(sb->data + offset, len + 1, fmt, args);
		}
	}
}

void sbs_reset(sbs_t *sbs)
{
	for(u32 i = 0; i < sbs->count; ++i) {
		sb_reset(sbs->data + i);
	}
	bba_free(*sbs);
}
