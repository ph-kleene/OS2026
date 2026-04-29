CC = gcc
LD = ld
OBJCOPY = objcopy

CFLAGS = -m32 -march=i386 -static \
	 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	 -Wall -Werror -O2 -I../lib -g
LDFLAGS = -m elf_i386 

# Ensure start.o is the FIRST object linked
ENTRY_OBJ = start.o
UCFILES = $(shell find ./ -name "*.c")
LIB_DIR = ../lib
LIB = $(LIB_DIR)/myos.lib
UOBJS = $(ENTRY_OBJ) $(UCFILES:.c=.o)

app.bin: $(UOBJS) $(LIB)
	$(LD) $(LDFLAGS) -T app.ld -o uMain.elf $(UOBJS) $(LIB)
	$(OBJCOPY) -O binary uMain.elf app.bin
	@echo "OK: App binary is $$(wc -c < app.bin | tr -d ' ') bytes"

$(LIB):
	make -C $(LIB_DIR)

%.o: %.asm
	nasm -f elf32 $< -o $@

clean:
	find . -name "*.o" -delete
	rm -rf *.elf *.bin
	make -C $(LIB_DIR) clean
