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

#include "stringtab.h"

static void str_dtor(void *ptr)
{
    ef_string_free((ef_string_t *)ptr, 1);
}

ef_stringtab_t *ef_stringtab_new(uint32_t cap)
{
    return ef_hashtable_new(cap, str_dtor);
}

ef_string_t *ef_stringtab_find(ef_stringtab_t *tb, const char *key, size_t len)
{
    ef_bucket_t *pb = ef_hashtable_find_key(tb, key, len);
    if (!pb) {
        return NULL;
    }
    return pb->val.str;
}

ef_string_t *ef_stringtab_set(ef_stringtab_t *tb, const char *key, size_t klen, const char *val, size_t vlen)
{
    ef_string_t *str = NULL;
    if (val) {
        str = ef_string_new(val, vlen);
    }
    if (!str && val) {
        return NULL;
    }
    ef_bucket_t *pb = ef_hashtable_set_key_value(tb, key, klen, str);
    if (!pb) {
        ef_string_free(str, 1);
        return NULL;
    }
    return pb->val.str;
}

