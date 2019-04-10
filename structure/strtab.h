#ifndef _STRTAB_HEADER_
#define _STRTAB_HEADER_

#include "hashtable.h"

typedef hashtable_t strtab_t;

strtab_t *strtab_new(uint32_t cap);

ef_string_t *strtab_find(strtab_t *tb, const char *key, size_t len);

ef_string_t *strtab_set(strtab_t *tb, const char *key, size_t klen, const char *val, size_t vlen);

#define strtab_remove(t, k, l) hash_remove_key(t, k, l)

#define strtab_free(t) hash_free(t)

#endif
