#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 6379
#define DEFAULT_HOST "127.0.0.1"
#define RES_BUFSIZE 1024

#define REPLY_TYPE_ERR (1<<0)
#define REPLY_TYPE_STR (1<<1)
#define REPLY_TYPE_LONG (1<<2)
#define REPLY_TYPE_ARR (1<<3)
#define REPLY_TYPE_NULL (1<<4)

typedef struct ef_redis_connection_t {
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
} ef_redis_connection;

typedef struct reply_string_t {
	char *buf;
	int len;
} reply_string;
typedef struct reply_error_t {
	char *type;
 	char *err;
} reply_error;
typedef struct reply_array_t {
	size_t num;
	struct ef_redis_reply_t **elem;
} reply_array;

typedef struct ef_redis_reply_t {
    uint type;
    union {
    	reply_string *str;
    	long d;
    	reply_array *arr;
    	reply_error *err;
    } reply;
} ef_redis_reply;

ef_redis_connection * ef_redis_connect(char *host,int port);
int ef_redis_close(ef_redis_connection *con);

int ef_redis_free_reply(ef_redis_reply *rep);
int free_reply_long(ef_redis_reply *rep);
int free_reply_str(ef_redis_reply *rep);
int free_reply_arr(ef_redis_reply *rep);
int free_reply_err(ef_redis_reply *rep);

int ef_redis_cmd(ef_redis_connection * con,const char *fmt,...);
ef_redis_reply * ef_redis_get(ef_redis_connection *con,const char *key);

int reply_read_more(ef_redis_connection *con);
ef_redis_reply* ef_redis_read_reply(ef_redis_connection *con);
ef_redis_reply* parse_long(char *buf,ef_redis_connection *con);
ef_redis_reply* parse_single_string(char *buf,ef_redis_connection *con);
ef_redis_reply* parse_bulk_string(char *buf,ef_redis_connection *con);
ef_redis_reply* parse_string(char *buf,size_t strlen,ef_redis_connection *con);
ef_redis_reply * parse_array(char *buf,ef_redis_connection *con);
ef_redis_reply* parse_error(char *buf,ef_redis_connection *con);
int parse_len(char *res);