#include "headertab.h"

static void str_dtor(void *ptr)
{
    ef_string_free((ef_string_t *)ptr, 1);
}

static void vec_dtor(void *ptr)
{
    ef_vector_free((ef_vector_t *)ptr, 1);
}

headertab_t *headertab_new(uint32_t cap)
{
    return new_hash_table(cap, vec_dtor);
}

ef_vector_t *headertab_find(headertab_t *tb, const char *key, size_t len)
{
    bucket_t *pb = hash_find_key(tb, key, len);
    if (!pb) {
        return NULL;
    }
    return (ef_vector_t *)pb->val.ptr;
}

ef_string_t *headertab_set(headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = ef_string_new(val, vlen);
    if (!str) {
        return NULL;
    }
    ef_vector_t *vec = headertab_find(tb, key, klen);
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
    bucket_t *pb = hash_set_key_value(tb, key, klen, vec);
    if (!pb) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    return (ef_string_t *)vec->ptr[idx];
}

ef_string_t *headertab_add(headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = ef_string_new(val, vlen);
    if (!str) {
        return NULL;
    }
    ef_vector_t *vec = headertab_find(tb, key, klen);
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
    bucket_t *pb = hash_set_key_value(tb, key, klen, vec);
    if (!pb) {
        ef_vector_free(vec, 1);
        return NULL;
    }
    return (ef_string_t *)vec->ptr[idx];
}

