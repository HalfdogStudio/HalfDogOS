[bits 16]
; 开启保护模式
switch_to_pm:
    cli             ;必须禁止中断,因为中断向量表即将改变
    lgdt [gdt_descriptor]   ;载入gdt
    
    mov eax, cr0
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

    mov ebp, 0x90000        ;更新栈空间
    mov esp, ebp

    jmp BEGIN_PM

