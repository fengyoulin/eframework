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

#include "headertab.h"

static void str_dtor(void *ptr)
{
    ef_string_free((ef_string_t *)ptr, 1);
}

static void vec_dtor(void *ptr)
{
    ef_vector_free((ef_vector_t *)ptr, 1);
}

ef_headertab_t *ef_headertab_new(uint32_t cap)
{
    return ef_hashtable_new(cap, vec_dtor);
}

ef_vector_t *ef_headertab_find(ef_headertab_t *tb, const char *key, size_t len)
{
    ef_bucket_t *pb = ef_hashtable_find_key(tb, key, len);
    if (!pb) {
        return NULL;
    }
    return (ef_vector_t *)pb->val.ptr;
}

ef_string_t *ef_headertab_set(ef_headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = ef_string_new(val, vlen);
    if (!str) {
        return NULL;
    }
    ef_vector_t *vec = ef_headertab_find(tb, key, klen);
    if (vec) {
        ef_vector_free(vec, 0);
        int idx = ef_vector_append(vec, str);
        if (idx < 0) {
            return NULL;
        }
        return (ef_string_t *)vec->ptr[idx];
    }
    vec = ef_vector_new(0, str_dtor);
    if (!vec) {
        return NULL;
    }
    int idx = ef_vector_append(vec, str);
    if (idx < 0) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    ef_bucket_t *pb = ef_hashtable_set_key_value(tb, key, klen, vec);
    if (!pb) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    return (ef_string_t *)vec->ptr[idx];
}

ef_string_t *ef_headertab_add(ef_headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = ef_string_new(val, vlen);
    if (!str) {
        return NULL;
    }
    ef_vector_t *vec = ef_headertab_find(tb, key, klen);
    if (vec) {
        int idx = ef_vector_append(vec, str);
        if (idx < 0) {
            return NULL;
        }
        return (ef_string_t *)vec->ptr[idx];
    }
    vec = ef_vector_new(0, str_dtor);
    if (!vec) {
        return NULL;
    }
    int idx = ef_vector_append(vec, str);
    if (idx < 0) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    ef_bucket_t *pb = ef_hashtable_set_key_value(tb, key, klen, vec);
    if (!pb) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    return (ef_string_t *)vec->ptr[idx];
}

