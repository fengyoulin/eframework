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

inline ef_queue_fd_t *ef_alloc_fd(ef_runtime_t *rt) __attribute__((always_inline));
inline int ef_routine_run(ef_runtime_t *rt, ef_routine_proc_t proc, int socket) __attribute__((always_inline));

long ef_proc(void *param)
{
    ef_routine_t *er = (ef_routine_t*)param;
    int fd = er->poll_data.fd;
    long retval = 0;
    if(er->poll_data.ef_proc)
    {
        retval = er->poll_data.ef_proc(fd, er);
    }
    close(fd);
    return retval;
}

inline ef_queue_fd_t *ef_alloc_fd(ef_runtime_t *rt)
{
    ef_queue_fd_t *qf = NULL;
    if(!list_empty(&rt->free_fd_list))
    {
        qf = CAST_PARENT_PTR(list_remove_after(&rt->free_fd_list), ef_queue_fd_t, list_entry);
    }
    else
    {
        qf = (ef_queue_fd_t*)malloc(sizeof(ef_queue_fd_t));
    }
    return qf;
}

inline int ef_routine_run(ef_runtime_t *rt, ef_routine_proc_t proc, int socket)
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

int ef_init(ef_runtime_t *rt, size_t stack_size, int limit_min, int limit_max, int shrink_millisecs, int count_per_shrink)
{
    rt->epfd = epoll_create1(EPOLL_CLOEXEC);
    rt->stopping = 0;
    rt->shrink_millisecs = shrink_millisecs;
    rt->count_per_shrink = count_per_shrink;
    ef_coroutine_pool_init(&rt->co_pool, stack_size, limit_min, limit_max);
    list_init(&rt->listen_list);
    list_init(&rt->free_fd_list);
    return rt->epfd < 0 ? -1 : 0;
}

int ef_add_listen(ef_runtime_t *rt, int socket, ef_routine_proc_t proc)
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
    li->ef_proc = proc;
    list_init(&li->fd_list);
    list_insert_after(&rt->listen_list, &li->list_entry);
    return 0;
}

