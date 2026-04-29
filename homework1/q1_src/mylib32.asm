[bits 32]
global mywrite
global mysleep
global myexit

section .text

; long mywrite(int fd, const void *buf, unsigned int count)
mywrite:
    push ebp
    mov ebp, esp
    push ebx

    mov eax, 4          ; __NR_write (x86)
    mov ebx, [ebp + 8]  ; fd
    mov ecx, [ebp + 12] ; buf
    mov edx, [ebp + 16] ; count
    int 0x80

    pop ebx
    pop ebp
    ret

; int mysleep(unsigned int sec)
; sleep(sec) -> select(0, NULL, NULL, NULL, &tv)
mysleep:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    sub esp, 8
    mov eax, [ebp + 8]
    mov [esp], eax      ; tv_sec
    mov dword [esp + 4], 0 ; tv_usec

    mov eax, 142        ; __NR_select (x86)
    xor ebx, ebx        ; nfds = 0
    xor ecx, ecx        ; readfds = NULL
    xor edx, edx        ; writefds = NULL
    xor esi, esi        ; exceptfds = NULL
    mov edi, esp        ; timeout = &tv
    int 0x80

    add esp, 8
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; void myexit(int status)
myexit:
    mov eax, 1          ; __NR_exit (x86)
    mov ebx, [esp + 4]  ; status
    int 0x80
    hlt
