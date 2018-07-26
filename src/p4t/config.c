// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "config.h"

#include "appdata.h"
#include "file_utils.h"
#include "json_generated.h"
#include "sb.h"
#include "va.h"

#include "bb_array.h"
#include "bb_string.h"

#include "bb_wrap_stdio.h"
#include <stdlib.h>

static sb_t config_get_path(void)
{
	sb_t s = appdata_get();
	sb_append(&s, "\\p4t_preferences.json");
	return s;
}

//////////////////////////////////////////////////////////////////////////

void config_getwindowplacement(HWND hwnd)
{
	memset(&g_config.wp, 0, sizeof(g_config.wp));
	g_config.wp.length = sizeof(g_config.wp);
	GetWindowPlacement(hwnd, &g_config.wp);
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
	return target;
}

void config_reset(config_t *config)
{
	sb_reset(&config->logFontConfig.path);
	sb_reset(&config->uiFontConfig.path);
	sb_reset(&config->diff.path);
	sb_reset(&config->diff.args);
}

void config_free(config_t *config)
{
	config_reset(config);
	free(config);
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
		config->autoTileViews = true;
		config->singleInstanceCheck = true;
		config->alternateRowBackground = true;
		config->recordingsOpen = true;
		config->doubleClickSeconds = 0.3f;
		config->autoDeleteAfterDays = 14;
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
	config->version = kConfigVersion;
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

//////////////////////////////////////////////////////////////////////////

config_t g_config;

#if 0
static b32 config_get_path_new(char *buffer, size_t bufferSize)
{
	size_t dirLen;
	get_appdata_folder(buffer, bufferSize);
	dirLen = strlen(buffer);

	if(bb_snprintf(buffer + dirLen, bufferSize - dirLen, "\\bb_preferences.bbconfig") < 0)
		return false;

	return true;
}

static bool config_get_path(char *buffer, size_t bufferSize)
{
	size_t dirLen;
	get_appdata_folder(buffer, bufferSize);
	dirLen = strlen(buffer);

	if(bb_snprintf(buffer + dirLen, bufferSize - dirLen, "/bb.bbconfig") < 0) {
		buffer[bufferSize - 1] = '\0';
	}

	return true;
}

char *escape_string(const char *src)
{
	sb_t sb;
	char *ret;
	sb_init(&sb);
	while(*src) {
		if(*src == '\\') {
			sb_append(&sb, "\\\\");
		} else if(*src == '\r') {
			sb_append(&sb, "\\r");
		} else if(*src == '\n') {
			sb_append(&sb, "\\n");
		} else if(*src == '\t') {
			sb_append(&sb, "\\t");
		} else if(*src == '\"') {
			sb_append(&sb, "\\'");
		} else {
			sb_append_char(&sb, *src);
		}
		++src;
	}
	ret = va("%s", sb_get(&sb));
	sb_reset(&sb);
	return ret;
}

char *unescape_string(const char *src)
{
	sb_t sb;
	char *ret;
	sb_init(&sb);
	while(*src) {
		if(*src == '\\') {
			++src;
			switch(*src) {
			case '\\':
				sb_append_char(&sb, '\\');
				break;
			case 'r':
				sb_append_char(&sb, '\r');
				break;
			case 'n':
				sb_append_char(&sb, '\n');
				break;
			case 't':
				sb_append_char(&sb, '\t');
				break;
			case '\'':
				sb_append_char(&sb, '\"');
				break;
			}
		} else {
			sb_append_char(&sb, *src);
		}
		++src;
	}
	ret = va("%s", sb_get(&sb));
	sb_reset(&sb);
	return ret;
}

void whitelist_move_entry(configWhitelist_t *whitelist, u32 indexA, u32 indexB)
{
	if(indexA < whitelist->count &&
	   indexB < whitelist->count &&
	   indexA != indexB) {
		configWhitelistEntry_t *entryA = whitelist->data + indexA;
		configWhitelistEntry_t *entryB = whitelist->data + indexB;
		configWhitelistEntry_t tmp = *entryA;
		*entryA = *entryB;
		*entryB = tmp;
	}
}

void config_validate_whitelist(configWhitelist_t *whitelist)
{
	if(!whitelist->count) {
		configWhitelistEntry_t *entry = bba_add(*whitelist, 1);
		if(entry) {
			entry->allow = true;
			bb_strncpy(entry->addressPlusMask, "localhost", sizeof(entry->addressPlusMask));
		}
	}
}

void open_target_move_entry(openTargetList_t *openTargets, u32 indexA, u32 indexB)
{
	if(indexA < openTargets->count &&
	   indexB < openTargets->count &&
	   indexA != indexB) {
		openTargetEntry_t *entryA = openTargets->data + indexA;
		openTargetEntry_t *entryB = openTargets->data + indexB;
		openTargetEntry_t tmp = *entryA;
		*entryA = *entryB;
		*entryB = tmp;
	}
}

void config_validate_open_targets(openTargetList_t *openTargets)
{
	if(!openTargets->count) {
		openTargetEntry_t *entry = bba_add(*openTargets, 1);
		if(entry) {
			bb_strncpy(entry->displayName, "Open in Visual Studio 2012", sizeof(entry->displayName));
			bb_strncpy(entry->commandLine, "\"C:\\Program Files (x86)\\Microsoft Visual Studio 11.0\\Common7\\IDE\\devenv.exe\" /edit \"{File}\" /command \"edit.goto {Line}\"", sizeof(entry->commandLine));
		}
		//entry = bba_add(*openTargets, 1);
		//if(entry) {
		//	bb_strncpy(entry->displayName, "Open in Visual Studio 2015", sizeof(entry->displayName));
		//	bb_strncpy(entry->commandLine, "\"C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\Common7\\IDE\\devenv.exe\" /edit \"{File}\" /command \"edit.goto {Line}\"", sizeof(entry->commandLine));
		//}
		//entry = bba_add(*openTargets, 1);
		//if(entry) {
		//	bb_strncpy(entry->displayName, "Open in Visual Studio 2017 Community", sizeof(entry->displayName));
		//	bb_strncpy(entry->commandLine, "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\Common7\\IDE\\devenv.exe\" /edit \"{File}\" /command \"edit.goto {Line}\"", sizeof(entry->commandLine));
		//}
	}
}

void path_fixup_move_entry(pathFixupList_t *pathFixups, u32 indexA, u32 indexB)
{
	if(indexA < pathFixups->count &&
	   indexB < pathFixups->count &&
	   indexA != indexB) {
		pathFixupEntry_t *entryA = pathFixups->data + indexA;
		pathFixupEntry_t *entryB = pathFixups->data + indexB;
		pathFixupEntry_t tmp = *entryA;
		*entryA = *entryB;
		*entryB = tmp;
	}
}

config_t *config_clone(config_t *config)
{
	config_t *target = (config_t *)malloc(sizeof(config_t));
	if(!target)
		return NULL;

	memcpy(target, config, sizeof(*target));
	memset(&target->whitelist, 0, sizeof(target->whitelist));
	bba_add(target->whitelist, config->whitelist.count);
	if(target->whitelist.count) {
		memcpy(target->whitelist.data, config->whitelist.data, target->whitelist.count * sizeof(configWhitelistEntry_t));
	}
	memset(&target->openTargets, 0, sizeof(target->openTargets));
	bba_add(target->openTargets, config->openTargets.count);
	if(target->openTargets.count) {
		memcpy(target->openTargets.data, config->openTargets.data, target->openTargets.count * sizeof(openTargetEntry_t));
	}
	memset(&target->pathFixups, 0, sizeof(target->pathFixups));
	bba_add(target->pathFixups, config->pathFixups.count);
	if(target->pathFixups.count) {
		memcpy(target->pathFixups.data, config->pathFixups.data, target->pathFixups.count * sizeof(pathFixupEntry_t));
	}
	return target;
}

void config_free(config_t *config)
{
	bba_free(config->whitelist);
	bba_free(config->openTargets);
	bba_free(config->pathFixups);
	free(config);
}

static void parse_whitelist_entry(line_parser_t *parser, config_t *config)
{
	char *token;
	configWhitelistEntry_t entry;
	memset(&entry, 0, sizeof(entry));
	entry.allow = true;

	while((token = line_parser_next_token(parser)) != NULL) {
		if(!strcmp(token, "address")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.addressPlusMask, unescape_string(valueStr), sizeof(entry.addressPlusMask));
		} else if(!strcmp(token, "application")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.applicationName, unescape_string(valueStr), sizeof(entry.applicationName));
		} else if(!strcmp(token, "allow")) {
			entry.allow = true;
		} else if(!strcmp(token, "deny")) {
			entry.allow = false;
		} else {
			line_parser_error(parser, va("unknown keyword '%s'", token));
			break;
		}
	}

	bba_push(config->whitelist, entry);
}

