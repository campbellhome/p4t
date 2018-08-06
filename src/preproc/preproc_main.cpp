#include "preproc.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEXER_IMPLEMENTATION
#define LEXER_STATIC
#include "thirdparty/mm_lexer/mm_lexer.h"

#include "thirdparty/parson/parson.h"

#include <windows.h>

#ifdef _DEBUG
#define LEAK_CHECK
#endif // #ifdef _DEBUG

#ifdef LEAK_CHECK
#include <crtdbg.h>
#endif // #ifdef LEAK_CHECK

std::vector< enum_s > g_enums;
std::vector< struct_s > g_structs;
std::set< std::string > g_paths;

std::string lexer_token_string(const lexer_token &tok)
{
	char buffer[1024];
	buffer[0] = 0;
	if(tok.type == LEXER_TOKEN_STRING) {
		buffer[0] = '\"';
		lexer_size count = lexer_token_cpy(buffer + 1, 1022, &tok);
		buffer[count + 1] = '\"';
		buffer[count + 2] = 0;
	} else if(tok.type == LEXER_TOKEN_NUMBER && (tok.subtype & LEXER_TOKEN_SINGLE_PREC) != 0) {
		lexer_size count = lexer_token_cpy(buffer, 1023, &tok);
		buffer[count] = 'f';
		buffer[count + 1] = 0;
	} else {
		lexer_token_cpy(buffer, 1024, &tok);
	}
	return buffer;
}

bool mm_lexer_parse_enum(lexer *lex, std::string defaultVal)
{
	lexer_token name;
	if(!lexer_read_on_line(lex, &name))
		return false;

	BB_LOG("mm_lexer", "AUTOJSON enum %s", lexer_token_string(name).c_str());
	enum_s e = { lexer_token_string(name) };

	lexer_token tok;
	if(!lexer_expect_type(lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_BRACE_OPEN, &tok))
		return false;

	while(1) {
		if(!lexer_read(lex, &tok))
			break;

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACE_CLOSE) {
			BB_LOG("mm_lexer", "AUTOJSON end enum");
			break;
		}

		if(tok.type != LEXER_TOKEN_NAME) {
			break;
		}

		enum_member_s member = { lexer_token_string(tok) };
		BB_LOG("mm_lexer", "member: %s", member.name.c_str());

		e.members.push_back(member);

		if(!lexer_read_on_line(lex, &tok))
			break;

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_COMMA) {
			BB_LOG("mm_lexer", "no value");
		} else if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_ASSIGN) {
			BB_LOG("mm_lexer", "value");
			lexer_skip_line(lex);
		} else {
			break;
		}
	}

	if(!e.members.empty()) {
		if(defaultVal.empty()) {
			e.defaultVal = e.members.back().name;
		} else {
			e.defaultVal = defaultVal;
		}
		g_enums.push_back(e);
		return true;
	}
	return false;
}

