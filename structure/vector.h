#ifndef _VECTOR_HEADER_
#define _VECTOR_HEADER_

#include <stdlib.h>
#include <string.h>

#define VECTOR_GROW_SIZE 8

typedef void (*val_dtor_t)(void *ptr);

typedef struct _ef_vector {
    void **ptr;
    size_t cap;
    size_t len;
    val_dtor_t val_dtor;
} ef_vector_t;

inline ef_vector_t *ef_vector_new(size_t cap, val_dtor_t val_dtor) __attribute__((always_inline));
inline int ef_vector_append(ef_vector_t *vec, void *ptr) __attribute__((always_inline));
inline void ef_vector_free(ef_vector_t *vec, int destroy) __attribute__((always_inline));

inline ef_vector_t *ef_vector_new(size_t cap, val_dtor_t val_dtor)
{
    ef_vector_t *vec = (ef_vector_t *)malloc(sizeof(ef_vector_t));
    if(!vec) {
        return NULL;
    }
    vec->cap = cap;
    vec->len = 0;
    vec->ptr = cap ? (void **)malloc(cap * sizeof(void *)) : NULL;
    vec->val_dtor = val_dtor;
    if(cap && !vec->ptr) {
        free(vec);
        return NULL;
    }
    return vec;
}

inline int ef_vector_append(ef_vector_t *vec, void *ptr)
{
    size_t idx = vec->len;
    if(vec->cap > vec->len) {
        ++vec->len;
        vec->ptr[idx] = ptr;
        return idx;
    }
    vec->cap += VECTOR_GROW_SIZE;
    void **nptr = NULL;
    if(vec->ptr) {
        nptr = (void **)realloc(vec->ptr, vec->cap);
    } else {
        nptr = (void **)malloc(vec->cap);
    }
    if(nptr) {
        ++vec->len;
        vec->ptr = nptr;
        vec->ptr[idx] = ptr;
        return idx;
    }
    return -1;
}

inline void ef_vector_free(ef_vector_t *vec, int destroy)
{
    if(!vec) {
        return;
    }
    if(vec->val_dtor && vec->ptr) {
        for(size_t idx = 0; idx < vec->len; ++idx) {
            if(vec->ptr[idx]) {
                vec->val_dtor(vec->ptr[idx]);
            }
        }
    }
    free(vec->ptr);
    if(destroy) {
        free(vec);
    } else {
        vec->ptr = NULL;
        vec->cap = 0;
        vec->len = 0;
    }
}

#endif // _VECTOR_HEADER_