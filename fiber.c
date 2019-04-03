#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include "fiber.h"

static long _ef_page_size = 0;
static ef_fiber_sched_t *_ef_fiber_sched = NULL;

long ef_internal_swap_fiber(void *new_sp, void **old_sp_ptr, long retval);

ef_fiber_t *ef_create_fiber(ef_fiber_sched_t *rt, size_t stack_size, size_t header_size, ef_fiber_proc_t fiber_proc, void *param)
{
    long page_size = _ef_page_size;
    if(stack_size == 0)
    {
        stack_size = (size_t)page_size;
    }
    stack_size = (size_t)((stack_size + page_size - 1) & ~(page_size - 1));
    void *stack = mmap(NULL, stack_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(MAP_FAILED == stack)
    {
        return NULL;
    }
    if(mprotect((char *)stack + stack_size - page_size, page_size, PROT_READ | PROT_WRITE) < 0)
    {
        return NULL;
    }
    ef_fiber_t *fiber = (ef_fiber_t*)((char *)stack + stack_size - header_size);
    fiber->stack_size = stack_size;
    fiber->stack_area = stack;
    fiber->stack_upper = (char *)stack + stack_size - header_size;
    fiber->stack_lower = (char *)stack + stack_size - page_size;
    fiber->sched = rt;
    ef_init_fiber(fiber, fiber_proc, param);
    return fiber;
}

long ef_resume_fiber(ef_fiber_sched_t *rt, ef_fiber_t *_to, long retval)
{
    if(_to->status != FIBER_STATUS_INITED)
    {
        if(_to->status == FIBER_STATUS_EXITED)
        {
            return ERROR_FIBER_EXITED;
        }
        return ERROR_FIBER_NOT_INITED;
    }
    ef_fiber_t *_current = rt->current_fiber;
    _to->parent = _current;
    rt->current_fiber = _to;
    return ef_internal_swap_fiber(_to->stack_ptr, &_current->stack_ptr, retval);
}

long ef_yield_fiber(ef_fiber_sched_t *rt, long retval)
{
    ef_fiber_t *_current = rt->current_fiber;
    rt->current_fiber = _current->parent;
    return ef_internal_swap_fiber(_current->parent->stack_ptr, &_current->stack_ptr, retval);
}

int ef_expand_fiber_stack(ef_fiber_t *fiber, void *addr)
{
    int retval = -1;
    void *lower = (char *)((long)addr & ~(_ef_page_size - 1));
    if(lower >= fiber->stack_area && lower < fiber->stack_lower)
    {
        size_t size = (char *)fiber->stack_lower - (char *)lower;
        retval = mprotect(lower, size, PROT_READ | PROT_WRITE);
        if(retval >= 0)
        {
            fiber->stack_lower = lower;
        }
    }
    return retval;
}

void _ef_fiber_sigsegv_handler(int sig, siginfo_t *info, void *ucontext)
{
    if(SIGSEGV == sig)
    {
        ef_expand_fiber_stack(_ef_fiber_sched->current_fiber, info->si_addr);
    }
}

int ef_init_fiber_sched(ef_fiber_sched_t *rt, int handle_sigsegv)
{
    _ef_fiber_sched = rt;
    rt->current_fiber = &rt->thread_fiber;
    _ef_page_size = sysconf(_SC_PAGESIZE);
    if(_ef_page_size < 0)
    {
        return -1;
    }

    if(!handle_sigsegv)
    {
        return 0;
    }

    stack_t ss;
    ss.ss_sp = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(ss.ss_sp == NULL)
    {
        return -1;
    }
    ss.ss_size = 4096;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1)
    {
        return -1;
    }

    struct sigaction sa = {0};
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = _ef_fiber_sigsegv_handler;
    return sigaction(SIGSEGV, &sa, NULL);
}