bool mm_lexer_parse_struct(lexer *lex, bool autovalidate, bool headerOnly, bool isTypedef)
{
	lexer_token name;
	if(!lexer_read_on_line(lex, &name)) {
		BB_ERROR("mm_lexer::parse_struct", "Failed to parse 'unknown': expected name on line %u", lex->line);
		return false;
	}

	struct_s s = { lexer_token_string(name), "", autovalidate, headerOnly };
	//BB_LOG("mm_lexer", "AUTOJSON struct %s on line %u", s.name.c_str(), name.line);

	lexer_token tok;
	if(!lexer_expect_type(lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_BRACE_OPEN, &tok)) {
		BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected { on line %u", s.name.c_str(), lex->line);
		return false;
	}

	int indent = 1;
	while(1) {
		if(!lexer_read(lex, &tok)) {
			BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': out of data on line %u", s.name.c_str(), lex->line);
			return false;
		}

		if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACE_CLOSE) {
			--indent;
			if(!indent) {
				if(isTypedef) {
					s.typedefBaseName = s.name;
					if(lexer_read(lex, &tok) && tok.type == LEXER_TOKEN_NAME) {
						s.name = lexer_token_string(tok);
					}
				}
				break;
			}
		} else if(tok.type == LEXER_TOKEN_NAME) {
			std::vector< lexer_token > tokens;
			tokens.push_back(tok);
			size_t equalsIndex = 0;
			size_t bracketIndex = 0;
			while(1) {
				if(!lexer_read(lex, &tok)) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': out of data on line %u", s.name.c_str(), lex->line);
					return false;
				}
				if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_SEMICOLON) {
					break;
				} else {
					if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_ASSIGN) {
						if(!equalsIndex) {
							equalsIndex = tokens.size();
						}
					}
					if(tok.type == LEXER_TOKEN_PUNCTUATION && tok.subtype == LEXER_PUNCT_BRACKET_OPEN) {
						if(!bracketIndex) {
							bracketIndex = tokens.size();
						}
					}
					tokens.push_back(tok);
				}
			}

			if(tokens.size() < 2)
				return false;

			if(equalsIndex > 0 && bracketIndex > 0) {
				BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': [] and = cannot be combined on line %u", s.name.c_str(), lex->line);
				return false;
			}

			if(equalsIndex > 1) {
				lexer_token name = tokens[equalsIndex - 1];
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < equalsIndex - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				std::string valstr;
				for(auto i = equalsIndex + 1; i < tokens.size(); ++i) {
					valstr += " ";
					valstr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s\n  val:%s", lexer_token_string(name), typestr.c_str(), valstr.c_str());

				struct_member_s m;
				m.name = lexer_token_string(name);
				m.val = valstr[0] == ' ' ? valstr.c_str() + 1 : valstr;
				for(auto i = 0; i < equalsIndex - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			} else if(bracketIndex > 1) {
				lexer_token name = tokens[bracketIndex - 1];
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < bracketIndex - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				std::string valstr;
				for(auto i = bracketIndex + 1; i < tokens.size() - 1; ++i) {
					valstr += " ";
					valstr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s\n  val:%s", lexer_token_string(name), typestr.c_str(), valstr.c_str());

				struct_member_s m;
				m.name = lexer_token_string(name);
				m.arr = valstr[0] == ' ' ? valstr.c_str() + 1 : valstr;
				for(auto i = 0; i < bracketIndex - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			} else if(!equalsIndex) {
				lexer_token name = tokens.back();
				if(name.type != LEXER_TOKEN_NAME) {
					BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': expected name on line %u", s.name.c_str(), lex->line);
					return false;
				}

				std::string typestr;
				for(auto i = 0; i < tokens.size() - 1; ++i) {
					typestr += " ";
					typestr += lexer_token_string(tokens[i]);
				}

				//BB_LOG("mm_lexer", "member %s:\n  type:%s", lexer_token_string(name), typestr.c_str());
				struct_member_s m;

				m.name = lexer_token_string(name);
				for(auto i = 0; i < tokens.size() - 1; ++i) {
					m.typeTokens.push_back(tokens[i]);
				}
				s.members.push_back(m);
			}
		} else {
			BB_ERROR("mm_lexer::parse_struct", "Failed to parse '%s': unexpected token on line %u", s.name.c_str(), lex->line);
			return false;
		}
	}

	//BB_LOG("mm_lexer", "struct end");

	sb_t sb = {};
	sb_append(&sb, "AUTOJSON ");
	if(isTypedef) {
		sb_append(&sb, "typedef ");
	} else {
		sb_append(&sb, "struct ");
	}

	if(autovalidate) {
		sb_append(&sb, "with AUTOVALIDATE ");
	}
	sb_va(&sb, "'%s'\n", s.name.c_str());

	for(auto &m : s.members) {
		std::string ps;
		lexer_token pt = {};
		for(const auto &it : m.typeTokens) {
			std::string s = lexer_token_string(it);
			if(s == ">") {
				m.typeStr += " >";
			} else {
				if(pt.type == LEXER_TOKEN_NAME && it.type == LEXER_TOKEN_NAME) {
					m.typeStr += " ";
				}
				m.typeStr += s;
				if(s == "<") {
					m.typeStr += " ";
				}
			}
			ps = s;
			pt = it;
		}

		if(!m.val.empty()) {
			sb_va(&sb, "  %s %s = %s\n", m.typeStr.c_str(), m.name.c_str(), m.val.c_str());
		} else if(!m.arr.empty()) {
			sb_va(&sb, "  %s %s[%s]\n", m.typeStr.c_str(), m.name.c_str(), m.arr.c_str());
		}
		else {
			sb_va(&sb, "  %s %s\n", m.typeStr.c_str(), m.name.c_str());
		}
	}
	g_structs.push_back(s);

	BB_LOG("mm_lexer", "%s", sb_get(&sb));
	sb_reset(&sb);
	return true;
}

void mm_lexer_scan_file(const char *text, lexer_size text_length, const char *path, const sb_t *basePath)
{
	/* initialize lexer */
	lexer lex;
	lexer_init(&lex, text, text_length, NULL, NULL, NULL);

	/* parse tokens */

	bool foundAny = false;
	while(lexer_skip_until(&lex, "AUTOJSON")) {
		if(lexer_check_string(&lex, "enum")) {
			foundAny = mm_lexer_parse_enum(&lex, "") || foundAny;
		} else if(lexer_check_string(&lex, "AUTODEFAULT")) {
			std::string defaultVal;
			lexer_token tok;
			if(lexer_check_type(&lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_PARENTHESE_OPEN, &tok)) {
				if(lexer_read(&lex, &tok)) {
					if(tok.type == LEXER_TOKEN_NAME) {
						defaultVal = lexer_token_string(tok);
						if(lexer_check_type(&lex, LEXER_TOKEN_PUNCTUATION, LEXER_PUNCT_PARENTHESE_CLOSE, &tok)) {
							if(lexer_check_string(&lex, "enum")) {
								foundAny = mm_lexer_parse_enum(&lex, defaultVal) || foundAny;
							}
						}
					}
				}
			}
		} else {
			bool autovalidate = false;
			bool headerOnly = false;

			while(1) {
				if(lexer_check_string(&lex, "AUTOVALIDATE")) {
					autovalidate = true;
				} else if(lexer_check_string(&lex, "AUTOHEADERONLY")) {
					headerOnly = true;
				} else {
					break;
				}
			}

			if(lexer_check_string(&lex, "struct")) {
				foundAny = mm_lexer_parse_struct(&lex, autovalidate, headerOnly, false) || foundAny;
			} else if(lexer_check_string(&lex, "typedef")) {
				if(lexer_check_string(&lex, "struct")) {
					foundAny = mm_lexer_parse_struct(&lex, autovalidate, headerOnly, true) || foundAny;
				}
			}
		}
	}

	if(foundAny) {
		g_paths.insert(path + basePath->count);
	}
}

static void find_include_files_in_dir(const char *dir, sdict_t *sd)
{
	WIN32_FIND_DATA find;
	HANDLE hFind;

	sb_t filter = {};
	sb_va(&filter, "%s\\*.*", dir);
	if(INVALID_HANDLE_VALUE != (hFind = FindFirstFileA(sb_get(&filter), &find))) {
		do {
			if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if(find.cFileName[0] != '.') {
					sb_t subdir = {};
					sb_va(&subdir, "%s\\%s", dir, find.cFileName);
					find_include_files_in_dir(sb_get(&subdir), sd);
					sb_reset(&subdir);
				}
			} else {
				const char *ext = strrchr(find.cFileName, '.');
				if(ext && !_stricmp(ext, ".h")) {
					sdictEntry_t entry = {};
					sb_va(&entry.key, "%s\\%s", dir, find.cFileName);
					sb_append(&entry.value, find.cFileName);
					sdict_add(sd, &entry);
				}
			}
		} while(FindNextFileA(hFind, &find));
		FindClose(hFind);
	}
	sb_reset(&filter);
}

