CC = gcc
LD = ld
OBJCOPY = objcopy

CFLAGS = -m32 -march=i386 -static \
	 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	 -Wall -Werror -O2 -I./include -g
ASFLAGS = -m32
LDFLAGS = -m elf_i386 

# Ensure start.o is the FIRST object linked
ENTRY_OBJ = start.o
KCFILES = $(shell find ./ -name "*.c")
KSFILES = $(shell find ./ -name "*.S")
KASMFILES = $(shell find ./x86 ./device ./debug -name "*.asm")
KOBJS_ALL = $(KCFILES:.c=.o) $(KSFILES:.S=.o) $(KASMFILES:.asm=.o)
KOBJS = $(ENTRY_OBJ) $(filter-out $(ENTRY_OBJ) ./$(ENTRY_OBJ), $(KOBJS_ALL))

kernel.bin: $(KOBJS)
	$(LD) $(LDFLAGS) -T kernel.ld -o kMain.elf $(KOBJS)
	$(OBJCOPY) -O binary kMain.elf kernel.bin
	@echo "OK: Kernel binary is $$(wc -c < kernel.bin | tr -d ' ') bytes"

%.o: %.asm
	nasm -f elf32 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	find . -name "*.o" -delete
	rm -rf *.elf *.bin

