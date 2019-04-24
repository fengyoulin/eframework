#ifndef _STRING_HEADER_
#define _STRING_HEADER_

#include <stdlib.h>
#include <string.h>

typedef struct _ef_string {
    char *str;
    size_t len;
} ef_string_t;

ef_string_t *ef_string_new(const char *sz, size_t len);

int ef_string_append(ef_string_t *str, const char *sz, size_t len);

void ef_string_free(ef_string_t *str, int destroy);

#endif
