#include "bootpack.h"

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
    struct SHEET *sht_back, *sht_mouse;
    unsigned char *buf_back, buf_mouse[16 * 16];    //16x16的鼠标大小

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
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);   //320*200字节
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   //没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99

    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    init_mouse_cursor8(buf_mouse, 99);
    sheet_slide(sht_back, 0, 0);    //背景

    mx = (binfo->scrnx - 16) / 2;   //显示到画面中央
    my = (binfo->scrny - 16 - 28) / 2;   //显示到画面中央

    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_mouse, 1);

    sprintf(s, "memory %dMB free: %dKB", memtotal / 1024 / 1024,
            memman_total(memman) / 1024);
    putfont8_asc(buf_back, binfo->scrnx, 3, 63, COL8_WHITE, s);

    sprintf(s, "(%3d, %3d)", mx, my);
    putfont8_asc(buf_back, binfo->scrnx, 3, 93, COL8_WHITE, s);
    sheet_refresh(sht_back, 3, 63, binfo->scrnx, 133);

    for(;;){
        io_cli();                   //禁止中断
        if (fifo8_status(&keyinfo) + fifo8_status(&mouseinfo) == 0){      // keybuf为空
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
                    boxfill8(buf_back, binfo->scrnx, COL8_DARK_CYAN, 3, 93, binfo->scrnx-1, 123);
                    putfont8_asc(buf_back, binfo->scrnx, 3, 93, COL8_WHITE, s);
                    // 刷新鼠标状态和位置
                    sheet_refresh(sht_back, 3, 13, 153, 123);
                    // 移动鼠标
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}

