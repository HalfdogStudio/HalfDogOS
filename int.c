#include "bootpack.h"
#define PORT_KEYDAT 0x0060

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

void inthandler21(int *esp){
    // 来自键盘的中断0x21
    unsigned char data;     // 键盘中断数据
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

    io_out8(PIC0_OCW2, 0x61);   //通知IRQ-1已经受理完毕
    data = io_in8(PORT_KEYDAT);
    if (keybuf.flag < 32) {     //如果缓冲区为空
        keybuf.data = data;
        keybuf.flag = 1;
    }
    // 不把屏幕渲染放入中断处理中
    //boxfill8(binfo->vram, binfo->scrnx, COL8_BLACK, 0, 0, 32 * 8 - 1, 15);
    //putfont8_asc(binfo->vram, binfo->scrnx, 123, 3, COL8_WHITE, "INT 21 (IQR-1) : PS/2 Keyboard");
    //
    //putfont8_hex(binfo->vram, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&data);
    return;
}
