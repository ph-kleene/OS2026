[bits 32]

global displayRow
global displayCol
global displayMem
global initVga
global clearScreen
global updateCursor
global scrollScreen

section .data
displayRow dd 0
displayCol dd 0

section .bss
displayMem resw 80 * 25

section .text
initVga:
    mov dword [displayRow], 0
    mov dword [displayCol], 0
    call clearScreen
    push 0
    push 0
    call updateCursor
    add esp, 8
    ret

clearScreen:
    push ebp
    mov ebp, esp
    push edi
    mov edi, 0xb8000
    mov eax, 0x0c00
    mov ecx, 80 * 25
    rep stosw
    pop edi
    pop ebp
    ret

updateCursor:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    imul eax, 80
    add eax, [ebp + 12]
    mov ecx, eax

    mov dx, 0x3d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x3d5
    mov eax, ecx
    out dx, al

    mov dx, 0x3d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x3d5
    mov eax, ecx
    shr eax, 8
    out dx, al

    pop ebp
    ret

scrollScreen:
    push ebp
    mov ebp, esp
    push esi
    push edi

    ; Backup to displayMem
    mov esi, 0xb8000
    mov edi, displayMem
    mov ecx, 80 * 25
    rep movsw

    ; Scroll up in video memory
    mov esi, displayMem + 80 * 2
    mov edi, 0xb8000
    mov ecx, 80 * 24
    rep movsw

    ; Clear last row
    mov edi, 0xb8000 + 80 * 24 * 2
    mov eax, 0x0c00
    mov ecx, 80
    rep stosw

    pop edi
    pop esi
    pop ebp
    ret