static void parse_open_target_entry(line_parser_t *parser, config_t *config)
{
	char *token;
	openTargetEntry_t entry;
	memset(&entry, 0, sizeof(entry));

	while((token = line_parser_next_token(parser)) != NULL) {
		if(!strcmp(token, "name")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.displayName, unescape_string(valueStr), sizeof(entry.displayName));
		} else if(!strcmp(token, "command")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.commandLine, unescape_string(valueStr), sizeof(entry.commandLine));
		} else {
			line_parser_error(parser, va("unknown keyword '%s'", token));
			break;
		}
	}

	bba_push(config->openTargets, entry);
}

static void parse_path_fixup_entry(line_parser_t *parser, config_t *config)
{
	char *token;
	pathFixupEntry_t entry;
	memset(&entry, 0, sizeof(entry));

	while((token = line_parser_next_token(parser)) != NULL) {
		if(!strcmp(token, "src")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.src, unescape_string(valueStr), sizeof(entry.src));
		} else if(!strcmp(token, "dst")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(entry.dst, unescape_string(valueStr), sizeof(entry.dst));
		} else {
			line_parser_error(parser, va("unknown keyword '%s'", token));
			break;
		}
	}

	bba_push(config->pathFixups, entry);
}

static void parse_windowplacement(line_parser_t *parser, config_t *config)
{
	char *token;
	memset(&config->wp, 0, sizeof(config->wp));
	config->wp.length = sizeof(config->wp);

	while((token = line_parser_next_token(parser)) != NULL) {
		if(!strcmp(token, "flags")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			config->wp.flags = strtoul(valueStr, NULL, 16);
		} else if(!strcmp(token, "show")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			config->wp.showCmd = strtoul(valueStr, NULL, 10);
		} else if(!strcmp(token, "min")) {
			char *xStr = line_parser_next_token(parser);
			char *yStr = line_parser_next_token(parser);
			if(!xStr || !yStr) {
				line_parser_error(parser, va("missing x and y value after '%s'", token));
				break;
			}
			config->wp.ptMinPosition.x = atoi(xStr);
			config->wp.ptMinPosition.y = atoi(yStr);
		} else if(!strcmp(token, "max")) {
			char *xStr = line_parser_next_token(parser);
			char *yStr = line_parser_next_token(parser);
			if(!xStr || !yStr) {
				line_parser_error(parser, va("missing x and y value after '%s'", token));
				break;
			}
			config->wp.ptMaxPosition.x = atoi(xStr);
			config->wp.ptMaxPosition.y = atoi(yStr);
		} else if(!strcmp(token, "rc")) {
			char *leftStr = line_parser_next_token(parser);
			char *topStr = line_parser_next_token(parser);
			char *rightStr = line_parser_next_token(parser);
			char *bottomStr = line_parser_next_token(parser);
			if(!leftStr || !topStr || !rightStr || !bottomStr) {
				line_parser_error(parser, va("missing values after '%s'", token));
				break;
			}
			config->wp.rcNormalPosition.left = atoi(leftStr);
			config->wp.rcNormalPosition.top = atoi(topStr);
			config->wp.rcNormalPosition.right = atoi(rightStr);
			config->wp.rcNormalPosition.bottom = atoi(bottomStr);
		} else {
			line_parser_error(parser, va("unknown keyword '%s'", token));
			break;
		}
	}

	if(config->wp.showCmd == SW_SHOWMINIMIZED) {
		config->wp.showCmd = SW_SHOWNORMAL;
	}
}

