[bits 16]
;GDT
; 发现我对GDT的理解有误,参见有道云笔记 (http://note.youdao.com/share/?id=fc82ff38b404868cb7f8df2eec2e7bb7&type=note)

gdt_start:

gdt_null:
    dd 0x0
    dd 0x0

gdt_data: ; code segment descriptor
    ; base=0x0, limit=0xfffff
    ; 1st flag:(present)1 (privilege)00 (descriptor type)1 -> 1001b
    ; type flags: (code)0 (expand down)0 (writable)1 (accessed)0 -> 0010b
    ; 2nd flags: (granularity)1 (32bit default)1 (64-bit seg)0 (AVL)0->1100b
    dw 0xffff      ;limit (bits 0-15)
    dw 0x0000         ;Base(bits 0-15)
    db 0x00          ;Base(bits 16-23)
    db 0x92    ;1st flags, type flags
    db 0xcf    ;2nd flags, Limit(bits 16-19)
    db 0x0          ; base(bits 24-31)

gdt_code: ; code segment descriptor
    ; base=0x000000, limit=0xfffff
    ; 1st flag:(present)1 (privilege)00 (descriptor type)1 -> 1001b
    ; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    ; 2nd flags: (granularity)1 (32bit default)1 (64-bit seg)0 (AVL)0->0011b
    dw 0xffff      ;limit (bits 0-15)
    dw 0x0000          ;Base(bits 0-15)
    db 0x28          ;Base(bits 16-23)
    db 0x9a    ;1st flags, type flags
    db 0x47    ;2nd flags, Limit(bits 16-19)
    db 0x00          ; base(bits 24-31)

gdt_end:    ;要计算长度
; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start -1   ; Size of our GDT, always less one of true size
    dd gdt_start                ; start address of our GDT

; Define some handy constants for the GDT segment descriptor offsets , which
; are what segment registers must contain when in protected mode. For example ,
; when we set DS = 0 x10 in PM , the CPU knows that we mean it to use the
; segment described at offset 0 x10 ( i.e. 16 bytes ) in our GDT , which in our
; case is the DATA segment (0 x0 -> NULL ; 0 x08 -> DATA ; 0 x10 -> CODE )
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

