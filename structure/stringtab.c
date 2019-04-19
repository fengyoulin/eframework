#include "stringtab.h"

static void str_dtor(void *ptr)
{
    ef_string_free((ef_string_t *)ptr, 1);
}

stringtab_t *stringtab_new(uint32_t cap)
{
    return new_hash_table(cap, str_dtor);
}

ef_string_t *stringtab_find(stringtab_t *tb, const char *key, size_t len)
{
    bucket_t *pb = hash_find_key(tb, key, len);
    if (!pb) {
        return NULL;
    }
    return pb->val.str;
}

ef_string_t *stringtab_set(stringtab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = NULL;
    if (val) {
        str = ef_string_new(val, vlen);
    }
    if (!str && val) {
        return NULL;
    }
    bucket_t *pb = hash_set_key_value(tb, key, klen, str);
    if (!pb) {
        ef_string_free(str, 1);
        return NULL;
    }
    return pb->val.str;
}

