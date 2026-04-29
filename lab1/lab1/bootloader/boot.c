#include "boot.h"

// Physical address to load the app module
#define APP_START 0x10000

// Number of sectors to read
#define SECTOR_COUNT 128

// Boot entry point
void bootMain(void) {
	/* TODO: 循环读取扇区
	 */
	int i;
	for (i = 0; i < SECTOR_COUNT; i++) {
		readSect((void *)(APP_START + i * 512), i + 1);
	}

	((void (*)(void))APP_START)();

}
