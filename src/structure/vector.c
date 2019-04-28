// Copyright (c) 2018-2019 The EFramework Project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "vector.h"
#include "util.h"

#define VECTOR_GROW_SIZE 8

ef_vector_t *ef_vector_new(size_t cap, val_dtor_t val_dtor)
{
    ef_vector_t *vec = (ef_vector_t *)malloc(sizeof(ef_vector_t));
    if (!vec) {
        return NULL;
    }
    if (cap) {
        cap = ef_resize(cap, VECTOR_GROW_SIZE);
    }
    vec->cap = cap;
    vec->len = 0;
    vec->ptr = cap ? (void **)malloc(cap * sizeof(void *)) : NULL;
    vec->val_dtor = val_dtor;
    if (cap && !vec->ptr) {
        free(vec);
        return NULL;
    }
    return vec;
}

int ef_vector_append(ef_vector_t *vec, void *ptr)
{
    size_t idx = vec->len;
    if (vec->cap > vec->len) {
        ++vec->len;
        vec->ptr[idx] = ptr;
        return idx;
    }
    vec->cap += VECTOR_GROW_SIZE;
    void **nptr = NULL;
    if (vec->ptr) {
        nptr = (void **)realloc(vec->ptr, vec->cap);
    } else {
        nptr = (void **)malloc(vec->cap);
    }
    if (nptr) {
        ++vec->len;
        vec->ptr = nptr;
        vec->ptr[idx] = ptr;
        return idx;
    }
    return -1;
}

void ef_vector_free(ef_vector_t *vec, int destroy)
{
    if (!vec) {
        return;
    }
    if (vec->val_dtor && vec->ptr) {
        for (size_t idx = 0; idx < vec->len; ++idx) {
            if (vec->ptr[idx]) {
                vec->val_dtor(vec->ptr[idx]);
            }
        }
    }
    free(vec->ptr);
    if (destroy) {
        free(vec);
    } else {
        vec->ptr = NULL;
        vec->cap = 0;
        vec->len = 0;
    }
}
