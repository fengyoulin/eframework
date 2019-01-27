#include "framework.h"
#include "coroutine.h"
#include "dlist.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

long ef_proc(void *param)
{
    ef_routine_t *er = ef_routine_current();
    int fd = er->poll_data.fd;
    long retval = 0;
    if(er->poll_data.ef_proc)
    {
        retval = er->poll_data.ef_proc(fd);
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return retval;
}

int event_routine_run(ef_runtime_t *rt, ef_routine_proc_t proc, int socket)
{
    ef_routine_t *er = (ef_routine_t*)ef_coroutine_create(&rt->co_pool, ef_proc, NULL);
    if(er)
    {
        er->poll_data.type = POLL_TYPE_RDWRCON;
        er->poll_data.fd = socket;
        er->poll_data.routine_ptr = er;
        er->poll_data.runtime_ptr = rt;
        er->poll_data.ef_proc = proc;
        ef_coroutine_resume(&rt->co_pool, &er->co, 0);
        return 0;
    }
    return -1;
}

int ef_init(ef_runtime_t *rt, size_t stack_size, int limit_min, int limit_max)
{
    rt->epfd = epoll_create1(EPOLL_CLOEXEC);
    rt->stopping = 0;
    ef_coroutine_pool_init(&rt->co_pool, stack_size, limit_min, limit_max);
    list_init(&rt->listen_list);
    return rt->epfd < 0;
}

int ef_add_listen(ef_runtime_t *rt, int socket, ef_routine_proc_t ef_proc)
{
    int retval = fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
    if(retval < 0)
    {
        return retval;
    }
    ef_listen_info_t *li = (ef_listen_info_t*)malloc(sizeof(ef_listen_info_t));
    if(li == NULL)
    {
        return -1;
    }
    li->poll_data.type = POLL_TYPE_LISTEN;
    li->poll_data.fd = socket;
    li->poll_data.routine_ptr = NULL;
    li->poll_data.runtime_ptr = rt;
    li->ef_proc = ef_proc;
    list_init(&li->queue_list);
    list_insert_after(&rt->listen_list, &li->list_entry);
    return 0;
}

int ef_run_loop(ef_runtime_t *rt)
{
    int retval;
    struct epoll_event evt = {0};
    evt.events = EPOLLIN;
    dlist_entry_t *lle = list_entry_after(&rt->listen_list);
    while(lle != &rt->listen_list)
    {
        ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
        evt.data.ptr = &li->poll_data;
        retval = epoll_ctl(rt->epfd, EPOLL_CTL_ADD, li->poll_data.fd, &evt);
        if(retval < 0)
        {
            return retval;
        }
        lle = list_entry_after(lle);
    }
    struct epoll_event evts[1024] = {0};
    while(1)
    {
        retval = epoll_wait(rt->epfd, &evts[0], 1024, 1000);
        if(retval < 0)
        {
            return retval;
        }
        printf("all_count: %ld\nrun_count: %ld\n", rt->co_pool.full_count, rt->co_pool.full_count - rt->co_pool.free_count);
        for(int i = 0; i < retval; ++i)
        {
            ef_epoll_data_t *pfi = (ef_epoll_data_t*)evts[i].data.ptr;
            if(pfi->type == POLL_TYPE_RDWRCON)
            {
                ef_coroutine_resume(&rt->co_pool, &pfi->routine_ptr->co, evts[i].events);
            }
            else if(pfi->type == POLL_TYPE_LISTEN)
            {
                int retval = 0;
                while(1)
                {
                    int socket = accept(pfi->fd, NULL, NULL);
                    if(socket < 0)
                    {
                        break;
                    }
                    fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
                    ef_listen_info_t *pli = CAST_PARENT_PTR(pfi, ef_listen_info_t, poll_data);
                    if(retval >= 0)
                    {
                        retval = event_routine_run(rt, pli->ef_proc, socket);
                    }
                    if(retval < 0)
                    {
                        ef_queue_fd_t *qi = (ef_queue_fd_t*)malloc(sizeof(ef_queue_fd_t));
                        if(qi)
                        {
                            qi->fd = socket;
                            list_insert_before(&pli->queue_list, &qi->list_entry);
                        }
                        else
                        {
                            shutdown(socket, SHUT_RDWR);
                            close(socket);
                        }
                    }
                }
            }
        }
        lle = list_entry_after(&rt->listen_list);
        while(lle != &rt->listen_list)
        {
            ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
            dlist_entry_t *qie = list_entry_after(&li->queue_list);
            while(qie != &li->queue_list)
            {
                ef_queue_fd_t *qi = CAST_PARENT_PTR(qie, ef_queue_fd_t, list_entry);
                qie = list_entry_after(qie);
                int retval = event_routine_run(rt, li->ef_proc, qi->fd);
                if(retval < 0)
                {
                    goto exit_queue;
                }
                else
                {
                    list_remove(&qi->list_entry);
                    free(qi);
                }
            }
            lle = list_entry_after(lle);
        }
exit_queue:
        if(rt->stopping)
        {
            if(!list_empty(&rt->listen_list))
            {
                lle = list_remove_after(&rt->listen_list);
                while(lle != NULL)
                {
                    ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
                    epoll_ctl(rt->epfd, EPOLL_CTL_DEL, li->poll_data.fd, &evt);
                    shutdown(li->poll_data.fd, SHUT_RDWR);
                    close(li->poll_data.fd);
                    free(li);
                    lle = list_remove_after(lle);
                }
            }
            if(rt->co_pool.free_count == rt->co_pool.full_count)
            {
                close(rt->epfd);
                ef_coroutine_pool_shrink(&rt->co_pool, 0, -rt->co_pool.full_count);
                break;
            }
            else
            {
                ef_coroutine_pool_shrink(&rt->co_pool, 0, -rt->co_pool.free_count);
            }
        }
    }
    return 0;
}

int ef_wrap_socket(int domain, int type, int protocol)
{
    int fd = socket(domain, type, protocol);
    if(fd < 0)
    {
        return fd;
    }
    int retval = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    if(retval < 0)
    {
        close(fd);
        return retval;
    }
    return fd;
}

int ef_wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    ef_routine_t *er = ef_routine_current();
    struct epoll_event evt = {0};
    evt.events = EPOLLOUT;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = sockfd;
    int added = 0;
    int retval = connect(sockfd, addr, addrlen);
    if(retval < 0)
    {
        if(errno != EINPROGRESS)
        {
            return retval;
        }
        retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, sockfd, &evt);
        if(retval < 0)
        {
            return retval;
        }
    }
    long events = ef_coroutine_yield(0);
    if(events & (EPOLLERR | EPOLLHUP))
    {
        retval = -1;
    }
    else if(events & EPOLLOUT)
    {
        retval = connect(sockfd, addr, addrlen);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, sockfd, &evt);
    return retval;
}

ssize_t ef_wrap_read(int fd, void *buf, size_t count)
{
    ef_routine_t *er = ef_routine_current();
    struct epoll_event evt = {0};
    evt.events = EPOLLIN;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = fd;
    int retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, fd, &evt);
    if(retval < 0)
    {
        return retval;
    }
    long events = ef_coroutine_yield(0);
    if(events & EPOLLERR)
    {
        retval = -1;
    }
    else if(events & EPOLLIN)
    {
        retval = read(fd, buf, count);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, fd, &evt);
    return retval;
}

ssize_t ef_wrap_write(int fd, const void *buf, size_t count)
{
    ef_routine_t *er = ef_routine_current();
    struct epoll_event evt = {0};
    evt.events = EPOLLOUT;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = fd;
    int retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, fd, &evt);
    if(retval < 0)
    {
        return retval;
    }
    long events = ef_coroutine_yield(0);
    if(events & (EPOLLERR | EPOLLHUP))
    {
        retval = -1;
    }
    else if(events & EPOLLOUT)
    {
        retval = write(fd, buf, count);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, fd, &evt);
    return retval;
}
