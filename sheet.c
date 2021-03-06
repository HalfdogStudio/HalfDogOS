#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));  //9232字节
    if (ctl == 0){      // 返回0表示没空间了
        goto err;
    }
    // map 设定
    ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {
        // 如果map分配内存失败，释放ctl内存
        memman_free_4k(memman, (int) ctl, sizeof(struct SHTCTL));
        goto err;
    }
    // END map
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;      // 一个sheet也没有
    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;   //全标记为未使用
        ctl->sheets0[i].ctl = ctl;  //记录所属的ctl
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

void sheet_updown(struct SHEET *sht, int height){
    int h;
    int old = sht->height;
    struct SHTCTL *ctl = sht->ctl;
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
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1); //重绘界面
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old); //重绘界面
        } else {    //隐藏
            if (ctl->top > old) {   //如果old不再顶层
                // old上的都下移
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h]->height = h;
                }
            }   //在顶层就不用管,只ctl->top--就够了
            ctl->top--;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0); //重绘界面
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1); //重绘界面
        }
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
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }   //再就是=old了,什么也不做
    return;
}

void sheet_refresh(struct SHEET *sht,
        int bx0, int by0, int bx1, int by1){
    // 只更新sht中buf对应的范围bx-by内区域
    // 图层的上下关系没有变化
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0,
                sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0){
    //旧位置
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        // 重写map
        // 移动前
        sheet_refreshmap(sht->ctl, old_vx0, old_vy0,
                old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        // 移动后
        sheet_refreshmap(sht->ctl, vx0, vy0,
                vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        //更新实际vram
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0,
                old_vx0 + sht->bxsize, old_vy0 + sht->bysize,
                0, sht->height - 1);
        sheet_refreshsub(sht->ctl, vx0, vy0,
                vx0 + sht->bxsize, vy0 + sht->bysize,
                sht->height, sht->height);
    }
    return;
}

void sheet_free(struct SHEET *sht){
    if (sht->height >= 0) {
        sheet_updown(sht, -1); // 若显示则设为隐藏
    }
    sht->flags = 0;
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1){
    // h0->自h0以上刷新
    int h, bx, by, bx0, by0, bx1, by1, vx, vy;
    unsigned char *buf;
    unsigned char c;        //缓存图像信息方便判断
    unsigned char sid;
    unsigned char *vram = ctl->vram;
    unsigned char *map = ctl->map;
    struct SHEET *sht;
    //修正超出画面范围
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 = 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    for (h = h0; h <= h1; h++) {   //包含顶层
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;   //计算当前sid
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
                c = map[vy * ctl->xsize + vx]; //map v 不是b
                if (c == sid) {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                }
            }
        }
    }
    return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0){
    //参考之前的sheet_refreshsub
    // h0->自h0以上刷新
    int h, bx, by, bx0, by0, bx1, by1, vx, vy;
    unsigned char *buf;
    char c;                     //色彩缓冲区
    unsigned char sid;        // 图层号
    unsigned char *map = ctl->map;
    struct SHEET *sht;
    //修正超出画面范围
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 = 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    // END修正
    for (h = h0; h <= ctl->top; h++) {   //包含顶层
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;  //地址计算，相对于ctl->sheets[0]位置
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
                    map[vy * ctl->xsize + vx] = sid;
                }
            }
        }
    }
    return;
}
