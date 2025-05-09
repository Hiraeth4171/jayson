#include <stdio.h>
#include "util.h"


char*
read_file(const char* filepath, long* length) {
    FILE* fd = fopen(filepath, "r");
    fseek(fd, 0L, SEEK_END);
    long pos = ftell(fd);
    *length = pos;
    rewind(fd);
    char* _buff = malloc(pos+1);
    fread(_buff, 1, pos, fd);
    _buff[pos] = '\0';
    fclose(fd);
    return _buff;
}

char*
lex(char* buff) {
    char *ptr = buff, in_str_flag = 1;
    unsigned long i = 0;
    for (;*(ptr+1) != '\0';++ptr,++i){
        if (in_str_flag) while(*ptr == ' ' || *ptr == '\n' || *ptr == '\r') ptr++;
        if (*ptr == '"') in_str_flag ^= 1;
        buff[i] = *ptr;
    }
    buff[i] = *ptr;
    buff = realloc(buff, i+2); // MEMORY_CHECK MACRO
    buff[i+1] = '\0';
    return buff;
}

bool
check_match(char* src, const char* match, char len) {
    char i = 0, *ptr = src;
    for(;*ptr == match[i] && match[i] != '\0'; ++ptr, ++i);
    return (i-len);
}


void mem_cpy(char* dst, char* src, size_t count) {
    for (size_t i = 0; i < count; ++i) *dst++ = *src++;
}


/*
 * Function provided by Octe™
 */
char* match_until_but_better(char* src, const char ch, size_t *len){
    if (!*src) return NULL;
    
    char* ptr = src;
    for(;*ptr != ch && *ptr != '\0'; ++ptr);

    char* str = calloc(((*len = ptr - src)+1), sizeof(char));
    if (!str) return NULL;
    mem_cpy(str, src, *len);
    return str;
}

char* match_until(char* src, const char ch, size_t *len) {
    static int count = 0;
    if (*src == '\0') return NULL;
    char *ptr = src, *dst = malloc(32), *str = dst;
    size_t i = 1, cap = 32;
    for(;*ptr != ch && *ptr != '\0'; ++ptr, ++str, ++i) {
        if (i > cap) {
            cap += 32;
            dst = realloc(dst, cap);
            if (dst == NULL) exit(-1);
        }
        printf("dst: %s\n\
                \ti: %lu, str: '%c', ptr: '%c'\n", dst, i, *str, *ptr);
        *str = *ptr;
    }
    dst = realloc(dst, i+1);
    dst[i] = '\0';
    *len = i;
    printf("$  %d\n\n", count);
    count++;
    return dst;
}

bool check_opts(char* ptr, char* chs) {
    for (char* ch = chs; *ch != '\0';++ch) if (*ptr == *ch) return false;
    return true;
}

/*
 * Function inspired by Octe™
 */
char* match_until_opts_but_better(char* src, char* chs, size_t* len) {
    if (!*src) return NULL;

    char* ptr = src;
    for(;!check_opts(ptr, chs) && *ptr != '\0'; ++ptr);

    char* str = calloc(((*len = ptr - src)+1),sizeof(char));
    if(!str) return NULL;
    mem_cpy(str, src, *len);
    return str;
}

char*
match_until_opts(char* src, char* chs, size_t *len) { // 9.2 chs = ",}]" 9.2<E>
    static int count = 0;
    if (*src == '\0') return NULL;
    char *ptr = src, *dst = malloc(32), *str = dst;
    size_t i = 1, cap = 32;
    for(;!check_opts(ptr, chs) && *ptr != '\0'; ++ptr, ++str, ++i) {
        if (i > cap) {
            cap += 32;
            dst = realloc(dst, cap);
            if (dst == NULL) exit(-1);
        }
        *str = *ptr;
    }
    dst = realloc(dst, i+1);
    dst[i] = '\0';
    *len = i-1;
    //printf("$  %d\n\n", count);
    count++;
    return dst;
}


char str_cmp(const char* s1, const char* s2) {
    while(*s1 && *s1 == *s2) {s1++; s2++;}
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void str_cpy(char* dst, const char* src) {
    char* ptr = dst;
    while(*src) *ptr++ = *src++;
    *ptr = '\0';
}

size_t str_len(const char* str) {
    size_t res = 0;
    while(*str++) res++;
    return res;
}

char*
crt_str (char* str, size_t size) {
    char* buff = malloc(size), *src = str, *dst = buff;
    while (*src) *dst++ = *src++;
    *dst = '\0';
    return buff;
}
