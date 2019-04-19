#ifndef _BUFFER_HEADER_
#define _BUFFER_HEADER_

#include <stdlib.h>
#include <string.h>

typedef struct _ef_buffer {
    unsigned char *ptr;
    size_t cap;
    size_t len;
} ef_buffer_t;

inline ef_buffer_t *ef_buffer_new(size_t cap) __attribute__((always_inline));
inline int ef_buffer_expand(ef_buffer_t *buf, size_t len) __attribute__((always_inline));
inline int ef_buffer_append(ef_buffer_t *buf, void *data, size_t len) __attribute__((always_inline));
inline void ef_buffer_free(ef_buffer_t *buf, int destroy) __attribute__((always_inline));

inline ef_buffer_t *ef_buffer_new(size_t cap)
{
    ef_buffer_t *buf = (ef_buffer_t *)malloc(sizeof(ef_buffer_t));
    if (!buf) {
        return NULL;
    }
    buf->ptr = cap ? (unsigned char*)malloc(cap) : NULL;
    if (cap && !buf->ptr) {
        free(buf);
        return NULL;
    }
    buf->cap = cap;
    buf->len = 0;
    return buf;
}

inline int ef_buffer_expand(ef_buffer_t *buf, size_t len)
{
    size_t cap;
    unsigned char *ptr = NULL;
    if (buf->cap - buf->len >= len) {
        return 0;
    }
    cap = buf->len + len;
    if (buf->ptr) {
        ptr = (unsigned char*)realloc(buf->ptr, cap);
    } else {
        ptr = (unsigned char*)malloc(cap);
    }
    if (ptr) {
        buf->ptr = ptr;
        buf->cap = cap;
        return 0;
    }
    return -1;
}

inline int ef_buffer_append(ef_buffer_t *buf, void *data, size_t len)
{
copy_data:
    if (buf->cap - buf->len >= len) {
        memcpy(buf->ptr + buf->len, data, len);
        buf->len += len;
        return 0;
    }
    if (ef_buffer_expand(buf, len) < 0) {
        return -1;
    } else {
        goto copy_data;
    }
    return -1;
}

inline void ef_buffer_free(ef_buffer_t *buf, int destroy)
{
    free(buf->ptr);
    if (destroy) {
        free(buf);
    } else {
        buf->ptr = NULL;
        buf->cap = 0;
        buf->len = 0;
    }
}

#endif