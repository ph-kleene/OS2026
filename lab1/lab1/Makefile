# QEMU emulator
QEMU = qemu-system-i386

.PHONY: all clean run debug help

# Default target: build OS image
all: os.img

# Build OS image from bootloader and app
os.img:
	cd bootloader && make bootloader.bin
	cd app && make app.bin
	cat bootloader/bootloader.bin app/app.bin > os.img
	truncate -s 1474560 os.img

# Remove generated files
clean:
	cd bootloader && make clean
	cd app && make clean
	rm -f os.img

# Run QEMU with GUI
run: os.img
	$(QEMU) -rtc base=localtime -drive format=raw,file=os.img


# Run QEMU in debug mode
debug: os.img
	$(QEMU) -s -S -rtc base=localtime -drive format=raw,file=os.img

# Display help message
help:
	@echo "Available targets:"
	@echo "  all      - Build os.img (default)"
	@echo "  clean    - Remove all generated files"
	@echo "  run      - Run QEMU with GUI"
	@echo "  debug    - Run QEMU in debug mode (-s -S)"
	@echo "  help     - Show this help message"
