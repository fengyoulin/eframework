#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//HashTable
#define HASH_TABLE_INIT_CAP (8)
#define HASH_VAL_TYPE_STR (1<<1)
#define HASH_VAL_TYPE_LONG (1<<2)
#define HASH_VAL_TYPE_ARR (1<<3)

#define HASH_DATA_START(ht) ((size_t*)ht->arrData - ht->cap)
#define HASH_SIZEMASK(ht) (-(ht->cap))
#define HASH_OFFSET(ht,h) (h | ht->sizemask)

typedef struct block_val_t {
    short type;
    union {
    	long d;
    	char *str;
    	struct hashtable_t *arr;
    } v;
} block_val;

typedef struct block_t {
    uint h;
    char *key;
    block_val *val;
    size_t next;
} block;

typedef struct hashtable_t {
    size_t cap;
    size_t sizemask;
    size_t used;
    size_t next;
    block *arrData;
} hashtable;


hashtable *new_hash_table(size_t cap);
void init_hash_data(size_t *data,size_t cap);
size_t hash(char *key);

block_val *hash_get(hashtable *ht,char *key);
int hash_set(hashtable *ht,char *key,char *val);
unsigned short hash_exists(hashtable *ht,char *key);
int hash_remove(hashtable *ht,char *key);

int hash_resize(hashtable *ht);
void hash_rehash(hashtable *ht);
size_t hash_recap(size_t cap);
void hash_copy_block(block* dest,block* src,size_t count);

void hash_free_block(block *pb,char freeval);
void hash_free_block_val(block_val *pval);
void hash_free(hashtable *ht);