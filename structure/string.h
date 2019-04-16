#ifndef _STRING_HEADER_
#define _STRING_HEADER_

#include <stdlib.h>
#include <string.h>

typedef struct _ef_string {
    char *str;
    size_t len;
} ef_string_t;

inline ef_string_t *ef_string_new(const char *sz, size_t len) __attribute__((always_inline));
inline int ef_string_append(ef_string_t *str, const char *sz, size_t len) __attribute__((always_inline));
inline void ef_string_free(ef_string_t *str, int destroy) __attribute__((always_inline));

inline ef_string_t *ef_string_new(const char *sz, size_t len)
{
    ef_string_t *str = (ef_string_t *)malloc(sizeof(ef_string_t));
    if(!str) {
        return NULL;
    }
    if (!len) {
        len = sz ? strlen(sz) : 0;
    }
    str->str = len ? (char *)malloc(len + 1) : NULL;
    if(str->str) {
        memcpy(str->str, sz, len);
        str->str[len] = 0;
    }
    str->len = len;
    return str;
}

inline int ef_string_append(ef_string_t *str, const char *sz, size_t len)
{
    if(len <= 0) {
        len = sz ? strlen(sz) : 0;
    }
    if(!len) {
        return 0;
    }
    char *ptr = NULL;
    if(str->str) {
        ptr = (char *)realloc(str->str, str->len + len + 1);
    } else {
        ptr = (char *)malloc(len + 1);
    }
    if(!ptr) {
        return -1;
    }
    memcpy(ptr + str->len, sz, len);
    str->len += len;
    ptr[str->len] = 0;
    str->str = ptr;
    return 0;
}

inline void ef_string_free(ef_string_t *str, int destroy)
{
    if(!str) {
        return;
    }
    free(str->str);
    if(destroy) {
        free(str);
    } else {
        str->str = NULL;
        str->len = 0;
    }
}

#endif