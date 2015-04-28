#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
//void make_textbox8(struct SHEET* sht, int x0, int y0, int sx, int sy, int c);
// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    //字符缓冲区
    unsigned char s[40];
    // 键盘鼠标中断变量
    int i;
    int fifobuf[128];
    struct MOUSE_DEC mdec;
    int mx, my;     //鼠标
    // 内存
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    // 图层管理
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, *buf_win, buf_mouse[16 * 16];    //16x16的鼠标大小
    struct TIMER *timer, *timer2, *timer3;
    struct FIFO32 fifo;

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断

    init_pit();

    io_out8(PIC0_IMR, 0xf8); // 开放 PIC1, 键盘终端, 计时器 11111000
    io_out8(PIC1_IMR, 0xef);    // 开放0x2c 8+3=11 11101111

    fifo32_init(&fifo, 128, fifobuf);

    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000);

    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);

    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

    init_keyboard(&fifo, 256);        // 鼠标和键盘控制电路是一个
    enable_mouse(&fifo, 512, &mdec);

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

    make_window8(buf_win, 160, 52, "window");
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

    //sprintf(s, "memory %dMB free: %dKB", memtotal / 1024 / 1024,
    //        memman_total(memman) / 1024);
    //putfont8_asc_sht(sht_back, 3, 33, COL8_WHITE, COL8_DARK_CYAN, s, 28);

    //sprintf(s, "(%3d, %3d)", mx, my);
    //putfont8_asc_sht(sht_back, 3, 63, COL8_WHITE, COL8_DARK_CYAN, s, 14);

    // 输入处理
    int cursor_x, cursor_c;
    /*make_textbox8(sht_win, 8, 28, 144, 16, COL8_WHITE);*/
    cursor_x = 8;
    cursor_c = COL8_WHITE;

    for (;;) {
        // 计数器
        // 计数器结束
        io_cli();                   //禁止中断
        if (fifo32_status(&fifo) == 0){
            io_stihlt();            //恢复允许中断并等待
        } else {
            i = fifo32_get(&fifo);
            io_sti();               //恢复中断,不因为图像处理阻塞
            if (256 <= i && i <= 511) {
                // 键盘中断
                //sprintf(s, "%02x", i - 256);
                //putfont8_asc_sht(sht_back, 3, 3, COL8_WHITE, COL8_DARK_CYAN, s, 5);
                if (i - 256 < 0x54) {
                    if(keytable[i - 256] != 0 && cursor_x < 144){
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfont8_asc_sht(sht_win, cursor_x, 28, COL8_BLACK, COL8_WHITE, s, 2);
                        cursor_x += 8;
                    }
                }
                if (i - 256 == 0x0e && cursor_x > 8) {      //backspace
                    putfont8_asc_sht(sht_win, cursor_x-8, 28, COL8_BLACK, COL8_WHITE, "  ", 2);
                    cursor_x -= 8;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 2, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 12, 44);
            } else if (512 <= i && i <= 767){
                // 鼠标数据
                //sprintf(s, "[lcr]", mdec.x, mdec.y);
                if (mouse_decode(&mdec, i - 512) != 0) {
                    if (mdec.btn&0x1 != 0){
                        //s[1] = 'L';
                        sheet_slide(sht_win, mx, my);
                    }
                    if (mdec.btn>>1&0x1 != 0){
                        //s[3] = 'R';
                    }
                    if (mdec.btn>>2&0x1 != 0){
                        //s[2] = 'C';
                    }
                    //putfont8_asc_sht(sht_back, 3, 20, COL8_WHITE, COL8_DARK_CYAN, s, 15);
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
                    //sprintf(s, "(%3d, %3d)", mx, my);
                    //putfont8_asc_sht(sht_back, 3, 63, COL8_WHITE, COL8_DARK_CYAN, s, 15);
                    // 移动鼠标
                    sheet_slide(sht_mouse, mx, my);
                }
            } else if (i == 10) {
                putfont8_asc_sht(sht_back, 3, 95, COL8_WHITE, COL8_DARK_CYAN, "10[sec]", 8);
            } else if (i == 3) {
                putfont8_asc_sht(sht_back, 3, 125, COL8_WHITE, COL8_DARK_CYAN, "3[sec]", 6);
            } else if (i <= 1) {
                if (i != 0) {
                    timer_init(timer3, &fifo, 0);
                    cursor_c = COL8_BLACK;
                } else {
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_WHITE;
                }
                boxfill8(buf_win, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 2, 44);
                timer_settime(timer3, 50);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}


void make_window8(unsigned char *buf, int xsize, int ysize, char *title){
    static const char closebtn[14][16] = {
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

void make_textbox8(struct SHEET* sht, int x0, int y0, int sx, int sy, int c){
    /*int x1 = x0 + sx;*/
    /*int y1 = y0 + sy;*/
    /*boxfill8(sht->buf, sht->bxsize, COL8_GRAY, x0-2, y0-3, x1 + 1, y1-3)*/
    return;
}
