#include "fastcgi.h"
#include "structure/bufferio.h"

#define BUFFER_SIZE 8192

static int fcgi_parse_params(stringtab_t *tbl, unsigned char *data, size_t len)
{
    unsigned char *ptr = data, *name, *value;
    size_t name_len, value_len;

    while (ptr - data < len) {
        name_len = *ptr++;
        if (name_len >= 128) {
            name_len = ((name_len & 0x7f) << 24);
            name_len |= (*ptr++ << 16);
            name_len |= (*ptr++ << 8);
            name_len |= *ptr++;
        }
        value_len = *ptr++;
        if (value_len >= 128) {
            value_len = ((value_len & 0x7f) << 24);
            value_len |= (*ptr++ << 16);
            value_len |= (*ptr++ << 8);
            value_len |= *ptr++;
        }
        name = ptr;
        ptr += name_len;
        value = ptr;
        ptr += value_len;
        stringtab_set(tbl, name, name_len, value_len ? value : NULL, value_len);
    }
    return 0;
}

fcgi_request_t *fcgi_read_request(int sockfd)
{
    ef_buffer_t *buffer;
    ef_bio_reader_t *reader;
    fcgi_request_t *request;
    fcgi_header_t fcgi_hdr;
    fcgi_begin_request_t bg_req;
    size_t reads, length;

    reader = ef_bio_reader_new(sockfd, BUFFER_SIZE);
    if (!reader) {
        return NULL;
    }

    // FCGI_BEGIN_REQUEST
    reads = ef_bio_read(reader, (unsigned char *)&fcgi_hdr, sizeof(fcgi_header_t));
    if (reads < sizeof(fcgi_header_t)) {
        goto exit_read;
    }

    if (fcgi_hdr.version != 1 || fcgi_hdr.type != FCGI_BEGIN_REQUEST) {
        goto exit_read;
    }

    if (MAKE_UINT16(fcgi_hdr.contentLengthB0, fcgi_hdr.contentLengthB1) != sizeof(fcgi_begin_request_t)) {
        goto exit_read;
    }

    reads = ef_bio_read(reader, (unsigned char *)&bg_req, sizeof(fcgi_begin_request_t));
    if (reads < sizeof(fcgi_begin_request_t)) {
        goto exit_read;
    }

    request = (fcgi_request_t *)malloc(sizeof(fcgi_request_t));
    if (!request) {
        goto exit_read;
    }

    request->request_id = MAKE_UINT16(fcgi_hdr.requestIdB0, fcgi_hdr.requestIdB1);
    request->role = MAKE_UINT16(bg_req.roleB0, bg_req.roleB1);
    request->flags = bg_req.flags;
    request->params = stringtab_new(0);
    request->data = NULL;

    // FCGI_PARAMS
    while (1) {
        reads = ef_bio_read(reader, (unsigned char *)&fcgi_hdr, sizeof(fcgi_header_t));
        if (reads < sizeof(fcgi_header_t)) {
            goto exit_request;
        }

        if (fcgi_hdr.version != 1 || fcgi_hdr.type != FCGI_PARAMS) {
            break;
        }

        size_t length = MAKE_UINT16(fcgi_hdr.contentLengthB0, fcgi_hdr.contentLengthB1);
        if (!length) {
            break;
        }

        if (!buffer) {
            buffer = ef_buffer_new(length);
        } else {
            if (ef_buffer_expand(buffer, length) < 0) {
                goto exit_request;
            }
        }
        if (!buffer) {
            goto exit_request;
        }

        reads = ef_bio_read(reader, buffer->ptr + buffer->len, length);
        buffer->len += reads;
        if (reads < length) {
            ef_buffer_free(buffer, 1);
            goto exit_request;
        }
    }
    if (buffer) {
        fcgi_parse_params(request->params, buffer->ptr, buffer->len);
        ef_buffer_free(buffer, 1);
    }

    // FCGI_STDIN
    while (1) {
        reads = ef_bio_read(reader, (unsigned char *)&fcgi_hdr, sizeof(fcgi_header_t));
        if (reads < sizeof(fcgi_header_t)) {
            goto exit_request;
        }

        if (fcgi_hdr.version != 1 || fcgi_hdr.type != FCGI_BEGIN_REQUEST) {
            break;
        }

        length = MAKE_UINT16(fcgi_hdr.contentLengthB0, fcgi_hdr.contentLengthB1);
        if (!length) {
            break;
        }

        if (!request->data) {
            request->data = ef_buffer_new(length);
        } else {
            if (ef_buffer_expand(request->data, length) < 0) {
                goto exit_request;
            }
        }
        if (!request->data) {
            goto exit_request;
        }

        reads = ef_bio_read(reader, request->data->ptr + request->data->len, length);
        request->data->len = reads;
        if (reads < length) {
            goto exit_request;
        }
    }

    goto exit_read;

exit_request:
    if (request) {
        fcgi_free_request(request);
        request = NULL;
    }

exit_read:
    if (reader) {
        ef_bio_reader_free(reader);
    }
    return request;
}

