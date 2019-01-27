#ifndef _EFRAMEWORK_HEADER_
#define _EFRAMEWORK_HEADER_

#include "coroutine.h"
#include "dlist.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#define POLL_TYPE_LISTEN 1
#define POLL_TYPE_RDWRCON 2

typedef long (*ef_routine_proc_t)(int fd);

typedef struct _ef_epoll_data_t {
    int type;
    int fd;
    struct _ef_routine_t *routine_ptr;
    struct _ef_runtime_t *runtime_ptr;
    ef_routine_proc_t ef_proc;
} ef_epoll_data_t;

typedef struct _ef_queue_fd_t {
    int fd;
    dlist_entry_t list_entry;
} ef_queue_fd_t;

typedef struct _ef_listen_info_t {
    ef_epoll_data_t poll_data;
    ef_routine_proc_t ef_proc;
    dlist_entry_t list_entry;
    dlist_entry_t queue_list;
} ef_listen_info_t;

typedef struct _ef_routine_t {
    ef_coroutine_t co;
    ef_epoll_data_t poll_data;
} ef_routine_t;

typedef struct _ef_runtime_t {
    int epfd;
    int stopping;
    ef_coroutine_pool_t co_pool;
    dlist_entry_t listen_list;
} ef_runtime_t;

#define ef_routine_current() ((ef_routine_t*)ef_coroutine_current())

int ef_init(ef_runtime_t *rt, size_t stack_size, int limit_min, int limit_max);
int ef_add_listen(ef_runtime_t *rt, int socket, ef_routine_proc_t ef_proc);
int ef_run_loop(ef_runtime_t *rt);
int ef_wrap_socket(int domain, int type, int protocol);
int ef_wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t ef_wrap_read(int fd, void *buf, size_t count);
ssize_t ef_wrap_write(int fd, const void *buf, size_t count);

#endif
