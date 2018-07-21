// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "py_parser.h"

#include "env_utils.h"
#include "file_utils.h"
#include "output.h"
#include "process.h"
#include "sdict.h"
#include "span.h"
#include "tokenize.h"
#include "va.h"

#include "bb.h"
#include "bb_array.h"

#define parser_error(parser, msg) parser_error_((parser), (msg), __FILE__, __LINE__)
static void parser_error_(pyParser *parser, const char *msg, const char *file, int line)
{
	BB_ERROR_DYNAMIC(file, line, "p4::parser", "Error parsing output from %s\n  %s\n", parser->cmdline, msg);
	parser->state = kParser_Error;
}

static b32 parser_peek_char(pyParser *parser, char *val)
{
	b32 ret;
	if(parser->consumed < parser->count) {
		*val = parser->data[parser->consumed];
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}

static b32 parser_peek_u32(pyParser *parser, u32 *val)
{
	b32 ret;
	if(parser->consumed + 4 <= parser->count) {
		*val = *(u32 *)(parser->data + parser->consumed);
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}

static b32 parser_advance(pyParser *parser, u32 bytes)
{
	b32 ret;
	if(parser->consumed + bytes <= parser->count) {
		parser->consumed += bytes;
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}

b32 py_parser_tick(pyParser *parser, sdicts *dicts)
{
	b32 ret = false;
	if(parser->state == kParser_Error) {
	} else if(parser->state == kParser_Dict) {
		char c;
		if(parser_peek_char(parser, &c)) {
			if(c == '0') {
				++parser->consumed;
				parser->state = kParser_Idle;
				ret = true;
				//output_log("finished dict w/%u entries", parser->dict.count);
				if(bba_add_noclear(*dicts, 1)) {
					bba_last(*dicts) = parser->dict;
					memset(&parser->dict, 0, sizeof(parser->dict));
				} else {
					BB_ERROR("p4::parser", "could not save dict");
					sdict_reset(&parser->dict);
				}
			} else if(c == 's') {
				// we need a complete key/val string pair, so don't process otherwise
				u32 consumed = parser->consumed;
				++parser->consumed;
				u32 keyLen, valLen;
				const char *key, *val;
				if(parser_peek_u32(parser, &keyLen)) {
					parser_advance(parser, 4);
					key = parser->data + parser->consumed;
					if(parser_advance(parser, keyLen)) {
						if(parser_peek_char(parser, &c)) {
							if(c == 's') {
								++parser->consumed;
								if(parser_peek_u32(parser, &valLen)) {
									parser_advance(parser, 4);
									val = parser->data + parser->consumed;
									if(parser_advance(parser, valLen)) {
										ret = true;
										sdictEntry_t entry;
										sb_init(&entry.key);
										sb_append_range(&entry.key, key, key + keyLen);
										sb_init(&entry.value);
										sb_append_range(&entry.value, val, val + valLen);
										output_log("parsed key/value: %s = %s\n", sb_get(&entry.key), sb_get(&entry.value));
										sdict_add(&parser->dict, &entry);
									}
								}
							} else if(c == 'i') {
								++parser->consumed;
								u32 n;
								if(parser_peek_u32(parser, &n)) {
									if(parser_advance(parser, 4)) {
										ret = true;
										sdictEntry_t entry;
										sb_init(&entry.key);
										sb_append_range(&entry.key, key, key + keyLen);
										sb_init(&entry.value);
										sb_va(&entry.value, "%u", n);
										output_log("parsed key/value: %s = %s\n", sb_get(&entry.key), sb_get(&entry.value));
										sdict_add(&parser->dict, &entry);
									}
								}
							} else {
								parser_error(parser, va("saw char 0x%2.2x instead of dict val", c));
							}
						}
					}
				}
				if(!ret) {
					parser->consumed = consumed;
				}
			} else {
				parser_error(parser, va("saw char 0x%2.2x instead of dict key or terminator", c));
			}
		}
	} else if(parser->state == kParser_Idle) {
		char c;
		if(parser_peek_char(parser, &c)) {
			if(c == '{') {
				++parser->consumed;
				parser->state = kParser_Dict;
				ret = true;
			} else {
				parser_error(parser, va("saw char 0x%2.2x instead of dict", c));
			}
		}
	} else {
		parser_error(parser, va("unknown parser state %d", parser->state));
	}
	return ret;
}
