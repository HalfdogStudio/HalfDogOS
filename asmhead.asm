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

;------------
; 320 x 200 画面模式
mov al, 0x13
mov ah, 0x00
int 0x10
mov byte [vmode], 8 ;记录画面模式
mov word [scrnx], 320
mov word [scrny], 200
mov dword [vram], 0x000a0000    ;320x200x8模式下是0xa0000-0xaffff的64kb
;---------------

; 用BIOS取得键盘上各种LED指示灯状态
mov ah, 0x02
int 0x16

mov [leds], al
;--------------------------------
; PIC关闭一切中断
mov al, 0xff
out 0x21, al    ;禁止PIC0中断
nop     ;某些奇葩机种不能连续out
out 0xa1, al    ;禁止PIC1中断

call switch_to_pm

[bits 32]
%include "gdt.asm"
%include "switch_to_pm.asm"
%include "pm_print.asm"

BEGIN_PM:   ;实模式终于开始了

    ;更新CS
    jmp CODE_SEG:0xc600
    ;mov esi, MSG_PROT_MODE
    ;call print_string_pm


    ;MSG_PROT_MODE db "Successfully landed in 32-bit Protected Mode", 0xa, 0

waitkbdout:
    in al, 0x64
    and al, 0x02
    in al, 0x60 ;空读,清楚数据缓冲区垃圾
    jnz waitkbdout
    ret

