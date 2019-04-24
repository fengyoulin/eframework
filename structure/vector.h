#ifndef _VECTOR_HEADER_
#define _VECTOR_HEADER_

#include <stdlib.h>
#include <string.h>

typedef void (*val_dtor_t)(void *ptr);

typedef struct _ef_vector {
    void **ptr;
    size_t cap;
    size_t len;
    val_dtor_t val_dtor;
} ef_vector_t;

ef_vector_t *ef_vector_new(size_t cap, val_dtor_t val_dtor);

int ef_vector_append(ef_vector_t *vec, void *ptr);

void ef_vector_free(ef_vector_t *vec, int destroy);

#endif // _VECTOR_HEADER_