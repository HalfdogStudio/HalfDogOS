#include "bootpack.h"

// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    //unsigned char mcursor[16 * 16];//就此才发现[16][16]与[16*16]区别
    // 键盘鼠标中断变量
    int i; //存放键盘数据
    char s[40], keybuf[32], mousebuf[128];
    struct MOUSE_DEC mdec;
    int mx, my;     //鼠标
    // 内存
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    // 图层管理
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse;
    unsigned char *buf_back, buf_mouse[256];    //16x16的鼠标大小

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
    //memman_free(memman, 0x00001000, 0x0009e000); //0x00001000-0x0009efff
    memman_free(memman, 0x00400000, memtotal * 1024 * 1024 - 0x00400000); //
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
    sheet_slide(shtctl, sht_back, 0, 0);    //背景

    mx = (binfo->scrnx - 16) / 2;   //显示到画面中央
    my = (binfo->scrny - 16 - 28) / 2;   //显示到画面中央

    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);

    putfont8_asc(buf_back, binfo->scrnx, 60, 60, COL8_WHITE, "HalfDog");
    sheet_refresh(shtctl);

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
                sheet_refresh(shtctl);
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
                    sheet_slide(shtctl, sht_mouse, mx, my);
                }
            }
        }
    }
}

