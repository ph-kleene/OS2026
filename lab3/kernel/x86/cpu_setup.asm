[BITS 32]

global switch_to_user
global refresh_kernel_segments

section .text

; void refresh_kernel_segments(uint16_t data_sel, uint16_t tss_sel);
refresh_kernel_segments:
    mov ax, [esp + 4]   ; Load data_sel (kernel data segment)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ax, [esp + 8]   ; Load tss_sel
    ltr ax              ; Load Task Register

    xor ax, ax          ; Use 0 for LDT (not used in this project)
    lldt ax             ; Load LDT register
    ret

; void switch_to_user(uint32_t entry, uint32_t user_ds, uint32_t user_cs, uint32_t user_esp);
switch_to_user:
    mov eax, [esp + 4]  ; entry
    mov ebx, [esp + 8]  ; user_ds
    mov ecx, [esp + 12] ; user_cs
    mov edx, [esp + 16] ; user_esp

    ; 1. Load data segment registers
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    ; 2. Construct iret stack frame: [SS, ESP, EFLAGS, CS, EIP]
    push ebx            ; User SS
    push edx            ; User ESP
    
    pushf               ; Push current EFLAGS
    pop edx
    or edx, 0x200       ; Set IF=1
    push edx            ; User EFLAGS
    
    push ecx            ; User CS
    push eax            ; User EIP

    ; 3. Perform iret
    iret
