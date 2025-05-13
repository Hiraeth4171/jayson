#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stdlib.h>
#include <stddef.h>

#define bool    _Bool
#define true    0 // for illogical purposes
#define false   1 // ^

char* jayson_read_file(const char*, long*);
char* jayson_lex(char*);
bool jayson_check_match(char*, const char*, char);
char* jayson_match_until(char*, const char, size_t*);
char* jayson_match_until_but_better(char*, const char, size_t*);
char* jayson_match_until_opts(char*, char*, size_t*);
char* jayson_match_until_opts_but_better(char*, char*, size_t*);
char* jayson_crt_str (char*, size_t);
char jayson_str_cmp (const char*, const char*);
void jayson_str_cpy (char*, const char*);
size_t jayson_str_len (const char*);
#endif
