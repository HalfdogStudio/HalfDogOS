#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    //字符缓冲区
    unsigned char s[40];
    // 键盘鼠标中断变量
    int i; //存放键盘数据
    char keybuf[32], mousebuf[128];
    struct MOUSE_DEC mdec;
    int mx, my;     //鼠标
    // 内存
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    // 图层管理
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, *buf_win, buf_mouse[16 * 16];    //16x16的鼠标大小
    char timerbuf[8], timerbuf2[8], timerbuf3[8];   //计时器buffer
    struct TIMER *timer, *timer2, *timer3;
    struct FIFO8 timerfifo, timerfifo2, timerfifo3;

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断

    init_pit();

    io_out8(PIC0_IMR, 0xf8); // 开放 PIC1, 键盘终端, 计时器 11111000
    io_out8(PIC1_IMR, 0xef);    // 开放0x2c 8+3=11 11101111

    fifo8_init(&keyinfo, 32, keybuf);
    fifo8_init(&mouseinfo, 128, mousebuf);

    fifo8_init(&timerfifo, 8, timerbuf);
    timer = timer_alloc();
    timer_init(timer, &timerfifo, 1);
    timer_settime(timer, 1000);

    fifo8_init(&timerfifo2, 8, timerbuf2);
    timer2 = timer_alloc();
    timer_init(timer2, &timerfifo2, 1);
    timer_settime(timer2, 300);

    fifo8_init(&timerfifo3, 8, timerbuf3);
    timer3 = timer_alloc();
    timer_init(timer3, &timerfifo3, 1);
    timer_settime(timer3, 50);

    init_keyboard();        // 鼠标和键盘控制电路是一个
    enable_mouse(&mdec);

    unsigned int memtotal;
    memtotal = memtest(0x00400000, 0xbfffffff);
    // 初始化内存管理
    memman_init(memman);
    // 分配可用内存
    memman_free(memman, 0x00400000, memtotal - 0x00400000); //
    // 初始化图形界面
    init_palette(); // 调色板
    shtctl = shtctl_init(memman, binfo->vram,
            binfo->scrnx, binfo->scrny);    //初始化图层管理结构
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);

    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);   //320*200字节
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);

    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   //没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 52, -1);    //没有透明色

    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    init_mouse_cursor8(buf_mouse, 99);

    make_window8(buf_win, 160, 52, "counter");
    //putfont8_asc(buf_win, 160, 24, 28, COL8_BLACK, "Welcome to");
    //putfont8_asc(buf_win, 160, 24, 44, COL8_BLACK, "HalfDog OS!");

    sheet_slide(sht_back, 0, 0);    //背景

    mx = (binfo->scrnx - 16) / 2;   //显示到画面中央
    my = (binfo->scrny - 16 - 28) / 2;   //显示到画面中央

    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);

    sprintf(s, "memory %dMB free: %dKB", memtotal / 1024 / 1024,
            memman_total(memman) / 1024);
    putfont8_asc_sht(sht_back, 3, 33, COL8_WHITE, COL8_DARK_CYAN, s, 28);

    sprintf(s, "(%3d, %3d)", mx, my);
    putfont8_asc_sht(sht_back, 3, 63, COL8_WHITE, COL8_DARK_CYAN, s, 14);

    for(;;){
        // 计数器
        sprintf(s, "%010d", timerctl.count);
        putfont8_asc_sht(sht_win, 40, 28, COL8_BLACK, COL8_GRAY, s, 10);
        // 计数器结束
        io_cli();                   //禁止中断
        if (fifo8_status(&keyinfo) + fifo8_status(&mouseinfo) +
                fifo8_status(&timerfifo) +
                fifo8_status(&timerfifo2) +
                fifo8_status(&timerfifo3) == 0){      // keybuf为空
            io_stihlt();            //恢复允许中断并等待
        } else {
            if (fifo8_status(&keyinfo) != 0){
                i = fifo8_get(&keyinfo);
                io_sti();               //恢复中断,不因为图像处理阻塞
                boxfill8(buf_back, binfo->scrnx, COL8_DARK_CYAN, 0, 0, 32 * 8 - 1, 16);
                putfont8_hex(buf_back, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&i);
                sheet_refresh(sht_back, 0, 0, 48, 30);
            } else if (fifo8_status(&mouseinfo) != 0){
                i = fifo8_get(&mouseinfo);
                io_sti();               //恢复中断,不因为图像处理阻塞
                if (mouse_decode(&mdec, i) != 0) {
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
                    boxfill8(buf_back, binfo->scrnx, COL8_DARK_CYAN, 0, 17, binfo->scrnx-1, 32);
                    putfont8_asc(buf_back, binfo->scrnx, 3, 19, COL8_WHITE, mouse_status);
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < -15) {
                        mx = -15;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my < -15) {
                        my = -15;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    //鼠标位置
                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfont8_asc_sht(sht_back, 3, 63, COL8_WHITE, COL8_DARK_CYAN, s, 15);
                    // 移动鼠标
                    sheet_slide(sht_mouse, mx, my);
                }
            } else if (fifo8_status(&timerfifo) != 0) {
                i = fifo8_get(&timerfifo);
                io_sti();
                putfont8_asc_sht(sht_back, 3, 95, COL8_WHITE, COL8_DARK_CYAN, "10[sec]", 7);
            } else if (fifo8_status(&timerfifo2) != 0) {
                i = fifo8_get(&timerfifo2);
                io_sti();
                putfont8_asc_sht(sht_back, 3, 125, COL8_WHITE, COL8_DARK_CYAN, "3[sec]", 6);
            } else if (fifo8_status(&timerfifo3) != 0) {
                i = fifo8_get(&timerfifo3);
                io_sti();
                if (i != 0) {
                    timer_init(timer3, &timerfifo3, 0);
                    boxfill8(buf_back, sht_back->bxsize, COL8_WHITE, 8, 140, 15, 164);
                } else {
                    timer_init(timer3, &timerfifo3, 1);
                    boxfill8(buf_back, sht_back->bxsize, COL8_DARK_CYAN, 8, 140, 15, 164);
                }
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 140, 15, 164);
            }
        }
    }
}


void make_window8(unsigned char *buf, int xsize, int ysize, char *title){
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c; //色彩
    boxfill8(buf, xsize, COL8_GRAY, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_WHITE, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_GRAY, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_WHITE, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_DARK_GRAY, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_BLACK, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_GRAY, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_DARK_BLUE, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, COL8_DARK_GRAY, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_BLACK, 0, ysize - 1, xsize - 1, ysize - 1);
    putfont8_asc(buf, xsize, 24, 4, COL8_WHITE, title);
    // 按钮
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
                c = COL8_BLACK;
            } else if (c == '$') {
                c = COL8_DARK_GRAY;
            } else if (c == 'Q') {
                c = COL8_GRAY;
            } else {
                c = COL8_WHITE;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}
