// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "config.h"

#include "app.h"
#include "appdata.h"
#include "file_utils.h"
#include "json_generated.h"
#include "sb.h"
#include "va.h"

#include "bb_array.h"
#include "bb_string.h"

#include "bb_wrap_stdio.h"
#include <stdlib.h>

config_t g_config;
appTypeConfig g_apptypeConfig;

void config_getwindowplacement(HWND hwnd)
{
	memset(&g_apptypeConfig.wp, 0, sizeof(g_apptypeConfig.wp));
	g_apptypeConfig.wp.length = sizeof(g_apptypeConfig.wp);
	GetWindowPlacement(hwnd, &g_apptypeConfig.wp);
}

config_t *config_clone(config_t *config)
{
	config_t *target = (config_t *)malloc(sizeof(config_t));
	if(!target)
		return NULL;

	memcpy(target, config, sizeof(*target));
	sb_init(&target->logFontConfig.path);
	sb_append(&target->logFontConfig.path, sb_get(&config->logFontConfig.path));
	sb_init(&target->uiFontConfig.path);
	sb_append(&target->uiFontConfig.path, sb_get(&config->uiFontConfig.path));
	sb_init(&target->diff.path);
	sb_append(&target->diff.path, sb_get(&config->diff.path));
	sb_init(&target->diff.args);
	sb_append(&target->diff.args, sb_get(&config->diff.args));
	sb_init(&target->p4.clientspec);
	sb_append(&target->p4.clientspec, sb_get(&config->p4.clientspec));
	sb_init(&target->colorscheme);
	sb_append(&target->colorscheme, sb_get(&config->colorscheme));
	return target;
}

void config_reset(config_t *config)
{
	sb_reset(&config->logFontConfig.path);
	sb_reset(&config->uiFontConfig.path);
	sb_reset(&config->diff.path);
	sb_reset(&config->diff.args);
	sb_reset(&config->p4.clientspec);
	sb_reset(&config->colorscheme);
}

void config_free(config_t *config)
{
	config_reset(config);
	free(config);
}

static sb_t config_get_path(void)
{
	sb_t s = appdata_get();
	sb_append(&s, "\\p4t_common_config.json");
	return s;
}

b32 config_read(config_t *config)
{
	b32 ret = false;
	sb_t path = config_get_path();
	JSON_Value *val = json_parse_file(sb_get(&path));
	if(val) {
		*config = json_deserialize_config_t(val);
		json_value_free(val);
		ret = true;
	}
	sb_reset(&path);

	if(config->version == 0) {
		config->singleInstanceCheck = true;
		config->doubleClickSeconds = 0.3f;
		config->dpiAware = true;
		config->dpiScale = 1.0f;
		config->uiFontConfig.size = 16;
		sb_append(&config->uiFontConfig.path, "C:\\Windows\\Fonts\\verdana.ttf");
		config->logFontConfig.size = 14;
		sb_append(&config->logFontConfig.path, "C:\\Windows\\Fonts\\consola.ttf");
	}
	if(config->version < 2) {
		sb_append(&config->diff.args, "%1 %2");
	}
	if(config->version < 3) {
		config->p4.changelistBlockSize = 1000;
	}
	config->version = kConfigVersion;
	if(!globals.appSpecific.allowSingleInstance) {
		config->singleInstanceCheck = false;
	}
	return ret;
}

b32 config_write(config_t *config)
{
	b32 result = false;
	JSON_Value *val = json_serialize_config_t(config);
	if(val) {
		sb_t path = config_get_path();
		FILE *fp = fopen(sb_get(&path), "wb");
		if(fp) {
			char *serialized_string = json_serialize_to_string_pretty(val);
			fputs(serialized_string, fp);
			fclose(fp);
			json_free_serialized_string(serialized_string);
			result = true;
		}
		sb_reset(&path);
	}
	json_value_free(val);
	return result;
}

static sb_t config_get_apptype_path(void)
{
	sb_t s = appdata_get();
	sb_va(&s, "\\%s_config.json", globals.appSpecific.configName);
	return s;
}

b32 config_read_apptype(appTypeConfig *config)
{
	b32 ret = false;
	sb_t path = config_get_apptype_path();
	JSON_Value *val = json_parse_file(sb_get(&path));
	if(val) {
		*config = json_deserialize_appTypeConfig(val);
		json_value_free(val);
		ret = true;
	}
	sb_reset(&path);
	config->version = kConfigAppTypeVersion;
	return ret;
}

b32 config_write_apptype(appTypeConfig *config)
{
	b32 result = false;
	JSON_Value *val = json_serialize_appTypeConfig(config);
	if(val) {
		sb_t path = config_get_apptype_path();
		FILE *fp = fopen(sb_get(&path), "wb");
		if(fp) {
			char *serialized_string = json_serialize_to_string_pretty(val);
			fputs(serialized_string, fp);
			fclose(fp);
			json_free_serialized_string(serialized_string);
			result = true;
		}
		sb_reset(&path);
	}
	json_value_free(val);
	return result;
}
