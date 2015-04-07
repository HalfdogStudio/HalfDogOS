#include "bootpack.h"

// entry_point
void HalfDogMain(void){

    struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
    extern char hankaku[4096];
    unsigned char mcursor[16 * 16];//就此才发现[16][16]与[16*16]区别
    int i;      // 存储键盘中断数据

    init_gdtidt();
    init_pic();
    io_sti(); //IDT／PIC 的初始化完成，于是开放CPU中断
    io_out8(PIC0_IMR, 0xf9); // 开放 PIC1, 键盘终端, 计时器 11111000
    init_palette(); // 调色板

    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    //putfont8_asc(binfo->vram, binfo->scrnx, 8, 60, COL8_BLACK, "HALFDOG STUDIO");
    //putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_DARK_GRAY, "HalfDog OS");
    //putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_WHITE, "HalfDog OS");

    init_mouse_cursor8(mcursor, COL8_DARK_CYAN);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 140, 80, mcursor, 16);
    for(;;){
        io_cli();                   //禁止中断
        //putfont8_hex(binfo->vram, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&keybuf.flag);
        if (keybuf.flag == 0){      // keybuf为空
            io_stihlt();            //恢复允许中断并等待
        } else {
            i = keybuf.data;
            keybuf.flag = 0;
            io_sti();               //恢复中断,不因为图像处理阻塞
            boxfill8(binfo->vram, binfo->scrnx, COL8_DARK_GRAY, 0, 0, 32 * 8 - 1, 15);
            putfont8_hex(binfo->vram, binfo->scrnx, 3, 3, COL8_WHITE, (unsigned char *)&i);
        }
    }
}
