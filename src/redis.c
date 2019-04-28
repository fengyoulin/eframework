#include "framework.h"
#include "redis.h"

static int free_reply_long(ef_redis_reply_t *rep);
static int free_reply_str(ef_redis_reply_t *rep);
static int free_reply_arr(ef_redis_reply_t *rep);
static int free_reply_err(ef_redis_reply_t *rep);

static ef_redis_reply_t * parse_long(char *buf, ef_redis_connection_t *con);
static ef_redis_reply_t * parse_single_string(char *buf, ef_redis_connection_t *con);
static ef_redis_reply_t * parse_bulk_string(char *buf, ef_redis_connection_t *con);
static ef_redis_reply_t * parse_string(char *buf, size_t strlen, ef_redis_connection_t *con);
static ef_redis_reply_t * parse_array(char *buf, ef_redis_connection_t *con);
static ef_redis_reply_t * parse_error(char *buf, ef_redis_connection_t *con);

static int reply_read_more(ef_redis_connection_t *con);
static int parse_len(char *res);

ef_redis_connection_t * ef_redis_connect(char *host, int port)
{
    ef_redis_connection_t *con = (ef_redis_connection_t*)malloc(sizeof(ef_redis_connection_t));
    con->host = host;
    con->port = port;
    con->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    con->buf = (char*)malloc(RES_BUFSIZE);
    con->seek = 0;
    if (con->buf == NULL) {
        printf("res buffer error\n");
        return NULL;
    }
    if (con->host == NULL) {
        con->host = DEFAULT_HOST;
    }
    if (con->port == 0) {
        con->port = DEFAULT_PORT;
    }
    struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(con->port);
    addr_in.sin_addr.s_addr = inet_addr(con->host);
    int res = ef_wrap_connect(con->sockfd, (const struct sockaddr*)&addr_in, sizeof(addr_in));
    if (res < 0) {
        printf("connect fail:%d\n", res);
        return NULL;
    }
    return con;
}

int ef_redis_close(ef_redis_connection_t *con)
{
    close(con->sockfd);
    free(con->buf);
    return 0;
}

int ef_redis_free_reply(ef_redis_reply_t *rep)
{
    if (rep == NULL) {
        return 0;
    }
    if (rep->type & REPLY_TYPE_NULL) {
        free(rep);
        return 0;
    }
    if (rep->type & REPLY_TYPE_LONG) {
        return free_reply_long(rep);
    }
    if (rep->type & REPLY_TYPE_STR) {
        return free_reply_str(rep);
    }
    if (rep->type & REPLY_TYPE_ERR) {
        return free_reply_err(rep);
    }
    if (rep->type & REPLY_TYPE_ARR) {
        return free_reply_arr(rep);
    }
    return -1;
}

static int free_reply_long(ef_redis_reply_t *rep)
{
    free(rep);
    return 0;
}

static int free_reply_str(ef_redis_reply_t *rep)
{
    if (rep->reply.str != NULL) {
        if (rep->reply.str->buf != NULL) {
            free(rep->reply.str->buf);
        }
        free(rep->reply.str);
    }
    free(rep);
    return 0;
}

static int free_reply_err(ef_redis_reply_t *rep)
{
    if (rep->reply.err != NULL) {
        if (rep->reply.err->type != NULL) {
            free(rep->reply.err->type);
        }
        if (rep->reply.err->err != NULL) {
            free(rep->reply.err->err);
        }
        free(rep->reply.err);
    }
    free(rep);
    return 0;
}

static int free_reply_arr(ef_redis_reply_t *rep)
{
    if (rep->reply.arr != NULL) {
        int i;
        for (i = 0; i < rep->reply.arr->num; ++i) {
            ef_redis_free_reply(*(rep->reply.arr->elem+i));
        }
        free(rep->reply.arr->elem);
        free(rep->reply.arr);
    }
    free(rep);
    return 0;
}

int ef_redis_cmd(ef_redis_connection_t * con, const char *fmt, ...)
{
    char cmd[1024];
    va_list argp;
    va_start(argp, fmt);
    int cmdlen = vsprintf(cmd, fmt, argp);
    va_end(argp);

    if (cmdlen < 0) {
        printf("vsprintf error:%d\n", cmdlen);
        return cmdlen;
    }
    strcat(cmd, "\r\n");
    cmdlen += 2;

    int res = ef_wrap_write(con->sockfd, cmd, cmdlen);
    if (res < 0) {
        printf("send cmd fail:%d\n", res);
        return res;
    }
    return res;
}

