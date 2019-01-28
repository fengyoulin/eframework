#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "framework.h"

ef_runtime_t efr = {0};

ef_fiber_sched_t *ef_get_fiber_sched(void)
{
    return &efr.co_pool.fiber_sched;
}

long echo_proc(int fd, ef_routine_t *er)
{
    char buffer[1024];
    ssize_t ret = ef_routine_read(er, fd, buffer, 1024);
    if(ret <= 0)
    {
        return ret;
    }
    ret = ef_routine_write(er, fd, buffer, ret);
    return ret;
}

void signal_handler(int num)
{
    efr.stopping = 1;
}

int main(int argc, char *argv[])
{
    ef_init(&efr, 0, 64, 128);

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
    ef_add_listen(&efr, sockfd, echo_proc);

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
    ef_add_listen(&efr, sockfd, echo_proc);

    return ef_run_loop(&efr);
}
