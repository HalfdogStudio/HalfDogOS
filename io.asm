[BITS 32]

; 只有几个寄存器能用,eax,ebx,ecx,edx,esi,edi
;------------------------------------
global write_mem8
; sti, hlt, cli
global io_hlt
global io_cli
global io_sti
global io_stihlt
; io_in
global io_in8
global io_in16
global io_in32
; io_out
global io_out8
global io_out16
global io_out32
; eflags
global io_load_eflags
global io_store_eflags

;------------------------------------
io_hlt:
    hlt
    ret

io_cli:
    cli
    ret

io_sti:
    sti
    ret

io_stihlt:  ; void io_stihlt(void)
    sti
    hlt
    ret

;-----------------------------------
;nasm manual中:port不是8位立即数就是dx, 读到eax中
io_in8:     ; int io_int8(int port)
    mov edx, [esp+4]
    mov eax, 0
    in al, dx
    ret

io_in16:     ; int io_int16(int port)
    mov edx, [esp+4]
    mov eax, 0
    in ax, dx
    ret

io_in32:     ; int io_int32(int port)
    mov edx, [esp+4]
    in eax, dx
    ret

;------------------------------------
; nasm manual中有写: port要么是8位立即数要么是DX
io_out8:     ; int io_out8(int port, char data)
    mov edx, [esp+4]
    mov eax, [esp+8]
    out dx, al
    ret

io_out16:     ; int io_out8(int port, short data)
    mov edx, [esp+4]
    mov eax, [esp+8]
    out dx, ax
    ret

io_out32:     ; int io_out8(int port, int data)
    mov edx, [esp+4]
    mov eax, [esp+8]
    out dx, eax
    ret

io_load_eflags:     ;int io_load_eflags(void);
    pushfd
    pop eax
    ret

io_store_eflags:    ;void io_store_eflags(int eflags)
    mov eax, [esp+4]
    push eax
    popfd
    ret


write_mem8:
    mov ecx, [esp+4]
    mov al, [esp+8]
    mov [ecx], al
    ret



