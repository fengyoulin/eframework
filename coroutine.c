#include "coroutine.h"

void ef_coroutine_pool_init(ef_coroutine_pool_t *pool, size_t stack_size, int limit_min, int limit_max)
{
    ef_init_fiber_sched(&pool->fiber_sched, 1);
    pool->stack_size = stack_size;
    pool->limit_min = limit_min;
    pool->limit_max = limit_max;
    ef_list_init(&pool->full_list);
    ef_list_init(&pool->free_list);
    pool->full_count = 0;
    pool->free_count = 0;
}

ef_coroutine_t *ef_coroutine_create(ef_coroutine_pool_t *pool, size_t header_size, ef_coroutine_proc_t fiber_proc, void *param)
{
    ef_coroutine_t *co = NULL;
    if(pool->free_count > 0)
    {
        --pool->free_count;
        co = CAST_PARENT_PTR(ef_list_remove_after(&pool->free_list), ef_coroutine_t, free_entry);
        ef_init_fiber(&co->fiber, fiber_proc, param);
        return co;
    }
    if(pool->full_count >= pool->limit_max)
    {
        return NULL;
    }
    ef_fiber_t *fiber = ef_create_fiber(&pool->fiber_sched, pool->stack_size, header_size, fiber_proc, param);
    if(fiber == NULL)
    {
        return NULL;
    }
    co = (ef_coroutine_t*)fiber;
    ++pool->full_count;
    ef_list_insert_after(&pool->full_list, &co->full_entry);
    return co;
}

long ef_coroutine_resume(ef_coroutine_pool_t *pool, ef_coroutine_t *co, long to_yield)
{
    long retval = ef_resume_fiber(&pool->fiber_sched, &co->fiber, to_yield);
    if(ef_is_fiber_exited(&co->fiber) && retval != ERROR_FIBER_EXITED)
    {
        gettimeofday(&co->last_run_time, NULL);
        ef_list_insert_after(&pool->free_list, &co->free_entry);
        ++pool->free_count;
    }
    return retval;
}

int ef_coroutine_pool_shrink(ef_coroutine_pool_t *pool, int idle_millisecs, int max_count)
{
    if(pool->free_count <= 0 || (max_count > 0 && pool->full_count <= pool->limit_min))
    {
        return 0;
    }
    int beyond_min = pool->full_count - pool->limit_min;
    if(max_count > beyond_min)
    {
        max_count = beyond_min;
    }
    if(max_count < 0)
    {
        max_count = -max_count;
    }
    int free_count = 0;
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    ef_list_entry_t *list_tail = ef_list_entry_before(&pool->free_list);
    while(list_tail != &pool->free_list && max_count--)
    {
        ef_coroutine_t *co = CAST_PARENT_PTR(list_tail, ef_coroutine_t, free_entry);
        list_tail = ef_list_entry_before(list_tail);
        if(((tv.tv_sec - co->last_run_time.tv_sec) * 1000 > idle_millisecs) ||
            (((tv.tv_sec - co->last_run_time.tv_sec) * 1000 == idle_millisecs) && tv.tv_usec - co->last_run_time.tv_usec >= idle_millisecs % 1000))
        {
            --pool->free_count;
            --pool->full_count;
            ef_list_remove(&co->free_entry);
            ef_list_remove(&co->full_entry);
            ++free_count;
            ef_delete_fiber(&co->fiber);
        }
        else
        {
            break;
        }
    }
    return free_count;
}
