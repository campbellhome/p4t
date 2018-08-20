// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "sdict.h"

#include "json_generated.h"

JSON_Value *json_serialize_sdictEntry_t(const sdictEntry_t *src)
{
	JSON_Value *val = json_value_init_object();
	JSON_Object *obj = json_value_get_object(val);
	if(obj) {
		json_object_set_value(obj, "key", json_serialize_sb_t(&src->key));
		json_object_set_value(obj, "value", json_serialize_sb_t(&src->value));
	}
	return val;
}

sdictEntry_t json_deserialize_sdictEntry_t(JSON_Value *src)
{
	sdictEntry_t dst;
	memset(&dst, 0, sizeof(dst));
	if(src) {
		JSON_Object *obj = json_value_get_object(src);
		if(obj) {
			dst.key = json_deserialize_sb_t(json_object_get_value(obj, "key"));
			dst.value = json_deserialize_sb_t(json_object_get_value(obj, "value"));
		}
	}
	return dst;
}

JSON_Value *json_serialize_sdict_t(const sdict_t *src)
{
	JSON_Value *val;
	if(src->unique) {
		val = json_value_init_object();
		JSON_Object *obj = json_value_get_object(val);
		if(obj) {
			for(u32 i = 0; i < src->count; ++i) {
				const sdictEntry_t *e = src->data + i;
				json_object_set_value(obj, sb_get(&e->key), json_serialize_sb_t(&e->value));
			}
		}
	} else {
		val = json_value_init_array();
		JSON_Array *arr = json_value_get_array(val);
		if(arr) {
			for(u32 i = 0; i < src->count; ++i) {
				json_array_append_value(arr, json_serialize_sdictEntry_t(src->data + i));
			}
		}
	}
	return val;
}

sdict_t json_deserialize_sdict_t(JSON_Value *src)
{
	sdict_t dst;
	sdict_init(&dst);
	JSON_Object *obj = json_value_get_object(src);
	if(obj) {
		size_t count = json_object_get_count(obj);
		for(size_t i = 0; i < count; ++i) {
			const char *key = json_object_get_name(obj, i);
			JSON_Value *val = json_object_get_value_at(obj, i);
			const char *valStr = json_value_get_string(val);
			if(key && val) {
				sdictEntry_t e;
				sb_init(&e.key);
				sb_append(&e.key, key);
				sb_init(&e.value);
				sb_append(&e.value, valStr);
				sdict_add(&dst, &e);
			}
		}
		dst.unique = true;
	} else {
		JSON_Array *arr = json_value_get_array(src);
		if(arr) {
			size_t count = json_array_get_count(arr);
			for(size_t i = 0; i < count; ++i) {
				JSON_Object *arrObj = json_array_get_object(arr, i);
				const char *key = json_object_get_string(arrObj, "key");
				const char *val = json_object_get_string(arrObj, "value");
				if(key && val) {
					sdictEntry_t e;
					sb_init(&e.key);
					sb_append(&e.key, key);
					sb_init(&e.value);
					sb_append(&e.value, val);
					sdict_add(&dst, &e);
				}
			}
		}
	}
	return dst;
}