static void parse_fontconfig(line_parser_t *parser, config_t *config, fontConfig_t *fontConfig)
{
	char *token;
	memset(fontConfig, 0, sizeof(*fontConfig));

	while((token = line_parser_next_token(parser)) != NULL) {
		if(!strcmp(token, "enabled")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			fontConfig->enabled = atoi(valueStr);
		} else if(!strcmp(token, "dpiAware")) {
			// dpiAware no longer lives in fontConfig, but this allows backcompat
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			config->dpiAware = atoi(valueStr);
		} else if(!strcmp(token, "size")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			fontConfig->size = atoi(valueStr);
		} else if(!strcmp(token, "path")) {
			char *valueStr = line_parser_next_token(parser);
			if(!valueStr) {
				line_parser_error(parser, va("missing value after '%s'", token));
				break;
			}
			bb_strncpy(fontConfig->path, valueStr, sizeof(fontConfig->path));
		} else {
			line_parser_error(parser, va("unknown keyword '%s'", token));
			break;
		}
	}
}

static b32 preferences_read_config_lines(line_parser_t *parser, config_t *config)
{
	char *line;
	char *token;
	b32 ret = true;
	while((line = line_parser_next_line(parser)) != NULL) {
		//BB_LOG("Parser", "Line %u: [%s]\n", parser->lineIndex, line);
		token = line_parser_next_token(parser);
		if(!strcmp(token, "autoTileViews")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->autoTileViews = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for autoTileViews: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for autoTileViews");
			}
		} else if(!strcmp(token, "alternateRowBackground")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->alternateRowBackground = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for alternateRowBackground: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for alternateRowBackground");
			}
		} else if(!strcmp(token, "recordingsOpen")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->recordingsOpen = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for recordingsOpen: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for recordingsOpen");
			}
		} else if(!strcmp(token, "doubleClickSeconds")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->doubleClickSeconds = (float)atof(token);
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for doubleClickSeconds: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for doubleClickSeconds");
			}
		} else if(!strcmp(token, "autoDeleteAfterDays")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->autoDeleteAfterDays = (u32)atoi(token);
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for autoDeleteAfterDays: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for autoDeleteAfterDays");
			}
		} else if(!strcmp(token, "singleInstanceCheck")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->singleInstanceCheck = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for singleInstanceCheck: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for singleInstanceCheck");
			}
		} else if(!strcmp(token, "singleInstancePrompt")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->singleInstancePrompt = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for singleInstancePrompt: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for singleInstancePrompt");
			}
		} else if(!strcmp(token, "dpiAware")) {
			token = line_parser_next_token(parser);
			if(token) {
				config->dpiAware = atoi(token) == 1;
				if(line_parser_next_token(parser)) {
					ret = line_parser_error(parser, va("garbage after value for dpiAware: '%s'", token));
				}
			} else {
				ret = line_parser_error(parser, "missing value for dpiAware");
			}
		} else if(!strcmp(token, "whitelist")) {
			parse_whitelist_entry(parser, config);
		} else if(!strcmp(token, "openTarget")) {
			parse_open_target_entry(parser, config);
		} else if(!strcmp(token, "pathFixup")) {
			parse_path_fixup_entry(parser, config);
		} else if(!strcmp(token, "window")) {
			parse_windowplacement(parser, config);
		} else if(!strcmp(token, "uiFontConfig")) {
			parse_fontconfig(parser, config, &config->uiFontConfig);
		} else if(!strcmp(token, "logFontConfig")) {
			parse_fontconfig(parser, config, &config->logFontConfig);
		} else {
			ret = line_parser_error(parser, va("unknown token '%s'", token));
		}
	}
	return ret;
}

