// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

// AUTOGENERATED FILE - DO NOT EDIT

// clang-format off

#pragma once

#include "parson/parson.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct sb_s;
struct sbs_s;
struct sdictEntry_s;
struct sdict_s;
struct uuid_node_s;
struct fontConfig_s;
struct fontConfigs_s;
struct tagPOINT;
struct tagRECT;
struct tagWINDOWPLACEMENT;
struct tag_uiChangelistConfig;
struct tag_uiChangesetConfig;
struct diffConfig_s;
struct tag_appTypeConfig;
struct tag_p4Config;
struct tag_changelistConfig;
struct tag_changesetConfig;
struct tag_tabConfig;
struct tag_tabsConfig;
struct tag_updatesConfig;
struct config_s;
struct updateConfig_s;
struct site_config_s;

typedef struct sb_s sb_t;
typedef struct sbs_s sbs_t;
typedef struct sdictEntry_s sdictEntry_t;
typedef struct sdict_s sdict_t;
typedef struct uuid_node_s uuid_node_t;
typedef struct fontConfig_s fontConfig_t;
typedef struct fontConfigs_s fontConfigs_t;
typedef struct tagPOINT POINT;
typedef struct tagRECT RECT;
typedef struct tagWINDOWPLACEMENT WINDOWPLACEMENT;
typedef struct tag_uiChangelistConfig uiChangelistConfig;
typedef struct tag_uiChangesetConfig uiChangesetConfig;
typedef struct diffConfig_s diffConfig_t;
typedef struct tag_appTypeConfig appTypeConfig;
typedef struct tag_p4Config p4Config;
typedef struct tag_changelistConfig changelistConfig;
typedef struct tag_changesetConfig changesetConfig;
typedef struct tag_tabConfig tabConfig;
typedef struct tag_tabsConfig tabsConfig;
typedef struct tag_updatesConfig updatesConfig;
typedef struct config_s config_t;
typedef struct updateConfig_s updateConfig_t;
typedef struct site_config_s site_config_t;


sb_t json_deserialize_sb_t(JSON_Value *src);
sbs_t json_deserialize_sbs_t(JSON_Value *src);
sdictEntry_t json_deserialize_sdictEntry_t(JSON_Value *src);
sdict_t json_deserialize_sdict_t(JSON_Value *src);
uuid_node_t json_deserialize_uuid_node_t(JSON_Value *src);
fontConfig_t json_deserialize_fontConfig_t(JSON_Value *src);
fontConfigs_t json_deserialize_fontConfigs_t(JSON_Value *src);
POINT json_deserialize_POINT(JSON_Value *src);
RECT json_deserialize_RECT(JSON_Value *src);
WINDOWPLACEMENT json_deserialize_WINDOWPLACEMENT(JSON_Value *src);
uiChangelistConfig json_deserialize_uiChangelistConfig(JSON_Value *src);
uiChangesetConfig json_deserialize_uiChangesetConfig(JSON_Value *src);
diffConfig_t json_deserialize_diffConfig_t(JSON_Value *src);
appTypeConfig json_deserialize_appTypeConfig(JSON_Value *src);
p4Config json_deserialize_p4Config(JSON_Value *src);
changelistConfig json_deserialize_changelistConfig(JSON_Value *src);
changesetConfig json_deserialize_changesetConfig(JSON_Value *src);
tabConfig json_deserialize_tabConfig(JSON_Value *src);
tabsConfig json_deserialize_tabsConfig(JSON_Value *src);
updatesConfig json_deserialize_updatesConfig(JSON_Value *src);
config_t json_deserialize_config_t(JSON_Value *src);
updateConfig_t json_deserialize_updateConfig_t(JSON_Value *src);
site_config_t json_deserialize_site_config_t(JSON_Value *src);

JSON_Value *json_serialize_sb_t(const sb_t *src);
JSON_Value *json_serialize_sbs_t(const sbs_t *src);
JSON_Value *json_serialize_sdictEntry_t(const sdictEntry_t *src);
JSON_Value *json_serialize_sdict_t(const sdict_t *src);
JSON_Value *json_serialize_uuid_node_t(const uuid_node_t *src);
JSON_Value *json_serialize_fontConfig_t(const fontConfig_t *src);
JSON_Value *json_serialize_fontConfigs_t(const fontConfigs_t *src);
JSON_Value *json_serialize_POINT(const POINT *src);
JSON_Value *json_serialize_RECT(const RECT *src);
JSON_Value *json_serialize_WINDOWPLACEMENT(const WINDOWPLACEMENT *src);
JSON_Value *json_serialize_uiChangelistConfig(const uiChangelistConfig *src);
JSON_Value *json_serialize_uiChangesetConfig(const uiChangesetConfig *src);
JSON_Value *json_serialize_diffConfig_t(const diffConfig_t *src);
JSON_Value *json_serialize_appTypeConfig(const appTypeConfig *src);
JSON_Value *json_serialize_p4Config(const p4Config *src);
JSON_Value *json_serialize_changelistConfig(const changelistConfig *src);
JSON_Value *json_serialize_changesetConfig(const changesetConfig *src);
JSON_Value *json_serialize_tabConfig(const tabConfig *src);
JSON_Value *json_serialize_tabsConfig(const tabsConfig *src);
JSON_Value *json_serialize_updatesConfig(const updatesConfig *src);
JSON_Value *json_serialize_config_t(const config_t *src);
JSON_Value *json_serialize_updateConfig_t(const updateConfig_t *src);
JSON_Value *json_serialize_site_config_t(const site_config_t *src);



#if defined(__cplusplus)
} // extern "C"
#endif
