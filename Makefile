AS = nasm
ASFLAGS = -f bin

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

bootpack.bin: bootpack.c io.o
	cc -c -m32 -ffreestanding -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs bootpack.c -o bootpack.o
	ld -Ttext 0xc600 -Tdata 0xe540 -melf_i386 bootpack.o io.o --oformat binary -o bootpack.bin

clean:
	rm -rf boot.img halfdogos.bin *.o *.elf
