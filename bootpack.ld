ENTRY(HalfDogMain)

SECTIONS {
    . = 0xc600;

    .text ALIGN(4):
    {
        *(.text*)
    }

    .data ALIGN(4):
    {
        *(.data)
    }
}
