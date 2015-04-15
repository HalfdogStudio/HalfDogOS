#include "bootpack.h"

void init_pit(void){
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    //低八位
    io_out8(PIT_CNT0, 0x2e);    //高八位
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timer[i].flags = 0;    //未使用
    }
    return;
}

struct TIMER *timer_alloc(void){
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == 0) {
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;   //没找到
}

void timer_free(struct TIMER *timer){
    timer->flags = 0;   //未使用
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, char data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
    // FIXME: 时刻调整程序
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    // 设定超时时更新next
    if (timerctl.next > timer->timeout){
        timerctl.next = timer->timeout;
    }
    return;
}

void inthandler20(int *esp){
    int i;
    io_out8(PIC0_OCW2, 0x60);   //把IRQ0信号接收完毕信息通知PIC
    timerctl.count++;
    // 还未到下一个时刻
    // 通过此策略大大较小不需要的循环和比较
    if (timerctl.next > timerctl.count) {
        return;
    }
    timerctl.next = 0xffffffff; //赋一个大值
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
            if (timerctl.timer[i].timeout < timerctl.count) {   //发生超时
                // timeout表示予定时间
                timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
            } else {    // 没有超时
                if (timerctl.next > timerctl.timer[i].timeout) {
                    //这个timer的timeout值在前面，则设为next
                    timerctl.next = timerctl.timer[i].timeout;
                }
            }
        }
    }
    return;
}

