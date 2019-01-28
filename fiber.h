#ifndef _FIBER_HEADER_
#define _FIBER_HEADER_

#include <limits.h>
#include <sys/mman.h>

#define ERROR_FIBER_EXITED LONG_MIN
#define ERROR_FIBER_NOT_INITED (LONG_MIN+1)

#define FIBER_STATUS_EXITED 0
#define FIBER_STATUS_INITED 1

typedef struct _ef_fiber_t {
    size_t stack_size;
    void *stack_ptr;
    struct _ef_fiber_t *parent;
    long status;
} ef_fiber_t;

typedef struct _ef_fiber_sched_t {
    ef_fiber_t *current_fiber;
    ef_fiber_t thread_fiber;
} ef_fiber_sched_t;

typedef long (*ef_fiber_proc_t)(void *param);

ef_fiber_sched_t *ef_get_fiber_sched(void);

inline void ef_init_fiber_sched(ef_fiber_sched_t *rt) __attribute__((always_inline));
inline void ef_init_fiber(ef_fiber_t *fiber, ef_fiber_proc_t fiber_proc, void *param) __attribute__((always_inline));
inline void ef_delete_fiber(ef_fiber_t *fiber) __attribute__((always_inline));

inline int ef_is_fiber_exited(ef_fiber_t *fiber) __attribute__((always_inline));

ef_fiber_t *ef_create_fiber(size_t stack_size, ef_fiber_proc_t fiber_proc, void *param);

long ef_resume_fiber(ef_fiber_t *_to, long retval);
long ef_yield_fiber(long retval);

void *ef_internal_init_fiber(void *stack_base, ef_fiber_proc_t proc, void *param);

inline void ef_init_fiber_sched(ef_fiber_sched_t *rt)
{
    rt->current_fiber = &rt->thread_fiber;
}

inline void ef_init_fiber(ef_fiber_t *fiber, ef_fiber_proc_t fiber_proc, void *param)
{
    fiber->stack_ptr = ef_internal_init_fiber((char*)fiber + fiber->stack_size, fiber_proc, (param != NULL) ? param : fiber);
    fiber->status = FIBER_STATUS_INITED;
}

inline void ef_delete_fiber(ef_fiber_t *fiber)
{
    munmap(fiber, fiber->stack_size);
}

inline int ef_is_fiber_exited(ef_fiber_t *fiber)
{
    return fiber->status == FIBER_STATUS_EXITED;
}

#endif
