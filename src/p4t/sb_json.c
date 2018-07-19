// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "sb.h"

#include "json_generated.h"

JSON_Value *json_serialize_sb_t(const sb_t *src)
{
	JSON_Value *val = json_value_init_string(sb_get(src));
	return val;
}

sb_t json_deserialize_sb_t(JSON_Value *src)
{
	sb_t dst;
	sb_init(&dst);
	const char *str = json_value_get_string(src);
	if(str) {
		sb_append(&dst, str);
	}
	return dst;
}
