ENTRY(HalfDogMain)

SECTIONS {
    . = 0xc600;

    .text :
    {
        *(.text*)
    }

    . = 0xe540;
    .data  :
    {
        *(.data)
    }

    .rodata  :
    {
        *(.rodata*)
    }

    .bss  :
    {
        *(COMMON)
        *(.bss)
    }
}
