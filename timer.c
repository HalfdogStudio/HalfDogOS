#include "bootpack.h"

void init_pit(void){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    //低八位
    io_out8(PIT_CNT0, 0x2e);    //高八位
    timerctl.count = 0;
    timerctl.timeout = 0;
    return;
}

void inthandler20(int *esp){
    io_out8(PIC0_OCW2, 0x60);   //把IRQ0信号接收完毕信息通知PIC
    timerctl.count++;
    if (timerctl.timeout > 0) {
        timerctl.timeout--;
        if (timerctl.timeout == 0) {
            fifo8_put(timerctl.fifo, timerctl.data);
        }
    }
    return;
}

void settimer(unsigned int timeout, struct FIFO8 *fifo, char data){
    //防止设定定时器时被中断干扰
    int eflags;
    eflags = io_load_eflags();
    io_cli();
    timerctl.timeout = timeout;
    timerctl.fifo = fifo;
    timerctl.data = data;
    io_store_eflags(eflags);
    return;
}