ef_redis_reply_t * ef_redis_get(ef_redis_connection_t *con, const char *key)
{
    int res = ef_redis_cmd(con, "GET %s", key);
    if (res < 0) {
        return NULL;
    }
    return ef_redis_read_reply(con);
}

ef_redis_reply_t* ef_redis_read_reply(ef_redis_connection_t *con)
{
    int r = ef_wrap_read(con->sockfd, con->buf, RES_BUFSIZE);
    printf("read from server:%d\n", r);
    if (r < 0) {
        printf("read response fail:%d\n", r);
        return NULL;
    }
    con->end = r;
    con->seek = 0;

    char *start = con->buf + con->seek;
    switch (*(start)) {
        case '-': {
            return parse_error(start, con);
        }
        case '+': {
            return parse_single_string(start, con);
        }
        case ':': {
            return parse_long(start, con);
        }
        case '*': {
            return parse_array(start, con);
        }
        case '$': {
            return parse_bulk_string(start, con);
        }
        default: {
            printf("ef_redis_read_reply unsupported type:%c\n", *(con->buf));
            return NULL;
        }
    }   
}

static int reply_read_more(ef_redis_connection_t *con)
{
    size_t copylen = con->end - con->seek;
    if (copylen > 0) {
        strncpy(con->buf, con->buf + con->seek, copylen);        
    }
    con->seek = 0;
    con->end = copylen;
    int r = ef_wrap_read(con->sockfd, con->buf + con->end, RES_BUFSIZE - con->end);
    printf("read from server:%d\n", r);
    if (r < 0) {
        printf("reply read more error:%d\n", r);
        return r;
    }
    con->end += r;
    return 0;
}

// parse reply with prefix ':'
static ef_redis_reply_t* parse_long(char *buf, ef_redis_connection_t *con)
{
    ef_redis_reply_t *rep = (ef_redis_reply_t *)malloc(sizeof(ef_redis_reply_t));
    if (rep == NULL) {
        return rep;
    }
    rep->type = REPLY_TYPE_LONG;
    // check the completion
    while (strstr(buf, "\r\n") == NULL) {
        int res = reply_read_more(con);
        if (res < 0) {
            free(rep);
            return NULL;
        }
        buf = con->buf;
    }
    long n = 0;
    char * start = buf;
    buf++;
    while (*buf != '\r') {
        n = n * 10 + (*buf) - '0';
        buf++;
    }
    rep->reply.d = n;
    con->seek = con->seek + (buf - start) + 2;
    return rep;
}

// parse reply with prefix '+'
static ef_redis_reply_t* parse_single_string(char *buf, ef_redis_connection_t *con)
{
    buf += 1;
    con->seek += 1;
    char *pos = strstr(buf, "\r\n");
    while (pos == NULL) {
        int res = reply_read_more(con);
        if (res < 0) {
            return NULL;
        }
        buf = con->buf;
        pos = strstr(buf, "\r\n");
    }
    return parse_string(buf, pos-buf, con);
}

// parse reply with prefix '$'
static ef_redis_reply_t* parse_bulk_string(char *buf, ef_redis_connection_t *con)
{
    char *pos = strstr(buf, "\r\n");
    while (pos == NULL) {
        int res = reply_read_more(con);
        if (res < 0) {
            return NULL;
        }
        buf = con->buf;
        pos = strstr(buf, "\r\n");
    }
    char *start = buf;
    int strlen = parse_len(buf);
    buf = pos + 2;
    con->seek = con->seek + (buf - start);
    return parse_string(buf, strlen, con);
}

// parse string content with sufix "\r\n"
static ef_redis_reply_t* parse_string(char *buf, size_t strlen, ef_redis_connection_t *con)
{
    ef_redis_reply_t *rep = (ef_redis_reply_t *)malloc(sizeof(ef_redis_reply_t));
    if (rep == NULL) {
        return rep;
    }

    if (strlen == -1) {
        rep->type = REPLY_TYPE_NULL;
        return rep;
    }

    rep->type = REPLY_TYPE_STR;
    if (strlen >= 0) {
        reply_string_t *str = (reply_string_t*)malloc(sizeof(reply_string_t));
        if (str == NULL) {
            return NULL;
        }
        char *content = (char*)malloc(strlen);
        if (content == NULL) {
            free(rep);
            return NULL;
        }  

        size_t read = 0;
        if (con->end - con->seek < strlen) {
            read = (con->end - con->seek);
            content = strncpy(content, buf, read);
            con->seek += read;
            while (read < strlen) {
                int res = reply_read_more(con);
                if (res < 0) {
                    return NULL;
                }
                buf = con->buf;
                if (con->end - con->seek + read < strlen) {
                    strncpy(content + read, buf, con->end - con->seek);
                    read += (con->end - con->seek);
                    con->seek = con->end;
                } else {
                    strncpy(content + read, buf, strlen - read);
                    con->seek = con->seek + (strlen - read);
                    break;
                }
            }
        } else {
            content = strncpy(content, buf, strlen);
            con->seek = con->seek + strlen + 2;
        }
        
        str->buf = content;
        str->len = strlen;
        rep->reply.str = str;
    }

    return rep;
}

