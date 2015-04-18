#include "bootpack.h"
int mousedata0;

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec){
    mousefifo = fifo;
    mousedata0 = data0;
    // 激活鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;    //等待0xfa状态
    return; //如果顺利键盘控制返回ACK(0xfa)
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
    if (mdec->phase == 0){
        if(dat == 0xfa){
            mdec->phase = 1;
        }
        return 0;
    } else if (mdec->phase == 1){
        //等待鼠标的第一字节
        if ((dat & 0xc8) == 0x08) {
            // 第一字节高四位在0-3范围内,低四位在8-F范围
            // 防止鼠标接触不良
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    } else if (mdec->phase == 2){
        //等待鼠标的第二字节
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    } else if(mdec->phase == 3){
        mdec->buf[2] = dat;
        mdec->phase = 1;
        // 鼠标的3个字节都凑齐了
        mdec->btn = mdec->buf[0] & 0x07; //第一个字符的低三位,鼠标状态(中键|右键|左键)
        mdec->x = mdec->buf[1];  //第二个字节
        mdec->y = mdec->buf[2]; //第三个字节
        if ((mdec->buf[0] & 0x10) != 0) {   //第一个字节中(第5位)对鼠标x移动有用的位
            mdec->x |= 0xffffff00;          //高24位全置1
        }
        if ((mdec->buf[0] & 0x20) != 0) {   //第一个字节中(第6位)对鼠标x移动有用的位
            mdec->y |= 0xffffff00;          //高24位全置1
        }
        mdec->y = -mdec->y;                 //鼠标y坐标和画面相反
        return 1;
    }
    return -1;
}

void inthandler2c(int *esp){
    int data;
    //struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    //boxfill8(binfo->vram, binfo->scrnx, COL8_BLACK, 0, 0, 32 * 8 - 1, 16);
    //putfont8_asc(binfo->vram, binfo->scrnx, 123, 3, COL8_WHITE, "INT 2C (IQR-12) : PS/2 MOUSE");
    io_out8(PIC1_OCW2, 0x64);       // 通知PIC1 IRQ-12已经受理
    io_out8(PIC0_OCW2, 0x62);       // 通知PIC0 IRQ-2已经受理
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    return;
}


