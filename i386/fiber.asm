FIBER_STACKPTR_OFFSET equ 4
FIBER_PARENT_OFFSET   equ 8
FIBER_STATUS_OFFSET   equ 12
FIBER_STATUS_EXITED   equ 0

section .data

section .code

extern ef_get_fiber_sched

global ef_internal_swap_fiber
global ef_internal_init_fiber

ef_internal_swap_fiber:
mov eax,esp
push edx
push ecx
push ebx
push ebp
push esi
push edi
pushfd
mov edx,[eax+8]
mov [edx],esp
mov esp,[eax+4]
mov eax,[eax+12]
_ef_fiber_restore:
popfd
pop edi
pop esi
pop ebp
pop ebx
pop ecx
pop edx
ret

_ef_fiber_exit:
push eax
call ef_get_fiber_sched
mov edx,[eax]
mov ecx,FIBER_STATUS_EXITED
mov [edx+FIBER_STATUS_OFFSET],ecx
mov edx,[edx+FIBER_PARENT_OFFSET]
mov [eax],edx
pop eax
mov esp,[edx+FIBER_STACKPTR_OFFSET]
jmp _ef_fiber_restore

ef_internal_init_fiber:
push ebp
mov ebp,esp
push edx
mov edx,[ebp+8]
mov eax,[ebp+16]
mov [edx-4],eax
mov eax,_ef_fiber_exit
mov [edx-8],eax
mov eax,[ebp+12]
mov [edx-12],eax
xor eax,eax
mov [edx-16],eax
mov [edx-20],eax
mov [edx-24],eax
mov [edx-28],edx
mov [edx-32],eax
mov [edx-36],eax
mov [edx-40],eax
mov eax,edx
sub eax,40
pop edx
mov esp,ebp
pop ebp
ret

