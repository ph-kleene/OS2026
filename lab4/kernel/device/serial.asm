[bits 32]

global initSerial
global putChar
global putStr
global putNum

%define SERIAL_PORT 0x3F8

initSerial:
    mov dx, SERIAL_PORT + 1
    mov al, 0x00
    out dx, al

    mov dx, SERIAL_PORT + 3
    mov al, 0x80
    out dx, al

    mov dx, SERIAL_PORT + 0
    mov al, 0x01
    out dx, al

    mov dx, SERIAL_PORT + 1
    mov al, 0x00
    out dx, al

    mov dx, SERIAL_PORT + 3
    mov al, 0x03
    out dx, al

    mov dx, SERIAL_PORT + 2
    mov al, 0xC7
    out dx, al

    mov dx, SERIAL_PORT + 4
    mov al, 0x0B
    out dx, al
    ret

putChar:
    mov dx, SERIAL_PORT + 5
.wait:
    in al, dx
    and al, 0x20
    jz .wait
    
    mov al, [esp + 4]
    mov dx, SERIAL_PORT
    out dx, al
    ret

putStr:
    push ebp
    mov ebp, esp
    push ebx
    mov ebx, [ebp + 8]
.loop:
    movzx eax, byte [ebx]
    test al, al
    jz .done
    push eax
    call putChar
    add esp, 4
    inc ebx
    jmp .loop
.done:
    pop ebx
    pop ebp
    ret

putNum:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, [ebp + 8]
    test eax, eax
    jnz .not_zero
    push '0'
    call putChar
    add esp, 4
    jmp .done
.not_zero:
    jns .positive
    push '-'
    call putChar
    add esp, 4
    mov eax, [ebp + 8]
    neg eax
.positive:
    mov ebx, 10
    mov ecx, 0
.div_loop:
    test eax, eax
    jz .print_loop
    mov edx, 0
    div ebx
    add edx, '0'
    push edx
    inc ecx
    jmp .div_loop
.print_loop:
    test ecx, ecx
    jz .done
    call putChar
    add esp, 4
    dec ecx
    jmp .print_loop
.done:
    pop ebx
    pop ebp
    ret
