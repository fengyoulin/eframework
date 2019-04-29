// Copyright (c) 2018-2019 The EFramework Project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "poll.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

typedef struct epoll_event epoll_event_t;

typedef struct _ef_event_pair {
    int old_fd;
    int new_fd;
    ef_event_t old_event;
    ef_event_t new_event;
    unsigned int old_id;
    unsigned int new_id;
    int op;
} ef_event_pair_t;

typedef struct _ef_epoll {
    ef_poll_t poll;
    int epfd;

    /*
     * poll event buffer
     */
    int cap;
    epoll_event_t *events;

    /*
     * optimize associate/dissociate
     */
    int total;
    int used;
    int modified;
    int *index;
    ef_event_pair_t *pairs;
} ef_epoll_t;

static int ef_epoll_expand(ef_epoll_t *ep, int fd)
{
    int total;
    int *index;
    ef_event_pair_t *pairs;

    total = ep->total;
    if (total > fd) {
        return 0;
    }

    /*
     * every time multiply 2
     */
    while (total <= fd) {
        total <<= 1;
    }

    index = (int *)realloc(ep->index, sizeof(int) * total);
    if (!index) {
        return -1;
    }

    /*
     * set unused area to -1
     */
    memset(index + ep->total, -1, sizeof(int) * (total - ep->total));
    ep->index = index;

    pairs = (ef_event_pair_t *)realloc(ep->pairs, sizeof(ef_event_pair_t) * total);
    if (!pairs) {
        return -1;
    }

    ep->pairs = pairs;

    /*
     * update total to the new
     */
    ep->total = total;

    return 0;
}

static int ef_epoll_associate(ef_poll_t *p, int fd, int events, void *ptr, unsigned int id, int fired)
{
    ef_epoll_t *ep;
    ef_event_pair_t *pp, tmp;
    int idx, newly;

    if (fd < 0) {
        return -1;
    }

    /*
     * epoll will not auto dissociate fd after event fired
     */
    if (fired) {
        return 0;
    }

    ep = (ef_epoll_t *)p;

    if (fd >= ep->total && ef_epoll_expand(ep, fd) < 0) {
        return -1;
    }

    idx = ep->index[fd];
    if (idx < 0) {
        idx = ep->used++;
        ep->index[fd] = idx;
        memset(&ep->pairs[idx], -1, sizeof(ef_event_pair_t));
    }

    pp = &ep->pairs[idx];

    newly = (pp->old_fd != fd || pp->old_id != id || pp->old_event.ptr != ptr);

    if (newly || pp->old_event.events != events) {

        pp->new_fd = fd;
        pp->new_id = id;
        pp->new_event.events = events;
        pp->new_event.ptr = ptr;

        pp->op = newly ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

        /*
         * swap from used to modified if need
         */
        if (idx > ep->modified) {
            ep->index[pp->new_fd] = ep->modified;
            ep->index[ep->pairs[ep->modified].new_fd] = idx;
            memcpy(&tmp, pp, sizeof(ef_event_pair_t));
            memcpy(pp, &ep->pairs[ep->modified], sizeof(ef_event_pair_t));
            memcpy(&ep->pairs[ep->modified], &tmp, sizeof(ef_event_pair_t));
        }

        if (idx >= ep->modified) {
            ++ep->modified;
        }
    } else {

        /*
         * readd a masked deleted fd, restore to unmodified
         */
        if (idx < ep->modified && pp->op == EPOLL_CTL_DEL) {
            --ep->modified;
            if (idx < ep->modified) {
                ep->index[pp->new_fd] = ep->modified;
                ep->index[ep->pairs[ep->modified].new_fd] = idx;
                memcpy(&tmp, pp, sizeof(ef_event_pair_t));
                memcpy(pp, &ep->pairs[ep->modified], sizeof(ef_event_pair_t));
                memcpy(&ep->pairs[ep->modified], &tmp, sizeof(ef_event_pair_t));
            }
        }
    }

    return 0;
}

