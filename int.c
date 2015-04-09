#include "bootpack.h"

void init_pic(void){
    io_out8(PIC0_IMR, 0xff);
    io_out8(PIC1_IMR, 0xff);

    io_out8(PIC0_ICW1, 0x11);
    io_out8(PIC0_ICW2, 0x20); //中断从int 0x20-0x27
    io_out8(PIC0_ICW3, 1 << 2); // IRQ 2接从PIC
    io_out8(PIC0_ICW4, 0x01);

    io_out8(PIC1_ICW1, 0x11);
    io_out8(PIC1_ICW2, 0x28); //中断从int 0x28-0x2f
    io_out8(PIC1_ICW3, 2); // IRQ 2接主PIC
    io_out8(PIC1_ICW4, 0x01);

    io_out8(PIC0_IMR, 0xfb);    //11111011 PIC以外全禁止
    io_out8(PIC1_IMR, 0xff);    //11111011 禁止所有中断
    return;
}