b32 config_read(config_t *config)
{
	char path[kBBSize_MaxPath];
	fileData_t fileData;
	line_parser_t parser;
	u32 version;
	b32 result;

	json_config_t jsonConfig = { 0 };
	config_read_json(&jsonConfig);
	sdict_reset(&jsonConfig.kv);

	if(!config_get_path_new(path, sizeof(path)))
		return false;

	fileData = fileData_read(path);
	if(!fileData.buffer)
		return false;

	line_parser_init(&parser, path, (char *)fileData.buffer);
	version = line_parser_read_version(&parser);
	if(!version)
		return false;
	if(version > 1) {
		return line_parser_error(&parser, va("expected version 1, saw [%u]", version));
	}
	result = preferences_read_config_lines(&parser, config);
	fileData_reset(&fileData);

	// #temp #hack: remove this after clients have time to fix entries that had allow set to default accidentally
	if(result && config->whitelist.count == 1 &&
	   !strcmp(config->whitelist.data[0].addressPlusMask, "localhost") &&
	   !strcmp(config->whitelist.data[0].applicationName, "")) {
		config->whitelist.data[0].allow = true;
	}
	return result;
}

void config_getwindowplacement(HWND hwnd)
{
	memset(&g_config.wp, 0, sizeof(g_config.wp));
	g_config.wp.length = sizeof(g_config.wp);
	GetWindowPlacement(hwnd, &g_config.wp);
}

