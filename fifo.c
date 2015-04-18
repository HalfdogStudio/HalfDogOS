#include "bootpack.h"

void fifo32_init(struct FIFO32 *fifo, int size, int *buf){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

#define FLAGS_OVERRUN 0x0001

int fifo32_put(struct FIFO32 *fifo, int data){
    //写入一个数据时
    //1. 判断free是否等于0
    //2. 在p处写入并增加p
    //3. 如果p=size则p=0
    //4. free--
    if (fifo->free == 0) {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

int fifo32_get(struct FIFO32 *fifo){
    //读出一个数据
    //1. 判断free是否和size一样(空fifo)
    //2. 从q处读出并且q++
    //3. 如果q==size则q=0
    //4. free++
    int data;
    if (fifo->free == fifo->size) {
        return -1;  //缓冲区为空
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32 *fifo){
    // 报告还有多少数据
    return fifo->size - fifo->free;
}

