#ifndef _COROUTINE_HEADER_
#define _COROUTINE_HEADER_

#include <unistd.h>
#include <sys/time.h>
#include "fiber.h"
#include "dlist.h"

#define ERROR_CO_EXITED ERROR_FIBER_EXITED
#define ERROR_CO_NOT_INITED ERROR_FIBER_NOT_INITED

typedef struct _ef_coroutine_t {
    ef_fiber_t fiber;
    dlist_entry_t full_entry;
    dlist_entry_t free_entry;
    struct timeval last_run_time;
} ef_coroutine_t;

typedef struct _ef_coroutine_pool_t {
    ef_fiber_sched_t fiber_sched;
    size_t stack_size;
    int limit_min;
    int limit_max;
    dlist_entry_t full_list;
    dlist_entry_t free_list;
    int full_count;
    int free_count;
} ef_coroutine_pool_t;

#define ef_coroutine_proc_t ef_fiber_proc_t
#define ef_coroutine_yield(retval) ef_yield_fiber(retval)

inline ef_coroutine_t *ef_coroutine_current(void) __attribute__((always_inline));

void ef_coroutine_pool_init(ef_coroutine_pool_t *pool, size_t stack_size, int limit_min, int limit_max);
ef_coroutine_t *ef_coroutine_create(ef_coroutine_pool_t *pool, ef_coroutine_proc_t fiber_proc, void *param);
long ef_coroutine_resume(ef_coroutine_pool_t *pool, ef_coroutine_t *co, long to_yield);
int ef_coroutine_pool_shrink(ef_coroutine_pool_t *pool, int idle_millisecs, int max_count);

inline ef_coroutine_t *ef_coroutine_current(void)
{
    ef_fiber_sched_t *rt = ef_get_fiber_sched();
    if(rt->current_fiber == &rt->thread_fiber)
    {
        return NULL;
    }
    return (ef_coroutine_t*)rt->current_fiber;
}

#endif
