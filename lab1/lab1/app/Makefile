# Build app binary (comment out the following 3 lines if you are using macOS with M chip).
CC = gcc
LD = ld
OBJCOPY = objcopy

# Build app binary (uncomment the following 3 lines if you are using macOS with M chip).
#CC = x86_64-elf-gcc
#LD = x86_64-elf-ld
#OBJCOPY = x86_64-elf-objcopy

app.bin: start.asm io.asm main.c
	nasm -f elf32 start.asm -o start.o
	nasm -f elf32 io.asm -o io.o
	$(CC) -c -m32 -O1 -fno-stack-protector main.c -o main.o
	$(LD) -m elf_i386 -e start -Ttext 0x10000 start.o io.o main.o -o app.elf
	$(OBJCOPY) -S -j .text -j .rodata -O binary app.elf app.bin

# Remove object files
clean:
	rm -rf *.o *.elf *.bin
