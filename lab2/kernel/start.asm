[bits 32]
extern kEntry
global _start

_start:
    call kEntry
    jmp $ ; If kEntry returns, hang
