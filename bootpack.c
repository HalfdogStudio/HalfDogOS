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

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断
    io_out8(PIC0_IMR, 0xf9); // 开放 PIC1, 键盘终端, 计时器 11111001
    io_out8(PIC1_IMR, 0xef);    // 开放0x2c 8+3=11 11101111

    fifo8_init(&keyinfo, 32, keybuf);
    fifo8_init(&mouseinfo, 128, mousebuf);
    init_keyboard();        // 鼠标和键盘控制电路是一个
    enable_mouse(&mdec);

    init_palette(); // 调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    //putfont8_asc(binfo->vram, binfo->scrnx, 8, 60, COL8_BLACK, "HALFDOG STUDIO");
    //putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_DARK_GRAY, "HalfDog OS");
    //putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_WHITE, "HalfDog OS");

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

void enable_mouse(struct MOUSE_DEC *mdec){
    // 激活鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;    //等待0xfa状态
    return; //如果顺利键盘控制返回ACK(0xfa)
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
    if (mdec->phase == 0){
        if(dat == 0xfa){
            mdec->phase = 1;
        }
        return 0;
    } else if (mdec->phase == 1){
        //等待鼠标的第一字节
        if ((dat & 0xc8) == 0x08) {
            // 第一字节高四位在0-3范围内,低四位在8-F范围
            // 防止鼠标接触不良
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    } else if (mdec->phase == 2){
        //等待鼠标的第二字节
        mdec->buf[1] = dat;
        mdec->phase = 3;
        return 0;
    } else if(mdec->phase == 3){
        mdec->buf[2] = dat;
        mdec->phase = 1;
        // 鼠标的3个字节都凑齐了
        mdec->btn = mdec->buf[0] & 0x07; //第一个字符的低三位,鼠标状态(中键|右键|左键)
        mdec->x = mdec->buf[1];  //第二个字节
        mdec->y = mdec->buf[2]; //第三个字节
        if ((mdec->buf[0] & 0x10) != 0) {   //第一个字节中(第5位)对鼠标x移动有用的位
            mdec->x |= 0xffffff00;          //高24位全置1
        }
        if ((mdec->buf[0] & 0x20) != 0) {   //第一个字节中(第6位)对鼠标x移动有用的位
            mdec->y |= 0xffffff00;          //高24位全置1
        }
        mdec->y = -mdec->y;                 //鼠标y坐标和画面相反
        return 1;
    }
    return -1;
}
