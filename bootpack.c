#include "bootpack.h"

// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    unsigned char mcursor[16 * 16];//就此才发现[16][16]与[16*16]区别
    // 键盘鼠标中断变量
    int i; //存放键盘数据
    char s[40], keybuf[32], mousebuf[128];
    struct MOUSE_DEC mdec;
    int mx, my;     //鼠标
    // 内存
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断
    io_out8(PIC0_IMR, 0xf9); // 开放 PIC1, 键盘终端, 计时器 11111001
    io_out8(PIC1_IMR, 0xef);    // 开放0x2c 8+3=11 11101111

    fifo8_init(&keyinfo, 32, keybuf);
    fifo8_init(&mouseinfo, 128, mousebuf);
    init_keyboard();        // 鼠标和键盘控制电路是一个
    enable_mouse(&mdec);

    unsigned int memtotal;
    memtotal = memtest(0x00400000, 0xbfffffff) / 1024 / 1024;
    // 初始化内存管理
    memman_init(memman);
    // 分配可用内存
    memman_free(memman, 0x00001000, 0x0009e000); //0x00001000-0x0009efff
    memman_free(memman, 0x00400000, memtotal * 1024 * 1024 - 0x00400000); //
    // 初始化图形界面
    init_palette(); // 调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    //putfont8_asc(binfo->vram, binfo->scrnx, 8, 60, COL8_BLACK, "HALFDOG STUDIO");
    //putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_DARK_GRAY, "HalfDog OS");
    //putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_WHITE, "HalfDog OS");
    //------------------------------------
    //内存检查结果
    putfont8_asc(binfo->vram, binfo->scrnx, 3, 60, COL8_WHITE, "Memory: ");
    putfont8_hex(binfo->vram, binfo->scrnx, 63, 60, COL8_WHITE, (unsigned char *)&memtotal+3);
    putfont8_hex(binfo->vram, binfo->scrnx, 93, 60, COL8_WHITE, (unsigned char *)&memtotal+2);
    putfont8_hex(binfo->vram, binfo->scrnx, 123, 60, COL8_WHITE, (unsigned char *)&memtotal+1);
    putfont8_hex(binfo->vram, binfo->scrnx, 153, 60, COL8_WHITE, (unsigned char *)&memtotal);
    putfont8_asc(binfo->vram, binfo->scrnx, 183, 60, COL8_WHITE, "Mb");

    putfont8_asc(binfo->vram, binfo->scrnx, 3, 90, COL8_WHITE, "Free: ");
    unsigned int freemem = memman_total(memman) / 1024;
    putfont8_hex(binfo->vram, binfo->scrnx, 63, 90, COL8_WHITE, (unsigned char *)&freemem+3);
    putfont8_hex(binfo->vram, binfo->scrnx, 93, 90, COL8_WHITE, (unsigned char *)&freemem+2);
    putfont8_hex(binfo->vram, binfo->scrnx, 123, 90, COL8_WHITE, (unsigned char *)&freemem+1);
    putfont8_hex(binfo->vram, binfo->scrnx, 153, 90, COL8_WHITE, (unsigned char *)&freemem);
    putfont8_asc(binfo->vram, binfo->scrnx, 183, 90, COL8_WHITE, "Kb");

    init_mouse_cursor8(mcursor, COL8_DARK_CYAN);
    //putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 140, 80, mcursor, 16);

    for(;;){
        io_cli();                   //禁止中断
        if (fifo8_status(&keyinfo) + fifo8_status(&mouseinfo) == 0){      // keybuf为空
            io_stihlt();            //恢复允许中断并等待
        } else {
            if (fifo8_status(&keyinfo) != 0){
                i = fifo8_get(&keyinfo);
                io_sti();               //恢复中断,不因为图像处理阻塞
                boxfill8(binfo->vram, binfo->scrnx, COL8_DARK_CYAN, 0, 0, 32 * 8 - 1, 16);
                putfont8_hex(binfo->vram, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&i);
            } else if (fifo8_status(&mouseinfo) != 0){
                i = fifo8_get(&mouseinfo);
                io_sti();               //恢复中断,不因为图像处理阻塞
                if (mouse_decode(&mdec, i) != 0) {
                    boxfill8(binfo->vram, binfo->scrnx, COL8_DARK_CYAN, 0, 17, binfo->scrnx-1, 32);
                    char mouse_status[] = "lcr";
                    if (mdec.btn&0x1 != 0){
                        mouse_status[0] = 'L';
                    }
                    if (mdec.btn>>1&0x1 != 0){
                        mouse_status[2] = 'R';
                    }
                    if (mdec.btn>>2&0x1 != 0){
                        mouse_status[1] = 'C';
                    }
                    putfont8_asc(binfo->vram, binfo->scrnx, 3, 19, COL8_WHITE, mouse_status);
                    // 隐藏鼠标
                    boxfill8(binfo->vram, binfo->scrnx, COL8_DARK_CYAN, mx, my, mx+15, my+15);
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
                }
            }
        }
    }
}

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
