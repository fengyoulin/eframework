#ifndef _FASTCGI_HEADER_
#define _FASTCGI_HEADER_

#include <stdint.h>
#include "structure/stringtab.h"
#include "structure/headertab.h"
#include "structure/buffer.h"

#define MAKE_UINT16(x,y) ((unsigned short)x | ((unsigned short)y << 8))

typedef enum _fcgi_request_type {
    FCGI_BEGIN_REQUEST      =  1, /* [in]                              */
    FCGI_ABORT_REQUEST      =  2, /* [in]  (not supported)             */
    FCGI_END_REQUEST        =  3, /* [out]                             */
    FCGI_PARAMS             =  4, /* [in]  environment variables       */
    FCGI_STDIN              =  5, /* [in]  post data                   */
    FCGI_STDOUT             =  6, /* [out] response                    */
    FCGI_STDERR             =  7, /* [out] errors                      */
    FCGI_DATA               =  8, /* [in]  filter data (not supported) */
    FCGI_GET_VALUES         =  9, /* [in]                              */
    FCGI_GET_VALUES_RESULT  = 10  /* [out]                             */
} fcgi_request_type_t;

typedef struct _fcgi_header {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} fcgi_header_t;

typedef enum _fcgi_role {
    FCGI_RESPONDER  = 1,
    FCGI_AUTHORIZER = 2,
    FCGI_FILTER = 3
} fcgi_role_t;

#define FCGI_KEEP_CONN 1

typedef struct _fcgi_begin_request {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} fcgi_begin_request_t;

typedef enum _fcgi_protocol_status {
    FCGI_REQUEST_COMPLETE   = 0,
    FCGI_CANT_MPX_CONN      = 1,
    FCGI_OVERLOADED         = 2,
    FCGI_UNKNOWN_ROLE       = 3
} fcgi_protocol_status_t;

typedef struct _fcgi_end_request {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} fcgi_end_request_t;

typedef struct _fcgi_request {
    uint16_t request_id;
    uint16_t role;
    uint8_t flags;
    stringtab_t *params;
    ef_buffer_t *data;
} fcgi_request_t;

typedef struct _fcgi_response {
    fcgi_request_t *request;
    headertab_t *headers;
    ef_buffer_t *data;
} fcgi_response_t;

fcgi_request_t *fcgi_read_request(int sockfd);

int fcgi_write_response(int sockfd, fcgi_response_t *response, int free);

fcgi_response_t *fcgi_new_response(fcgi_request_t *request);

void fcgi_free_request(fcgi_request_t *request);

void fcgi_free_response(fcgi_response_t *response, int free_request);

#endif