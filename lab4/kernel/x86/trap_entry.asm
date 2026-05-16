[bits 32]

global irqEmpty
global irqErrorCode
global irqDoubleFault
global irqInvalidTSS
global irqSegNotPresent
global irqStackSegFault
global irqGProtectFault
global irqPageFault
global irqAlignCheck
global irqSecException
global irqSyscall
global irqTimer
global asmDoIrq

extern irqHandle

irqEmpty:
    push 0
    push -1
    jmp asmDoIrq

irqTimer:
    push 0
    push 0x20
    jmp asmDoIrq

irqErrorCode:
    push -1
    jmp asmDoIrq

irqDoubleFault:
    push -1
    jmp asmDoIrq

irqInvalidTSS:
    push -1
    jmp asmDoIrq

irqSegNotPresent:
    push -1
    jmp asmDoIrq

irqStackSegFault:
    push -1
    jmp asmDoIrq

irqGProtectFault:
    push 0xd
    jmp asmDoIrq

irqPageFault:
    push -1
    jmp asmDoIrq

irqAlignCheck:
    push -1
    jmp asmDoIrq

irqSecException:
    push -1
    jmp asmDoIrq

irqSyscall:
    push 0
    push 0x80
    jmp asmDoIrq

asmDoIrq:
    push ds
    push es
    push fs
    push gs
    pushad

    mov ax, 0x10 ; KSEL(SEG_KDATA)
    mov ds, ax
    mov es, ax

    extern current
    mov eax, [current]
    test eax, eax
    jz .no_save
    mov [eax], esp ; pcb->tf = esp
.no_save:

    push esp
    call irqHandle
    add esp, 4

    mov eax, [current]
    test eax, eax
    jz .no_restore
    mov esp, [eax] ; esp = pcb->tf
.no_restore:

    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 8
    iret
