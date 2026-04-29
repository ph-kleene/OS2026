[bits 32]
global initTimer

initTimer:
    mov al, 0x34 ; Channel 0, lobyte/hibyte, Mode 2
    out 0x43, al
    
    ; Divisor = 1193182 / 100 = 11931.8 -> 11932 (0x2E9C) for 10ms
    mov ax, 11932
    out 0x40, al ; Low byte
    mov al, ah
    out 0x40, al ; High byte
    ret
