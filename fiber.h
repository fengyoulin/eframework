#ifndef _FIBER_HEADER_
#define _FIBER_HEADER_

#include <limits.h>
#include <sys/mman.h>

#define ERROR_FIBER_EXITED LONG_MIN
#define ERROR_FIBER_NOT_INITED (LONG_MIN+1)

#define FIBER_STATUS_EXITED 0
#define FIBER_STATUS_INITED 1

typedef struct _ef_fiber_t ef_fiber_t;
typedef struct _ef_fiber_sched_t ef_fiber_sched_t;

struct _ef_fiber_t {
    size_t stack_size;
    void *stack_area;
    void *stack_upper;
    void *stack_lower;
    void *stack_ptr;
    long status;
    ef_fiber_t *parent;
    ef_fiber_sched_t *sched;
};

struct _ef_fiber_sched_t {
    ef_fiber_t *current_fiber;
    ef_fiber_t thread_fiber;
};

typedef long (*ef_fiber_proc_t)(void *param);

#define ef_is_fiber_exited(fiber) ((fiber)->status == FIBER_STATUS_EXITED)

long ef_resume_fiber(ef_fiber_sched_t *rt, ef_fiber_t *_to, long retval);

long ef_yield_fiber(ef_fiber_sched_t *rt, long retval);

ef_fiber_t *ef_create_fiber(ef_fiber_sched_t *rt, size_t stack_size, size_t header_size, ef_fiber_proc_t fiber_proc, void *param);

int ef_expand_fiber_stack(ef_fiber_t *fiber, void *addr);

int ef_init_fiber_sched(ef_fiber_sched_t *rt, int handle_sigsegv);

void ef_init_fiber(ef_fiber_t *fiber, ef_fiber_proc_t fiber_proc, void *param);

void ef_delete_fiber(ef_fiber_t *fiber);

#endif
