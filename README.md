HalfDog OS
----------

Just for fun

30天自制操作系统:)

Proudly brought by halfdog studio

REQUIREMENTS
------------

nasm: 汇编器
qemu: 虚拟机,不知道为啥
truncate: 生成软盘镜像文件大小

OBSTACLE
-----------

### 找不到启动软盘

软盘镜像有几个要求:

- 大小合适,这里是1.44M即`18*2*80*512=1474560`字节.

floppy.img是我用`bximage`生成个镜像,然后`fdisk floppy.img`分区,最后mkfs.fat格式化后,参照FAT12格式,把62字节之前的部分写到boot.asm中.

### 让IPL找到操作系统

把软盘镜像挂载,然后把操作系统保存进去,卸载,再用`hexdump -C`查看相应文件位置.

### IPL hangs

bochs很奇怪,我只能读`18*3+10`个扇区.让后就在BIOS中hang了.参照[http://note.youdao.com/share/?id=fc82ff38b404868cb7f8df2eec2e7bb7&type=note](http://note.youdao.com/share/?id=fc82ff38b404868cb7f8df2eec2e7bb7&type=note)

这时候用qemu就正常.
