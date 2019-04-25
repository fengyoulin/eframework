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

#include "hashtable.h"

static ef_bucket_t *alloc_hash_data(uint32_t cap)
{
    size_t hash_size = sizeof(uint32_t) * cap;
    size_t data_size = sizeof(ef_bucket_t) * cap;
    uint32_t *data = (uint32_t *)malloc(hash_size + data_size);
    if (!data) {
        return NULL;
    }
    memset(data, -1, hash_size);
    memset(data + cap, 0, data_size);
    return (ef_bucket_t *)(data + cap);
}

static void free_hash_data(ef_bucket_t *data, uint32_t cap)
{
    free((uint32_t *)data - cap);
}

/* from PHP */
static unsigned long hash_func(const char *str, size_t len)
{
    unsigned long hash = 5381UL;

    /* variant with the hash unrolled eight times */
    for (; len >= 8; len -= 8) {
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
        hash = ((hash << 5) + hash) + *str++;
    }
    switch (len) {
        case 7: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 6: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 5: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 4: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 3: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 2: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
        case 1: hash = ((hash << 5) + hash) + *str++; break;
        case 0: break;
        default: break;
    }

    /* Hash value can't be zero, so we always set the high bit */
#if __x86_64__
    return hash | 0x8000000000000000UL;
#else
    return hash | 0x80000000UL;
#endif
}

// 计算大于等于cap的最小2^n
static uint32_t hash_recap(uint32_t cap)
{
    if (cap <= HASH_TABLE_INIT_CAP) {
        return HASH_TABLE_INIT_CAP;
    }
    
    cap -= 1;
    cap |= (cap >> 1);
    cap |= (cap >> 2);
    cap |= (cap >> 4);
    cap |= (cap >> 8);
    cap |= (cap >> 16);
    
    return cap + 1;
}

static void ef_hashtable_free_bucket(ef_hashtable_t *ht, ef_bucket_t *pb)
{
    if (!pb->h && !pb->key) {
        return;
    }
    pb->h = 0;
    if (pb->key) {
        ef_string_free(pb->key, 1);
        pb->key = NULL;
    }
    if (pb->val.ptr && ht->val_dtor){
        ht->val_dtor(pb->val.ptr);
        pb->val.ptr = NULL;
    }
    pb->next = ht->removed;
    ht->removed = pb - ht->arrData;
    --ht->used;
}

ef_hashtable_t *ef_hashtable_new(uint32_t cap, val_dtor_t val_dtor)
{
    cap = hash_recap(cap);

    ef_bucket_t *bks = alloc_hash_data(cap);
    if (!bks) {
        return NULL;
    }

    ef_hashtable_t *ht = (ef_hashtable_t *)malloc(sizeof(ef_hashtable_t));
    if (!ht) {
        free_hash_data(bks, cap);
        return NULL;
    }

    ht->cap = cap;
    ht->used = 0;
    ht->next = 0;
    ht->removed = -1;
    ht->sizemask = HASH_SIZEMASK(ht);
    ht->arrData = bks;
    ht->val_dtor = val_dtor;

    return ht;
}

ef_bucket_t *ef_hashtable_set_key_value(ef_hashtable_t *ht, const char *key, size_t klen, void *val)
{
    ef_bucket_t *pb = ef_hashtable_find_key(ht, key, klen);
    if (pb) {
        if (ht->val_dtor && pb->val.ptr) {
            ht->val_dtor(pb->val.ptr);
        }
        pb->val.ptr = val;
        return pb;
    }

alloc_bucket:
    if (ht->next < ht->cap) {
        pb = &ht->arrData[ht->next++];
    } else if (ht->removed != -1) {
        pb = &ht->arrData[ht->removed];
        ht->removed = pb->next;
    } else if (!ef_hashtable_resize(ht, ht->cap << 1)) {
        goto alloc_bucket;
    } else {
        return NULL;
    }
    ++ht->used;

    pb->key = ef_string_new(key, klen);
    if (!pb->key) {
        pb->next = ht->removed;
        ht->removed = pb - ht->arrData;
        --ht->used;
        return NULL;
    }
    pb->h = hash_func(key, klen);
    pb->val.ptr = val;

    int32_t offset = HASH_OFFSET(ht, pb->h);
    pb->next = HASH_ENTRY(ht, offset);
    HASH_ENTRY(ht, offset) = pb - ht->arrData;

    return pb;
}

ef_bucket_t *ef_hashtable_find_key(ef_hashtable_t *ht, const char *key, size_t len)
{
    unsigned long h = hash_func(key, len);
    int32_t offset = HASH_OFFSET(ht, h);
    uint32_t index = HASH_ENTRY(ht, offset);
    if (index == -1) {
        return NULL;
    }

    ef_bucket_t *pb = &ht->arrData[index];
    while (1) {
        if (pb->h == h && pb->key &&
            pb->key->len == len &&
            memcmp(pb->key->str, key, len) == 0) {
            return pb;
        }
        if (pb->next == -1) {
            break;
        }
        pb = &ht->arrData[pb->next];
    }
    return NULL;
}

int ef_hashtable_remove_key(ef_hashtable_t *ht, const char *key, size_t len)
{
    unsigned long h = hash_func(key, len);
    int32_t offset = HASH_OFFSET(ht, h);
    uint32_t *pidx = &HASH_ENTRY(ht, offset);
    if (*pidx == -1) {
        return 0;
    }

    ef_bucket_t *pb = &ht->arrData[*pidx];
    while (1) {
        if (pb->h == h && pb->key &&
            pb->key->len == len &&
            memcmp(pb->key->str, key, len) == 0) {
            *pidx = pb->next;
            ef_hashtable_free_bucket(ht, pb);
            return 0;
        }
        if (pb->next == -1) {
            break;
        }
        pidx = &pb->next;
        pb = &ht->arrData[pb->next];
    }
    return 0;
}

void ef_hashtable_free(ef_hashtable_t *ht)
{
    for (int idx = 0; idx < ht->cap; ++idx) {
        ef_hashtable_free_bucket(ht, &ht->arrData[idx]);
    }
    free_hash_data(ht->arrData, ht->cap);
    free(ht);
}

int ef_hashtable_resize(ef_hashtable_t *ht, uint32_t cap)
{
    cap = hash_recap(cap);
    if (cap < ht->used) {
        return -1;
    }

    ef_bucket_t *newbks = alloc_hash_data(cap);
    if (!newbks) {
        return -1;
    }

    ef_bucket_t *oldbks = ht->arrData;
    uint32_t oldcap = ht->cap;
    ht->arrData = newbks;
    ht->cap = cap;
    ht->sizemask = HASH_SIZEMASK(ht);
    ht->removed = -1;
    ht->next = ht->used;

    ef_bucket_t *newptr = newbks;
    for (int idx = 0; idx < oldcap; ++idx) {
        ef_bucket_t *pb = &oldbks[idx];
        if (!pb->h && !pb->key) {
            continue;
        }

        newptr->h = pb->h;
        newptr->key = pb->key;
        memcpy(&newptr->val, &pb->val, sizeof(ef_bucket_value_t));

        int32_t offset = HASH_OFFSET(ht, newptr->h);
        newptr->next = HASH_ENTRY(ht, offset);
        HASH_ENTRY(ht, offset) = newptr - newbks;

        ++newptr;
    }

    free_hash_data(oldbks, oldcap);

    return 0;
}

