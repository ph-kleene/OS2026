[bits 32]

global waitDisk
global readSect

waitDisk:
    push edx
    push eax
.wait:
    mov dx, 0x1F7
    in al, dx
    and al, 0xC0
    cmp al, 0x40
    jne .wait
    pop eax
    pop edx
    ret

readSect:
    push ebp
    mov ebp, esp
    pushad

    ; [ebp + 8]  : dst
    ; [ebp + 12] : offset

    call waitDisk

    mov dx, 0x1F2
    mov al, 1
    out dx, al

    mov eax, [ebp + 12]
    mov dx, 0x1F3
    out dx, al

    mov eax, [ebp + 12]
    shr eax, 8
    mov dx, 0x1F4
    out dx, al

    mov eax, [ebp + 12]
    shr eax, 16
    mov dx, 0x1F5
    out dx, al

    mov eax, [ebp + 12]
    shr eax, 24
    and al, 0x0F
    or al, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

    call waitDisk

    mov edi, [ebp + 8]
    mov ecx, 512 / 4
    mov dx, 0x1F0
.read_loop:
    in eax, dx
    stosd
    loop .read_loop

    popad
    pop ebp
    ret
