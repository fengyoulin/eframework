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

#include "framework.h"
#include <port.h>
#include <errno.h>

typedef struct _ef_port {
    ef_poll_t poll;
    int ptfd;
    int count;
    port_event_t events[0];
} ef_port_t;

static int ef_port_associate(ef_poll_t *p, int fd, int events, void *ptr)
{
    ef_port_t *ep = (ef_port_t *)p;
    return port_associate(ep->ptfd, PORT_SOURCE_FD, fd, events, ptr);
}

static int ef_port_dissociate(ef_poll_t *p, int fd)
{
    ef_port_t *ep = (ef_port_t *)p;
    return port_dissociate(ep->ptfd, PORT_SOURCE_FD, fd);
}

static int ef_port_wait(ef_poll_t *p, ef_event_t *evts, int count, int millisecs)
{
    uint_t nget, idx;
    timespec_t timeout;
    ef_port_t *ep = (ef_port_t *)p;

    if (count > ep->count) {
        count = ep->count;
    }

    timeout.tv_sec = millisecs / 1000;
    timeout.tv_nsec = (millisecs % 1000) * 1000000;
    nget = 1;

again:
    if (port_getn(ep->ptfd, &ep->events[0], count, &nget, &timeout) < 0) {
        if (errno == EINTR) {
            goto again;
        } else if (errno != ETIME) {
            return -1;
        }
    }

    for (idx = 0; idx < nget; ++idx) {
        evts[idx].events = ep->events[idx].portev_events;
        evts[idx].ptr = ep->events[idx].portev_user;
    }
    return (int)nget;
}

static int ef_port_free(ef_poll_t *p)
{
    ef_port_t *ep = (ef_port_t *)p;
    close(ep->ptfd);
    free(ep);
    return 0;
}

ef_poll_t *ef_port_create(int count)
{
    ef_port_t *ep;
    size_t size = sizeof(ef_port_t);

    /*
     * event buffer at least 16
     */
    if (count < 16) {
        count = 16;
    }

    size += sizeof(port_event_t) * count;
    ep = (ef_port_t *)malloc(size);
    if (!ep) {
        return NULL;
    }

    ep->ptfd = port_create();
    if (ep->ptfd < 0) {
        free(ep);
        return NULL;
    }

    ep->poll.associate = ef_port_associate;
    ep->poll.dissociate = ef_port_dissociate;
    ep->poll.wait = ef_port_wait;
    ep->poll.free = ef_port_free;
    ep->count = count;
    return &ep->poll;
}

