AS = nasm
ASFLAGS = -f bin
CC = gcc
OBJECTS = bootpack.o io.o graphic.o int.o desctbl.o
CFLAGS = -c -m32 -nostartfiles -fno-stack-protector -ffreestanding

all: halfdogos.bin

run: halfdogos.bin
	qemu-system-x86_64 -fda boot.img
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

io.o:
	$(AS) -f elf32 io.asm -o io.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

bootpack.bin: $(OBJECTS)
	ld -Ttext 0xc600 -Tdata 0xe540 -melf_i386 $(OBJECTS) --oformat binary -o bootpack.bin

clean:
	rm -rf boot.img halfdogos.bin *.o *.elf
