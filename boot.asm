; boot.asm
[BITS 16]
%macro dprint_string 1
    ;-------------
    push si
    mov si, word %1
    call bios_print_string
    pop si
    ;----------------
%endmacro
%macro dprint_hex 1
    ;-------------
    push si
    mov si, word %1
    call bios_print_hex
    pop si
    ;----------------
%endmacro

cyls equ 10  ; 在bochs中我不能载入超过64个扇区,不知道为什么.参见我的有道云笔记.但qemu可以

[ORG 0x7c00]
; fat12 header from mkfs.fat & fdisk
jmp entry
db 0x90, 0x6d, 0x6b, 0x66, 0x73, 0x2e, 0x66, 0x61, 0x74, 0x00, 0x02, 0x01, 0x01, 0x00
db 0x02, 0xe0, 0x00, 0x40, 0x0b, 0xf0, 0x09, 0x00, 0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00
db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x44, 0x01, 0x5a, 0x2e, 0x4e, 0x4f, 0x20, 0x4e, 0x41
db 0x4d, 0x45, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20
;jmp entry
;db 0x90
;db "HARIBOTE"
;dw 512
;db 1
;dw 1
;db 2
;dw 224
;dw 2880
;db 0xf0
;dw 9
;dw 18
;dw 2
;dd 0
;dd 2880
;db 0,0,0x29
;dd 0xffffffff
;db "HARIBOTEOS"
;db "FAT12   "

entry:
    xor ax, ax  ; make it zero
    mov ds, ax
    ; 准备栈
    mov bp, 0x8000
    mov sp, ax
    mov si, MSG_REAL_MODE
    call bios_print_string
    ;mov ax, 0x07c0
    ;mov ds, ax
    ; 不要假设什么,但似乎也没影响

; start read first 512 byte
; Cylinder = 0 to 1023 (maybe 4095), Head = 0 to 15 (maybe 254, maybe 255), Sector = 1 to 63
; 
;     Set AH = 2
;     AL = total sector count (0 is illegal) -- cannot cross ES page boundary, or a cylinder boundary, and must be < 128
;     CH = cylinder & 0xff
;     CL = Sector | ((cylinder >> 2) & 0xC0);
;     DH = Head -- may include two more cylinder bits
;     ES:BX -> buffer
;     Set DL = "drive number" -- typically 0x80, for the "C" drive
;     Issue an INT 0x13. 
; 
; The carry flag will be set if there is any error during the read. AH should be set to 0 on success. 
; 不过我们考虑80(柱面)*18(扇区)*2磁头就好
load_from_floppy:
    mov ax, 0x0820  ; 磁盘内容读到这里,自己定的es:bx = 0x8200
    mov es, ax      ; es=ax
    mov ch, 0       ; 柱面0
    mov dh, 0       ; 磁头0
    mov cl, 2       ; 扇区2
readloop:
    mov si, 0       ; 记录试错次数
retry:
    mov ah, 0x02    ; 读盘
    mov al, 1       ; 1个扇区
    mov bx, 0       ; es:bx
    mov dl, 0x0     ; A驱动器
    int 0x13        ; 调用磁盘BIOS
    jnc next         ; 如果没进位就是成功
    add si, 1
    cmp si, 5
    jae disk_error
    mov ah, 0x0
    mov dl, 0x0     ;驱动器号A
    int 0x13        ; 重置驱动器
    jmp retry
next:
    mov ax, es
    add ax, 0x0020
    mov es, ax      ;地址后移512字节(0x200)
    inc cl       ; 下一个扇区
    cmp cl, 18
    jbe readloop    ; 读完18个扇区
    ; 下一个磁头
    mov cl, 1       ; 恢复扇区1
    add dh, 1       ; 磁头1
    cmp dh, 2
    jb readloop     ;两个
    ; 下一个柱面
    mov dh, 0       ;磁头0
    inc ch       ; 下一个柱面
    cmp ch, cyls
    jb readloop

    dprint_string read_msg
    mov [0x0ff0], ch
    ; halfdog.bin的位置
    dprint_hex 0xc400
    ; 下一个512字节位置
    jmp 0xc400

disk_error:
    dprint_string disk_error_msg
    jmp $

disk_error_msg db "Load error:(", 0xa, 0
read_msg db "Read disk finished:)", 0xa, 0

%include "bios_print.asm"

MSG_REAL_MODE db "Started in 16-bit Real Mode", 0xa, 0

    times 510-($-$$) db 0   ;2 bytes less
    db 0x55
    db 0xaa
    ;dw 0xaa55

