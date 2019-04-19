#include "bufferio.h"
#include "../framework.h"

static size_t ef_must_read(int fd, unsigned char *buf, size_t len, size_t cap)
{
    size_t offset = 0;
    ssize_t reads = 0;

    if (cap < len) {
        cap = len;
    }

    while (offset < len) {
        reads = ef_wrap_read(fd, buf + offset, cap - offset);
        if (reads <= 0) {
            break;
        }
        offset += reads;
    }
    return offset;
}

static size_t ef_must_write(int fd, unsigned char *buf, size_t len)
{
    size_t offset = 0;
    ssize_t written = 0;

    while (offset < len) {
        written = ef_wrap_write(fd, buf + offset, len - offset);
        if (written <= 0) {
            break;
        }
        offset += written;
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

size_t ef_bio_read(ef_bio_reader_t *reader, unsigned char *buf, size_t len)
{
    ef_buffer_t *buffer = reader->buffer;
    size_t offset, reads, need;

    if (len < buffer->len - reader->offset) {
        memcpy(buf, buffer->ptr + reader->offset, len);
        reader->offset += len;
        return len;
    }

    offset = buffer->len - reader->offset;
    if (offset > 0) {
        memcpy(buf, buffer->ptr + reader->offset, offset);
    }

    reader->offset = 0;
    buffer->len = 0;
    need = len - offset;

    if (need > (buffer->cap >> 2)) {
        return ef_must_read(reader->fd, buf + offset, need, 0);
    }

    reads = ef_must_read(reader->fd, buffer->ptr, need, buffer->cap);
    buffer->len = reads;

    if (need > reads) {
        need = reads;
    }

    memcpy(buf + offset, buffer->ptr, need);
    reader->offset = need;

    return need;
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

size_t ef_bio_write(ef_bio_writer_t *writer, unsigned char *buf, size_t len)
{
    size_t written;
    if (writer->buffer->len + len > writer->buffer->cap || len > (writer->buffer->cap >> 2)) {
        if (writer->buffer->len) {
            written = ef_must_write(writer->fd, writer->buffer->ptr, writer->buffer->len);
            writer->buffer->len -= written;
            if (writer->buffer->len) {
                return 0;
            }
        }
        if (len) {
            return ef_must_write(writer->fd, buf, len);
        }
    }
    if (ef_buffer_append(writer->buffer, buf, len)) {
        return 0;
    }
    return len;
}

size_t ef_bio_writer_flush(ef_bio_writer_t *writer)
{
    size_t written;
    if (!writer->buffer->len) {
        return 0;
    }
    written = ef_must_write(writer->fd, writer->buffer->ptr, writer->buffer->len);
    writer->buffer->len -= written;
    return written;
}
