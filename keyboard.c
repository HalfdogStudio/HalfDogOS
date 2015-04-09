#include "bootpack.h"

void wait_KBC_sendready(void){
    // 等待键盘控制电路准备完毕
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

void init_keyboard(void){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();       // 等到可以接收
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

void inthandler21(int *esp){
    // 来自键盘的中断0x21
    unsigned char data;     // 键盘中断数据
    //struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

    io_out8(PIC0_OCW2, 0x61);   //通知IRQ-1已经受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyinfo, data);
    // 不把屏幕渲染放入中断处理中
    //boxfill8(binfo->vram, binfo->scrnx, COL8_BLACK, 0, 0, 32 * 8 - 1, 15);
    //putfont8_asc(binfo->vram, binfo->scrnx, 123, 3, COL8_WHITE, "INT 21 (IQR-1) : PS/2 Keyboard");
    //
    //putfont8_hex(binfo->vram, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&data);
    return;
}

