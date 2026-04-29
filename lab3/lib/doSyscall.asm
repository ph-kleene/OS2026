[bits 32]

global syscall

syscall:
    push ebp
    mov ebp, esp
    
    ; Save registers that will be used for syscall arguments
    push ebx
    push esi
    push edi

    ; Load arguments into registers
    mov eax, [ebp + 8]  ; num
    mov ebx, [ebp + 12] ; a1
    mov ecx, [ebp + 16] ; a2
    mov edx, [ebp + 20] ; a3
    mov esi, [ebp + 24] ; a4
    mov edi, [ebp + 28] ; a5

    ; System call interrupt
    int 0x80

    ; Restore registers
    pop edi
    pop esi
    pop ebx
    
    pop ebp
    ret
