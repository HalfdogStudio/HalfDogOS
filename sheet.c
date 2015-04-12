#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));  //9232字节
    if (ctl == 0){      // 返回0表示没空间了
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;      // 一个sheet也没有
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;   //全标记为未使用
    }
err:
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl){
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) {
        if(ctl->sheets0[i].flags == 0){      //未使用
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1;       //隐藏
            return sht;
        }
    }
    return 0;       //所有sheet都在使用
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf,
        int xsize, int ysize, int col_inv){
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height){
    int h;
    int old = sht->height;
    //修正height不超出边界
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht->height = height;
    //对sheets[]进行排列
    if (height < old) { //新位置比之前的低
        if (height >= 0){   //新位置没有隐藏
            //所有在height+1到old的图层上移
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h]->height = h;
            }
            // 把sht放到height位置
            ctl->sheets[height] = sht;
        } else {    //隐藏
            if (ctl->top > old) {   //如果old不再顶层
                // old上的都下移
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h]->height = h;
                }
            }   //在顶层就不用管,只ctl->top--就够了
            ctl->top--;
        }
        sheet_refresh(ctl, sht, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize); //重绘界面
    } else if (height > old) {  //新位置比老位置高
        if (old >= 0){          //老位置不是隐藏
            //把old+1到height下拉
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h+1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {        // old之前隐藏
            //把height上面的往上提
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h+1] = ctl->sheets[h];
                ctl->sheets[h+1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++; //更新top
        }
        sheet_refresh(ctl, sht, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }   //再就是=old了,什么也不做
    return;
}

void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht,
        int bx0, int by0, int bx1, int by1){
    // 只更新sht中buf对应的范围bx-by内区域
    if (sht->height >= 0) {
        sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0,
                sht->vx0 + bx1, sht->vy0 + by1);
    }
    return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0){
    //旧位置
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        sheet_refreshsub(ctl, old_vx0, old_vy0,
                old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        sheet_refreshsub(ctl, vx0, vy0,
                vx0 + sht->bxsize, vy0 + sht->bysize);
    }
    return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht){
    if (sht->height >= 0) {
        sheet_updown(ctl, sht, -1); // 若显示则设为隐藏
    }
    sht->flags = 0;
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1){
    int h, bx, by, bx0, by0, bx1, by1, vx, vy;
    unsigned char *buf;
    unsigned char c;        //缓存图像信息方便判断
    unsigned char *vram = ctl->vram;
    struct SHEET *sht;
    //修正超出画面范围
    for (h = 0; h <= ctl->top; h++) {   //包含顶层
        sht = ctl->sheets[h];
        buf = sht->buf;
        // 由vxy倒推bxy范围
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        //处理超出buf范围
        if (bx0 < 0) {
            bx0 = 0;
        }
        if (by0 < 0) {
            by0 = 0;
        }
        if (bx1 > sht->bxsize) {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
            by1 = sht->bysize;
        }
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                c = buf[by * sht->bxsize + bx];
                if (c != sht->col_inv) {
                    vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}
