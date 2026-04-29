[bits 32]

global readRTC

; uint8_t readRTC(uint8_t index)
readRTC:
    push ebp
    mov ebp, esp

    ; Wait if RTC is busy (Update In Progress bit)
    ; Register A (0x0A), bit 7 is UIP
.wait_busy:
    mov al, 0x0A
    out 0x70, al
    in al, 0x71
    test al, 0x80
    jnz .wait_busy

    ; Set register index
    mov al, [ebp + 8]
    out 0x70, al
    
    ; Read data from register
    in al, 0x71
    
    movzx eax, al

    pop ebp
    ret
