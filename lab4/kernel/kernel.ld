SECTIONS {
    . = 0x00010000;
    .text : {
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
        *(COMMON)
    }
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
