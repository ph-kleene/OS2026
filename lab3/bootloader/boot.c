#include "boot.h"

#define KERNEL_START 0x10000

void bootMain(void) {
	// Load raw kernel binary (128 sectors = 64KB)
	for (int i = 0; i < 128; i++) {
		readSect((void*)(KERNEL_START + i*512), 1+i);
	}

	// Jump directly to kernel entry
	((void(*)(void))KERNEL_START)();
}
