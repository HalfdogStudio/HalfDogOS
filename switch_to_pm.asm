[bits 16]
cli             ;必须禁止中断,因为中断向量表即将改变
;开启A20GATE,用键盘控制器的一条线扩展地址线
call waitkbdout
mov al, 0xd1
out 0x64, al
call waitkbdout
mov al, 0xdf
out 0x60, al
call waitkbdout
; 开启保护模式
switch_to_pm:
    lgdt [gdt_descriptor]   ;载入gdt
    
    mov eax, cr0
    and eax, 0x7fffffff ;禁止分页
    or eax, 0x1     ; 设置CR0最低位为1
    mov cr0, eax

    jmp CODE_SEG:init_pm    ;far jump(比如新的段,清空流水线)

[bits 32]
; 初始化保护模式中使用的寄存器和栈
init_pm:
    mov ax, DATA_SEG        ;在保护模式中,段寄存器中是GDT中偏移地址
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

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
    mov ebp, 0x00310000        ;更新栈空间
    mov esp, ebp

    jmp BEGIN_PM

memcpy:
    mov eax, [esi]
    add esi, 4
    mov [edi], eax
    add edi, 4
    sub ecx, 1
    jnz memcpy
    ret

waitkbdout:
    in al, 0x64
    and al, 0x02
    in al, 0x60 ;空读,清楚数据缓冲区垃圾
    jnz waitkbdout
    ret

