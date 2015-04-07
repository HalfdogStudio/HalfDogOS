#include "color.h"

// PIC寄存器
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1
// io.asm
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void write_mem8(int addr, int data);
void io_out8(int port, int data);
int io_in8(int port);
int io_load_eflags(void);
void io_store_eflags(int eflags);
// 图像相关
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c,
        int x0, int y0, int x1, int y1);
void init_screen8(unsigned char *vram, int xsize, int ysize);
void putfont8(unsigned char *vram, int xsize, int x, int y, unsigned char c, char *font);
void putfont8_asc(unsigned char *vram, int xsize, int x, int y, char c, unsigned char *s);
void putfont8_hex(unsigned char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(unsigned char *mouse, char bc);
void putblock8_8(unsigned char *vram, int vxsize, int pxsize, int pysize,
        int px0, int py0, unsigned char *buf, int bxsize);
// 键盘中断
#define PORT_KEYDAT 0x0060
struct KEYBUF {
    unsigned char data;     //缓冲数据
    unsigned char flag;     //是否已使用
};

struct KEYBUF keybuf;
// 启动信息
struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

//权限位, BOOTINFO
#define ADR_BOOTINFO 0x00000ff0
#define AR_INTGATE32 0x008e

// GDT和IDT
struct SEGMENT_DESCRIPTOR{
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
struct GATE_DESCRIPTOR{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, unsigned int offset, int selector, int ar);
void init_gdtidt(void);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
// init PIC
void init_pic(void);
void asm_inthandler21(void);
