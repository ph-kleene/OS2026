[bits 32]

global syscall

; TODO: 实现 syscall 汇编函数
; - 参数：EAX=系统调用号，ECX/EDX/EBX/ESI/EDI=参数1-5
; - 使用栈帧传递参数
; - 保存寄存器
; - 触发软中断
; - 返回值在 EAX 中
; - 恢复寄存器并返回
syscall:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    mov eax, [ebp + 8]
    mov ecx, [ebp + 12]
    mov edx, [ebp + 16]
    mov ebx, [ebp + 20]
    mov esi, [ebp + 24]
    mov edi, [ebp + 28]

    int 0x80

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
