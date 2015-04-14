#include "bootpack.h"

void init_pit(void){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    //低八位
    io_out8(PIT_CNT0, 0x2e);    //高八位
    return;
}

void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);   //把IRQ0信号接收完毕信息通知PIC
    return;
}
