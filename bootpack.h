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
// 中断所需FIFO
struct FIFO8 {
    unsigned char *buf;     //缓冲数据
    int p;                  // 写指针
    int q;                  // 读指针
    int size;
    int free;
    int flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);
// 鼠标键盘
struct FIFO8 keyinfo;   // FIFO8结构体
struct FIFO8 mouseinfo;   // FIFO8结构体
#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

// 内存检查
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int load_cr0(void);
void store_cr0(unsigned int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);

// 内存管理
#define MEMMAN_FREES 4090   // 大约32kb
#define MEMMAN_ADDR 0x003c0000  //MEMMAN大概要32kb

struct FREEINFO {   // 可用信息
    unsigned int addr, size;
};

struct MEMMAN {
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

// 图层
// 图层信息结构体
struct SHEET {
    unsigned char *buf;     // 图层内容保存在这里
    int bxsize, bysize;     //图像大小(缓冲区)
    int vx0, vy0;           //图层位置
    int col_inv;            //透明色
    int height;             //图层高度
    int flags;              //是否在使用
    struct SHTCTL *ctl;
};
//图层信息管理结构体
#define MAX_SHEETS 256
struct SHTCTL {
    unsigned char * vram;   //vga缓冲区地址
    int xsize, ysize;
    int top;    //当前最高高度
    struct SHEET *sheets[MAX_SHEETS];   //地址数组,用来按高度排序图层
    struct SHEET sheets0[MAX_SHEETS];    //实际保存图层信息
};
// 初始化图层管理结构体
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
//取得新生成的未使用图层
#define SHEET_USE 1
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
// 设置图层缓冲区大小和透明色
void sheet_setbuf(struct SHEET *sht, unsigned char *buf,
        int xsize, int ysize, int col_inv);
// 设定图层高度
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht,
        int bx0, int by0, int bx1, int by1);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

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
void asm_inthandler2c(void);
