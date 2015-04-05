[BITS 16]
bios_print_string:
    pusha
ch_loop:
    lodsb
    or al, al   ; zero=end or str
    jz finish;    ; get out
    mov ah, 0xe
    int 0x10
    jmp ch_loop

finish:
    popa
    ret


; si 想要打印的地址
bios_print_hex:
    mov di, outstr16
    add di, 2
    mov ax, [si]
    rol ax, 8
    mov si, hexstr
    mov cx, 4   ; four places
hexloop:
    rol ax, 4   ; leftmost will
    mov bx, ax  ; become
    and bx, 0xf ; rightmost
    mov bl, [si + bx]   ; index into hexstr
    mov [di], bl
    inc di
    dec cx
    jnz hexloop

    mov si, outstr16
    call bios_print_string
    ret

hexstr db "0123456789ABCDEF"
outstr16 db '0x0000', 0xa, 0   ;register value string
reg16   dw  0           ;pass values to bios_print_hex

