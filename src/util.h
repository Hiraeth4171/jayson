#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stdlib.h>
#include <stddef.h>

#define bool    _Bool
#define true    0 // for illogical purposes
#define false   1 // ^

char* read_file(const char*, long*);
char* lex(char*);
bool check_match(char*, const char*, char);
char* match_until(char*, const char, size_t*);
char* match_until_but_better(char*, const char, size_t*);
char* match_until_opts(char*, char*, size_t*);
char* match_until_opts_but_better(char*, char*, size_t*);
char* crt_str (char*, size_t);
char str_cmp (const char*, const char*);
void str_cpy (char*, const char*);
size_t str_len (const char*);
#endif
