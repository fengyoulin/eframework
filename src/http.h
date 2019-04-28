#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "basic/basic.h"
#include "structure/stringtab.h"

#define MAX_RESP_BUFSIZE 4096
#define MAX_HEADER_BUFSIZE 1024

#define DEFAULT_HTTP_PORT 80
#define DEFAULT_HTTP_HOST ("127.0.0.1")
#define CURL_METHOD_GET ("GET")
#define CURL_METHOD_POST ("POST")

#define HTTP_PROTOCOL_0 ("HTTP/1.0")
#define HTTP_PROTOCOL_1 ("HTTP/1.1")

#define HTTP_STATUS_OK (200)

#define R_STATUS_INIT (1<<0)
#define R_STATUS_HEADER (1<<1)
#define R_STATUS_BODY (1<<2)
#define R_STATUS_DONE (1<<3)

typedef struct ef_curl_request_t {
    char *method;
    unsigned short status;
    const char *protocol;
    char *request_uri;
    ef_stringtab_t *headers;
    ef_stringtab_t *cookies;
    ef_stringtab_t *request_body;
} ef_curl_request;

typedef struct ef_curl_response_t {
    unsigned short status;
    unsigned short resp_status;
    long content_length;
    long content_received;
    char *bufend;
    char *bufsend;
    long chunk_remain;
    ef_stringtab_t *headers;
    char buf[MAX_RESP_BUFSIZE];
} ef_curl_response;

typedef struct ef_curl_t {
    int fd;
    ef_curl_request *req;
    ef_curl_response *resp;
} ef_curl;

ef_curl *ef_curl_init(char *host,int port,char *request_uri);
int ef_curl_set_header(ef_curl *cu,char *header,char *content);
int ef_curl_set_cookie(ef_curl *cu,char *name,char *val,long expired,char *path,char *domain,char httponly,char security);
int ef_curl_set_request_body(ef_curl *cu,char *key,char *val);
int ef_curl_exec(ef_curl *cu);
int ef_curl_read_response(ef_curl *cu,char *buf,size_t len);

int http_parse_resp_chunked(ef_curl *cu,char *buf,size_t len);
int http_buffer_resp_body(int fd,ef_curl_response *resp);
int http_parse_resp_headers(ef_curl *cu);
int http_parse_resp_header(ef_curl_response *resp,char *start,char *bufend);
int http_parse_resp_line(ef_curl_response *resp,char *buf);

int http_build_request_body(ef_curl_request *req,char *buf,size_t cap);
int http_build_request_header(ef_curl_request *req,char *buf,size_t cap);
int http_build_request_line(ef_curl_request *req,char *buf,size_t cap);

void ef_curl_close(ef_curl *cu);
void ef_curl_free_request(ef_curl_request *req);
void ef_curl_free_response(ef_curl_response *resp);