static int ef_epoll_dissociate(ef_poll_t *p, int fd, int fired)
{
    ef_epoll_t *ep;
    ef_event_pair_t *pp, tmp;
    int idx;

    if (fd < 0) {
        return -1;
    }

    ep = (ef_epoll_t *)p;

    if (fd >= ep->total) {
        return 0;
    }

    idx = ep->index[fd];
    if (idx < 0) {
        return 0;
    }

    pp = &ep->pairs[idx];

    /*
     * free the just masked added fd
     */
    if (idx < ep->modified && pp->op == EPOLL_CTL_ADD) {

        --ep->modified;
        if (idx < ep->modified) {
            ep->index[pp->new_fd] = ep->modified;
            ep->index[ep->pairs[ep->modified].new_fd] = idx;
            memcpy(&tmp, pp, sizeof(ef_event_pair_t));
            memcpy(pp, &ep->pairs[ep->modified], sizeof(ef_event_pair_t));
            memcpy(&ep->pairs[ep->modified], &tmp, sizeof(ef_event_pair_t));
        }

        idx = ep->modified;
        pp = &ep->pairs[idx];

        --ep->used;
        ep->index[pp->new_fd] = -1;
        if (idx < ep->used) {
            ep->index[ep->pairs[ep->used].new_fd] = idx;
            memcpy(pp, &ep->pairs[ep->used], sizeof(ef_event_pair_t));
        }

        return 0;
    }

    pp->op = EPOLL_CTL_DEL;

    /*
     * swap from used to modified
     */
    if (idx > ep->modified) {
        ep->index[pp->new_fd] = ep->modified;
        ep->index[ep->pairs[ep->modified].new_fd] = idx;
        memcpy(&tmp, pp, sizeof(ef_event_pair_t));
        memcpy(pp, &ep->pairs[ep->modified], sizeof(ef_event_pair_t));
        memcpy(&ep->pairs[ep->modified], &tmp, sizeof(ef_event_pair_t));
    }

    if (idx >= ep->modified) {
        ++ep->modified;
    }

    return 0;
}

static int ef_epoll_apply(ef_epoll_t *ep)
{
    int retval;
    ef_event_pair_t *pp;
    epoll_event_t *pe;

    if (ep->modified <= 0) {
        return 0;
    }

    pe = &ep->events[0];

    while (ep->modified > 0) {
        --ep->modified;
        pp = &ep->pairs[ep->modified];

        if (pp->op != EPOLL_CTL_DEL) {
            pe->events = pp->new_event.events;
            pe->data.ptr = pp->new_event.ptr;
            pp->old_fd = pp->new_fd;
            pp->old_id = pp->new_id;
            pp->old_event.events = pp->new_event.events;
            pp->old_event.ptr = pp->new_event.ptr;
        }

        /*
         * 1) EBADF the fd maybe closed and auto deleted from epoll
         * 2) ENOENT it maybe closed in one coroutine and open returned
         * the same value in another coroutine
         */
        retval = epoll_ctl(ep->epfd, pp->op, pp->new_fd, pe);
        if (retval < 0 && errno != 0 && errno != EBADF && errno != ENOENT) {
            return retval;
        }

        if (pp->op == EPOLL_CTL_DEL) {
            --ep->used;
            ep->index[pp->new_fd] = -1;
            if (ep->modified < ep->used) {
                ep->index[ep->pairs[ep->used].new_fd] = ep->modified;
                memcpy(pp, &ep->pairs[ep->used], sizeof(ef_event_pair_t));
            }
        }
    }

    return 0;
}

static int ef_epoll_wait(ef_poll_t *p, ef_event_t *evts, int count, int millisecs)
{
    int ret, idx;
    ef_epoll_t *ep = (ef_epoll_t *)p;

    if (ef_epoll_apply(ep) < 0) {
        return -1;
    }

    if (count > ep->cap) {
        count = ep->cap;
    }

    ret = epoll_wait(ep->epfd, ep->events, count, millisecs);
    if (ret <= 0) {
        return ret;
    }

    for (idx = 0; idx < ret; ++idx) {
        evts[idx].events = ep->events[idx].events;
        evts[idx].ptr = ep->events[idx].data.ptr;
    }
    return ret;
}

static int ef_epoll_free(ef_poll_t *p)
{
    ef_epoll_t *ep = (ef_epoll_t *)p;
    close(ep->epfd);
    free(ep->pairs);
    free(ep->index);
    free(ep->events);
    free(ep);
    return 0;
}

static ef_poll_t *ef_epoll_create(int cap)
{
    ef_epoll_t *ep;
    size_t size;

    /*
     * event buffer at least 128
     */
    if (cap < 128) {
        cap = 128;
    }

    ep = (ef_epoll_t *)calloc(1, sizeof(ef_epoll_t));
    if (!ep) {
        return NULL;
    }

    ep->events = (epoll_event_t *)malloc(sizeof(epoll_event_t) * cap);
    if (!ep->events) {
        goto error_exit;
    }

    size = sizeof(int) * cap;
    ep->index = (int *)malloc(size);
    if (!ep->index) {
        goto error_exit;
    }
    memset(ep->index, -1, size);

    ep->pairs = (ef_event_pair_t *)malloc(sizeof(ef_event_pair_t) * cap);
    if (!ep->pairs) {
        goto error_exit;
    }

    ep->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (ep->epfd < 0) {
        goto error_exit;
    }

    ep->poll.associate = ef_epoll_associate;
    ep->poll.dissociate = ef_epoll_dissociate;
    ep->poll.wait = ef_epoll_wait;
    ep->poll.free = ef_epoll_free;
    ep->cap = cap;
    ep->total = cap;
    return &ep->poll;

error_exit:

    if (ep->pairs) {
        free(ep->pairs);
    }
    if (ep->index) {
        free(ep->index);
    }
    if (ep->events) {
        free(ep->events);
    }
    free(ep);

    return NULL;
}

create_func_t ef_create_poll = ef_epoll_create;
