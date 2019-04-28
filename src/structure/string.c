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

#include "string.h"

ef_string_t *ef_string_new(const char *sz, size_t len)
{
    ef_string_t *str = (ef_string_t *)malloc(sizeof(ef_string_t));
    if (!str) {
        return NULL;
    }
    if (len == 0) {
        len = sz ? strlen(sz) : 0;
    }
    str->str = (char *)malloc(len + 1);
    if (!str->str) {
        free(str);
        return NULL;
    }
    if (len) {
        memcpy(str->str, sz, len);
    }
    str->str[len] = 0;
    str->len = len;
    return str;
}

int ef_string_append(ef_string_t *str, const char *sz, size_t len)
{
    char *ptr = NULL;
    if (len == 0) {
        len = sz ? strlen(sz) : 0;
    }
    if (len == 0) {
        return 0;
    }
    if (str->str) {
        ptr = (char *)realloc(str->str, str->len + len + 1);
    } else {
        ptr = (char *)malloc(len + 1);
    }
    if (!ptr) {
        return -1;
    }
    memcpy(ptr + str->len, sz, len);
    str->len += len;
    ptr[str->len] = 0;
    str->str = ptr;
    return 0;
}

void ef_string_free(ef_string_t *str, int destroy)
{
    free(str->str);
    if (destroy) {
        free(str);
    } else {
        str->str = NULL;
        str->len = 0;
    }
}
