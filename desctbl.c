#include "bootpack.h"

void init_gdtidt(void){
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)0x00270000;
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *)0x26f800;
    // GDT的初始化
    int i;
    for (i = 0; i < 8192; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);  //data
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);  //code 512k
    set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);  //code 512k
    set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);  //code 512k
    load_gdtr(0xffff, 0x00270000);
    // IDT初始化, 256个中断
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(0x7ff, 0x0026f800);

    // 注册键盘中断
    set_gatedesc(idt + 0x21, (unsigned int) asm_inthandler21, 2 * 8, AR_INTGATE32); // seg 2 is code
    // 注册鼠标中断
    set_gatedesc(idt + 0x2c, (unsigned int) asm_inthandler2c, 2 * 8, AR_INTGATE32); // seg 2 is code
    // 注册定时器中断
    set_gatedesc(idt + 0x20, (unsigned int) asm_inthandler20, 2 * 8, AR_INTGATE32); // seg 2 is code

    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
    if (limit > 0xfffff) {
        ar |= 0x8000;   // G_bit = 1
        limit /= 0x1000;    // 用4kb计量 0x1000=4096=4kb
    }
    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high= (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, unsigned int offset, int selector, int ar){
    gd->offset_low = offset & 0xffff;
    gd->selector = selector & 0xffff;
    gd->dw_count = (ar >> 8) & 0xff;    //must be zero
    gd->access_right= ar  & 0xff;
    gd->offset_high= (offset >> 16) & 0xffff;
    return;
}