// parse reply with prefix '*'
static ef_redis_reply_t * parse_array(char *buf, ef_redis_connection_t *con)
{
    ef_redis_reply_t *rep = (ef_redis_reply_t *)malloc(sizeof(ef_redis_reply_t));
    if (rep == NULL) {
        return rep;
    }
    rep->type = REPLY_TYPE_ARR;

    char *pos, *start;
    pos = strstr(buf, "\r\n");
    while (pos == NULL) {
        int res = reply_read_more(con);
        if (res < 0) {
            free(rep);
            return NULL;
        }
        buf = con->buf;
    }
    start = buf;   
    int repnum = parse_len(buf);
    buf = pos + 2;
    con->seek = con->seek + (buf - start);

    rep->reply.arr = (reply_array_t*)malloc(sizeof(reply_array_t));
    if (rep->reply.arr == NULL) {
        printf("ef_redis_reply_t malloc error\n");
        return NULL;
    }
    rep->reply.arr->num = repnum;
    if (repnum == -1) {
        rep->type = REPLY_TYPE_NULL;
        return rep;
    }
    rep->reply.arr->elem = (ef_redis_reply_t**)malloc(repnum * sizeof(ef_redis_reply_t*));
    if (rep->reply.arr->elem == NULL) {
        printf("arr elem malloc error\n");
        return NULL;
    }

    int i;
    for (i = 0; i < repnum; i++) {
        switch (*buf) {
            case '$': {
                *(rep->reply.arr->elem + i) = parse_bulk_string(buf, con);
                buf = con->buf + con->seek;
                continue;
            }
            case ':': {
                *(rep->reply.arr->elem + i) = parse_long(buf, con);
                buf = con->buf + con->seek;
                continue;
            }
            case '*': {
                *(rep->reply.arr->elem + i) = parse_array(buf, con);
                buf = con->buf + con->seek;
                continue;
            }
            default: {
                printf("unsupport type:%c\n", *buf);
                break;
            }
        }
    }

    return rep;
}

// parse reply with prefix '-'
static ef_redis_reply_t* parse_error(char *buf, ef_redis_connection_t *con)
{   
    ef_redis_reply_t *rep = (ef_redis_reply_t *)malloc(sizeof(ef_redis_reply_t));
    if (rep == NULL) {
        return rep;
    }
    rep->type = REPLY_TYPE_ERR;

    char *type, *errinfo, *start, *pos, *end;
    end = strstr(buf, "\r\n");
    while (end == NULL) {
        int res = reply_read_more(con);
        if (res < 0) {
            free(rep);
            return NULL;
        }
        buf = con->buf;
        end = strstr(buf, "\r\n");
    }

    reply_error_t *err = (reply_error_t*)malloc(sizeof(reply_error_t));
    if (err == NULL) {
        return NULL;
    }
    start = buf + 1;
    pos = strstr(buf, " ");
    if (pos != NULL) {
        type = (char *)malloc(pos - start);
        if (type != NULL) {
            type = strncpy(type, start, pos - start);
            if (type == NULL) {
                printf("strcpy error\n");
            }
            *(type + (pos - start)) = '\0';
        }
    }
    start = pos + 1;
    while (*(start) == ' ') {
        start++;
    }

    errinfo = (char *)malloc(end - start);
    errinfo = strncpy(errinfo, start, end - start);
    *(errinfo + (end - start)) = '\0';
    err->type = type;
    err->err = errinfo;
    rep->reply.err = err;
    return rep;
}

// parse length in response with format:
// *3\r\n number of multiple responses
// $12\r\n length of a single response
static int parse_len(char *res)
{
    if ((*res != '$') && (*res != '*') || *(res + 1) == '-') {
        return -1;
    }
    res++;
    int len = 0;
    while (*res != '\r') {
        len = len * 10 + (*res - '0');
        res++;
    }
    return len;
}