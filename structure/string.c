#include "string.h"

ef_string_t *ef_string_new(const char *sz, size_t len)
{
    ef_string_t *str = (ef_string_t *)malloc(sizeof(ef_string_t));
    if (!str) {
        return NULL;
    }
    if (len == 0) {
        len = sz ? strlen(sz) : 0;
    }
    str->str = (char *)malloc(len + 1);
    if (!str->str) {
        free(str);
        return NULL;
    }
    if (len) {
        memcpy(str->str, sz, len);
    }
    str->str[len] = 0;
    str->len = len;
    return str;
}

int ef_string_append(ef_string_t *str, const char *sz, size_t len)
{
    char *ptr = NULL;
    if (len == 0) {
        len = sz ? strlen(sz) : 0;
    }
    if (len == 0) {
        return 0;
    }
    if (str->str) {
        ptr = (char *)realloc(str->str, str->len + len + 1);
    } else {
        ptr = (char *)malloc(len + 1);
    }
    if (!ptr) {
        return -1;
    }
    memcpy(ptr + str->len, sz, len);
    str->len += len;
    ptr[str->len] = 0;
    str->str = ptr;
    return 0;
}

void ef_string_free(ef_string_t *str, int destroy)
{
    free(str->str);
    if (destroy) {
        free(str);
    } else {
        str->str = NULL;
        str->len = 0;
    }
}
