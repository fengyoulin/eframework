.equ FIBER_STACKPTR_OFFSET, 4
.equ FIBER_PARENT_OFFSET, 8
.equ FIBER_STATUS_OFFSET, 12
.equ FIBER_STATUS_EXITED, 0

.global ef_internal_swap_fiber
.global ef_internal_init_fiber

.text

ef_internal_swap_fiber:
mov %esp,%eax
push %edx
push %ecx
push %ebx
push %ebp
push %esi
push %edi
pushfl
mov 8(%eax),%edx
mov %esp,(%edx)
mov 4(%eax),%esp
mov 12(%eax),%eax
_ef_fiber_restore:
popfl
pop %edi
pop %esi
pop %ebp
pop %ebx
pop %ecx
pop %edx
ret

_ef_fiber_exit:
push %eax
call ef_get_fiber_sched
mov (%eax),%edx
mov $FIBER_STATUS_EXITED,%ecx
mov %ecx,FIBER_STATUS_OFFSET(%edx)
mov FIBER_PARENT_OFFSET(%edx),%edx
mov %edx,(%eax)
pop %eax
mov FIBER_STACKPTR_OFFSET(%edx),%esp
jmp _ef_fiber_restore

ef_internal_init_fiber:
push %ebp
mov %esp,%ebp
push %edx
mov 8(%ebp),%edx
mov 16(%ebp),%eax
mov %eax,-4(%edx)
mov $_ef_fiber_exit,%eax
mov %eax,-8(%edx)
mov 12(%ebp),%eax
mov %eax,-12(%edx)
xor %eax,%eax
mov %eax,-16(%edx)
mov %eax,-20(%edx)
mov %eax,-24(%edx)
mov %edx,-28(%edx)
mov %eax,-32(%edx)
mov %eax,-36(%edx)
mov %eax,-40(%edx)
mov %edx,%eax
sub $40,%eax
pop %edx
mov %ebp,%esp
pop %ebp
ret

