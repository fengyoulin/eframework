#ifndef _HEADERTAB_HEADER_
#define _HEADERTAB_HEADER_

#include "hashtable.h"
#include "vector.h"

typedef hashtable_t headertab_t;

headertab_t *headertab_new(uint32_t cap);

ef_vector_t *headertab_find(headertab_t *tb, const char *key, size_t len);

ef_string_t *headertab_set(headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen);

ef_string_t *headertab_add(headertab_t *tb, const char *key, size_t klen, const char *val, size_t vlen);

#define headertab_remove(t, k, l) hash_remove_key(t, k, l)

#define headertab_free(t) hash_free(t)

#endif // _HEADERTAB_HEADER_
