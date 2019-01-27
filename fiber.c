#include <unistd.h>
#include <sys/mman.h>
#include "fiber.h"

long ef_internal_swap_fiber(void *new_sp, void **old_sp_ptr, long retval);

ef_fiber_t *ef_create_fiber(size_t stack_size, ef_fiber_proc_t fiber_proc, void *param)
{
    long page_size = sysconf(_SC_PAGESIZE);
    if(page_size < 0)
    {
        return NULL;
    }
    if(stack_size == 0)
    {
        stack_size = (size_t)page_size;
    }
    stack_size = (size_t)((stack_size + page_size - 1) & ~(page_size - 1));
    void *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(MAP_FAILED == stack)
    {
        return NULL;
    }
    ef_fiber_t *fiber = (ef_fiber_t*)stack;
    fiber->stack_size = stack_size;
    ef_init_fiber(fiber, fiber_proc, param);
    return fiber;
}

long ef_resume_fiber(ef_fiber_t *_to, long retval)
{
    if(_to->status != FIBER_STATUS_INITED)
    {
        if(_to->status == FIBER_STATUS_EXITED)
        {
            return ERROR_FIBER_EXITED;
        }
        return ERROR_FIBER_NOT_INITED;
    }
    ef_fiber_sched_t *rt = ef_get_fiber_sched();
    ef_fiber_t *_current = rt->current_fiber;
    _to->parent = _current;
    rt->current_fiber = _to;
    return ef_internal_swap_fiber(_to->stack_ptr, &_current->stack_ptr, retval);
}

long ef_yield_fiber(long retval)
{
    ef_fiber_sched_t *rt = ef_get_fiber_sched();
    ef_fiber_t *_current = rt->current_fiber;
    rt->current_fiber = _current->parent;
    return ef_internal_swap_fiber(_current->parent->stack_ptr, &_current->stack_ptr, retval);
}
