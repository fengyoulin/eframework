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