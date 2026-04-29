QEMU = qemu-system-i386

help:
	@echo "Available targets:"
	@echo "  make os.img   - Build the OS image"
	@echo "  make run      - Run OS in QEMU"
	@echo "  make bochs    - Run OS in Bochs (Strict segment checks)"
	@echo "  make debug    - Run OS with GDB debugging support (GUI mode)"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make help     - Show this help message"

os.img:
	@cd bootloader; make
	@cd kernel; make
	@cd app; make
	@# Initialize 1MB empty image (enough for 2000 sectors)
	@dd if=/dev/zero of=os.img bs=512 count=2880 2>/dev/null
	@# Sector 0: Bootloader
	@dd if=bootloader/bootloader.bin of=os.img conv=notrunc 2>/dev/null
	@# Sector 1-128: Kernel (Max 64KB)
	@dd if=kernel/kernel.bin of=os.img seek=1 conv=notrunc 2>/dev/null
	@# Sector 129-256: App (Max 64KB)
	@dd if=app/app.bin of=os.img seek=129 conv=notrunc 2>/dev/null
	@echo "OK: os.img created. Kernel starts at sector 1, App starts at sector 129."

run: os.img
	$(QEMU) -drive format=raw,file=os.img -rtc base=localtime -serial stdio

bochs: os.img
	bochs -f bochsrc -q

debug: os.img
	$(QEMU) -drive format=raw,file=os.img -s -monitor stdio -rtc base=localtime

clean:
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd app; make clean
	rm -f os.img
	rm -f bochsout.txt
