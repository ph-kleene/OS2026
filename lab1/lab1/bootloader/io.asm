bits 32

global readSect

; Wait for disk ready
waitDisk:
	mov dx, 0x1F7
	in al, dx
	and al, 0xC0
	cmp al, 0x40
	jne waitDisk
	ret

; Read sectors from disk
readSect:
	call waitDisk
	mov edi, [esp + 4]
	mov eax, [esp + 8]

	; Set sector count to 1
	mov dx, 0x1F2
	mov al, 1
	out dx, al

	; Set LBA address (28-bit)
	mov eax, [esp + 8]
	mov dx, 0x1F3
	out dx, al ; LBA 0-7

	shr eax, 8
	inc dx
	out dx, al ; LBA 8-15

	shr eax, 8
	inc dx
	out dx, al ; LBA 16-23

	shr eax, 8
	and al, 0x0F
	or al, 0xE0 ; LBA 24-27 + Drive 0 (Master) + LBA mode
	inc dx
	out dx, al

	; Send read command
	mov dx, 0x1F7
	mov al, 0x20
	out dx, al

	; Wait for data ready
	call waitDisk

	; Read 512 bytes (128 dwords)
	mov dx, 0x1F0
	mov ecx, 128
	rep insd
	ret
