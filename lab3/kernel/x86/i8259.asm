[bits 32]

global initIntr

%define PORT_PIC_MASTER 0x20
%define PORT_PIC_SLAVE  0xA0

initIntr:
    ; ICW1: Init command
    mov al, 0x11
    out PORT_PIC_MASTER, al
    out PORT_PIC_SLAVE, al

    ; ICW2: Vector offset
    mov al, 32
    out PORT_PIC_MASTER + 1, al
    mov al, 40
    out PORT_PIC_SLAVE + 1, al

    ; ICW3: Cascade
    mov al, 4
    out PORT_PIC_MASTER + 1, al
    mov al, 2
    out PORT_PIC_SLAVE + 1, al

    ; ICW4: Mode
    mov al, 0x03
    out PORT_PIC_MASTER + 1, al
    out PORT_PIC_SLAVE + 1, al

    ret
