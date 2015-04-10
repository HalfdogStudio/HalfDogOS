#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT;  //AC bit = 1
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT; //AC bit = 0
    io_store_eflags(eflg);
    // 如果是486,禁止缓存
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    // 开始测试
    i = memtest_sub(start, end);
    // 恢复缓存
    if (flg486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    return i;
}

unsigned int memtest_sub(unsigned int start, unsigned int end){
    //将内存中的值翻转,检查结果是否正确,再翻转,在检查是否恢复初值
    //有的机型因为主板或芯片组的原因需要如此.

    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *)(i + 0xffc);    //最后四个字节
        old = *p;   //保留旧值
        *p = pat0;  //新值
        *p ^= 0xffffffff;    // 在内存中与
        if (*p != pat1) {
not_memory:
            *p = old;   // 恢复旧值
            break;
        }
        *p ^= 0xffffffff;
        if (*p != pat0) {
            goto not_memory;
        }
        *p = old;
    }
    return i;
}

void memman_init(struct MEMMAN *man){
    man->frees = 0;     //可用内存记录数
    man->maxfrees = 0;  //frees的最大值
    man->lostsize = 0;  //释放失败内存的总大小
    man->losts = 0;     //释放失败的次数
    return;
}
unsigned int memman_total(struct MEMMAN *man){
    // 报告空余内存大小合计
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }
    return t;
}
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
    unsigned int i, a;
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0){
                //减去一条free可用信息
                man->frees--;
                //  下面这种作用域真的可以吗?
                for (; i < man->frees; i++) {
                    man->free[i] = man->free[i + 1];
                }
            }
            return a;   //返回地址
        }
    }
    return 0;           //没有可用空间
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    int i, j;
    // 为了便于归纳内存,free[]按地址先后顺序排列
    // 先决定放到哪里
    for (i = 0; i < man->frees; i++) {
        if(man->free[i].addr > addr){
            break;  //显然addr在[free[i-1], free[i])之间
        }
    }
    if (i > 0) {    // 前面有可用内存, 如果free[0].addr>addr则前面没有
        if (man->free[i-1].addr + man->free[i-1].size == addr) {
            //能与前面的内存归在一起
            //1. size
            man->free[i-1].size += size;
            if (i < man->frees) {   // 如果i后面还有有效内存
                if(addr + size == man->free[i].addr){
                    //和后面的可合并
                    man->free[i-1].size += man->free[i].size;
                    // 删除free[i]
                    man->frees--;   //总数减一
                    for (; i < man->frees; i++){
                        man->free[i] = man->free[i+1];
                    }
                }
            }   // 后面没有能合在一起的则不管
            return 0;   //合并完成
        }   //如果不能与前面合并则不管
    }   //如果前面没有可用内存
    if (i < man->frees){    // 但后面有可用内存
        if (addr + size == man->free[i].addr) {
            man->free[i].addr = addr;
            man->free[i].size +=size;
            return 0;   //合并成功
        }   // 如果后面不是接着的则不管
    }   //如果后面也没有能合并的则不管
    // 这时是前面也没有且后面也没有可合并的
    // 必须新添加一个
    if (man->frees < MEMMAN_FREES) {
        // 如果还有可用来放free[]的空间
        // free[i]向后移动
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j-1];
        }
        man->frees++;
        if(man->maxfrees < man->frees){
            man->maxfrees = man->frees;
        }   // 更新最大值
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;   //完成
    }   //如果内存列表还有空间,还能分配内存
    //如果连新分配都分不了
    man->losts++;
    man->lostsize += size;
    return -1;      // 失败
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
    unsigned int a;
    // 神奇的向上取整 0x1000对齐 4kb
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
