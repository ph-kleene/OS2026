[bits 64]
global mywrite
global mysleep
global myexit

section .text

; long mywrite(int fd, const void *buf, unsigned int count)
mywrite:
    mov eax, 1          ; __NR_write (x86_64)
    syscall
    ret

; int mysleep(unsigned int sec)
; sleep(sec) -> select(0, NULL, NULL, NULL, &tv)
mysleep:
    push rbp
    mov rbp, rsp
    sub rsp, 16

    mov [rsp], rdi      ; tv_sec (long, 8 bytes)
    mov qword [rsp + 8], 0 ; tv_usec

    mov eax, 23         ; __NR_select (x86_64)
    xor edi, edi        ; nfds
    xor esi, esi        ; readfds
    xor edx, edx        ; writefds
    xor r10d, r10d      ; exceptfds
    lea r8, [rsp]       ; timeout
    syscall

    leave
    ret

; void myexit(int status)
myexit:
    mov eax, 60         ; __NR_exit (x86_64)
    syscall
    hlt
