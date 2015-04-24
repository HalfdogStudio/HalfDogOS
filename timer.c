#include "bootpack.h"

// FIXME: 时刻调整程序
// 由于timeout可能溢出unsigned int可表示的范围
// 隔一段时间重新调整下时间

void init_pit(void){
    int i;
    struct TIMER *t;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    //低八位
    io_out8(PIT_CNT0, 0x2e);    //高八位
    timerctl.count = 0;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;    //未使用
    }
    //一个哨兵
    t = timer_alloc();
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next_timer = 0;
    // 设置timerctl
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
    //timerctl.using = 1234;
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
    int eflags;
    struct TIMER *s, *t;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    // 禁止中断
    eflags = io_load_eflags();
    io_cli();
    //搜索注册timer到ctl.timers位置
    // 只有两种情况
    // 2. 插入到最前面
    // 3. 插入到中间
    // 注意
    // 是否要更新timerctl.t0
    // 是否更新timerctl.next
    // 更新timer->next_timer
    // 之前有计时器
    t = timerctl.t0;
    if (timer->timeout <= t->timeout) {    //计时器应该在当前计时器之前插入
        timerctl.t0 = timer;
        timer->next_timer = t;
        timerctl.next = timer->timeout;
        io_store_eflags(eflags);
        return;
    }
    for (;;) {
        // s为前一个
        s = t;
        t = t->next_timer;
        if (t == 0) {
            break;
        }
        //如果下一个为空
        if (timer->timeout <= t->timeout) { //插入到s和t之间
            s->next_timer = timer;
            timer->next_timer = t;
            io_store_eflags(eflags);
            return;
        }
    }
    io_store_eflags(eflags);
    return;
}

void inthandler20(int *esp){
    int i;
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60);   //把IRQ0信号接收完毕信息通知PIC
    timerctl.count++;
    // 还未到下一个时刻
    // 通过此策略大大减小不需要的循环和比较
    if (timerctl.next > timerctl.count) {
        return;
    }
    // 将最前面的地址赋给timer
    timer = timerctl.t0;
    for (;;) {
        if (timer->timeout > timerctl.count) {
            //如果排在下一个的没有超时
            break;
        }
        // 超时
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next_timer;
    }
    timerctl.t0 = timer;
    // 如果还有定时器，next指向下一个
    timerctl.next = timerctl.t0->timeout;
    //timerctl.using = 2345;
    return;
}