int ef_run_loop(ef_runtime_t *rt)
{
    struct epoll_event evt = {0};
    evt.events = EPOLLIN;
    dlist_entry_t *lle = list_entry_after(&rt->listen_list);
    while(lle != &rt->listen_list)
    {
        ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
        evt.data.ptr = &li->poll_data;
        int retval = epoll_ctl(rt->epfd, EPOLL_CTL_ADD, li->poll_data.fd, &evt);
        if(retval < 0)
        {
            return retval;
        }
        lle = list_entry_after(lle);
    }
    struct epoll_event evts[1024] = {0};
    while(1)
    {
        int retcnt = epoll_wait(rt->epfd, &evts[0], 1024, 1000);
        if(retcnt < 0 && errno != EINTR)
        {
            return retcnt;
        }
        // printf("all_count: %ld\nrun_count: %ld\n", rt->co_pool.full_count, rt->co_pool.full_count - rt->co_pool.free_count);
        for(int i = 0; i < retcnt; ++i)
        {
            ef_epoll_data_t *ed = (ef_epoll_data_t*)evts[i].data.ptr;
            if(ed->type == POLL_TYPE_LISTEN)
            {
                int runret = 0;
                while(1)
                {
                    int socket = accept(ed->fd, NULL, NULL);
                    if(socket < 0)
                    {
                        break;
                    }
                    ef_listen_info_t *li = CAST_PARENT_PTR(ed, ef_listen_info_t, poll_data);
                    if(runret >= 0)
                    {
                        runret = ef_routine_run(rt, li->ef_proc, socket);
                    }
                    if(runret < 0)
                    {
                        ef_queue_fd_t *qf = ef_alloc_fd(rt);
                        if(qf)
                        {
                            qf->fd = socket;
                            list_insert_before(&li->fd_list, &qf->list_entry);
                        }
                        else
                        {
                            close(socket);
                        }
                    }
                }
            }
            else if(ed->type == POLL_TYPE_RDWRCON)
            {
                ef_coroutine_resume(&rt->co_pool, &ed->routine_ptr->co, evts[i].events);
            }
        }
        lle = list_entry_after(&rt->listen_list);
        while(lle != &rt->listen_list)
        {
            ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
            dlist_entry_t *fle = list_entry_after(&li->fd_list);
            while(fle != &li->fd_list)
            {
                ef_queue_fd_t *qf = CAST_PARENT_PTR(fle, ef_queue_fd_t, list_entry);
                fle = list_entry_after(fle);
                int runret = ef_routine_run(rt, li->ef_proc, qf->fd);
                if(runret < 0)
                {
                    goto exit_queue;
                }
                else
                {
                    list_remove(&qf->list_entry);
                    list_insert_after(&rt->free_fd_list, &qf->list_entry);
                }
            }
            lle = list_entry_after(lle);
        }
exit_queue:
        if(rt->stopping)
        {
            if(!list_empty(&rt->listen_list))
            {
                lle = list_entry_after(&rt->listen_list);
                while(lle != &rt->listen_list)
                {
                    ef_listen_info_t *li = CAST_PARENT_PTR(lle, ef_listen_info_t, list_entry);
                    lle = list_entry_after(lle);
                    if(li->poll_data.fd >= 0)
                    {
                        epoll_ctl(rt->epfd, EPOLL_CTL_DEL, li->poll_data.fd, &evt);
                        close(li->poll_data.fd);
                        li->poll_data.fd = -1;
                    }
                    if(list_empty(&li->fd_list))
                    {
                        list_remove(&li->list_entry);
                        free(li);
                    }
                }
            }
            if(!list_empty(&rt->free_fd_list))
            {
                dlist_entry_t *ffle = list_remove_after(&rt->free_fd_list);
                while(ffle != NULL)
                {
                    ef_queue_fd_t *qf = CAST_PARENT_PTR(ffle, ef_queue_fd_t, list_entry);
                    ffle = list_remove_after(&rt->free_fd_list);
                    free(qf);
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
        if(rt->co_pool.free_count > 0 && rt->co_pool.full_count > rt->co_pool.limit_min)
        {
            ef_coroutine_pool_shrink(&rt->co_pool, rt->shrink_millisecs, rt->count_per_shrink);
        }
    }
    return 0;
}

int ef_routine_connect(ef_routine_t *er, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if(er == NULL)
    {
        er = ef_routine_current();
    }
    struct epoll_event evt = {0};
    evt.events = EPOLLOUT;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = sockfd;
    int retval = 0;
    int flags = fcntl(sockfd, F_GETFL);
    if(!(flags & O_NONBLOCK))
    {
        retval = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        if(retval < 0)
        {
            goto exit_conn;
        }
    }
    retval = connect(sockfd, addr, addrlen);
    if(retval < 0)
    {
        if(errno != EINPROGRESS)
        {
            goto exit_conn;
        }
        retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, sockfd, &evt);
        if(retval < 0)
        {
            goto exit_conn;
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
exit_conn:
    if(!(flags & O_NONBLOCK))
    {
        fcntl(sockfd, F_SETFL, flags);
    }
    return retval;
}

ssize_t ef_routine_read(ef_routine_t *er, int fd, void *buf, size_t count)
{
    if(er == NULL)
    {
        er = ef_routine_current();
    }
    struct epoll_event evt = {0};
    evt.events = EPOLLIN | EPOLLRDHUP;
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
    else if(events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP))
    {
        retval = read(fd, buf, count);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, fd, &evt);
    return retval;
}

ssize_t ef_routine_write(ef_routine_t *er, int fd, const void *buf, size_t count)
{
    if(er == NULL)
    {
        er = ef_routine_current();
    }
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

ssize_t ef_routine_recv(ef_routine_t *er, int sockfd, void *buf, size_t len, int flags)
{
    if(er == NULL)
    {
        er = ef_routine_current();
    }
    struct epoll_event evt = {0};
    evt.events = EPOLLIN | EPOLLRDHUP;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = sockfd;
    int retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, sockfd, &evt);
    if(retval < 0)
    {
        return retval;
    }
    long events = ef_coroutine_yield(0);
    if(events & EPOLLERR)
    {
        retval = -1;
    }
    else if(events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP))
    {
        retval = recv(sockfd, buf, len, flags);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, sockfd, &evt);
    return retval;
}

ssize_t ef_routine_send(ef_routine_t *er, int sockfd, const void *buf, size_t len, int flags)
{
    if(er == NULL)
    {
        er = ef_routine_current();
    }
    struct epoll_event evt = {0};
    evt.events = EPOLLOUT;
    evt.data.ptr = &er->poll_data;
    er->poll_data.type = POLL_TYPE_RDWRCON;
    er->poll_data.fd = sockfd;
    int retval = epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_ADD, sockfd, &evt);
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
        retval = send(sockfd, buf, len, flags);
    }
    epoll_ctl(er->poll_data.runtime_ptr->epfd, EPOLL_CTL_DEL, sockfd, &evt);
    return retval;
}
