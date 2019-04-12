#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 6379
#define DEFAULT_HOST "127.0.0.1"
#define RES_BUFSIZE  1024

#define REPLY_TYPE_ERR  (1<<0)
#define REPLY_TYPE_STR  (1<<1)
#define REPLY_TYPE_LONG (1<<2)
#define REPLY_TYPE_ARR  (1<<3)
#define REPLY_TYPE_NULL (1<<4)

typedef struct _ef_redis_connection ef_redis_connection_t;
typedef struct _ef_redis_reply ef_redis_reply_t;
typedef struct _reply_string reply_string_t;
typedef struct _reply_error reply_error_t;
typedef struct _reply_array reply_array_t;

struct _ef_redis_connection {
    int port;
    char* host;
    int sockfd;
    // redis reply buffer
    char *buf;
    size_t buflen;
    // redis reply parse seek
    size_t seek;
    // redis reply buffer used
    size_t end;
};

struct _ef_redis_reply {
    uint type;
    union {
        long d;
        reply_string_t *str;
        reply_array_t *arr;
        reply_error_t *err;
    } reply;
};

struct _reply_string {
    char *buf;
    int len;
};

struct _reply_error {
    char *type;
    char *err;
};

struct _reply_array {
    size_t num;
    ef_redis_reply_t **elem;
};

ef_redis_connection_t * ef_redis_connect(char *host, int port);

int ef_redis_close(ef_redis_connection_t *con);

int ef_redis_free_reply(ef_redis_reply_t *rep);

int ef_redis_cmd(ef_redis_connection_t * con, const char *fmt, ...);

ef_redis_reply_t * ef_redis_read_reply(ef_redis_connection_t *con);

ef_redis_reply_t * ef_redis_get(ef_redis_connection_t *con, const char *key);
