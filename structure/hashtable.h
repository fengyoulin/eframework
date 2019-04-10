#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "string.h"

//HashTable
#define HASH_TABLE_INIT_CAP (8)

#define HASH_DATA_START(ht) ((uint32_t*)ht->arrData - ht->cap)
#define HASH_SIZEMASK(ht) (-(ht->cap))
#define HASH_OFFSET(ht,h) ((int32_t)((uint32_t)h | ht->sizemask))

typedef void (*val_dtor_t)(void *ptr);

typedef union _bucket_value bucket_value_t;
typedef struct _bucket bucket_t;
typedef struct _hashtable hashtable_t;

union _bucket_value {
    long lval;
    double dval;
    void *ptr;
    char *sz;
    ef_string_t *str;
    hashtable_t *arr;
};

struct _bucket {
    unsigned long h;
    ef_string_t *key;
    bucket_value_t val;
    uint32_t next;
};

struct _hashtable {
    uint32_t cap;
    uint32_t sizemask;
    uint32_t used;
    uint32_t next;
    uint32_t removed;
    val_dtor_t val_dtor;
    bucket_t *arrData;
};

hashtable_t *new_hash_table(uint32_t cap, val_dtor_t val_dtor);

bucket_t *hash_find_key(hashtable_t *ht, const char *key, size_t len);

bucket_t *hash_set_key_value(hashtable_t *ht, const char *key, size_t klen, void *val);

inline int hash_exists(hashtable_t *ht, const char *key, size_t len)
{
    return hash_find_key(ht, key, len) != NULL;
}

int hash_remove_key(hashtable_t *ht, const char *key, size_t len);

int hash_resize(hashtable_t *ht, uint32_t cap);

void hash_free(hashtable_t *ht);

