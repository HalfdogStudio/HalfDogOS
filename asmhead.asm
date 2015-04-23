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

; 画面模式
vbemode equ 0x105

; 确认VBE是否存在
mov ax, 0x9000
mov es, ax      ; es=0x9000
mov di, 0       ; di=0
mov ax, 0x4f00  ; ax=0x4f00
int 0x10
cmp ax, 0x004f  ;如果有vbe则变成0x004f
jne scrn320

; 检查vbe版本
mov ax, [es:di+4]
cmp ax, 0x0200  ; vbe>=2.0
jb scrn320

mov cx, vbemode
mov ax, 0x4f01
int 0x10
cmp ax, 0x004f  ;确认下
jne scrn320

;画面模式信息确认
cmp byte [es:di+0x19], 8    ;色深位数
jne scrn320
cmp byte [es:di+0x1b], 4    ;调色板模式
jne scrn320
mov ax, [es:di+0x00]        ;模式属性，bit7要为1
and ax, 0x0080
jz scrn320

;画面切换
mov bx, vbemode+0x4000  ;必须加上0x4000
mov ax, 0x4f02
int 0x10
mov byte [vmode], 8
mov ax, [es:di+0x12]    ;x
mov [scrnx], ax
mov ax, [es:di+0x14]    ;y
mov [scrny], ax
mov eax, [es:di+0x28]    ;vram地址
mov [vram], eax
jmp keystatus

scrn320:
    mov al, 0x13
    mov ah, 0x00
    int 0x10
    mov byte [vmode], 8 ;记录画面模式
    mov word [scrnx], 320
    mov word [scrny], 200
    mov dword [vram], 0x000a0000    ;320x200x8模式下是0xa0000-0xaffff的64kb

; 用BIOS取得键盘上各种LED指示灯状态
keystatus:
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

