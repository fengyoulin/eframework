#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "framework.h"

ef_runtime_t efr = {0};

ef_fiber_sched_t *ef_get_fiber_sched(void)
{
    return &efr.co_pool.fiber_sched;
}

long echo_proc(int fd)
{
    char buffer[1024];
    ssize_t ret = ef_wrap_read(fd, buffer, 1024);
    if(ret <= 0)
    {
        return ret;
    }
    ret = ef_wrap_write(fd, buffer, ret);
    return ret;
}

int main(int argc, char *argv[])
{
    ef_init(&efr, 0, 64, 128);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        return -1;
    }
    struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(80);
    bind(sockfd, (const struct sockaddr *)&addr_in, sizeof(addr_in));
    listen(sockfd, 512);
    ef_add_listen(&efr, sockfd, echo_proc);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        return -1;
    }
    addr_in.sin_port = htons(8080);
    bind(sockfd, (const struct sockaddr *)&addr_in, sizeof(addr_in));
    listen(sockfd, 512);
    ef_add_listen(&efr, sockfd, echo_proc);
    return ef_run_loop(&efr);
}
