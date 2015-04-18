AS = nasm
ASFLAGS = -f bin
CC = gcc
OBJECTS = bootpack.o io.o graphic.o int.o desctbl.o fifo.o keyboard.o mouse.o memory.o sheet.o timer.o libosdevc.a
CFLAGS = -c -m32 -nostartfiles -fno-stack-protector -ffreestanding -O0 -ggdb -Werror

all: halfdogos.bin tags

run: all
	qemu-system-x86_64 -m 32 -cpu coreduo -fda boot.img
	#bochs -f bochsrc.txt -q

boot.img: boot.asm
	$(AS) $(ASFLAGS) $< -o $@
	truncate -s 1474560 $@

halfdogos.bin: asmhead.asm boot.img bootpack.bin
	$(AS) $(ASFLAGS) asmhead.asm -o halfdogos.bin
	sudo mount -o loop boot.img fd
	sudo mv halfdogos.bin fd/
	sudo mv bootpack.bin fd/
	sudo umount fd/

tags:
	ctags -R

io.o:
	$(AS) -f elf32 io.asm -o io.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

bootpack.bin: $(OBJECTS)
	ld -T bootpack.ld -melf_i386 $(OBJECTS) --oformat binary -o bootpack.bin
	#ld -Ttext 0xc600 -Tdata 0xe540 -melf_i386 $(OBJECTS) --oformat binary -o bootpack.bin

clean:
	rm -rf boot.img halfdogos.bin *.o *.elf *.s
