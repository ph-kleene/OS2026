%define BOOT_STACK_TOP 0x10000

bits 16

global start
extern bootMain

start:
	; Init segments
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	cli

	; Clear screen
	mov ax, 0x0600
	mov bh, 0x07
	mov cx, 0x0000
	mov dx, 0x184f
	int 0x10

	; Disable cursor
	mov dx, 0x3d4
	mov al, 0x0a
	out dx, al
	mov dx, 0x3d5
	mov al, 0x20
	out dx, al

	; Enable A20
	in al, 0x92
	or al, 0x02
	out 0x92, al

	; Load GDT
	lgdt [gdtDesc]

	; Enable protected mode
	mov eax, cr0
	or al, 1
	mov cr0, eax

	; Jump to 32-bit code
	jmp dword 0x08:start32

bits 32
start32:
	; Set segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax
	mov ax, 0x18
	mov gs, ax

	; Init stack and call bootMain
	mov eax, BOOT_STACK_TOP
	mov esp, eax
	jmp bootMain

align 4
gdt:
	dw 0, 0
	db 0, 0, 0, 0

	dw 0xffff, 0
	db 0, 0x9a, 0xcf, 0

	dw 0xffff, 0
	db 0, 0x92, 0xcf, 0

	dw 0xffff, 0x8000
	db 0x0b, 0x92, 0xcf, 0

gdtDesc:
	dw gdtDesc - gdt - 1
	dd gdt
