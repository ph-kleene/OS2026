#include "common.h"
#include "x86.h"
#include "device.h"

int abort(const char *fname, int line) {
	disableInterrupt();
	
	kprintf("\n[KERNEL PANIC] Assertion failed at %s:%d\n", fname, line);
	
	// Also display on VGA if possible
	const char *msg = "KERNEL PANIC";
	volatile uint16_t *vga = (volatile uint16_t *)0xb8000;
	for (int i = 0; i < 80; i++) vga[i] = ' ' | (0x4f << 8); // Red background
	for (int i = 0; msg[i]; i++) vga[i] = msg[i] | (0x4f << 8);

	while (TRUE) {
		waitForInterrupt();
	}
}
