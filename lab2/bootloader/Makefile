# Build bootloader binary
CC = gcc
LD = ld
OBJCOPY = objcopy

bootloader.bin: start.asm io.asm boot.c boot.h
	nasm -f elf32 start.asm -o start.o
	nasm -f elf32 io.asm -o io.o
	$(CC) -c -m32 -O1 -fno-stack-protector boot.c -o boot.o
	$(LD) -m elf_i386 -e start -Ttext 0x7c00 start.o io.o boot.o -o bootloader.elf
	$(OBJCOPY) -S -j .text -O binary bootloader.elf bootloader.bin
	dd if=bootloader.bin of=bootloader.bin bs=512 count=1 conv=notrunc,sync 2>/dev/null
	/bin/bash -c 'echo -ne "\x55\xAA"' | dd of=bootloader.bin bs=1 count=2 seek=510 conv=notrunc 2>/dev/null

# Remove object files
clean:
	rm -rf *.o *.elf *.bin
