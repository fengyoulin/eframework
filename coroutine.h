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

#ifndef _COROUTINE_HEADER_
#define _COROUTINE_HEADER_

#include <unistd.h>
#include <sys/time.h>
#include "fiber.h"
#include "structure/list.h"

#define ERROR_CO_EXITED ERROR_FIBER_EXITED
#define ERROR_CO_NOT_INITED ERROR_FIBER_NOT_INITED

typedef struct _ef_coroutine_t {
    ef_fiber_t fiber;
    ef_list_entry_t full_entry;
    ef_list_entry_t free_entry;
    struct timeval last_run_time;
} ef_coroutine_t;

typedef struct _ef_coroutine_pool_t {
    ef_fiber_sched_t fiber_sched;
    size_t stack_size;
    int limit_min;
    int limit_max;
    ef_list_entry_t full_list;
    ef_list_entry_t free_list;
    int full_count;
    int free_count;
} ef_coroutine_pool_t;

#define ef_coroutine_proc_t ef_fiber_proc_t

void ef_coroutine_pool_init(ef_coroutine_pool_t *pool, size_t stack_size, int limit_min, int limit_max);

ef_coroutine_t *ef_coroutine_create(ef_coroutine_pool_t *pool, size_t header_size, ef_coroutine_proc_t fiber_proc, void *param);

long ef_coroutine_resume(ef_coroutine_pool_t *pool, ef_coroutine_t *co, long to_yield);

int ef_coroutine_pool_shrink(ef_coroutine_pool_t *pool, int idle_millisecs, int max_count);

inline ef_coroutine_t *ef_coroutine_current(ef_coroutine_pool_t *pool) __attribute__((always_inline));

inline ef_coroutine_t *ef_coroutine_current(ef_coroutine_pool_t *pool)
{
    ef_fiber_sched_t *rt = &pool->fiber_sched;
    if (rt->current_fiber == &rt->thread_fiber) {
        return NULL;
    }
    return (ef_coroutine_t*)rt->current_fiber;
}

#endif
