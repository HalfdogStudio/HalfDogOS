#include "bootpack.h"

// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    unsigned char mcursor[16 * 16];//就此才发现[16][16]与[16*16]区别
    // 键盘鼠标中断变量
    int i; //存放键盘数据
    char s[40], keybuf[32], mousebuf[128];

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断
    io_out8(PIC0_IMR, 0xf9); // 开放 PIC1, 键盘终端, 计时器 11111001
    io_out8(PIC1_IMR, 0xef);    // 开放0x2c 8+3=11 11101111

    fifo8_init(&keyinfo, 32, keybuf);
    fifo8_init(&mouseinfo, 128, mousebuf);
    init_keyboard();        // 鼠标和键盘控制电路是一个
    enable_mouse();

    init_palette(); // 调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    //putfont8_asc(binfo->vram, binfo->scrnx, 8, 60, COL8_BLACK, "HALFDOG STUDIO");
    //putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_DARK_GRAY, "HalfDog OS");
    //putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_WHITE, "HalfDog OS");

    init_mouse_cursor8(mcursor, COL8_DARK_CYAN);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 140, 80, mcursor, 16);

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
                boxfill8(binfo->vram, binfo->scrnx, COL8_DARK_CYAN, 0, 17, 32 * 8 - 1, 32);
                putfont8_hex(binfo->vram, binfo->scrnx, 3, 19, COL8_WHITE, (unsigned char *)&i);
            }
        }
    }
}

void wait_KBC_sendready(void){
    // 等待键盘控制电路准备完毕
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
    return;
}

void init_keyboard(void){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();       // 等到可以接收
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

void enable_mouse(void){
    // 激活鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return; //如果顺利键盘控制返回ACK(0xfa)
}

