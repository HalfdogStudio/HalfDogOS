[ORG 0xc400]
[BITS 16]

; 第一个
;mov si, welmsg
;
;ch_loop:
;    lodsb
;    or al, al   ; zero=end or str
;    jz finish;    ; get out
;    mov ah, 0xe
;    int 0x10
;    jmp ch_loop
;
;finish:
;    jmp end  ; 这个地址
;
;welmsg db 'welcome halfdog OS^_^', 0xd, 0xa, 0

;---------------------------------
; 第二个
;mov al, 0x13        ;VGA 显卡 320x200x8位彩色
;mov ah, 0x00        ;切换模式
;int 0x10
;----------------------------------
BOOTPACK equ 0x00280000
DSKCAC equ 0x00100000	
DSKCAC0 equ 0x00008000

; BOOT_INFO
cyls equ 0x0ff0
leds equ 0x0ff1
vmode equ 0x0ff2
scrnx equ 0x0ff4
scrny equ 0x0ff6
vram equ 0x0ff8

mov al, 0x13
mov ah, 0x00
int 0x10
mov byte [vmode], 8 ;记录画面模式
mov word [scrnx], 320
mov word [scrny], 200
mov dword [vram], 0x000a0000    ;320x200x8模式下是0xa0000-0xaffff的64kb

; 用BIOS取得键盘上各种LED指示灯状态
mov ah, 0x02
int 0x16

mov [leds], al

end:
    call switch_to_pm

[bits 32]
%include "gdt.asm"
%include "switch_to_pm.asm"
%include "pm_print.asm"

BEGIN_PM:   ;实模式终于开始了

    ;mov esi, MSG_PROT_MODE
    ;call print_string_pm

    ; 准备把bootpack映射到0x280000
    mov esi, 0xc600
    mov edi, BOOTPACK + 0xc600
    mov ecx, 512*1024/4
    call memcpy

    ; 准备把ipl映射到0x7c00
    mov esi, 0x7c00
    mov edi, DSKCAC
    mov ecx, 512/4
    call memcpy

    ; 准备把512字节之后映射到0x8000
    mov esi, DSKCAC0+512
    mov edi, DSKCAC+512
    mov ecx, 0
    mov cl, byte [cyls]
    imul ecx, 512*18*2/4
    sub ecx, 512/4
    call memcpy
    ; 准备栈
    mov esp, 0x00310000
    jmp 0xc600

memcpy:
    mov eax, [esi]
    add esi, 4
    mov [edi], eax
    add edi, 4
    sub ecx, 1
    jnz memcpy
    ret

    ;MSG_PROT_MODE db "Successfully landed in 32-bit Protected Mode", 0xa, 0