b32 config_write(config_t *config)
{
	json_config_t jc = { 0 };
	jc.autoTileViews = config->autoTileViews;
	jc.alternateRowBackground = config->alternateRowBackground;
	jc.recordingsOpen = config->recordingsOpen;
	jc.singleInstanceCheck = config->singleInstanceCheck;
	jc.singleInstancePrompt = config->singleInstancePrompt;
	jc.dpiAware = config->dpiAware;
	jc.autoDeleteAfterDays = config->autoDeleteAfterDays;
	jc.doubleClickSeconds = config->doubleClickSeconds;
	jc.dpiScale = config->dpiScale;

	sdict_init(&jc.kv);
	jc.kv.unique = true;

	sdictEntry_t e;
	sb_init(&e.key);
	sb_init(&e.value);

	sb_append(&e.key, "this is a key");
	sb_append(&e.value, "this is a value");
	sdict_add(&jc.kv, &e);

	sb_append(&e.key, "this is a key");
	sb_append(&e.value, "this is another value");
	sdict_add(&jc.kv, &e);

	b32 result = config_write_json(&jc);
	sdict_reset(&jc.kv);
	return result;
#if 0
	char path[kBBSize_MaxPath];
	FILE *fp;
	u32 i;
	if(!config_get_path_new(path, sizeof(path)))
		return false;

	fp = fopen(path, "wb");
	if(!fp)
		return false;

	fprintf(fp, "[1] # Version - do not remove\n");
	fprintf(fp, "\n");
	fprintf(fp, "autoTileViews %d\n", config->autoTileViews);
	fprintf(fp, "alternateRowBackground %d\n", config->alternateRowBackground);
	fprintf(fp, "recordingsOpen %d\n", config->recordingsOpen);
	fprintf(fp, "doubleClickSeconds %f\n", config->doubleClickSeconds);
	fprintf(fp, "autoDeleteAfterDays %u\n", config->autoDeleteAfterDays);
	fprintf(fp, "singleInstanceCheck %d\n", config->singleInstanceCheck);
	fprintf(fp, "singleInstancePrompt %d\n", config->singleInstancePrompt);
	fprintf(fp, "dpiAware %d\n", config->dpiAware);
	fprintf(fp, "\n");
	for(i = 0; i < config->whitelist.count; ++i) {
		configWhitelistEntry_t *entry = config->whitelist.data + i;
		fprintf(fp, "whitelist %s address \"%s\" application \"%s\"\n",
		        (entry->allow) ? "allow" : "deny",
		        escape_string(entry->addressPlusMask),
		        escape_string(entry->applicationName));
	}
	fprintf(fp, "\n");
	for(i = 0; i < config->openTargets.count; ++i) {
		openTargetEntry_t *entry = config->openTargets.data + i;
		fprintf(fp, "openTarget name \"%s\" command \"%s\"\n",
		        escape_string(entry->displayName),
		        escape_string(entry->commandLine));
	}
	fprintf(fp, "\n");
	for(i = 0; i < config->pathFixups.count; ++i) {
		pathFixupEntry_t *entry = config->pathFixups.data + i;
		fprintf(fp, "pathFixup src \"%s\" dst \"%s\"\n",
		        escape_string(entry->src),
		        escape_string(entry->dst));
	}
	fprintf(fp, "window flags %x show %u min %ld %ld max %ld %ld rc %ld %ld %ld %ld\n",
	        config->wp.flags, config->wp.showCmd,
	        config->wp.ptMinPosition.x, config->wp.ptMinPosition.y,
	        config->wp.ptMaxPosition.x, config->wp.ptMaxPosition.y,
	        config->wp.rcNormalPosition.left, config->wp.rcNormalPosition.top, config->wp.rcNormalPosition.right, config->wp.rcNormalPosition.bottom);
	fprintf(fp, "\n");
	fprintf(fp, "uiFontConfig enabled %d size %u path \"%s\"\n",
	        config->uiFontConfig.enabled, config->uiFontConfig.size, config->uiFontConfig.path);
	fprintf(fp, "logFontConfig enabled %d size %u path \"%s\"\n",
	        config->logFontConfig.enabled, config->logFontConfig.size, config->logFontConfig.path);
	fprintf(fp, "\n");
	fclose(fp);
	return true;
#endif
}
#endif
