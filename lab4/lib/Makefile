CC = gcc
AR = ar

CFLAGS = -m32 -march=i386 -static \
	 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	 -Wall -Werror -O2 -g

CFILES = $(shell find ./ -name "*.c")
ASMFILES = $(shell find ./ -name "*.asm")
OBJS = $(CFILES:.c=.o) $(ASMFILES:.asm=.o)

myos.lib: $(OBJS)
	$(AR) rcs myos.lib $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf32 $< -o $@

clean:
	find . -name "*.o" -delete
	rm -rf myos.lib
