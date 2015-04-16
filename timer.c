#include "bootpack.h"

// FIXME: 时刻调整程序
// 由于timeout可能溢出unsigned int可表示的范围
// 隔一段时间重新调整下时间

void init_pit(void){
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    //低八位
    io_out8(PIT_CNT0, 0x2e);    //高八位
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;    //未使用
    }
    return;
}

struct TIMER *timer_alloc(void){
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;   //没找到
}

void timer_free(struct TIMER *timer){
    timer->flags = 0;   //未使用
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
    int eflags, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    // 禁止中断
    eflags = io_load_eflags();
    io_cli();
    //搜索注册timer到ctl.timers位置
    for (i = 0; i < timerctl.using; i++) {
        if (timerctl.timers[i]->timeout >= timer->timeout) {
            break;
        }
    }
    //i之后全部后移
    for (j = timerctl.using; j > i; j--) {  //从i+1到using
        timerctl.timers[j] = timerctl.timers[j-1];
    }
    // IMPORTANT！
    timerctl.using++;
    // 把timer放到i上
    timerctl.timers[i] = timer;
    //更新next
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(eflags);
    return;
}

void inthandler20(int *esp){
    int i, j;
    io_out8(PIC0_OCW2, 0x60);   //把IRQ0信号接收完毕信息通知PIC
    timerctl.count++;
    // 还未到下一个时刻
    // 通过此策略大大较小不需要的循环和比较
    if (timerctl.next > timerctl.count) {
        return;
    }
    for (i = 0; i < timerctl.using; i++) {
        if (timerctl.timers[i]->timeout > timerctl.count) {
            //如果排在下一个的没有超时
            break;
        }
        // 超时
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    // 正好有i个定时器超时了
    timerctl.using -= i;
    for (j = 0; j < timerctl.using; j++) {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    // 如果还有定时器，next指向下一个
    if (timerctl.using > 0) {
        timerctl.next = timerctl.timers[0]->timeout;
    } else {
        timerctl.next = 0xffffffff;
    }
    return;
}