static void scanHeaders(const char *exeDir, const char *subdir)
{
	sb_t scanDir = {};
	sb_va(&scanDir, "%s\\..\\..\\..\\..\\..\\src\\%s", exeDir, subdir);
	path_resolve_inplace(&scanDir);
	sdict_t sd = {};
	find_include_files_in_dir(sb_get(&scanDir), &sd);
	sdict_sort(&sd);

	for(u32 i = 0; i < sd.count; ++i) {
		const char *path = sb_get(&sd.data[i].key);
		fileData_t contents = fileData_read(path);
		if(!contents.buffer)
			continue;

		BB_LOG("mm_lexer::scan_file", "^8%s", path);
		mm_lexer_scan_file((char *)contents.buffer, contents.bufferSize, path, &scanDir);
	}

	sdict_reset(&sd);
	sb_reset(&scanDir);
}

void CheckFreeType(sb_t *outDir)
{
	sb_t data;
	sb_init(&data);
	sb_t *s = &data;

	sb_t freetypePath;
	sb_init(&freetypePath);
	sb_va(&freetypePath, "%s\\..\\thirdparty\\freetype\\include\\freetype\\freetype.h", sb_get(outDir));
	path_resolve_inplace(&freetypePath);

	sb_append(s, "// Copyright (c) 2012-2018 Matt Campbell\n");
	sb_append(s, "// MIT license (see License.txt)\n");
	sb_append(s, "\n");
	sb_append(s, "// AUTOGENERATED FILE - DO NOT EDIT\n");
	sb_append(s, "\n");
	sb_append(s, "// clang-format off\n");
	sb_append(s, "\n");
	sb_append(s, "#pragma once\n");
	sb_append(s, "\n");
	sb_append(s, "#include \"bb_common.h\"\n");
	sb_append(s, "\n");
	if(file_readable(sb_get(&freetypePath))) {
		sb_append(s, "#define FEATURE_FREETYPE BB_ON\n");
	} else {
		sb_append(s, "#define FEATURE_FREETYPE BB_OFF\n");
	}

	sb_t path;
	sb_init(&path);
	sb_va(&path, "%s\\fonts_generated.h", sb_get(outDir));
	fileData_writeIfChanged(sb_get(&path), NULL, { data.data, sb_len(s) });
	sb_reset(&path);
	sb_reset(&data);
}

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
#ifdef LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif // #ifdef LEAK_CHECK

	// uncomment to debug preprocessing
	//BB_INIT_WITH_FLAGS("p4t_preproc", kBBInitFlag_NoOpenView);
	BB_THREAD_SET_NAME("main");

	char exeDir[_MAX_PATH];
	const char *exeFilename;

	GetModuleFileNameA(0, exeDir, sizeof(exeDir));
	char *exeSep = strrchr(exeDir, '\\');
	exeFilename = exeSep + 1;
	*exeSep = 0;

	scanHeaders(exeDir, "p4t");
	scanHeaders(exeDir, "common");

	sb_t outDir = {};
	sb_va(&outDir, "%s\\..\\..\\..\\..\\..\\src\\p4t", exeDir);
	path_resolve_inplace(&outDir);
	GenerateJson(&outDir);
	CheckFreeType(&outDir);
	sb_reset(&outDir);

	BB_SHUTDOWN();

	return 0;
}
