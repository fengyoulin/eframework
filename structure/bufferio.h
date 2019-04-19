#ifndef _BUFFER_IO_HEADER_
#define _BUFFER_IO_HEADER_

#include "buffer.h"

typedef struct _ef_bio_reader {
    int fd;
    size_t offset;
    ef_buffer_t *buffer;
} ef_bio_reader_t;

ef_bio_reader_t *ef_bio_reader_new(int fd, size_t buf_size);

void ef_bio_reader_free(ef_bio_reader_t *reader);

size_t ef_bio_read(ef_bio_reader_t *reader, unsigned char *buf, size_t len);

typedef struct _ef_bio_writer {
    int fd;
    ef_buffer_t *buffer;
} ef_bio_writer_t;

ef_bio_writer_t *ef_bio_writer_new(int fd, size_t buf_size);

void ef_bio_writer_free(ef_bio_writer_t *writer);

size_t ef_bio_write(ef_bio_writer_t *writer, unsigned char *buf, size_t len);

size_t ef_bio_writer_flush(ef_bio_writer_t *writer);

#endif