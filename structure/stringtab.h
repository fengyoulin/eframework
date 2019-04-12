#ifndef _STRINGTAB_HEADER_
#define _STRINGTAB_HEADER_

#include "hashtable.h"

typedef hashtable_t stringtab_t;

stringtab_t *stringtab_new(uint32_t cap);

ef_string_t *stringtab_find(stringtab_t *tb, const char *key, size_t len);

ef_string_t *stringtab_set(stringtab_t *tb, const char *key, size_t klen, const char *val, size_t vlen);

#define stringtab_remove(t, k, l) hash_remove_key(t, k, l)

#define stringtab_free(t) hash_free(t)

#endif
