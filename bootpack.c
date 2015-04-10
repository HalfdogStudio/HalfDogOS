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

