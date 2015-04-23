ENTRY(HalfDogMain)

SECTIONS {
    . = 0xc600;

    .text :
    {
        *(.text*)
    }

    .data :
    {
        *(.data*)
    }
}
