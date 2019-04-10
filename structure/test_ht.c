#include <stdio.h>
#include "hashtable.h"

void test_int()
{
    hashtable_t *ht = new_hash_table(0, NULL);

    char key[5] = {0};
    for (int idx = 0; idx < 1000; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!hash_set_key_value(ht, key, 4, (void *)idx)) {
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
        if (!hash_set_key_value(ht, key, 4, (void *)idx)) {
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
        if (!hash_set_key_value(ht, key, 4, (void *)idx)) {
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

void str_dtor(void *ptr)
{
    ef_string_free((ef_string_t *)ptr, 1);
}

void test_str()
{
    hashtable_t *ht = new_hash_table(0, str_dtor);

    char key[5] = {0};
    for (int idx = 0; idx < 1000; ++idx) {
        key[3] = '0' + idx % 10;
        key[2] = '0' + (idx / 10) % 10;
        key[1] = '0' + (idx / 100) % 10;
        key[0] = '0' + idx / 1000;
        if (!hash_set_key_value(ht, key, 4, ef_string_new(key, 4))) {
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
            printf("%s: %s\n", pb->key->str, pb->val.str->str);
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
        if (!hash_set_key_value(ht, key, 4, ef_string_new(key, 4))) {
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
        if (!hash_set_key_value(ht, key, 4, ef_string_new(key, 4))) {
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
            printf("%s: %s\n", pb->key->str, pb->val.str->str);
        }
    }
    hash_free(ht);
}

int main(int argc, char *argv[])
{
    test_int();
    test_str();
    return 0;
}

