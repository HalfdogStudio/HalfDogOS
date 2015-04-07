#include "bootpack.h"
#include "hankaku.h"

void init_palette(void){
    static unsigned char table_rgb[] =
    {
        0x00, 0x00, 0x00,   // 0: 黑
        0xff, 0x00, 0x00,   // 1: 亮红
        0x00, 0xff, 0x00,   // 2: 亮绿
        0xff, 0xff, 0x00,   // 3: 亮黄
        0x00, 0x00, 0xff,   // 4: 亮蓝
        0xff, 0x00, 0xff,   // 5: 亮紫
        0x00, 0xff, 0xff,   // 6: 浅亮蓝
        0xff, 0xff, 0xff,   // 7: 白
        0xc6, 0xc6, 0xc6,   // 8: 亮灰
        0x84, 0x00, 0x00,   // 9: 暗红
        0x00, 0x84, 0x00,   // 10: 暗绿
        0x84, 0x84, 0x00,   // 11: 暗黄
        0x00, 0x00, 0x84,   // 12: 暗青
        0x84, 0x00, 0x84,   // 13: 暗紫
        0x00, 0x84, 0x84,   // 14: 浅暗蓝
        0x84, 0x84, 0x84    // 15: 暗灰
    };
    set_palette(0, 15, table_rgb);
    return;
}

void set_palette(int start, int end, unsigned char *rgb){
    int i, eflags;
    eflags = io_load_eflags();  //记录中断许可标志
    io_cli();                   //将中断许可标志设为0,禁止中断
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c,
        int x0, int y0, int x1, int y1){
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
    return;
}

void init_screen8(unsigned char *vram, int xsize, int ysize){
    boxfill8(vram, xsize, COL8_DARK_CYAN, 0, 0, xsize - 1, ysize - 29);
    boxfill8(vram, xsize, COL8_GRAY, 0, ysize - 28, xsize - 1, ysize - 28);
    boxfill8(vram, xsize, COL8_DARK_GRAY, 0, ysize - 27, xsize - 1, ysize - 1);
    // button shade up
    boxfill8(vram, xsize, COL8_GRAY, 3, ysize - 24, 59, ysize - 4);
    // button shade down
    boxfill8(vram, xsize, COL8_BLACK, 5, ysize - 22, 60, ysize - 3);
    // button
    boxfill8(vram, xsize, COL8_DARK_GRAY, 4, ysize - 23, 59, ysize - 4);

    // button shade up
    boxfill8(vram, xsize, COL8_BLACK, xsize - 47, ysize - 24, xsize - 4, ysize - 4);
    // button shade down
    boxfill8(vram, xsize, COL8_WHITE, xsize - 46, ysize - 22, xsize - 3, ysize - 3);
    // button
    boxfill8(vram, xsize, COL8_DARK_GRAY, xsize - 46, ysize - 23, xsize - 4, ysize - 4);
}

void putfont8(unsigned char *vram, int xsize, int x, int y, unsigned char c, char *font){
    int i;
    char *p, d;
    for (i = 0; i < 16; i++) {
        // p = 第n行指针
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0){
            p[0] = c;
        }
        if ((d & 0x40) != 0){
            p[1] = c;
        }
        if ((d & 0x20) != 0){
            p[2] = c;
        }
        if ((d & 0x10) != 0){
            p[3] = c;
        }
        if ((d & 0x08) != 0){
            p[4] = c;
        }
        if ((d & 0x04) != 0){
            p[5] = c;
        }
        if ((d & 0x02) != 0){
            p[6] = c;
        }
        if ((d & 0x01) != 0){
            p[7] = c;
        }
    }
    return;
}

void putfont8_asc(unsigned char *vram, int xsize, int x, int y, char c, unsigned char *s){
    extern char hankaku[4096];
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    return;
}

void putfont8_hex(unsigned char *vram, int xsize, int x, int y, char c, unsigned char *s){
    char hex[]="0123456789ABCDEF";
    char output[]="0x00";
    int i;
    for (i = 2; i < 4; i++) {
        output[i] = hex[*s / (16 >> (4 * (i - 2))) & 0xf];
    }
    putfont8_asc(vram, xsize, x, y, c, output);
    return;
}

void init_mouse_cursor8(unsigned char *mouse, char bc){
    // 我猜要单独设置这个函数就是因为背景会变
    // 先辈筚路蓝缕,以启山林
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };
    int x, y;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            switch (cursor[y][x]) {
                case '*':
                    mouse[y * 16 + x] = COL8_BLACK; // 话说 int和char,有点晕
                    break;
                case 'O':
                    mouse[y * 16 + x] = COL8_WHITE;
                    break;
                case '.':
                    mouse[y * 16 + x] = bc;
                    break;
                default:
                    break;
            }
        }
    }
    return;
}

void putblock8_8(unsigned char *vram, int vxsize, int pxsize, int pysize,
        int px0, int py0, unsigned char *buf, int bxsize){
    int x, y;
    for (y = 0; y < pysize; y++){
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + px0 + x] = buf[y * bxsize + x];
        }
    }
    return;
}

