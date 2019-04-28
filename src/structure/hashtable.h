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

#ifndef _HASHTABLE_HEADER_
#define _HASHTABLE_HEADER_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "string.h"

//HashTable
#define HASH_TABLE_INIT_CAP (8)

#define HASH_SIZEMASK(ht) (-(ht->cap))
#define HASH_OFFSET(ht,h) ((int32_t)((uint32_t)h | ht->sizemask))
#define HASH_ENTRY(ht,off) (((uint32_t *)ht->arrData)[off])

typedef void (*val_dtor_t)(void *ptr);

typedef union _ef_bucket_value ef_bucket_value_t;
typedef struct _ef_bucket ef_bucket_t;
typedef struct _ef_hashtable ef_hashtable_t;

union _ef_bucket_value {
    long lval;
    double dval;
    void *ptr;
    char *sz;
    ef_string_t *str;
    ef_hashtable_t *arr;
};

struct _ef_bucket {
    unsigned long h;
    ef_string_t *key;
    ef_bucket_value_t val;
    uint32_t next;
};

struct _ef_hashtable {
    uint32_t cap;
    uint32_t sizemask;
    uint32_t used;
    uint32_t next;
    uint32_t removed;
    val_dtor_t val_dtor;
    ef_bucket_t *arrData;
};

ef_hashtable_t *ef_hashtable_new(uint32_t cap, val_dtor_t val_dtor);

ef_bucket_t *ef_hashtable_find_key(ef_hashtable_t *ht, const char *key, size_t len);

ef_bucket_t *ef_hashtable_set_key_value(ef_hashtable_t *ht, const char *key, size_t klen, void *val);

int ef_hashtable_remove_key(ef_hashtable_t *ht, const char *key, size_t len);

int ef_hashtable_resize(ef_hashtable_t *ht, uint32_t cap);

void ef_hashtable_free(ef_hashtable_t *ht);

#endif