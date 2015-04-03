AS = nasm
ASFLAGS = -f bin

all: halfdogos.bin

run: halfdogos.bin
	qemu-system-x86_64 -fda boot.img
	#bochs -f bochsrc.txt -q

boot.img: boot.asm
	$(AS) $(ASFLAGS) $< -o $@
	truncate -s 1474560 $@

halfdogos.bin: asmhead.asm boot.img
	$(AS) $(ASFLAGS) asmhead.asm -o halfdogos.bin
	sudo mount -o loop boot.img fd
	sudo mv halfdogos.bin fd
	sudo umount fd


clean:
	rm -rf boot.img halfdogos.bin
