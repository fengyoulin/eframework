FIBER_STACKPTR_OFFSET equ 8
FIBER_PARENT_OFFSET   equ 16
FIBER_STATUS_OFFSET   equ 24
FIBER_STATUS_EXITED   equ 0

section .data

section .code

extern ef_get_fiber_sched

global ef_internal_swap_fiber
global ef_internal_init_fiber

ef_internal_swap_fiber:
mov rax,rdx
push rbx
push rbp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
pushfq
mov [rsi],rsp
mov rsp,rdi
_ef_fiber_restore:
popfq
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rbp
pop rbx
ret

_ef_fiber_exit:
push rax
call ef_get_fiber_sched
mov rdx,[rax]
mov rcx,FIBER_STATUS_EXITED
mov [rdx+FIBER_STATUS_OFFSET],rcx
mov rdx,[rdx+FIBER_PARENT_OFFSET]
mov [rax],rdx
pop rax
mov rsp,[rdx+FIBER_STACKPTR_OFFSET]
jmp _ef_fiber_restore

ef_internal_init_fiber:
mov rax,_ef_fiber_exit
mov [rdi-8],rax
mov [rdi-16],rsi
xor rax,rax
mov [rdi-24],rax
mov [rdi-32],rdi
mov [rdi-40],rax
mov [rdi-48],rdx
mov [rdi-56],rax
mov [rdi-64],rax
mov [rdi-72],rax
mov [rdi-80],rax
mov [rdi-88],rax
mov [rdi-96],rax
mov [rdi-104],rax
mov [rdi-112],rax
mov [rdi-120],rax
mov rax,rdi
sub rax,120
ret