void fcgi_free_request(fcgi_request_t *request)
{
    if (request->params) {
        stringtab_free(request->params);
    }
    if (request->data) {
        ef_buffer_free(request->data, 1);
    }
    free(request);
}

fcgi_response_t *fcgi_new_response(fcgi_request_t *request)
{
    fcgi_response_t *response = (fcgi_response_t *)malloc(sizeof(fcgi_response_t));
    if (!response) {
        return NULL;
    }
    response->request = request;
    response->data = ef_buffer_new(0);
    if (!response->data) {
        free(response);
        return NULL;
    }
    response->headers = headertab_new(0);
    if (!response->headers) {
        ef_buffer_free(response->data, 1);
        free(response);
        return NULL;
    }
    return response;
}

void fcgi_free_response(fcgi_response_t *response, int free_request)
{
    if (response->data) {
        ef_buffer_free(response->data, 1);
    }
    if (response->headers) {
        stringtab_free(response->headers);
    }
    if (free_request && response->request) {
        fcgi_free_request(response->request);
    }
    free(response);
}

int fcgi_write_response(int sockfd, fcgi_response_t *response, int free)
{
    char buf[8];
    ef_buffer_t *bufs[3], **pbuf;
    size_t length, offset = 0;
    fcgi_header_t fcgi_hdr;
    fcgi_end_request_t end_req = {0};

    bufs[0] = ef_buffer_new(BUFFER_SIZE);
    if (!bufs[0]) {
        return -1;
    }
    bufs[1] = response->data;
    bufs[2] = NULL;

    if (response->headers) {
        for(uint32_t idx = 0; idx < response->headers->cap; ++idx) {
            bucket_t *pb = &response->headers->arrData[idx];
            if (!pb->key) {
                continue;
            }
            ef_vector_t *vec = (ef_vector_t *)pb->val.ptr;
            if (!vec) {
                continue;
            }
            for(size_t i = 0; i < vec->len; ++i) {
                ef_string_t *str = (ef_string_t *)vec->ptr[i];
                ef_buffer_append(bufs[0], pb->key->str, pb->key->len);
                ef_buffer_append(bufs[0], ": ", 2);
                ef_buffer_append(bufs[0], str->str, str->len);
                ef_buffer_append(bufs[0], "\r\n", 2);
            }
        }
        ef_buffer_append(bufs[0], "\r\n", 2);
        length = bufs[0]->len;
    }

    if (response->data) {
        length += response->data->len;
    }

    ef_bio_writer_t *writer = ef_bio_writer_new(sockfd, BUFFER_SIZE);
    if (!writer) {
        return -1;
    }

    fcgi_hdr.version = 1;
    fcgi_hdr.type = FCGI_STDOUT;
    fcgi_hdr.requestIdB1 = (uint8_t)(response->request->request_id >> 8);
    fcgi_hdr.requestIdB0 = (uint8_t)(response->request->request_id & 0xff);
    fcgi_hdr.reserved = 0;

    pbuf = &bufs[0];
    while (length) {
        ef_buffer_t *buffer;
        size_t wtsz, pksz = (length <= 65535) ? length : 65535;
        length -= pksz;

        fcgi_hdr.contentLengthB1 = (uint8_t)(pksz >> 8);
        fcgi_hdr.contentLengthB0 = (uint8_t)(pksz & 0xff);
        fcgi_hdr.paddingLength = (8 - pksz % 8) & 7;
        ef_bio_write(writer, (unsigned char *)&fcgi_hdr, sizeof(fcgi_hdr));

next_buf:
        buffer = *pbuf;
        if (!buffer) {
            break;
        }
        wtsz = buffer->len - offset;
        if (wtsz > pksz) {
            wtsz = pksz;
        }
        ef_bio_write(writer, buffer->ptr + offset, wtsz);
        offset += wtsz;
        pksz -= wtsz;
        if (offset == buffer->len) {
            offset = 0;
            ++pbuf;
        }
        if (pksz) {
            goto next_buf;
        } else if (fcgi_hdr.paddingLength) {
            memset(buf, 0, fcgi_hdr.paddingLength);
            ef_bio_write(writer, buf, fcgi_hdr.paddingLength);
        }
    }
    ef_buffer_free(bufs[0], 1);

    fcgi_hdr.type = FCGI_END_REQUEST;
    fcgi_hdr.contentLengthB1 = 0;
    fcgi_hdr.contentLengthB0 = sizeof(end_req);
    fcgi_hdr.paddingLength = 0;
    ef_bio_write(writer, (unsigned char *)&fcgi_hdr, sizeof(fcgi_hdr));
    ef_bio_write(writer, (unsigned char *)&end_req, sizeof(end_req));
    ef_bio_writer_flush(writer);
    ef_bio_writer_free(writer);
    if (free) {
        fcgi_free_response(response, 1);
    }
    return 0;
}
