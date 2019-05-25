#define _HAS_EXCEPTIONS 0
#define _ITERATOR_DEBUG_LEVEL 0

// Copyright (c) 2012-2019 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "file_utils.h"
#include "path_utils.h"
#include "sdict.h"

#define LEXER_STATIC
#include "thirdparty/mm_lexer/mm_lexer.h"

#include <set>
#include <string>
#include <vector>

struct enum_member_s {
	std::string name;
};
struct enum_s {
	std::string name;
	std::string typedefBaseName;
	std::string defaultVal;
	std::vector< enum_member_s > members;
};
extern std::vector< enum_s > g_enums;

struct struct_member_s {
	std::string name;
	std::string val;
	std::string arr;
	std::string typeStr;
	std::vector< lexer_token > typeTokens;
	bool parseempty = false;
};
struct struct_s {
	std::string name;
	std::string typedefBaseName;
	bool autovalidate;
	bool headerOnly;
	bool fromLoc;
	std::vector< struct_member_s > members;
};
extern std::vector< struct_s > g_structs;

extern std::set< std::string > g_paths;

void GenerateJson(sb_t *srcDir);
void GenerateReset(sb_t *srcDir);

void find_files_in_dir(const char *dir, const char *desiredExt, sdict_t *sd);
