[bits 32]

extern main
global _start

_start:
    ; Init user data segments (ds, es, fs, gs)
    ; ss is already set by kernel's iret
    mov ax, ss
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call the C entry point
    call main

    ; If uEntry returns, loop forever
.halt:
    jmp .halt
