// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#pragma once

#include "thirdparty/parson/parson.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tagPOINT POINT;
typedef struct tagRECT RECT;
typedef struct tagWINDOWPLACEMENT WINDOWPLACEMENT;
typedef struct fontConfig_s fontConfig_t;
typedef struct tag_uiChangelistConfig uiChangelistConfig;
typedef struct tag_uiChangesetConfig uiChangesetConfig;
typedef struct diffConfig_s diffConfig_t;
typedef struct tag_appTypeConfig appTypeConfig;
typedef struct tag_p4Config p4Config;
typedef struct config_s config_t;
typedef struct sb_s sb_t;
typedef struct sdictEntry_s sdictEntry_t;
typedef struct sdict_s sdict_t;

POINT json_deserialize_POINT(JSON_Value *src);
RECT json_deserialize_RECT(JSON_Value *src);
WINDOWPLACEMENT json_deserialize_WINDOWPLACEMENT(JSON_Value *src);
fontConfig_t json_deserialize_fontConfig_t(JSON_Value *src);
uiChangelistConfig json_deserialize_uiChangelistConfig(JSON_Value *src);
uiChangesetConfig json_deserialize_uiChangesetConfig(JSON_Value *src);
diffConfig_t json_deserialize_diffConfig_t(JSON_Value *src);
appTypeConfig json_deserialize_appTypeConfig(JSON_Value *src);
p4Config json_deserialize_p4Config(JSON_Value *src);
config_t json_deserialize_config_t(JSON_Value *src);
sb_t json_deserialize_sb_t(JSON_Value *src);
sdictEntry_t json_deserialize_sdictEntry_t(JSON_Value *src);
sdict_t json_deserialize_sdict_t(JSON_Value *src);

JSON_Value *json_serialize_POINT(const POINT *src);
JSON_Value *json_serialize_RECT(const RECT *src);
JSON_Value *json_serialize_WINDOWPLACEMENT(const WINDOWPLACEMENT *src);
JSON_Value *json_serialize_fontConfig_t(const fontConfig_t *src);
JSON_Value *json_serialize_uiChangelistConfig(const uiChangelistConfig *src);
JSON_Value *json_serialize_uiChangesetConfig(const uiChangesetConfig *src);
JSON_Value *json_serialize_diffConfig_t(const diffConfig_t *src);
JSON_Value *json_serialize_appTypeConfig(const appTypeConfig *src);
JSON_Value *json_serialize_p4Config(const p4Config *src);
JSON_Value *json_serialize_config_t(const config_t *src);
JSON_Value *json_serialize_sb_t(const sb_t *src);
JSON_Value *json_serialize_sdictEntry_t(const sdictEntry_t *src);
JSON_Value *json_serialize_sdict_t(const sdict_t *src);

#if defined(__cplusplus)
} // extern "C"
#endif
