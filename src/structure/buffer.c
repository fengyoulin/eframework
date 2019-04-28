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

#include "buffer.h"
#include "util.h"

ef_buffer_t *ef_buffer_new(size_t cap)
{
    ef_buffer_t *buf = (ef_buffer_t *)malloc(sizeof(ef_buffer_t));
    if (!buf) {
        return NULL;
    }
    if (cap) {
        cap = ef_resize(cap, 0);
    }
    buf->ptr = cap ? (unsigned char*)malloc(cap) : NULL;
    if (cap && !buf->ptr) {
        free(buf);
        return NULL;
    }
    buf->cap = cap;
    buf->len = 0;
    return buf;
}

int ef_buffer_expand(ef_buffer_t *buf, size_t len)
{
    size_t cap;
    unsigned char *ptr = NULL;
    if (buf->cap - buf->len >= len) {
        return 0;
    }
    cap = ef_resize(buf->len + len, 0);
    if (buf->ptr) {
        ptr = (unsigned char*)realloc(buf->ptr, cap);
    } else {
        ptr = (unsigned char*)malloc(cap);
    }
    if (ptr) {
        buf->ptr = ptr;
        buf->cap = cap;
        return 0;
    }
    return -1;
}

int ef_buffer_append(ef_buffer_t *buf, void *data, size_t len)
{
copy_data:
    if (buf->cap - buf->len >= len) {
        memcpy(buf->ptr + buf->len, data, len);
        buf->len += len;
        return 0;
    }
    if (ef_buffer_expand(buf, len) < 0) {
        return -1;
    } else {
        goto copy_data;
    }
    return -1;
}

void ef_buffer_free(ef_buffer_t *buf, int destroy)
{
    free(buf->ptr);
    if (destroy) {
        free(buf);
    } else {
        buf->ptr = NULL;
        buf->cap = 0;
        buf->len = 0;
    }
}
