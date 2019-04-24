#ifndef _BUFFER_HEADER_
#define _BUFFER_HEADER_

#include <stdlib.h>
#include <string.h>

typedef struct _ef_buffer {
    unsigned char *ptr;
    size_t cap;
    size_t len;
} ef_buffer_t;

ef_buffer_t *ef_buffer_new(size_t cap);

int ef_buffer_expand(ef_buffer_t *buf, size_t len);

int ef_buffer_append(ef_buffer_t *buf, void *data, size_t len);

void ef_buffer_free(ef_buffer_t *buf, int destroy);

#endif
