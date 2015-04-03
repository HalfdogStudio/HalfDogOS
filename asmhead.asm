[ORG 0xc400]

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
    hlt
    jmp end

