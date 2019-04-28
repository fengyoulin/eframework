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

#include "bufferio.h"
#include "../framework.h"
#include <errno.h>

static ssize_t ef_safe_read(int fd, unsigned char *buf, size_t len, size_t cap)
{
    ssize_t retval, offset = 0;

    if (cap < len) {
        cap = len;
    }

    while (offset < len) {

        errno = 0;
 
        retval = ef_wrap_read(fd, buf + offset, cap - offset);

        if (retval > 0) {
            offset += retval;
        } else if (retval == 0 && errno == 0) {
            break;
        } else if (retval <= 0 && errno != 0 && errno != EINTR) {
            return retval;
        }
    }

    return offset;
}

static ssize_t ef_safe_write(int fd, unsigned char *buf, size_t len)
{
    ssize_t retval, offset = 0;

    while (offset < len) {

        errno = 0;

        retval = ef_wrap_write(fd, buf + offset, len - offset);

        if (retval > 0) {
            offset += retval;
        } else if (retval <= 0 && errno != 0 && errno != EINTR) {
            return retval;
        }
    }

    return offset;
}

ef_bio_reader_t *ef_bio_reader_new(int fd, size_t buf_size)
{
    ef_bio_reader_t *reader = (ef_bio_reader_t *)malloc(sizeof(ef_bio_reader_t));
    if (!reader) {
        return NULL;
    }

    reader->buffer = ef_buffer_new(buf_size);
    if (!reader->buffer) {
        free(reader);
        return NULL;
    }
    reader->fd = fd;
    reader->offset = 0;

    return reader;
}

void ef_bio_reader_free(ef_bio_reader_t *reader)
{
    if (reader->buffer) {
        ef_buffer_free(reader->buffer, 1);
    }
    free(reader);
}

ssize_t ef_bio_read(ef_bio_reader_t *reader, unsigned char *buf, size_t len)
{
    ef_buffer_t *buffer = reader->buffer;
    ssize_t offset, reads, need;

    if (len <= buffer->len - reader->offset) {
        memcpy(buf, buffer->ptr + reader->offset, len);
        reader->offset += len;
        return len;
    }

    offset = buffer->len - reader->offset;
    if (offset > 0) {
        memcpy(buf, buffer->ptr + reader->offset, offset);
    }

    buffer->len = 0;
    reader->offset = 0;
    need = len - offset;

    if (need > (buffer->cap >> 2)) {
        return ef_safe_read(reader->fd, buf + offset, need, 0);
    }

    reads = ef_safe_read(reader->fd, buffer->ptr, need, buffer->cap);
    if (reads < 0) {
        return reads;
    }
    buffer->len = reads;

    if (need > reads) {
        need = reads;
    }

    memcpy(buf + offset, buffer->ptr, need);
    reader->offset = need;

    return need;
}

ssize_t ef_bio_reader_skip(ef_bio_reader_t *reader, size_t len)
{
    ef_buffer_t *buffer = reader->buffer;
    ssize_t reads, remain, need;

    if (len <= buffer->len - reader->offset) {
        reader->offset += len;
        return len;
    }

    remain = len + reader->offset - buffer->len;
    buffer->len = 0;
    reader->offset = 0;

    while (remain > 0) {
        need = (remain < buffer->cap) ? remain : buffer->cap;
        reads = ef_safe_read(reader->fd, buffer->ptr, need, need);
        if (reads < 0) {
            return reads;
        }
        remain -= reads;
    }

    return len;
}

ef_bio_writer_t *ef_bio_writer_new(int fd, size_t buf_size)
{
    ef_bio_writer_t *writer = (ef_bio_writer_t *)malloc(sizeof(ef_bio_writer_t));
    if (!writer) {
        return NULL;
    }

    writer->buffer = ef_buffer_new(buf_size);
    if (!writer->buffer) {
        free(writer);
        return NULL;
    }
    writer->fd = fd;

    return writer;
}

void ef_bio_writer_free(ef_bio_writer_t *writer)
{
    if (writer->buffer) {
        ef_buffer_free(writer->buffer, 1);
    }
    free(writer);
}

ssize_t ef_bio_write(ef_bio_writer_t *writer, unsigned char *buf, size_t len)
{
    if ((writer->buffer->len + len) > writer->buffer->cap || 
        (writer->buffer->cap >> 2) < len) {
        if (writer->buffer->len) {
            if (ef_safe_write(writer->fd, writer->buffer->ptr, writer->buffer->len) < 0) {
                return -1;
            }
            writer->buffer->len = 0;
        }
        return ef_safe_write(writer->fd, buf, len);
    }

    if (ef_buffer_append(writer->buffer, buf, len) < 0) {
        return -1;
    }

    return len;
}

ssize_t ef_bio_writer_flush(ef_bio_writer_t *writer)
{
    ssize_t written;

    if (!writer->buffer->len) {
        return 0;
    }

    written = ef_safe_write(writer->fd, writer->buffer->ptr, writer->buffer->len);
    if (written < 0) {
        return written;
    }
    writer->buffer->len = 0;
    return written;
}
