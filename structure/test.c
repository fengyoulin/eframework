#include "hashtable.h"
#include "stringtab.h"
#include <stdio.h>

int test_recap()
{
    hashtable_t *ht = new_hash_table(15, NULL);
    if (ht->cap != 16) {
        printf("16 != ht->cap=%d\n", ht->cap);
        return -1;
    }
    hash_free(ht);
    ht = new_hash_table(0, NULL);
    if (ht->cap != HASH_TABLE_INIT_CAP) {
        printf("%d != ht->cap=%d\n", HASH_TABLE_INIT_CAP, ht->cap);
        return -1;
    }
    hash_free(ht);
    ht = new_hash_table(32, NULL);
    if (ht->cap != 32) {
        printf("32 != ht->cap=%d\n", ht->cap);
        return -1;
    }
    
    hash_free(ht);
    ht = new_hash_table(33, NULL);
    if (ht->cap != 64) {
        printf("64 != ht->cap=%d\n", ht->cap);
        return -1;
    }
    hash_free(ht);

    return 0;
}

int test_set(hashtable_t *ht)
{
    if (hash_set_key_value(ht, "a", 1, "aaa") == NULL) {
        printf("hash_set_key_value(a) fail\n");
        return -1;
    }
    if (ht->used != 1) {
        printf("ht->used:%d != 1\n", ht->used);
        return -1;
    }

    if (hash_set_key_value(ht, "ab", 2, "ababab") == NULL) {
        printf("hash_set_key_value(ab) fail\n");
        return -1;
    }
    if (ht->used != 2) {
        printf("ht->used:%d != 2\n", ht->used);
        return -1;
    }

    if (hash_set_key_value(ht, "a", 1, "aaaaaaaaaa") == NULL) {
        printf("hash_set_key_value(abc) fail\n");
        return -1;
    }
    if (ht->used != 2) {
        printf("ht->used:%d != 2\n", ht->used);
        return -1;
    }

    if (hash_set_key_value(ht, "ccccc", 5, "cccccc") == NULL) {
        printf("hash_set_key_value fail\n");
        return -1;
    }

    if (hash_set_key_value(ht, "ddddddd", 7, "ddddddd") == NULL) {
        printf("hash_set_key_value fail\n");
        return -1;
    }    

    return 0;
}

int test_get(hashtable_t *ht)
{
    bucket_t * pb = hash_find_key(ht, "a", 1);
    if (!pb) {
        return -1;
    }
 
    if (strcmp(pb->val.sz, "aaaaaaaaaa") != 0) {
        printf("value:%s != %s\n", pb->val.sz, "aaaaaaaaaa");
        return -1;
    }

    return 0;
}

int test_remove(hashtable_t *ht)
{
    int res = hash_remove_key(ht, "a", 1);
    if (res < 0) {
        printf("hash_remove_key(a) fail\n");
        return res;
    }
    int e = hash_exists(ht, "a", 1);
    if (e) {
        printf("hash_exists(a)=%d\n", e);
        return -1;
    }
    return 0;
}

int test_resize(hashtable_t *ht)
{
    uint32_t i = 0, origcap = ht->cap;
    char sz[6] = {0};

    for (; i < origcap; ++i) {
        memset(sz, '0' + i, 5);
        if (!hash_set_key_value(ht, sz, 5, sz)) {
            printf("hash_set fail %d\n", i);
            return -1;
        }
    }

    if (ht->cap != 2 * origcap) {
        printf("ht->cap:%d != %d\n", ht->cap, 2 * origcap);
        return -1;
    }

    return 0;
}

void test_int()
{
    hashtable_t *ht = new_hash_table(0, NULL);

    char key[5] = {0};
    for (int idx = 0; idx < 1000; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!hash_set_key_value(ht, key, 4, (void *)(long)idx)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    for (int idx = 0; idx < 1000; idx += 100) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        bucket_t *pb = hash_find_key(ht, key, 4);
        if (pb) {
            printf("%s: %d\n", pb->key->str, (int)pb->val.lval);
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 0; idx < 400; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (hash_remove_key(ht, key, 4) != 0) {
            printf("remove failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 1000; idx < 1300; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!hash_set_key_value(ht, key, 4, (void *)(long)idx)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 1300; idx < 1500; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!hash_set_key_value(ht, key, 4, (void *)(long)idx)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 0; idx < 2000; idx += 100) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        bucket_t *pb = hash_find_key(ht, key, 4);
        if (pb) {
            printf("%s: %d\n", pb->key->str, (int)pb->val.lval);
        }
    }
    hash_free(ht);
}

void test_str()
{
    stringtab_t *ht = stringtab_new(0);

    char key[5] = {0};
    for (int idx = 0; idx < 1000; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!stringtab_set(ht, key, 4, key, 4)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    for (int idx = 0; idx < 1000; idx += 100) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        ef_string_t *str = stringtab_find(ht, key, 4);
        if (str) {
            printf("%d: %s\n", idx, str->str);
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 0; idx < 400; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (stringtab_remove(ht, key, 4) != 0) {
            printf("remove failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 1000; idx < 1300; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!stringtab_set(ht, key, 4, key, 4)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 1300; idx < 1500; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!stringtab_set(ht, key, 4, key, 4)) {
            printf("set failed: %d\n", idx);
            break;
        }
    }
    printf("cap: %d, used: %d\n", ht->cap, ht->used);
    for (int idx = 0; idx < 2000; idx += 100) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        ef_string_t *str = stringtab_find(ht, key, 4);
        if (str) {
            printf("%d: %s\n", idx, str->str);
        }
    }
    stringtab_free(ht);
}

int main(int argc,char* argv[])
{    
    int res;
    res = test_recap();
    if (res == 0) {
        printf("PASS hash_recap\n");
    }

    hashtable_t *ht = new_hash_table(0, NULL);
    res = test_set(ht);
    if (res == 0) {
        printf("PASS hash_set\n");
    }

    res = test_get(ht);
    if (res == 0) {
        printf("PASS hash_get\n");
    }
    res = test_remove(ht);
    if (res == 0) {
        printf("PASS hash_remove\n");
    }
    res = test_resize(ht);
    if (res == 0) {
        printf("PASS hash_resize\n");
    }
    hash_free(ht);

    test_int();
    test_str();
    return 0;
}