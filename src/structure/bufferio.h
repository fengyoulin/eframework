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

#ifndef _BUFFER_IO_HEADER_
#define _BUFFER_IO_HEADER_

#include "buffer.h"

typedef struct _ef_bio_reader {
    int fd;
    size_t offset;
    ef_buffer_t *buffer;
} ef_bio_reader_t;

typedef struct _ef_bio_writer {
    int fd;
    ef_buffer_t *buffer;
} ef_bio_writer_t;

ef_bio_reader_t *ef_bio_reader_new(int fd, size_t buf_size);

void ef_bio_reader_free(ef_bio_reader_t *reader);

size_t ef_bio_read(ef_bio_reader_t *reader, unsigned char *buf, size_t len);

ef_bio_writer_t *ef_bio_writer_new(int fd, size_t buf_size);

void ef_bio_writer_free(ef_bio_writer_t *writer);

size_t ef_bio_write(ef_bio_writer_t *writer, unsigned char *buf, size_t len);

size_t ef_bio_writer_flush(ef_bio_writer_t *writer);

#endif