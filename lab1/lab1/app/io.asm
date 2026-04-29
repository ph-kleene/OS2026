bits 32

global displayStr
global clearStr
global readRTC

; Display string at row/col
displayStr:
	push ebx
	push edi
	mov edx, [esp+12]
	mov eax, [esp+16]
	imul edx, edx, 80
	add edx, eax
	shl edx, 1
	mov edi, edx
	mov ebx, [esp+20]
	mov ecx, [esp+24]
	mov ah, 0x14
nextChar:
	mov al, [ebx]
	mov [gs:edi], ax
	add edi, 2
	inc ebx
	loop nextChar
	pop edi
	pop ebx
	ret

; Clear string at row/col
clearStr:
	push edi
	mov edx, [esp+8]
	mov eax, [esp+12]
	imul edx, edx, 80
	add edx, eax
	shl edx, 1
	mov edi, edx
	mov ecx, [esp+16]
	mov ax, 0x1f20
clearLoop:
	mov [gs:edi], ax
	add edi, 2
	loop clearLoop
	pop edi
	ret

; Read from RTC register
readRTC:
	; TODO: 实现readRTC，函数定义见main.h
	mov eax, [esp+4]
	and al, 0x7f
	or al, 0x80
	mov dx, 0x70
	out dx, al
	mov dx, 0x71
	in al, dx
	movzx eax, al
	ret
