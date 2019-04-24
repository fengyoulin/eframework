#ifndef _EFRAMEWORK_HEADER_
#define _EFRAMEWORK_HEADER_

#include "coroutine.h"
#include "structure/list.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#define POLL_TYPE_LISTEN 1
#define POLL_TYPE_RDWRCON 2

typedef struct _ef_routine ef_routine_t;
typedef struct _ef_runtime ef_runtime_t;
typedef struct _ef_queue_fd ef_queue_fd_t;
typedef struct _ef_epoll_data ef_epoll_data_t;
typedef struct _ef_listen_info ef_listen_info_t;

typedef long (*ef_routine_proc_t)(int fd, ef_routine_t *er);

struct _ef_epoll_data {
    int type;
    int fd;
    ef_routine_t *routine_ptr;
    ef_runtime_t *runtime_ptr;
    ef_routine_proc_t ef_proc;
};

struct _ef_queue_fd {
    int fd;
    ef_list_entry_t list_entry;
};

struct _ef_listen_info {
    ef_epoll_data_t poll_data;
    ef_routine_proc_t ef_proc;
    ef_list_entry_t list_entry;
    ef_list_entry_t fd_list;
};

struct _ef_runtime {
    int epfd;
    int stopping;
    int shrink_millisecs;
    int count_per_shrink;
    ef_coroutine_pool_t co_pool;
    ef_list_entry_t listen_list;
    ef_list_entry_t free_fd_list;
};

struct _ef_routine {
    ef_coroutine_t co;
    ef_epoll_data_t poll_data;
};

extern ef_runtime_t *_ef_runtime;

#define ef_routine_current() ((ef_routine_t*)ef_coroutine_current(&_ef_runtime->co_pool))

int ef_init(ef_runtime_t *rt, size_t stack_size, int limit_min, int limit_max, int shrink_millisecs, int count_per_shrink);
int ef_add_listen(ef_runtime_t *rt, int socket, ef_routine_proc_t ef_proc);
int ef_run_loop(ef_runtime_t *rt);

int ef_routine_connect(ef_routine_t *er, int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t ef_routine_read(ef_routine_t *er, int fd, void *buf, size_t count);
ssize_t ef_routine_write(ef_routine_t *er, int fd, const void *buf, size_t count);
ssize_t ef_routine_recv(ef_routine_t *er, int sockfd, void *buf, size_t len, int flags);
ssize_t ef_routine_send(ef_routine_t *er, int sockfd, const void *buf, size_t len, int flags);

#define ef_wrap_connect(sockfd, addr, addrlen) \
    ef_routine_connect(NULL, sockfd, addr, addrlen)
#define ef_wrap_read(fd, buf, count) \
    ef_routine_read(NULL, fd, buf, count)
#define ef_wrap_write(fd, buf, count) \
    ef_routine_write(NULL, fd, buf, count)
#define ef_wrap_recv(sockfd, buf, len, flags) \
    ef_routine_recv(NULL, sockfd, buf, len, flags)
#define ef_wrap_send(sockfd, buf, len, flags) \
    ef_routine_send(NULL, sockfd, buf, len, flags)

#endif
