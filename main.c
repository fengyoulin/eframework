#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "framework.h"
#include "redis.h"
#include "basic/basic.h"
#include "http.h"

ef_runtime_t efr = {0};

#define BUFFER_SIZE 8192

long get_proc(int fd,ef_routine_t *er){
    printf("request comes~");
    int ret,w,send,port;
    char buffer[BUFFER_SIZE];
    char *host = "127.0.0.1";
    port = 8081;
    char *request_uri = "/test";
    ef_curl *cu = ef_curl_init(host,port,request_uri);
    if(!cu){
       return -1;
    }
    ret = ef_curl_exec(cu);
    printf("ret=%d\n",ret);
    if(ret < 0){
        goto exit;
    }
    ret = ef_curl_read_response(cu,buffer,BUFFER_SIZE);
    while(ret > 0){
        send = 0;
        while(send < ret){
            w = ef_routine_write(er,fd,buffer,ret);
            if(w < 0){
                ret = w;
                goto exit;
            }
            send += w;
        }
        ret = ef_curl_read_response(cu,buffer,BUFFER_SIZE);
    }
exit:
    ef_curl_close(cu);
    return ret;
}

long redis_proc(int fd,ef_routine_t *er)
{
    ef_redis_connection_t *con = ef_redis_connect("127.0.0.1", 6379);
    if (con == NULL) {
        printf("connect error\n");
        return -1;
    }
    int ret = 0;
    // test PING
    ret = ef_redis_cmd(con, "PING");
    if (ret < 0) {
        printf("cmd PING error:%d", ret);
        goto exit;
    }
    ef_redis_reply_t *rep = ef_redis_read_reply(con);
    if (rep == NULL) {
        printf("read reply error\n");
        ret = -1;
        goto exit;
    }
    ret = ef_routine_write(er, fd, rep->reply.str->buf, rep->reply.str->len);
    ef_redis_free_reply(rep);
    if (ret < 0) {
        printf("write error:%d", ret);
        goto exit;
    }
/*
    // test parse string
    rep = ef_redis_get(con,"a");
    if(rep == NULL){
        printf("ef_redis_get error\n");
        ret = -1;
        goto exit;
    }
    ret = ef_routine_write(er,fd,rep->reply.str->buf,rep->reply.str->len);
    ef_redis_free_reply(rep);
    if(ret < 0){
        printf("write error:%d\n",ret);
        goto exit;
    }

    // test parse integer
    ret = ef_redis_cmd(con,"incr n");
    if (ret < 0){
        printf("cmd incr error:%d",ret);
        goto exit;
    }
    rep = ef_redis_read_reply(con);
    if(rep == NULL){
        printf("reply incr error\n");
        ret = -1;
        goto exit;
    }
    char * val = int2str(rep->reply.d);
    ef_redis_free_reply(rep);
    ret = ef_routine_write(er,fd,val,strlen(val));
    if(ret < 0){
        printf("write error:%d,%d,%s\n",ret,rep->reply.d,val);
        goto exit;
    }

    // test parse array
    ret = ef_redis_cmd(con,"mget %s %s %s","a","n","abc");
    if(ret < 0){
        printf("mget error:%d\n",ret);
        goto exit;
    }
    rep = ef_redis_read_reply(con);
    if(rep == NULL){
        printf("reply mget error\n");
        ret = -1;
        goto exit;
    }
    int i;
    ef_redis_reply_t *subrep;
    for(i=0;i<rep->reply.arr->num;++i){
        subrep = *(rep->reply.arr->elem + i);
        if(subrep->type & REPLY_TYPE_STR){
            ret = ef_routine_write(er,fd,subrep->reply.str->buf,subrep->reply.str->len);
            ef_redis_free_reply(subrep);
            if(ret < 0){
                printf("write subrep str error:%d\n",ret);
                goto exit;
            }
        }
        if(subrep->type & REPLY_TYPE_NULL){
            ret = ef_routine_write(er,fd,"nil",3);
            ef_redis_free_reply(subrep);
            if(ret < 0){
                printf("write subrep str error:%d\n",ret);
                goto exit;
            }
        }
    }
    ef_redis_free_reply(rep);*/
    
    // test error
    ret = ef_redis_cmd(con, "hmget %s", "h");
    if (ret < 0) {
        printf("cmd hmget error\n");
        goto exit;
    }
    rep = ef_redis_read_reply(con);
    if (rep == NULL) {
        printf("reply hmget err\n");
        goto exit;
    }
    if (!(rep->type & REPLY_TYPE_ERR)) {
        printf("parse error error:%d\n", rep->type);
        goto exit;
    }
    ret = ef_routine_write(er, fd, rep->reply.err->type, strlen(rep->reply.err->type));
    if (ret < 0) {
        printf("write err type error\n");
        goto exit;
    }
    ret = ef_routine_write(er, fd, rep->reply.err->err, strlen(rep->reply.err->err));
    if (ret < 0) {
        printf("write err content error\n");
        goto exit;
    }
exit:
    ef_redis_free_reply(rep);
    ef_redis_close(con);
    return ret;
}

// for performance test
// forward port 80 <=> 90, 8080 <=> 90
// let a http server run at localhost:90
// use HTTP/1.0 or set http header Connection: close
long forward_proc(int fd, ef_routine_t *er)
{
    char buffer[BUFFER_SIZE];
    ssize_t r = ef_routine_read(er, fd, buffer, BUFFER_SIZE);
    if(r <= 0)
    {
        return r;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(90);
    int ret = ef_routine_connect(er, sockfd, (const struct sockaddr *)&addr_in, sizeof(addr_in));
    if(ret < 0)
    {
        return ret;
    }
    ssize_t w = ef_routine_write(er, sockfd, buffer, r);
    if(w < 0)
    {
        goto exit_proc;
    }
    while(1)
    {
        r = ef_routine_read(er, sockfd, buffer, BUFFER_SIZE);
        if(r <= 0)
        {
            break;
        }
        ssize_t wrt = 0;
        while(wrt < r)
        {
            w = ef_routine_write(er, fd, &buffer[wrt], r - wrt);
            if(w < 0)
            {
                goto exit_proc;
            }
            wrt += w;
        }
    }
exit_proc:
    close(sockfd);
    return ret;
}

void signal_handler(int num)
{
    efr.stopping = 1;
}

int main(int argc, char *argv[])
{
    ef_init(&efr, 1024 * 1024, 256, 1024, 1000 * 30, 16);

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigaction(SIGTERM, &sa, NULL);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        return -1;
    }
    struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(80);
    int retval = bind(sockfd, (const struct sockaddr *)&addr_in, sizeof(addr_in));
    if(retval < 0)
    {
        return -1;
    }
    listen(sockfd, 512);
    ef_add_listen(&efr, sockfd, get_proc);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        return -1;
    }
    addr_in.sin_port = htons(8080);
    retval = bind(sockfd, (const struct sockaddr *)&addr_in, sizeof(addr_in));
    if(retval < 0)
    {
        return -1;
    }
    listen(sockfd, 512);
    ef_add_listen(&efr, sockfd, forward_proc);

    return ef_run_loop(&efr);
}
