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

#ifndef _STRINGTAB_HEADER_
#define _STRINGTAB_HEADER_

#include "hashtable.h"

typedef ef_hashtable_t ef_stringtab_t;

ef_stringtab_t *ef_stringtab_new(uint32_t cap);

ef_string_t *ef_stringtab_find(ef_stringtab_t *tb, const char *key, size_t len);

ef_string_t *ef_stringtab_set(ef_stringtab_t *tb, const char *key, size_t klen, const char *val, size_t vlen);

#define ef_stringtab_remove(t, k, l) ef_hashtable_remove_key(t, k, l)

#define ef_stringtab_free(t) ef_hashtable_free(t)

#endif
