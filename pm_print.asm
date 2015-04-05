[BITS 32]
VIDEO_MEMORY equ 0xb8000    ;字符模式下的VRAM
WHITE_ON_BLACK equ 0x0f

print_string_pm:
    ; esi: 字符
    pusha
    mov edx, VIDEO_MEMORY

print_string_pm_loop:
    mov al, [esi]
    mov ah, WHITE_ON_BLACK

    cmp al, 0
    je print_string_pm_done
    
    mov word [edx], ax
    inc esi
    add edx, 2
    jmp print_string_pm_loop

print_string_pm_done:
    popa
    ret
