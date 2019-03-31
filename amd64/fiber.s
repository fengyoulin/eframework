.equ FIBER_STACKPTR_OFFSET, 16
.equ FIBER_STATUS_OFFSET, 24
.equ FIBER_PARENT_OFFSET, 32
.equ FIBER_SCHED_OFFSET, 40
.equ FIBER_STATUS_EXITED, 0
.equ FIBER_STATUS_INITED, 1

.global ef_internal_swap_fiber
.global ef_internal_init_fiber

.text

ef_internal_swap_fiber:
mov %rdx,%rax
push %rbx
push %rbp
push %rsi
push %rdi
push %r8
push %r9
push %r10
push %r11
push %r12
push %r13
push %r14
push %r15
pushfq
mov %rsp,(%rsi)
mov %rdi,%rsp
_ef_fiber_restore:
popfq
pop %r15
pop %r14
pop %r13
pop %r12
pop %r11
pop %r10
pop %r9
pop %r8
pop %rdi
pop %rsi
pop %rbp
pop %rbx
ret

_ef_fiber_exit:
pop %rdx
mov $FIBER_STATUS_EXITED,%rcx
mov %rcx,FIBER_STATUS_OFFSET(%rdx)
mov FIBER_PARENT_OFFSET(%rdx),%rcx
mov FIBER_SCHED_OFFSET(%rdx),%rdx
mov %rcx,(%rdx)
mov FIBER_STACKPTR_OFFSET(%rcx),%rsp
jmp _ef_fiber_restore

ef_internal_init_fiber:
mov $FIBER_STATUS_INITED,%rax
mov %rax,FIBER_STATUS_OFFSET(%rdi)
mov %rdi,%rcx
mov (%rdi),%rax
add %rax,%rdi
mov %rcx,-8(%rdi)
mov $_ef_fiber_exit,%rax
mov %rax,-16(%rdi)
mov %rsi,-24(%rdi)
xor %rax,%rax
mov %rax,-32(%rdi)
mov %rdi,-40(%rdi)
mov %rax,-48(%rdi)
mov %rdx,-56(%rdi)
mov %rax,-64(%rdi)
mov %rax,-72(%rdi)
mov %rax,-80(%rdi)
mov %rax,-88(%rdi)
mov %rax,-96(%rdi)
mov %rax,-104(%rdi)
mov %rax,-112(%rdi)
mov %rax,-120(%rdi)
mov %rax,-128(%rdi)
mov %rdi,%rax
sub $128,%rax
ret

