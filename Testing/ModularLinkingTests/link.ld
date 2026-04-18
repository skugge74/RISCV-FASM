ENTRY(_start)
SECTIONS
{
    . = 0x10000;          /* Start code at 64KB */
    .text : { *(.text) }
    
    . = 0x20000;          /* Start data at 128KB */
    .data : { *(.data) }
    
    .bss  : { *(.bss)  }
}
